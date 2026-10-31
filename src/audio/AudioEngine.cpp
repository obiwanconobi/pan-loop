#include "AudioEngine.h"

#include <array>
#include <cmath>

namespace
{
    constexpr auto twoPi = juce::MathConstants<double>::twoPi;
    constexpr auto smoothingSeconds = 0.02;
    constexpr auto testToneLevel = 0.1f; // -20 dBFS
}

AudioEngine::AudioEngine()
    : liveLooper (transport)
{
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    const auto sampleRate = device->getCurrentSampleRate();

    currentSampleRate.store (sampleRate);
    currentBlockSize.store (device->getCurrentBufferSizeSamples());

    inputGainSmoother.reset (sampleRate, smoothingSeconds);
    outputGainSmoother.reset (sampleRate, smoothingSeconds);
    monitorGainSmoother.reset (sampleRate, smoothingSeconds);
    testToneGainSmoother.reset (sampleRate, smoothingSeconds);

    inputGainSmoother.setCurrentAndTargetValue (inputGainLinear.load (std::memory_order_relaxed));
    outputGainSmoother.setCurrentAndTargetValue (outputGainLinear.load (std::memory_order_relaxed));
    monitorGainSmoother.setCurrentAndTargetValue (monitoringEnabled.load (std::memory_order_relaxed) ? 1.0f : 0.0f);
    testToneGainSmoother.setCurrentAndTargetValue (0.0f);

    testTonePhase = 0.0;

    const auto rateChanged = ! enginePrepared
                          || std::abs (sampleRate - preparedSampleRate) > 0.5;

    if (rateChanged)
    {
        if (! playlist.isEmpty())
        {
            juce::Logger::writeToLog ("Sample rate changed; clearing backing playlist");
            playlist.clear();
        }

        if (liveLooper.hasLayers())
            juce::Logger::writeToLog ("Sample rate changed; clearing live loop layers");

        liveLooper.reset();

        transport.prepare (sampleRate);
        metronome.prepare (sampleRate);

        lastTransportState = Transport::State::Stopped;
        lastTransportPositionSamples = 0;
        nextBeatPosition = 0.0;
        lastSamplesPerBeat = 0.0;

        preparedSampleRate = sampleRate;
        enginePrepared = true;
    }
    else
    {
        // A JACK port or buffer change can restart the device. Keep the transport
        // and live layers running and only clear the click tail.
        metronome.reset();
    }

    mixBuffer.setSize (2, juce::jmax (8192, device->getCurrentBufferSizeSamples() * 2),
                       false, true, false);

    inputLatencySamples.store (device->getInputLatencyInSamples(), std::memory_order_relaxed);
    outputLatencySamples.store (device->getOutputLatencyInSamples(), std::memory_order_relaxed);

    liveLooper.prepare (sampleRate);
    updateLiveLatency();

    juce::Logger::writeToLog ("Audio device started: " + device->getName()
                              + " | " + juce::String (sampleRate, 1) + " Hz | "
                              + juce::String (device->getCurrentBufferSizeSamples()) + " samples | "
                              + juce::String (device->getActiveInputChannels().countNumberOfSetBits()) + " in / "
                              + juce::String (device->getActiveOutputChannels().countNumberOfSetBits()) + " out | "
                              + "latency " + juce::String (device->getInputLatencyInSamples())
                              + "+" + juce::String (device->getOutputLatencyInSamples()) + " samples");
}

void AudioEngine::audioDeviceStopped()
{
    juce::Logger::writeToLog ("Audio device stopped");
    currentSampleRate.store (0.0);
    currentBlockSize.store (0);
    inputLevel.store (0.0f);
    outputLevel.store (0.0f);
    inputLatencySamples.store (0, std::memory_order_relaxed);
    outputLatencySamples.store (0, std::memory_order_relaxed);
    testTonePhase = 0.0;
    metronome.reset();
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext&)
{
    // Always clear every output buffer first: the host does not guarantee silence.
    for (int channel = 0; channel < numOutputChannels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[channel], numSamples);

    const auto sampleRate = currentSampleRate.load (std::memory_order_relaxed);

    if (sampleRate <= 0.0 || numSamples <= 0)
        return;

    if (numSamples > mixBuffer.getNumSamples())
    {
        jassertfalse;
        return;
    }

    mixBuffer.clear (0, numSamples);

    const auto monitor = monitoringEnabled.load (std::memory_order_relaxed);
    const auto toneOn = testToneEnabled.load (std::memory_order_relaxed);
    const auto toneFrequency = testToneFrequency.load (std::memory_order_relaxed);

    inputGainSmoother.setTargetValue (inputGainLinear.load (std::memory_order_relaxed));
    outputGainSmoother.setTargetValue (outputGainLinear.load (std::memory_order_relaxed));
    monitorGainSmoother.setTargetValue (monitor ? 1.0f : 0.0f);
    testToneGainSmoother.setTargetValue (toneOn ? testToneLevel : 0.0f);
    metronome.beginBlock();

    const auto phaseIncrement = twoPi * juce::jlimit (20.0, sampleRate * 0.45, toneFrequency) / sampleRate;

    const auto transportState = transport.getState();
    const auto transportPlaying = transportState == Transport::State::Playing;
    const auto transportPositionSamples = transport.getPositionSamples();
    const auto transportPosition = static_cast<double> (transportPositionSamples);
    const auto samplesPerBeat = transport.getSamplesPerBeat();
    const auto beatsPerBar = juce::jmax (1, transport.getTimeSignatureNumerator());
    const auto cycleLengthSamples = transport.getCycleLengthSamples();

    if (transportPlaying && ! backingMuted.load (std::memory_order_relaxed))
        playlist.addTo (mixBuffer, 0, numSamples, transportPositionSamples);

    const auto liveResult = liveLooper.processBlock (mixBuffer,
                                                     inputChannelData,
                                                     numInputChannels,
                                                     numSamples,
                                                     transportPositionSamples,
                                                     cycleLengthSamples,
                                                     transportPlaying);

    if (samplesPerBeat > 0.0
        && std::abs (samplesPerBeat - lastSamplesPerBeat) > 1.0e-6)
    {
        lastSamplesPerBeat = samplesPerBeat;
        nextBeatPosition = transportPlaying
                             ? std::ceil (transportPosition / samplesPerBeat) * samplesPerBeat
                             : transportPosition;
    }

    // The metronome schedule is based on the transport sample position. After a
    // playlist cycle wrap the position jumps back to zero, so restart the click
    // schedule at the wrapped position.
    if (cycleLengthSamples > 0 && transportPositionSamples < lastTransportPositionSamples)
        nextBeatPosition = transportPosition;

    lastTransportPositionSamples = transportPositionSamples;

    if (transportPlaying && lastTransportState != Transport::State::Playing
        && samplesPerBeat > 0.0)
    {
        nextBeatPosition = transportPosition <= 0.0
                             ? 0.0
                             : std::ceil (transportPosition / samplesPerBeat) * samplesPerBeat;
    }
    else if (! transportPlaying)
    {
        nextBeatPosition = transportPosition;
    }

    lastTransportState = transportState;

    constexpr int maxClicksPerBlock = 8;
    std::array<int, maxClicksPerBlock> clickOffsets {};
    std::array<bool, maxClicksPerBlock> clickAccents {};
    int numClicks = 0;

    if (transportPlaying && samplesPerBeat > 0.0)
    {
        if (nextBeatPosition < transportPosition - 0.5)
            nextBeatPosition = std::ceil (transportPosition / samplesPerBeat) * samplesPerBeat;

        const auto blockEnd = transportPosition + static_cast<double> (numSamples);

        while (nextBeatPosition < blockEnd && numClicks < maxClicksPerBlock)
        {
            auto offset = static_cast<int> (std::llround (nextBeatPosition - transportPosition));
            offset = juce::jlimit (0, numSamples - 1, offset);

            const auto beatIndex = static_cast<juce::int64> (std::llround (nextBeatPosition / samplesPerBeat));
            const auto clickIndex = static_cast<std::size_t> (numClicks);
            clickOffsets[clickIndex] = offset;
            clickAccents[clickIndex] = (beatIndex % beatsPerBar == 0);
            ++numClicks;

            nextBeatPosition += samplesPerBeat;
        }
    }

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;
    bool clipped = false;

    const auto* inputL = numInputChannels > 0 ? inputChannelData[0] : nullptr;
    const auto* inputR = numInputChannels > 1 ? inputChannelData[1] : nullptr;

    auto* outputL = numOutputChannels > 0 ? outputChannelData[0] : nullptr;
    auto* outputR = numOutputChannels > 1 ? outputChannelData[1] : nullptr;

    int nextClickIndex = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        while (nextClickIndex < numClicks
               && clickOffsets[static_cast<std::size_t> (nextClickIndex)] == i)
        {
            metronome.trigger (clickAccents[static_cast<std::size_t> (nextClickIndex)]);
            ++nextClickIndex;
        }

        const auto inL = inputL != nullptr ? inputL[i] : 0.0f;
        const auto inR = inputR != nullptr ? inputR[i] : inL;

        inputPeak = juce::jmax (inputPeak, juce::jmax (std::abs (inL), std::abs (inR)));

        const auto monitorGain = monitorGainSmoother.getNextValue();
        const auto inputGain = inputGainSmoother.getNextValue();
        const auto outputGain = outputGainSmoother.getNextValue();
        const auto toneGain = testToneGainSmoother.getNextValue();

        auto mixL = mixBuffer.getSample (0, i) + inL * inputGain * monitorGain;
        auto mixR = mixBuffer.getSample (1, i) + inR * inputGain * monitorGain;

        if (toneGain > 0.0f)
        {
            const auto tone = static_cast<float> (std::sin (testTonePhase)) * toneGain;
            mixL += tone;
            mixR += tone;

            testTonePhase += phaseIncrement;
            if (testTonePhase >= twoPi)
                testTonePhase -= twoPi;
        }

        const auto clickSample = metronome.getNextSample();
        mixL += clickSample;
        mixR += clickSample;

        mixL *= outputGain;
        mixR *= outputGain;

        if (std::abs (mixL) > 1.0f || std::abs (mixR) > 1.0f)
            clipped = true;

        outputPeak = juce::jmax (outputPeak, juce::jmax (std::abs (mixL), std::abs (mixR)));

        if (outputL != nullptr)
            outputL[i] = juce::jlimit (-1.0f, 1.0f, mixL);

        if (outputR != nullptr)
            outputR[i] = juce::jlimit (-1.0f, 1.0f, mixR);
    }

    // Extra outputs stay silent; extra inputs are ignored in this phase.
    inputLevel.store (inputPeak, std::memory_order_relaxed);
    outputLevel.store (outputPeak, std::memory_order_relaxed);

    if (liveResult == LiveLooper::ProcessResult::ResetTransport)
    {
        transport.setPositionSamples (0);
    }
    else if (liveResult == LiveLooper::ProcessResult::StopTransport)
    {
        transport.stop();
        transport.setCycleLengthSamples (0);
    }
    else
    {
        transport.advance (numSamples);
    }

    if (clipped)
        clipIndicator.store (true, std::memory_order_relaxed);
}

void AudioEngine::audioDeviceError (const juce::String& errorMessage)
{
    juce::Logger::writeToLog ("Audio device error: " + errorMessage);
}

void AudioEngine::playlistChanged()
{
    const auto totalLength = playlist.getTotalLengthSamples();

    if (totalLength > 0)
    {
        if (liveLooper.hasLayers() && transport.getCycleLengthSamples() > 0)
        {
            juce::Logger::writeToLog ("Playlist cycle change ignored while live layers exist");
            return;
        }

        transport.setCycleLengthSamples (totalLength);
    }
    else if (! liveLooper.hasLayers())
    {
        transport.setCycleLengthSamples (0);
    }

    juce::Logger::writeToLog ("Playlist changed: "
                              + juce::String (totalLength) + " samples, cycle "
                              + juce::String (transport.getCycleLengthSamples()) + " samples");
}

void AudioEngine::record()
{
    if (liveLooper.isRecordingOrArmed())
        return;

    const auto sampleRate = getCurrentSampleRate();

    if (sampleRate <= 0.0)
    {
        juce::Logger::writeToLog ("Record ignored: no audio device");
        return;
    }

    auto cycleLength = transport.getCycleLengthSamples();

    if (cycleLength <= 0 && ! playlist.isEmpty())
    {
        cycleLength = playlist.getTotalLengthSamples();

        if (cycleLength > 0)
            transport.setCycleLengthSamples (cycleLength);
    }

    if (cycleLength > 0)
    {
        const auto startImmediately = ! transport.isPlaying();

        if (startImmediately)
        {
            liveLooper.startOverdub (cycleLength, sampleRate, true);
            juce::Logger::writeToLog ("Live overdub recording started immediately, cycle "
                                      + juce::String (cycleLength) + " samples");

            if (liveLooper.isRecording())
            {
                transport.setPositionSamples (0);
                transport.play();
            }
        }
        else
        {
            liveLooper.startOverdub (cycleLength, sampleRate, false);
            juce::Logger::writeToLog ("Live overdub armed for next cycle, cycle "
                                      + juce::String (cycleLength) + " samples");
        }
    }
    else
    {
        liveLooper.startFirstLoop (sampleRate);
        juce::Logger::writeToLog ("Live first-loop recording started");

        if (liveLooper.isRecording())
        {
            transport.stop();
            transport.setCycleLengthSamples (0);
            transport.play();
        }
    }
}

void AudioEngine::stopRecording()
{
    juce::Logger::writeToLog ("Live recording stop requested");
    liveLooper.stopRecording();
}

void AudioEngine::discardLiveLayer()
{
    juce::Logger::writeToLog ("Live layer discard requested");

    if (! liveLooper.discard())
        return;

    if (playlist.isEmpty() && ! liveLooper.hasLayers())
    {
        transport.stop();
        transport.setCycleLengthSamples (0);
    }
}

void AudioEngine::clearLiveLayers()
{
    liveLooper.clearLayers();

    if (playlist.isEmpty())
    {
        transport.stop();
        transport.setCycleLengthSamples (0);
    }
}

bool AudioEngine::removeLiveLayer (int layerId)
{
    if (! liveLooper.removeLayer (layerId))
        return false;

    if (playlist.isEmpty() && ! liveLooper.hasLayers())
    {
        transport.stop();
        transport.setCycleLengthSamples (0);
    }

    return true;
}

bool AudioEngine::hasLiveLayers() const
{
    return liveLooper.hasLayers();
}

void AudioEngine::setBarSnapEnabled (bool shouldSnap)
{
    liveLooper.setBarSnapEnabled (shouldSnap);
}

bool AudioEngine::isBarSnapEnabled() const
{
    return liveLooper.isBarSnapEnabled();
}

void AudioEngine::setLatencyOffsetMs (float milliseconds)
{
    latencyOffsetMs.store (juce::jlimit (-200.0f, 200.0f, milliseconds),
                           std::memory_order_relaxed);
    updateLiveLatency();
}

float AudioEngine::getLatencyOffsetMs() const noexcept
{
    return latencyOffsetMs.load (std::memory_order_relaxed);
}

int AudioEngine::getRecordingLatencySamples() const noexcept
{
    const auto sampleRate = getCurrentSampleRate();

    if (sampleRate <= 0.0)
        return 0;

    const auto offsetSamples = static_cast<int> (
        std::llround (latencyOffsetMs.load (std::memory_order_relaxed)
                      * static_cast<float> (sampleRate) / 1000.0f));

    return juce::jmax (0,
                       inputLatencySamples.load (std::memory_order_relaxed)
                       + outputLatencySamples.load (std::memory_order_relaxed)
                       + offsetSamples);
}

void AudioEngine::updateLiveLatency() noexcept
{
    liveLooper.setLatencySamples (getRecordingLatencySamples());
}

void AudioEngine::setInputGainDb (float db) noexcept
{
    inputGainLinear.store (juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 12.0f, db)),
                           std::memory_order_relaxed);
}

void AudioEngine::setOutputGainDb (float db) noexcept
{
    outputGainLinear.store (juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 12.0f, db)),
                            std::memory_order_relaxed);
}

void AudioEngine::setMonitoringEnabled (bool shouldMonitor) noexcept
{
    monitoringEnabled.store (shouldMonitor, std::memory_order_relaxed);
}

bool AudioEngine::isMonitoringEnabled() const noexcept
{
    return monitoringEnabled.load (std::memory_order_relaxed);
}

void AudioEngine::toggleInputMonitoring() noexcept
{
    setMonitoringEnabled (! isMonitoringEnabled());
}

void AudioEngine::setBackingMuted (bool muted) noexcept
{
    backingMuted.store (muted, std::memory_order_relaxed);
}

bool AudioEngine::isBackingMuted() const noexcept
{
    return backingMuted.load (std::memory_order_relaxed);
}

void AudioEngine::toggleBackingMute() noexcept
{
    setBackingMuted (! isBackingMuted());
}

void AudioEngine::setTestToneEnabled (bool shouldPlayTone) noexcept
{
    testToneEnabled.store (shouldPlayTone, std::memory_order_relaxed);
}

void AudioEngine::setTestToneFrequency (double hz) noexcept
{
    testToneFrequency.store (juce::jlimit (20.0, 20000.0, hz), std::memory_order_relaxed);
}

float AudioEngine::getInputLevel() const noexcept
{
    return inputLevel.load (std::memory_order_relaxed);
}

float AudioEngine::getOutputLevel() const noexcept
{
    return outputLevel.load (std::memory_order_relaxed);
}

float AudioEngine::getOutputGainLinear() const noexcept
{
    return outputGainLinear.load (std::memory_order_relaxed);
}

double AudioEngine::getCurrentSampleRate() const noexcept
{
    return currentSampleRate.load (std::memory_order_relaxed);
}

int AudioEngine::getCurrentBlockSize() const noexcept
{
    return currentBlockSize.load (std::memory_order_relaxed);
}

int AudioEngine::getInputLatencySamples() const noexcept
{
    return inputLatencySamples.load (std::memory_order_relaxed);
}

int AudioEngine::getOutputLatencySamples() const noexcept
{
    return outputLatencySamples.load (std::memory_order_relaxed);
}

double AudioEngine::getRoundTripLatencyMs() const noexcept
{
    const auto rate = getCurrentSampleRate();

    if (rate <= 0.0)
        return 0.0;

    const auto samples = getInputLatencySamples() + getOutputLatencySamples();
    return static_cast<double> (samples) * 1000.0 / rate;
}

bool AudioEngine::hasClipped() const noexcept
{
    return clipIndicator.load (std::memory_order_relaxed);
}

void AudioEngine::resetClipIndicator() noexcept
{
    clipIndicator.store (false, std::memory_order_relaxed);
}

#include "AudioEngine.h"

namespace
{
    constexpr auto twoPi = juce::MathConstants<double>::twoPi;
    constexpr auto smoothingSeconds = 0.02;
    constexpr auto testToneLevel = 0.1f; // -20 dBFS
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

    juce::Logger::writeToLog ("Audio device started: " + device->getName()
                              + " | " + juce::String (sampleRate, 1) + " Hz | "
                              + juce::String (device->getCurrentBufferSizeSamples()) + " samples | "
                              + juce::String (device->getActiveInputChannels().countNumberOfSetBits()) + " in / "
                              + juce::String (device->getActiveOutputChannels().countNumberOfSetBits()) + " out");
}

void AudioEngine::audioDeviceStopped()
{
    juce::Logger::writeToLog ("Audio device stopped");
    currentSampleRate.store (0.0);
    currentBlockSize.store (0);
    inputLevel.store (0.0f);
    outputLevel.store (0.0f);
    testTonePhase = 0.0;
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

    const auto monitor = monitoringEnabled.load (std::memory_order_relaxed);
    const auto toneOn = testToneEnabled.load (std::memory_order_relaxed);
    const auto toneFrequency = testToneFrequency.load (std::memory_order_relaxed);

    inputGainSmoother.setTargetValue (inputGainLinear.load (std::memory_order_relaxed));
    outputGainSmoother.setTargetValue (outputGainLinear.load (std::memory_order_relaxed));
    monitorGainSmoother.setTargetValue (monitor ? 1.0f : 0.0f);
    testToneGainSmoother.setTargetValue (toneOn ? testToneLevel : 0.0f);

    const auto phaseIncrement = twoPi * juce::jlimit (20.0, sampleRate * 0.45, toneFrequency) / sampleRate;

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;
    bool clipped = false;

    const auto* inputL = numInputChannels > 0 ? inputChannelData[0] : nullptr;
    const auto* inputR = numInputChannels > 1 ? inputChannelData[1] : nullptr;

    auto* outputL = numOutputChannels > 0 ? outputChannelData[0] : nullptr;
    auto* outputR = numOutputChannels > 1 ? outputChannelData[1] : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto inL = inputL != nullptr ? inputL[i] : 0.0f;
        const auto inR = inputR != nullptr ? inputR[i] : inL;

        inputPeak = juce::jmax (inputPeak, juce::jmax (std::abs (inL), std::abs (inR)));

        const auto monitorGain = monitorGainSmoother.getNextValue();
        const auto inputGain = inputGainSmoother.getNextValue();
        const auto outputGain = outputGainSmoother.getNextValue();
        const auto toneGain = testToneGainSmoother.getNextValue();

        auto mixL = inL * inputGain * monitorGain;
        auto mixR = inR * inputGain * monitorGain;

        if (toneGain > 0.0f)
        {
            const auto tone = static_cast<float> (std::sin (testTonePhase)) * toneGain;
            mixL += tone;
            mixR += tone;

            testTonePhase += phaseIncrement;
            if (testTonePhase >= twoPi)
                testTonePhase -= twoPi;
        }

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

    if (clipped)
        clipIndicator.store (true, std::memory_order_relaxed);
}

void AudioEngine::audioDeviceError (const juce::String& errorMessage)
{
    juce::Logger::writeToLog ("Audio device error: " + errorMessage);
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

double AudioEngine::getCurrentSampleRate() const noexcept
{
    return currentSampleRate.load (std::memory_order_relaxed);
}

int AudioEngine::getCurrentBlockSize() const noexcept
{
    return currentBlockSize.load (std::memory_order_relaxed);
}

bool AudioEngine::hasClipped() const noexcept
{
    return clipIndicator.load (std::memory_order_relaxed);
}

void AudioEngine::resetClipIndicator() noexcept
{
    clipIndicator.store (false, std::memory_order_relaxed);
}

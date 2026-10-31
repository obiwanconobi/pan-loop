#include "LiveLooper.h"

#include <algorithm>
#include <cmath>
#include <limits>

LiveLooper::LiveLooper (Transport& transportToUse)
    : transport (transportToUse)
{
}

void LiveLooper::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    maxFirstLoopSamples = juce::jmax (1, static_cast<int> (std::llround (sampleRate * maxFirstLoopSeconds)));
}

void LiveLooper::reset()
{
    const juce::ScopedWriteLock sl (lock);

    layers.clear();
    activeLayer = nullptr;
    state.store (State::Idle, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = 0;
    discardRemaining = 0;
    firstLoopTargetSamples = 0;
    nextLayerId = 1;
}

LiveLooper::Snapshot LiveLooper::getSnapshot() const
{
    const juce::ScopedReadLock sl (lock);

    Snapshot snapshot;

    for (const auto& layer : layers)
    {
        if (layer == nullptr)
            continue;

        const auto layerState = layer->state.load (std::memory_order_relaxed);

        if (layerState == LiveLayer::State::Empty)
            continue;

        const auto armed = layerState == LiveLayer::State::Armed;
        const auto recording = layerState == LiveLayer::State::Recording;
        const auto recorded = layer->recordedSamples.load (std::memory_order_relaxed);
        const auto usable = layer->usableLengthSamples.load (std::memory_order_relaxed);

        LayerInfo info;
        info.id = layer->id;
        info.name = layer->name;
        info.gain = layer->gain.load (std::memory_order_relaxed);
        info.pan = layer->pan.load (std::memory_order_relaxed);
        info.muted = layer->muted.load (std::memory_order_relaxed);
        info.solo = layer->solo.load (std::memory_order_relaxed);
        info.armed = armed;
        info.recording = recording;
        info.durationSeconds = armed
                                 ? 0.0
                                 : (recording
                                      ? (layer->sampleRate > 0.0 ? recorded / layer->sampleRate : 0.0)
                                      : (layer->sampleRate > 0.0 ? usable / layer->sampleRate : 0.0));
        info.memoryUsageBytes = layer->getMemoryUsageBytes();

        snapshot.layers.push_back (std::move (info));
        snapshot.totalMemoryUsageBytes += layer->getMemoryUsageBytes();

        if (layer->solo.load (std::memory_order_relaxed))
            snapshot.hasSolo = true;

        if ((armed || recording) && layer.get() == activeLayer)
            snapshot.recordingLayerId = layer->id;
    }

    return snapshot;
}

std::vector<LiveLooper::SavedLayer> LiveLooper::copyLayersForSave() const
{
    const juce::ScopedReadLock sl (lock);

    std::vector<SavedLayer> result;

    for (const auto& layer : layers)
    {
        if (layer == nullptr
            || layer->state.load (std::memory_order_relaxed) != LiveLayer::State::Playing)
            continue;

        const auto usable = juce::jlimit (0,
                                          layer->audio.getNumSamples(),
                                          layer->usableLengthSamples.load (std::memory_order_relaxed));

        if (usable <= 0)
            continue;

        SavedLayer saved;
        saved.name = layer->name;
        saved.gain = layer->gain.load (std::memory_order_relaxed);
        saved.pan = layer->pan.load (std::memory_order_relaxed);
        saved.muted = layer->muted.load (std::memory_order_relaxed);
        saved.solo = layer->solo.load (std::memory_order_relaxed);
        saved.audio.setSize (2, usable, false, true, false);

        for (int channel = 0; channel < 2; ++channel)
        {
            const auto sourceChannel = juce::jmin (channel, layer->audio.getNumChannels() - 1);
            saved.audio.copyFrom (channel, 0,
                                  layer->audio, sourceChannel, 0,
                                  usable);
        }

        result.push_back (std::move (saved));
    }

    return result;
}

bool LiveLooper::restoreLayers (std::vector<SavedLayer> savedLayers, juce::int64 cycleLengthSamples)
{
    if (cycleLengthSamples <= 0
        || cycleLengthSamples > static_cast<juce::int64> (std::numeric_limits<int>::max()))
        return false;

    const auto cycleLength = static_cast<int> (cycleLengthSamples);

    const juce::ScopedWriteLock sl (lock);

    layers.clear();
    activeLayer = nullptr;
    state.store (State::Idle, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = 0;
    discardRemaining = 0;
    firstLoopTargetSamples = 0;
    nextLayerId = 1;

    for (auto& saved : savedLayers)
    {
        if (saved.audio.getNumSamples() <= 0 || saved.audio.getNumChannels() <= 0)
            continue;

        auto layer = std::make_shared<LiveLayer>();
        layer->id = nextLayerId++;
        layer->name = saved.name.isNotEmpty() ? saved.name
                                              : "Layer " + juce::String (layer->id);
        layer->sampleRate = sampleRate;
        layer->gain.store (juce::jlimit (0.0f, 4.0f, saved.gain), std::memory_order_relaxed);
        layer->pan.store (juce::jlimit (-1.0f, 1.0f, saved.pan), std::memory_order_relaxed);
        layer->muted.store (saved.muted, std::memory_order_relaxed);
        layer->solo.store (saved.solo, std::memory_order_relaxed);
        layer->usableLengthSamples.store (cycleLength, std::memory_order_relaxed);
        layer->recordedSamples.store (cycleLength, std::memory_order_relaxed);

        layer->audio.setSize (2, cycleLength, false, true, false);
        layer->audio.clear();

        const auto copyLength = juce::jmin (cycleLength, saved.audio.getNumSamples());

        for (int channel = 0; channel < 2; ++channel)
        {
            const auto sourceChannel = juce::jmin (channel, saved.audio.getNumChannels() - 1);
            layer->audio.copyFrom (channel, 0,
                                   saved.audio, sourceChannel, 0,
                                   copyLength);
        }

        layer->state.store (LiveLayer::State::Playing, std::memory_order_relaxed);
        layers.push_back (std::move (layer));
    }

    return ! layers.empty();
}

bool LiveLooper::hasLayers() const
{
    const juce::ScopedReadLock sl (lock);
    return ! layers.empty();
}

int LiveLooper::getLayerCount() const
{
    const juce::ScopedReadLock sl (lock);
    return static_cast<int> (layers.size());
}

void LiveLooper::startFirstLoop (double newSampleRate)
{
    prepare (newSampleRate);

    if (maxFirstLoopSamples <= 0)
        return;

    auto layer = std::make_shared<LiveLayer>();
    layer->sampleRate = sampleRate;
    layer->audio.setSize (2, maxFirstLoopSamples, false, true, false);
    layer->audio.clear();
    layer->state.store (LiveLayer::State::Recording, std::memory_order_relaxed);
    layer->usableLengthSamples.store (0, std::memory_order_relaxed);
    layer->recordedSamples.store (0, std::memory_order_relaxed);

    const juce::ScopedWriteLock sl (lock);

    if (state.load (std::memory_order_relaxed) != State::Idle || ! layers.empty())
        return;

    layer->id = nextLayerId++;
    layer->name = "Layer " + juce::String (layer->id);

    layers.push_back (layer);
    activeLayer = layer.get();

    state.store (State::Recording, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = 0;
    discardRemaining = latencySamples.load (std::memory_order_relaxed);
    firstLoopTargetSamples = 0;
}

void LiveLooper::startOverdub (juce::int64 cycleLengthSamples,
                               double newSampleRate,
                               bool startImmediately)
{
    if (cycleLengthSamples <= 0
        || cycleLengthSamples > static_cast<juce::int64> (std::numeric_limits<int>::max()))
        return;

    prepare (newSampleRate);

    const auto length = static_cast<int> (cycleLengthSamples);

    auto layer = std::make_shared<LiveLayer>();
    layer->sampleRate = sampleRate;
    layer->audio.setSize (2, length, false, true, false);
    layer->audio.clear();
    layer->state.store (startImmediately ? LiveLayer::State::Recording
                                         : LiveLayer::State::Armed,
                        std::memory_order_relaxed);
    layer->usableLengthSamples.store (length, std::memory_order_relaxed);
    layer->recordedSamples.store (0, std::memory_order_relaxed);

    const juce::ScopedWriteLock sl (lock);

    if (state.load (std::memory_order_relaxed) != State::Idle)
        return;

    layer->id = nextLayerId++;
    layer->name = "Layer " + juce::String (layer->id);

    layers.push_back (layer);
    activeLayer = layer.get();

    state.store (startImmediately ? State::Recording : State::Armed,
                 std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = 0;
    discardRemaining = latencySamples.load (std::memory_order_relaxed);
    firstLoopTargetSamples = 0;
}

void LiveLooper::stopRecording()
{
    const juce::ScopedWriteLock sl (lock);
    const auto currentState = state.load (std::memory_order_relaxed);

    if (currentState == State::Armed)
    {
        if (activeLayer != nullptr)
        {
            layers.erase (std::remove_if (layers.begin(), layers.end(),
                                          [this] (const auto& layer)
                                          {
                                              return layer.get() == activeLayer;
                                          }),
                          layers.end());
        }

        activeLayer = nullptr;
        state.store (State::Idle, std::memory_order_relaxed);
        stopRequested.store (false, std::memory_order_relaxed);
        recordedSamples = 0;
        discardRemaining = 0;
        firstLoopTargetSamples = 0;
    }
    else if (currentState == State::Recording)
    {
        stopRequested.store (true, std::memory_order_relaxed);
    }
}

bool LiveLooper::discard()
{
    const juce::ScopedWriteLock sl (lock);
    const auto currentState = state.load (std::memory_order_relaxed);

    if (currentState == State::Armed || currentState == State::Recording)
    {
        if (activeLayer != nullptr)
        {
            layers.erase (std::remove_if (layers.begin(), layers.end(),
                                          [this] (const auto& layer)
                                          {
                                              return layer.get() == activeLayer;
                                          }),
                          layers.end());
        }

        activeLayer = nullptr;
        state.store (State::Idle, std::memory_order_relaxed);
        stopRequested.store (false, std::memory_order_relaxed);
        recordedSamples = 0;
        discardRemaining = 0;
        firstLoopTargetSamples = 0;
        return true;
    }

    if (! layers.empty())
    {
        if (activeLayer == layers.back().get())
            activeLayer = nullptr;

        layers.pop_back();
        return true;
    }

    return false;
}

bool LiveLooper::removeLayer (int layerId)
{
    const juce::ScopedWriteLock sl (lock);
    const auto index = std::find_if (layers.begin(), layers.end(),
                                     [layerId] (const auto& layer)
                                     {
                                         return layer != nullptr && layer->id == layerId;
                                     });

    if (index == layers.end())
        return false;

    if ((*index).get() == activeLayer)
    {
        state.store (State::Idle, std::memory_order_relaxed);
        stopRequested.store (false, std::memory_order_relaxed);
        activeLayer = nullptr;
        recordedSamples = 0;
        discardRemaining = 0;
        firstLoopTargetSamples = 0;
    }

    layers.erase (index);
    return true;
}

void LiveLooper::clearLayers()
{
    const juce::ScopedWriteLock sl (lock);

    layers.clear();
    activeLayer = nullptr;
    state.store (State::Idle, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = 0;
    discardRemaining = 0;
    firstLoopTargetSamples = 0;
    nextLayerId = 1;
}

void LiveLooper::reapEmptyLayers()
{
    const juce::ScopedWriteLock sl (lock);

    layers.erase (std::remove_if (layers.begin(), layers.end(),
                                  [] (const auto& layer)
                                  {
                                      return layer == nullptr
                                          || layer->state.load (std::memory_order_relaxed) == LiveLayer::State::Empty;
                                  }),
                  layers.end());

    if (activeLayer != nullptr)
    {
        const auto stillPresent = std::any_of (layers.begin(), layers.end(),
                                               [this] (const auto& layer)
                                               {
                                                   return layer.get() == activeLayer;
                                               });

        if (! stillPresent)
            activeLayer = nullptr;
    }
}

void LiveLooper::trimFinishedLayers()
{
    struct Trim
    {
        int layerId = 0;
        juce::AudioBuffer<float> audio;
    };

    std::vector<Trim> trims;

    {
        const juce::ScopedReadLock sl (lock);

        for (const auto& layer : layers)
        {
            if (layer == nullptr
                || layer->state.load (std::memory_order_relaxed) != LiveLayer::State::Playing)
                continue;

            if (! layer->trimRequested.exchange (false, std::memory_order_relaxed))
                continue;

            const auto usable = layer->usableLengthSamples.load (std::memory_order_relaxed);

            if (usable <= 0 || usable >= layer->audio.getNumSamples())
                continue;

            Trim trim;
            trim.layerId = layer->id;
            trim.audio.setSize (layer->audio.getNumChannels(), usable, false, true, false);

            for (int channel = 0; channel < trim.audio.getNumChannels(); ++channel)
                trim.audio.copyFrom (channel, 0,
                                     layer->audio, channel, 0,
                                     usable);

            trims.push_back (std::move (trim));
        }
    }

    if (trims.empty())
        return;

    std::vector<juce::AudioBuffer<float>> replacedBuffers;

    {
        const juce::ScopedWriteLock sl (lock);

        for (auto& trim : trims)
        {
            if (auto* layer = findLayerUnderLock (trim.layerId))
            {
                if (layer->state.load (std::memory_order_relaxed) == LiveLayer::State::Playing
                    && layer->usableLengthSamples.load (std::memory_order_relaxed) == trim.audio.getNumSamples())
                {
                    replacedBuffers.push_back (std::move (layer->audio));
                    layer->audio = std::move (trim.audio);
                }
            }
        }
    }
}

void LiveLooper::setLayerGain (int layerId, float gain)
{
    const juce::ScopedReadLock sl (lock);

    if (auto* layer = findLayerUnderLock (layerId))
        layer->gain.store (juce::jlimit (0.0f, 4.0f, gain), std::memory_order_relaxed);
}

void LiveLooper::setLayerPan (int layerId, float pan)
{
    const juce::ScopedReadLock sl (lock);

    if (auto* layer = findLayerUnderLock (layerId))
        layer->pan.store (juce::jlimit (-1.0f, 1.0f, pan), std::memory_order_relaxed);
}

void LiveLooper::setLayerMuted (int layerId, bool muted)
{
    const juce::ScopedReadLock sl (lock);

    if (auto* layer = findLayerUnderLock (layerId))
        layer->muted.store (muted, std::memory_order_relaxed);
}

void LiveLooper::setLayerSolo (int layerId, bool solo)
{
    const juce::ScopedReadLock sl (lock);

    if (auto* layer = findLayerUnderLock (layerId))
        layer->solo.store (solo, std::memory_order_relaxed);
}

void LiveLooper::setLatencySamples (int samples) noexcept
{
    latencySamples.store (juce::jmax (0, samples), std::memory_order_relaxed);
}

void LiveLooper::setBarSnapEnabled (bool shouldSnap) noexcept
{
    barSnapEnabled.store (shouldSnap, std::memory_order_relaxed);
}

bool LiveLooper::isBarSnapEnabled() const noexcept
{
    return barSnapEnabled.load (std::memory_order_relaxed);
}

bool LiveLooper::isArmed() const noexcept
{
    return state.load (std::memory_order_relaxed) == State::Armed;
}

bool LiveLooper::isRecording() const noexcept
{
    return state.load (std::memory_order_relaxed) == State::Recording;
}

bool LiveLooper::isRecordingOrArmed() const noexcept
{
    const auto currentState = state.load (std::memory_order_relaxed);
    return currentState == State::Armed || currentState == State::Recording;
}

LiveLooper::ProcessResult LiveLooper::processBlock (juce::AudioBuffer<float>& mix,
                                                    const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    int numSamples,
                                                    juce::int64 transportPosition,
                                                    juce::int64 cycleLength,
                                                    bool transportPlaying) noexcept
{
    const juce::ScopedReadLock sl (lock);

    if (transportPlaying)
        addLayersToMix (mix, numSamples, transportPosition, cycleLength);

    if (! transportPlaying)
        return ProcessResult::None;

    auto currentState = state.load (std::memory_order_relaxed);

    if (currentState == State::Idle)
        return ProcessResult::None;

    if (currentState == State::Recording
        && stopRequested.exchange (false, std::memory_order_relaxed))
    {
        if (cycleLength <= 0)
        {
            const auto target = computeFirstLoopTarget();

            if (target <= 0)
            {
                if (activeLayer != nullptr)
                {
                    activeLayer->state.store (LiveLayer::State::Empty, std::memory_order_relaxed);
                    activeLayer->recordedSamples.store (0, std::memory_order_relaxed);
                }

                state.store (State::Idle, std::memory_order_relaxed);
                stopRequested.store (false, std::memory_order_relaxed);
                return ProcessResult::StopTransport;
            }

            if (target <= recordedSamples)
            {
                finalizeFirstLoop (target);
                return ProcessResult::ResetTransport;
            }

            firstLoopTargetSamples = target;
        }
        else
        {
            finalizeOverdub (static_cast<int> (cycleLength));
            return ProcessResult::None;
        }
    }

    int startOffset = -1;

    if (currentState == State::Armed)
    {
        if (cycleLength <= 0)
        {
            startOffset = 0;
        }
        else
        {
            const auto positionInCycle = transportPosition % cycleLength;
            const auto samplesUntilCycleEnd = cycleLength - positionInCycle;

            if (samplesUntilCycleEnd > numSamples)
                return ProcessResult::None;

            startOffset = static_cast<int> (samplesUntilCycleEnd);
        }

        if (activeLayer == nullptr)
        {
            state.store (State::Idle, std::memory_order_relaxed);
            return ProcessResult::None;
        }

        state.store (State::Recording, std::memory_order_relaxed);
        activeLayer->state.store (LiveLayer::State::Recording, std::memory_order_relaxed);
        activeLayer->recordedSamples.store (0, std::memory_order_relaxed);
        recordedSamples = 0;
        discardRemaining = latencySamples.load (std::memory_order_relaxed);
        firstLoopTargetSamples = 0;
        currentState = State::Recording;
    }

    if (currentState != State::Recording)
        return ProcessResult::None;

    if (activeLayer == nullptr)
    {
        state.store (State::Idle, std::memory_order_relaxed);
        return ProcessResult::None;
    }

    auto* layer = activeLayer;
    const auto layerCapacity = layer->audio.getNumSamples();
    const auto begin = startOffset >= 0 ? startOffset : 0;

    const auto* inputL = numInputChannels > 0 ? inputChannelData[0] : nullptr;
    const auto* inputR = numInputChannels > 1 ? inputChannelData[1] : nullptr;

    for (int i = begin; i < numSamples; ++i)
    {
        if (discardRemaining > 0)
        {
            --discardRemaining;
            continue;
        }

        if (recordedSamples >= layerCapacity)
            break;

        const auto inL = inputL != nullptr ? inputL[i] : 0.0f;
        const auto inR = inputR != nullptr ? inputR[i] : inL;

        layer->audio.setSample (0, recordedSamples, inL);
        layer->audio.setSample (1, recordedSamples, inR);
        ++recordedSamples;

        if (cycleLength > 0 && recordedSamples >= static_cast<int> (cycleLength))
        {
            finalizeOverdub (static_cast<int> (cycleLength));
            return ProcessResult::None;
        }

        if (cycleLength <= 0)
        {
            if (firstLoopTargetSamples > 0 && recordedSamples >= firstLoopTargetSamples)
            {
                finalizeFirstLoop (firstLoopTargetSamples);
                return ProcessResult::ResetTransport;
            }

            if (recordedSamples >= maxFirstLoopSamples)
            {
                finalizeFirstLoop (maxFirstLoopSamples);
                return ProcessResult::ResetTransport;
            }
        }
    }

    layer->recordedSamples.store (recordedSamples, std::memory_order_relaxed);
    return ProcessResult::None;
}

void LiveLooper::addToMixForRender (juce::AudioBuffer<float>& mix,
                                    int numSamples,
                                    juce::int64 cyclePosition,
                                    juce::int64 cycleLength) const
{
    const juce::ScopedReadLock sl (lock);
    addLayersToMix (mix, numSamples, cyclePosition, cycleLength);
}

void LiveLooper::addLayersToMix (juce::AudioBuffer<float>& mix,
                                 int numSamples,
                                 juce::int64 cyclePosition,
                                 juce::int64 cycleLength) const noexcept
{
    if (cycleLength <= 0 || numSamples <= 0)
        return;

    bool anySolo = false;

    for (const auto& layer : layers)
        if (layer != nullptr && layer->solo.load (std::memory_order_relaxed))
        {
            anySolo = true;
            break;
        }

    for (const auto& layer : layers)
    {
        if (layer == nullptr)
            continue;

        if (layer->state.load (std::memory_order_relaxed) != LiveLayer::State::Playing)
            continue;

        if (anySolo)
        {
            if (! layer->solo.load (std::memory_order_relaxed))
                continue;
        }
        else if (layer->muted.load (std::memory_order_relaxed))
        {
            continue;
        }

        const auto usableLength = layer->usableLengthSamples.load (std::memory_order_relaxed);
        auto lengthLimit = std::min<juce::int64> (cycleLength, usableLength);
        lengthLimit = std::min<juce::int64> (lengthLimit, layer->audio.getNumSamples());
        const auto length = static_cast<int> (lengthLimit);

        if (length <= 0)
            continue;

        const auto gain = layer->gain.load (std::memory_order_relaxed);
        const auto pan = juce::jlimit (-1.0f, 1.0f,
                                       layer->pan.load (std::memory_order_relaxed));
        const auto leftGain = gain * (pan > 0.0f ? 1.0f - pan : 1.0f);
        const auto rightGain = gain * (pan < 0.0f ? 1.0f + pan : 1.0f);
        auto position = cyclePosition % cycleLength;

        if (position >= length)
            position = 0;

        int written = 0;

        while (written < numSamples)
        {
            const auto available64 = std::min<juce::int64> (
                length - position, static_cast<juce::int64> (numSamples - written));
            const auto available = static_cast<int> (available64);

            if (available <= 0)
                break;

            for (int channel = 0; channel < mix.getNumChannels(); ++channel)
            {
                const auto sourceChannel = juce::jmin (channel, layer->audio.getNumChannels() - 1);
                const auto channelGain = (channel % 2 == 0) ? leftGain : rightGain;
                mix.addFrom (channel,
                             written,
                             layer->audio,
                             sourceChannel,
                             static_cast<int> (position),
                             available,
                             channelGain);
            }

            written += available;
            position += available;

            if (position >= length)
                position = 0;
        }
    }
}

int LiveLooper::computeFirstLoopTarget() const noexcept
{
    if (recordedSamples <= 0)
        return 0;

    if (! barSnapEnabled.load (std::memory_order_relaxed))
        return recordedSamples;

    const auto samplesPerBar = transport.getSamplesPerBar();

    if (samplesPerBar <= 0.0)
        return recordedSamples;

    auto bars = static_cast<juce::int64> (std::llround (recordedSamples / samplesPerBar));
    bars = std::max<juce::int64> (1, bars);

    auto target = bars * samplesPerBar;
    target = std::min (target, static_cast<double> (maxFirstLoopSamples));

    if (activeLayer != nullptr)
        target = std::min (target, static_cast<double> (activeLayer->audio.getNumSamples()));

    return static_cast<int> (std::max (1.0, target));
}

void LiveLooper::finalizeFirstLoop (int targetSamples) noexcept
{
    if (activeLayer == nullptr)
    {
        state.store (State::Idle, std::memory_order_relaxed);
        return;
    }

    const auto target = juce::jlimit (1, activeLayer->audio.getNumSamples(), targetSamples);

    activeLayer->usableLengthSamples.store (target, std::memory_order_relaxed);
    activeLayer->recordedSamples.store (target, std::memory_order_relaxed);
    activeLayer->state.store (LiveLayer::State::Playing, std::memory_order_relaxed);

    if (activeLayer->audio.getNumSamples() > target)
        activeLayer->trimRequested.store (true, std::memory_order_relaxed);

    state.store (State::Idle, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    recordedSamples = target;
    discardRemaining = 0;
    firstLoopTargetSamples = 0;

    transport.setCycleLengthSamples (target);
}

void LiveLooper::finalizeOverdub (int cycleLengthSamples) noexcept
{
    if (activeLayer != nullptr)
    {
        activeLayer->usableLengthSamples.store (cycleLengthSamples, std::memory_order_relaxed);
        activeLayer->state.store (LiveLayer::State::Playing, std::memory_order_relaxed);
    }

    state.store (State::Idle, std::memory_order_relaxed);
    stopRequested.store (false, std::memory_order_relaxed);
    firstLoopTargetSamples = 0;
}

LiveLayer* LiveLooper::findLayerUnderLock (int layerId) const noexcept
{
    for (const auto& layer : layers)
        if (layer != nullptr && layer->id == layerId)
            return layer.get();

    return nullptr;
}

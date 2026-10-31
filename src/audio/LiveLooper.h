#pragma once

#include <JuceHeader.h>

#include "Transport.h"

#include <atomic>
#include <memory>
#include <vector>

struct LiveLayer
{
    enum class State
    {
        Empty = 0,
        Armed,
        Recording,
        Playing
    };

    int id = 0;
    juce::String name;
    juce::AudioBuffer<float> audio;
    double sampleRate = 48000.0;

    std::atomic<State> state { State::Empty };
    std::atomic<int> usableLengthSamples { 0 };
    std::atomic<int> recordedSamples { 0 };
    std::atomic<bool> trimRequested { false };
    std::atomic<float> gain { 1.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> solo { false };

    int getMemoryUsageBytes() const noexcept
    {
        return audio.getNumSamples() * audio.getNumChannels() * static_cast<int> (sizeof (float));
    }
};

/**
    Live loop layer storage and recording state machine.

    The message thread creates/removes layers and changes mix controls. The audio
    thread mixes finished layers and captures the active layer. A read/write lock
    protects structural changes while allowing the UI to read layer metadata.
*/
class LiveLooper
{
public:
    enum class ProcessResult
    {
        None,
        ResetTransport,
        StopTransport
    };

    explicit LiveLooper (Transport& transportToUse);
    ~LiveLooper() = default;

    void prepare (double sampleRate);
    void reset();

    struct LayerInfo
    {
        int id = 0;
        juce::String name;
        float gain = 1.0f;
        bool muted = false;
        bool solo = false;
        bool armed = false;
        bool recording = false;
        double durationSeconds = 0.0;
        int memoryUsageBytes = 0;
    };

    struct Snapshot
    {
        std::vector<LayerInfo> layers;
        int totalMemoryUsageBytes = 0;
        bool hasSolo = false;
        int recordingLayerId = -1;
    };

    Snapshot getSnapshot() const;
    bool hasLayers() const;
    int getLayerCount() const;

    // Message-thread commands.
    void startFirstLoop (double sampleRate);
    void startOverdub (juce::int64 cycleLengthSamples, double sampleRate, bool startImmediately);
    void stopRecording();
    bool discard();
    bool removeLayer (int layerId);
    void clearLayers();
    void reapEmptyLayers();
    void trimFinishedLayers();

    void setLayerGain (int layerId, float gain);
    void setLayerMuted (int layerId, bool muted);
    void setLayerSolo (int layerId, bool solo);

    void setLatencySamples (int samples) noexcept;
    void setBarSnapEnabled (bool shouldSnap) noexcept;
    bool isBarSnapEnabled() const noexcept;

    bool isArmed() const noexcept;
    bool isRecording() const noexcept;
    bool isRecordingOrArmed() const noexcept;

    // Audio-thread processing. Returns whether the transport should reset or stop
    // after the block (used when the first loop is finalized or aborted).
    ProcessResult processBlock (juce::AudioBuffer<float>& mix,
                                const float* const* inputChannelData,
                                int numInputChannels,
                                int numSamples,
                                juce::int64 transportPosition,
                                juce::int64 cycleLength,
                                bool transportPlaying) noexcept;

private:
    enum class State
    {
        Idle = 0,
        Armed,
        Recording
    };

    void addLayersToMix (juce::AudioBuffer<float>& mix,
                         int numSamples,
                         juce::int64 cyclePosition,
                         juce::int64 cycleLength) noexcept;
    int computeFirstLoopTarget() const noexcept;
    void finalizeFirstLoop (int targetSamples) noexcept;
    void finalizeOverdub (int cycleLengthSamples) noexcept;
    LiveLayer* findLayerUnderLock (int layerId) const noexcept;

    Transport& transport;
    mutable juce::ReadWriteLock lock;
    std::vector<std::shared_ptr<LiveLayer>> layers;
    LiveLayer* activeLayer = nullptr;

    std::atomic<State> state { State::Idle };
    std::atomic<bool> stopRequested { false };
    std::atomic<bool> barSnapEnabled { false };
    std::atomic<int> latencySamples { 0 };

    double sampleRate = 48000.0;
    int maxFirstLoopSamples = 0;
    int nextLayerId = 1;

    // Audio-thread-only counters.
    int recordedSamples = 0;
    int discardRemaining = 0;
    int firstLoopTargetSamples = 0;

    static constexpr double maxFirstLoopSeconds = 60.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveLooper)
};

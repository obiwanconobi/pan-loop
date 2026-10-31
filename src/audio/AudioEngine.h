#pragma once

#include <JuceHeader.h>
#include <atomic>

#include "LiveLooper.h"
#include "Metronome.h"
#include "Playlist.h"
#include "Transport.h"

/**
    Phase 1 audio engine.

    At the moment this is deliberately small: stereo input monitoring, a test
    tone, gain controls and meters. The looping/playlist engine will be added
    behind this same callback in the next phases.
*/
class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override = default;

    //==========================================================================
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceError (const juce::String& errorMessage) override;

    //==========================================================================
    void setInputGainDb (float db) noexcept;
    void setOutputGainDb (float db) noexcept;
    void setMonitoringEnabled (bool shouldMonitor) noexcept;
    bool isMonitoringEnabled() const noexcept;
    void toggleInputMonitoring() noexcept;
    void setBackingMuted (bool muted) noexcept;
    bool isBackingMuted() const noexcept;
    void toggleBackingMute() noexcept;
    void setTestToneEnabled (bool shouldPlayTone) noexcept;
    void setTestToneFrequency (double hz) noexcept;

    float getInputLevel() const noexcept;
    float getOutputLevel() const noexcept;
    float getOutputGainLinear() const noexcept;
    double getCurrentSampleRate() const noexcept;
    int getCurrentBlockSize() const noexcept;

    int getInputLatencySamples() const noexcept;
    int getOutputLatencySamples() const noexcept;
    double getRoundTripLatencyMs() const noexcept;

    Transport& getTransport() noexcept              { return transport; }
    const Transport& getTransport() const noexcept  { return transport; }
    Metronome& getMetronome() noexcept              { return metronome; }
    const Metronome& getMetronome() const noexcept  { return metronome; }
    Playlist& getPlaylist() noexcept                { return playlist; }
    const Playlist& getPlaylist() const noexcept    { return playlist; }
    LiveLooper& getLiveLooper() noexcept            { return liveLooper; }
    const LiveLooper& getLiveLooper() const noexcept { return liveLooper; }

    /** Recomputes the transport cycle after a playlist edit. Message thread only. */
    void playlistChanged();

    // Live-loop transport commands, called from the message thread.
    void record();
    void stopRecording();
    void discardLiveLayer();
    void clearLiveLayers();
    bool removeLiveLayer (int layerId);
    bool hasLiveLayers() const;

    void setBarSnapEnabled (bool shouldSnap);
    bool isBarSnapEnabled() const;

    void setLatencyOffsetMs (float milliseconds);
    float getLatencyOffsetMs() const noexcept;
    int getRecordingLatencySamples() const noexcept;

    bool hasClipped() const noexcept;
    void resetClipIndicator() noexcept;

private:
    void updateLiveLatency() noexcept;

    Transport transport;
    Metronome metronome;
    Playlist playlist;
    LiveLooper liveLooper;
    juce::AudioBuffer<float> mixBuffer;

    Transport::State lastTransportState = Transport::State::Stopped;
    juce::int64 lastTransportPositionSamples = 0;
    double nextBeatPosition = 0.0;
    double lastSamplesPerBeat = 0.0;
    double preparedSampleRate = 0.0;
    bool enginePrepared = false;

    std::atomic<int> inputLatencySamples { 0 };
    std::atomic<int> outputLatencySamples { 0 };
    std::atomic<float> latencyOffsetMs { 0.0f };

    std::atomic<float> inputGainLinear { 1.0f };
    std::atomic<float> outputGainLinear { 1.0f };
    std::atomic<bool> monitoringEnabled { true };
    std::atomic<bool> backingMuted { false };
    std::atomic<bool> testToneEnabled { false };
    std::atomic<double> testToneFrequency { 440.0 };
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<int> currentBlockSize { 0 };
    std::atomic<bool> clipIndicator { false };

    juce::SmoothedValue<float> inputGainSmoother;
    juce::SmoothedValue<float> outputGainSmoother;
    juce::SmoothedValue<float> monitorGainSmoother;
    juce::SmoothedValue<float> testToneGainSmoother;

    double testTonePhase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};

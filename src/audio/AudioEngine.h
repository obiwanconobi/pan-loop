#pragma once

#include <JuceHeader.h>
#include <atomic>

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
    AudioEngine() = default;
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
    void setTestToneEnabled (bool shouldPlayTone) noexcept;
    void setTestToneFrequency (double hz) noexcept;

    float getInputLevel() const noexcept;
    float getOutputLevel() const noexcept;
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

    /** Recomputes the transport cycle after a playlist edit. Message thread only. */
    void playlistChanged();

    bool hasClipped() const noexcept;
    void resetClipIndicator() noexcept;

private:
    Transport transport;
    Metronome metronome;
    Playlist playlist;
    juce::AudioBuffer<float> mixBuffer;

    Transport::State lastTransportState = Transport::State::Stopped;
    juce::int64 lastTransportPositionSamples = 0;
    double nextBeatPosition = 0.0;
    double lastSamplesPerBeat = 0.0;
    double preparedSampleRate = 0.0;
    bool enginePrepared = false;

    std::atomic<int> inputLatencySamples { 0 };
    std::atomic<int> outputLatencySamples { 0 };

    std::atomic<float> inputGainLinear { 1.0f };
    std::atomic<float> outputGainLinear { 1.0f };
    std::atomic<bool> monitoringEnabled { true };
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

#pragma once

#include <JuceHeader.h>
#include <atomic>

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

    bool hasClipped() const noexcept;
    void resetClipIndicator() noexcept;

private:
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

#pragma once

#include <JuceHeader.h>
#include "ui/LevelMeter.h"

#include <vector>

class AudioEngine;

/** Device selection, monitoring, gain and meter controls for phase 1. */
class AudioSettingsPanel final : public juce::Component,
                                 private juce::Timer
{
public:
    AudioSettingsPanel (juce::AudioDeviceManager& deviceManager,
                        AudioEngine& audioEngine);
    ~AudioSettingsPanel() override = default;

    void resized() override;
    void paint (juce::Graphics&) override;

    void refreshFromManager();

private:
    void timerCallback() override;

    void rebuildBackendList();
    void rebuildDeviceLists();
    void rebuildSampleRateList();
    void applyDeviceSelection();
    void updateStatus();
    void updateMeters();
    void showError (const juce::String& message);

    juce::AudioDeviceManager& deviceManager;
    AudioEngine& engine;

    juce::GroupComponent deviceGroup { "Audio device", "Audio device" };
    juce::GroupComponent mixGroup { "Monitoring and test", "Monitoring and test" };

    juce::Label backendLabel;
    juce::Label inputLabel;
    juce::Label outputLabel;
    juce::Label sampleRateLabel;
    juce::Label sampleRateHintLabel;

    juce::ComboBox backendBox;
    juce::ComboBox inputBox;
    juce::ComboBox outputBox;
    juce::ComboBox sampleRateBox;

    std::vector<double> sampleRates;

    juce::TextButton applyButton { "Apply" };
    juce::TextButton rescanButton { "Rescan" };

    juce::Label jackStatusLabel;
    juce::Label statusLabel;
    juce::Label errorLabel;

    juce::ToggleButton monitorButton { "Input monitoring" };
    juce::ToggleButton testToneButton { "Test tone (440 Hz)" };

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;
    juce::Label latencyOffsetLabel;
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider latencyOffsetSlider;

    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    LevelMeter inputMeter;
    LevelMeter outputMeter;

    bool updatingControls = false;
    int clipHoldTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSettingsPanel)
};

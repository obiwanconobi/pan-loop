#pragma once

#include <JuceHeader.h>

class AudioEngine;
class LevelMeter;

/** Full-window stage view with oversized transport and loop controls. */
class PerformancePanel final : public juce::Component,
                               private juce::Timer
{
public:
    explicit PerformancePanel (AudioEngine& audioEngine);
    ~PerformancePanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;

    std::function<void()> onExitRequested;
    std::function<void()> onToggleFullScreen;

private:
    void timerCallback() override;
    void updateControls();
    void startRecording();
    void stopRecording();
    void discardLayer();
    void togglePlayPause();

    AudioEngine& engine;

    juce::Label stateLabel;
    juce::Label timeLabel;
    juce::Label backingLabel;
    juce::Label layerLabel;
    juce::Label tempoLabel;
    juce::Label hintLabel;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    juce::TextButton recordButton { "RECORD" };
    juce::TextButton stopRecButton { "STOP REC" };
    juce::TextButton discardButton { "DISCARD" };
    juce::TextButton playPauseButton { "PLAY" };
    juce::TextButton backingMuteButton { "Backing Mute" };
    juce::TextButton clearLayersButton { "Clear Layers" };
    juce::TextButton metronomeButton { "Metronome" };
    juce::TextButton fullScreenButton { "Full Screen" };
    juce::TextButton exitButton { "Exit  (Esc)" };

    std::unique_ptr<LevelMeter> inputMeter;
    std::unique_ptr<LevelMeter> outputMeter;
    std::unique_ptr<juce::LookAndFeel_V4> stageLook;

    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> progressArea;
    juce::Rectangle<int> buttonArea;
    juce::Rectangle<int> infoArea;
    juce::Rectangle<int> meterArea;
    juce::Rectangle<int> backingColumn;
    juce::Rectangle<int> layerColumn;
    juce::Rectangle<int> tempoColumn;
    bool wasRecording = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformancePanel)
};

#pragma once

#include <JuceHeader.h>

class AudioEngine;

/** Live transport surface for phase 2: play/pause, tempo, bar/beat display,
    metronome and latency readout. The loop-layer controls are placeholders. */
class LivePanel final : public juce::Component,
                        private juce::Timer
{
public:
    explicit LivePanel (AudioEngine& audioEngine);
    ~LivePanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;

private:
    void timerCallback() override;

    void togglePlayPause();
    void stopTransport();
    void startRecording();
    void stopRecording();
    void discardLayer();
    void updateBpm();
    void updateTimeSignature();
    void tapTempo();
    void updateControls();
    void updateLiveControls();

    static juce::String formatTime (double seconds);

    AudioEngine& engine;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label stateLabel;

    juce::Rectangle<int> transportCard;
    juce::Rectangle<int> loopCard;
    juce::Rectangle<int> beatIndicatorArea;
    juce::Rectangle<int> barProgressArea;

    juce::Label barBeatLabel;
    juce::Label timeLabel;
    juce::Label latencyLabel;
    juce::Label cycleLabel;
    juce::Label transportHintLabel;

    juce::TextButton playPauseButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton tapButton { "Tap" };

    juce::Label bpmLabel;
    juce::Label timeSigLabel;
    juce::Label metroLevelLabel;
    juce::Slider bpmSlider;
    juce::ComboBox timeSigBox;
    juce::ToggleButton metroToggle { "Metronome" };
    juce::Slider metroLevelSlider;

    juce::Label loopTitleLabel;
    juce::Label loopHintLabel;
    juce::ToggleButton barSnapToggle { "Snap first loop to bar" };
    juce::TextButton recordButton { "Record" };
    juce::TextButton stopRecButton { "Stop Rec" };
    juce::TextButton discardButton { "Discard" };

    juce::Array<double> tapIntervals;
    double lastTapTimeMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivePanel)
};

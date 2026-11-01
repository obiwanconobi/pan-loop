#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "midi/MidiAction.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void handleMidiAction (MidiAction action);
    void setPerformanceMode (bool shouldBeEnabled);
    void toggleFullScreen();
    void updateNavState();
    void updateAudioStatus();

    static constexpr int numTabs = 6;

    juce::AudioDeviceManager deviceManager;
    AudioEngine audioEngine;
    std::unique_ptr<class MidiManager> midiManager;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label audioStatusLabel;
    juce::TextButton performanceButton { "Performance" };
    juce::TextButton navButtons[numTabs];

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::TooltipWindow tooltipWindow { this, 700 };
    class PlaylistPanel* playlistPanel = nullptr;
    std::unique_ptr<class PerformancePanel> performancePanel;
    bool performanceMode = false;

    juce::Rectangle<int> navDockArea;
    juce::Rectangle<int> audioStatusPillArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

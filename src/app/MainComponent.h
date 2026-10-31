#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "midi/MidiAction.h"

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void handleMidiAction (MidiAction action);
    void setPerformanceMode (bool shouldBeEnabled);
    void toggleFullScreen();

    juce::AudioDeviceManager deviceManager;
    AudioEngine audioEngine;
    std::unique_ptr<class MidiManager> midiManager;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label phaseLabel;
    juce::TextButton performanceButton { "Performance" };

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::TooltipWindow tooltipWindow { this, 700 };
    class PlaylistPanel* playlistPanel = nullptr;
    std::unique_ptr<class PerformancePanel> performancePanel;
    bool performanceMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

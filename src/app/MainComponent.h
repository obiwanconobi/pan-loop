#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioDeviceManager deviceManager;
    AudioEngine audioEngine;
    std::unique_ptr<class MidiManager> midiManager;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label phaseLabel;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::TooltipWindow tooltipWindow { this, 700 };
    class PlaylistPanel* playlistPanel = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

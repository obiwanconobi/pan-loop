#pragma once

#include <JuceHeader.h>
#include "MidiManager.h"

/** MIDI device selection and activity monitor for phase 1. */
class MidiPanel final : public juce::Component,
                        private juce::Timer
{
public:
    explicit MidiPanel (MidiManager& midiManager);
    ~MidiPanel() override = default;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void rebuildDeviceList();
    void updateSelection();

    MidiManager& midi;

    juce::GroupComponent deviceGroup { "MIDI input", "MIDI input" };
    juce::GroupComponent logGroup { "MIDI activity", "MIDI activity" };

    juce::Label deviceLabel;
    juce::Label statusLabel;
    juce::Label lastMessageLabel;
    juce::Label hintLabel;

    juce::ComboBox deviceBox;
    juce::TextButton rescanButton { "Rescan" };
    juce::TextEditor logEditor;

    juce::Array<juce::MidiDeviceInfo> devices;
    juce::Rectangle<int> activityBounds;
    juce::uint64 lastSeenCount = 0;
    int activityHoldTicks = 0;
    bool updatingControls = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPanel)
};

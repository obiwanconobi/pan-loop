#pragma once

#include <JuceHeader.h>
#include "MidiManager.h"

#include <memory>
#include <vector>

/** MIDI device selection, activity monitor and footswitch mapping UI. */
class MidiPanel final : public juce::Component,
                        private juce::Timer
{
public:
    explicit MidiPanel (MidiManager& midiManager);
    ~MidiPanel() override = default;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    struct MappingRow
    {
        MidiAction action = MidiAction::Record;
        juce::Label actionLabel;
        juce::Label mappingLabel;
        juce::TextButton learnButton;
        juce::TextButton clearButton;
    };

    void timerCallback() override;
    void rebuildDeviceList();
    void updateSelection();
    void buildMappingRows();
    void updateMappingRows();

    MidiManager& midi;

    juce::GroupComponent deviceGroup { "MIDI input", "MIDI input" };
    juce::GroupComponent mappingGroup { "Footswitch mappings", "Footswitch mappings" };
    juce::GroupComponent logGroup { "MIDI activity", "MIDI activity" };

    juce::Label deviceLabel;
    juce::Label statusLabel;
    juce::Label lastMessageLabel;
    juce::Label hintLabel;
    juce::Label mappingHintLabel;

    juce::ComboBox deviceBox;
    juce::TextButton rescanButton { "Rescan" };
    juce::TextEditor logEditor;

    std::vector<std::unique_ptr<MappingRow>> mappingRows;
    juce::Array<juce::MidiDeviceInfo> devices;
    juce::Rectangle<int> activityBounds;
    juce::uint64 lastSeenCount = 0;
    int activityHoldTicks = 0;
    bool updatingControls = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPanel)
};

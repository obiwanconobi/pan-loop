#pragma once

#include <JuceHeader.h>
#include "MidiManager.h"

#include <memory>
#include <vector>

/** Small MIDI activity indicator drawn above its parent groups. */
class MidiActivityLight final : public juce::Component
{
public:
    void setActive (bool shouldBeActive)
    {
        if (active != shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    void paint (juce::Graphics&) override;

private:
    bool active = false;
};

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
    void rebuildProfileList (const juce::String& selectName = {});
    void updateSelection();
    void buildMappingRows();
    void updateMappingRows();
    void saveCurrentProfile();
    void deleteSelectedProfile();
    juce::String getSelectedProfileName() const;
    void setProfileStatus (const juce::String& message, bool isError);

    MidiManager& midi;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::GroupComponent deviceGroup { "MIDI input", "MIDI input" };
    juce::GroupComponent mappingGroup { "Footswitch mappings", "Footswitch mappings" };
    juce::GroupComponent logGroup { "MIDI activity", "MIDI activity" };

    juce::Label deviceLabel;
    juce::Label statusLabel;
    juce::Label lastMessageLabel;
    juce::Label mappingHintLabel;
    juce::Label profileLabel;
    juce::Label profileStatusLabel;

    juce::ComboBox deviceBox;
    juce::ComboBox profileBox;
    juce::TextEditor profileNameEditor;
    juce::TextButton rescanButton { "Rescan" };
    juce::TextButton saveProfileButton { "Save" };
    juce::TextButton deleteProfileButton { "Delete" };
    juce::TextEditor logEditor;

    MidiActivityLight activityLight;

    std::vector<std::unique_ptr<MappingRow>> mappingRows;
    juce::Array<juce::MidiDeviceInfo> devices;
    juce::StringArray profileNames;
    juce::uint64 lastSeenCount = 0;
    int activityHoldTicks = 0;
    bool updatingControls = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPanel)
};

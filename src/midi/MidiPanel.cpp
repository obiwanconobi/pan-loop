#include "MidiPanel.h"
#include "app/DarkLookAndFeel.h"

#include <array>

namespace
{
    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        label.setFont (juce::Font (juce::FontOptions (height,
                                                      bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
        label.setJustificationType (juce::Justification::centredLeft);
    }
}

MidiPanel::MidiPanel (MidiManager& midiManager)
    : midi (midiManager)
{
    deviceGroup.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
    deviceGroup.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());
    mappingGroup.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
    mappingGroup.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());
    logGroup.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
    logGroup.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());

    styleLabel (deviceLabel, 14.0f);
    styleLabel (statusLabel, 13.0f, true);
    styleLabel (lastMessageLabel, 13.0f);
    styleLabel (hintLabel, 12.0f);
    styleLabel (mappingHintLabel, 12.0f);
    styleLabel (profileLabel, 14.0f);
    styleLabel (profileStatusLabel, 12.0f);

    deviceLabel.setText ("Device", juce::dontSendNotification);
    statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
    lastMessageLabel.setText ("No MIDI activity yet.", juce::dontSendNotification);
    hintLabel.setText ("Choose an input, learn the footswitches, then save the setup as a preset.",
                       juce::dontSendNotification);
    hintLabel.setJustificationType (juce::Justification::topLeft);
    hintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    profileLabel.setText ("Preset", juce::dontSendNotification);
    profileStatusLabel.setText ("No preset selected.", juce::dontSendNotification);
    profileStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    profileStatusLabel.setJustificationType (juce::Justification::centredLeft);
    deleteProfileButton.setEnabled (false);
    saveProfileButton.setTooltip ("Save the current input device and footswitch mappings under the name in the box");
    deleteProfileButton.setTooltip ("Delete the selected MIDI preset");
    profileBox.setTooltip ("Choose a saved MIDI preset to restore its device and mappings");

    profileNameEditor.setFont (juce::Font (juce::FontOptions (14.0f)));
    profileNameEditor.setTextToShowWhenEmpty ("Preset name", DarkLookAndFeel::mutedText());
    profileNameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f1115));
    profileNameEditor.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::text());
    profileNameEditor.setColour (juce::TextEditor::outlineColourId, DarkLookAndFeel::outline());
    profileNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, DarkLookAndFeel::accent());
    profileNameEditor.onReturnKey = [this] { saveCurrentProfile(); };

    mappingHintLabel.setText ("Click Learn, then press a footswitch. Suggested: "
                              "FS1 Record, FS2 Stop Recording, FS3 Discard.",
                              juce::dontSendNotification);
    mappingHintLabel.setJustificationType (juce::Justification::topLeft);
    mappingHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    logEditor.setMultiLine (true);
    logEditor.setReadOnly (true);
    logEditor.setScrollbarsShown (true);
    logEditor.setCaretVisible (false);
    logEditor.setPopupMenuEnabled (true);
    logEditor.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                                      12.0f,
                                                      juce::Font::plain)));
    logEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f1115));
    logEditor.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::text());
    logEditor.setColour (juce::TextEditor::outlineColourId, DarkLookAndFeel::outline());

    addAndMakeVisible (deviceGroup);
    addAndMakeVisible (mappingGroup);
    addAndMakeVisible (logGroup);
    addAndMakeVisible (deviceLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (lastMessageLabel);
    addAndMakeVisible (hintLabel);
    addAndMakeVisible (mappingHintLabel);
    addAndMakeVisible (deviceBox);
    addAndMakeVisible (rescanButton);
    addAndMakeVisible (profileLabel);
    addAndMakeVisible (profileBox);
    addAndMakeVisible (profileNameEditor);
    addAndMakeVisible (profileStatusLabel);
    addAndMakeVisible (saveProfileButton);
    addAndMakeVisible (deleteProfileButton);
    addAndMakeVisible (logEditor);

    buildMappingRows();

    deviceBox.onChange = [this]
    {
        if (! updatingControls)
            updateSelection();
    };

    profileBox.onChange = [this]
    {
        if (updatingControls)
            return;

        deleteProfileButton.setEnabled (profileBox.getSelectedId() > 1);

        const auto name = getSelectedProfileName();
        profileNameEditor.setText (name, juce::dontSendNotification);

        if (name.isEmpty())
        {
            setProfileStatus ("No preset selected.", false);
            return;
        }

        const auto error = midi.applyProfile (name);
        rebuildDeviceList();
        updateMappingRows();
        setProfileStatus (error.isNotEmpty() ? error
                                             : "Preset loaded: " + name,
                          error.isNotEmpty());
    };

    rescanButton.onClick = [this]
    {
        rebuildDeviceList();
    };

    saveProfileButton.onClick = [this] { saveCurrentProfile(); };
    deleteProfileButton.onClick = [this] { deleteSelectedProfile(); };

    rebuildDeviceList();
    rebuildProfileList();
    updateMappingRows();
    startTimerHz (30);

   #if JUCE_DEBUG
    const auto testProfile = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_MIDI_PROFILE", {});

    if (testProfile.isNotEmpty())
    {
        juce::Component::SafePointer<MidiPanel> safeThis (this);

        juce::Timer::callAfterDelay (1500, [safeThis, testProfile]
        {
            if (safeThis == nullptr)
                return;

            for (int i = 0; i < safeThis->profileNames.size(); ++i)
            {
                if (safeThis->profileNames[i].equalsIgnoreCase (testProfile))
                {
                    safeThis->profileBox.setSelectedId (i + 2, juce::sendNotification);
                    break;
                }
            }
        });
    }

    const auto testSaveProfile = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_MIDI_PROFILE_SAVE", {});

    if (testSaveProfile.isNotEmpty())
    {
        juce::Component::SafePointer<MidiPanel> safeThis (this);

        juce::Timer::callAfterDelay (2500, [safeThis, testSaveProfile]
        {
            if (safeThis == nullptr)
                return;

            const auto error = safeThis->midi.saveCurrentProfile (testSaveProfile);
            safeThis->rebuildProfileList (testSaveProfile);
            safeThis->setProfileStatus (
                error.isNotEmpty() ? error : "Preset saved: " + testSaveProfile,
                error.isNotEmpty());
            juce::Logger::writeToLog ("Test MIDI profile save " + testSaveProfile + ": "
                                      + (error.isEmpty() ? "ok" : error));
        });
    }
   #endif
}

void MidiPanel::resized()
{
    auto area = getLocalBounds().reduced (18);

    deviceGroup.setBounds (area.removeFromTop (230));
    area.removeFromTop (10);
    mappingGroup.setBounds (area.removeFromTop (280));
    area.removeFromTop (10);
    logGroup.setBounds (area);

    {
        auto inner = deviceGroup.getLocalBounds().reduced (14);
        inner.translate (deviceGroup.getX(), deviceGroup.getY());
        inner.removeFromTop (12);

        auto row = inner.removeFromTop (30);
        deviceLabel.setBounds (row.removeFromLeft (75));
        deviceBox.setBounds (row.removeFromLeft (360));
        row.removeFromLeft (10);
        rescanButton.setBounds (row.removeFromLeft (100));
        row.removeFromLeft (12);
        activityBounds = row.removeFromLeft (36).reduced (10);

        inner.removeFromTop (8);
        row = inner.removeFromTop (30);
        profileLabel.setBounds (row.removeFromLeft (75));
        profileBox.setBounds (row.removeFromLeft (240));
        row.removeFromLeft (8);
        profileNameEditor.setBounds (row.removeFromLeft (180).reduced (0, 1));
        row.removeFromLeft (8);
        saveProfileButton.setBounds (row.removeFromLeft (90).reduced (0, 1));
        row.removeFromLeft (8);
        deleteProfileButton.setBounds (row.removeFromLeft (80).reduced (0, 1));

        inner.removeFromTop (4);
        profileStatusLabel.setBounds (inner.removeFromTop (18));

        inner.removeFromTop (6);
        statusLabel.setBounds (inner.removeFromTop (22));

        inner.removeFromTop (4);
        lastMessageLabel.setBounds (inner.removeFromTop (22));

        inner.removeFromTop (8);
        hintLabel.setBounds (inner.removeFromTop (34));
    }

    {
        auto inner = mappingGroup.getLocalBounds().reduced (14);
        inner.translate (mappingGroup.getX(), mappingGroup.getY());
        inner.removeFromTop (12);

        mappingHintLabel.setBounds (inner.removeFromTop (20));
        inner.removeFromTop (4);

        for (auto& row : mappingRows)
        {
            auto rowArea = inner.removeFromTop (27);
            row->actionLabel.setBounds (rowArea.removeFromLeft (170));
            row->mappingLabel.setBounds (rowArea.removeFromLeft (250));
            rowArea.removeFromLeft (10);
            row->learnButton.setBounds (rowArea.removeFromLeft (100).reduced (0, 1));
            rowArea.removeFromLeft (8);
            row->clearButton.setBounds (rowArea.removeFromLeft (76).reduced (0, 1));
        }
    }

    {
        auto inner = logGroup.getLocalBounds().reduced (14);
        inner.translate (logGroup.getX(), logGroup.getY());
        inner.removeFromTop (12);
        logEditor.setBounds (inner);
    }
}

void MidiPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    if (! activityBounds.isEmpty())
    {
        g.setColour (activityHoldTicks > 0 ? DarkLookAndFeel::playing()
                                           : DarkLookAndFeel::outline());
        g.fillEllipse (activityBounds.toFloat());
    }
}

void MidiPanel::timerCallback()
{
    const auto snapshot = midi.getSnapshot();

    if (snapshot.messageCount != lastSeenCount)
    {
        lastSeenCount = snapshot.messageCount;
        activityHoldTicks = 10;

        lastMessageLabel.setText (snapshot.lastDevice + ": " + snapshot.lastMessage,
                                  juce::dontSendNotification);
        lastMessageLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());

        logEditor.setText (snapshot.logText, false);
        logEditor.moveCaretToEnd();
    }

    if (activityHoldTicks > 0)
        --activityHoldTicks;

    repaint (activityBounds);

    const auto enabled = midi.getEnabledDeviceIdentifier();
    const auto learning = midi.getLearningAction();

    if (learning.has_value())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Learning " + juce::String (getMidiActionName (*learning))
                                 + " - press a footswitch.",
                             juce::dontSendNotification);
    }
    else if (enabled.isEmpty())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
        statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
        statusLabel.setText ("Connected: " + deviceBox.getText(), juce::dontSendNotification);
    }

    updateMappingRows();
}

void MidiPanel::rebuildDeviceList()
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    midi.refreshDeviceList();
    devices = midi.getAvailableDevices();

    deviceBox.clear (juce::dontSendNotification);
    deviceBox.addItem ("No MIDI input", 1);

    for (int i = 0; i < devices.size(); ++i)
        deviceBox.addItem (devices[i].name, i + 2);

    const auto enabled = midi.getEnabledDeviceIdentifier();
    int selectedId = 1;

    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].identifier == enabled)
        {
            selectedId = i + 2;
            break;
        }
    }

    deviceBox.setSelectedId (selectedId, juce::dontSendNotification);

    if (enabled.isEmpty())
    {
        statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    }
    else
    {
        statusLabel.setText ("Connected: " + deviceBox.getText(), juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
    }
}

void MidiPanel::rebuildProfileList (const juce::String& selectName)
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    const auto previousName = selectName.isNotEmpty() ? selectName
                                                      : getSelectedProfileName();
    profileNames = midi.getProfileNames();

    profileBox.clear (juce::dontSendNotification);
    profileBox.addItem ("No preset", 1);

    int selectedId = 1;

    for (int i = 0; i < profileNames.size(); ++i)
    {
        profileBox.addItem (profileNames[i], i + 2);

        if (profileNames[i].equalsIgnoreCase (previousName))
            selectedId = i + 2;
    }

    profileBox.setSelectedId (selectedId, juce::dontSendNotification);
    deleteProfileButton.setEnabled (selectedId > 1);

    profileNameEditor.setText (selectedId > 1 ? profileNames[selectedId - 2]
                                              : juce::String(),
                               juce::dontSendNotification);
}

juce::String MidiPanel::getSelectedProfileName() const
{
    const auto selectedId = profileBox.getSelectedId();

    if (selectedId <= 1 || selectedId - 2 >= profileNames.size())
        return {};

    return profileNames[selectedId - 2];
}

void MidiPanel::setProfileStatus (const juce::String& message, bool isError)
{
    profileStatusLabel.setColour (juce::Label::textColourId,
                                  isError ? DarkLookAndFeel::recording()
                                          : DarkLookAndFeel::mutedText());
    profileStatusLabel.setText (message, juce::dontSendNotification);
}

void MidiPanel::saveCurrentProfile()
{
    const auto name = profileNameEditor.getText().trim();

    if (name.isEmpty())
    {
        setProfileStatus ("Enter a preset name first.", true);
        return;
    }

    const auto error = midi.saveCurrentProfile (name);
    rebuildProfileList (name);
    setProfileStatus (error.isNotEmpty() ? error
                                         : "Preset saved: " + name,
                      error.isNotEmpty());
}

void MidiPanel::deleteSelectedProfile()
{
    const auto name = getSelectedProfileName();

    if (name.isEmpty())
    {
        setProfileStatus ("No preset selected to delete.", true);
        return;
    }

    const auto error = midi.deleteProfile (name);
    rebuildProfileList();
    setProfileStatus (error.isNotEmpty() ? error
                                         : "Preset deleted: " + name,
                      error.isNotEmpty());
}

void MidiPanel::updateSelection()
{
    const auto selectedId = deviceBox.getSelectedId();

    if (selectedId <= 1)
    {
        midi.setDevice ({});
        statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
        return;
    }

    const auto index = selectedId - 2;

    if (juce::isPositiveAndBelow (index, devices.size()))
    {
        midi.setDevice (devices[index].identifier);

        if (midi.getEnabledDeviceIdentifier() == devices[index].identifier)
        {
            statusLabel.setText ("Connected: " + devices[index].name, juce::dontSendNotification);
            statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
        }
        else
        {
            statusLabel.setText ("Could not open: " + devices[index].name
                                     + " - check /dev/snd permissions",
                                 juce::dontSendNotification);
            statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        }
    }
}

void MidiPanel::buildMappingRows()
{
    const std::array<MidiAction, numMidiActions> actions
    {
        MidiAction::Record,
        MidiAction::StopRecording,
        MidiAction::DiscardLayer,
        MidiAction::PlayPause,
        MidiAction::ClearLayers,
        MidiAction::BackingMute,
        MidiAction::InputMonitor,
        MidiAction::Metronome
    };

    for (const auto action : actions)
    {
        auto row = std::make_unique<MappingRow>();
        row->action = action;

        styleLabel (row->actionLabel, 13.0f);
        styleLabel (row->mappingLabel, 12.0f);
        row->mappingLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

        row->actionLabel.setText (getMidiActionName (action), juce::dontSendNotification);
        row->mappingLabel.setText ("Not assigned", juce::dontSendNotification);

        row->learnButton.setButtonText ("Learn");
        row->clearButton.setButtonText ("Clear");
        row->learnButton.setTooltip ("Listen for the next footswitch press");
        row->clearButton.setTooltip ("Remove this mapping");

        row->learnButton.onClick = [this, action]
        {
            const auto learning = midi.getLearningAction();

            if (learning.has_value() && *learning == action)
                midi.cancelLearning();
            else
                midi.startLearning (action);

            updateMappingRows();
        };

        row->clearButton.onClick = [this, action]
        {
            midi.clearMapping (action);
            updateMappingRows();
        };

        addAndMakeVisible (row->actionLabel);
        addAndMakeVisible (row->mappingLabel);
        addAndMakeVisible (row->learnButton);
        addAndMakeVisible (row->clearButton);

        mappingRows.push_back (std::move (row));
    }
}

void MidiPanel::updateMappingRows()
{
    const auto hasDevice = midi.getEnabledDeviceIdentifier().isNotEmpty();
    const auto learning = midi.getLearningAction();

    for (auto& row : mappingRows)
    {
        const auto mapping = midi.getMapping (row->action);

        if (mapping.has_value())
        {
            row->mappingLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
            row->mappingLabel.setText (describeMidiMapping (*mapping), juce::dontSendNotification);
        }
        else
        {
            row->mappingLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            row->mappingLabel.setText ("Not assigned", juce::dontSendNotification);
        }

        const auto isLearning = learning.has_value() && *learning == row->action;

        row->learnButton.setButtonText (isLearning ? "Listening..." : "Learn");
        row->learnButton.setColour (juce::TextButton::buttonColourId,
                                    isLearning ? DarkLookAndFeel::armed().withAlpha (0.25f)
                                               : juce::Colour (0xff2a2f38));
        row->learnButton.setColour (juce::TextButton::textColourOffId,
                                    isLearning ? DarkLookAndFeel::armed()
                                               : DarkLookAndFeel::text());
        row->learnButton.setEnabled (hasDevice || isLearning);
        row->clearButton.setEnabled (mapping.has_value());
    }
}

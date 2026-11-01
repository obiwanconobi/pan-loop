#include "MidiPanel.h"
#include "app/DarkLookAndFeel.h"

#include <array>

namespace
{
    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        DarkLookAndFeel::styleLabel (label, height, bold);
    }
}

void MidiActivityLight::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto colour = active ? DarkLookAndFeel::playing() : DarkLookAndFeel::outline();

    g.setColour (colour.withAlpha (active ? 0.18f : 0.0f));
    g.fillEllipse (bounds.reduced (1.0f));

    g.setColour (colour);
    g.fillEllipse (bounds.reduced (6.0f));
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

    styleLabel (titleLabel, 28.0f, true);
    styleLabel (subtitleLabel, 12.5f);
    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.02f));
    titleLabel.setText ("MIDI", juce::dontSendNotification);
    subtitleLabel.setFont (DarkLookAndFeel::sans (12.5f));
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    subtitleLabel.setText ("Footswitch control, device input and presets", juce::dontSendNotification);

    deviceLabel.setFont (DarkLookAndFeel::kicker (10.5f));
    deviceLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setFont (DarkLookAndFeel::sans (12.0f, true));
    lastMessageLabel.setFont (DarkLookAndFeel::mono (11.5f));
    mappingHintLabel.setFont (DarkLookAndFeel::mono (10.5f));
    profileLabel.setFont (DarkLookAndFeel::kicker (10.5f));
    profileLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    profileStatusLabel.setFont (DarkLookAndFeel::sans (11.0f));

    deviceLabel.setText ("DEVICE", juce::dontSendNotification);
    statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
    lastMessageLabel.setText ("No MIDI activity yet.", juce::dontSendNotification);

    profileLabel.setText ("PRESET", juce::dontSendNotification);
    profileStatusLabel.setText ("No preset selected.", juce::dontSendNotification);
    profileStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    profileStatusLabel.setJustificationType (juce::Justification::centredLeft);
    deleteProfileButton.setEnabled (false);
    saveProfileButton.setTooltip ("Save the current input device and footswitch mappings under the name in the box");
    deleteProfileButton.setTooltip ("Delete the selected MIDI preset");
    profileBox.setTooltip ("Choose a saved MIDI preset to restore its device and mappings");

    profileNameEditor.setFont (juce::Font (juce::FontOptions (13.5f)));
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
                                                      11.5f,
                                                      juce::Font::plain)));
    logEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f1115));
    logEditor.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::text());
    logEditor.setColour (juce::TextEditor::outlineColourId, DarkLookAndFeel::outline());

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (deviceGroup);
    addAndMakeVisible (mappingGroup);
    addAndMakeVisible (logGroup);
    addAndMakeVisible (deviceLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (lastMessageLabel);
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
    addAndMakeVisible (activityLight);

    DarkLookAndFeel::setButtonVariant (rescanButton, "ghost");
    DarkLookAndFeel::setButtonIcon (rescanButton, "loop");
    DarkLookAndFeel::setButtonTextSize (rescanButton, 11.5f);
    DarkLookAndFeel::setButtonVariant (saveProfileButton, "accent");
    DarkLookAndFeel::setButtonIcon (saveProfileButton, "save");
    DarkLookAndFeel::setButtonTextSize (saveProfileButton, 11.5f);
    DarkLookAndFeel::setButtonVariant (deleteProfileButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (deleteProfileButton, "trash");
    DarkLookAndFeel::setButtonTextSize (deleteProfileButton, 11.5f);

    deviceBox.setTooltip ("Choose the MIDI input that your footswitch is connected to");
    profileBox.setTooltip ("Restore a saved MIDI device and footswitch mapping preset");
    profileNameEditor.setTooltip ("Name for the current device and mappings");

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
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (44);
    auto titleArea = header.removeFromLeft (header.getWidth());
    titleLabel.setBounds (titleArea.removeFromTop (30));
    subtitleLabel.setBounds (titleArea);

    area.removeFromTop (10);

    auto left = area.removeFromLeft (juce::roundToInt (area.getWidth() * 0.60f));
    area.removeFromLeft (12);
    auto right = area;

    const auto deviceHeight = juce::jlimit (180, 200,
                                            juce::roundToInt (static_cast<float> (left.getHeight()) * 0.32f));
    deviceGroup.setBounds (left.removeFromTop (deviceHeight));
    left.removeFromTop (10);
    mappingGroup.setBounds (left);
    logGroup.setBounds (right);

    {
        auto inner = deviceGroup.getLocalBounds().reduced (14);
        inner.translate (deviceGroup.getX(), deviceGroup.getY());
        inner.removeFromTop (10);

        auto row = inner.removeFromTop (30);
        deviceLabel.setBounds (row.removeFromLeft (64));
        rescanButton.setBounds (row.removeFromRight (96));
        row.removeFromRight (8);
        activityLight.setBounds (row.removeFromRight (22).withSizeKeepingCentre (22, 22));
        row.removeFromRight (8);
        deviceBox.setBounds (row);

        inner.removeFromTop (8);
        row = inner.removeFromTop (30);
        profileLabel.setBounds (row.removeFromLeft (64));
        deleteProfileButton.setBounds (row.removeFromRight (82));
        row.removeFromRight (8);
        profileBox.setBounds (row);

        inner.removeFromTop (8);
        row = inner.removeFromTop (30);
        saveProfileButton.setBounds (row.removeFromRight (80));
        row.removeFromRight (8);
        profileNameEditor.setBounds (row);

        inner.removeFromTop (6);
        profileStatusLabel.setBounds (inner.removeFromTop (16));
        inner.removeFromTop (4);
        statusLabel.setBounds (inner.removeFromTop (18));
    }

    {
        auto inner = mappingGroup.getLocalBounds().reduced (14);
        inner.translate (mappingGroup.getX(), mappingGroup.getY());
        inner.removeFromTop (10);

        mappingHintLabel.setBounds (inner.removeFromTop (16));
        inner.removeFromTop (6);

        const auto rowHeight = juce::jlimit (22, 34,
                                             (inner.getHeight() - 4) / static_cast<int> (mappingRows.size()));

        for (auto& row : mappingRows)
        {
            auto rowArea = inner.removeFromTop (rowHeight);
            rowArea = rowArea.reduced (0, 1);
            row->actionLabel.setBounds (rowArea.removeFromLeft (150));
            row->clearButton.setBounds (rowArea.removeFromRight (74));
            rowArea.removeFromRight (8);
            row->learnButton.setBounds (rowArea.removeFromRight (94));
            rowArea.removeFromRight (10);
            row->mappingLabel.setBounds (rowArea);
        }
    }

    {
        auto inner = logGroup.getLocalBounds().reduced (14);
        inner.translate (logGroup.getX(), logGroup.getY());
        inner.removeFromTop (10);
        lastMessageLabel.setBounds (inner.removeFromTop (22));
        inner.removeFromTop (8);
        logEditor.setBounds (inner);
    }
}

void MidiPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());
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

    activityLight.setActive (activityHoldTicks > 0);

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
        styleLabel (row->mappingLabel, 11.5f);
        row->mappingLabel.setFont (DarkLookAndFeel::mono (11.0f));
        row->mappingLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

        row->actionLabel.setText (getMidiActionName (action), juce::dontSendNotification);
        row->mappingLabel.setText ("Not assigned", juce::dontSendNotification);

        row->learnButton.setButtonText ("Learn");
        row->clearButton.setButtonText ("Clear");
        row->learnButton.setTooltip ("Listen for the next footswitch press");
        row->clearButton.setTooltip ("Remove this mapping");

        DarkLookAndFeel::setButtonVariant (row->learnButton, "accent");
        DarkLookAndFeel::setButtonIcon (row->learnButton, "listen");
        DarkLookAndFeel::setButtonTextSize (row->learnButton, 11.5f);
        DarkLookAndFeel::setButtonVariant (row->clearButton, "ghost");
        DarkLookAndFeel::setButtonIcon (row->clearButton, "x");
        DarkLookAndFeel::setButtonTextSize (row->clearButton, 11.5f);

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
        DarkLookAndFeel::setButtonVariant (row->learnButton,
                                           isLearning ? "warning" : "accent");
        row->learnButton.setEnabled (hasDevice || isLearning);
        row->clearButton.setEnabled (mapping.has_value());
    }
}

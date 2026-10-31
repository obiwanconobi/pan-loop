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

    deviceLabel.setText ("Device", juce::dontSendNotification);
    statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
    lastMessageLabel.setText ("No MIDI activity yet.", juce::dontSendNotification);
    hintLabel.setText ("Select the Blackstar Live Logic above, then assign footswitches below.",
                       juce::dontSendNotification);
    hintLabel.setJustificationType (juce::Justification::topLeft);
    hintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

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
    addAndMakeVisible (logEditor);

    buildMappingRows();

    deviceBox.onChange = [this]
    {
        if (! updatingControls)
            updateSelection();
    };

    rescanButton.onClick = [this]
    {
        rebuildDeviceList();
    };

    rebuildDeviceList();
    updateMappingRows();
    startTimerHz (30);
}

void MidiPanel::resized()
{
    auto area = getLocalBounds().reduced (18);

    deviceGroup.setBounds (area.removeFromTop (190));
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
        statusLabel.setBounds (inner.removeFromTop (22));

        inner.removeFromTop (4);
        lastMessageLabel.setBounds (inner.removeFromTop (22));

        inner.removeFromTop (8);
        hintLabel.setBounds (inner.removeFromTop (46));
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

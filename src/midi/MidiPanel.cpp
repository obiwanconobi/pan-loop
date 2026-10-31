#include "MidiPanel.h"
#include "app/DarkLookAndFeel.h"

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
    logGroup.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
    logGroup.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());

    styleLabel (deviceLabel, 14.0f);
    styleLabel (statusLabel, 13.0f, true);
    styleLabel (lastMessageLabel, 13.0f);
    styleLabel (hintLabel, 12.0f);

    deviceLabel.setText ("Device", juce::dontSendNotification);
    statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
    lastMessageLabel.setText ("No MIDI activity yet.", juce::dontSendNotification);
    hintLabel.setText ("Select the Blackstar Live Logic above. MIDI Learn is added in Phase 5; "
                       "this page currently monitors incoming CC, Note and Program Change messages.",
                       juce::dontSendNotification);
    hintLabel.setJustificationType (juce::Justification::topLeft);
    hintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

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
    addAndMakeVisible (logGroup);
    addAndMakeVisible (deviceLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (lastMessageLabel);
    addAndMakeVisible (hintLabel);
    addAndMakeVisible (deviceBox);
    addAndMakeVisible (rescanButton);
    addAndMakeVisible (logEditor);

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
    startTimerHz (30);
}

void MidiPanel::resized()
{
    auto area = getLocalBounds().reduced (18);

    deviceGroup.setBounds (area.removeFromTop (190));
    area.removeFromTop (14);
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

    if (enabled.isEmpty() && statusLabel.getText() != "No MIDI input selected")
    {
        statusLabel.setText ("No MIDI input selected", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    }
    else if (! enabled.isEmpty() && ! statusLabel.getText().startsWith ("Connected:"))
    {
        statusLabel.setText ("Connected: " + deviceBox.getText(), juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
    }
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
        statusLabel.setText ("Connected: " + devices[index].name, juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
    }
}

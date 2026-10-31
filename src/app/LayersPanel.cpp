#include "LayersPanel.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioEngine.h"

#include <cmath>

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

LayersPanel::LayersPanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);

    styleLabel (titleLabel, 30.0f, true);
    styleLabel (subtitleLabel, 13.0f);
    styleLabel (statusLabel, 12.0f);
    styleLabel (totalLabel, 13.0f);
    styleLabel (selectionLabel, 13.0f, true);
    styleLabel (gainLabel, 12.0f);
    styleLabel (panLabel, 12.0f);
    styleLabel (emptyLabel, 15.0f);

    titleLabel.setText ("Layers", juce::dontSendNotification);
    subtitleLabel.setText ("Live loop channels and mix controls",
                           juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    statusLabel.setText ("Use Record, Stop Rec and Discard on the Live tab.",
                         juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    totalLabel.setText ("Layers: 0   |   Memory: 0 KB", juce::dontSendNotification);
    totalLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    selectionLabel.setText ("No layer selected", juce::dontSendNotification);
    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    panLabel.setText ("Pan", juce::dontSendNotification);
    panLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    emptyLabel.setJustificationType (juce::Justification::centred);
    emptyLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    emptyLabel.setInterceptsMouseClicks (false, false);
    emptyLabel.setText ("No live layers yet.\nUse Record on the Live tab to create one.",
                        juce::dontSendNotification);

    clearButton.onClick = [this]
    {
        engine.clearLiveLayers();
        selectedLayerId = -1;
        refreshSnapshot();
    };

    deleteButton.onClick = [this] { deleteSelectedLayer(); };

    muteButton.onClick = [this]
    {
        if (selectedLayerId >= 0)
            engine.getLiveLooper().setLayerMuted (selectedLayerId, muteButton.getToggleState());
    };

    soloButton.onClick = [this]
    {
        if (selectedLayerId >= 0)
            engine.getLiveLooper().setLayerSolo (selectedLayerId, soloButton.getToggleState());
    };

    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    gainSlider.setRange (0.0, 200.0, 1.0);
    gainSlider.setValue (100.0, juce::dontSendNotification);
    gainSlider.setTextValueSuffix (" %");
    gainSlider.setEnabled (false);
    gainSlider.onValueChange = [this]
    {
        if (selectedLayerId >= 0)
            engine.getLiveLooper().setLayerGain (selectedLayerId,
                                                 static_cast<float> (gainSlider.getValue() / 100.0));
    };

    panSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.setValue (0.0, juce::dontSendNotification);
    panSlider.setEnabled (false);
    panSlider.setTooltip ("Stereo balance for the selected layer. C is centre.");
    panSlider.textFromValueFunction = [] (double value)
    {
        const auto amount = juce::roundToInt (std::abs (value) * 100.0);

        if (amount == 0)
            return juce::String ("C");

        return juce::String (value < 0.0 ? "L " : "R ") + juce::String (amount);
    };
    panSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        const auto trimmed = text.trim().toLowerCase();

        if (trimmed == "c" || trimmed == "center" || trimmed == "centre")
            return 0.0;

        const auto left = trimmed.startsWith ("l");
        const auto right = trimmed.startsWith ("r");
        const auto number = trimmed.retainCharacters ("0123456789.").getDoubleValue();
        const auto normalised = juce::jlimit (0.0, 1.0,
                                              number > 1.0 ? number / 100.0 : number);

        if (left)
            return -normalised;

        if (right)
            return normalised;

        return juce::jlimit (-1.0, 1.0,
                             number > 1.0 ? number / 100.0 : number);
    };
    panSlider.onValueChange = [this]
    {
        if (selectedLayerId >= 0)
            engine.getLiveLooper().setLayerPan (selectedLayerId,
                                                static_cast<float> (panSlider.getValue()));
    };
    panSlider.updateText();

    listBox.setRowHeight (34);
    listBox.setColour (juce::ListBox::backgroundColourId, DarkLookAndFeel::panelAlt());
    listBox.setColour (juce::ListBox::outlineColourId, DarkLookAndFeel::outline());
    listBox.setOutlineThickness (1);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (totalLabel);
    addAndMakeVisible (selectionLabel);
    addAndMakeVisible (gainLabel);
    addAndMakeVisible (panLabel);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (deleteButton);
    addAndMakeVisible (muteButton);
    addAndMakeVisible (soloButton);
    addAndMakeVisible (gainSlider);
    addAndMakeVisible (panSlider);
    addAndMakeVisible (listBox);
    addAndMakeVisible (emptyLabel);

    refreshSnapshot();
    startTimerHz (15);
}

LayersPanel::~LayersPanel()
{
    stopTimer();
}

void LayersPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    if (! bottomBarArea.isEmpty())
    {
        g.setColour (DarkLookAndFeel::panel());
        g.fillRoundedRectangle (bottomBarArea.toFloat(), 8.0f);
        g.setColour (DarkLookAndFeel::outline());
        g.drawRoundedRectangle (bottomBarArea.toFloat().reduced (0.5f), 8.0f, 1.0f);
    }
}

void LayersPanel::resized()
{
    auto area = getLocalBounds().reduced (28);

    auto header = area.removeFromTop (56);
    auto titleArea = header.removeFromLeft (juce::jmin (700, header.getWidth() - 40));
    titleLabel.setBounds (titleArea.removeFromTop (34));
    subtitleLabel.setBounds (titleArea);

    area.removeFromTop (8);

    auto buttons = area.removeFromTop (36);
    clearButton.setBounds (buttons.removeFromLeft (130));

    area.removeFromTop (8);
    statusLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (8);

    bottomBarArea = area.removeFromBottom (124);
    area.removeFromBottom (10);

    listArea = area;
    listBox.setBounds (listArea);
    emptyLabel.setBounds (listArea);

    auto inner = bottomBarArea.reduced (18, 12);
    auto left = inner.removeFromLeft (inner.getWidth() / 2);
    totalLabel.setBounds (left.removeFromTop (22));
    selectionLabel.setBounds (left.removeFromTop (22));

    auto right = inner;
    auto gainRow = right.removeFromTop (28);
    gainLabel.setBounds (gainRow.removeFromLeft (56));
    gainSlider.setBounds (gainRow.removeFromLeft (220));
    gainRow.removeFromLeft (16);
    muteButton.setBounds (gainRow.removeFromLeft (80));
    gainRow.removeFromLeft (8);
    soloButton.setBounds (gainRow.removeFromLeft (80));
    gainRow.removeFromLeft (12);
    deleteButton.setBounds (gainRow.removeFromLeft (90));

    right.removeFromTop (8);
    auto panRow = right.removeFromTop (28);
    panLabel.setBounds (panRow.removeFromLeft (56));
    panSlider.setBounds (panRow.removeFromLeft (220));
}

int LayersPanel::getNumRows()
{
    return static_cast<int> (snapshot.layers.size());
}

void LayersPanel::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                    int width, int height, bool rowIsSelected)
{
    if (! juce::isPositiveAndBelow (rowNumber, static_cast<int> (snapshot.layers.size())))
        return;

    if (rowIsSelected)
    {
        g.setColour (DarkLookAndFeel::accent().withAlpha (0.18f));
        g.fillRect (0, 0, width, height);
    }

    const auto& layer = snapshot.layers[static_cast<size_t> (rowNumber)];

    g.setColour (DarkLookAndFeel::text());
    g.setFont (juce::Font (juce::FontOptions (14.0f)));
    g.drawText (layer.name, 14, 0, 240, height,
                juce::Justification::centredLeft, true);

    if (layer.armed)
    {
        g.setColour (DarkLookAndFeel::armed());
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText ("ARMED", 260, 0, 90, height,
                    juce::Justification::centredLeft, false);
    }
    else if (layer.recording)
    {
        g.setColour (DarkLookAndFeel::recording());
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText ("REC", 260, 0, 50, height,
                    juce::Justification::centredLeft, false);
    }

    g.setColour (DarkLookAndFeel::mutedText());
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (formatDuration (layer.durationSeconds),
                width - 112, 0, 100, height,
                juce::Justification::centredRight, false);

    int badgeX = width - 178;

    if (layer.muted)
    {
        g.setColour (DarkLookAndFeel::armed());
        g.fillRoundedRectangle (static_cast<float> (badgeX), 9.0f, 22.0f, 16.0f, 3.0f);
        g.setColour (DarkLookAndFeel::background());
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText ("M", badgeX, 9, 22, 16, juce::Justification::centred, false);
        badgeX += 28;
    }

    if (layer.solo)
    {
        g.setColour (DarkLookAndFeel::accent());
        g.fillRoundedRectangle (static_cast<float> (badgeX), 9.0f, 22.0f, 16.0f, 3.0f);
        g.setColour (DarkLookAndFeel::background());
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText ("S", badgeX, 9, 22, 16, juce::Justification::centred, false);
    }

    g.setColour (DarkLookAndFeel::outline().withAlpha (0.5f));
    g.fillRect (0, height - 1, width, 1);
}

void LayersPanel::selectedRowsChanged (int lastRowSelected)
{
    if (juce::isPositiveAndBelow (lastRowSelected, static_cast<int> (snapshot.layers.size())))
        selectedLayerId = snapshot.layers[static_cast<size_t> (lastRowSelected)].id;
    else
        selectedLayerId = -1;

    updateSelectionControls();
    updateControls();
}

void LayersPanel::deleteKeyPressed (int lastRowSelected)
{
    selectedRowsChanged (lastRowSelected);
    deleteSelectedLayer();
}

void LayersPanel::timerCallback()
{
    engine.getLiveLooper().trimFinishedLayers();
    engine.getLiveLooper().reapEmptyLayers();
    refreshSnapshot();
}

void LayersPanel::refreshSnapshot()
{
    snapshot = engine.getLiveLooper().getSnapshot();
    const auto count = static_cast<int> (snapshot.layers.size());

    bool selectionStillPresent = false;

    for (const auto& layer : snapshot.layers)
        if (layer.id == selectedLayerId)
            selectionStillPresent = true;

    if (! selectionStillPresent)
    {
        if (count > 0)
            selectedLayerId = snapshot.layers.back().id;
        else
            selectedLayerId = -1;
    }

    if (count != lastLayerCount)
    {
        lastLayerCount = count;
        listBox.updateContent();
    }

    int selectedRow = -1;

    for (int i = 0; i < count; ++i)
        if (snapshot.layers[static_cast<size_t> (i)].id == selectedLayerId)
            selectedRow = i;

    if (selectedRow >= 0)
        listBox.selectRow (selectedRow, true, true);
    else
        listBox.deselectAllRows();

    listBox.repaint();

    totalLabel.setText ("Layers: " + juce::String (count)
                            + "   |   Memory: " + formatBytes (snapshot.totalMemoryUsageBytes),
                        juce::dontSendNotification);

    emptyLabel.setVisible (count == 0);

    updateSelectionControls();
    updateControls();
}

void LayersPanel::updateControls()
{
    const auto hasSelection = selectedLayerId >= 0;
    const auto liveBusy = engine.getLiveLooper().isRecordingOrArmed();

    clearButton.setEnabled (! liveBusy && ! snapshot.layers.empty());
    deleteButton.setEnabled (hasSelection);
    muteButton.setEnabled (hasSelection);
    soloButton.setEnabled (hasSelection);
    gainSlider.setEnabled (hasSelection);
    panSlider.setEnabled (hasSelection);

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Use Record, Stop Rec and Discard on the Live tab.",
                         juce::dontSendNotification);
}

void LayersPanel::updateSelectionControls()
{
    if (selectedLayerId < 0)
    {
        selectionLabel.setText ("No layer selected", juce::dontSendNotification);
        muteButton.setToggleState (false, juce::dontSendNotification);
        soloButton.setToggleState (false, juce::dontSendNotification);
        gainSlider.setValue (100.0, juce::dontSendNotification);
        panSlider.setValue (0.0, juce::dontSendNotification);
        panSlider.updateText();
        return;
    }

    for (const auto& layer : snapshot.layers)
    {
        if (layer.id != selectedLayerId)
            continue;

        selectionLabel.setText (layer.name, juce::dontSendNotification);
        muteButton.setToggleState (layer.muted, juce::dontSendNotification);
        soloButton.setToggleState (layer.solo, juce::dontSendNotification);

        if (! gainSlider.isMouseButtonDown())
            gainSlider.setValue (layer.gain * 100.0, juce::dontSendNotification);

        if (! panSlider.isMouseButtonDown() && ! panSlider.hasKeyboardFocus (true))
        {
            panSlider.setValue (layer.pan, juce::dontSendNotification);
            panSlider.updateText();
        }

        return;
    }
}

void LayersPanel::deleteSelectedLayer()
{
    if (selectedLayerId < 0)
        return;

    engine.removeLiveLayer (selectedLayerId);
    selectedLayerId = -1;
    refreshSnapshot();
}

juce::String LayersPanel::formatDuration (double seconds)
{
    const auto totalMs = static_cast<juce::int64> (std::llround (juce::jmax (0.0, seconds) * 1000.0));
    const auto minutes = totalMs / 60000;
    const auto secs = (totalMs / 1000) % 60;
    const auto ms = totalMs % 1000;

    return juce::String (minutes) + ":"
         + juce::String (secs).paddedLeft ('0', 2) + "."
         + juce::String (ms).paddedLeft ('0', 3);
}

juce::String LayersPanel::formatBytes (int bytes)
{
    if (bytes < 1024 * 1024)
        return juce::String (static_cast<double> (bytes) / 1024.0, 1) + " KB";

    return juce::String (static_cast<double> (bytes) / (1024.0 * 1024.0), 1) + " MB";
}

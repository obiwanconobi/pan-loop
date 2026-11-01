#include "LayersPanel.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioEngine.h"

#include <array>
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

    styleLabel (titleLabel, 28.0f, true);
    styleLabel (subtitleLabel, 12.5f);
    styleLabel (statusLabel, 12.0f);
    styleLabel (totalLabel, 11.5f, true);
    styleLabel (selectionLabel, 17.0f, true);
    styleLabel (gainLabel, 10.5f, true);
    styleLabel (panLabel, 10.5f, true);
    styleLabel (emptyLabel, 13.5f);

    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.02f));
    subtitleLabel.setFont (DarkLookAndFeel::sans (12.5f));
    statusLabel.setFont (DarkLookAndFeel::mono (11.5f));
    totalLabel.setFont (DarkLookAndFeel::mono (11.5f, true));
    totalLabel.setJustificationType (juce::Justification::centred);
    gainLabel.setFont (DarkLookAndFeel::kicker (10.5f));
    panLabel.setFont (DarkLookAndFeel::kicker (10.5f));

    titleLabel.setText ("Layers", juce::dontSendNotification);
    subtitleLabel.setText ("Live loop channels and mix controls",
                           juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    statusLabel.setText ("Use Record, Stop Rec and Discard on the Live tab.",
                         juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    totalLabel.setText ("0 layers   |   0 KB", juce::dontSendNotification);
    totalLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    selectionLabel.setText ("No layer selected", juce::dontSendNotification);
    selectionLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
    gainLabel.setText ("GAIN", juce::dontSendNotification);
    gainLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    panLabel.setText ("PAN", juce::dontSendNotification);
    panLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    emptyLabel.setJustificationType (juce::Justification::centred);
    emptyLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    emptyLabel.setInterceptsMouseClicks (false, false);
    emptyLabel.setText ("No live layers yet.\nUse Record on the Live tab to create one.",
                        juce::dontSendNotification);

    DarkLookAndFeel::setButtonVariant (clearButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (clearButton, "trash");
    DarkLookAndFeel::setButtonTextSize (clearButton, 12.5f);
    clearButton.setTooltip ("Remove every live layer");

    clearButton.onClick = [this]
    {
        engine.clearLiveLayers();
        selectedLayerId = -1;
        refreshSnapshot();
    };

    DarkLookAndFeel::setButtonVariant (deleteButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (deleteButton, "trash");
    DarkLookAndFeel::setButtonTextSize (deleteButton, 12.5f);
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

    listBox.setRowHeight (48);
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    listBox.setOutlineThickness (0);

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

    if (! statsPillArea.isEmpty())
        DarkLookAndFeel::drawPill (g, statsPillArea,
                                   DarkLookAndFeel::panelAlt().withAlpha (0.72f),
                                   DarkLookAndFeel::outline(), 9.0f);

    DarkLookAndFeel::drawCard (g, listCardArea);
    DarkLookAndFeel::drawCard (g, bottomBarArea);

    if (snapshot.layers.empty() && ! emptyIconArea.isEmpty())
    {
        g.setColour (DarkLookAndFeel::panelAlt());
        g.fillEllipse (emptyIconArea.toFloat());
        g.setColour (DarkLookAndFeel::outline());
        g.drawEllipse (emptyIconArea.toFloat().reduced (0.5f), 1.0f);

        DarkLookAndFeel::drawIcon (g, "layers", emptyIconArea.toFloat().reduced (17.0f),
                                   DarkLookAndFeel::accent(), 1.8f);
    }

    if (! bottomBarArea.isEmpty())
    {
        auto inner = bottomBarArea.reduced (20, 14);

        g.setColour (DarkLookAndFeel::mutedText());
        g.setFont (DarkLookAndFeel::kicker (10.0f));
        g.drawText ("SELECTED LAYER", inner.getX(), inner.getY(),
                    280, 14, juce::Justification::centredLeft, false);

        g.setFont (DarkLookAndFeel::sans (11.0f));
        g.setColour (DarkLookAndFeel::mutedText().withAlpha (0.8f));
        g.drawText ("Mute, solo, gain and pan are saved with the session.",
                    inner.getX(), bottomBarArea.getBottom() - 24,
                    juce::jmax (240, inner.getWidth() / 3), 16,
                    juce::Justification::centredLeft, false);
    }
}

void LayersPanel::resized()
{
    auto area = getLocalBounds().reduced (24);

    auto header = area.removeFromTop (54);
    clearButton.setBounds (header.removeFromRight (150).withSizeKeepingCentre (150, 38));
    header.removeFromRight (10);
    statsPillArea = header.removeFromRight (210).withSizeKeepingCentre (210, 30);
    totalLabel.setBounds (statsPillArea.reduced (10, 0));

    auto titleArea = header;
    titleLabel.setBounds (titleArea.removeFromTop (32));
    subtitleLabel.setBounds (titleArea);

    area.removeFromTop (8);
    statusLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (8);

    bottomBarArea = area.removeFromBottom (148);
    area.removeFromBottom (10);
    listCardArea = area;

    listArea = listCardArea.reduced (8);
    listBox.setBounds (listArea);

    emptyIconArea = juce::Rectangle<int> (56, 56)
                        .withCentre ({ listCardArea.getCentreX(),
                                       listCardArea.getCentreY() - 28 });
    emptyLabel.setBounds (listCardArea.withTrimmedTop (listCardArea.getHeight() / 2 + 8)
                                      .withTrimmedBottom (20)
                                      .reduced (30, 0));

    auto inner = bottomBarArea.reduced (20, 12);
    inner.removeFromTop (16);

    auto left = inner.removeFromLeft (juce::roundToInt (inner.getWidth() * 0.34f));
    selectionLabel.setBounds (left.removeFromTop (28));

    inner.removeFromLeft (12);
    auto right = inner;
    right.removeFromTop (juce::jmax (0, (right.getHeight() - 76) / 2));

    auto gainRow = right.removeFromTop (34);
    gainLabel.setBounds (gainRow.removeFromLeft (44));

    deleteButton.setBounds (gainRow.removeFromRight (96));
    gainRow.removeFromRight (8);
    soloButton.setBounds (gainRow.removeFromRight (82));
    gainRow.removeFromRight (8);
    muteButton.setBounds (gainRow.removeFromRight (82));
    gainRow.removeFromRight (10);
    gainSlider.setBounds (gainRow);

    right.removeFromTop (8);
    auto panRow = right.removeFromTop (34);
    panLabel.setBounds (panRow.removeFromLeft (44));
    panSlider.setBounds (panRow.removeFromLeft (juce::jmin (panRow.getWidth(), 420)));
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

    const auto& layer = snapshot.layers[static_cast<size_t> (rowNumber)];

    if (rowIsSelected)
    {
        g.setColour (DarkLookAndFeel::accent().withAlpha (0.13f));
        g.fillRoundedRectangle (juce::Rectangle<float> (6.0f, 2.0f,
                                                        static_cast<float> (width - 12),
                                                        static_cast<float> (height - 4)), 8.0f);
    }
    else if ((rowNumber % 2) == 1)
    {
        g.setColour (juce::Colours::white.withAlpha (0.012f));
        g.fillRect (6, 2, width - 12, height - 4);
    }

    const std::array<juce::Colour, 5> palette
    {
        DarkLookAndFeel::accent(),
        DarkLookAndFeel::violet(),
        DarkLookAndFeel::pink(),
        DarkLookAndFeel::armed(),
        DarkLookAndFeel::playing()
    };

    const auto stripeColour = layer.recording
                                ? DarkLookAndFeel::recording()
                                : layer.armed
                                    ? DarkLookAndFeel::armed()
                                    : palette[static_cast<size_t> (std::abs (layer.id)) % palette.size()];

    g.setColour (stripeColour);
    g.fillRoundedRectangle (16.0f, static_cast<float> (height) * 0.5f - 14.0f,
                            3.0f, 28.0f, 1.5f);

    g.setColour (DarkLookAndFeel::text().withAlpha (layer.muted ? 0.45f : 1.0f));
    g.setFont (DarkLookAndFeel::sans (14.0f, rowIsSelected || layer.recording));
    g.drawFittedText (layer.name, 30, 0, width - 480, height,
                      juce::Justification::centredLeft, 1, 0.92f);

    auto badgeX = width - 430;
    const auto drawBadge = [&g, &badgeX] (const juce::String& label, juce::Colour colour)
    {
        const auto badgeWidth = juce::jmax (34.0f,
                                            18.0f + juce::GlyphArrangement::getStringWidth (
                                                       DarkLookAndFeel::mono (9.0f, true), label));
        auto badge = juce::Rectangle<float> (static_cast<float> (badgeX), 0.0f,
                                              badgeWidth, 18.0f);

        g.setColour (colour.withAlpha (0.16f));
        g.fillRoundedRectangle (badge.withY (15.0f), 4.0f);
        g.setColour (colour);
        g.setFont (DarkLookAndFeel::mono (9.0f, true));
        g.drawText (label, badge.withY (15.0f), juce::Justification::centred, false);

        badgeX += static_cast<int> (badgeWidth) + 6;
    };

    if (layer.recording)
        drawBadge ("REC", DarkLookAndFeel::recording());
    else if (layer.armed)
        drawBadge ("ARMED", DarkLookAndFeel::armed());

    if (layer.muted)
        drawBadge ("MUTE", DarkLookAndFeel::armed());

    if (layer.solo)
        drawBadge ("SOLO", DarkLookAndFeel::accent());

    const auto panAmount = juce::roundToInt (std::abs (layer.pan) * 100.0f);
    const auto panText = panAmount == 0
                           ? juce::String ("C")
                           : juce::String (layer.pan < 0.0f ? "L " : "R ") + juce::String (panAmount);

    g.setColour (DarkLookAndFeel::mutedText());
    g.setFont (DarkLookAndFeel::mono (10.5f));
    g.drawText (juce::String (juce::roundToInt (layer.gain * 100.0f)) + "%   " + panText,
                width - 300, 0, 180, height,
                juce::Justification::centredRight, false);

    g.setColour (DarkLookAndFeel::mutedText());
    g.setFont (DarkLookAndFeel::mono (11.5f));
    g.drawText (formatDuration (layer.durationSeconds),
                width - 108, 0, 90, height,
                juce::Justification::centredRight, false);

    g.setColour (DarkLookAndFeel::outlineSoft().withAlpha (0.6f));
    g.fillRect (16, height - 1, width - 32, 1);
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

    totalLabel.setText (juce::String (count)
                            + (count == 1 ? " layer" : " layers")
                            + "   |   " + formatBytes (snapshot.totalMemoryUsageBytes),
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

#include "LivePanel.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioEngine.h"
#include "audio/Metronome.h"
#include "audio/Transport.h"

#include <cmath>

namespace
{
    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        DarkLookAndFeel::styleLabel (label, height, bold);
    }
}

LivePanel::LivePanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    styleLabel (titleLabel, 28.0f, true);
    styleLabel (subtitleLabel, 12.5f);
    styleLabel (stateLabel, 11.5f, true);
    styleLabel (barBeatLabel, 48.0f, true);
    styleLabel (timeLabel, 14.0f);
    styleLabel (latencyLabel, 11.5f);
    styleLabel (cycleLabel, 11.5f);
    styleLabel (transportHintLabel, 11.0f);
    styleLabel (loopTitleLabel, 11.5f, true);
    styleLabel (loopHintLabel, 13.5f);
    styleLabel (bpmLabel, 10.5f, true);
    styleLabel (timeSigLabel, 10.5f, true);
    styleLabel (metroLevelLabel, 10.5f, true);

    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.02f));
    subtitleLabel.setFont (DarkLookAndFeel::sans (12.5f));
    stateLabel.setFont (DarkLookAndFeel::kicker (11.5f));
    barBeatLabel.setFont (DarkLookAndFeel::mono (48.0f, true));
    timeLabel.setFont (DarkLookAndFeel::mono (14.0f));
    latencyLabel.setFont (DarkLookAndFeel::mono (11.5f));
    cycleLabel.setFont (DarkLookAndFeel::mono (11.5f));
    transportHintLabel.setFont (DarkLookAndFeel::mono (11.0f));
    loopTitleLabel.setFont (DarkLookAndFeel::kicker (11.5f));
    bpmLabel.setFont (DarkLookAndFeel::kicker (10.5f));
    timeSigLabel.setFont (DarkLookAndFeel::kicker (10.5f));
    metroLevelLabel.setFont (DarkLookAndFeel::kicker (10.5f));

    titleLabel.setText ("Live", juce::dontSendNotification);
    subtitleLabel.setText ("Transport, tempo and loop layers",
                           juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    stateLabel.setText ("STOPPED", juce::dontSendNotification);
    stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    stateLabel.setJustificationType (juce::Justification::centred);

    barBeatLabel.setText ("BAR 1.1", juce::dontSendNotification);
    timeLabel.setText ("00:00.000   |   120.0 BPM   |   4/4", juce::dontSendNotification);
    timeLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    latencyLabel.setText ("Round-trip latency: 0.0 ms", juce::dontSendNotification);
    latencyLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    cycleLabel.setText ("Cycle: free-running", juce::dontSendNotification);
    cycleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    transportHintLabel.setText ("SPACE play/pause   |   S stop   |   R record   |   T stop rec   |   D discard",
                                juce::dontSendNotification);
    transportHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    loopTitleLabel.setText ("LOOP RECORDING", juce::dontSendNotification);
    loopTitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    loopHintLabel.setText ("No live layers yet. Record starts the first loop.",
                           juce::dontSendNotification);
    loopHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    timeSigLabel.setText ("TIME SIGNATURE", juce::dontSendNotification);
    timeSigLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    metroLevelLabel.setText ("LEVEL", juce::dontSendNotification);
    metroLevelLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    playPauseButton.setClickingTogglesState (false);
    playPauseButton.setTooltip ("Play or pause the transport (Space)");
    DarkLookAndFeel::setButtonVariant (playPauseButton, "primary");
    DarkLookAndFeel::setButtonIcon (playPauseButton, "play");
    DarkLookAndFeel::setButtonTextSize (playPauseButton, 18.0f);
    playPauseButton.onClick = [this] { togglePlayPause(); };

    stopButton.setTooltip ("Stop and rewind to bar 1 (S)");
    DarkLookAndFeel::setButtonVariant (stopButton, "ghost");
    DarkLookAndFeel::setButtonIcon (stopButton, "stop");
    DarkLookAndFeel::setButtonTextSize (stopButton, 14.0f);
    stopButton.onClick = [this] { stopTransport(); };

    tapButton.setTooltip ("Tap four times to set the tempo");
    DarkLookAndFeel::setButtonVariant (tapButton, "accent");
    DarkLookAndFeel::setButtonIcon (tapButton, "tap");
    DarkLookAndFeel::setButtonTextSize (tapButton, 13.0f);
    tapButton.onClick = [this] { tapTempo(); };

    bpmSlider.setSliderStyle (juce::Slider::IncDecButtons);
    bpmSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 72, 30);
    bpmSlider.setRange (40.0, 240.0, 0.1);
    bpmSlider.setNumDecimalPlacesToDisplay (1);
    bpmSlider.setValue (engine.getTransport().getBpm(), juce::dontSendNotification);
    bpmSlider.onValueChange = [this] { updateBpm(); };

    timeSigBox.addItem ("4/4", 1);
    timeSigBox.addItem ("3/4", 2);
    timeSigBox.addItem ("2/4", 3);
    timeSigBox.addItem ("6/8", 4);
    timeSigBox.addItem ("5/4", 5);
    timeSigBox.addItem ("7/8", 6);
    timeSigBox.setSelectedId (1, juce::dontSendNotification);
    timeSigBox.onChange = [this] { updateTimeSignature(); };

    metroToggle.setToggleState (engine.getMetronome().isEnabled(), juce::dontSendNotification);
    metroToggle.setTooltip ("Enable the metronome click");
    metroToggle.onClick = [this]
    {
        engine.getMetronome().setEnabled (metroToggle.getToggleState());
    };

    metroLevelSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    metroLevelSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 24);
    metroLevelSlider.setRange (0.0, 100.0, 1.0);
    metroLevelSlider.setValue (engine.getMetronome().getLevel() * 100.0,
                               juce::dontSendNotification);
    metroLevelSlider.setTextValueSuffix (" %");
    metroLevelSlider.onValueChange = [this]
    {
        engine.getMetronome().setLevel (static_cast<float> (metroLevelSlider.getValue() / 100.0));
    };

    recordButton.setTooltip ("Start recording a new loop layer (R)");
    stopRecButton.setTooltip ("Stop recording and keep the take (T)");
    discardButton.setTooltip ("Discard the current take or most recent layer (D)");

    DarkLookAndFeel::setButtonVariant (recordButton, "danger");
    DarkLookAndFeel::setButtonIcon (recordButton, "record");
    DarkLookAndFeel::setButtonTextSize (recordButton, 16.0f);

    DarkLookAndFeel::setButtonVariant (stopRecButton, "warning");
    DarkLookAndFeel::setButtonIcon (stopRecButton, "stop");
    DarkLookAndFeel::setButtonTextSize (stopRecButton, 16.0f);

    DarkLookAndFeel::setButtonVariant (discardButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (discardButton, "trash");
    DarkLookAndFeel::setButtonTextSize (discardButton, 16.0f);

    recordButton.onClick = [this] { startRecording(); };
    stopRecButton.onClick = [this] { stopRecording(); };
    discardButton.onClick = [this] { discardLayer(); };

    barSnapToggle.setTooltip ("Snap the first loop length to the nearest bar");
    barSnapToggle.setToggleState (engine.isBarSnapEnabled(), juce::dontSendNotification);
    barSnapToggle.onClick = [this]
    {
        engine.setBarSnapEnabled (barSnapToggle.getToggleState());
    };

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (stateLabel);
    addAndMakeVisible (barBeatLabel);
    addAndMakeVisible (timeLabel);
    addAndMakeVisible (latencyLabel);
    addAndMakeVisible (cycleLabel);
    addAndMakeVisible (transportHintLabel);
    addAndMakeVisible (playPauseButton);
    addAndMakeVisible (stopButton);
    addAndMakeVisible (tapButton);
    addAndMakeVisible (bpmLabel);
    addAndMakeVisible (bpmSlider);
    addAndMakeVisible (timeSigLabel);
    addAndMakeVisible (timeSigBox);
    addAndMakeVisible (metroToggle);
    addAndMakeVisible (metroLevelLabel);
    addAndMakeVisible (metroLevelSlider);
    addAndMakeVisible (loopTitleLabel);
    addAndMakeVisible (loopHintLabel);
    addAndMakeVisible (barSnapToggle);
    addAndMakeVisible (recordButton);
    addAndMakeVisible (stopRecButton);
    addAndMakeVisible (discardButton);

    updateControls();
    startTimerHz (30);
}

void LivePanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    DarkLookAndFeel::drawCard (g, transportCard, true);
    DarkLookAndFeel::drawCard (g, tempoCard);
    DarkLookAndFeel::drawCard (g, loopCard);

    const auto drawKicker = [&g] (juce::Rectangle<int> card, const juce::String& text)
    {
        if (card.isEmpty())
            return;

        g.setColour (DarkLookAndFeel::mutedText());
        g.setFont (DarkLookAndFeel::kicker (10.5f));
        g.drawText (text, card.getX() + 20, card.getY() + 12,
                    card.getWidth() - 40, 16,
                    juce::Justification::centredLeft, false);
    };

    drawKicker (transportCard, "TRANSPORT");
    drawKicker (tempoCard, "TEMPO / CLICK");

    if (! statePillArea.isEmpty())
    {
        const auto stateColour = stateLabel.findColour (juce::Label::textColourId);

        DarkLookAndFeel::drawPill (g, statePillArea,
                                   stateColour.withAlpha (0.12f),
                                   stateColour.withAlpha (0.38f));

        g.setColour (stateColour);
        g.fillEllipse (static_cast<float> (statePillArea.getX() + 12),
                       static_cast<float> (statePillArea.getCentreY() - 3),
                       6.0f, 6.0f);
    }

    if (! loopStatusDotArea.isEmpty())
    {
        const auto colour = loopHintLabel.findColour (juce::Label::textColourId);

        g.setColour (colour.withAlpha (0.22f));
        g.fillEllipse (loopStatusDotArea.toFloat().expanded (3.0f));

        g.setColour (colour);
        g.fillEllipse (loopStatusDotArea.toFloat());
    }

    const auto info = engine.getTransport().getPositionInfo();

    const auto beats = juce::jmax (1, info.beatsPerBar);
    const auto diameter = juce::jmin (14.0f, static_cast<float> (beatIndicatorArea.getHeight()) * 0.62f);
    const auto gap = 22.0f;
    const auto totalWidth = static_cast<float> (beats) * diameter
                          + static_cast<float> (beats - 1) * gap;

    auto indicator = beatIndicatorArea.toFloat();
    auto x = indicator.getCentreX() - totalWidth * 0.5f;

    for (int i = 0; i < beats; ++i)
    {
        const juce::Rectangle<float> circle (x,
                                             indicator.getCentreY() - diameter * 0.5f,
                                             diameter,
                                             diameter);

        const auto isCurrent = i == info.beat - 1;
        const auto isActive = info.state != Transport::State::Stopped && isCurrent;

        if (isActive)
        {
            const auto colour = i == 0 ? DarkLookAndFeel::armed() : DarkLookAndFeel::playing();

            g.setColour (colour.withAlpha (0.18f));
            g.fillEllipse (circle.expanded (5.0f));

            g.setColour (colour);
            g.fillEllipse (circle);
        }
        else
        {
            g.setColour (DarkLookAndFeel::outline().withAlpha (0.9f));
            g.drawEllipse (circle, 1.5f);
        }

        x += diameter + gap;
    }

    auto bar = barProgressArea.toFloat();
    g.setColour (DarkLookAndFeel::panelAlt());
    g.fillRoundedRectangle (bar, 4.0f);

    if (info.state != Transport::State::Stopped)
    {
        const auto filledWidth = bar.getWidth() * static_cast<float> (juce::jlimit (0.0, 1.0, info.barPhase));

        if (filledWidth > 1.0f)
        {
            const auto colour = info.state == Transport::State::Playing
                                  ? DarkLookAndFeel::accent()
                                  : DarkLookAndFeel::armed();

            juce::ColourGradient gradient (DarkLookAndFeel::accentDeep(), bar.getTopLeft(),
                                           colour, bar.getTopRight(), false);
            g.setGradientFill (gradient);
            g.fillRoundedRectangle (bar.withWidth (filledWidth), 4.0f);

            g.setColour (colour.withAlpha (0.20f));
            g.fillEllipse (bar.getX() + filledWidth - 7.0f, bar.getCentreY() - 7.0f,
                           14.0f, 14.0f);

            g.setColour (colour);
            g.fillEllipse (bar.getX() + filledWidth - 2.5f, bar.getCentreY() - 2.5f,
                           5.0f, 5.0f);
        }
    }
}

void LivePanel::resized()
{
    auto area = getLocalBounds().reduced (24);

    auto header = area.removeFromTop (54);
    auto titleArea = header.removeFromLeft (juce::jmin (560, juce::jmax (300, header.getWidth() - 170)));
    titleLabel.setBounds (titleArea.removeFromTop (32));
    subtitleLabel.setBounds (titleArea);

    statePillArea = header.removeFromRight (150).withSizeKeepingCentre (150, 32);
    stateLabel.setBounds (statePillArea.withTrimmedLeft (22).withTrimmedRight (6));

    area.removeFromTop (12);

    const auto topHeight = juce::jmin (juce::jlimit (230, 340, juce::roundToInt (area.getHeight() * 0.60f)),
                                       juce::jmax (0, area.getHeight() - 120));
    auto top = area.removeFromTop (topHeight);
    const auto transportWidth = juce::roundToInt (static_cast<float> (top.getWidth()) * 0.62f);

    transportCard = top.removeFromLeft (transportWidth);
    top.removeFromLeft (14);
    tempoCard = top;

    area.removeFromTop (14);
    loopCard = area;

    {
        auto inner = transportCard.reduced (20);
        inner.removeFromTop (26);
        inner.removeFromTop (juce::jmax (0, (inner.getHeight() - 232) / 2));

        const auto controlsHeight = juce::jlimit (48, 68,
                                                   juce::roundToInt (inner.getHeight() * 0.26f));
        auto controls = inner.removeFromTop (controlsHeight);
        auto playArea = controls.removeFromLeft (juce::roundToInt (controls.getWidth() * 0.62f));
        playPauseButton.setBounds (playArea);
        controls.removeFromLeft (10);
        stopButton.setBounds (controls);

        inner.removeFromTop (8);

        const auto barHeight = juce::jlimit (36, 58, inner.getHeight() - 98);
        barBeatLabel.setBounds (inner.removeFromTop (barHeight));
        timeLabel.setBounds (inner.removeFromTop (20));
        inner.removeFromTop (4);
        beatIndicatorArea = inner.removeFromTop (26);
        inner.removeFromTop (4);
        barProgressArea = inner.removeFromTop (8);
        inner.removeFromTop (8);

        auto stats = inner.removeFromTop (20);
        latencyLabel.setBounds (stats.removeFromLeft (juce::jmin (250, stats.getWidth() / 2)));
        cycleLabel.setBounds (stats);
    }

    {
        auto inner = tempoCard.reduced (20);
        inner.removeFromTop (26);
        inner.removeFromTop (juce::jmax (0, (inner.getHeight() - 184) / 2));

        bpmLabel.setBounds (inner.removeFromTop (14));
        inner.removeFromTop (4);

        auto bpmRow = inner.removeFromTop (34);
        tapButton.setBounds (bpmRow.removeFromRight (78));
        bpmRow.removeFromRight (8);
        bpmSlider.setBounds (bpmRow);

        inner.removeFromTop (12);
        timeSigLabel.setBounds (inner.removeFromTop (14));
        inner.removeFromTop (4);
        timeSigBox.setBounds (inner.removeFromTop (30).removeFromLeft (juce::jmin (130, inner.getWidth())));

        inner.removeFromTop (12);
        metroToggle.setBounds (inner.removeFromTop (28));

        inner.removeFromTop (8);
        auto levelRow = inner.removeFromTop (24);
        metroLevelLabel.setBounds (levelRow.removeFromLeft (46));
        metroLevelSlider.setBounds (levelRow);
    }

    {
        auto inner = loopCard.reduced (20);
        auto titleRow = inner.removeFromTop (22);
        barSnapToggle.setBounds (titleRow.removeFromRight (juce::jmin (260, titleRow.getWidth() / 2)));
        loopTitleLabel.setBounds (titleRow);

        inner.removeFromTop (4);

        auto statusRow = inner.removeFromTop (20);
        loopStatusDotArea = statusRow.removeFromLeft (10).withSizeKeepingCentre (7, 7);
        loopHintLabel.setBounds (statusRow);

        transportHintLabel.setBounds (inner.removeFromBottom (20));
        inner.removeFromTop (6);

        auto buttons = inner;
        const auto gap = 10;
        const auto availableHeight = juce::jmin (96, buttons.getHeight());
        buttons = buttons.withSizeKeepingCentre (buttons.getWidth(), availableHeight);
        const auto available = juce::jmax (0, buttons.getWidth() - gap * 2);
        const auto recordWidth = juce::roundToInt (static_cast<float> (available) * 0.40f);
        const auto stopWidth = juce::roundToInt (static_cast<float> (available) * 0.32f);

        recordButton.setBounds (buttons.removeFromLeft (recordWidth));
        buttons.removeFromLeft (gap);
        stopRecButton.setBounds (buttons.removeFromLeft (stopWidth));
        buttons.removeFromLeft (gap);
        discardButton.setBounds (buttons);
    }
}

bool LivePanel::keyPressed (const juce::KeyPress& key)
{
    const auto liveBusy = engine.getLiveLooper().isRecordingOrArmed();

    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        if (! liveBusy)
            togglePlayPause();

        return true;
    }

    const auto character = key.getTextCharacter();

    if (character == 's' || character == 'S')
    {
        if (! liveBusy)
            stopTransport();

        return true;
    }

    if (character == 'r' || character == 'R')
    {
        startRecording();
        return true;
    }

    if (character == 't' || character == 'T')
    {
        stopRecording();
        return true;
    }

    if (character == 'd' || character == 'D')
    {
        discardLayer();
        return true;
    }

    return false;
}

void LivePanel::visibilityChanged()
{
    if (! isVisible())
        return;

    juce::Component::SafePointer<LivePanel> safeThis (this);

    juce::MessageManager::callAsync ([safeThis]
    {
        if (safeThis != nullptr && safeThis->isShowing())
            safeThis->grabKeyboardFocus();
    });
}

void LivePanel::timerCallback()
{
    updateControls();
}

void LivePanel::togglePlayPause()
{
    engine.getTransport().togglePlayPause();
    updateControls();
}

void LivePanel::stopTransport()
{
    engine.getTransport().stop();
    updateControls();
}

void LivePanel::startRecording()
{
    engine.record();
    updateControls();
}

void LivePanel::stopRecording()
{
    engine.stopRecording();
    updateControls();
}

void LivePanel::discardLayer()
{
    engine.discardLiveLayer();
    updateControls();
}

void LivePanel::updateBpm()
{
    engine.getTransport().setBpm (bpmSlider.getValue());
}

void LivePanel::updateTimeSignature()
{
    const auto text = timeSigBox.getText();
    const auto slash = text.indexOfChar ('/');

    if (slash <= 0)
        return;

    const auto numerator = text.substring (0, slash).getIntValue();
    const auto denominator = text.substring (slash + 1).getIntValue();

    if (numerator > 0 && denominator > 0)
        engine.getTransport().setTimeSignature (numerator, denominator);
}

void LivePanel::tapTempo()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (lastTapTimeMs > 0.0)
    {
        const auto interval = now - lastTapTimeMs;

        if (interval >= 200.0 && interval <= 2000.0)
        {
            tapIntervals.add (interval);

            while (tapIntervals.size() > 5)
                tapIntervals.remove (0);
        }
        else if (interval > 2000.0)
        {
            tapIntervals.clear();
        }
    }

    lastTapTimeMs = now;

    if (tapIntervals.isEmpty())
        return;

    double total = 0.0;
    for (const auto interval : tapIntervals)
        total += interval;

    const auto bpm = 60000.0 / (total / static_cast<double> (tapIntervals.size()));

    bpmSlider.setValue (juce::jlimit (40.0, 240.0, bpm), juce::sendNotificationSync);
}

void LivePanel::updateControls()
{
    const auto& transport = engine.getTransport();
    const auto info = transport.getPositionInfo();
    const auto playing = info.state == Transport::State::Playing;

    const juce::String playText (playing ? "Pause" : "Play");

    if (playPauseButton.getButtonText() != playText)
        playPauseButton.setButtonText (playText);

    DarkLookAndFeel::setButtonVariant (playPauseButton, playing ? "success" : "primary");
    DarkLookAndFeel::setButtonIcon (playPauseButton, playing ? "pause" : "play");

    if (metroToggle.getToggleState() != engine.getMetronome().isEnabled())
        metroToggle.setToggleState (engine.getMetronome().isEnabled(),
                                    juce::dontSendNotification);

    if (! bpmSlider.isMouseButtonDown()
        && ! bpmSlider.hasKeyboardFocus (true)
        && std::abs (bpmSlider.getValue() - transport.getBpm()) > 0.05)
    {
        bpmSlider.setValue (transport.getBpm(), juce::dontSendNotification);
    }

    const auto signature = juce::String (transport.getTimeSignatureNumerator())
                         + "/" + juce::String (transport.getTimeSignatureDenominator());

    if (! timeSigBox.isPopupActive() && timeSigBox.getText() != signature)
    {
        int itemId = 0;

        for (int i = 1; i <= timeSigBox.getNumItems(); ++i)
        {
            if (timeSigBox.getItemText (i - 1) == signature)
            {
                itemId = i;
                break;
            }
        }

        if (itemId == 0)
        {
            itemId = timeSigBox.getNumItems() + 1;
            timeSigBox.addItem (signature, itemId);
        }

        timeSigBox.setSelectedId (itemId, juce::dontSendNotification);
    }

    switch (info.state)
    {
        case Transport::State::Playing:
            stateLabel.setText ("PLAYING", juce::dontSendNotification);
            stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
            break;

        case Transport::State::Paused:
            stateLabel.setText ("PAUSED", juce::dontSendNotification);
            stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
            break;

        case Transport::State::Stopped:
            stateLabel.setText ("STOPPED", juce::dontSendNotification);
            stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            break;
    }

    barBeatLabel.setText ("BAR " + juce::String (info.bar) + "." + juce::String (info.beat),
                          juce::dontSendNotification);
    timeLabel.setText (formatTime (info.seconds)
                           + "   |   " + juce::String (info.bpm, 1) + " BPM"
                           + "   |   " + juce::String (info.beatsPerBar)
                           + "/" + juce::String (info.beatUnit),
                       juce::dontSendNotification);
    latencyLabel.setText ("LATENCY  " + juce::String (engine.getRoundTripLatencyMs(), 1) + " ms",
                          juce::dontSendNotification);

    if (info.cycleLengthSamples > 0)
    {
        const auto sampleRate = engine.getCurrentSampleRate();
        const auto cycleSeconds = sampleRate > 0.0
                                    ? static_cast<double> (info.cycleLengthSamples) / sampleRate
                                    : 0.0;
        cycleLabel.setText ("CYCLE  " + formatTime (cycleSeconds),
                            juce::dontSendNotification);
    }
    else
    {
        cycleLabel.setText ("CYCLE  free-running", juce::dontSendNotification);
    }

    updateLiveControls();
    repaint (beatIndicatorArea.getUnion (barProgressArea)
                 .getUnion (statePillArea)
                 .getUnion (loopStatusDotArea));
}

void LivePanel::updateLiveControls()
{
    const auto& looper = engine.getLiveLooper();
    const auto snapshot = looper.getSnapshot();
    const auto armed = looper.isArmed();
    const auto recording = looper.isRecording();
    const auto liveBusy = armed || recording;
    const auto layerCount = static_cast<int> (snapshot.layers.size());

    playPauseButton.setEnabled (! liveBusy);
    stopButton.setEnabled (! liveBusy);
    bpmSlider.setEnabled (! liveBusy);
    timeSigBox.setEnabled (! liveBusy);
    tapButton.setEnabled (! liveBusy);
    barSnapToggle.setEnabled (! liveBusy);

    recordButton.setEnabled (! liveBusy && engine.getCurrentSampleRate() > 0.0);
    stopRecButton.setEnabled (liveBusy);
    discardButton.setEnabled (liveBusy || layerCount > 0);

    if (barSnapToggle.getToggleState() != engine.isBarSnapEnabled())
        barSnapToggle.setToggleState (engine.isBarSnapEnabled(), juce::dontSendNotification);

    juce::String status;
    auto colour = DarkLookAndFeel::mutedText();

    if (armed)
    {
        status = "ARMED - recording starts at the next cycle.";
        colour = DarkLookAndFeel::armed();
    }
    else if (recording)
    {
        juce::String currentName = "layer";

        for (const auto& layer : snapshot.layers)
            if (layer.id == snapshot.recordingLayerId)
                currentName = layer.name;

        status = "RECORDING " + currentName;

        if (engine.getTransport().getCycleLengthSamples() <= 0)
            status << " - first loop; press Stop Rec to close it.";
        else
            status << " - auto-stops at the loop boundary.";

        colour = DarkLookAndFeel::recording();
    }
    else if (layerCount == 0)
    {
        status = "No live layers yet. Record starts the first loop.";
        colour = DarkLookAndFeel::mutedText();
    }
    else if (! engine.getTransport().isPlaying())
    {
        status = juce::String (layerCount)
               + (layerCount == 1 ? " layer" : " layers")
               + " ready. Press Play to hear them.";
        colour = DarkLookAndFeel::text();
    }
    else
    {
        status = juce::String (layerCount)
               + (layerCount == 1 ? " layer" : " layers")
               + " playing. Record adds another layer.";
        colour = DarkLookAndFeel::playing();
    }

    loopHintLabel.setText (status, juce::dontSendNotification);
    loopHintLabel.setColour (juce::Label::textColourId, colour);
}

juce::String LivePanel::formatTime (double seconds)
{
    const auto totalMs = static_cast<juce::int64> (std::llround (juce::jmax (0.0, seconds) * 1000.0));
    const auto hours = totalMs / 3600000;
    const auto minutes = (totalMs / 60000) % 60;
    const auto secs = (totalMs / 1000) % 60;
    const auto ms = totalMs % 1000;

    if (hours > 0)
        return juce::String (hours) + ":"
             + juce::String (minutes).paddedLeft ('0', 2) + ":"
             + juce::String (secs).paddedLeft ('0', 2)
             + "." + juce::String (ms).paddedLeft ('0', 3);

    return juce::String (minutes).paddedLeft ('0', 2)
         + ":" + juce::String (secs).paddedLeft ('0', 2)
         + "." + juce::String (ms).paddedLeft ('0', 3);
}

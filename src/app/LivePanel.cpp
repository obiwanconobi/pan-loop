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
        label.setFont (juce::Font (juce::FontOptions (height,
                                                      bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
        label.setJustificationType (juce::Justification::centredLeft);
    }
}

LivePanel::LivePanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    styleLabel (titleLabel, 30.0f, true);
    styleLabel (subtitleLabel, 13.0f);
    styleLabel (stateLabel, 13.0f, true);
    styleLabel (barBeatLabel, 46.0f, true);
    styleLabel (timeLabel, 14.0f);
    styleLabel (latencyLabel, 12.0f);
    styleLabel (cycleLabel, 12.0f);
    styleLabel (transportHintLabel, 12.0f);
    styleLabel (loopTitleLabel, 18.0f, true);
    styleLabel (loopHintLabel, 13.0f);
    styleLabel (bpmLabel, 13.0f);
    styleLabel (timeSigLabel, 13.0f);
    styleLabel (metroLevelLabel, 13.0f);

    titleLabel.setText ("Live", juce::dontSendNotification);
    subtitleLabel.setText ("Transport and metronome foundation",
                           juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    stateLabel.setText ("STOPPED", juce::dontSendNotification);
    stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    stateLabel.setJustificationType (juce::Justification::centredRight);

    barBeatLabel.setText ("BAR 1.1", juce::dontSendNotification);
    timeLabel.setText ("00:00.000   |   120.0 BPM   |   4/4", juce::dontSendNotification);
    timeLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    latencyLabel.setText ("Round-trip latency: 0.0 ms", juce::dontSendNotification);
    latencyLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    cycleLabel.setText ("Cycle: free-running", juce::dontSendNotification);
    cycleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    transportHintLabel.setText ("Space: play/pause   |   S: stop and rewind",
                                juce::dontSendNotification);
    transportHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    loopTitleLabel.setText ("Loop layers", juce::dontSendNotification);
    loopHintLabel.setText ("Record, Stop Rec and Discard will be enabled in Phase 4.",
                           juce::dontSendNotification);
    loopHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    timeSigLabel.setText ("Time signature", juce::dontSendNotification);
    timeSigLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    metroLevelLabel.setText ("Level", juce::dontSendNotification);
    metroLevelLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    playPauseButton.setClickingTogglesState (false);
    playPauseButton.setTooltip ("Play or pause the transport (Space)");
    playPauseButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff24405e));
    playPauseButton.setColour (juce::TextButton::textColourOffId, DarkLookAndFeel::text());
    playPauseButton.onClick = [this] { togglePlayPause(); };

    stopButton.setTooltip ("Stop and rewind to bar 1 (S)");
    stopButton.onClick = [this] { stopTransport(); };

    tapButton.setTooltip ("Tap four times to set the tempo");
    tapButton.onClick = [this] { tapTempo(); };

    bpmSlider.setSliderStyle (juce::Slider::IncDecButtons);
    bpmSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 28);
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

    for (auto* button : { &recordButton, &stopRecButton, &discardButton })
        button->setEnabled (false);

    recordButton.setTooltip ("Available in Phase 4");
    stopRecButton.setTooltip ("Available in Phase 4");
    discardButton.setTooltip ("Available in Phase 4");

    recordButton.setColour (juce::TextButton::buttonColourId,
                            DarkLookAndFeel::recording().withAlpha (0.12f));
    stopRecButton.setColour (juce::TextButton::buttonColourId,
                             DarkLookAndFeel::armed().withAlpha (0.12f));
    discardButton.setColour (juce::TextButton::buttonColourId,
                             DarkLookAndFeel::mutedText().withAlpha (0.12f));

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
    addAndMakeVisible (recordButton);
    addAndMakeVisible (stopRecButton);
    addAndMakeVisible (discardButton);

    updateControls();
    startTimerHz (30);
}

void LivePanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    auto drawCard = [&g] (juce::Rectangle<int> bounds)
    {
        if (bounds.isEmpty())
            return;

        g.setColour (DarkLookAndFeel::panel());
        g.fillRoundedRectangle (bounds.toFloat(), 10.0f);
        g.setColour (DarkLookAndFeel::outline());
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 10.0f, 1.0f);
    };

    drawCard (transportCard);
    drawCard (loopCard);

    const auto info = engine.getTransport().getPositionInfo();

    const auto beats = juce::jmax (1, info.beatsPerBar);
    const auto diameter = juce::jmin (26.0f, static_cast<float> (beatIndicatorArea.getHeight()));
    const auto gap = 14.0f;
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
            g.setColour (i == 0 ? DarkLookAndFeel::armed() : DarkLookAndFeel::playing());
            g.fillEllipse (circle);
        }
        else
        {
            g.setColour (DarkLookAndFeel::outline());
            g.drawEllipse (circle, 1.5f);
        }

        x += diameter + gap;
    }

    auto bar = barProgressArea.toFloat();
    g.setColour (DarkLookAndFeel::panelAlt());
    g.fillRoundedRectangle (bar, 4.0f);

    if (info.state != Transport::State::Stopped)
    {
        g.setColour (info.state == Transport::State::Playing
                       ? DarkLookAndFeel::accent()
                       : DarkLookAndFeel::mutedText());

        const auto filledWidth = bar.getWidth() * static_cast<float> (info.barPhase);
        if (filledWidth > 1.0f)
            g.fillRoundedRectangle (bar.withWidth (filledWidth), 4.0f);
    }
}

void LivePanel::resized()
{
    auto area = getLocalBounds().reduced (28);

    auto header = area.removeFromTop (56);
    auto titleArea = header.removeFromLeft (560);
    titleLabel.setBounds (titleArea.removeFromTop (34));
    subtitleLabel.setBounds (titleArea);
    stateLabel.setBounds (header.removeFromRight (380));

    area.removeFromTop (12);
    transportCard = area.removeFromTop (360);
    area.removeFromTop (16);
    loopCard = area.removeFromTop (140);

    {
        auto inner = transportCard.reduced (24, 18);
        auto top = inner.removeFromTop (150);

        auto buttonArea = top.removeFromLeft (420);
        playPauseButton.setBounds (buttonArea.removeFromTop (90).reduced (6));
        buttonArea.removeFromTop (8);
        stopButton.setBounds (buttonArea.removeFromTop (44).reduced (6));

        top.removeFromLeft (24);
        barBeatLabel.setBounds (top.removeFromTop (66));
        timeLabel.setBounds (top.removeFromTop (26));
        beatIndicatorArea = top.removeFromTop (38);
        top.removeFromTop (6);
        barProgressArea = top.removeFromTop (10);

        inner.removeFromTop (16);
        auto controls = inner.removeFromTop (42);
        bpmLabel.setBounds (controls.removeFromLeft (42));
        bpmSlider.setBounds (controls.removeFromLeft (180));
        controls.removeFromLeft (22);
        timeSigLabel.setBounds (controls.removeFromLeft (92));
        timeSigBox.setBounds (controls.removeFromLeft (90));
        controls.removeFromLeft (12);
        tapButton.setBounds (controls.removeFromLeft (70));
        controls.removeFromLeft (24);
        metroToggle.setBounds (controls.removeFromLeft (130));
        controls.removeFromLeft (8);
        metroLevelLabel.setBounds (controls.removeFromLeft (46));
        metroLevelSlider.setBounds (controls.removeFromLeft (170));

        inner.removeFromTop (18);
        auto bottom = inner.removeFromTop (24);
        latencyLabel.setBounds (bottom.removeFromLeft (260));
        cycleLabel.setBounds (bottom.removeFromLeft (230));
        transportHintLabel.setBounds (bottom);
    }

    {
        auto inner = loopCard.reduced (24, 18);
        loopTitleLabel.setBounds (inner.removeFromTop (26));
        loopHintLabel.setBounds (inner.removeFromTop (20));
        inner.removeFromTop (10);

        auto buttons = inner.removeFromTop (44);
        recordButton.setBounds (buttons.removeFromLeft (170).reduced (6));
        stopRecButton.setBounds (buttons.removeFromLeft (170).reduced (6));
        discardButton.setBounds (buttons.removeFromLeft (170).reduced (6));
    }
}

bool LivePanel::keyPressed (const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        togglePlayPause();
        return true;
    }

    if (key.getTextCharacter() == 's' || key.getTextCharacter() == 'S')
    {
        stopTransport();
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

    playPauseButton.setColour (juce::TextButton::buttonColourId,
                               playing ? juce::Colour (0xff1e5b3a)
                                       : juce::Colour (0xff24405e));
    playPauseButton.setColour (juce::TextButton::textColourOffId,
                               playing ? DarkLookAndFeel::playing()
                                       : DarkLookAndFeel::text());

    if (metroToggle.getToggleState() != engine.getMetronome().isEnabled())
        metroToggle.setToggleState (engine.getMetronome().isEnabled(),
                                    juce::dontSendNotification);

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
    latencyLabel.setText ("Round-trip latency: "
                              + juce::String (engine.getRoundTripLatencyMs(), 1) + " ms",
                          juce::dontSendNotification);

    if (info.cycleLengthSamples > 0)
    {
        const auto sampleRate = engine.getCurrentSampleRate();
        const auto cycleSeconds = sampleRate > 0.0
                                    ? static_cast<double> (info.cycleLengthSamples) / sampleRate
                                    : 0.0;
        cycleLabel.setText ("Cycle: " + formatTime (cycleSeconds),
                            juce::dontSendNotification);
    }
    else
    {
        cycleLabel.setText ("Cycle: free-running", juce::dontSendNotification);
    }

    repaint (beatIndicatorArea.getUnion (barProgressArea));
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

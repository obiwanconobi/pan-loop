#include "PerformancePanel.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioEngine.h"
#include "audio/LiveLooper.h"
#include "audio/Metronome.h"
#include "audio/Playlist.h"
#include "audio/Transport.h"
#include "ui/LevelMeter.h"

#include <cmath>

namespace
{
    class StageLookAndFeel final : public DarkLookAndFeel
    {
    public:
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
        {
            return DarkLookAndFeel::sans (
                juce::jlimit (18.0f, 32.0f, static_cast<float> (buttonHeight) * 0.24f),
                true);
        }
    };

    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        DarkLookAndFeel::styleLabel (label, height, bold);
    }

    juce::String formatTime (double seconds)
    {
        const auto clamped = juce::jmax (0.0, seconds);
        const auto minutes = static_cast<int> (clamped / 60.0);
        const auto remainder = clamped - minutes * 60.0;
        const auto wholeSeconds = static_cast<int> (remainder);
        const auto milliseconds = static_cast<int> ((remainder - wholeSeconds) * 1000.0);

        return juce::String (minutes).paddedLeft ('0', 2)
             + ":" + juce::String (wholeSeconds).paddedLeft ('0', 2)
             + "." + juce::String (milliseconds).paddedLeft ('0', 3);
    }
}

PerformancePanel::PerformancePanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);

    stageLook = std::make_unique<StageLookAndFeel>();

    styleLabel (stateLabel, 64.0f, true);
    styleLabel (timeLabel, 38.0f, true);
    styleLabel (backingLabel, 22.0f, true);
    styleLabel (layerLabel, 20.0f, true);
    styleLabel (tempoLabel, 18.0f);
    styleLabel (hintLabel, 10.5f);
    styleLabel (inputMeterLabel, 10.0f, true);
    styleLabel (outputMeterLabel, 10.0f, true);

    stateLabel.setFont (DarkLookAndFeel::sans (64.0f, true)
                            .withExtraKerningFactor (0.02f));
    timeLabel.setFont (DarkLookAndFeel::mono (38.0f, true));
    backingLabel.setFont (DarkLookAndFeel::sans (22.0f, true));
    layerLabel.setFont (DarkLookAndFeel::sans (20.0f, true));
    tempoLabel.setFont (DarkLookAndFeel::mono (18.0f, true));
    hintLabel.setFont (DarkLookAndFeel::mono (10.5f));
    inputMeterLabel.setFont (DarkLookAndFeel::kicker (10.0f));
    outputMeterLabel.setFont (DarkLookAndFeel::kicker (10.0f));

    stateLabel.setText ("STOPPED", juce::dontSendNotification);
    stateLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    stateLabel.setJustificationType (juce::Justification::centredLeft);

    timeLabel.setText ("00:00.000", juce::dontSendNotification);
    timeLabel.setJustificationType (juce::Justification::centredRight);

    backingLabel.setText ("No backing playlist", juce::dontSendNotification);
    backingLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());

    layerLabel.setText ("No live layers", juce::dontSendNotification);
    layerLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    tempoLabel.setText ("120.0 BPM  |  4/4", juce::dontSendNotification);
    tempoLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    hintLabel.setText ("SPACE play/pause   R record   T stop rec   D discard   B backing mute   C clear layers   F11 full screen   ESC exit",
                       juce::dontSendNotification);
    hintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    inputMeterLabel.setText ("INPUT", juce::dontSendNotification);
    inputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    inputMeterLabel.setJustificationType (juce::Justification::centred);

    outputMeterLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    outputMeterLabel.setJustificationType (juce::Justification::centred);

    DarkLookAndFeel::setButtonVariant (recordButton, "danger");
    DarkLookAndFeel::setButtonIcon (recordButton, "record");
    DarkLookAndFeel::setButtonVariant (stopRecButton, "warning");
    DarkLookAndFeel::setButtonIcon (stopRecButton, "stop");
    DarkLookAndFeel::setButtonVariant (discardButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (discardButton, "trash");
    DarkLookAndFeel::setButtonVariant (playPauseButton, "primary");
    DarkLookAndFeel::setButtonIcon (playPauseButton, "play");

    DarkLookAndFeel::setButtonVariant (backingMuteButton, "default");
    DarkLookAndFeel::setButtonIcon (backingMuteButton, "volume");
    DarkLookAndFeel::setButtonTextSize (backingMuteButton, 14.0f);
    DarkLookAndFeel::setButtonVariant (clearLayersButton, "ghost");
    DarkLookAndFeel::setButtonIcon (clearLayersButton, "trash");
    DarkLookAndFeel::setButtonTextSize (clearLayersButton, 14.0f);
    DarkLookAndFeel::setButtonVariant (metronomeButton, "default");
    DarkLookAndFeel::setButtonIcon (metronomeButton, "metronome");
    DarkLookAndFeel::setButtonTextSize (metronomeButton, 14.0f);
    DarkLookAndFeel::setButtonVariant (fullScreenButton, "ghost");
    DarkLookAndFeel::setButtonIcon (fullScreenButton, "expand");
    DarkLookAndFeel::setButtonTextSize (fullScreenButton, 14.0f);
    DarkLookAndFeel::setButtonVariant (exitButton, "ghost");
    DarkLookAndFeel::setButtonIcon (exitButton, "x");
    DarkLookAndFeel::setButtonTextSize (exitButton, 14.0f);

    recordButton.onClick = [this] { startRecording(); };
    stopRecButton.onClick = [this] { stopRecording(); };
    discardButton.onClick = [this] { discardLayer(); };
    playPauseButton.onClick = [this] { togglePlayPause(); };

    backingMuteButton.setClickingTogglesState (true);
    backingMuteButton.onClick = [this]
    {
        engine.setBackingMuted (backingMuteButton.getToggleState());
    };

    clearLayersButton.onClick = [this] { engine.clearLiveLayers(); };

    metronomeButton.setClickingTogglesState (true);
    metronomeButton.onClick = [this]
    {
        engine.getMetronome().setEnabled (metronomeButton.getToggleState());
    };

    fullScreenButton.onClick = [this]
    {
        if (onToggleFullScreen != nullptr)
            onToggleFullScreen();
    };

    exitButton.onClick = [this]
    {
        if (onExitRequested != nullptr)
            onExitRequested();
    };

    for (auto* button : { &recordButton, &stopRecButton, &discardButton, &playPauseButton,
                          &backingMuteButton, &clearLayersButton, &metronomeButton,
                          &fullScreenButton, &exitButton })
    {
        button->setLookAndFeel (stageLook.get());
    }

    inputMeter = std::make_unique<LevelMeter>();
    outputMeter = std::make_unique<LevelMeter>();
    inputMeter->setOrientation (true);
    outputMeter->setOrientation (true);

    addAndMakeVisible (stateLabel);
    addAndMakeVisible (timeLabel);
    addAndMakeVisible (backingLabel);
    addAndMakeVisible (layerLabel);
    addAndMakeVisible (tempoLabel);
    addAndMakeVisible (hintLabel);
    addAndMakeVisible (inputMeterLabel);
    addAndMakeVisible (outputMeterLabel);
    addAndMakeVisible (recordButton);
    addAndMakeVisible (stopRecButton);
    addAndMakeVisible (discardButton);
    addAndMakeVisible (playPauseButton);
    addAndMakeVisible (backingMuteButton);
    addAndMakeVisible (clearLayersButton);
    addAndMakeVisible (metronomeButton);
    addAndMakeVisible (fullScreenButton);
    addAndMakeVisible (exitButton);
    addAndMakeVisible (*inputMeter);
    addAndMakeVisible (*outputMeter);

    updateControls();
    startTimerHz (30);
}

PerformancePanel::~PerformancePanel()
{
    for (auto* button : { &recordButton, &stopRecButton, &discardButton, &playPauseButton,
                          &backingMuteButton, &clearLayersButton, &metronomeButton,
                          &fullScreenButton, &exitButton })
    {
        button->setLookAndFeel (nullptr);
    }
}

void PerformancePanel::visibilityChanged()
{
    if (isVisible())
    {
        updateControls();
        grabKeyboardFocus();
    }
}

void PerformancePanel::paint (juce::Graphics& g)
{
    juce::ColourGradient backgroundGradient (juce::Colour (0xff0c1017), 0.0f, 0.0f,
                                             juce::Colour (0xff06080c),
                                             0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (backgroundGradient);
    g.fillAll();

    const auto recordingNow = engine.getLiveLooper().isRecording();
    const auto glowColour = recordingNow ? DarkLookAndFeel::recording()
                                         : DarkLookAndFeel::accent();

    auto header = headerArea.toFloat();

    if (! header.isEmpty())
    {
        g.setGradientFill (juce::ColourGradient (glowColour.withAlpha (recordingNow ? 0.13f : 0.07f),
                                                 header.getTopLeft(),
                                                 juce::Colours::transparentBlack,
                                                 header.getBottomRight(),
                                                 true));
        g.fillRoundedRectangle (header.expanded (12.0f), 16.0f);

        g.setColour (DarkLookAndFeel::accent());
        g.setFont (DarkLookAndFeel::kicker (11.0f));
        g.drawText ("PAN LOOP  /  LIVE PERFORMANCE",
                    headerArea.getX(), headerArea.getY() - 2,
                    headerArea.getWidth(), 18,
                    juce::Justification::centredLeft, false);
    }

    if (infoArea.isEmpty())
        return;

    DarkLookAndFeel::drawCard (g, infoArea);
    DarkLookAndFeel::drawCard (g, meterArea);

    if (! backingColumn.isEmpty())
    {
        g.setColour (DarkLookAndFeel::mutedText());
        g.setFont (DarkLookAndFeel::kicker (10.0f));
        g.drawText ("BACKING TRACK", backingColumn.getX(), backingColumn.getY(),
                    backingColumn.getWidth(), 16,
                    juce::Justification::centredLeft, false);
        g.drawText ("LIVE LAYERS", layerColumn.getX(), layerColumn.getY(),
                    layerColumn.getWidth(), 16,
                    juce::Justification::centredLeft, false);
        g.drawText ("TEMPO", tempoColumn.getX(), tempoColumn.getY(),
                    tempoColumn.getWidth(), 16,
                    juce::Justification::centredLeft, false);

        g.setColour (DarkLookAndFeel::outlineSoft());
        g.drawLine (static_cast<float> (layerColumn.getX() - 14),
                    static_cast<float> (backingColumn.getY() + 2),
                    static_cast<float> (layerColumn.getX() - 14),
                    static_cast<float> (backingColumn.getBottom() - 4), 1.0f);
        g.drawLine (static_cast<float> (tempoColumn.getX() - 14),
                    static_cast<float> (backingColumn.getY() + 2),
                    static_cast<float> (tempoColumn.getX() - 14),
                    static_cast<float> (backingColumn.getBottom() - 4), 1.0f);
    }

    const auto info = engine.getTransport().getPositionInfo();
    auto bar = progressArea.toFloat();

    if (bar.isEmpty())
        return;

    g.setColour (juce::Colour (0xff171d27));
    g.fillRoundedRectangle (bar, 9.0f);

    double phase = 0.0;

    if (info.cycleLengthSamples > 0)
        phase = static_cast<double> (info.sample) / static_cast<double> (info.cycleLengthSamples);
    else if (info.state != Transport::State::Stopped)
        phase = info.barPhase;

    phase = juce::jlimit (0.0, 1.0, phase);

    if (phase > 0.0)
    {
        const auto colour = recordingNow
                              ? DarkLookAndFeel::recording()
                              : (info.state == Transport::State::Playing
                                   ? DarkLookAndFeel::accent()
                                   : DarkLookAndFeel::armed());

        const auto filledWidth = juce::jmax (9.0f, bar.getWidth() * static_cast<float> (phase));

        juce::ColourGradient fill (DarkLookAndFeel::accentDeep(), bar.getTopLeft(),
                                   colour, bar.getTopRight(), false);

        if (recordingNow)
            fill = juce::ColourGradient (DarkLookAndFeel::recording().darker (0.25f), bar.getTopLeft(),
                                         DarkLookAndFeel::recording(), bar.getTopRight(), false);

        g.setGradientFill (fill);
        g.fillRoundedRectangle (bar.withWidth (filledWidth), 9.0f);

        g.setColour (colour.withAlpha (0.18f));
        g.fillEllipse (bar.getX() + filledWidth - 16.0f, bar.getCentreY() - 16.0f,
                       32.0f, 32.0f);

        g.setColour (colour);
        g.fillEllipse (bar.getX() + filledWidth - 5.5f, bar.getCentreY() - 5.5f,
                       11.0f, 11.0f);
    }

    if (info.cycleLengthSamples > 0)
    {
        const auto samplesPerBar = engine.getTransport().getSamplesPerBar();

        if (samplesPerBar > 1.0)
        {
            g.setColour (DarkLookAndFeel::background().withAlpha (0.55f));

            for (double position = samplesPerBar;
                 position < static_cast<double> (info.cycleLengthSamples);
                 position += samplesPerBar)
            {
                const auto x = bar.getX()
                             + bar.getWidth() * static_cast<float> (
                                   position / static_cast<double> (info.cycleLengthSamples));
                g.fillRect (x, bar.getY() + 4.0f, 1.0f, bar.getHeight() - 8.0f);
            }
        }
    }

    g.setColour (DarkLookAndFeel::outline());
    g.drawRoundedRectangle (bar.reduced (0.5f), 9.0f, 1.0f);
}

void PerformancePanel::resized()
{
    auto area = getLocalBounds().reduced (juce::jmax (18, getWidth() / 40),
                                          juce::jmax (16, getHeight() / 40));

    headerArea = area.removeFromTop (juce::jmax (104, getHeight() / 6));
    {
        auto header = headerArea;
        header.removeFromTop (22);
        stateLabel.setBounds (header.removeFromLeft (header.getWidth() * 3 / 5));
        timeLabel.setBounds (header);
    }

    area.removeFromTop (12);
    progressArea = area.removeFromTop (juce::jlimit (20, 38, getHeight() / 24));
    area.removeFromTop (juce::jmax (18, getHeight() / 28));

    const auto buttonHeight = juce::jlimit (96, 420, area.getHeight() * 50 / 100);
    buttonArea = area.removeFromTop (buttonHeight);

    {
        auto row = buttonArea;
        const auto gap = juce::jmax (8, getWidth() / 120);
        const auto buttonWidth = (row.getWidth() - gap * 3) / 4;
        recordButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (gap);
        stopRecButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (gap);
        discardButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (gap);
        playPauseButton.setBounds (row.removeFromLeft (buttonWidth));
    }

    area.removeFromTop (juce::jmax (16, getHeight() / 30));

    const auto secondaryHeight = juce::jlimit (32, 52, area.getHeight() / 6);
    auto secondary = area.removeFromTop (secondaryHeight);
    const auto secondaryGap = juce::jmax (6, getWidth() / 180);
    const auto secondaryWidth = juce::jlimit (110, 210, secondary.getWidth() / 7);

    backingMuteButton.setBounds (secondary.removeFromLeft (secondaryWidth));
    secondary.removeFromLeft (secondaryGap);
    clearLayersButton.setBounds (secondary.removeFromLeft (secondaryWidth));
    secondary.removeFromLeft (secondaryGap);
    metronomeButton.setBounds (secondary.removeFromLeft (secondaryWidth));

    fullScreenButton.setBounds (secondary.removeFromRight (secondaryWidth));
    secondary.removeFromRight (secondaryGap);
    exitButton.setBounds (secondary.removeFromRight (secondaryWidth));

    area.removeFromTop (juce::jmax (12, getHeight() / 40));

    meterArea = area.removeFromRight (juce::jlimit (96, 160, getWidth() / 10));
    {
        auto meters = meterArea.reduced (14, 12);
        const auto labelHeight = 18;
        const auto meterWidth = (meters.getWidth() - 20) / 2;

        auto inputArea = meters.removeFromLeft (meterWidth);
        inputMeterLabel.setBounds (inputArea.removeFromTop (labelHeight));
        inputMeter->setBounds (inputArea.reduced (0, 6));

        meters.removeFromLeft (20);
        auto outputArea = meters.removeFromLeft (meterWidth);
        outputMeterLabel.setBounds (outputArea.removeFromTop (labelHeight));
        outputMeter->setBounds (outputArea.reduced (0, 6));
    }

    area.removeFromRight (12);
    infoArea = area;

    auto infoInner = infoArea.reduced (20, 16);
    hintLabel.setBounds (infoInner.removeFromBottom (18));
    infoInner.removeFromBottom (8);

    backingColumn = infoInner.removeFromLeft (juce::roundToInt (static_cast<float> (infoInner.getWidth()) * 0.55f));
    layerColumn = infoInner.removeFromLeft (juce::roundToInt (static_cast<float> (infoInner.getWidth()) * 0.48f));
    tempoColumn = infoInner;

    backingLabel.setBounds (backingColumn.withTrimmedTop (22).withHeight (34));
    layerLabel.setBounds (layerColumn.withTrimmedTop (22).withHeight (30));
    tempoLabel.setBounds (tempoColumn.withTrimmedTop (22).withHeight (24));
}

void PerformancePanel::timerCallback()
{
    updateControls();
}

void PerformancePanel::updateControls()
{
    const auto& transport = engine.getTransport();
    const auto info = transport.getPositionInfo();
    const auto& looper = engine.getLiveLooper();

    const auto recording = looper.isRecording();
    const auto armed = looper.isArmed() && ! recording;

    juce::String state;
    juce::Colour stateColour;

    if (recording)
    {
        state = "RECORDING";
        stateColour = DarkLookAndFeel::recording();
    }
    else if (armed)
    {
        state = "ARMED";
        stateColour = DarkLookAndFeel::armed();
    }
    else
    {
        switch (info.state)
        {
            case Transport::State::Playing:
                state = "PLAYING";
                stateColour = DarkLookAndFeel::playing();
                break;

            case Transport::State::Paused:
                state = "PAUSED";
                stateColour = DarkLookAndFeel::accent();
                break;

            case Transport::State::Stopped:
            default:
                state = "STOPPED";
                stateColour = DarkLookAndFeel::mutedText();
                break;
        }
    }

    if (stateLabel.getText() != state)
        stateLabel.setText (state, juce::dontSendNotification);

    stateLabel.setColour (juce::Label::textColourId, stateColour);

    const auto cycleLength = info.cycleLengthSamples > 0
                               ? static_cast<double> (info.cycleLengthSamples)
                                   / juce::jmax (1.0, transport.getSampleRate())
                               : 0.0;
    const auto position = info.cycleLengthSamples > 0
                            ? static_cast<double> (info.sample)
                                / juce::jmax (1.0, transport.getSampleRate())
                            : info.seconds;

    timeLabel.setText (formatTime (position), juce::dontSendNotification);

    const auto playlist = engine.getPlaylist().getSnapshot();
    juce::String backingText;

    if (playlist.items.empty())
    {
        backingText = "No backing playlist";
    }
    else if (playlist.items.size() == 1)
    {
        backingText = "Backing: " + playlist.items.front().name;
    }
    else
    {
        backingText = "Backing: " + playlist.items.front().name
                    + "  (+" + juce::String (static_cast<int> (playlist.items.size()) - 1) + " more)";
    }

    if (cycleLength > 0.0)
        backingText << "   |   cycle " << formatTime (cycleLength);

    if (backingLabel.getText() != backingText)
        backingLabel.setText (backingText, juce::dontSendNotification);

    const auto layerSnapshot = looper.getSnapshot();
    juce::String layerText;

    if (layerSnapshot.recordingLayerId >= 0)
    {
        for (const auto& layer : layerSnapshot.layers)
        {
            if (layer.id == layerSnapshot.recordingLayerId)
            {
                layerText = "Recording: " + layer.name;
                break;
            }
        }
    }

    if (layerText.isEmpty())
        layerText = juce::String (static_cast<int> (layerSnapshot.layers.size()))
                  + (layerSnapshot.layers.size() == 1 ? " live layer" : " live layers");

    if (layerLabel.getText() != layerText)
        layerLabel.setText (layerText, juce::dontSendNotification);

    const auto tempoText = juce::String (info.bpm, 1) + " BPM"
                         + "   |   " + juce::String (info.beatsPerBar)
                         + "/" + juce::String (info.beatUnit);

    if (tempoLabel.getText() != tempoText)
        tempoLabel.setText (tempoText, juce::dontSendNotification);

    const juce::String playText (info.state == Transport::State::Playing ? "PAUSE" : "PLAY");

    if (playPauseButton.getButtonText() != playText)
        playPauseButton.setButtonText (playText);

    DarkLookAndFeel::setButtonVariant (playPauseButton,
                                       info.state == Transport::State::Playing
                                           ? "success" : "primary");
    DarkLookAndFeel::setButtonIcon (playPauseButton,
                                    info.state == Transport::State::Playing
                                        ? "pause" : "play");

    playPauseButton.setEnabled (! recording && ! armed);
    recordButton.setEnabled (! recording && ! armed);
    stopRecButton.setEnabled (recording || armed);
    discardButton.setEnabled (recording || armed || ! layerSnapshot.layers.empty());
    clearLayersButton.setEnabled (! layerSnapshot.layers.empty());

    if (backingMuteButton.getToggleState() != engine.isBackingMuted())
        backingMuteButton.setToggleState (engine.isBackingMuted(), juce::dontSendNotification);

    if (metronomeButton.getToggleState() != engine.getMetronome().isEnabled())
        metronomeButton.setToggleState (engine.getMetronome().isEnabled(),
                                        juce::dontSendNotification);

    inputMeter->setLevel (engine.getInputLevel());
    outputMeter->setLevel (engine.getOutputLevel());

    repaint (progressArea);

    if (wasRecording != recording)
    {
        wasRecording = recording;
        repaint (headerArea);
    }
}

void PerformancePanel::startRecording()
{
    engine.record();
    updateControls();
}

void PerformancePanel::stopRecording()
{
    engine.stopRecording();
    updateControls();
}

void PerformancePanel::discardLayer()
{
    engine.discardLiveLayer();
    updateControls();
}

void PerformancePanel::togglePlayPause()
{
    if (engine.getLiveLooper().isRecordingOrArmed())
        return;

    engine.getTransport().togglePlayPause();
    updateControls();
}

bool PerformancePanel::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onExitRequested != nullptr)
            onExitRequested();

        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::F11Key)
    {
        if (onToggleFullScreen != nullptr)
            onToggleFullScreen();

        return true;
    }

    if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R')
    {
        startRecording();
        return true;
    }

    if (key.getTextCharacter() == 't' || key.getTextCharacter() == 'T')
    {
        stopRecording();
        return true;
    }

    if (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D')
    {
        discardLayer();
        return true;
    }

    if (key.getTextCharacter() == 'b' || key.getTextCharacter() == 'B')
    {
        engine.toggleBackingMute();
        updateControls();
        return true;
    }

    if (key.getTextCharacter() == 'c' || key.getTextCharacter() == 'C')
    {
        engine.clearLiveLayers();
        updateControls();
        return true;
    }

    if (key == juce::KeyPress::spaceKey)
    {
        togglePlayPause();
        return true;
    }

    return false;
}

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
    class StageLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
        {
            return juce::Font (juce::FontOptions (
                juce::jlimit (18.0f, 34.0f, static_cast<float> (buttonHeight) * 0.26f),
                juce::Font::bold));
        }
    };

    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        label.setFont (juce::Font (juce::FontOptions (height,
                                                      bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
        label.setJustificationType (juce::Justification::centredLeft);
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

    void styleStageButton (juce::TextButton& button, juce::Colour colour)
    {
        button.setColour (juce::TextButton::buttonColourId, colour.withAlpha (0.16f));
        button.setColour (juce::TextButton::buttonOnColourId, colour.withAlpha (0.34f));
        button.setColour (juce::TextButton::textColourOffId, DarkLookAndFeel::text());
        button.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

PerformancePanel::PerformancePanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);

    stageLook = std::make_unique<StageLookAndFeel>();

    styleLabel (stateLabel, 68.0f, true);
    styleLabel (timeLabel, 38.0f, true);
    styleLabel (backingLabel, 22.0f, true);
    styleLabel (layerLabel, 18.0f);
    styleLabel (tempoLabel, 16.0f);
    styleLabel (hintLabel, 13.0f);
    styleLabel (inputMeterLabel, 12.0f, true);
    styleLabel (outputMeterLabel, 12.0f, true);

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

    inputMeterLabel.setText ("IN", juce::dontSendNotification);
    inputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    inputMeterLabel.setJustificationType (juce::Justification::centred);

    outputMeterLabel.setText ("OUT", juce::dontSendNotification);
    outputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    outputMeterLabel.setJustificationType (juce::Justification::centred);

    styleStageButton (recordButton, DarkLookAndFeel::recording());
    styleStageButton (stopRecButton, DarkLookAndFeel::armed());
    styleStageButton (discardButton, DarkLookAndFeel::mutedText());
    styleStageButton (playPauseButton, DarkLookAndFeel::playing());

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
    g.fillAll (juce::Colour (0xff0e1014));

    auto header = headerArea.toFloat();

    if (! header.isEmpty())
    {
        juce::ColourGradient glow (DarkLookAndFeel::accent().withAlpha (0.08f),
                                   header.getTopLeft(),
                                   juce::Colours::transparentBlack,
                                   header.getBottomRight(),
                                   true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (header.expanded (12.0f), 16.0f);
    }

    if (progressArea.isEmpty())
        return;

    const auto info = engine.getTransport().getPositionInfo();
    auto bar = progressArea.toFloat();

    g.setColour (juce::Colour (0xff1b1f26));
    g.fillRoundedRectangle (bar, 9.0f);

    double phase = 0.0;

    if (info.cycleLengthSamples > 0)
        phase = static_cast<double> (info.sample) / static_cast<double> (info.cycleLengthSamples);
    else if (info.state != Transport::State::Stopped)
        phase = info.barPhase;

    phase = juce::jlimit (0.0, 1.0, phase);

    if (phase > 0.0)
    {
        const auto colour = engine.getLiveLooper().isRecording()
                              ? DarkLookAndFeel::recording()
                              : (info.state == Transport::State::Playing
                                   ? DarkLookAndFeel::accent()
                                   : DarkLookAndFeel::mutedText());

        g.setColour (colour);
        g.fillRoundedRectangle (bar.withWidth (juce::jmax (9.0f,
                                                          bar.getWidth() * static_cast<float> (phase))),
                                9.0f);
    }

    if (info.cycleLengthSamples > 0)
    {
        const auto samplesPerBar = engine.getTransport().getSamplesPerBar();

        if (samplesPerBar > 1.0)
        {
            g.setColour (DarkLookAndFeel::background().withAlpha (0.45f));

            for (double position = samplesPerBar;
                 position < static_cast<double> (info.cycleLengthSamples);
                 position += samplesPerBar)
            {
                const auto x = bar.getX()
                             + bar.getWidth() * static_cast<float> (
                                   position / static_cast<double> (info.cycleLengthSamples));
                g.fillRect (x, bar.getY() + 3.0f, 1.0f, bar.getHeight() - 6.0f);
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

    headerArea = area.removeFromTop (juce::jmax (100, getHeight() / 6));
    {
        auto header = headerArea;
        stateLabel.setBounds (header.removeFromLeft (header.getWidth() * 3 / 5));
        timeLabel.setBounds (header);
    }

    area.removeFromTop (12);
    progressArea = area.removeFromTop (juce::jlimit (20, 38, getHeight() / 24));
    area.removeFromTop (juce::jmax (18, getHeight() / 28));

    const auto buttonHeight = juce::jlimit (96, 340, area.getHeight() * 42 / 100);
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

    const auto secondaryHeight = juce::jlimit (30, 52, area.getHeight() / 6);
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

    meterArea = area.removeFromRight (juce::jlimit (90, 150, getWidth() / 10));
    {
        auto meters = meterArea;
        const auto labelHeight = 20;
        const auto meterWidth = (meters.getWidth() - 16) / 2;

        auto inputArea = meters.removeFromLeft (meterWidth);
        inputMeterLabel.setBounds (inputArea.removeFromTop (labelHeight));
        inputMeter->setBounds (inputArea.reduced (0, 4));

        meters.removeFromLeft (16);
        auto outputArea = meters.removeFromLeft (meterWidth);
        outputMeterLabel.setBounds (outputArea.removeFromTop (labelHeight));
        outputMeter->setBounds (outputArea.reduced (0, 4));
    }

    area.removeFromRight (12);
    infoArea = area;
    backingLabel.setBounds (infoArea.removeFromTop (juce::jlimit (28, 48, infoArea.getHeight() / 3)));
    layerLabel.setBounds (infoArea.removeFromTop (28));
    tempoLabel.setBounds (infoArea.removeFromTop (26));
    hintLabel.setBounds (infoArea.removeFromBottom (20));
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

    playPauseButton.setEnabled (! recording && ! armed);

    recordButton.setColour (juce::TextButton::buttonColourId,
                            recording ? DarkLookAndFeel::recording().withAlpha (0.38f)
                                      : DarkLookAndFeel::recording().withAlpha (0.16f));
    stopRecButton.setEnabled (recording || armed);

    if (backingMuteButton.getToggleState() != engine.isBackingMuted())
        backingMuteButton.setToggleState (engine.isBackingMuted(), juce::dontSendNotification);

    if (metronomeButton.getToggleState() != engine.getMetronome().isEnabled())
        metronomeButton.setToggleState (engine.getMetronome().isEnabled(),
                                        juce::dontSendNotification);

    inputMeter->setLevel (engine.getInputLevel());
    outputMeter->setLevel (engine.getOutputLevel());
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

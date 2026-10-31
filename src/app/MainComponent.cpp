#include "MainComponent.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioSettingsPanel.h"
#include "audio/AudioSetup.h"
#include "core/AppConfig.h"
#include "midi/MidiManager.h"
#include "midi/MidiPanel.h"

namespace
{
    class LivePlaceholder final : public juce::Component
    {
    public:
        LivePlaceholder()
        {
            setOpaque (true);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (DarkLookAndFeel::background());

            auto area = getLocalBounds().reduced (40);

            g.setColour (DarkLookAndFeel::text());
            g.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
            g.drawText ("Live loop", area.removeFromTop (46),
                        juce::Justification::centredLeft, false);

            g.setColour (DarkLookAndFeel::mutedText());
            g.setFont (juce::Font (juce::FontOptions (15.0f)));
            g.drawText ("Audio and MIDI are wired up. The loop engine arrives in phase 4.",
                        area.removeFromTop (28), juce::Justification::centredLeft, false);

            area.removeFromTop (28);

            auto buttons = area.removeFromTop (104);
            const auto buttonWidth = buttons.getWidth() / 3;

            drawButton (g, buttons.removeFromLeft (buttonWidth).reduced (8),
                        "RECORD", DarkLookAndFeel::recording());
            drawButton (g, buttons.removeFromLeft (buttonWidth).reduced (8),
                        "STOP REC", DarkLookAndFeel::armed());
            drawButton (g, buttons.reduced (8),
                        "DISCARD", DarkLookAndFeel::mutedText());

            area.removeFromTop (30);

            auto roadmap = area.removeFromTop (92);

            g.setColour (DarkLookAndFeel::panel());
            g.fillRoundedRectangle (roadmap.toFloat(), 8.0f);
            g.setColour (DarkLookAndFeel::outline());
            g.drawRoundedRectangle (roadmap.toFloat().reduced (0.5f), 8.0f, 1.0f);

            auto info = roadmap.reduced (20, 14);
            g.setColour (DarkLookAndFeel::text());
            g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
            g.drawText ("Phase 4: first-loop capture, overdub layers and the backing playlist",
                        info.removeFromTop (24), juce::Justification::centredLeft, false);

            g.setColour (DarkLookAndFeel::mutedText());
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            g.drawFittedText ("Record starts a new layer, Stop Rec finalizes it and arms the next channel, "
                              "Discard removes the current take or the most recent layer. "
                              "Footswitch MIDI learn arrives in Phase 5.",
                              info, juce::Justification::topLeft, 3);
        }

    private:
        static void drawButton (juce::Graphics& g,
                                juce::Rectangle<int> bounds,
                                const juce::String& text,
                                juce::Colour colour)
        {
            const auto rect = bounds.toFloat();

            g.setColour (DarkLookAndFeel::panel());
            g.fillRoundedRectangle (rect, 10.0f);
            g.setColour (colour.withAlpha (0.7f));
            g.drawRoundedRectangle (rect.reduced (1.0f), 10.0f, 2.0f);

            g.setColour (colour);
            g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
            g.drawText (text, bounds, juce::Justification::centred, false);

            g.setColour (colour.withAlpha (0.25f));
            g.fillEllipse (rect.getX() + 16.0f, rect.getCentreY() - 5.0f, 10.0f, 10.0f);
        }
    };

    class SessionInfoPanel final : public juce::Component
    {
    public:
        SessionInfoPanel (juce::AudioDeviceManager& manager, MidiManager& midiManager)
            : deviceManager (manager), midi (midiManager)
        {
            setOpaque (true);

            title.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
            title.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
            title.setText ("Session and diagnostics", juce::dontSendNotification);

            info.setFont (juce::Font (juce::FontOptions (13.0f)));
            info.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
            info.setJustificationType (juce::Justification::topLeft);
            refreshInfo();

            openConfigButton.onClick = [this]
            {
                AppConfig::getConfigDirectory().revealToUser();
            };

            openLogButton.onClick = [this]
            {
                AppConfig::getLogFile().revealToUser();
            };

            copyButton.onClick = [this]
            {
                juce::SystemClipboard::copyTextToClipboard (buildDiagnostics());
            };

            addAndMakeVisible (title);
            addAndMakeVisible (info);
            addAndMakeVisible (openConfigButton);
            addAndMakeVisible (openLogButton);
            addAndMakeVisible (copyButton);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (DarkLookAndFeel::background());
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (32);

            title.setBounds (area.removeFromTop (38));
            area.removeFromTop (12);
            info.setBounds (area.removeFromTop (150));

            area.removeFromTop (16);
            auto row = area.removeFromTop (32);
            openConfigButton.setBounds (row.removeFromLeft (170));
            row.removeFromLeft (10);
            openLogButton.setBounds (row.removeFromLeft (150));
            row.removeFromLeft (10);
            copyButton.setBounds (row.removeFromLeft (170));
        }

    private:
        void refreshInfo()
        {
            juce::String text;
            text << "Application: Pan Loop " << ProjectInfo::versionString << "\n"
                 << "Phase: 1 - skeleton, audio I/O, MIDI monitoring\n"
                 << "Config directory: " << AppConfig::getConfigDirectory().getFullPathName() << "\n"
                 << "Log file: " << AppConfig::getLogFile().getFullPathName() << "\n"
                 << "Build: " << AppConfig::getBuildInfo();

            info.setText (text, juce::dontSendNotification);
        }

        juce::String buildDiagnostics() const
        {
            juce::String text;
            text << "Pan Loop " << ProjectInfo::versionString << "\n"
                 << AppConfig::getBuildInfo() << "\n"
                 << "Config: " << AppConfig::getConfigDirectory().getFullPathName() << "\n";

            if (auto* device = deviceManager.getCurrentAudioDevice())
            {
                text << "Audio: " << device->getTypeName() << " / " << device->getName() << "\n"
                     << "Sample rate: " << device->getCurrentSampleRate() << " Hz\n"
                     << "Buffer: " << device->getCurrentBufferSizeSamples() << " samples\n"
                     << "Channels: " << device->getActiveInputChannels().countNumberOfSetBits()
                     << " in / " << device->getActiveOutputChannels().countNumberOfSetBits() << " out\n"
                     << "Xruns: " << deviceManager.getXRunCount() << "\n";
            }
            else
            {
                text << "Audio: no device open\n";
            }

            text << "MIDI inputs:\n";

            const auto devices = midi.getAvailableDevices();
            if (devices.isEmpty())
                text << "  (none)\n";
            else
                for (const auto& device : devices)
                    text << "  " << device.name << " [" << device.identifier << "]\n";

            const auto enabled = midi.getEnabledDeviceIdentifier();
            text << "MIDI enabled: " << (enabled.isNotEmpty() ? enabled : "(none)") << "\n";
            return text;
        }

        juce::AudioDeviceManager& deviceManager;
        MidiManager& midi;

        juce::Label title;
        juce::Label info;
        juce::TextButton openConfigButton { "Open config folder" };
        juce::TextButton openLogButton { "Open log file" };
        juce::TextButton copyButton { "Copy diagnostics" };
    };
}

MainComponent::MainComponent()
{
    setOpaque (true);

    titleLabel.setText ("PAN LOOP", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());

    subtitleLabel.setText ("Linux live looping  |  JACK audio  |  MIDI foot control",
                           juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    phaseLabel.setText ("PHASE 1  |  AUDIO + MIDI SKELETON", juce::dontSendNotification);
    phaseLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    phaseLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::accent());
    phaseLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (phaseLabel);

    const auto audioError = AudioSetup::initialise (deviceManager);
    if (audioError.isNotEmpty())
        juce::Logger::writeToLog ("Initial audio setup: " + audioError);

    deviceManager.addAudioCallback (&audioEngine);

    midiManager = std::make_unique<MidiManager> (deviceManager);

    tabs.setTabBarDepth (40);
    tabs.setColour (juce::TabbedComponent::backgroundColourId, DarkLookAndFeel::background());
    tabs.addTab ("Live", DarkLookAndFeel::panel(), new LivePlaceholder(), true);
    tabs.addTab ("Audio", DarkLookAndFeel::panel(),
                 new AudioSettingsPanel (deviceManager, audioEngine), true);
    tabs.addTab ("MIDI", DarkLookAndFeel::panel(), new MidiPanel (*midiManager), true);
    tabs.addTab ("Session", DarkLookAndFeel::panel(),
                 new SessionInfoPanel (deviceManager, *midiManager), true);
    tabs.setCurrentTabIndex (0);

    addAndMakeVisible (tabs);

    setSize (1180, 820);
}

MainComponent::~MainComponent()
{
    deviceManager.removeAudioCallback (&audioEngine);
    midiManager.reset();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    g.setColour (DarkLookAndFeel::outline());
    g.fillRect (0, 78, getWidth(), 1);

    g.setColour (DarkLookAndFeel::accent());
    g.fillRect (24, 24, 4, 28);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (79).reduced (24, 12);
    auto titleArea = header.removeFromLeft (500);
    titleLabel.setBounds (titleArea.removeFromTop (34));
    subtitleLabel.setBounds (titleArea);
    phaseLabel.setBounds (header.removeFromRight (400));

    tabs.setBounds (area.reduced (16, 14));
}

#include "MainComponent.h"

#include "app/DarkLookAndFeel.h"
#include "app/LayersPanel.h"
#include "app/LivePanel.h"
#include "app/PlaylistPanel.h"
#include "audio/AudioSettingsPanel.h"
#include "audio/AudioSetup.h"
#include "core/AppConfig.h"
#include "midi/MidiManager.h"
#include "midi/MidiPanel.h"

namespace
{
    class SessionInfoPanel final : public juce::Component,
                                   private juce::Timer
    {
    public:
        SessionInfoPanel (juce::AudioDeviceManager& manager,
                          MidiManager& midiManager,
                          AudioEngine& audioEngine)
            : deviceManager (manager), midi (midiManager), engine (audioEngine)
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

            startTimerHz (1);
        }

        void timerCallback() override
        {
            refreshInfo();
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
            info.setBounds (area.removeFromTop (180));

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
            const auto infoState = engine.getTransport().getPositionInfo();
            const auto playlistInfo = engine.getPlaylist().getSnapshot();
            const auto layerInfo = engine.getLiveLooper().getSnapshot();

            juce::String text;
            text << "Application: Pan Loop " << ProjectInfo::versionString << "\n"
                 << "Phase: 4 - live looper, backing playlist, transport and metronome\n"
                 << "Transport: " << transportStateName (infoState.state)
                 << " | bar " << infoState.bar << "." << infoState.beat
                 << " | " << juce::String (infoState.bpm, 1) << " BPM"
                 << " | " << infoState.beatsPerBar << "/" << infoState.beatUnit << "\n"
                 << "Playlist: " << static_cast<int> (playlistInfo.items.size()) << " file(s)"
                 << " | cycle " << juce::String (playlistInfo.totalDurationSeconds, 2) << " s"
                 << " | " << juce::String (playlistInfo.memoryUsageBytes / (1024.0 * 1024.0), 1) << " MB\n"
                 << "Live layers: " << static_cast<int> (layerInfo.layers.size())
                 << " | " << juce::String (layerInfo.totalMemoryUsageBytes / (1024.0 * 1024.0), 1) << " MB"
                 << " | " << (engine.getLiveLooper().isRecording() ? "recording"
                                   : engine.getLiveLooper().isArmed() ? "armed" : "idle") << "\n"
                 << "Metronome: " << (engine.getMetronome().isEnabled() ? "on" : "off")
                 << " | level " << juce::String (engine.getMetronome().getLevel() * 100.0f, 0) << " %\n"
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

            const auto transportInfo = engine.getTransport().getPositionInfo();
            text << "Transport: " << transportStateName (transportInfo.state)
                 << " | bar " << transportInfo.bar << "." << transportInfo.beat
                 << " | " << juce::String (transportInfo.bpm, 1) << " BPM"
                 << " | " << transportInfo.beatsPerBar << "/" << transportInfo.beatUnit << "\n"
                 << "Round-trip latency: " << juce::String (engine.getRoundTripLatencyMs(), 1) << " ms\n";

            const auto playlistInfo = engine.getPlaylist().getSnapshot();
            text << "Playlist: " << static_cast<int> (playlistInfo.items.size()) << " file(s)"
                 << " | cycle " << juce::String (playlistInfo.totalDurationSeconds, 2) << " s"
                 << " | " << juce::String (playlistInfo.memoryUsageBytes / (1024.0 * 1024.0), 1) << " MB\n";

            for (const auto& item : playlistInfo.items)
                text << "  " << item.name << " (" << juce::String (item.durationSeconds, 2) << " s)\n";

            const auto layerInfo = engine.getLiveLooper().getSnapshot();
            text << "Live layers: " << static_cast<int> (layerInfo.layers.size())
                 << " | " << juce::String (layerInfo.totalMemoryUsageBytes / (1024.0 * 1024.0), 1) << " MB\n";

            for (const auto& layer : layerInfo.layers)
                text << "  " << layer.name << " ("
                     << juce::String (layer.durationSeconds, 2) << " s"
                     << (layer.recording ? ", recording" : layer.armed ? ", armed" : "")
                     << ")\n";

            return text;
        }

        static const char* transportStateName (Transport::State state)
        {
            switch (state)
            {
                case Transport::State::Stopped: return "stopped";
                case Transport::State::Playing: return "playing";
                case Transport::State::Paused:  return "paused";
            }

            return "stopped";
        }

        juce::AudioDeviceManager& deviceManager;
        MidiManager& midi;
        AudioEngine& engine;

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

    phaseLabel.setText ("PHASE 4  |  LIVE LOOPER", juce::dontSendNotification);
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
    tabs.addTab ("Live", DarkLookAndFeel::panel(), new LivePanel (audioEngine), true);
    playlistPanel = new PlaylistPanel (audioEngine);
    tabs.addTab ("Playlist", DarkLookAndFeel::panel(), playlistPanel, true);
    tabs.addTab ("Layers", DarkLookAndFeel::panel(), new LayersPanel (audioEngine), true);
    tabs.addTab ("Audio", DarkLookAndFeel::panel(),
                 new AudioSettingsPanel (deviceManager, audioEngine), true);
    tabs.addTab ("MIDI", DarkLookAndFeel::panel(), new MidiPanel (*midiManager), true);
    tabs.addTab ("Session", DarkLookAndFeel::panel(),
                 new SessionInfoPanel (deviceManager, *midiManager, audioEngine), true);
    tabs.setCurrentTabIndex (0);

    addAndMakeVisible (tabs);

    setSize (1180, 820);

   #if JUCE_DEBUG
    const auto screenshotPath = juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT", {});

    if (screenshotPath.isNotEmpty())
    {
        const auto tabText = juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT_TAB", "0");
        const auto tabIndex = tabText.getIntValue();

        if (tabIndex >= 0 && tabIndex < tabs.getNumTabs())
            tabs.setCurrentTabIndex (tabIndex);

        const auto delayText = juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT_DELAY_MS", "3000");
        const auto screenshotDelay = juce::jlimit (100, 60000, delayText.getIntValue());

        juce::Component::SafePointer<MainComponent> safeThis (this);

        juce::Timer::callAfterDelay (screenshotDelay, [safeThis, screenshotPath]
        {
            if (safeThis == nullptr)
                return;

            auto image = safeThis->createComponentSnapshot (safeThis->getLocalBounds());

            if (image.isValid())
            {
                auto file = juce::File (screenshotPath);
                file.deleteFile();

                if (auto stream = file.createOutputStream())
                {
                    juce::PNGImageFormat png;
                    png.writeImageToStream (image, *stream);
                    juce::Logger::writeToLog ("Saved screenshot: " + file.getFullPathName());
                }
            }
        });
    }

    const auto testPlaylist = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_PLAYLIST", {});

    if (testPlaylist.isNotEmpty())
    {
        juce::Component::SafePointer<MainComponent> safeThis (this);

        juce::Timer::callAfterDelay (800, [safeThis, testPlaylist]
        {
            if (safeThis == nullptr || safeThis->playlistPanel == nullptr)
                return;

            auto paths = juce::StringArray::fromTokens (testPlaylist, ";", "");
            paths.removeEmptyStrings();
            safeThis->playlistPanel->addFilesForTest (paths);
        });
    }

    // Development smoke-test hook: start the transport after the initial JACK
    // port connections have settled. The metronome stays off when a test
    // playlist is loaded so the recorded output contains only the backing files.
    if (juce::SystemStats::getEnvironmentVariable ("PANLOOP_DEMO", {}) == "1")
    {
        juce::Component::SafePointer<MainComponent> safeThis (this);
        const auto hasTestPlaylist = testPlaylist.isNotEmpty();
        const auto forceMetronome = juce::SystemStats::getEnvironmentVariable ("PANLOOP_DEMO_METRONOME", {}) == "1";
        const auto delayMs = hasTestPlaylist ? 4000 : 2000;

        juce::Timer::callAfterDelay (delayMs, [safeThis, hasTestPlaylist, forceMetronome]
        {
            if (safeThis == nullptr)
                return;

            if (! hasTestPlaylist || forceMetronome)
                safeThis->audioEngine.getMetronome().setEnabled (true);

            safeThis->audioEngine.getTransport().play();
        });
    }

    if (juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_LOOP", {}) == "1")
    {
        juce::Component::SafePointer<MainComponent> safeThis (this);

        if (juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_LOOP_SNAP", {}) == "1")
            audioEngine.setBarSnapEnabled (true);

        juce::Timer::callAfterDelay (3000, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.record();
        });

        juce::Timer::callAfterDelay (5000, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.stopRecording();
        });

        juce::Timer::callAfterDelay (6000, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.record();
        });

        juce::Timer::callAfterDelay (8500, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.stopRecording();
        });

        juce::Timer::callAfterDelay (9500, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.record();
        });

        juce::Timer::callAfterDelay (10500, [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->audioEngine.discardLiveLayer();
        });

        if (juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_LOOP_EXTRA", {}) == "1")
        {
            juce::Timer::callAfterDelay (11500, [safeThis]
            {
                if (safeThis != nullptr)
                    safeThis->audioEngine.discardLiveLayer();
            });

            juce::Timer::callAfterDelay (13000, [safeThis]
            {
                if (safeThis != nullptr)
                    safeThis->audioEngine.clearLiveLayers();
            });
        }
    }
   #endif
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

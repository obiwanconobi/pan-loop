#include "MainComponent.h"

#include "app/DarkLookAndFeel.h"
#include "app/LayersPanel.h"
#include "app/LivePanel.h"
#include "app/PerformancePanel.h"
#include "app/PlaylistPanel.h"
#include "audio/AudioSettingsPanel.h"
#include "audio/AudioSetup.h"
#include "core/AppConfig.h"
#include "midi/MidiManager.h"
#include "midi/MidiPanel.h"
#include "session/SessionStore.h"

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

            title.setFont (DarkLookAndFeel::sans (28.0f, true)
                               .withExtraKerningFactor (0.02f));
            title.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
            title.setText ("Session", juce::dontSendNotification);

            subtitle.setFont (DarkLookAndFeel::sans (12.5f));
            subtitle.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            subtitle.setText ("Save a performance, load a previous session or export a mixdown",
                              juce::dontSendNotification);

            info.setFont (DarkLookAndFeel::mono (11.0f));
            info.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
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
                setSessionStatus ("Diagnostics copied to the clipboard.", false);
            };

            saveSessionButton.onClick = [this] { chooseSaveSession(); };
            loadSessionButton.onClick = [this] { chooseLoadSession(); };
            exportButton.onClick = [this] { chooseExportMixdown(); };
            newSessionButton.onClick = [this] { newSession(); };

            sessionStatusLabel.setFont (DarkLookAndFeel::sans (13.5f, true));
            sessionStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            sessionStatusLabel.setJustificationType (juce::Justification::centredLeft);
            sessionStatusLabel.setText ("No session loaded.", juce::dontSendNotification);

            sessionPathLabel.setFont (DarkLookAndFeel::mono (11.0f));
            sessionPathLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            sessionPathLabel.setJustificationType (juce::Justification::centredLeft);

            DarkLookAndFeel::setButtonVariant (saveSessionButton, "primary");
            DarkLookAndFeel::setButtonIcon (saveSessionButton, "save");
            DarkLookAndFeel::setButtonVariant (loadSessionButton, "default");
            DarkLookAndFeel::setButtonIcon (loadSessionButton, "load");
            DarkLookAndFeel::setButtonVariant (exportButton, "default");
            DarkLookAndFeel::setButtonIcon (exportButton, "export");
            DarkLookAndFeel::setButtonVariant (newSessionButton, "ghost");
            DarkLookAndFeel::setButtonIcon (newSessionButton, "new");
            DarkLookAndFeel::setButtonVariant (openConfigButton, "ghost");
            DarkLookAndFeel::setButtonIcon (openConfigButton, "folder");
            DarkLookAndFeel::setButtonVariant (openLogButton, "ghost");
            DarkLookAndFeel::setButtonIcon (openLogButton, "file");
            DarkLookAndFeel::setButtonVariant (copyButton, "accent");
            DarkLookAndFeel::setButtonIcon (copyButton, "copy");

            saveSessionButton.setTooltip ("Save the current session (.panloop)");
            loadSessionButton.setTooltip ("Load a saved session file");
            exportButton.setTooltip ("Render one master cycle to a stereo WAV file");
            newSessionButton.setTooltip ("Clear the playlist, live layers and transport");
            openConfigButton.setTooltip ("Open the configuration folder");
            openLogButton.setTooltip ("Open the application log");
            copyButton.setTooltip ("Copy a diagnostics report to the clipboard");

            addAndMakeVisible (title);
            addAndMakeVisible (subtitle);
            addAndMakeVisible (info);
            addAndMakeVisible (sessionStatusLabel);
            addAndMakeVisible (sessionPathLabel);
            addAndMakeVisible (saveSessionButton);
            addAndMakeVisible (loadSessionButton);
            addAndMakeVisible (exportButton);
            addAndMakeVisible (newSessionButton);
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

            DarkLookAndFeel::drawCard (g, sessionCardArea, true);
            DarkLookAndFeel::drawCard (g, diagnosticsCardArea);

            const auto drawKicker = [&g] (juce::Rectangle<int> card, const juce::String& text)
            {
                if (card.isEmpty())
                    return;

                g.setColour (DarkLookAndFeel::mutedText());
                g.setFont (DarkLookAndFeel::kicker (10.5f));
                g.drawText (text, card.getX() + 20, card.getY() + 14,
                            card.getWidth() - 40, 16,
                            juce::Justification::centredLeft, false);
            };

            drawKicker (sessionCardArea, "SESSION FILE");
            drawKicker (diagnosticsCardArea, "DIAGNOSTICS");
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (24);

            auto header = area.removeFromTop (54);
            auto titleArea = header.removeFromLeft (juce::jmin (700, header.getWidth()));
            title.setBounds (titleArea.removeFromTop (32));
            subtitle.setBounds (titleArea);

            area.removeFromTop (12);

            const auto sessionHeight = juce::jlimit (164, 180,
                                                     juce::roundToInt (area.getHeight() * 0.32f));
            sessionCardArea = area.removeFromTop (sessionHeight);
            area.removeFromTop (12);
            diagnosticsCardArea = area;

            {
                auto inner = sessionCardArea.reduced (20);
                inner.removeFromTop (28);

                sessionStatusLabel.setBounds (inner.removeFromTop (22));
                inner.removeFromTop (8);

                auto sessionRow = inner.removeFromTop (42);
                saveSessionButton.setBounds (sessionRow.removeFromLeft (150));
                sessionRow.removeFromLeft (10);
                loadSessionButton.setBounds (sessionRow.removeFromLeft (140));
                sessionRow.removeFromLeft (10);
                exportButton.setBounds (sessionRow.removeFromLeft (170));
                sessionRow.removeFromLeft (10);
                newSessionButton.setBounds (sessionRow.removeFromLeft (130));

                inner.removeFromTop (6);
                sessionPathLabel.setBounds (inner.removeFromTop (18));
            }

            {
                auto inner = diagnosticsCardArea.reduced (20);
                copyButton.setBounds (diagnosticsCardArea.getRight() - 170,
                                      diagnosticsCardArea.getY() + 12, 150, 30);

                inner.removeFromTop (42);

                auto bottomRow = inner.removeFromBottom (34);
                openConfigButton.setBounds (bottomRow.removeFromLeft (180));
                bottomRow.removeFromLeft (10);
                openLogButton.setBounds (bottomRow.removeFromLeft (150));

                inner.removeFromBottom (8);
                info.setBounds (inner);
            }
        }

    private:
        void chooseSaveSession()
        {
            if (engine.getLiveLooper().isRecordingOrArmed())
            {
                setSessionStatus ("Stop recording before saving a session.", true);
                return;
            }

            const auto defaultFile = currentSessionFile.existsAsFile()
                                       ? currentSessionFile
                                       : AppConfig::getSessionsDirectory().getChildFile ("Untitled.panloop");

            fileChooser = std::make_unique<juce::FileChooser> ("Save session",
                                                               defaultFile,
                                                               "*.panloop");

            juce::Component::SafePointer<SessionInfoPanel> safeThis (this);

            const auto browserFlags = juce::FileBrowserComponent::saveMode
                                    | juce::FileBrowserComponent::canSelectFiles
                                    | juce::FileBrowserComponent::warnAboutOverwriting;

            fileChooser->launchAsync (browserFlags, [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                auto file = chooser.getResult();

                if (file == juce::File())
                    return;

                if (! file.hasFileExtension ("panloop"))
                    file = file.withFileExtension ("panloop");

                const auto error = SessionStore::save (file, safeThis->engine,
                                                      safeThis->midi.getMappingsForSession());

                if (error.isNotEmpty())
                {
                    safeThis->setSessionStatus (error, true);
                }
                else
                {
                    safeThis->currentSessionFile = file;
                    safeThis->setSessionStatus ("Saved: " + file.getFullPathName(), false);
                    safeThis->refreshInfo();
                }
            });
        }

        void chooseLoadSession()
        {
            const auto defaultFile = currentSessionFile.existsAsFile()
                                       ? currentSessionFile
                                       : AppConfig::getSessionsDirectory().getChildFile ("Untitled.panloop");

            fileChooser = std::make_unique<juce::FileChooser> ("Load session",
                                                               defaultFile,
                                                               "*.panloop");

            juce::Component::SafePointer<SessionInfoPanel> safeThis (this);

            fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                        | juce::FileBrowserComponent::canSelectFiles,
                                      [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                const auto file = chooser.getResult();

                if (file == juce::File())
                    return;

                std::vector<MidiMapping> mappings;
                const auto error = SessionStore::load (file, safeThis->engine, &mappings);

                if (error.isNotEmpty())
                {
                    safeThis->setSessionStatus (error, true);
                }
                else
                {
                    safeThis->midi.restoreMappingsForSession (mappings);
                    safeThis->currentSessionFile = file;
                    safeThis->setSessionStatus ("Loaded: " + file.getFullPathName(), false);
                    safeThis->refreshInfo();
                }
            });
        }

        void chooseExportMixdown()
        {
            auto defaultFile = AppConfig::getSessionsDirectory().getChildFile ("Mixdown.wav");

            if (currentSessionFile.existsAsFile())
                defaultFile = currentSessionFile.getSiblingFile ("Mixdown.wav");

            fileChooser = std::make_unique<juce::FileChooser> ("Export mixdown",
                                                               defaultFile,
                                                               "*.wav");

            juce::Component::SafePointer<SessionInfoPanel> safeThis (this);

            const auto browserFlags = juce::FileBrowserComponent::saveMode
                                    | juce::FileBrowserComponent::canSelectFiles
                                    | juce::FileBrowserComponent::warnAboutOverwriting;

            fileChooser->launchAsync (browserFlags, [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                auto file = chooser.getResult();

                if (file == juce::File())
                    return;

                if (! file.hasFileExtension ("wav"))
                    file = file.withFileExtension ("wav");

                const auto error = SessionStore::exportMixdown (file, safeThis->engine);

                if (error.isNotEmpty())
                    safeThis->setSessionStatus (error, true);
                else
                    safeThis->setSessionStatus ("Exported mixdown: " + file.getFullPathName(), false);
            });
        }

        void newSession()
        {
            engine.getTransport().stop();
            engine.clearLiveLayers();
            engine.getPlaylist().clear();
            engine.playlistChanged();
            currentSessionFile = juce::File();
            setSessionStatus ("New empty session.", false);
            refreshInfo();
        }

        void setSessionStatus (const juce::String& text, bool isError)
        {
            sessionStatusLabel.setColour (juce::Label::textColourId,
                                          isError ? DarkLookAndFeel::recording()
                                                  : DarkLookAndFeel::mutedText());
            sessionStatusLabel.setText (text, juce::dontSendNotification);
        }

        void refreshInfo()
        {
            const auto infoState = engine.getTransport().getPositionInfo();
            const auto playlistInfo = engine.getPlaylist().getSnapshot();
            const auto layerInfo = engine.getLiveLooper().getSnapshot();

            juce::String text;
            const auto compact = getHeight() < 560;

            text << "Application: Pan Loop " << ProjectInfo::versionString << "\n"
                 << "Build: " << AppConfig::getBuildInfo() << "\n";

            if (! compact)
            {
                if (auto* device = deviceManager.getCurrentAudioDevice())
                {
                    text << "Audio: " << device->getName()
                         << " | " << deviceManager.getCurrentAudioDeviceType() << "\n"
                         << "Rate: " << juce::String (device->getCurrentSampleRate(), 1) << " Hz"
                         << " | buffer " << device->getCurrentBufferSizeSamples() << " samples"
                         << " | " << device->getActiveInputChannels().countNumberOfSetBits()
                         << " in / " << device->getActiveOutputChannels().countNumberOfSetBits()
                         << " out | CPU " << juce::String (deviceManager.getCpuUsage() * 100.0, 1) << " %\n";
                }
                else
                {
                    text << "Audio: no device open\n";
                }
            }

            text << "Transport: " << transportStateName (infoState.state)
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
                 << " | level " << juce::String (engine.getMetronome().getLevel() * 100.0f, 0) << " %";

            if (! compact)
                text << "\nRound-trip latency: " << juce::String (engine.getRoundTripLatencyMs(), 1) << " ms";

            info.setText (text, juce::dontSendNotification);

            sessionPathLabel.setText (currentSessionFile.existsAsFile()
                                        ? "FILE  " + currentSessionFile.getFullPathName()
                                        : "No session file saved yet.",
                                      juce::dontSendNotification);
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
        juce::Label subtitle;
        juce::Label info;
        juce::Label sessionStatusLabel;
        juce::Label sessionPathLabel;
        juce::TextButton saveSessionButton { "Save Session..." };
        juce::TextButton loadSessionButton { "Load Session..." };
        juce::TextButton exportButton { "Export Mixdown..." };
        juce::TextButton newSessionButton { "New Session" };
        juce::TextButton openConfigButton { "Open config folder" };
        juce::TextButton openLogButton { "Open log file" };
        juce::TextButton copyButton { "Copy diagnostics" };

        juce::Rectangle<int> sessionCardArea;
        juce::Rectangle<int> diagnosticsCardArea;

        juce::File currentSessionFile;
        std::unique_ptr<juce::FileChooser> fileChooser;
    };
}

MainComponent::MainComponent()
{
    setOpaque (true);

    titleLabel.setText ("PAN LOOP", juce::dontSendNotification);
    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.08f));
    titleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());

    subtitleLabel.setText ("LIVE LOOP STATION  |  JACK AUDIO  |  MIDI FOOT CONTROL",
                           juce::dontSendNotification);
    subtitleLabel.setFont (DarkLookAndFeel::mono (10.5f).withExtraKerningFactor (0.12f));
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    audioStatusLabel.setFont (DarkLookAndFeel::mono (11.5f, true));
    audioStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    audioStatusLabel.setJustificationType (juce::Justification::centredLeft);

    performanceButton.setTooltip ("Open the stage view (F11)");
    DarkLookAndFeel::setButtonVariant (performanceButton, "accent");
    DarkLookAndFeel::setButtonIcon (performanceButton, "expand");
    DarkLookAndFeel::setButtonTextSize (performanceButton, 13.0f);
    performanceButton.onClick = [this] { setPerformanceMode (! performanceMode); };

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (audioStatusLabel);
    addAndMakeVisible (performanceButton);

    const auto audioError = AudioSetup::initialise (deviceManager);
    if (audioError.isNotEmpty())
        juce::Logger::writeToLog ("Initial audio setup: " + audioError);

    deviceManager.addAudioCallback (&audioEngine);

    midiManager = std::make_unique<MidiManager> (deviceManager);
    midiManager->onAction = [this] (MidiAction action)
    {
        handleMidiAction (action);
    };

   #if JUCE_DEBUG
    const auto testMidiDevice = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_MIDI_DEVICE", {});

    if (testMidiDevice.isNotEmpty())
        midiManager->selectDeviceMatching (testMidiDevice);
   #endif

    const char* tabLabels[numTabs] = { "Live", "Playlist", "Layers", "Audio", "MIDI", "Session" };
    const char* tabIcons[numTabs] = { "live", "playlist", "layers", "audio", "midi", "session" };

    for (int i = 0; i < numTabs; ++i)
    {
        auto& button = navButtons[i];
        button.setButtonText (tabLabels[i]);
        button.setClickingTogglesState (true);
        button.setRadioGroupId (0x504c4e);
        DarkLookAndFeel::setButtonVariant (button, "nav");
        DarkLookAndFeel::setButtonIcon (button, tabIcons[i]);
        DarkLookAndFeel::setButtonTextSize (button, 13.0f);
        button.setTooltip ("Open the " + juce::String (tabLabels[i]) + " view");
        button.onClick = [this, i]
        {
            tabs.setCurrentTabIndex (i);
            updateNavState();
        };

        addAndMakeVisible (button);
    }

    tabs.setTabBarDepth (0);
    tabs.setIndent (0);
    tabs.setColour (juce::TabbedComponent::backgroundColourId, DarkLookAndFeel::background());
    tabs.addTab ("Live", DarkLookAndFeel::background(), new LivePanel (audioEngine), true);
    playlistPanel = new PlaylistPanel (audioEngine);
    tabs.addTab ("Playlist", DarkLookAndFeel::background(), playlistPanel, true);
    tabs.addTab ("Layers", DarkLookAndFeel::background(), new LayersPanel (audioEngine), true);
    tabs.addTab ("Audio", DarkLookAndFeel::background(),
                 new AudioSettingsPanel (deviceManager, audioEngine), true);
    tabs.addTab ("MIDI", DarkLookAndFeel::background(), new MidiPanel (*midiManager), true);
    tabs.addTab ("Session", DarkLookAndFeel::background(),
                 new SessionInfoPanel (deviceManager, *midiManager, audioEngine), true);
    tabs.setCurrentTabIndex (0);

    performancePanel = std::make_unique<PerformancePanel> (audioEngine);
    performancePanel->setVisible (false);
    performancePanel->onExitRequested = [this] { setPerformanceMode (false); };
    performancePanel->onToggleFullScreen = [this] { toggleFullScreen(); };

    addAndMakeVisible (tabs);
    addChildComponent (*performancePanel);

    setWantsKeyboardFocus (true);
    updateNavState();
    updateAudioStatus();
    startTimerHz (2);
    setSize (1220, 880);

   #if JUCE_DEBUG
    const auto screenshotPath = juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT", {});

    if (screenshotPath.isNotEmpty())
    {
        const auto tabText = juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT_TAB", "0");
        const auto tabIndex = tabText.getIntValue();

        if (juce::SystemStats::getEnvironmentVariable ("PANLOOP_SCREENSHOT_PERFORMANCE", "0") == "1")
        {
            juce::Component::SafePointer<MainComponent> safeThis (this);

            juce::Timer::callAfterDelay (500, [safeThis]
            {
                if (safeThis != nullptr)
                    safeThis->setPerformanceMode (true);
            });
        }

        if (tabIndex >= 0 && tabIndex < tabs.getNumTabs())
        {
            tabs.setCurrentTabIndex (tabIndex);
            updateNavState();
        }

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

    const auto testMidiLearn = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_MIDI_LEARN", {});

    if (testMidiLearn.isNotEmpty())
    {
        if (const auto action = midiActionFromKey (testMidiLearn))
        {
            juce::Component::SafePointer<MainComponent> safeThis (this);

            juce::Timer::callAfterDelay (1500, [safeThis, action]
            {
                if (safeThis != nullptr && safeThis->midiManager != nullptr)
                    safeThis->midiManager->startLearning (*action);
            });
        }
    }

    const auto testMidiInject = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_MIDI_INJECT", {});

    if (testMidiInject.isNotEmpty())
    {
        const auto tokens = juce::StringArray::fromTokens (testMidiInject, "|", "");
        juce::Component::SafePointer<MainComponent> safeThis (this);

        for (int i = 0; i < tokens.size(); ++i)
        {
            const auto fields = juce::StringArray::fromTokens (tokens[i], ":", "");

            if (fields.size() < 3)
                continue;

            const auto type = fields[0].toLowerCase();
            const auto number = fields[1].getIntValue();
            const auto value = fields[2].getIntValue();
            juce::MidiMessage message;

            if (type == "cc")
                message = juce::MidiMessage::controllerEvent (1, number, value);
            else if (type == "note")
                message = juce::MidiMessage::noteOn (1, number,
                                                     static_cast<juce::uint8> (juce::jlimit (1, 127, value)));
            else if (type == "pc")
                message = juce::MidiMessage::programChange (1, number);
            else
                continue;

            juce::Timer::callAfterDelay (2000 + i * 1500, [safeThis, message]
            {
                if (safeThis != nullptr && safeThis->midiManager != nullptr)
                    safeThis->midiManager->injectMidiMessageForTest (message);
            });
        }
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
    stopTimer();
    deviceManager.removeAudioCallback (&audioEngine);
    midiManager.reset();
}

void MainComponent::timerCallback()
{
    updateAudioStatus();
    updateNavState();
}

void MainComponent::updateNavState()
{
    const auto current = tabs.getCurrentTabIndex();

    for (int i = 0; i < numTabs; ++i)
        if (navButtons[i].getToggleState() != (i == current))
            navButtons[i].setToggleState (i == current, juce::dontSendNotification);
}

void MainComponent::updateAudioStatus()
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto type = deviceManager.getCurrentAudioDeviceType();
        const auto rate = device->getCurrentSampleRate();
        const auto rateText = rate >= 1000.0
                                ? juce::String (rate / 1000.0, rate >= 10000.0 ? 0 : 1) + " kHz"
                                : juce::String (rate, 0) + " Hz";

        audioStatusLabel.setText ((type.isNotEmpty() ? type.toUpperCase() : "AUDIO")
                                      + "  |  " + rateText,
                                  juce::dontSendNotification);
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    type.equalsIgnoreCase ("JACK")
                                        ? DarkLookAndFeel::playing()
                                        : DarkLookAndFeel::armed());
        audioStatusLabel.setTooltip (device->getName());
    }
    else
    {
        audioStatusLabel.setText ("NO AUDIO DEVICE", juce::dontSendNotification);
        audioStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        audioStatusLabel.setTooltip ("Start JACK or choose another backend in the Audio tab");
    }

    repaint (audioStatusPillArea);
}

void MainComponent::handleMidiAction (MidiAction action)
{
    juce::Logger::writeToLog ("Executing MIDI action: " + juce::String (getMidiActionName (action)));

    switch (action)
    {
        case MidiAction::Record:
            audioEngine.record();
            break;

        case MidiAction::StopRecording:
            audioEngine.stopRecording();
            break;

        case MidiAction::DiscardLayer:
            audioEngine.discardLiveLayer();
            break;

        case MidiAction::PlayPause:
            if (! audioEngine.getLiveLooper().isRecordingOrArmed())
                audioEngine.getTransport().togglePlayPause();
            break;

        case MidiAction::ClearLayers:
            audioEngine.clearLiveLayers();
            break;

        case MidiAction::BackingMute:
            audioEngine.toggleBackingMute();
            break;

        case MidiAction::InputMonitor:
            audioEngine.toggleInputMonitoring();
            break;

        case MidiAction::Metronome:
            audioEngine.getMetronome().setEnabled (! audioEngine.getMetronome().isEnabled());
            break;
    }
}

void MainComponent::setPerformanceMode (bool shouldBeEnabled)
{
    if (performanceMode == shouldBeEnabled)
        return;

    performanceMode = shouldBeEnabled;

    tabs.setVisible (! performanceMode);
    performancePanel->setVisible (performanceMode);
    titleLabel.setVisible (! performanceMode);
    subtitleLabel.setVisible (! performanceMode);
    audioStatusLabel.setVisible (! performanceMode);
    performanceButton.setVisible (! performanceMode);

    for (auto& button : navButtons)
        button.setVisible (! performanceMode);

    resized();

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        window->setFullScreen (performanceMode);

    if (performanceMode)
        performancePanel->grabKeyboardFocus();
    else
        tabs.grabKeyboardFocus();
}

void MainComponent::toggleFullScreen()
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        window->setFullScreen (! window->isFullScreen());
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::F11Key)
    {
        if (performanceMode)
            toggleFullScreen();
        else
            setPerformanceMode (true);

        return true;
    }

    if (performanceMode && key == juce::KeyPress::escapeKey)
    {
        setPerformanceMode (false);
        return true;
    }

    return false;
}

void MainComponent::paint (juce::Graphics& g)
{
    juce::ColourGradient backgroundGradient (DarkLookAndFeel::panel().darker (0.35f),
                                             0.0f, 0.0f,
                                             DarkLookAndFeel::background(),
                                             0.0f, static_cast<float> (getHeight()),
                                             false);
    g.setGradientFill (backgroundGradient);
    g.fillAll();

    if (performanceMode)
        return;

    g.setGradientFill (juce::ColourGradient (DarkLookAndFeel::accent().withAlpha (0.07f),
                                             0.0f, 0.0f,
                                             juce::Colours::transparentBlack,
                                             380.0f, 0.0f, true));
    g.fillRect (0, 0, 380, 84);

    auto brandMark = juce::Rectangle<float> (22.0f, 22.0f, 40.0f, 40.0f);
    g.setColour (DarkLookAndFeel::panelHigh());
    g.fillRoundedRectangle (brandMark, 11.0f);
    g.setColour (DarkLookAndFeel::outline());
    g.drawRoundedRectangle (brandMark.reduced (0.5f), 11.0f, 1.0f);
    DarkLookAndFeel::drawIcon (g, "loop", brandMark.reduced (9.0f),
                               DarkLookAndFeel::accent(), 2.2f);

    g.setGradientFill (juce::ColourGradient (DarkLookAndFeel::accent().withAlpha (0.55f),
                                             0.0f, 83.0f,
                                             DarkLookAndFeel::outlineSoft().withAlpha (0.45f),
                                             static_cast<float> (getWidth()), 83.0f,
                                             false));
    g.fillRect (0, 83, getWidth(), 1);

    if (! navDockArea.isEmpty())
    {
        DarkLookAndFeel::drawPill (g, navDockArea,
                                   DarkLookAndFeel::panel().withAlpha (0.64f),
                                   DarkLookAndFeel::outlineSoft(), 12.0f);

        auto hint = navDockArea.reduced (16, 0).removeFromRight (170);
        g.setColour (DarkLookAndFeel::mutedText().withAlpha (0.72f));
        g.setFont (DarkLookAndFeel::kicker (9.5f));
        g.drawText ("F11  |  STAGE VIEW", hint,
                    juce::Justification::centredRight, false);
    }

    if (! audioStatusPillArea.isEmpty())
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        const auto type = deviceManager.getCurrentAudioDeviceType();
        const auto statusColour = device == nullptr
                                    ? DarkLookAndFeel::recording()
                                    : type.equalsIgnoreCase ("JACK")
                                        ? DarkLookAndFeel::playing()
                                        : DarkLookAndFeel::armed();

        DarkLookAndFeel::drawPill (g, audioStatusPillArea,
                                   DarkLookAndFeel::panelAlt().withAlpha (0.72f),
                                   DarkLookAndFeel::outline(), 9.0f);

        const auto centre = juce::Point<float> (static_cast<float> (audioStatusPillArea.getX() + 14),
                                                static_cast<float> (audioStatusPillArea.getCentreY()));
        g.setColour (statusColour.withAlpha (0.18f));
        g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (centre));
        g.setColour (statusColour);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (centre));
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    if (performanceMode)
    {
        if (performancePanel != nullptr)
            performancePanel->setBounds (area);

        return;
    }

    if (performancePanel != nullptr)
        performancePanel->setBounds (area);

    auto header = area.removeFromTop (84);
    auto headerInner = header.reduced (22, 12);

    performanceButton.setBounds (headerInner.removeFromRight (164)
                                       .withSizeKeepingCentre (164, 38));
    headerInner.removeFromRight (12);

    audioStatusPillArea = headerInner.removeFromRight (224)
                                       .withSizeKeepingCentre (224, 32);
    audioStatusLabel.setBounds (audioStatusPillArea.withTrimmedLeft (26)
                                                    .withTrimmedRight (12));
    headerInner.removeFromRight (12);
    headerInner.removeFromLeft (52);

    titleLabel.setBounds (headerInner.removeFromTop (34));
    subtitleLabel.setBounds (headerInner);

    navDockArea = area.removeFromTop (58).reduced (16, 6);
    auto navArea = navDockArea.reduced (6, 2);

    for (auto& button : navButtons)
    {
        button.setBounds (navArea.removeFromLeft (118));
        navArea.removeFromLeft (4);
    }

    tabs.setBounds (area.reduced (16, 10));
}

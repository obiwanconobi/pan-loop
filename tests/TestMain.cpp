#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "audio/AudioFileLoader.h"
#include "audio/BufferSizeControl.h"
#include "audio/LiveLooper.h"
#include "audio/Playlist.h"
#include "audio/Transport.h"
#include "midi/MidiAction.h"
#include "midi/MidiMapping.h"
#include "midi/MidiProfile.h"
#include "midi/RawMidiInput.h"
#include "session/SessionStore.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
    struct TestContext
    {
        int checks = 0;
        int failures = 0;

        void check (bool condition, const juce::String& description)
        {
            ++checks;

            if (! condition)
            {
                ++failures;
                std::cout << "FAIL: " << description << "\n";
            }
        }

        void close (double actual, double expected, double tolerance,
                    const juce::String& description)
        {
            check (std::abs (actual - expected) <= tolerance,
                   description + " (actual " + juce::String (actual, 6)
                       + ", expected " + juce::String (expected, 6) + ")");
        }
    };

    class FakeAudioDevice final : public juce::AudioIODevice
    {
    public:
        FakeAudioDevice (double rateToUse, int bufferSizeToUse)
            : juce::AudioIODevice ("Test device", "Test"),
              rate (rateToUse), bufferSize (bufferSizeToUse)
        {
        }

        juce::StringArray getOutputChannelNames() override { return { "Out 1", "Out 2" }; }
        juce::StringArray getInputChannelNames() override  { return { "In 1", "In 2" }; }
        juce::Array<double> getAvailableSampleRates() override { return { rate }; }
        juce::Array<int> getAvailableBufferSizes() override { return { 128, 256, 512, 1024 }; }
        int getDefaultBufferSize() override { return bufferSize; }

        juce::String open (const juce::BigInteger&, const juce::BigInteger&,
                           double, int) override
        {
            openFlag = true;
            return {};
        }

        void close() override { openFlag = false; }
        bool isOpen() override { return openFlag; }

        void start (juce::AudioIODeviceCallback* newCallback) override
        {
            callback = newCallback;

            if (callback != nullptr)
                callback->audioDeviceAboutToStart (this);
        }

        void stop() override
        {
            if (callback != nullptr)
                callback->audioDeviceStopped();

            callback = nullptr;
        }

        bool isPlaying() override { return callback != nullptr; }
        juce::String getLastError() override { return {}; }
        int getCurrentBufferSizeSamples() override { return bufferSize; }
        double getCurrentSampleRate() override { return rate; }
        int getCurrentBitDepth() override { return 32; }
        juce::BigInteger getActiveOutputChannels() const override { return juce::BigInteger (3); }
        juce::BigInteger getActiveInputChannels() const override  { return juce::BigInteger (3); }
        int getOutputLatencyInSamples() override { return 64; }
        int getInputLatencyInSamples() override  { return 64; }

    private:
        double rate;
        int bufferSize;
        bool openFlag = false;
        juce::AudioIODeviceCallback* callback = nullptr;
    };

    std::shared_ptr<PlaylistItem> makePlaylistItem (int length,
                                                    float leftValue,
                                                    float rightValue,
                                                    double sampleRate = 48000.0)
    {
        auto item = std::make_shared<PlaylistItem>();
        item->name = "Item " + juce::String (length);
        item->sampleRate = sampleRate;
        item->audio.setSize (2, length);

        for (int channel = 0; channel < 2; ++channel)
        {
            auto* data = item->audio.getWritePointer (channel);
            juce::FloatVectorOperations::fill (data,
                                               channel == 0 ? leftValue : rightValue,
                                               length);
        }

        return item;
    }

    bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer,
                   double sampleRate, juce::String& error)
    {
        file.deleteFile();

        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();

        if (stream == nullptr)
        {
            error = "Could not create test WAV stream";
            return false;
        }

        juce::WavAudioFormat wav;
        auto options = juce::AudioFormatWriterOptions{}
                           .withSampleRate (sampleRate)
                           .withChannelLayout (juce::AudioChannelSet::stereo())
                           .withBitsPerSample (32)
                           .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

        auto writer = wav.createWriterFor (stream, options);

        if (writer == nullptr)
        {
            error = "Could not create test WAV writer";
            return false;
        }

        return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    }

    void testTransport (TestContext& test)
    {
        Transport transport;
        transport.prepare (48000.0);
        transport.setBpm (120.0);
        transport.setTimeSignature (4, 4);

        test.close (transport.getSamplesPerBeat(), 24000.0, 1.0e-9,
                    "Transport beat length at 120 BPM");
        test.close (transport.getSamplesPerBar(), 96000.0, 1.0e-9,
                    "Transport bar length at 120 BPM / 4-4");

        transport.setBpm (300.0);
        test.close (transport.getBpm(), 300.0, 1.0e-9, "Transport clamps BPM high");
        transport.setBpm (1.0);
        test.close (transport.getBpm(), 20.0, 1.0e-9, "Transport clamps BPM low");
        transport.setBpm (120.0);

        transport.play();
        transport.advance (24000);
        auto info = transport.getPositionInfo();
        test.check (info.state == Transport::State::Playing, "Transport is playing");
        test.close (info.seconds, 0.5, 1.0e-9, "Transport position in seconds");
        test.check (info.bar == 1 && info.beat == 2, "Transport reports bar 1 beat 2");

        transport.setCycleLengthSamples (48000);
        transport.advance (24000);
        test.check (transport.getPositionSamples() == 0, "Transport wraps at cycle boundary");

        transport.pause();
        const auto pausedPosition = transport.getPositionSamples();
        transport.advance (1000);
        test.check (transport.getPositionSamples() == pausedPosition,
                    "Paused transport does not advance");

        transport.stop();
        test.check (transport.isStopped() && transport.getPositionSamples() == 0,
                    "Transport stop rewinds to zero");
    }

    void testPlaylist (TestContext& test)
    {
        Playlist playlist;
        playlist.addItem (makePlaylistItem (100, 0.1f, 0.2f));
        playlist.addItem (makePlaylistItem (200, 0.3f, 0.4f));

        test.check (playlist.getTotalLengthSamples() == 300, "Playlist total length");
        test.check (! playlist.isEmpty(), "Playlist is not empty");

        auto snapshot = playlist.getSnapshot();
        test.check (snapshot.items.size() == 2, "Playlist snapshot item count");
        test.close (snapshot.items[0].durationSeconds, 100.0 / 48000.0, 1.0e-9,
                    "Playlist first item duration");
        test.check (snapshot.items[0].sourceFile == juce::File(),
                    "Playlist item starts without a source file");

        playlist.setItemGain (0, 0.5f);
        snapshot = playlist.getSnapshot();
        test.close (snapshot.items[0].gain, 0.5, 1.0e-6, "Playlist item gain");

        juce::AudioBuffer<float> destination (2, 5);
        destination.clear();
        playlist.addTo (destination, 0, 5, 90);

        test.close (destination.getSample (0, 0), 0.05, 1.0e-6,
                    "Playlist applies item gain at the end of an item");
        test.close (destination.getSample (1, 4), 0.1, 1.0e-6,
                    "Playlist uses the right channel and item gain");

        destination.clear();
        playlist.addTo (destination, 0, 5, 295);
        test.close (destination.getSample (0, 0), 0.3, 1.0e-6,
                    "Playlist wraps to the first item");
        test.close (destination.getSample (0, 4), 0.3, 1.0e-6,
                    "Playlist wraps within the requested block");

        playlist.moveItem (0, 1);
        snapshot = playlist.getSnapshot();
        test.check (snapshot.items.size() == 2
                        && snapshot.items[0].name == "Item 200"
                        && snapshot.items[1].name == "Item 100",
                    "Playlist reorder moves items");

        playlist.removeItem (0);
        test.check (playlist.getTotalLengthSamples() == 100, "Playlist remove recalculates length");

        playlist.clear();
        test.check (playlist.isEmpty() && playlist.getTotalLengthSamples() == 0,
                    "Playlist clear resets the sequence");
    }

    void testLiveLooper (TestContext& test)
    {
        Transport transport;
        transport.prepare (48000.0);

        LiveLooper looper (transport);
        looper.prepare (48000.0);

        std::vector<LiveLooper::SavedLayer> layers;
        layers.emplace_back();
        layers.back().name = "Restored layer";
        layers.back().audio.setSize (2, 1000);
        juce::FloatVectorOperations::fill (layers.back().audio.getWritePointer (0), 0.1f, 1000);
        juce::FloatVectorOperations::fill (layers.back().audio.getWritePointer (1), 0.2f, 1000);
        layers.back().gain = 0.5f;

        test.check (looper.restoreLayers (std::move (layers), 1000),
                    "LiveLooper restores saved layers");
        test.check (looper.hasLayers() && looper.getLayerCount() == 1,
                    "LiveLooper reports restored layers");

        auto snapshot = looper.getSnapshot();
        test.check (snapshot.layers.size() == 1
                        && snapshot.layers[0].name == "Restored layer",
                    "LiveLooper snapshot describes the restored layer");
        test.close (snapshot.layers[0].gain, 0.5, 1.0e-6, "LiveLooper restored gain");

        auto saved = looper.copyLayersForSave();
        test.check (saved.size() == 1 && saved[0].audio.getNumSamples() == 1000,
                    "LiveLooper copies layers for saving");
        test.close (saved[0].audio.getSample (0, 500), 0.1, 1.0e-6,
                    "LiveLooper copied audio matches");

        juce::AudioBuffer<float> mix (2, 1000);
        mix.clear();
        looper.addToMixForRender (mix, 1000, 0, 1000);
        test.close (mix.getSample (0, 0), 0.05, 1.0e-6,
                    "Offline blend applies layer gain");
        test.close (mix.getSample (1, 999), 0.1, 1.0e-6,
                    "Offline blend covers the full cycle");

        looper.setLayerPan (snapshot.layers[0].id, -1.0f);
        mix.clear();
        looper.addToMixForRender (mix, 1000, 0, 1000);
        test.close (mix.getSample (0, 0), 0.05, 1.0e-6,
                    "Hard-left pan keeps the left channel");
        test.close (mix.getSample (1, 0), 0.0, 1.0e-6,
                    "Hard-left pan silences the right channel");

        looper.setLayerPan (snapshot.layers[0].id, 0.0f);
        looper.setLayerMuted (snapshot.layers[0].id, true);
        mix.clear();
        looper.addToMixForRender (mix, 1000, 0, 1000);
        test.close (mix.getSample (0, 0), 0.0, 1.0e-9,
                    "Muted layers are excluded from the offline blend");

        looper.clearLayers();
        test.check (! looper.hasLayers(), "LiveLooper clear removes all layers");

        std::vector<LiveLooper::SavedLayer> invalid;
        test.check (! looper.restoreLayers (std::move (invalid), 0),
                    "LiveLooper rejects an invalid cycle length");
    }

    void testMidiMappingHelpers (TestContext& test)
    {
        test.check (midiActionFromKey ("record") == MidiAction::Record,
                    "MIDI action key parses record");
        test.check (! midiActionFromKey ("unknown").has_value(),
                    "MIDI action key rejects unknown actions");
        test.check (midiMappingTypeFromString ("cc") == MidiMappingType::ControlChange,
                    "MIDI mapping type parses CC");
        test.check (midiMappingTypeFromString ("note") == MidiMappingType::NoteOn,
                    "MIDI mapping type parses note");
        test.check (midiMappingTypeFromString ("program") == MidiMappingType::ProgramChange,
                    "MIDI mapping type parses program change");
        test.check (! midiMappingTypeFromString ("pitchbend").has_value(),
                    "MIDI mapping type rejects unknown types");

        MidiMapping mapping { MidiAction::StopRecording, MidiMappingType::ControlChange, 2, 21 };
        const auto description = describeMidiMapping (mapping);
        test.check (description.contains ("CC 21") && description.contains ("ch 2"),
                    "MIDI mapping description includes type and channel");
    }

    void testBufferSizeMetadata (TestContext& test)
    {
        const juce::String metadata =
            "Found \"settings\" metadata 31\n"
            "update: id:0 key:'clock.rate' value:'48000' type:''\n"
            "update: id:0 key:'clock.quantum' value:'1024' type:''\n"
            "update: id:0 key:'clock.min-quantum' value:'32' type:''\n"
            "update: id:0 key:'clock.max-quantum' value:'2048' type:''\n"
            "update: id:0 key:'clock.force-quantum' value:'256' type:''\n";

        const auto state = BufferSizeControl::parsePipeWireMetadata (metadata);
        test.check (state.valid, "PipeWire metadata is recognised");
        test.check (state.quantum == 1024, "PipeWire quantum is parsed");
        test.check (state.forceQuantum == 256, "PipeWire forced quantum is parsed");
        test.check (state.minQuantum == 32 && state.maxQuantum == 2048,
                    "PipeWire quantum limits are parsed");

        const auto empty = BufferSizeControl::parsePipeWireMetadata ("no metadata here");
        test.check (! empty.valid, "Invalid PipeWire metadata is rejected");
    }

    void testRawMidiIdentifiers (TestContext& test)
    {
        RawMidi::DeviceInfo info;
        test.check (RawMidi::parseIdentifier ("rawmidi:3:0", info),
                    "Raw MIDI identifier is parsed");
        test.check (info.card == 3 && info.device == 0,
                    "Raw MIDI card and device are parsed");
        test.check (! RawMidi::parseIdentifier ("14-0", info),
                    "ALSA sequencer identifiers are not treated as raw MIDI");
        test.check (! RawMidi::parseIdentifier ("rawmidi:bad:0", info),
                    "Malformed raw MIDI identifiers are rejected");
    }

    void testMidiProfileSerialization (TestContext& test)
    {
        MidiProfile profile;
        profile.name = "Blackstar stage";
        profile.deviceName = "Live Logic MIDI Controller (raw MIDI)";
        profile.deviceIdentifier = "rawmidi:3:0";
        profile.mappings = {
            { MidiAction::Record, MidiMappingType::ControlChange, 1, 20 },
            { MidiAction::PlayPause, MidiMappingType::NoteOn, 2, 61 }
        };

        const auto json = MidiProfileStore::serialise ({ profile });
        const auto parsed = MidiProfileStore::parse (json);
        test.check (parsed.size() == 1, "MIDI preset serialises and parses");
        test.check (parsed[0].name == "Blackstar stage"
                        && parsed[0].deviceName == profile.deviceName
                        && parsed[0].deviceIdentifier == profile.deviceIdentifier,
                    "MIDI preset restores the device");
        test.check (parsed[0].mappings.size() == 2
                        && parsed[0].mappings[0].action == MidiAction::Record
                        && parsed[0].mappings[1].type == MidiMappingType::NoteOn,
                    "MIDI preset restores mappings");
        test.check (MidiProfileStore::parse ("{}").empty(),
                    "Invalid MIDI preset JSON is rejected");
    }

    void testSessionRoundTrip (TestContext& test, const juce::File& root)
    {
        FakeAudioDevice device (48000.0, 512);
        AudioEngine engine;
        engine.audioDeviceAboutToStart (&device);

        test.close (engine.getCurrentSampleRate(), 48000.0, 1.0e-9,
                    "Test engine opens at 48 kHz");

        const auto backingFile = root.getChildFile ("backing.wav");
        juce::AudioBuffer<float> backingAudio (2, 48000);
        backingAudio.clear();

        for (int channel = 0; channel < 2; ++channel)
            juce::FloatVectorOperations::fill (backingAudio.getWritePointer (channel), 0.1f, 48000);

        juce::String error;

        if (! writeWav (backingFile, backingAudio, 48000.0, error))
        {
            test.check (false, "Could create test backing WAV: " + error);
            return;
        }

        auto backingItem = AudioFileLoader::loadFileSynchronously (backingFile, 48000.0, error);
        test.check (backingItem != nullptr, "Session test decodes the backing WAV");

        if (backingItem == nullptr)
            return;

        backingItem->sourceFile = backingFile;
        engine.getPlaylist().addItem (backingItem);
        engine.playlistChanged();
        test.check (engine.getTransport().getCycleLengthSamples() == 48000,
                    "Playlist defines the session cycle");

        std::vector<LiveLooper::SavedLayer> layers;
        layers.emplace_back();
        layers.back().name = "Saved layer";
        layers.back().audio.setSize (2, 48000);
        juce::FloatVectorOperations::fill (layers.back().audio.getWritePointer (0), 0.2f, 48000);
        juce::FloatVectorOperations::fill (layers.back().audio.getWritePointer (1), 0.2f, 48000);
        layers.back().gain = 0.8f;
        layers.back().pan = -0.5f;
        layers.back().muted = false;
        layers.back().solo = false;

        test.check (engine.getLiveLooper().restoreLayers (std::move (layers), 48000),
                    "Session test restores a live layer");

        engine.getTransport().setBpm (128.0);
        engine.getTransport().setTimeSignature (3, 4);
        engine.setBarSnapEnabled (true);
        engine.getMetronome().setEnabled (true);
        engine.getMetronome().setLevel (0.3f);
        engine.setMonitoringEnabled (false);
        engine.setLatencyOffsetMs (12.5f);
        engine.setBackingMuted (false);

        const std::vector<MidiMapping> mappings {
            { MidiAction::Record, MidiMappingType::ControlChange, 1, 20 },
            { MidiAction::StopRecording, MidiMappingType::NoteOn, 2, 61 }
        };

        const auto sessionFile = root.getChildFile ("roundtrip.panloop");
        error = SessionStore::save (sessionFile, engine, mappings);
        test.check (error.isEmpty(), "Session saves without error: " + error);
        test.check (sessionFile.existsAsFile(), "Session file exists after saving");

        const auto layerDirectory = SessionStore::getLayerDirectory (sessionFile);
        test.check (layerDirectory.isDirectory(), "Session layer directory exists");
        test.check (layerDirectory.findChildFiles (juce::File::findFiles, false, "*.wav").size() == 1,
                    "Session writes one layer WAV");

        const auto json = juce::JSON::parse (sessionFile.loadFileAsString());
        auto* rootObject = json.getDynamicObject();
        test.check (rootObject != nullptr, "Session file is valid JSON");

        if (rootObject != nullptr)
        {
            test.check (rootObject->getProperty ("format").toString() == "pan-loop-session",
                        "Session file records the format marker");
            test.check (static_cast<int> (rootObject->getProperty ("version")) == 1,
                        "Session file records the version");
            test.check (std::abs (static_cast<double> (rootObject->getProperty ("sampleRate"))
                                    - 48000.0) < 1.0e-9,
                        "Session file records the sample rate");

            if (auto* playlistArray = rootObject->getProperty ("playlist").getArray())
                test.check (playlistArray->size() == 1, "Session JSON stores one playlist item");

            if (auto* layerArray = rootObject->getProperty ("layers").getArray())
                test.check (layerArray->size() == 1, "Session JSON stores one layer");

            if (auto* midiArray = rootObject->getProperty ("midiMappings").getArray())
                test.check (midiArray->size() == 2, "Session JSON stores MIDI mappings");
        }

        AudioEngine loadedEngine;
        loadedEngine.audioDeviceAboutToStart (&device);
        std::vector<MidiMapping> loadedMappings;
        error = SessionStore::load (sessionFile, loadedEngine, &loadedMappings);
        test.check (error.isEmpty(), "Session loads without error: " + error);

        test.close (loadedEngine.getTransport().getBpm(), 128.0, 1.0e-6,
                    "Loaded session restores BPM");
        test.check (loadedEngine.getTransport().getTimeSignatureNumerator() == 3
                        && loadedEngine.getTransport().getTimeSignatureDenominator() == 4,
                    "Loaded session restores the time signature");
        test.check (loadedEngine.isBarSnapEnabled(), "Loaded session restores bar snap");
        test.check (loadedEngine.getMetronome().isEnabled(), "Loaded session restores the metronome");
        test.close (loadedEngine.getMetronome().getLevel(), 0.3, 1.0e-6,
                    "Loaded session restores the metronome level");
        test.check (! loadedEngine.isMonitoringEnabled(),
                    "Loaded session restores input monitoring");
        test.close (loadedEngine.getLatencyOffsetMs(), 12.5, 1.0e-4,
                    "Loaded session restores the record offset");
        test.check (loadedEngine.getPlaylist().getSnapshot().items.size() == 1,
                    "Loaded session restores the playlist");
        test.check (loadedEngine.getLiveLooper().getSnapshot().layers.size() == 1,
                    "Loaded session restores layers");
        test.check (loadedEngine.getTransport().getCycleLengthSamples() == 48000,
                    "Loaded session restores the cycle length");
        test.check (loadedMappings.size() == 2,
                    "Loaded session provides the stored MIDI mappings");

        if (loadedMappings.size() == 2)
        {
            test.check (loadedMappings[0].action == MidiAction::Record
                            && loadedMappings[0].type == MidiMappingType::ControlChange
                            && loadedMappings[0].channel == 1
                            && loadedMappings[0].number == 20,
                        "Loaded session restores the first MIDI mapping");
            test.check (loadedMappings[1].action == MidiAction::StopRecording
                            && loadedMappings[1].type == MidiMappingType::NoteOn
                            && loadedMappings[1].channel == 2
                            && loadedMappings[1].number == 61,
                        "Loaded session restores the second MIDI mapping");
        }

        const auto loadedLayers = loadedEngine.getLiveLooper().copyLayersForSave();
        test.check (! loadedLayers.empty(), "Loaded layer can be copied for inspection");

        if (! loadedLayers.empty())
        {
            test.close (loadedLayers[0].gain, 0.8, 1.0e-6, "Loaded layer restores gain");
            test.close (loadedLayers[0].pan, -0.5, 1.0e-6, "Loaded layer restores pan");
            test.close (loadedLayers[0].audio.getSample (0, 1234), 0.2, 1.0e-6,
                        "Loaded layer restores audio");
        }

        const auto mixFile = root.getChildFile ("mixdown.wav");
        error = SessionStore::exportMixdown (mixFile, loadedEngine);
        test.check (error.isEmpty(), "Mixdown exports without error: " + error);
        test.check (mixFile.existsAsFile(), "Mixdown file exists");

        auto mixed = AudioFileLoader::loadFileSynchronously (mixFile, 48000.0, error);
        test.check (mixed != nullptr, "Mixdown decodes as a WAV file");

        if (mixed != nullptr)
        {
            test.check (mixed->getNumSamples() == 48000, "Mixdown is exactly one cycle long");
            test.close (mixed->audio.getSample (0, 100), 0.26, 2.0e-4,
                        "Mixdown sums backing and live layer with gain");
            test.close (mixed->audio.getSample (1, 47999), 0.18, 2.0e-4,
                        "Mixdown applies layer pan and covers the full cycle");
        }

        FakeAudioDevice wrongRateDevice (44100.0, 512);
        AudioEngine wrongRateEngine;
        wrongRateEngine.audioDeviceAboutToStart (&wrongRateDevice);
        error = SessionStore::load (sessionFile, wrongRateEngine);
        test.check (error.contains ("sample rate"),
                    "Session load rejects a sample-rate mismatch");
        test.check (wrongRateEngine.getPlaylist().isEmpty(),
                    "Rejected session load leaves the playlist untouched");

        error = SessionStore::load (root.getChildFile ("missing.panloop"), loadedEngine);
        test.check (error.contains ("does not exist"),
                    "Session load reports a missing file");

        error = SessionStore::exportMixdown (root.getChildFile ("empty.wav"), wrongRateEngine);
        test.check (error.isNotEmpty(), "Mixdown export fails when there is no cycle");

        loadedEngine.record();
        test.check (loadedEngine.getLiveLooper().isRecordingOrArmed(),
                    "Engine starts an overdub take for the save guard test");
        error = SessionStore::save (root.getChildFile ("blocked.panloop"), loadedEngine);
        test.check (error.contains ("Stop recording"),
                    "Session save refuses to run while recording");
        loadedEngine.clearLiveLayers();
    }
}

int main()
{
    TestContext test;
    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("pan-loop-tests-"
                                         + juce::String (juce::Time::currentTimeMillis()));
    root.createDirectory();

    testTransport (test);
    testPlaylist (test);
    testLiveLooper (test);
    testMidiMappingHelpers (test);
    testBufferSizeMetadata (test);
    testRawMidiIdentifiers (test);
    testMidiProfileSerialization (test);
    testSessionRoundTrip (test, root);

    root.deleteRecursively();

    std::cout << (test.failures == 0 ? "PASS" : "FAIL")
              << ": " << test.checks << " checks, "
              << test.failures << " failures\n";

    return test.failures == 0 ? 0 : 1;
}

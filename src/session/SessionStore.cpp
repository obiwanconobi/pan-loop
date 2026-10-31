#include "SessionStore.h"

#include "audio/AudioEngine.h"
#include "audio/AudioFileLoader.h"
#include "audio/LiveLooper.h"
#include "audio/Playlist.h"
#include "audio/Transport.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace SessionStore
{
    namespace
    {
        constexpr int sessionVersion = 1;
        constexpr const char* sessionFormat = "pan-loop-session";

        double getDouble (const juce::var& value, double fallback)
        {
            return value.isDouble() || value.isInt() || value.isInt64()
                     ? static_cast<double> (value)
                     : fallback;
        }

        int getInt (const juce::var& value, int fallback)
        {
            if (value.isInt())
                return static_cast<int> (value);

            if (value.isInt64())
                return static_cast<int> (juce::jlimit (
                    static_cast<juce::int64> (std::numeric_limits<int>::min()),
                    static_cast<juce::int64> (std::numeric_limits<int>::max()),
                    static_cast<juce::int64> (value)));

            if (value.isDouble())
            {
                const auto number = static_cast<double> (value);

                if (! std::isfinite (number))
                    return fallback;

                return static_cast<int> (juce::jlimit (
                    static_cast<double> (std::numeric_limits<int>::min()),
                    static_cast<double> (std::numeric_limits<int>::max()),
                    number));
            }

            return fallback;
        }

        juce::int64 getInt64 (const juce::var& value, juce::int64 fallback)
        {
            if (value.isInt() || value.isInt64())
                return static_cast<juce::int64> (value);

            if (value.isDouble())
            {
                const auto number = static_cast<double> (value);

                if (! std::isfinite (number))
                    return fallback;

                // Stay inside the exactly-representable range before casting.
                return static_cast<juce::int64> (juce::jlimit (-9.0e18, 9.0e18, number));
            }

            return fallback;
        }

        bool getBool (const juce::var& value, bool fallback)
        {
            if (value.isBool())
                return static_cast<bool> (value);

            if (value.isInt() || value.isInt64())
                return static_cast<int> (value) != 0;

            return fallback;
        }

        juce::String getString (const juce::var& value, const juce::String& fallback = {})
        {
            return value.isString() ? value.toString() : fallback;
        }

        juce::File resolvePlaylistFile (const juce::File& sessionFile, const juce::String& path)
        {
            juce::File file (path);

            if (! juce::File::isAbsolutePath (path))
                file = sessionFile.getParentDirectory().getChildFile (path);

            return file;
        }

        bool writeWav (const juce::AudioBuffer<float>& buffer,
                       double sampleRate,
                       const juce::File& file,
                       juce::String& error)
        {
            file.deleteFile();

            std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();

            if (stream == nullptr)
            {
                error = "Could not create " + file.getFullPathName();
                return false;
            }

            juce::WavAudioFormat wav;
            const auto options = juce::AudioFormatWriterOptions{}
                                     .withSampleRate (sampleRate)
                                     .withChannelLayout (juce::AudioChannelSet::stereo())
                                     .withBitsPerSample (32)
                                     .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

            auto writer = wav.createWriterFor (stream, options);

            if (writer == nullptr)
            {
                error = "Could not create WAV writer for " + file.getFullPathName();
                return false;
            }

            if (! writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()))
            {
                error = "Could not write audio data to " + file.getFullPathName();
                return false;
            }

            return true;
        }

        juce::String normaliseSessionFile (const juce::File& sessionFile)
        {
            if (sessionFile == juce::File())
                return "No session file selected.";

            if (sessionFile.isDirectory())
                return "The session path is a directory; choose a .panloop file.";

            return {};
        }
    }

    juce::File getLayerDirectory (const juce::File& sessionFile)
    {
        const auto baseName = sessionFile.getFileNameWithoutExtension();
        return sessionFile.getSiblingFile (baseName + ".layers");
    }

    juce::String save (const juce::File& sessionFile,
                       AudioEngine& engine,
                       const std::vector<MidiMapping>& midiMappings)
    {
        auto target = sessionFile;

        if (! target.hasFileExtension ("panloop"))
            target = target.withFileExtension ("panloop");

        if (const auto error = normaliseSessionFile (target); error.isNotEmpty())
            return error;

        if (engine.getLiveLooper().isRecordingOrArmed())
            return "Stop recording before saving a session.";

        const auto sampleRate = engine.getCurrentSampleRate();

        if (sampleRate <= 0.0)
            return "No audio device is open.";

        target.getParentDirectory().createDirectory();

        auto layerDirectory = getLayerDirectory (target);
        auto savedLayers = engine.getLiveLooper().copyLayersForSave();

        juce::Array<juce::var> layerArray;

        if (! savedLayers.empty())
        {
            layerDirectory.createDirectory();

            const auto oldLayers = layerDirectory.findChildFiles (juce::File::findFiles,
                                                                   false,
                                                                   "layer-*.wav");

            for (const auto& oldLayer : oldLayers)
                oldLayer.deleteFile();

            int index = 0;

            for (const auto& layer : savedLayers)
            {
                const auto file = layerDirectory.getChildFile ("layer-"
                                                               + juce::String (++index)
                                                               + ".wav");
                juce::String error;

                if (! writeWav (layer.audio, sampleRate, file, error))
                    return error;

                auto* object = new juce::DynamicObject();
                object->setProperty ("file", file.getFileName());
                object->setProperty ("name", layer.name);
                object->setProperty ("gain", layer.gain);
                object->setProperty ("pan", layer.pan);
                object->setProperty ("muted", layer.muted);
                object->setProperty ("solo", layer.solo);
                layerArray.add (juce::var (object));
            }
        }
        else if (layerDirectory.isDirectory())
        {
            // A session saved with no live layers must not leave stale sidecar WAVs.
            layerDirectory.deleteRecursively();
        }

        auto* root = new juce::DynamicObject();
        root->setProperty ("format", sessionFormat);
        root->setProperty ("version", sessionVersion);
        root->setProperty ("sampleRate", sampleRate);
        root->setProperty ("cycleLengthSamples",
                           engine.getTransport().getCycleLengthSamples());

        if (! layerArray.isEmpty())
            root->setProperty ("layerDirectory", layerDirectory.getFileName());

        auto* transportObject = new juce::DynamicObject();
        transportObject->setProperty ("bpm", engine.getTransport().getBpm());
        transportObject->setProperty ("numerator", engine.getTransport().getTimeSignatureNumerator());
        transportObject->setProperty ("denominator", engine.getTransport().getTimeSignatureDenominator());
        transportObject->setProperty ("barSnap", engine.isBarSnapEnabled());
        root->setProperty ("transport", juce::var (transportObject));

        auto* metronomeObject = new juce::DynamicObject();
        metronomeObject->setProperty ("enabled", engine.getMetronome().isEnabled());
        metronomeObject->setProperty ("level", engine.getMetronome().getLevel());
        root->setProperty ("metronome", juce::var (metronomeObject));

        auto* mixObject = new juce::DynamicObject();
        mixObject->setProperty ("monitoring", engine.isMonitoringEnabled());
        mixObject->setProperty ("backingMuted", engine.isBackingMuted());
        mixObject->setProperty ("recordOffsetMs", engine.getLatencyOffsetMs());
        root->setProperty ("mix", juce::var (mixObject));

        juce::Array<juce::var> playlistArray;
        const auto playlist = engine.getPlaylist().getSnapshot();

        for (const auto& item : playlist.items)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty ("path", item.sourceFile.getFullPathName());
            object->setProperty ("gain", item.gain);
            playlistArray.add (juce::var (object));
        }

        root->setProperty ("playlist", playlistArray);
        root->setProperty ("layers", layerArray);

        juce::Array<juce::var> midiArray;

        for (const auto& mapping : midiMappings)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty ("action", getMidiActionKey (mapping.action));
            object->setProperty ("type", midiMappingTypeToString (mapping.type));
            object->setProperty ("channel", mapping.channel);
            object->setProperty ("number", mapping.number);
            midiArray.add (juce::var (object));
        }

        root->setProperty ("midiMappings", midiArray);

        const auto json = juce::JSON::toString (juce::var (root), true);

        if (! target.replaceWithText (json))
            return "Could not write session file " + target.getFullPathName();

        juce::Logger::writeToLog ("Saved session: " + target.getFullPathName());
        return {};
    }

    juce::String load (const juce::File& sessionFile,
                       AudioEngine& engine,
                       std::vector<MidiMapping>* mappingsOut)
    {
        if (const auto error = normaliseSessionFile (sessionFile); error.isNotEmpty())
            return error;

        if (! sessionFile.existsAsFile())
            return "Session file does not exist: " + sessionFile.getFullPathName();

        const auto parsed = juce::JSON::parse (sessionFile.loadFileAsString());
        auto* root = parsed.getDynamicObject();

        if (root == nullptr)
            return "Session file is not valid JSON.";

        if (getString (root->getProperty ("format")) != sessionFormat)
            return "Not a Pan Loop session file.";

        if (getInt (root->getProperty ("version"), 0) != sessionVersion)
            return "Unsupported Pan Loop session version.";

        const auto sessionSampleRate = getDouble (root->getProperty ("sampleRate"), 0.0);
        const auto currentSampleRate = engine.getCurrentSampleRate();

        if (currentSampleRate <= 0.0)
            return "No audio device is open.";

        if (sessionSampleRate > 0.0 && std::abs (sessionSampleRate - currentSampleRate) > 0.5)
        {
            return "Session sample rate is " + juce::String (sessionSampleRate, 1)
                 + " Hz but the audio device is at " + juce::String (currentSampleRate, 1)
                 + " Hz. Change the sample rate before loading.";
        }

        std::vector<MidiMapping> loadedMappings;

        if (auto* midiArray = root->getProperty ("midiMappings").getArray())
        {
            for (const auto& mappingVar : *midiArray)
            {
                auto* object = mappingVar.getDynamicObject();

                if (object == nullptr)
                    continue;

                const auto action = midiActionFromKey (
                    getString (object->getProperty ("action")));
                const auto type = midiMappingTypeFromString (
                    getString (object->getProperty ("type")));
                const auto channel = getInt (object->getProperty ("channel"), 1);
                const auto number = getInt (object->getProperty ("number"), -1);

                if (! action.has_value()
                    || ! type.has_value()
                    || channel < 0 || channel > 16
                    || number < 0 || number > 127)
                    continue;

                loadedMappings.push_back ({ *action, *type, channel, number });
            }
        }

        std::vector<std::shared_ptr<PlaylistItem>> loadedPlaylist;
        juce::int64 playlistLength = 0;

        if (auto* playlistArray = root->getProperty ("playlist").getArray())
        {
            for (const auto& itemVar : *playlistArray)
            {
                auto* object = itemVar.getDynamicObject();

                if (object == nullptr)
                    continue;

                const auto path = getString (object->getProperty ("path"));
                const auto file = resolvePlaylistFile (sessionFile, path);

                if (! file.existsAsFile())
                    return "Missing backing file: " + file.getFullPathName();

                juce::String error;
                auto item = AudioFileLoader::loadFileSynchronously (file, currentSampleRate, error);

                if (item == nullptr)
                    return error;

                item->gain.store (static_cast<float> (getDouble (object->getProperty ("gain"), 1.0)),
                                  std::memory_order_relaxed);
                playlistLength += item->getNumSamples();
                loadedPlaylist.push_back (std::move (item));
            }
        }

        std::vector<LiveLooper::SavedLayer> loadedLayers;
        const auto layerDirectoryName = getString (root->getProperty ("layerDirectory"));
        auto layerDirectory = layerDirectoryName.isNotEmpty()
                                ? sessionFile.getSiblingFile (layerDirectoryName)
                                : getLayerDirectory (sessionFile);

        if (! layerDirectory.isDirectory())
            layerDirectory = getLayerDirectory (sessionFile);

        if (auto* layerArray = root->getProperty ("layers").getArray())
        {
            for (const auto& layerVar : *layerArray)
            {
                auto* object = layerVar.getDynamicObject();

                if (object == nullptr)
                    continue;

                const auto layerFileName = getString (object->getProperty ("file"));
                auto layerFile = juce::File (layerFileName);

                if (! juce::File::isAbsolutePath (layerFileName))
                    layerFile = layerDirectory.getChildFile (layerFile.getFileName());

                if (! layerFile.existsAsFile())
                    return "Missing live layer file: " + layerFile.getFullPathName();

                juce::String error;
                auto item = AudioFileLoader::loadFileSynchronously (layerFile, currentSampleRate, error);

                if (item == nullptr)
                    return error;

                LiveLooper::SavedLayer saved;
                saved.name = getString (object->getProperty ("name"), item->name);
                saved.audio = std::move (item->audio);
                saved.gain = static_cast<float> (getDouble (object->getProperty ("gain"), 1.0));
                saved.pan = static_cast<float> (juce::jlimit (-1.0, 1.0,
                                                             getDouble (object->getProperty ("pan"), 0.0)));
                saved.muted = getBool (object->getProperty ("muted"), false);
                saved.solo = getBool (object->getProperty ("solo"), false);
                loadedLayers.push_back (std::move (saved));
            }
        }

        auto cycleLength = playlistLength;

        if (cycleLength <= 0)
        {
            cycleLength = getInt64 (root->getProperty ("cycleLengthSamples"), 0);

            if (cycleLength <= 0 && ! loadedLayers.empty())
                cycleLength = loadedLayers.front().audio.getNumSamples();
        }

        if (! loadedLayers.empty() && cycleLength <= 0)
            return "Session cycle length is invalid.";

        if (cycleLength > static_cast<juce::int64> (std::numeric_limits<int>::max()))
            return "Session cycle is too long.";

        engine.getTransport().stop();
        engine.clearLiveLayers();
        engine.getPlaylist().clear();

        for (auto& item : loadedPlaylist)
            engine.getPlaylist().addItem (item);

        engine.playlistChanged();

        if (playlistLength > 0)
            cycleLength = engine.getPlaylist().getTotalLengthSamples();

        if (! loadedLayers.empty())
        {
            if (! engine.getLiveLooper().restoreLayers (std::move (loadedLayers), cycleLength))
                return "Could not restore live layers.";
        }

        if (playlistLength <= 0 && cycleLength > 0)
            engine.getTransport().setCycleLengthSamples (cycleLength);

        if (auto* transportObject = root->getProperty ("transport").getDynamicObject())
        {
            engine.getTransport().setBpm (getDouble (transportObject->getProperty ("bpm"), 120.0));
            engine.getTransport().setTimeSignature (
                getInt (transportObject->getProperty ("numerator"), 4),
                getInt (transportObject->getProperty ("denominator"), 4));
            engine.setBarSnapEnabled (getBool (transportObject->getProperty ("barSnap"), false));
        }

        if (auto* metronomeObject = root->getProperty ("metronome").getDynamicObject())
        {
            engine.getMetronome().setEnabled (getBool (metronomeObject->getProperty ("enabled"), false));
            engine.getMetronome().setLevel (
                static_cast<float> (getDouble (metronomeObject->getProperty ("level"), 0.6)));
        }

        if (auto* mixObject = root->getProperty ("mix").getDynamicObject())
        {
            engine.setMonitoringEnabled (getBool (mixObject->getProperty ("monitoring"), true));
            engine.setBackingMuted (getBool (mixObject->getProperty ("backingMuted"), false));
            engine.setLatencyOffsetMs (
                static_cast<float> (getDouble (mixObject->getProperty ("recordOffsetMs"), 0.0)));
        }

        engine.getTransport().stop();

        if (mappingsOut != nullptr)
            *mappingsOut = std::move (loadedMappings);

        juce::Logger::writeToLog ("Loaded session: " + sessionFile.getFullPathName());
        return {};
    }

    juce::String exportMixdown (const juce::File& wavFile, AudioEngine& engine)
    {
        auto target = wavFile;

        if (! target.hasFileExtension ("wav"))
            target = target.withFileExtension ("wav");

        const auto cycleLength = engine.getTransport().getCycleLengthSamples();

        if (cycleLength <= 0)
            return "There is no loop cycle to export.";

        if (cycleLength > static_cast<juce::int64> (std::numeric_limits<int>::max()))
            return "The loop is too long to export.";

        if (engine.getPlaylist().isEmpty() && ! engine.getLiveLooper().hasLayers())
            return "There is no playlist or live-layer audio to export.";

        const auto sampleRate = engine.getCurrentSampleRate();

        if (sampleRate <= 0.0)
            return "No audio device is open.";

        const auto length = static_cast<int> (cycleLength);
        juce::AudioBuffer<float> mix (2, length);
        mix.clear();

        if (! engine.isBackingMuted())
            engine.getPlaylist().addTo (mix, 0, length, 0);

        engine.getLiveLooper().addToMixForRender (mix, length, 0, cycleLength);
        mix.applyGain (engine.getOutputGainLinear());

        juce::String error;

        if (! writeWav (mix, sampleRate, target, error))
            return error;

        juce::Logger::writeToLog ("Exported mixdown: " + target.getFullPathName());
        return {};
    }
}

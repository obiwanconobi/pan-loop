#pragma once

#include <JuceHeader.h>

#include "midi/MidiMapping.h"

#include <vector>

class AudioEngine;

namespace SessionStore
{
    /** Saves the current playlist, layers, MIDI mappings and session settings.
        Returns an empty string on success, otherwise an error message. */
    juce::String save (const juce::File& sessionFile,
                       AudioEngine& engine,
                       const std::vector<MidiMapping>& midiMappings = {});

    /** Loads a session file, replacing the current playlist and live layers.
        When mappingsOut is supplied it receives the session's MIDI mappings.
        Returns an empty string on success, otherwise an error message. */
    juce::String load (const juce::File& sessionFile,
                       AudioEngine& engine,
                       std::vector<MidiMapping>* mappingsOut = nullptr);

    /** Renders one cycle of the current mix to a stereo float WAV file. */
    juce::String exportMixdown (const juce::File& wavFile, AudioEngine& engine);

    /** The sidecar directory used for recorded layer WAV files. */
    juce::File getLayerDirectory (const juce::File& sessionFile);
}

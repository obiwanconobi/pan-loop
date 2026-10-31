#pragma once

#include <JuceHeader.h>

#include <vector>

/**
    Buffer-size helpers for JACK and PipeWire.

    Native JACK owns its period, but on Linux it is commonly provided by PipeWire.
    PipeWire exposes its quantum through the "settings" metadata, which can be set
    with pw-metadata. This helper keeps those platform details out of the audio UI.
*/
namespace BufferSizeControl
{
    struct PipeWireState
    {
        bool valid = false;
        int quantum = 0;
        int forceQuantum = 0;
        int minQuantum = 0;
        int maxQuantum = 0;
    };

    /** Parses the output of `pw-metadata -n settings`. Public for tests. */
    PipeWireState parsePipeWireMetadata (const juce::String& text);

    struct Info
    {
        bool pipeWireAvailable = false;
        bool jackBufferToolAvailable = false;
        juce::String provider;
        int minimumSamples = 32;
        int maximumSamples = 2048;
        int currentSamples = 0;
        std::vector<int> choices;
    };

    /** Detects the buffer-size mechanism available for the current JACK server. */
    Info query();

    /** Applies a JACK/PipeWire buffer size. Zero asks PipeWire for its automatic
        quantum. Returns an empty string on success, otherwise an error message. */
    juce::String apply (int samples);
}

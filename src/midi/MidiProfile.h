#pragma once

#include <JuceHeader.h>

#include "MidiMapping.h"

#include <vector>

/** A named MIDI setup: input device plus footswitch mappings. */
struct MidiProfile
{
    juce::String name;
    juce::String deviceName;
    juce::String deviceIdentifier;
    std::vector<MidiMapping> mappings;
};

namespace MidiProfileStore
{
    juce::String serialise (const std::vector<MidiProfile>& profiles);
    std::vector<MidiProfile> parse (const juce::String& json);
    std::vector<MidiProfile> load (const juce::File& file);
    bool save (const juce::File& file, const std::vector<MidiProfile>& profiles);
}

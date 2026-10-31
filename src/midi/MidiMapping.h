#pragma once

#include <JuceHeader.h>

#include "MidiAction.h"

#include <optional>

enum class MidiMappingType
{
    ControlChange = 0,
    NoteOn,
    ProgramChange
};

struct MidiMapping
{
    MidiAction action = MidiAction::Record;
    MidiMappingType type = MidiMappingType::ControlChange;
    int channel = 1; // 1-16, or 0 for omni
    int number = 0;  // CC number, MIDI note, or program number
};

inline juce::String midiMappingTypeToString (MidiMappingType type)
{
    switch (type)
    {
        case MidiMappingType::ControlChange: return "cc";
        case MidiMappingType::NoteOn:        return "note";
        case MidiMappingType::ProgramChange: return "program";
    }

    return "cc";
}

inline std::optional<MidiMappingType> midiMappingTypeFromString (const juce::String& text)
{
    if (text == "cc")
        return MidiMappingType::ControlChange;

    if (text == "note")
        return MidiMappingType::NoteOn;

    if (text == "program")
        return MidiMappingType::ProgramChange;

    return {};
}

inline juce::String describeMidiMapping (const MidiMapping& mapping)
{
    juce::String result;

    switch (mapping.type)
    {
        case MidiMappingType::ControlChange:
            result = "CC " + juce::String (mapping.number);
            break;

        case MidiMappingType::NoteOn:
            result = "Note " + juce::MidiMessage::getMidiNoteName (mapping.number, true, true, 3)
                   + " (" + juce::String (mapping.number) + ")";
            break;

        case MidiMappingType::ProgramChange:
            result = "Program " + juce::String (mapping.number);
            break;
    }

    result << (mapping.channel > 0 ? "  ch " : "  omni");

    if (mapping.channel > 0)
        result << mapping.channel;

    return result;
}

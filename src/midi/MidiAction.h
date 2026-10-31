#pragma once

#include <JuceHeader.h>

#include <optional>

/** Actions that can be assigned to a MIDI footswitch. */
enum class MidiAction
{
    Record = 0,
    StopRecording,
    DiscardLayer,
    PlayPause,
    ClearLayers,
    BackingMute,
    InputMonitor,
    Metronome
};

static constexpr int numMidiActions = 8;

inline const char* getMidiActionName (MidiAction action)
{
    switch (action)
    {
        case MidiAction::Record:        return "Record";
        case MidiAction::StopRecording: return "Stop Recording";
        case MidiAction::DiscardLayer:  return "Discard Layer";
        case MidiAction::PlayPause:     return "Play / Pause";
        case MidiAction::ClearLayers:   return "Clear Layers";
        case MidiAction::BackingMute:   return "Backing Mute";
        case MidiAction::InputMonitor:  return "Input Monitor";
        case MidiAction::Metronome:     return "Metronome";
    }

    return "Unknown";
}

inline const char* getMidiActionKey (MidiAction action)
{
    switch (action)
    {
        case MidiAction::Record:        return "record";
        case MidiAction::StopRecording: return "stop";
        case MidiAction::DiscardLayer:  return "discard";
        case MidiAction::PlayPause:     return "playpause";
        case MidiAction::ClearLayers:   return "clearlayers";
        case MidiAction::BackingMute:   return "backingmute";
        case MidiAction::InputMonitor:  return "inputmonitor";
        case MidiAction::Metronome:     return "metronome";
    }

    return "";
}

inline std::optional<MidiAction> midiActionFromKey (const juce::String& key)
{
    for (int i = 0; i < numMidiActions; ++i)
    {
        const auto action = static_cast<MidiAction> (i);

        if (key == getMidiActionKey (action))
            return action;
    }

    return {};
}

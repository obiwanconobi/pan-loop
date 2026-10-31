#pragma once

#include <JuceHeader.h>

#include <functional>
#include <vector>

/** ALSA raw-MIDI fallback for devices that are not exposed through the ALSA
    sequencer. PipeWire often grabs USB MIDI devices directly, leaving them
    invisible to JUCE's sequencer-based MidiInput backend. */
namespace RawMidi
{
    struct DeviceInfo
    {
        juce::String name;
        juce::String identifier;
        int card = 0;
        int device = 0;
    };

    std::vector<DeviceInfo> getAvailableDevices();

    /** Parses "rawmidi:<card>:<device>". */
    bool parseIdentifier (const juce::String& identifier, DeviceInfo& info);
}

class RawMidiInput final : private juce::Thread
{
public:
    RawMidiInput();
    ~RawMidiInput() override;

    bool open (int card, int device, int subdevice = 0);
    void close();
    bool isOpen() const noexcept;

    std::function<void(const juce::MidiMessage&)> onMessage;

private:
    void run() override;

    struct MessageCallback
    {
        std::function<void(const juce::MidiMessage&)> handler;

        void handleIncomingMidiMessage (void*, const juce::MidiMessage& message)
        {
            if (handler != nullptr)
                handler (message);
        }

        void handlePartialSysexMessage (void*, const juce::uint8*, int, double) {}
    };

    void* inputHandle = nullptr;
    juce::MidiDataConcatenator concatenator { 4096 };
    MessageCallback callback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RawMidiInput)
};

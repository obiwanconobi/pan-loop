#pragma once

#include <JuceHeader.h>

/** Owns the selected MIDI input and collects incoming messages for the UI.
    Phase 1 only monitors activity; MIDI learn and action dispatch come later. */
class MidiManager final : private juce::MidiInputCallback
{
public:
    explicit MidiManager (juce::AudioDeviceManager& deviceManager);
    ~MidiManager() override;

    void refreshDeviceList();
    void autoSelectLiveLogic();

    juce::Array<juce::MidiDeviceInfo> getAvailableDevices() const;
    juce::String getEnabledDeviceIdentifier() const;

    void setDevice (const juce::String& identifier);

    struct Snapshot
    {
        juce::uint64 messageCount = 0;
        juce::String lastMessage;
        juce::String lastDevice;
        double lastMessageTimeMs = 0.0;
        juce::String logText;
    };

    Snapshot getSnapshot() const;

    static juce::String describeMessage (const juce::MidiMessage&);

private:
    void handleIncomingMidiMessage (juce::MidiInput* source,
                                    const juce::MidiMessage& message) override;

    juce::AudioDeviceManager& deviceManager;

    mutable juce::CriticalSection lock;
    juce::String enabledIdentifier;
    juce::Array<juce::MidiDeviceInfo> availableDevices;

    juce::String lastMessage;
    juce::String lastDevice;
    double lastMessageTimeMs = 0.0;
    juce::StringArray logLines;
    juce::uint64 messageCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiManager)
};

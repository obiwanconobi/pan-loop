#include "MidiManager.h"

#include <algorithm>

namespace
{
    constexpr int maxLogLines = 40;
}

MidiManager::MidiManager (juce::AudioDeviceManager& manager)
    : deviceManager (manager)
{
    deviceManager.addMidiInputDeviceCallback ({}, this);
    refreshDeviceList();
    autoSelectLiveLogic();
}

MidiManager::~MidiManager()
{
    deviceManager.removeMidiInputDeviceCallback ({}, this);

    if (enabledIdentifier.isNotEmpty())
        deviceManager.setMidiInputDeviceEnabled (enabledIdentifier, false);
}

void MidiManager::refreshDeviceList()
{
    const auto devices = juce::MidiInput::getAvailableDevices();
    juce::String previousIdentifier;

    {
        const juce::ScopedLock sl (lock);
        availableDevices = devices;
        previousIdentifier = enabledIdentifier;
    }

    if (previousIdentifier.isNotEmpty())
    {
        const auto stillPresent = std::any_of (devices.begin(), devices.end(),
                                               [&previousIdentifier] (const juce::MidiDeviceInfo& d)
                                               {
                                                   return d.identifier == previousIdentifier;
                                               });

        if (! stillPresent)
        {
            deviceManager.setMidiInputDeviceEnabled (previousIdentifier, false);

            {
                const juce::ScopedLock sl (lock);
                enabledIdentifier.clear();
            }

            juce::Logger::writeToLog ("MIDI device disappeared: " + previousIdentifier);
        }
    }

}

void MidiManager::autoSelectLiveLogic()
{
    if (getEnabledDeviceIdentifier().isNotEmpty())
        return;

    for (const auto& device : getAvailableDevices())
    {
        if (device.name.containsIgnoreCase ("Live Logic"))
        {
            juce::Logger::writeToLog ("Auto-selecting MIDI device: " + device.name);
            setDevice (device.identifier);
            break;
        }
    }
}

juce::Array<juce::MidiDeviceInfo> MidiManager::getAvailableDevices() const
{
    const juce::ScopedLock sl (lock);
    return availableDevices;
}

juce::String MidiManager::getEnabledDeviceIdentifier() const
{
    const juce::ScopedLock sl (lock);
    return enabledIdentifier;
}

void MidiManager::setDevice (const juce::String& identifier)
{
    juce::String previous;

    {
        const juce::ScopedLock sl (lock);

        if (identifier == enabledIdentifier)
            return;

        previous = enabledIdentifier;
        enabledIdentifier = identifier;
    }

    if (previous.isNotEmpty())
        deviceManager.setMidiInputDeviceEnabled (previous, false);

    if (identifier.isNotEmpty())
    {
        deviceManager.setMidiInputDeviceEnabled (identifier, true);

        if (! deviceManager.isMidiInputDeviceEnabled (identifier))
        {
            juce::Logger::writeToLog ("Failed to enable MIDI input: " + identifier);

            const juce::ScopedLock sl (lock);
            enabledIdentifier.clear();
            return;
        }

        juce::Logger::writeToLog ("MIDI input enabled: " + identifier);
    }
    else
    {
        juce::Logger::writeToLog ("MIDI input disabled");
    }
}

MidiManager::Snapshot MidiManager::getSnapshot() const
{
    const juce::ScopedLock sl (lock);

    Snapshot snapshot;
    snapshot.messageCount = messageCount;
    snapshot.lastMessage = lastMessage;
    snapshot.lastDevice = lastDevice;
    snapshot.lastMessageTimeMs = lastMessageTimeMs;
    snapshot.logText = logLines.joinIntoString ("\n");
    return snapshot;
}

juce::String MidiManager::describeMessage (const juce::MidiMessage& message)
{
    const auto channel = message.getChannel();

    if (message.isController())
        return "CC #" + juce::String (message.getControllerNumber())
             + "  ch " + juce::String (channel)
             + "  value " + juce::String (message.getControllerValue());

    if (message.isNoteOn())
        return "Note On " + juce::MidiMessage::getMidiNoteName (message.getNoteNumber(), true, true, 3)
             + " (" + juce::String (message.getNoteNumber()) + ")"
             + "  ch " + juce::String (channel)
             + "  vel " + juce::String (message.getVelocity());

    if (message.isNoteOff())
        return "Note Off " + juce::MidiMessage::getMidiNoteName (message.getNoteNumber(), true, true, 3)
             + " (" + juce::String (message.getNoteNumber()) + ")"
             + "  ch " + juce::String (channel);

    if (message.isProgramChange())
        return "Program Change " + juce::String (message.getProgramChangeNumber())
             + "  ch " + juce::String (channel);

    if (message.isPitchWheel())
        return "Pitch Wheel " + juce::String (message.getPitchWheelValue())
             + "  ch " + juce::String (channel);

    if (message.isAftertouch())
        return "Aftertouch " + juce::String (message.getNoteNumber())
             + "  value " + juce::String (message.getAfterTouchValue())
             + "  ch " + juce::String (channel);

    if (message.isChannelPressure())
        return "Channel Pressure " + juce::String (message.getChannelPressureValue())
             + "  ch " + juce::String (channel);

    if (message.isSysEx())
        return "SysEx (" + juce::String (message.getRawDataSize()) + " bytes)";

    return "MIDI " + juce::String::toHexString (message.getRawData(),
                                                message.getRawDataSize(),
                                                2);
}

void MidiManager::handleIncomingMidiMessage (juce::MidiInput* source,
                                             const juce::MidiMessage& message)
{
    // Clock and active-sensing arrive continuously; don't feed them into the UI.
    if (message.isMidiClock() || message.isActiveSense())
        return;

    const auto deviceName = source != nullptr ? source->getName() : juce::String ("MIDI input");
    const auto description = describeMessage (message);
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    const auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S");

    const juce::ScopedLock sl (lock);

    lastMessage = description;
    lastDevice = deviceName;
    lastMessageTimeMs = nowMs;
    ++messageCount;

    logLines.add ("[" + timestamp + "] " + deviceName + ": " + description);

    while (logLines.size() > maxLogLines)
        logLines.remove (0);
}

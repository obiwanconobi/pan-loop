#include "MidiManager.h"

#include "RawMidiInput.h"
#include "core/AppConfig.h"

#include <algorithm>

namespace
{
    constexpr int maxLogLines = 40;
    constexpr double actionDebounceMs = 80.0;
}

MidiManager::MidiManager (juce::AudioDeviceManager& manager)
    : deviceManager (manager)
{
    deviceManager.addMidiInputDeviceCallback ({}, this);
    refreshDeviceList();
    autoSelectLiveLogic();
    loadMappings();
    loadProfiles();
}

MidiManager::~MidiManager()
{
    closeRawMidiDevice();
    deviceManager.removeMidiInputDeviceCallback ({}, this);

    if (enabledIdentifier.isNotEmpty() && ! enabledIdentifier.startsWith ("rawmidi:"))
        deviceManager.setMidiInputDeviceEnabled (enabledIdentifier, false);

    if (mappingDirty.load (std::memory_order_relaxed))
        saveMappings();
}

void MidiManager::refreshDeviceList()
{
    auto devices = juce::MidiInput::getAvailableDevices();

    // Some ALSA setups expose the same port through multiple endpoint views.
    juce::Array<juce::MidiDeviceInfo> uniqueDevices;

    for (const auto& device : devices)
    {
        const auto alreadyPresent = std::any_of (uniqueDevices.begin(), uniqueDevices.end(),
                                                 [&device] (const juce::MidiDeviceInfo& existing)
                                                 {
                                                     return existing.identifier == device.identifier;
                                                 });

        if (! alreadyPresent)
            uniqueDevices.add (device);
    }

    // ALSA sequencer clients are not created for some USB MIDI devices when
    // PipeWire owns the raw device. Fall back to opening those devices directly.
    for (const auto& rawDevice : RawMidi::getAvailableDevices())
    {
        const auto alreadyPresent = std::any_of (uniqueDevices.begin(), uniqueDevices.end(),
                                                 [&rawDevice] (const juce::MidiDeviceInfo& existing)
                                                 {
                                                     return existing.identifier == rawDevice.identifier;
                                                 });

        if (! alreadyPresent)
            uniqueDevices.add ({ rawDevice.name, rawDevice.identifier });
    }

    devices = std::move (uniqueDevices);
    juce::String previousIdentifier;

    for (const auto& device : devices)
        juce::Logger::writeToLog ("MIDI input device: " + device.name
                                  + " [" + device.identifier + "]");

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
            if (previousIdentifier.startsWith ("rawmidi:"))
                closeRawMidiDevice();
            else
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

bool MidiManager::selectDeviceMatching (const juce::String& nameContains)
{
    for (const auto& device : getAvailableDevices())
    {
        if (device.name.containsIgnoreCase (nameContains))
        {
            juce::Logger::writeToLog ("Selecting MIDI device by name: " + device.name);
            setDevice (device.identifier);
            return true;
        }
    }

    return false;
}

void MidiManager::setDevice (const juce::String& identifier)
{
    juce::String previous;

    {
        const juce::ScopedLock sl (lock);

        if (identifier == enabledIdentifier)
            return;

        previous = enabledIdentifier;
    }

    closeRawMidiDevice();

    if (previous.isNotEmpty() && ! previous.startsWith ("rawmidi:"))
        deviceManager.setMidiInputDeviceEnabled (previous, false);

    if (identifier.isEmpty())
    {
        const juce::ScopedLock sl (lock);
        enabledIdentifier.clear();
        juce::Logger::writeToLog ("MIDI input disabled");
        return;
    }

    if (identifier.startsWith ("rawmidi:"))
    {
        if (! openRawMidiDevice (identifier))
        {
            const juce::ScopedLock sl (lock);
            enabledIdentifier.clear();
            return;
        }
    }
    else
    {
        deviceManager.setMidiInputDeviceEnabled (identifier, true);

        if (! deviceManager.isMidiInputDeviceEnabled (identifier))
        {
            juce::Logger::writeToLog ("Failed to enable MIDI input: " + identifier);

            const juce::ScopedLock sl (lock);
            enabledIdentifier.clear();
            return;
        }
    }

    {
        const juce::ScopedLock sl (lock);
        enabledIdentifier = identifier;
    }

    juce::Logger::writeToLog ("MIDI input enabled: " + identifier);
}

bool MidiManager::openRawMidiDevice (const juce::String& identifier)
{
    RawMidi::DeviceInfo info;

    if (! RawMidi::parseIdentifier (identifier, info))
        return false;

    closeRawMidiDevice();

    {
        const juce::ScopedLock sl (lock);

        for (const auto& device : availableDevices)
        {
            if (device.identifier == identifier)
            {
                rawMidiDeviceName = device.name;
                break;
            }
        }
    }

    if (rawMidiDeviceName.isEmpty())
        rawMidiDeviceName = "Raw MIDI " + juce::String (info.card)
                          + ":" + juce::String (info.device);

    auto input = std::make_unique<RawMidiInput>();
    const auto deviceName = rawMidiDeviceName;
    input->onMessage = [this, deviceName] (const juce::MidiMessage& message)
    {
        processIncomingMidiMessage (deviceName, message);
    };

    if (! input->open (info.card, info.device))
    {
        juce::Logger::writeToLog ("Failed to open raw MIDI device: " + identifier);
        rawMidiDeviceName.clear();
        return false;
    }

    rawMidiInput = std::move (input);
    return true;
}

void MidiManager::closeRawMidiDevice()
{
    if (rawMidiInput != nullptr)
    {
        rawMidiInput->close();
        rawMidiInput.reset();
    }

    rawMidiDeviceName.clear();
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
    processIncomingMidiMessage (source != nullptr ? source->getName()
                                                  : juce::String ("MIDI input"),
                                message);
}

void MidiManager::processIncomingMidiMessage (const juce::String& deviceName,
                                              const juce::MidiMessage& message)
{
    // Clock and active-sensing arrive continuously; don't feed them into the UI.
    if (message.isMidiClock() || message.isActiveSense())
        return;

    const auto description = describeMessage (message);
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    const auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S");

    {
        const juce::ScopedLock sl (lock);

        lastMessage = description;
        lastDevice = deviceName;
        lastMessageTimeMs = nowMs;
        ++messageCount;

        logLines.add ("[" + timestamp + "] " + deviceName + ": " + description);

        while (logLines.size() > maxLogLines)
            logLines.remove (0);
    }

    const auto learning = learningAction.load (std::memory_order_relaxed);

    if (learning >= 0)
    {
        MidiMapping mapping;

        if (messageToMapping (message, mapping))
        {
            mapping.action = static_cast<MidiAction> (learning);
            setMapping (mapping);
            learningAction.store (-1, std::memory_order_relaxed);
            mappingDirty.store (true, std::memory_order_relaxed);
            triggerAsyncUpdate();

            juce::Logger::writeToLog ("MIDI learned " + juce::String (getMidiActionName (mapping.action))
                                      + " = " + describeMidiMapping (mapping));
        }

        return;
    }

    if (const auto action = findActionForMessage (message))
        dispatchAction (*action);
}

void MidiManager::injectMidiMessageForTest (const juce::MidiMessage& message)
{
    processIncomingMidiMessage ("Test MIDI", message);
}

void MidiManager::startLearning (MidiAction action)
{
    learningAction.store (static_cast<int> (action), std::memory_order_relaxed);
    juce::Logger::writeToLog ("MIDI learn started: " + juce::String (getMidiActionName (action)));
}

void MidiManager::cancelLearning()
{
    learningAction.store (-1, std::memory_order_relaxed);
    juce::Logger::writeToLog ("MIDI learn cancelled");
}

std::optional<MidiAction> MidiManager::getLearningAction() const
{
    const auto value = learningAction.load (std::memory_order_relaxed);

    if (value >= 0 && value < numMidiActions)
        return static_cast<MidiAction> (value);

    return {};
}

std::optional<MidiMapping> MidiManager::getMapping (MidiAction action) const
{
    const juce::ScopedLock sl (mappingLock);

    for (const auto& mapping : mappings)
        if (mapping.action == action)
            return mapping;

    return {};
}

std::vector<MidiMapping> MidiManager::getMappingsForSession() const
{
    const juce::ScopedLock sl (mappingLock);
    return mappings;
}

void MidiManager::restoreMappingsForSession (const std::vector<MidiMapping>& newMappings)
{
    cancelLearning();

    {
        const juce::ScopedLock sl (mappingLock);
        mappings = newMappings;
    }

    mappingDirty.store (false, std::memory_order_relaxed);
    saveMappings();
    juce::Logger::writeToLog ("Restored " + juce::String (static_cast<int> (newMappings.size()))
                              + " MIDI mappings from session");
}

juce::StringArray MidiManager::getProfileNames() const
{
    juce::StringArray names;

    for (const auto& profile : profiles)
        names.add (profile.name);

    names.sort (true);
    return names;
}

juce::String MidiManager::saveCurrentProfile (const juce::String& name)
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty())
        return "Enter a preset name.";

    MidiProfile profile;
    profile.name = trimmed;
    profile.deviceIdentifier = getEnabledDeviceIdentifier();

    for (const auto& device : getAvailableDevices())
    {
        if (device.identifier == profile.deviceIdentifier)
        {
            profile.deviceName = device.name;
            break;
        }
    }

    profile.mappings = getMappingsForSession();

    profiles.erase (std::remove_if (profiles.begin(), profiles.end(),
                                    [&trimmed] (const MidiProfile& existing)
                                    {
                                        return existing.name.equalsIgnoreCase (trimmed);
                                    }),
                    profiles.end());

    profiles.push_back (std::move (profile));

    if (! saveProfiles())
        return "Could not write the MIDI preset file.";

    juce::Logger::writeToLog ("Saved MIDI preset: " + trimmed);
    return {};
}

juce::String MidiManager::applyProfile (const juce::String& name)
{
    const auto* found = findProfile (name);

    if (found == nullptr)
        return "MIDI preset not found: " + name;

    const auto profile = *found;
    restoreMappingsForSession (profile.mappings);

    if (profile.deviceIdentifier.isEmpty() && profile.deviceName.isEmpty())
        return {};

    juce::MidiDeviceInfo target;
    bool foundDevice = false;
    const auto devices = getAvailableDevices();

    for (const auto& device : devices)
    {
        if (device.identifier == profile.deviceIdentifier)
        {
            target = device;
            foundDevice = true;
            break;
        }
    }

    if (! foundDevice)
    {
        for (const auto& device : devices)
        {
            if (device.name.equalsIgnoreCase (profile.deviceName))
            {
                target = device;
                foundDevice = true;
                break;
            }
        }
    }

    if (! foundDevice)
    {
        return "Mappings loaded, but " + profile.deviceName
             + " is not connected.";
    }

    setDevice (target.identifier);

    if (getEnabledDeviceIdentifier() != target.identifier)
    {
        return "Mappings loaded, but the MIDI device could not be opened: "
             + target.name;
    }

    juce::Logger::writeToLog ("Applied MIDI preset: " + name);
    return {};
}

juce::String MidiManager::deleteProfile (const juce::String& name)
{
    const auto oldSize = profiles.size();
    profiles.erase (std::remove_if (profiles.begin(), profiles.end(),
                                    [&name] (const MidiProfile& profile)
                                    {
                                        return profile.name.equalsIgnoreCase (name);
                                    }),
                    profiles.end());

    if (profiles.size() == oldSize)
        return "MIDI preset not found: " + name;

    if (! saveProfiles())
        return "Could not write the MIDI preset file.";

    juce::Logger::writeToLog ("Deleted MIDI preset: " + name);
    return {};
}

void MidiManager::clearMapping (MidiAction action)
{
    bool changed = false;

    {
        const juce::ScopedLock sl (mappingLock);
        const auto oldSize = mappings.size();
        mappings.erase (std::remove_if (mappings.begin(), mappings.end(),
                                        [action] (const MidiMapping& mapping)
                                        {
                                            return mapping.action == action;
                                        }),
                        mappings.end());
        changed = mappings.size() != oldSize;
    }

    if (const auto learning = getLearningAction(); learning.has_value() && *learning == action)
        learningAction.store (-1, std::memory_order_relaxed);

    if (changed)
    {
        mappingDirty.store (false, std::memory_order_relaxed);
        saveMappings();
        juce::Logger::writeToLog ("MIDI mapping cleared: " + juce::String (getMidiActionName (action)));
    }
}

void MidiManager::handleAsyncUpdate()
{
    const auto action = pendingAction.exchange (-1, std::memory_order_relaxed);

    if (action >= 0 && action < numMidiActions && onAction != nullptr)
        onAction (static_cast<MidiAction> (action));

    if (mappingDirty.exchange (false, std::memory_order_relaxed))
        saveMappings();
}

bool MidiManager::messageToMapping (const juce::MidiMessage& message, MidiMapping& mapping)
{
    if (message.isController())
    {
        if (message.getControllerValue() < 64)
            return false;

        mapping.type = MidiMappingType::ControlChange;
        mapping.channel = message.getChannel();
        mapping.number = message.getControllerNumber();
        return true;
    }

    if (message.isNoteOn())
    {
        mapping.type = MidiMappingType::NoteOn;
        mapping.channel = message.getChannel();
        mapping.number = message.getNoteNumber();
        return true;
    }

    if (message.isProgramChange())
    {
        mapping.type = MidiMappingType::ProgramChange;
        mapping.channel = message.getChannel();
        mapping.number = message.getProgramChangeNumber();
        return true;
    }

    return false;
}

std::optional<MidiAction> MidiManager::findActionForMessage (const juce::MidiMessage& message) const
{
    const juce::ScopedLock sl (mappingLock);

    for (const auto& mapping : mappings)
    {
        if (mapping.channel != 0 && mapping.channel != message.getChannel())
            continue;

        switch (mapping.type)
        {
            case MidiMappingType::ControlChange:
                if (message.isController()
                    && message.getControllerValue() >= 64
                    && message.getControllerNumber() == mapping.number)
                    return mapping.action;
                break;

            case MidiMappingType::NoteOn:
                if (message.isNoteOn() && message.getNoteNumber() == mapping.number)
                    return mapping.action;
                break;

            case MidiMappingType::ProgramChange:
                if (message.isProgramChange()
                    && message.getProgramChangeNumber() == mapping.number)
                    return mapping.action;
                break;
        }
    }

    return {};
}

void MidiManager::dispatchAction (MidiAction action)
{
    const auto index = static_cast<size_t> (static_cast<int> (action));

    if (index >= lastActionTimes.size())
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (now - lastActionTimes[index] < actionDebounceMs)
        return;

    lastActionTimes[index] = now;
    pendingAction.store (static_cast<int> (action), std::memory_order_relaxed);
    triggerAsyncUpdate();

    juce::Logger::writeToLog ("MIDI action: " + juce::String (getMidiActionName (action)));
}

void MidiManager::setMapping (const MidiMapping& mapping)
{
    const juce::ScopedLock sl (mappingLock);

    mappings.erase (std::remove_if (mappings.begin(), mappings.end(),
                                    [&mapping] (const MidiMapping& existing)
                                    {
                                        return existing.action == mapping.action
                                            || (existing.type == mapping.type
                                                && existing.channel == mapping.channel
                                                && existing.number == mapping.number);
                                    }),
                    mappings.end());

    mappings.push_back (mapping);
    mappingDirty.store (true, std::memory_order_relaxed);
}

juce::File MidiManager::getMappingFile()
{
    return AppConfig::getConfigDirectory().getChildFile ("midi-map.txt");
}

juce::File MidiManager::getProfilesFile()
{
    return AppConfig::getConfigDirectory().getChildFile ("midi-profiles.json");
}

void MidiManager::loadProfiles()
{
    profiles = MidiProfileStore::load (getProfilesFile());
    juce::Logger::writeToLog ("Loaded " + juce::String (static_cast<int> (profiles.size()))
                              + " MIDI presets");
}

bool MidiManager::saveProfiles()
{
    return MidiProfileStore::save (getProfilesFile(), profiles);
}

const MidiProfile* MidiManager::findProfile (const juce::String& name) const
{
    for (const auto& profile : profiles)
        if (profile.name.equalsIgnoreCase (name))
            return &profile;

    return nullptr;
}

void MidiManager::loadMappings()
{
    const auto file = getMappingFile();

    if (! file.existsAsFile())
        return;

    auto lines = juce::StringArray::fromLines (file.loadFileAsString());
    std::vector<MidiMapping> loaded;
    int lineNumber = 0;

    for (auto line : lines)
    {
        ++lineNumber;
        line = line.trim();

        if (line.isEmpty() || line.startsWith ("#"))
            continue;

        const auto tokens = juce::StringArray::fromTokens (line, " \t", "");

        if (tokens.size() < 4)
        {
            juce::Logger::writeToLog ("Ignoring malformed MIDI map line "
                                      + juce::String (lineNumber));
            continue;
        }

        const auto action = midiActionFromKey (tokens[0]);
        const auto type = midiMappingTypeFromString (tokens[1]);
        const auto channel = tokens[2].getIntValue();
        const auto number = tokens[3].getIntValue();

        if (! action.has_value() || ! type.has_value()
            || channel < 0 || channel > 16
            || number < 0 || number > 127)
        {
            juce::Logger::writeToLog ("Ignoring invalid MIDI map line "
                                      + juce::String (lineNumber));
            continue;
        }

        loaded.push_back ({ *action, *type, channel, number });
    }

    const auto loadedCount = static_cast<int> (loaded.size());

    {
        const juce::ScopedLock sl (mappingLock);
        mappings = std::move (loaded);
    }

    juce::Logger::writeToLog ("Loaded " + juce::String (loadedCount)
                              + " MIDI mappings from " + file.getFullPathName());
}

void MidiManager::saveMappings() const
{
    std::vector<MidiMapping> copy;

    {
        const juce::ScopedLock sl (mappingLock);
        copy = mappings;
    }

    juce::String text ("# Pan Loop MIDI map\n");
    text << "# action type channel number\n";

    for (const auto& mapping : copy)
    {
        text << getMidiActionKey (mapping.action) << " "
             << midiMappingTypeToString (mapping.type) << " "
             << mapping.channel << " "
             << mapping.number << "\n";
    }

    const auto file = getMappingFile();

    if (file.replaceWithText (text))
        juce::Logger::writeToLog ("Saved MIDI mappings to " + file.getFullPathName());
    else
        juce::Logger::writeToLog ("Failed to save MIDI mappings to " + file.getFullPathName());
}

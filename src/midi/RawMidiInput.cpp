#include "RawMidiInput.h"

#include <alsa/asoundlib.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <vector>

namespace RawMidi
{
    namespace
    {
        struct CardEntry
        {
            int index = 0;
            juce::String name;
        };

        std::vector<CardEntry> readCards()
        {
            std::vector<CardEntry> cards;
            const auto file = juce::File ("/proc/asound/cards");

            if (! file.existsAsFile())
                return cards;

            const auto lines = juce::StringArray::fromLines (file.loadFileAsString());

            for (const auto& line : lines)
            {
                const auto trimmed = line.trimStart();
                const auto cardToken = trimmed.upToFirstOccurrenceOf (" ", false, false);

                if (cardToken.isEmpty() || ! cardToken.containsOnly ("0123456789"))
                    continue;

                CardEntry card;
                card.index = cardToken.getIntValue();
                card.name = line.fromFirstOccurrenceOf (" - ", false, false).trim();

                if (card.name.isEmpty())
                    card.name = "ALSA MIDI card " + juce::String (card.index);

                cards.push_back (std::move (card));
            }

            return cards;
        }
    }

    std::vector<DeviceInfo> getAvailableDevices()
    {
        std::vector<DeviceInfo> devices;
        const auto soundDirectory = juce::File ("/dev/snd");

        if (! soundDirectory.isDirectory())
            return devices;

        for (const auto& card : readCards())
        {
            const auto midiFiles = soundDirectory.findChildFiles (
                juce::File::findFiles, false,
                "midiC" + juce::String (card.index) + "D*");

            for (const auto& midiFile : midiFiles)
            {
                const auto deviceToken = midiFile.getFileName()
                                                .fromLastOccurrenceOf ("D", false, false);
                const auto deviceIndex = deviceToken.getIntValue();

                DeviceInfo info;
                info.card = card.index;
                info.device = deviceIndex;
                info.identifier = "rawmidi:" + juce::String (card.index)
                                + ":" + juce::String (deviceIndex);
                info.name = card.name;

                if (midiFiles.size() > 1)
                    info.name << " MIDI " << (deviceIndex + 1);

                info.name << " (raw MIDI)";
                devices.push_back (std::move (info));
            }
        }

        return devices;
    }

    bool parseIdentifier (const juce::String& identifier, DeviceInfo& info)
    {
        if (! identifier.startsWith ("rawmidi:"))
            return false;

        const auto fields = juce::StringArray::fromTokens (identifier, ":", "");

        if (fields.size() != 3
            || ! fields[1].containsOnly ("0123456789")
            || ! fields[2].containsOnly ("0123456789"))
            return false;

        info = {};
        info.identifier = identifier;
        info.card = fields[1].getIntValue();
        info.device = fields[2].getIntValue();
        return true;
    }
}

RawMidiInput::RawMidiInput()
    : juce::Thread ("Pan Loop raw MIDI input")
{
    callback.handler = [this] (const juce::MidiMessage& message)
    {
        if (onMessage != nullptr)
            onMessage (message);
    };
}

RawMidiInput::~RawMidiInput()
{
    close();
}

bool RawMidiInput::open (int card, int device, int subdevice)
{
    close();

    const auto deviceName = "hw:" + juce::String (card)
                          + "," + juce::String (device)
                          + "," + juce::String (subdevice);

    snd_rawmidi_t* handle = nullptr;
    const auto result = snd_rawmidi_open (&handle, nullptr,
                                          deviceName.toRawUTF8(),
                                          SND_RAWMIDI_NONBLOCK);

    if (result < 0 || handle == nullptr)
    {
        juce::Logger::writeToLog ("Could not open raw MIDI " + deviceName + ": "
                                  + snd_strerror (result));
        return false;
    }

    inputHandle = handle;
    concatenator.reset();
    startThread();
    juce::Logger::writeToLog ("Opened raw MIDI input " + deviceName);
    return true;
}

void RawMidiInput::close()
{
    signalThreadShouldExit();

    if (isThreadRunning())
        stopThread (1500);

    if (inputHandle != nullptr)
    {
        snd_rawmidi_close (static_cast<snd_rawmidi_t*> (inputHandle));
        inputHandle = nullptr;
    }

    concatenator.reset();
}

bool RawMidiInput::isOpen() const noexcept
{
    return inputHandle != nullptr;
}

void RawMidiInput::run()
{
    auto* input = static_cast<snd_rawmidi_t*> (inputHandle);

    if (input == nullptr)
        return;

    const auto descriptorCount = snd_rawmidi_poll_descriptors_count (input);

    if (descriptorCount <= 0)
    {
        juce::Logger::writeToLog ("Raw MIDI input has no poll descriptors");
        return;
    }

    std::vector<pollfd> descriptors (static_cast<size_t> (descriptorCount));
    std::array<juce::uint8, 512> buffer {};

    while (! threadShouldExit())
    {
        if (snd_rawmidi_poll_descriptors (input, descriptors.data(),
                                          static_cast<unsigned int> (descriptorCount)) < 0)
        {
            juce::Logger::writeToLog ("Could not query raw MIDI poll descriptors");
            break;
        }

        const auto pollResult = poll (descriptors.data(),
                                      static_cast<nfds_t> (descriptorCount),
                                      100);

        if (pollResult < 0)
        {
            if (errno == EINTR)
                continue;

            juce::Logger::writeToLog ("Raw MIDI poll failed: "
                                      + juce::String (std::strerror (errno)));
            break;
        }

        if (pollResult == 0)
            continue;

        const auto bytesRead = snd_rawmidi_read (input, buffer.data(),
                                                 static_cast<size_t> (buffer.size()));

        if (bytesRead == -EAGAIN)
            continue;

        if (bytesRead == -EINTR)
            continue;

        if (bytesRead < 0)
        {
            juce::Logger::writeToLog ("Raw MIDI read failed: "
                                      + juce::String (snd_strerror (static_cast<int> (bytesRead))));
            break;
        }

        if (bytesRead > 0)
        {
            const auto time = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            concatenator.pushMidiData (buffer.data(),
                                       static_cast<int> (bytesRead),
                                       time,
                                       this,
                                       callback);
        }
    }
}

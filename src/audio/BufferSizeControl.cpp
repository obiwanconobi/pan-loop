#include "BufferSizeControl.h"

#include <algorithm>

namespace BufferSizeControl
{
    namespace
    {
        struct CommandResult
        {
            bool started = false;
            int exitCode = -1;
            juce::String output;
        };

        CommandResult runCommand (const juce::StringArray& arguments)
        {
            juce::ChildProcess process;
            CommandResult result;

            if (! process.start (arguments,
                                 juce::ChildProcess::wantStdOut
                                   | juce::ChildProcess::wantStdErr))
                return result;

            result.started = true;
            result.output = process.readAllProcessOutput();
            process.waitForProcessToFinish (1000);
            result.exitCode = static_cast<int> (process.getExitCode());
            return result;
        }

        int readMetadataInt (const juce::String& text, const juce::String& key)
        {
            const auto keyPosition = text.indexOf ("key:'" + key + "'");

            if (keyPosition < 0)
                return 0;

            const auto valuePosition = text.indexOf (keyPosition, "value:'");

            if (valuePosition < 0)
                return 0;

            const auto numberStart = valuePosition + juce::String ("value:'").length();
            const auto numberEnd = text.indexOfChar (numberStart, '\'');

            if (numberEnd <= numberStart)
                return 0;

            return text.substring (numberStart, numberEnd).getIntValue();
        }

        void addUnique (std::vector<int>& values, int value)
        {
            if (value <= 0)
                return;

            if (std::find (values.begin(), values.end(), value) == values.end())
                values.push_back (value);
        }

        std::vector<int> makeChoices (int minimum, int maximum, int current)
        {
            std::vector<int> choices;

            minimum = juce::jmax (16, minimum);
            maximum = juce::jmax (minimum, maximum);

            for (auto size = minimum; size <= maximum; size *= 2)
                addUnique (choices, size);

            addUnique (choices, current);
            std::sort (choices.begin(), choices.end());
            return choices;
        }

        bool isNativeJackServerRunning()
        {
           #if JUCE_LINUX
            const auto processes = juce::File ("/proc").findChildFiles (
                juce::File::findDirectories, false);

            for (const auto& process : processes)
            {
                const auto comm = process.getChildFile ("comm");

                if (! comm.existsAsFile())
                    continue;

                const auto name = comm.loadFileAsString().trim();

                if (name == "jackd" || name == "jackdmp")
                    return true;
            }
           #endif

            return false;
        }
    }

    PipeWireState parsePipeWireMetadata (const juce::String& text)
    {
        PipeWireState state;
        state.quantum = readMetadataInt (text, "clock.quantum");
        state.forceQuantum = readMetadataInt (text, "clock.force-quantum");
        state.minQuantum = readMetadataInt (text, "clock.min-quantum");
        state.maxQuantum = readMetadataInt (text, "clock.max-quantum");
        state.valid = state.quantum > 0 || state.forceQuantum > 0;
        return state;
    }

    Info query()
    {
        Info info;

        const auto pipeWireResult = runCommand ({ "pw-metadata", "-n", "settings" });
        const auto nativeJackRunning = isNativeJackServerRunning();

        if (pipeWireResult.started && ! nativeJackRunning)
        {
            const auto state = parsePipeWireMetadata (pipeWireResult.output);

            if (state.valid)
            {
                info.pipeWireAvailable = true;
                info.provider = "PipeWire";
                info.minimumSamples = state.minQuantum > 0 ? state.minQuantum : 32;
                info.maximumSamples = state.maxQuantum > 0 ? state.maxQuantum : 2048;
                info.currentSamples = state.forceQuantum > 0 ? state.forceQuantum
                                                             : state.quantum;
            }
        }

        if (! info.pipeWireAvailable)
        {
            // jack_bufsize is part of the JACK tools on native JACK systems.
            // Starting it without arguments only queries the current size.
            const auto jackResult = runCommand ({ "jack_bufsize" });

            if (jackResult.started && (jackResult.exitCode == 0
                                       || jackResult.output.isNotEmpty()))
            {
                info.jackBufferToolAvailable = true;
                info.provider = "JACK";
                info.minimumSamples = 16;
                info.maximumSamples = 8192;
            }
        }

        if (info.provider.isNotEmpty())
            info.choices = makeChoices (info.minimumSamples,
                                        info.maximumSamples,
                                        info.currentSamples);

        return info;
    }

    juce::String apply (int samples)
    {
        if (samples < 0)
            return "Choose a valid buffer size.";

        const auto info = query();
        juce::String lastError;

        if (info.pipeWireAvailable)
        {
            const auto result = runCommand ({ "pw-metadata", "-n", "settings",
                                              "0", "clock.force-quantum",
                                              juce::String (samples) });

            if (result.started
                && result.exitCode == 0
                && result.output.contains ("clock.force-quantum"))
            {
                juce::Logger::writeToLog ("Set PipeWire quantum to "
                                          + juce::String (samples) + " samples");
                return {};
            }

            lastError = result.started
                          ? result.output.trim()
                          : "pw-metadata was not found";
        }

        if (info.jackBufferToolAvailable)
        {
            if (samples <= 0)
                return lastError.isNotEmpty()
                         ? lastError
                         : "Choose a numeric buffer size for native JACK.";

            const auto result = runCommand ({ "jack_bufsize", juce::String (samples) });

            if (result.started && result.exitCode == 0)
            {
                juce::Logger::writeToLog ("Set JACK buffer size to "
                                          + juce::String (samples) + " samples");
                return {};
            }

            lastError = result.started
                          ? result.output.trim()
                          : "jack_bufsize was not found";
        }

        if (lastError.isNotEmpty())
            return "Could not set the buffer size: " + lastError;

        return "The JACK server owns the buffer size. Install the JACK tools "
               "(jack_bufsize) or, for PipeWire, make sure pw-metadata is available.";
    }
}

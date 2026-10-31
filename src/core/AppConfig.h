#pragma once

#include <JuceHeader.h>

namespace AppConfig
{
    /** The per-user configuration directory: ~/.config/pan-loop on Linux. */
    juce::File getConfigDirectory();

    /** The application log file. */
    juce::File getLogFile();

    /** Default directory for saved `.panloop` session files. */
    juce::File getSessionsDirectory();

    /** Starts file logging. Safe to call once at application startup. */
    void initialise();

    /** Stops file logging. Safe to call more than once. */
    void shutdown();

    juce::String getBuildInfo();
}

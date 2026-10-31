#include "AppConfig.h"

namespace AppConfig
{
    namespace
    {
        std::unique_ptr<juce::FileLogger> fileLogger;

        juce::File makeConfigDirectory()
        {
            auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("pan-loop");

            if (dir.getFullPathName().isEmpty())
                dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                          .getChildFile (".config")
                          .getChildFile ("pan-loop");

            return dir;
        }
    }

    juce::File getConfigDirectory()
    {
        return makeConfigDirectory();
    }

    juce::File getLogFile()
    {
        return getConfigDirectory().getChildFile ("pan-loop.log");
    }

    juce::File getSessionsDirectory()
    {
        return getConfigDirectory().getChildFile ("sessions");
    }

    void initialise()
    {
        auto configDir = getConfigDirectory();
        configDir.createDirectory();
        getSessionsDirectory().createDirectory();

        const juce::String version (ProjectInfo::versionString);

        fileLogger = std::make_unique<juce::FileLogger> (getLogFile(),
                                                         "Pan Loop " + version,
                                                         2 * 1024 * 1024);

        juce::Logger::setCurrentLogger (fileLogger.get());
        juce::Logger::writeToLog ("Pan Loop " + version + " starting");
        juce::Logger::writeToLog ("Config directory: " + configDir.getFullPathName());
        juce::Logger::writeToLog ("Log file: " + getLogFile().getFullPathName());
        juce::Logger::writeToLog (getBuildInfo());
    }

    void shutdown()
    {
        if (fileLogger != nullptr)
        {
            juce::Logger::writeToLog ("Pan Loop shutting down");
            juce::Logger::setCurrentLogger (nullptr);
            fileLogger.reset();
        }
    }

    juce::String getBuildInfo()
    {
       #if JUCE_DEBUG
        const juce::String buildType ("Debug");
       #else
        const juce::String buildType ("Release");
       #endif

        return buildType + " | JUCE " + juce::String (JUCE_MAJOR_VERSION) + "."
             + juce::String (JUCE_MINOR_VERSION) + "."
             + juce::String (JUCE_BUILDNUMBER);
    }
}

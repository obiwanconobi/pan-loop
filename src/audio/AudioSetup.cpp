#include "AudioSetup.h"

namespace AudioSetup
{
    namespace
    {
        void logDeviceState (juce::AudioDeviceManager& manager, const juce::String& prefix)
        {
            auto* device = manager.getCurrentAudioDevice();

            if (device == nullptr)
            {
                juce::Logger::writeToLog (prefix + ": no audio device open");
                return;
            }

            juce::Logger::writeToLog (prefix + ": " + device->getTypeName() + " / " + device->getName()
                                      + " | " + juce::String (device->getCurrentSampleRate(), 1) + " Hz | "
                                      + juce::String (device->getCurrentBufferSizeSamples()) + " samples | "
                                      + juce::String (device->getActiveInputChannels().countNumberOfSetBits()) + " in / "
                                      + juce::String (device->getActiveOutputChannels().countNumberOfSetBits()) + " out");
        }

        void fillMissingDeviceNames (juce::AudioDeviceManager& manager,
                                     juce::AudioDeviceManager::AudioDeviceSetup& setup)
        {
            auto* type = manager.getCurrentDeviceTypeObject();

            if (type == nullptr)
                return;

            if (setup.inputDeviceName.isEmpty())
            {
                const auto names = type->getDeviceNames (true);
                if (! names.isEmpty())
                    setup.inputDeviceName = names[0];
            }

            if (setup.outputDeviceName.isEmpty())
            {
                const auto names = type->getDeviceNames (false);
                if (! names.isEmpty())
                    setup.outputDeviceName = names[0];
            }
        }
    }

    juce::String initialise (juce::AudioDeviceManager& manager)
    {
        auto error = manager.initialise (2, 2, nullptr, true);

        if (error.isNotEmpty())
            juce::Logger::writeToLog ("Audio initialise error: " + error);

        preferJack (manager);

        auto setup = manager.getAudioDeviceSetup();
        fillMissingDeviceNames (manager, setup);

        if (setup.inputDeviceName.isEmpty() && setup.outputDeviceName.isEmpty())
        {
            juce::Logger::writeToLog ("No audio devices available");
            return error.isNotEmpty() ? error : "No audio devices available";
        }

        const auto setupError = applyStereoSetup (manager, setup.inputDeviceName, setup.outputDeviceName);

        if (setupError.isNotEmpty())
            juce::Logger::writeToLog ("Audio setup error: " + setupError);
        else
            logDeviceState (manager, "Audio ready");

        return setupError;
    }

    juce::String applyStereoSetup (juce::AudioDeviceManager& manager,
                                   const juce::String& inputDeviceName,
                                   const juce::String& outputDeviceName,
                                   double requestedSampleRate)
    {
        auto setup = manager.getAudioDeviceSetup();

        if (inputDeviceName.isNotEmpty())
            setup.inputDeviceName = inputDeviceName;

        if (outputDeviceName.isNotEmpty())
            setup.outputDeviceName = outputDeviceName;

        fillMissingDeviceNames (manager, setup);

        setup.useDefaultInputChannels = false;
        setup.useDefaultOutputChannels = false;
        setup.sampleRate = requestedSampleRate;
        setup.bufferSize = 0;

        auto tryOpen = [&manager, &setup] (int numInputs, int numOutputs)
        {
            setup.inputChannels.clear();
            setup.outputChannels.clear();

            for (int i = 0; i < numInputs; ++i)
                setup.inputChannels.setBit (i);

            for (int i = 0; i < numOutputs; ++i)
                setup.outputChannels.setBit (i);

            return manager.setAudioDeviceSetup (setup, true);
        };

        auto error = tryOpen (2, 2);

        if (error.isEmpty())
            return {};

        juce::Logger::writeToLog ("Stereo setup failed: " + error + " - trying mono input");
        error = tryOpen (1, 2);

        if (error.isEmpty())
            return {};

        juce::Logger::writeToLog ("Mono-input setup failed: " + error + " - trying mono output");
        error = tryOpen (1, 1);

        if (error.isEmpty())
            return {};

        juce::Logger::writeToLog ("Mono setup failed: " + error);
        return error;
    }

    void preferJack (juce::AudioDeviceManager& manager)
    {
        for (auto* type : manager.getAvailableDeviceTypes())
        {
            if (! type->getTypeName().equalsIgnoreCase ("JACK"))
                continue;

            type->scanForDevices();

            if (type->getDeviceNames (true).isEmpty() || type->getDeviceNames (false).isEmpty())
            {
                juce::Logger::writeToLog ("JACK backend found, but no JACK server/ports are visible");
                return;
            }

            manager.setCurrentAudioDeviceType ("JACK", true);
            juce::Logger::writeToLog ("Switched to JACK backend");
            return;
        }

        juce::Logger::writeToLog ("JACK backend not available in this build");
    }
}

#pragma once

#include <JuceHeader.h>

namespace AudioSetup
{
    /** Initialises the device manager, preferring JACK when a JACK server is available.
        Returns an empty string on success, otherwise a human-readable error. */
    juce::String initialise (juce::AudioDeviceManager& manager);

    /** Opens the named input/output devices with the first two channels enabled.
        Falls back to mono input and/or mono output if the device cannot provide stereo.
        A buffer size of 0 keeps the device/JACK default. */
    juce::String applyStereoSetup (juce::AudioDeviceManager& manager,
                                   const juce::String& inputDeviceName,
                                   const juce::String& outputDeviceName,
                                   double requestedSampleRate = 0.0,
                                   int requestedBufferSize = 0);

    /** Tries to switch the manager to the JACK backend. */
    void preferJack (juce::AudioDeviceManager& manager);
}

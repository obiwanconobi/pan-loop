#pragma once

#include <JuceHeader.h>
#include <atomic>

/**
    A small synthesized click generator.

    Click buffers are built in prepare() (never in the audio callback). The
    audio engine calls trigger() at the exact sample offset of a beat and then
    mixes getNextSample() into the output.
*/
class Metronome
{
public:
    Metronome() = default;

    void prepare (double sampleRate);
    void reset();

    /** Called once per audio block so the gain smoother can target the latest value. */
    void beginBlock() noexcept;

    /** Start a click. Called from the audio thread. */
    void trigger (bool accent) noexcept;

    /** Returns the next mono click sample, or 0 when no click is active. */
    float getNextSample() noexcept;

    void setEnabled (bool shouldBeEnabled) noexcept;
    bool isEnabled() const noexcept;

    /** Level in the range 0..1. */
    void setLevel (float newLevel) noexcept;
    float getLevel() const noexcept;

private:
    juce::AudioBuffer<float> accentClick;
    juce::AudioBuffer<float> normalClick;
    const juce::AudioBuffer<float>* activeClick = nullptr;
    int clickPosition = 0;

    juce::SmoothedValue<float> levelSmoother;
    std::atomic<bool> enabled { false };
    std::atomic<float> level { 0.6f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Metronome)
};

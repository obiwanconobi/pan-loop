#pragma once

#include <JuceHeader.h>
#include <atomic>

/**
    Sample-accurate transport clock.

    The audio thread advances the position; the message thread reads it and
    changes transport parameters. All control fields are atomic so the UI can
    safely call this class without locking the audio callback.
*/
class Transport
{
public:
    enum class State
    {
        Stopped = 0,
        Playing = 1,
        Paused = 2
    };

    Transport() = default;

    /** Called from the audio device start callback. Resets the clock. */
    void prepare (double newSampleRate);

    void play();
    void pause();
    void togglePlayPause();
    void stop(); // stop and rewind to position 0

    void setBpm (double newBpm);
    void setTimeSignature (int numerator, int denominator);

    /** Sets the master cycle length. Zero means free-running (no wrap).
        The position wraps immediately if it is already beyond the new length. */
    void setCycleLengthSamples (juce::int64 newLength) noexcept;
    juce::int64 getCycleLengthSamples() const noexcept;

    /** Moves the transport playhead. Used when finalizing the first loop. */
    void setPositionSamples (juce::int64 newPosition) noexcept;

    /** Called by the audio thread once per block. */
    void advance (int numSamples) noexcept;

    State getState() const noexcept;
    bool isPlaying() const noexcept;
    bool isPaused() const noexcept;
    bool isStopped() const noexcept;

    double getBpm() const noexcept;
    int getTimeSignatureNumerator() const noexcept;
    int getTimeSignatureDenominator() const noexcept;
    double getSampleRate() const noexcept;
    juce::int64 getPositionSamples() const noexcept;
    double getPositionSeconds() const noexcept;

    /** Length of one beat (the time-signature denominator) in samples. */
    double getSamplesPerBeat() const noexcept;

    /** Length of one bar in samples. */
    double getSamplesPerBar() const noexcept;

    struct PositionInfo
    {
        State state = State::Stopped;
        juce::int64 sample = 0;
        juce::int64 cycleLengthSamples = 0;
        double seconds = 0.0;
        double bpm = 120.0;
        int bar = 1;
        int beat = 1;
        int beatsPerBar = 4;
        int beatUnit = 4;
        double beatPhase = 0.0;
        double barPhase = 0.0;
    };

    PositionInfo getPositionInfo() const noexcept;

private:
    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<double> bpm { 120.0 };
    std::atomic<int> timeSignatureNumerator { 4 };
    std::atomic<int> timeSignatureDenominator { 4 };
    std::atomic<int> state { static_cast<int> (State::Stopped) };
    std::atomic<juce::int64> positionSamples { 0 };
    std::atomic<juce::int64> cycleLengthSamples { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Transport)
};

#include "Transport.h"

namespace
{
    constexpr double minBpm = 20.0;
    constexpr double maxBpm = 300.0;

    Transport::State stateFromInt (int value) noexcept
    {
        switch (value)
        {
            case 1:  return Transport::State::Playing;
            case 2:  return Transport::State::Paused;
            default: return Transport::State::Stopped;
        }
    }
}

void Transport::prepare (double newSampleRate)
{
    sampleRate.store (juce::jmax (1.0, newSampleRate), std::memory_order_relaxed);
    positionSamples.store (0, std::memory_order_relaxed);
    cycleLengthSamples.store (0, std::memory_order_relaxed);
    state.store (static_cast<int> (State::Stopped), std::memory_order_relaxed);
}

void Transport::play()
{
    state.store (static_cast<int> (State::Playing), std::memory_order_relaxed);
}

void Transport::pause()
{
    state.store (static_cast<int> (State::Paused), std::memory_order_relaxed);
}

void Transport::togglePlayPause()
{
    if (isPlaying())
        pause();
    else
        play();
}

void Transport::stop()
{
    state.store (static_cast<int> (State::Stopped), std::memory_order_relaxed);
    positionSamples.store (0, std::memory_order_relaxed);
}

void Transport::setBpm (double newBpm)
{
    bpm.store (juce::jlimit (minBpm, maxBpm, newBpm), std::memory_order_relaxed);
}

void Transport::setTimeSignature (int numerator, int denominator)
{
    timeSignatureNumerator.store (juce::jlimit (1, 32, numerator), std::memory_order_relaxed);
    timeSignatureDenominator.store (juce::jlimit (1, 32, denominator), std::memory_order_relaxed);
}

void Transport::setCycleLengthSamples (juce::int64 newLength) noexcept
{
    newLength = juce::jmax (static_cast<juce::int64> (0), newLength);
    cycleLengthSamples.store (newLength, std::memory_order_relaxed);

    if (newLength > 0)
    {
        const auto current = positionSamples.load (std::memory_order_relaxed);

        if (current >= newLength)
            positionSamples.store (current % newLength, std::memory_order_relaxed);
    }
}

juce::int64 Transport::getCycleLengthSamples() const noexcept
{
    return cycleLengthSamples.load (std::memory_order_relaxed);
}

void Transport::setPositionSamples (juce::int64 newPosition) noexcept
{
    auto position = juce::jmax (static_cast<juce::int64> (0), newPosition);
    const auto cycleLength = cycleLengthSamples.load (std::memory_order_relaxed);

    if (cycleLength > 0)
        position %= cycleLength;

    positionSamples.store (position, std::memory_order_relaxed);
}

void Transport::advance (int numSamples) noexcept
{
    if (numSamples <= 0 || ! isPlaying())
        return;

    auto newPosition = positionSamples.load (std::memory_order_relaxed)
                     + static_cast<juce::int64> (numSamples);

    const auto cycleLength = cycleLengthSamples.load (std::memory_order_relaxed);

    if (cycleLength > 0)
        newPosition %= cycleLength;

    positionSamples.store (newPosition, std::memory_order_relaxed);
}

Transport::State Transport::getState() const noexcept
{
    return stateFromInt (state.load (std::memory_order_relaxed));
}

bool Transport::isPlaying() const noexcept
{
    return getState() == State::Playing;
}

bool Transport::isPaused() const noexcept
{
    return getState() == State::Paused;
}

bool Transport::isStopped() const noexcept
{
    return getState() == State::Stopped;
}

double Transport::getBpm() const noexcept
{
    return bpm.load (std::memory_order_relaxed);
}

int Transport::getTimeSignatureNumerator() const noexcept
{
    return timeSignatureNumerator.load (std::memory_order_relaxed);
}

int Transport::getTimeSignatureDenominator() const noexcept
{
    return timeSignatureDenominator.load (std::memory_order_relaxed);
}

double Transport::getSampleRate() const noexcept
{
    return sampleRate.load (std::memory_order_relaxed);
}

juce::int64 Transport::getPositionSamples() const noexcept
{
    return positionSamples.load (std::memory_order_relaxed);
}

double Transport::getPositionSeconds() const noexcept
{
    const auto rate = getSampleRate();
    return rate > 0.0 ? static_cast<double> (getPositionSamples()) / rate : 0.0;
}

double Transport::getSamplesPerBeat() const noexcept
{
    const auto rate = getSampleRate();
    const auto currentBpm = getBpm();
    const auto denominator = getTimeSignatureDenominator();

    if (rate <= 0.0 || currentBpm <= 0.0 || denominator <= 0)
        return 0.0;

    // BPM is quarter-note based. One beat is one time-signature denominator note.
    const auto samplesPerQuarterNote = rate * 60.0 / currentBpm;
    return samplesPerQuarterNote * (4.0 / static_cast<double> (denominator));
}

double Transport::getSamplesPerBar() const noexcept
{
    return getSamplesPerBeat() * static_cast<double> (getTimeSignatureNumerator());
}

Transport::PositionInfo Transport::getPositionInfo() const noexcept
{
    PositionInfo info;
    info.state = getState();
    info.sample = getPositionSamples();
    info.cycleLengthSamples = getCycleLengthSamples();
    info.seconds = getPositionSeconds();
    info.bpm = getBpm();
    info.beatsPerBar = getTimeSignatureNumerator();
    info.beatUnit = getTimeSignatureDenominator();

    const auto samplesPerBeat = getSamplesPerBeat();

    if (samplesPerBeat <= 0.0 || info.beatsPerBar <= 0)
        return info;

    const auto totalBeats = static_cast<double> (info.sample) / samplesPerBeat;
    const auto barIndex = static_cast<juce::int64> (std::floor (totalBeats
                                                                / static_cast<double> (info.beatsPerBar)));
    const auto beatInBar = static_cast<int> (std::floor (totalBeats))
                         - static_cast<int> (barIndex) * info.beatsPerBar;

    info.bar = static_cast<int> (barIndex) + 1;
    info.beat = beatInBar + 1;
    info.beatPhase = totalBeats - std::floor (totalBeats);
    info.barPhase = (totalBeats / static_cast<double> (info.beatsPerBar))
                  - std::floor (totalBeats / static_cast<double> (info.beatsPerBar));

    return info;
}

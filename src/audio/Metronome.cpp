#include "Metronome.h"

namespace
{
    constexpr auto twoPi = juce::MathConstants<double>::twoPi;

    juce::AudioBuffer<float> makeClick (double sampleRate,
                                        double frequency,
                                        double durationSeconds,
                                        float amplitude)
    {
        const auto length = juce::jmax (1, static_cast<int> (std::round (sampleRate * durationSeconds)));
        juce::AudioBuffer<float> buffer (1, length);
        auto* samples = buffer.getWritePointer (0);

        for (int i = 0; i < length; ++i)
        {
            const auto t = static_cast<double> (i) / sampleRate;
            const auto attack = static_cast<float> (juce::jmin (1.0, t / 0.001));
            const auto decay = static_cast<float> (std::exp (-t * 90.0));
            samples[i] = std::sin (twoPi * frequency * t) * attack * decay * amplitude;
        }

        return buffer;
    }
}

void Metronome::prepare (double sampleRate)
{
    accentClick = makeClick (sampleRate, 1200.0, 0.05, 0.7f);
    normalClick = makeClick (sampleRate, 800.0, 0.035, 0.5f);

    activeClick = nullptr;
    clickPosition = 0;

    levelSmoother.reset (sampleRate, 0.01);
    levelSmoother.setCurrentAndTargetValue (level.load (std::memory_order_relaxed));
}

void Metronome::reset()
{
    activeClick = nullptr;
    clickPosition = 0;
    levelSmoother.setCurrentAndTargetValue (level.load (std::memory_order_relaxed));
}

void Metronome::beginBlock() noexcept
{
    levelSmoother.setTargetValue (level.load (std::memory_order_relaxed));
}

void Metronome::trigger (bool accent) noexcept
{
    if (! enabled.load (std::memory_order_relaxed))
        return;

    activeClick = accent ? &accentClick : &normalClick;
    clickPosition = 0;
}

float Metronome::getNextSample() noexcept
{
    const auto smoothedLevel = levelSmoother.getNextValue();

    if (activeClick == nullptr || clickPosition >= activeClick->getNumSamples())
    {
        activeClick = nullptr;
        return 0.0f;
    }

    return activeClick->getSample (0, clickPosition++) * smoothedLevel;
}

void Metronome::setEnabled (bool shouldBeEnabled) noexcept
{
    enabled.store (shouldBeEnabled, std::memory_order_relaxed);
}

bool Metronome::isEnabled() const noexcept
{
    return enabled.load (std::memory_order_relaxed);
}

void Metronome::setLevel (float newLevel) noexcept
{
    level.store (juce::jlimit (0.0f, 1.0f, newLevel), std::memory_order_relaxed);
}

float Metronome::getLevel() const noexcept
{
    return level.load (std::memory_order_relaxed);
}

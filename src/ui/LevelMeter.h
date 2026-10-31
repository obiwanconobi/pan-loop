#pragma once

#include <JuceHeader.h>

/** A simple horizontal peak meter with a peak-hold marker. */
class LevelMeter final : public juce::Component
{
public:
    LevelMeter();

    void setLevel (float newLevel) noexcept;
    void setOrientation (bool shouldBeVertical) noexcept;

    void paint (juce::Graphics&) override;

private:
    float level = 0.0f;
    float peak = 0.0f;
    bool vertical = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

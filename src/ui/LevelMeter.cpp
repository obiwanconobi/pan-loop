#include "LevelMeter.h"
#include "app/DarkLookAndFeel.h"

LevelMeter::LevelMeter()
{
    setInterceptsMouseClicks (false, false);
}

void LevelMeter::setLevel (float newLevel) noexcept
{
    const auto clamped = juce::jlimit (0.0f, 1.0f, newLevel);

    if (clamped >= level)
        level = clamped;
    else
        level = juce::jmax (0.0f, level - 0.06f);

    peak = juce::jmax (peak * 0.995f, level);
}

void LevelMeter::setOrientation (bool shouldBeVertical) noexcept
{
    vertical = shouldBeVertical;
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff0f1115));
    g.fillRoundedRectangle (bounds, 3.0f);

    auto inner = bounds.reduced (1.5f);
    const auto fillAmount = juce::jlimit (0.0f, 1.0f, level);

    auto filled = vertical
                    ? inner.removeFromBottom (inner.getHeight() * fillAmount)
                    : inner.removeFromLeft (inner.getWidth() * fillAmount);

    if (! filled.isEmpty())
    {
        juce::ColourGradient gradient (DarkLookAndFeel::playing(), filled.getBottomLeft(),
                                       DarkLookAndFeel::recording(), filled.getTopRight(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (filled, 2.0f);
    }

    if (peak > 0.001f)
    {
        g.setColour (DarkLookAndFeel::text().withAlpha (0.8f));

        if (vertical)
        {
            const auto y = inner.getBottom() - inner.getHeight() * juce::jlimit (0.0f, 1.0f, peak);
            g.fillRect (inner.getX(), y, inner.getWidth(), 1.5f);
        }
        else
        {
            const auto x = inner.getX() + inner.getWidth() * juce::jlimit (0.0f, 1.0f, peak);
            g.fillRect (x, inner.getY(), 1.5f, inner.getHeight());
        }
    }

    g.setColour (DarkLookAndFeel::outline());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);
}

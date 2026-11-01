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

    g.setColour (juce::Colour (0xff070a0e));
    g.fillRoundedRectangle (bounds, 4.0f);

    auto inner = bounds.reduced (2.0f);
    const auto fillAmount = juce::jlimit (0.0f, 1.0f, level);
    const auto peakAmount = juce::jlimit (0.0f, 1.0f, peak);

    const auto longSide = vertical ? inner.getHeight() : inner.getWidth();
    const auto segments = vertical
                            ? juce::jlimit (4, 28, static_cast<int> (longSide / 4.0f))
                            : juce::jlimit (4, 40, static_cast<int> (longSide / 5.0f));
    const auto gap = 1.5f;

    const auto colourForPosition = [] (float position)
    {
        if (position > 0.87f)
            return DarkLookAndFeel::recording();

        if (position > 0.70f)
            return DarkLookAndFeel::armed();

        return DarkLookAndFeel::playing();
    };

    for (int i = 0; i < segments; ++i)
    {
        const auto position = static_cast<float> (i + 1) / static_cast<float> (segments);
        const auto lit = fillAmount >= position;

        juce::Rectangle<float> segment;

        if (vertical)
        {
            const auto segmentHeight = (inner.getHeight() - gap * static_cast<float> (segments - 1))
                                     / static_cast<float> (segments);
            segment = { inner.getX(),
                        inner.getBottom() - (static_cast<float> (i) + 1.0f) * (segmentHeight + gap) + gap,
                        inner.getWidth(), segmentHeight };
        }
        else
        {
            const auto segmentWidth = (inner.getWidth() - gap * static_cast<float> (segments - 1))
                                    / static_cast<float> (segments);
            segment = { inner.getX() + static_cast<float> (i) * (segmentWidth + gap),
                        inner.getY(), segmentWidth, inner.getHeight() };
        }

        if (lit)
        {
            g.setColour (colourForPosition (position));
            g.fillRoundedRectangle (segment, 1.2f);
        }
        else
        {
            g.setColour (DarkLookAndFeel::outlineSoft().withAlpha (0.55f));
            g.fillRoundedRectangle (segment, 1.2f);
        }
    }

    if (peakAmount > 0.001f)
    {
        g.setColour (DarkLookAndFeel::text().withAlpha (0.75f));

        if (vertical)
        {
            const auto y = inner.getBottom() - inner.getHeight() * peakAmount;
            g.fillRect (inner.getX(), y, inner.getWidth(), 1.5f);
        }
        else
        {
            const auto x = inner.getX() + inner.getWidth() * peakAmount;
            g.fillRect (x, inner.getY(), 1.5f, inner.getHeight());
        }
    }

    g.setColour (DarkLookAndFeel::outline());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
}

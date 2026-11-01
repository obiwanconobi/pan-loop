#include "DarkLookAndFeel.h"

namespace
{
    juce::String buttonVariant (const juce::Button& button)
    {
        return button.getProperties()
                     .getWithDefault ("plVariant", "default")
                     .toString()
                     .toLowerCase();
    }

    juce::String buttonIconName (const juce::Button& button)
    {
        return button.getProperties().getWithDefault ("plIcon", {}).toString();
    }

    bool isSolidVariant (const juce::String& variant)
    {
        return variant == "primary" || variant == "danger" || variant == "warning"
            || variant == "success";
    }

    juce::Colour toneForVariant (const juce::String& variant)
    {
        if (variant == "primary" || variant == "accent") return DarkLookAndFeel::accent();
        if (variant == "danger" || variant == "dangerSoft") return DarkLookAndFeel::recording();
        if (variant == "warning" || variant == "warningSoft") return DarkLookAndFeel::armed();
        if (variant == "success") return DarkLookAndFeel::playing();

        return DarkLookAndFeel::text();
    }

    juce::Colour inkForVariant (const juce::String& variant)
    {
        if (variant == "primary") return DarkLookAndFeel::accentInk();
        if (variant == "danger") return DarkLookAndFeel::recordingInk();
        if (variant == "warning") return DarkLookAndFeel::armedInk();
        if (variant == "success") return DarkLookAndFeel::playingInk();

        return DarkLookAndFeel::text();
    }

    juce::Colour textForVariant (const juce::String& variant, bool active)
    {
        if (isSolidVariant (variant))
            return inkForVariant (variant);

        if (active || variant == "accent")
            return DarkLookAndFeel::accent();

        if (variant == "dangerSoft")
            return DarkLookAndFeel::recording();

        if (variant == "warningSoft")
            return DarkLookAndFeel::armed();

        if (variant == "ghost")
            return DarkLookAndFeel::mutedText();

        return DarkLookAndFeel::text();
    }
}

DarkLookAndFeel::DarkLookAndFeel()
{
    setColourScheme ({ 0xff0a0c10, 0xff11151d, 0xff171c26, 0xff2a3341,
                       0xfff2f5fa, 0xff07090d, 0xff5bd4ff, 0xff28a9d8,
                       0xff04171f });

    setColour (juce::ResizableWindow::backgroundColourId, background());
    setColour (juce::DocumentWindow::textColourId, text());

    setColour (juce::Label::textColourId, text());
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::TextButton::buttonColourId, panelHigh());
    setColour (juce::TextButton::buttonOnColourId, accent());
    setColour (juce::TextButton::textColourOffId, text());
    setColour (juce::TextButton::textColourOnId, accentInk());

    setColour (juce::ComboBox::backgroundColourId, panelAlt());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::ComboBox::outlineColourId, outline());
    setColour (juce::ComboBox::arrowColourId, mutedText());
    setColour (juce::ComboBox::focusedOutlineColourId, accent());

    setColour (juce::PopupMenu::backgroundColourId, panelAlt());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::headerTextColourId, mutedText());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent().withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId, text());

    setColour (juce::Slider::thumbColourId, accent());
    setColour (juce::Slider::trackColourId, accentDeep());
    setColour (juce::Slider::backgroundColourId, panelHigh());
    setColour (juce::Slider::rotarySliderFillColourId, accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, panelHigh());
    setColour (juce::Slider::textBoxTextColourId, text());
    setColour (juce::Slider::textBoxBackgroundColourId, panelAlt());
    setColour (juce::Slider::textBoxHighlightColourId, accent().withAlpha (0.25f));
    setColour (juce::Slider::textBoxOutlineColourId, outline());

    setColour (juce::ToggleButton::textColourId, text());
    setColour (juce::ToggleButton::tickColourId, accent());
    setColour (juce::ToggleButton::tickDisabledColourId, outline());

    setColour (juce::TabbedComponent::backgroundColourId, background());
    setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::frontOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TabbedButtonBar::tabTextColourId, mutedText());
    setColour (juce::TabbedButtonBar::frontTextColourId, text());

    setColour (juce::TextEditor::backgroundColourId, background().darker (0.2f));
    setColour (juce::TextEditor::textColourId, text());
    setColour (juce::TextEditor::outlineColourId, outline());
    setColour (juce::TextEditor::focusedOutlineColourId, accent());
    setColour (juce::TextEditor::highlightColourId, accent().withAlpha (0.22f));
    setColour (juce::TextEditor::highlightedTextColourId, text());
    setColour (juce::CaretComponent::caretColourId, accent());

    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ListBox::textColourId, text());
    setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);

    setColour (juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, mutedText().withAlpha (0.35f));
    setColour (juce::ScrollBar::trackColourId, panelAlt());

    setColour (juce::TooltipWindow::backgroundColourId, panelHigh());
    setColour (juce::TooltipWindow::textColourId, text());
    setColour (juce::TooltipWindow::outlineColourId, outline());
}

juce::Font DarkLookAndFeel::sans (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height,
                                         bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font DarkLookAndFeel::mono (float height, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                         height,
                                         bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font DarkLookAndFeel::kicker (float height)
{
    return sans (height, true).withExtraKerningFactor (0.09f);
}

void DarkLookAndFeel::styleLabel (juce::Label& label, float height, bool bold)
{
    styleLabel (label, height, bold, text());
}

void DarkLookAndFeel::styleLabel (juce::Label& label, float height, bool bold,
                                  juce::Colour colour, juce::Justification justification)
{
    label.setFont (sans (height, bold));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
}

void DarkLookAndFeel::drawCard (juce::Graphics& g, juce::Rectangle<int> bounds, bool accentGlow)
{
    drawCard (g, bounds.toFloat(), accentGlow);
}

void DarkLookAndFeel::drawCard (juce::Graphics& g, juce::Rectangle<float> bounds, bool accentGlow)
{
    if (bounds.isEmpty())
        return;

    const auto radius = 14.0f;

    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f).reduced (1.0f), radius + 1.0f);

    juce::ColourGradient gradient (panelHigh(), bounds.getTopLeft(),
                                   panel(), bounds.getBottomLeft(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds, radius);

    g.setColour (outlineSoft());
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.drawLine (bounds.getX() + 16.0f, bounds.getY() + 1.0f,
                bounds.getRight() - 16.0f, bounds.getY() + 1.0f, 1.0f);

    if (accentGlow)
    {
        g.setColour (accent().withAlpha (0.65f));
        g.fillRoundedRectangle (bounds.withWidth (3.0f).reduced (0.0f, 16.0f), 1.5f);
    }
}

void DarkLookAndFeel::drawPill (juce::Graphics& g, juce::Rectangle<int> bounds,
                                juce::Colour fill, juce::Colour border, float radius)
{
    if (bounds.isEmpty())
        return;

    const auto r = bounds.toFloat();
    const auto actualRadius = radius < 0.0f ? r.getHeight() * 0.5f : radius;

    g.setColour (fill);
    g.fillRoundedRectangle (r, actualRadius);

    if (! border.isTransparent())
    {
        g.setColour (border);
        g.drawRoundedRectangle (r.reduced (0.5f), actualRadius, 1.0f);
    }
}

void DarkLookAndFeel::drawIcon (juce::Graphics& g, const juce::String& name,
                                juce::Rectangle<float> bounds,
                                juce::Colour colour, float strokeWidth)
{
    if (bounds.isEmpty())
        return;

    const auto r = bounds.reduced (1.0f);
    const auto x = r.getX();
    const auto y = r.getY();
    const auto w = r.getWidth();
    const auto h = r.getHeight();
    const auto stroke = strokeWidth > 0.0f ? strokeWidth
                                           : juce::jmax (1.4f, r.getWidth() * 0.095f);

    juce::Path path;
    g.setColour (colour);

    const auto stroked = [&] (const juce::Path& p, float thickness)
    {
        g.strokePath (p, juce::PathStrokeType (thickness,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    };

    if (name == "play")
    {
        path.addTriangle (x + w * 0.30f, y + h * 0.16f,
                          x + w * 0.30f, y + h * 0.84f,
                          x + w * 0.82f, y + h * 0.50f);
        g.fillPath (path);
    }
    else if (name == "pause")
    {
        g.fillRoundedRectangle (x + w * 0.22f, y + h * 0.18f, w * 0.20f, h * 0.64f, 1.5f);
        g.fillRoundedRectangle (x + w * 0.58f, y + h * 0.18f, w * 0.20f, h * 0.64f, 1.5f);
    }
    else if (name == "stop")
    {
        g.fillRoundedRectangle (x + w * 0.24f, y + h * 0.24f, w * 0.52f, h * 0.52f, 2.0f);
    }
    else if (name == "record")
    {
        g.fillEllipse (x + w * 0.26f, y + h * 0.26f, w * 0.48f, h * 0.48f);
        g.setColour (colour.withAlpha (0.45f));
        g.drawEllipse (x + w * 0.17f, y + h * 0.17f, w * 0.66f, h * 0.66f, stroke * 0.75f);
    }
    else if (name == "live")
    {
        g.drawEllipse (x + w * 0.08f, y + h * 0.08f, w * 0.84f, h * 0.84f, stroke);
        path.addTriangle (x + w * 0.40f, y + h * 0.30f,
                          x + w * 0.40f, y + h * 0.70f,
                          x + w * 0.70f, y + h * 0.50f);
        g.fillPath (path);
    }
    else if (name == "playlist")
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto yy = y + h * (0.25f + 0.25f * static_cast<float> (i));
            g.fillEllipse (x + w * 0.10f, yy - w * 0.045f, w * 0.09f, w * 0.09f);
            path.addLineSegment ({ x + w * 0.30f, yy, x + w * 0.90f, yy }, 0.0f);
        }
        stroked (path, stroke);
    }
    else if (name == "layers")
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto offset = static_cast<float> (i) * h * 0.12f;
            juce::Path layer;
            layer.addRoundedRectangle (x + w * 0.18f + offset,
                                       y + h * 0.20f + offset * 1.4f,
                                       w * 0.64f - offset * 0.6f,
                                       h * 0.18f, 2.0f);
            stroked (layer, stroke * 0.9f);
        }
    }
    else if (name == "audio")
    {
        path.startNewSubPath (x + w * 0.12f, y + h * 0.38f);
        path.lineTo (x + w * 0.32f, y + h * 0.38f);
        path.lineTo (x + w * 0.52f, y + h * 0.20f);
        path.lineTo (x + w * 0.52f, y + h * 0.80f);
        path.lineTo (x + w * 0.32f, y + h * 0.62f);
        path.lineTo (x + w * 0.12f, y + h * 0.62f);
        path.closeSubPath();
        g.fillPath (path);

        juce::Path waves;
        waves.addCentredArc (x + w * 0.60f, y + h * 0.50f, w * 0.14f, h * 0.18f,
                             0.0f, -1.0f, 1.0f, true);
        waves.addCentredArc (x + w * 0.60f, y + h * 0.50f, w * 0.28f, h * 0.34f,
                             0.0f, -1.0f, 1.0f, true);
        stroked (waves, stroke * 0.85f);
    }
    else if (name == "midi")
    {
        g.drawEllipse (x + w * 0.08f, y + h * 0.08f, w * 0.84f, h * 0.84f, stroke);
        const auto dot = w * 0.10f;
        g.fillEllipse (x + w * 0.24f, y + h * 0.34f, dot, dot);
        g.fillEllipse (x + w * 0.45f, y + h * 0.25f, dot, dot);
        g.fillEllipse (x + w * 0.66f, y + h * 0.34f, dot, dot);
        g.fillEllipse (x + w * 0.33f, y + h * 0.60f, dot, dot);
        g.fillEllipse (x + w * 0.57f, y + h * 0.60f, dot, dot);
    }
    else if (name == "session" || name == "save")
    {
        path.addRoundedRectangle (x + w * 0.14f, y + h * 0.12f, w * 0.72f, h * 0.76f, 2.0f);
        stroked (path, stroke);
        g.fillRoundedRectangle (x + w * 0.30f, y + h * 0.12f, w * 0.34f, h * 0.24f, 1.0f);
        g.drawRoundedRectangle (x + w * 0.28f, y + h * 0.54f, w * 0.44f, h * 0.34f,
                                1.5f, stroke * 0.8f);
    }
    else if (name == "trash")
    {
        path.startNewSubPath (x + w * 0.18f, y + h * 0.28f);
        path.lineTo (x + w * 0.82f, y + h * 0.28f);
        stroked (path, stroke);
        path.clear();
        path.addRoundedRectangle (x + w * 0.24f, y + h * 0.32f, w * 0.52f, h * 0.56f, 2.0f);
        stroked (path, stroke * 0.9f);
        path.clear();
        path.startNewSubPath (x + w * 0.36f, y + h * 0.18f);
        path.lineTo (x + w * 0.64f, y + h * 0.18f);
        stroked (path, stroke);
        const auto inset = w * 0.09f;
        g.fillRect (x + w * 0.43f, y + h * 0.42f, inset * 0.75f, h * 0.30f);
        g.fillRect (x + w * 0.57f, y + h * 0.42f, inset * 0.75f, h * 0.30f);
    }
    else if (name == "plus" || name == "new")
    {
        path.startNewSubPath (x + w * 0.50f, y + h * 0.18f);
        path.lineTo (x + w * 0.50f, y + h * 0.82f);
        path.startNewSubPath (x + w * 0.18f, y + h * 0.50f);
        path.lineTo (x + w * 0.82f, y + h * 0.50f);
        stroked (path, stroke);
    }
    else if (name == "minus")
    {
        path.startNewSubPath (x + w * 0.18f, y + h * 0.50f);
        path.lineTo (x + w * 0.82f, y + h * 0.50f);
        stroked (path, stroke);
    }
    else if (name == "up" || name == "down")
    {
        const auto down = name == "down";
        path.startNewSubPath (x + w * 0.20f, y + h * (down ? 0.42f : 0.58f));
        path.lineTo (x + w * 0.50f, y + h * (down ? 0.70f : 0.30f));
        path.lineTo (x + w * 0.80f, y + h * (down ? 0.42f : 0.58f));
        stroked (path, stroke);
    }
    else if (name == "load")
    {
        path.startNewSubPath (x + w * 0.12f, y + h * 0.40f);
        path.lineTo (x + w * 0.12f, y + h * 0.78f);
        path.lineTo (x + w * 0.88f, y + h * 0.78f);
        path.lineTo (x + w * 0.88f, y + h * 0.40f);
        stroked (path, stroke);
        path.clear();
        path.startNewSubPath (x + w * 0.50f, y + h * 0.18f);
        path.lineTo (x + w * 0.50f, y + h * 0.60f);
        path.startNewSubPath (x + w * 0.32f, y + h * 0.45f);
        path.lineTo (x + w * 0.50f, y + h * 0.62f);
        path.lineTo (x + w * 0.68f, y + h * 0.45f);
        stroked (path, stroke);
    }
    else if (name == "export")
    {
        path.addRoundedRectangle (x + w * 0.14f, y + h * 0.52f, w * 0.72f, h * 0.34f, 2.0f);
        stroked (path, stroke * 0.9f);
        path.clear();
        path.startNewSubPath (x + w * 0.50f, y + h * 0.70f);
        path.lineTo (x + w * 0.50f, y + h * 0.18f);
        path.startNewSubPath (x + w * 0.32f, y + h * 0.34f);
        path.lineTo (x + w * 0.50f, y + h * 0.17f);
        path.lineTo (x + w * 0.68f, y + h * 0.34f);
        stroked (path, stroke);
    }
    else if (name == "folder")
    {
        path.startNewSubPath (x + w * 0.12f, y + h * 0.30f);
        path.lineTo (x + w * 0.42f, y + h * 0.30f);
        path.lineTo (x + w * 0.50f, y + h * 0.40f);
        path.lineTo (x + w * 0.88f, y + h * 0.40f);
        path.lineTo (x + w * 0.88f, y + h * 0.78f);
        path.lineTo (x + w * 0.12f, y + h * 0.78f);
        path.closeSubPath();
        stroked (path, stroke);
    }
    else if (name == "file")
    {
        path.startNewSubPath (x + w * 0.26f, y + h * 0.14f);
        path.lineTo (x + w * 0.60f, y + h * 0.14f);
        path.lineTo (x + w * 0.76f, y + h * 0.30f);
        path.lineTo (x + w * 0.76f, y + h * 0.86f);
        path.lineTo (x + w * 0.26f, y + h * 0.86f);
        path.closeSubPath();
        stroked (path, stroke);
        path.clear();
        path.startNewSubPath (x + w * 0.60f, y + h * 0.14f);
        path.lineTo (x + w * 0.60f, y + h * 0.30f);
        path.lineTo (x + w * 0.76f, y + h * 0.30f);
        stroked (path, stroke * 0.8f);
    }
    else if (name == "copy")
    {
        path.addRoundedRectangle (x + w * 0.28f, y + h * 0.18f, w * 0.52f, h * 0.52f, 2.0f);
        stroked (path, stroke * 0.9f);
        path.clear();
        path.addRoundedRectangle (x + w * 0.16f, y + h * 0.32f, w * 0.52f, h * 0.52f, 2.0f);
        stroked (path, stroke * 0.9f);
    }
    else if (name == "expand")
    {
        const auto a = w * 0.18f;
        const auto b = w * 0.34f;
        path.startNewSubPath (x + a, y + b);
        path.lineTo (x + a, y + a);
        path.lineTo (x + b, y + a);
        path.startNewSubPath (x + w - b, y + a);
        path.lineTo (x + w - a, y + a);
        path.lineTo (x + w - a, y + b);
        path.startNewSubPath (x + w - a, y + h - b);
        path.lineTo (x + w - a, y + h - a);
        path.lineTo (x + w - b, y + h - a);
        path.startNewSubPath (x + b, y + h - a);
        path.lineTo (x + a, y + h - a);
        path.lineTo (x + a, y + h - b);
        stroked (path, stroke);
    }
    else if (name == "metronome")
    {
        path.startNewSubPath (x + w * 0.30f, y + h * 0.84f);
        path.lineTo (x + w * 0.42f, y + h * 0.18f);
        path.lineTo (x + w * 0.58f, y + h * 0.18f);
        path.lineTo (x + w * 0.70f, y + h * 0.84f);
        path.closeSubPath();
        stroked (path, stroke);
        path.clear();
        path.startNewSubPath (x + w * 0.50f, y + h * 0.64f);
        path.lineTo (x + w * 0.62f, y + h * 0.30f);
        stroked (path, stroke * 0.9f);
    }
    else if (name == "tap")
    {
        g.fillEllipse (x + w * 0.42f, y + h * 0.42f, w * 0.16f, h * 0.16f);
        juce::Path arcs;
        arcs.addCentredArc (x + w * 0.50f, y + h * 0.50f, w * 0.24f, h * 0.24f,
                            0.0f, -1.1f, 1.1f, true);
        arcs.addCentredArc (x + w * 0.50f, y + h * 0.50f, w * 0.38f, h * 0.38f,
                            0.0f, -0.9f, 0.9f, true);
        stroked (arcs, stroke * 0.8f);
    }
    else if (name == "listen")
    {
        g.fillEllipse (x + w * 0.42f, y + h * 0.42f, w * 0.16f, h * 0.16f);
        juce::Path arcs;
        arcs.addCentredArc (x + w * 0.50f, y + h * 0.50f, w * 0.26f, h * 0.26f,
                            0.0f, 2.2f, 4.1f, true);
        arcs.addCentredArc (x + w * 0.50f, y + h * 0.50f, w * 0.40f, h * 0.40f,
                            0.0f, 2.35f, 3.95f, true);
        stroked (arcs, stroke * 0.8f);
    }
    else if (name == "x" || name == "clear")
    {
        path.startNewSubPath (x + w * 0.24f, y + h * 0.24f);
        path.lineTo (x + w * 0.76f, y + h * 0.76f);
        path.startNewSubPath (x + w * 0.76f, y + h * 0.24f);
        path.lineTo (x + w * 0.24f, y + h * 0.76f);
        stroked (path, stroke);
    }
    else if (name == "check")
    {
        path.startNewSubPath (x + w * 0.18f, y + h * 0.52f);
        path.lineTo (x + w * 0.42f, y + h * 0.74f);
        path.lineTo (x + w * 0.82f, y + h * 0.28f);
        stroked (path, stroke);
    }
    else if (name == "volume")
    {
        path.startNewSubPath (x + w * 0.12f, y + h * 0.38f);
        path.lineTo (x + w * 0.34f, y + h * 0.38f);
        path.lineTo (x + w * 0.54f, y + h * 0.20f);
        path.lineTo (x + w * 0.54f, y + h * 0.80f);
        path.lineTo (x + w * 0.34f, y + h * 0.62f);
        path.lineTo (x + w * 0.12f, y + h * 0.62f);
        path.closeSubPath();
        g.fillPath (path);
        path.clear();
        path.startNewSubPath (x + w * 0.66f, y + h * 0.28f);
        path.lineTo (x + w * 0.84f, y + h * 0.72f);
        stroked (path, stroke * 0.9f);
    }
    else if (name == "music")
    {
        g.fillEllipse (x + w * 0.14f, y + h * 0.66f, w * 0.22f, h * 0.17f);
        g.fillEllipse (x + w * 0.55f, y + h * 0.58f, w * 0.22f, h * 0.17f);
        g.fillRect (x + w * 0.31f, y + h * 0.22f, w * 0.045f, h * 0.50f);
        g.fillRect (x + w * 0.72f, y + h * 0.14f, w * 0.045f, h * 0.50f);
        path.startNewSubPath (x + w * 0.31f, y + h * 0.23f);
        path.lineTo (x + w * 0.77f, y + h * 0.14f);
        path.lineTo (x + w * 0.77f, y + h * 0.30f);
        path.lineTo (x + w * 0.31f, y + h * 0.39f);
        path.closeSubPath();
        g.fillPath (path);
    }
    else if (name == "loop")
    {
        path.addCentredArc (x + w * 0.50f, y + h * 0.50f,
                            w * 0.30f, h * 0.30f, 0.0f, 0.55f, 4.95f, true);
        stroked (path, stroke);
        path.clear();
        path.startNewSubPath (x + w * 0.70f, y + h * 0.14f);
        path.lineTo (x + w * 0.86f, y + h * 0.30f);
        path.lineTo (x + w * 0.66f, y + h * 0.38f);
        path.closeSubPath();
        g.fillPath (path);
    }
}

void DarkLookAndFeel::setButtonVariant (juce::Button& button, const juce::String& variant)
{
    button.getProperties().set ("plVariant", variant);
}

void DarkLookAndFeel::setButtonIcon (juce::Button& button, const juce::String& icon)
{
    button.getProperties().set ("plIcon", icon);
}

void DarkLookAndFeel::setButtonTextSize (juce::Button& button, float size)
{
    button.getProperties().set ("plTextSize", size);
}

void DarkLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    const auto variant = buttonVariant (button);
    const auto h = static_cast<float> (button.getHeight());
    const auto radius = juce::jlimit (8.0f, 16.0f, h * 0.17f);
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    if (variant == "nav")
    {
        const auto active = button.getToggleState();
        auto fill = juce::Colours::transparentBlack;
        auto border = juce::Colours::transparentBlack;

        if (active)
        {
            fill = accent().withAlpha (down ? 0.20f : 0.14f);
            border = accent().withAlpha (0.38f);
        }
        else if (down)
        {
            fill = juce::Colours::white.withAlpha (0.075f);
        }
        else if (highlighted)
        {
            fill = juce::Colours::white.withAlpha (0.045f);
        }

        drawPill (g, button.getLocalBounds(), fill, border, 9.0f);

        if (active)
        {
            g.setColour (accent());
            const auto indicator = bounds.withSizeKeepingCentre (bounds.getWidth() * 0.30f, 2.0f)
                                         .withY (bounds.getBottom() - 5.0f);
            g.fillRoundedRectangle (indicator, 1.0f);
        }

        if (button.hasKeyboardFocus (true))
        {
            g.setColour (accent().withAlpha (0.9f));
            g.drawRoundedRectangle (bounds.reduced (1.5f), 8.0f, 1.6f);
        }

        return;
    }

    if (! button.isEnabled())
    {
        g.setColour (panelAlt().withAlpha (0.55f));
        g.fillRoundedRectangle (bounds, radius);
        g.setColour (outlineSoft());
        g.drawRoundedRectangle (bounds, radius, 1.0f);
        return;
    }

    const auto tone = toneForVariant (variant);

    if (isSolidVariant (variant))
    {
        auto fill = tone;

        if (down)       fill = fill.darker (0.10f);
        else if (highlighted) fill = fill.brighter (0.07f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, radius);

        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawLine (bounds.getX() + radius, bounds.getY() + 1.0f,
                    bounds.getRight() - radius, bounds.getY() + 1.0f, 1.0f);

        g.setColour (tone.brighter (0.25f).withAlpha (0.65f));
        g.drawRoundedRectangle (bounds, radius, 1.0f);

        if (button.hasKeyboardFocus (true))
        {
            g.setColour (inkForVariant (variant).withAlpha (0.75f));
            g.drawRoundedRectangle (bounds.reduced (2.0f), radius - 1.0f, 2.0f);
        }

        return;
    }

    juce::Colour fill = panelHigh();
    juce::Colour border = outline();
    juce::Colour textTint = text();

    if (variant == "accent" || variant == "dangerSoft" || variant == "warningSoft")
    {
        fill = tone.withAlpha (down ? 0.26f : highlighted ? 0.20f : 0.13f);
        border = tone.withAlpha (0.42f);
        textTint = tone;
    }
    else if (variant == "ghost")
    {
        fill = down ? panelHigh().withAlpha (0.8f)
                    : highlighted ? panelAlt().withAlpha (0.8f)
                                  : juce::Colours::transparentBlack;
        border = highlighted ? outline() : outlineSoft();
        textTint = mutedText();
    }
    else if (button.getToggleState())
    {
        fill = accent().withAlpha (down ? 0.28f : highlighted ? 0.22f : 0.15f);
        border = accent().withAlpha (0.45f);
        textTint = accent();
    }
    else if (down)
    {
        fill = panelAlt();
    }
    else if (highlighted)
    {
        fill = panelHigh().brighter (0.06f);
    }

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, radius);

    g.setColour (border);
    g.drawRoundedRectangle (bounds, radius, 1.0f);

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (accent().withAlpha (0.9f));
        g.drawRoundedRectangle (bounds.reduced (2.0f), radius - 1.0f, 1.8f);
    }
}

void DarkLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                      bool highlighted, bool)
{
    const auto variant = buttonVariant (button);
    auto colour = textForVariant (variant, button.getToggleState());

    if (variant == "nav" && ! button.getToggleState())
        colour = highlighted ? text() : mutedText();

    if (variant == "ghost" && highlighted && button.isEnabled())
        colour = text();

    if (! button.isEnabled())
        colour = mutedText().withAlpha (0.55f);

    g.setColour (colour);
    const auto font = getTextButtonFont (button, button.getHeight());
    g.setFont (font);

    const auto text = button.getButtonText();
    const auto icon = buttonIconName (button);
    auto area = button.getLocalBounds().reduced (6, 0).toFloat();

    if (icon.isEmpty())
    {
        g.drawFittedText (text, button.getLocalBounds().reduced (6, 0),
                          juce::Justification::centred, 1, 0.9f);
        return;
    }

    const auto iconSize = juce::jlimit (14.0f, 32.0f,
                                        static_cast<float> (button.getHeight()) * 0.32f);
    const auto textWidth = text.isEmpty()
                             ? 0.0f
                             : juce::GlyphArrangement::getStringWidth (font, text);
    const auto gap = text.isEmpty() ? 0.0f : 8.0f;
    const auto total = iconSize + gap + textWidth;
    const auto startX = area.getCentreX() - total * 0.5f;

    auto iconColour = colour;

    if (variant == "nav")
        iconColour = button.getToggleState() ? accent() : colour;

    drawIcon (g, icon, { startX, area.getCentreY() - iconSize * 0.5f, iconSize, iconSize },
              iconColour);

    if (text.isNotEmpty())
    {
        g.setColour (colour);
        g.setFont (font);
        g.drawFittedText (text,
                          juce::Rectangle<float> (startX + iconSize + gap,
                                                  area.getY(),
                                                  textWidth + 6.0f,
                                                  area.getHeight()).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.92f);
    }
}

juce::Font DarkLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    const auto requested = button.getProperties().getWithDefault ("plTextSize", 0.0).toString();
    const auto size = requested.getDoubleValue();

    if (size > 0.0)
        return sans (static_cast<float> (size), true);

    return sans (juce::jlimit (12.5f, 16.5f, static_cast<float> (buttonHeight) * 0.38f), true);
}

void DarkLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool highlighted, bool down)
{
    auto area = button.getLocalBounds();
    const auto boxSize = juce::jlimit (15.0f, 19.0f, static_cast<float> (area.getHeight()) * 0.66f);
    const auto box = juce::Rectangle<float> (static_cast<float> (area.getX()) + 1.0f,
                                             static_cast<float> (area.getCentreY()) - boxSize * 0.5f,
                                             boxSize, boxSize);
    const auto on = button.getToggleState();
    const auto enabled = button.isEnabled();

    auto fill = on ? accent() : panelAlt();
    auto border = on ? accent() : outline();

    if (! enabled)
    {
        fill = panelAlt().withAlpha (0.5f);
        border = outlineSoft();
    }
    else if (down)
    {
        fill = on ? accent().darker (0.1f) : panelHigh();
    }
    else if (highlighted)
    {
        fill = on ? accent().brighter (0.06f) : panelHigh();
    }

    g.setColour (fill);
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (border);
    g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.0f);

    if (on)
    {
        juce::Path tick;
        tick.startNewSubPath (box.getX() + box.getWidth() * 0.24f, box.getCentreY());
        tick.lineTo (box.getX() + box.getWidth() * 0.44f, box.getY() + box.getHeight() * 0.70f);
        tick.lineTo (box.getX() + box.getWidth() * 0.78f, box.getY() + box.getHeight() * 0.30f);
        g.setColour (accentInk());
        g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    auto textArea = area.withTrimmedLeft (static_cast<int> (box.getWidth()) + 9);
    g.setColour (enabled ? (on ? text() : mutedText()) : mutedText().withAlpha (0.45f));
    g.setFont (sans (13.0f, on));
    g.drawFittedText (button.getButtonText(), textArea,
                      juce::Justification::centredLeft, 1, 0.92f);

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (accent().withAlpha (0.9f));
        g.drawRoundedRectangle (area.toFloat().reduced (1.0f), 6.0f, 1.6f);
    }
}

void DarkLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                    bool isButtonDown, int buttonX, int buttonY,
                                    int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH);

    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                          static_cast<float> (width),
                                          static_cast<float> (height)).reduced (0.5f);
    const auto enabled = box.isEnabled();
    auto fill = box.findColour (juce::ComboBox::backgroundColourId);
    auto border = box.findColour (juce::ComboBox::outlineColourId);

    if (! enabled)
    {
        fill = panelAlt().withAlpha (0.5f);
        border = outlineSoft();
    }
    else if (isButtonDown)
    {
        fill = panelHigh();
        border = accent().withAlpha (0.6f);
    }
    else if (box.isMouseOverOrDragging())
    {
        fill = panelHigh();
        border = outline().brighter (0.08f);
    }

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (border);
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    const auto arrowArea = juce::Rectangle<float> (bounds.getRight() - 28.0f,
                                                   bounds.getY(), 20.0f, bounds.getHeight());
    juce::Path chevron;
    chevron.startNewSubPath (arrowArea.getCentreX() - 4.0f, arrowArea.getCentreY() - 2.0f);
    chevron.lineTo (arrowArea.getCentreX(), arrowArea.getCentreY() + 2.5f);
    chevron.lineTo (arrowArea.getCentreX() + 4.0f, arrowArea.getCentreY() - 2.0f);
    g.setColour (enabled ? mutedText() : mutedText().withAlpha (0.4f));
    g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    if (box.hasKeyboardFocus (true))
    {
        g.setColour (accent().withAlpha (0.9f));
        g.drawRoundedRectangle (bounds.reduced (2.0f), 6.5f, 1.6f);
    }
}

juce::Font DarkLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return sans (13.5f);
}

void DarkLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    juce::LookAndFeel_V4::positionComboBoxText (box, label);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setBorderSize (juce::BorderSize<int> (0, 12, 0, 6));
    label.setColour (juce::Label::textColourId,
                     box.findColour (juce::ComboBox::textColourId));
    label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

void DarkLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::IncDecButtons)
        return;

    const auto enabled = slider.isEnabled();
    const auto horizontal = style == juce::Slider::LinearHorizontal
                         || style == juce::Slider::LinearBar
                         || style == juce::Slider::LinearBarVertical;
    const auto vertical = style == juce::Slider::LinearVertical;
    const auto alpha = enabled ? 1.0f : 0.4f;

    const auto trackColour = slider.findColour (juce::Slider::backgroundColourId)
                                   .withMultipliedAlpha (alpha);
    const auto fillColour = slider.findColour (juce::Slider::trackColourId)
                                  .withMultipliedAlpha (alpha);
    const auto thumbColour = slider.findColour (juce::Slider::thumbColourId)
                                   .withMultipliedAlpha (alpha);

    if (horizontal)
    {
        const auto track = juce::Rectangle<float> (static_cast<float> (x) + 8.0f,
                                                   static_cast<float> (y + height) * 0.5f - 2.5f,
                                                   static_cast<float> (width) - 16.0f,
                                                   5.0f);
        g.setColour (trackColour);
        g.fillRoundedRectangle (track, 2.5f);

        const auto fillRight = juce::jlimit (track.getX(), track.getRight(), sliderPos);
        auto filled = track.withWidth (fillRight - track.getX());

        if (slider.isTwoValue())
        {
            const auto left = juce::jlimit (track.getX(), track.getRight(), minSliderPos);
            const auto right = juce::jlimit (track.getX(), track.getRight(), maxSliderPos);
            filled = track.withX (left).withWidth (juce::jmax (0.0f, right - left));
        }

        if (filled.getWidth() > 0.5f)
        {
            g.setColour (fillColour);
            g.fillRoundedRectangle (filled, 2.5f);
        }

        const auto thumb = juce::Rectangle<float> (14.0f, 14.0f)
                               .withCentre ({ sliderPos, static_cast<float> (y + height) * 0.5f });
        g.setColour (thumbColour);
        g.fillEllipse (thumb);
        g.setColour (background().withAlpha (0.65f * alpha));
        g.fillEllipse (thumb.reduced (4.5f));
    }
    else if (vertical)
    {
        const auto track = juce::Rectangle<float> (static_cast<float> (x + width) * 0.5f - 2.5f,
                                                   static_cast<float> (y) + 8.0f,
                                                   5.0f,
                                                   static_cast<float> (height) - 16.0f);
        g.setColour (trackColour);
        g.fillRoundedRectangle (track, 2.5f);

        const auto filled = track.withTop (juce::jlimit (track.getY(), track.getBottom(), sliderPos));
        g.setColour (fillColour);
        g.fillRoundedRectangle (filled, 2.5f);

        const auto thumb = juce::Rectangle<float> (14.0f, 14.0f)
                               .withCentre ({ static_cast<float> (x + width) * 0.5f, sliderPos });
        g.setColour (thumbColour);
        g.fillEllipse (thumb);
        g.setColour (background().withAlpha (0.65f * alpha));
        g.fillEllipse (thumb.reduced (4.5f));
    }

    if (slider.hasKeyboardFocus (true))
    {
        g.setColour (accent().withAlpha (0.9f));
        g.drawRoundedRectangle (slider.getLocalBounds().toFloat().reduced (1.5f), 6.0f, 1.6f);
    }
}

int DarkLookAndFeel::getSliderThumbRadius (juce::Slider&)
{
    return 8;
}

juce::Button* DarkLookAndFeel::createSliderButton (juce::Slider&, bool isIncrement)
{
    auto* button = new juce::TextButton (isIncrement ? "+" : "-", {});
    setButtonVariant (*button, "ghost");
    setButtonTextSize (*button, 15.0f);
    return button;
}

juce::Label* DarkLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (mono (12.5f, true));
    label->setJustificationType (juce::Justification::centredRight);
    label->setBorderSize (juce::BorderSize<int> (0, 7, 0, 7));
    label->setColour (juce::Label::textColourId, text());
    label->setColour (juce::Label::backgroundColourId, panelAlt());
    label->setColour (juce::Label::outlineColourId, outline());
    label->setTooltip (slider.getTooltip());
    return label;
}

void DarkLookAndFeel::drawGroupComponentOutline (juce::Graphics& g, int w, int h,
                                                 const juce::String& text,
                                                 const juce::Justification&,
                                                 juce::GroupComponent& group)
{
    juce::ignoreUnused (group);

    drawCard (g, juce::Rectangle<int> (0, 0, w, h), false);

    if (text.isEmpty())
        return;

    g.setColour (accent());
    g.fillEllipse (16.0f, 15.0f, 4.0f, 4.0f);

    g.setColour (mutedText());
    g.setFont (kicker (11.0f));
    g.drawFittedText (text.toUpperCase(), 26, 7, w - 42, 18,
                      juce::Justification::centredLeft, 1);
}

void DarkLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&,
                                     int x, int y, int width, int height,
                                     bool isScrollbarVertical, int thumbStartPosition,
                                     int thumbSize, bool isMouseOver, bool)
{
    auto thumb = isScrollbarVertical
                   ? juce::Rectangle<int> (x + 3, thumbStartPosition, juce::jmax (4, width - 6), thumbSize)
                   : juce::Rectangle<int> (thumbStartPosition, y + 3, thumbSize, juce::jmax (4, height - 6));

    g.setColour (mutedText().withAlpha (isMouseOver ? 0.55f : 0.32f));
    g.fillRoundedRectangle (thumb.toFloat(), static_cast<float> (juce::jmin (thumb.getWidth(),
                                                                            thumb.getHeight())) * 0.5f);
}

void DarkLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text,
                                   int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                          static_cast<float> (width),
                                          static_cast<float> (height)).reduced (0.5f);
    g.setColour (panelHigh());
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (outline());
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    g.setColour (DarkLookAndFeel::text());
    g.setFont (sans (12.0f));
    g.drawFittedText (text, 10, 0, width - 20, height,
                      juce::Justification::centredLeft, 3, 0.92f);
}

void DarkLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                          static_cast<float> (width),
                                          static_cast<float> (height)).reduced (0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (bounds.reduced (2.0f), 11.0f);
    g.setColour (panelAlt());
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (outline());
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);
}

juce::Font DarkLookAndFeel::getPopupMenuFont()
{
    return sans (13.5f);
}

void DarkLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                                juce::TextEditor&)
{
    g.setColour (background().darker (0.2f));
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f,
                                                    static_cast<float> (width),
                                                    static_cast<float> (height)).reduced (0.5f),
                            8.0f);
}

void DarkLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                             juce::TextEditor& editor)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                          static_cast<float> (width),
                                          static_cast<float> (height)).reduced (0.5f);
    const auto focused = editor.hasKeyboardFocus (true);
    g.setColour (focused ? accent().withAlpha (0.9f) : outline());
    g.drawRoundedRectangle (bounds, 8.0f, focused ? 1.6f : 1.0f);
}

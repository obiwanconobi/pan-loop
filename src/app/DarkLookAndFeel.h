#pragma once

#include <JuceHeader.h>

/** Dark, high-contrast stage theme with a compact set of shared tokens. */
class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();

    // Core palette ------------------------------------------------------------
    static juce::Colour background()      { return juce::Colour (0xff0a0c10); }
    static juce::Colour panel()           { return juce::Colour (0xff11151d); }
    static juce::Colour panelAlt()        { return juce::Colour (0xff171c26); }
    static juce::Colour panelHigh()       { return juce::Colour (0xff1e2531); }
    static juce::Colour outline()         { return juce::Colour (0xff2a3341); }
    static juce::Colour outlineSoft()     { return juce::Colour (0xff1c2430); }
    static juce::Colour text()            { return juce::Colour (0xfff2f5fa); }
    static juce::Colour mutedText()       { return juce::Colour (0xff96a3b5); }
    static juce::Colour accent()          { return juce::Colour (0xff5bd4ff); }
    static juce::Colour accentDeep()      { return juce::Colour (0xff28a9d8); }
    static juce::Colour accentInk()       { return juce::Colour (0xff04171f); }
    static juce::Colour recording()       { return juce::Colour (0xffff5c6c); }
    static juce::Colour recordingInk()    { return juce::Colour (0xff2a050a); }
    static juce::Colour armed()           { return juce::Colour (0xffffc857); }
    static juce::Colour armedInk()        { return juce::Colour (0xff241700); }
    static juce::Colour playing()         { return juce::Colour (0xff58e08d); }
    static juce::Colour playingInk()      { return juce::Colour (0xff04180c); }
    static juce::Colour violet()          { return juce::Colour (0xffa78bfa); }
    static juce::Colour pink()            { return juce::Colour (0xfff472b6); }

    // Type helpers ------------------------------------------------------------
    static juce::Font sans (float height, bool bold = false);
    static juce::Font mono (float height, bool bold = false);
    static juce::Font kicker (float height = 11.0f);

    static void styleLabel (juce::Label&, float height, bool bold = false);
    static void styleLabel (juce::Label&, float height, bool bold,
                            juce::Colour colour,
                            juce::Justification = juce::Justification::centredLeft);

    // Shared painting ---------------------------------------------------------
    static void drawCard (juce::Graphics&, juce::Rectangle<int> bounds, bool accentGlow = false);
    static void drawCard (juce::Graphics&, juce::Rectangle<float> bounds, bool accentGlow = false);
    static void drawPill (juce::Graphics&, juce::Rectangle<int> bounds,
                          juce::Colour fill, juce::Colour border = {},
                          float radius = -1.0f);

    /** Small stroked/filled vector icons. Names are listed in DarkLookAndFeel.cpp. */
    static void drawIcon (juce::Graphics&, const juce::String& name,
                          juce::Rectangle<float> bounds,
                          juce::Colour colour, float strokeWidth = 0.0f);

    // Button decoration -------------------------------------------------------
    static void setButtonVariant (juce::Button&, const juce::String& variant);
    static void setButtonIcon (juce::Button&, const juce::String& icon);
    static void setButtonTextSize (juce::Button&, float size);

    // LookAndFeel overrides ---------------------------------------------------
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override;
    juce::Button* createSliderButton (juce::Slider&, bool isIncrement) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawGroupComponentOutline (juce::Graphics&, int w, int h,
                                    const juce::String& text,
                                    const juce::Justification&,
                                    juce::GroupComponent&) override;

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&,
                        int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition,
                        int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawTooltip (juce::Graphics&, const juce::String& text,
                      int width, int height) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    juce::Font getPopupMenuFont() override;

    void fillTextEditorBackground (juce::Graphics&, int width, int height,
                                   juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height,
                                juce::TextEditor&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLookAndFeel)
};

#pragma once

#include <JuceHeader.h>

/** A dark, high-contrast look for stage use. */
class DarkLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel();

    static juce::Colour background()      { return juce::Colour (0xff14161a); }
    static juce::Colour panel()           { return juce::Colour (0xff1d2026); }
    static juce::Colour panelAlt()        { return juce::Colour (0xff22262e); }
    static juce::Colour outline()         { return juce::Colour (0xff343b46); }
    static juce::Colour text()            { return juce::Colour (0xffe6e9ef); }
    static juce::Colour mutedText()       { return juce::Colour (0xff9aa4b2); }
    static juce::Colour accent()          { return juce::Colour (0xff5aa9ff); }
    static juce::Colour recording()       { return juce::Colour (0xffff5b5b); }
    static juce::Colour armed()           { return juce::Colour (0xffffb84d); }
    static juce::Colour playing()         { return juce::Colour (0xff5ad48a); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLookAndFeel)
};

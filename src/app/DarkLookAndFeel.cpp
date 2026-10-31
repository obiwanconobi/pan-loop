#include "DarkLookAndFeel.h"

DarkLookAndFeel::DarkLookAndFeel()
{
    setColourScheme ({ 0xff14161a, 0xff1d2026, 0xff22262e, 0xff343b46,
                       0xffe6e9ef, 0xff0f1115, 0xff5aa9ff, 0xff2a6fdb,
                       0xffffffff });

    setColour (juce::ResizableWindow::backgroundColourId, background());
    setColour (juce::DocumentWindow::textColourId, text());

    setColour (juce::Label::textColourId, text());
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2f38));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2a6fdb));
    setColour (juce::TextButton::textColourOffId, text());
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    setColour (juce::ComboBox::backgroundColourId, panelAlt());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::ComboBox::outlineColourId, outline());
    setColour (juce::ComboBox::arrowColourId, mutedText());
    setColour (juce::PopupMenu::backgroundColourId, panelAlt());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a6fdb));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::Slider::thumbColourId, accent());
    setColour (juce::Slider::trackColourId, juce::Colour (0xff2a6fdb));
    setColour (juce::Slider::backgroundColourId, panelAlt());
    setColour (juce::Slider::textBoxTextColourId, text());
    setColour (juce::Slider::textBoxBackgroundColourId, panelAlt());
    setColour (juce::Slider::textBoxOutlineColourId, outline());

    setColour (juce::ToggleButton::textColourId, text());
    setColour (juce::ToggleButton::tickColourId, accent());
    setColour (juce::ToggleButton::tickDisabledColourId, outline());

    setColour (juce::TabbedComponent::backgroundColourId, background());
    setColour (juce::TabbedComponent::outlineColourId, outline());
    setColour (juce::TabbedButtonBar::tabOutlineColourId, outline());
    setColour (juce::TabbedButtonBar::frontOutlineColourId, accent());
    setColour (juce::TabbedButtonBar::tabTextColourId, mutedText());
    setColour (juce::TabbedButtonBar::frontTextColourId, text());

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f1115));
    setColour (juce::TextEditor::textColourId, text());
    setColour (juce::TextEditor::outlineColourId, outline());
    setColour (juce::TextEditor::focusedOutlineColourId, accent());
    setColour (juce::TextEditor::highlightColourId, juce::Colour (0x552a6fdb));
    setColour (juce::CaretComponent::caretColourId, accent());

    setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0f1115));
    setColour (juce::ListBox::textColourId, text());
    setColour (juce::ListBox::outlineColourId, outline());

    setColour (juce::TooltipWindow::backgroundColourId, panelAlt());
    setColour (juce::TooltipWindow::textColourId, text());
    setColour (juce::TooltipWindow::outlineColourId, outline());
}

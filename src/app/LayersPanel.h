#pragma once

#include <JuceHeader.h>

#include "audio/LiveLooper.h"

class AudioEngine;

/** Live loop layer list and mix controls. */
class LayersPanel final : public juce::Component,
                          private juce::ListBoxModel,
                          private juce::Timer
{
public:
    explicit LayersPanel (AudioEngine& audioEngine);
    ~LayersPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void deleteKeyPressed (int lastRowSelected) override;

    void timerCallback() override;
    void refreshSnapshot();
    void updateControls();
    void updateSelectionControls();
    void deleteSelectedLayer();

    static juce::String formatDuration (double seconds);
    static juce::String formatBytes (int bytes);

    AudioEngine& engine;

    LiveLooper::Snapshot snapshot;
    int selectedLayerId = -1;
    int lastLayerCount = -1;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label statusLabel;
    juce::Label totalLabel;
    juce::Label selectionLabel;
    juce::Label gainLabel;
    juce::Label panLabel;
    juce::Label emptyLabel;

    juce::TextButton clearButton { "Clear layers" };
    juce::TextButton deleteButton { "Delete" };
    juce::ToggleButton muteButton { "Mute" };
    juce::ToggleButton soloButton { "Solo" };
    juce::Slider gainSlider;
    juce::Slider panSlider;

    juce::ListBox listBox { "live layers", this };

    juce::Rectangle<int> listArea;
    juce::Rectangle<int> listCardArea;
    juce::Rectangle<int> bottomBarArea;
    juce::Rectangle<int> emptyIconArea;
    juce::Rectangle<int> statsPillArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayersPanel)
};

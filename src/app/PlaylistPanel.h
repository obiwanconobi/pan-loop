#pragma once

#include <JuceHeader.h>

#include "audio/AudioFileLoader.h"
#include "audio/Playlist.h"

class AudioEngine;

/** Backing-file sequence: add, order, inspect and remove files. */
class PlaylistPanel final : public juce::Component,
                            private juce::ListBoxModel,
                            private juce::Timer,
                            private juce::FileDragAndDropTarget
{
public:
    explicit PlaylistPanel (AudioEngine& audioEngine);
    ~PlaylistPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Debug/test helper used by the development smoke-test hook. */
    void addFilesForTest (const juce::StringArray& paths);

private:
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void deleteKeyPressed (int lastRowSelected) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;

    void timerCallback() override;

    void chooseFiles();
    void beginLoading (const juce::Array<juce::File>& files);
    void handleLoadedItem (std::shared_ptr<PlaylistItem> item, int index, int total);
    void handleFileFailed (int index, int total, const juce::String& error);
    void handleLoadingFinished (int loaded, int failed);

    void refreshSnapshot();
    void updateButtons();
    void updateSelectionControls();
    void removeSelected();
    void moveSelected (int delta);
    void clearPlaylist();

    static juce::String formatDuration (double seconds);
    static juce::String formatBytes (int bytes);

    AudioEngine& engine;

    Playlist::Snapshot snapshot;
    int lastItemCount = -1;
    int selectedIndex = -1;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label statusLabel;
    juce::Label totalLabel;
    juce::Label memoryLabel;
    juce::Label selectionLabel;
    juce::Label gainLabel;
    juce::Label emptyLabel;

    juce::TextButton addButton { "Add files..." };
    juce::TextButton removeButton { "Remove" };
    juce::TextButton upButton { "Move up" };
    juce::TextButton downButton { "Move down" };
    juce::TextButton clearButton { "Clear" };
    juce::ToggleButton backingMuteButton { "Mute backing" };

    juce::ListBox listBox { "backing files", this };
    juce::Slider gainSlider;

    juce::Rectangle<int> listArea;
    juce::Rectangle<int> listCardArea;
    juce::Rectangle<int> bottomBarArea;
    juce::Rectangle<int> loadingBarArea;
    juce::Rectangle<int> emptyIconArea;
    juce::Rectangle<int> filesPillArea;
    juce::Rectangle<int> memoryPillArea;

    double loadingProgress = 0.0;
    bool loading = false;
    int loadTotal = 0;
    int loadCompleted = 0;
    int loadFailed = 0;

    bool chooserOpen = false;
    bool fileDragActive = false;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File lastDirectory;

    AudioFileLoader loader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaylistPanel)
};

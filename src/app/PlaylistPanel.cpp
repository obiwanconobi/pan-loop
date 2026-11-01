#include "PlaylistPanel.h"

#include "app/DarkLookAndFeel.h"
#include "audio/AudioEngine.h"
#include "audio/Transport.h"

#include <cmath>

namespace
{
    void styleLabel (juce::Label& label, float height, bool bold = false)
    {
        label.setFont (juce::Font (juce::FontOptions (height,
                                                      bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
        label.setJustificationType (juce::Justification::centredLeft);
    }
}

PlaylistPanel::PlaylistPanel (AudioEngine& audioEngine)
    : engine (audioEngine)
{
    setOpaque (true);

    lastDirectory = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    if (! lastDirectory.isDirectory())
        lastDirectory = juce::File::getSpecialLocation (juce::File::userHomeDirectory);

    styleLabel (titleLabel, 28.0f, true);
    styleLabel (subtitleLabel, 12.5f);
    styleLabel (statusLabel, 12.0f);
    styleLabel (totalLabel, 11.5f, true);
    styleLabel (memoryLabel, 11.5f, true);
    styleLabel (selectionLabel, 14.0f, true);
    styleLabel (gainLabel, 10.5f, true);
    styleLabel (emptyLabel, 13.5f);

    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.02f));
    subtitleLabel.setFont (DarkLookAndFeel::sans (12.5f));
    statusLabel.setFont (DarkLookAndFeel::mono (11.5f));
    totalLabel.setFont (DarkLookAndFeel::mono (11.5f, true));
    memoryLabel.setFont (DarkLookAndFeel::mono (11.5f, true));
    gainLabel.setFont (DarkLookAndFeel::kicker (10.5f));

    titleLabel.setText ("Backing playlist", juce::dontSendNotification);
    subtitleLabel.setText ("Files play in order and loop as one master cycle",
                           juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    statusLabel.setText ("Add WAV, FLAC or MP3 files to build a sequence.",
                         juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    totalLabel.setText ("0 files   |   Cycle 0:00.000", juce::dontSendNotification);
    totalLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
    totalLabel.setJustificationType (juce::Justification::centred);
    memoryLabel.setText ("0 KB RAM", juce::dontSendNotification);
    memoryLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    memoryLabel.setJustificationType (juce::Justification::centred);

    selectionLabel.setText ("No file selected", juce::dontSendNotification);
    selectionLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
    gainLabel.setText ("GAIN", juce::dontSendNotification);
    gainLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    emptyLabel.setJustificationType (juce::Justification::centred);
    emptyLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    emptyLabel.setInterceptsMouseClicks (false, false);
    emptyLabel.setText ("No backing tracks yet.\nAdd WAV, FLAC, MP3 or OGG files to build your loop.",
                        juce::dontSendNotification);

    addButton.setTooltip ("Add backing audio files");
    addButton.onClick = [this] { chooseFiles(); };

    removeButton.setTooltip ("Remove the selected file");
    upButton.setTooltip ("Move the selected file earlier");
    downButton.setTooltip ("Move the selected file later");
    clearButton.setTooltip ("Remove every file from the playlist");

    removeButton.onClick = [this] { removeSelected(); };
    upButton.onClick = [this] { moveSelected (-1); };
    downButton.onClick = [this] { moveSelected (1); };
    clearButton.onClick = [this] { clearPlaylist(); };

    DarkLookAndFeel::setButtonVariant (addButton, "primary");
    DarkLookAndFeel::setButtonIcon (addButton, "plus");
    DarkLookAndFeel::setButtonTextSize (addButton, 13.0f);

    DarkLookAndFeel::setButtonVariant (removeButton, "default");
    DarkLookAndFeel::setButtonIcon (removeButton, "minus");
    DarkLookAndFeel::setButtonTextSize (removeButton, 12.5f);

    DarkLookAndFeel::setButtonVariant (upButton, "ghost");
    DarkLookAndFeel::setButtonIcon (upButton, "up");
    DarkLookAndFeel::setButtonTextSize (upButton, 12.5f);

    DarkLookAndFeel::setButtonVariant (downButton, "ghost");
    DarkLookAndFeel::setButtonIcon (downButton, "down");
    DarkLookAndFeel::setButtonTextSize (downButton, 12.5f);

    DarkLookAndFeel::setButtonVariant (clearButton, "dangerSoft");
    DarkLookAndFeel::setButtonIcon (clearButton, "trash");
    DarkLookAndFeel::setButtonTextSize (clearButton, 12.5f);

    backingMuteButton.setTooltip ("Mute or restore the backing playlist without stopping transport");
    backingMuteButton.setToggleState (engine.isBackingMuted(), juce::dontSendNotification);
    backingMuteButton.onClick = [this]
    {
        engine.setBackingMuted (backingMuteButton.getToggleState());
    };

    listBox.setRowHeight (44);
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    listBox.setOutlineThickness (0);

    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    gainSlider.setRange (0.0, 200.0, 1.0);
    gainSlider.setValue (100.0, juce::dontSendNotification);
    gainSlider.setTextValueSuffix (" %");
    gainSlider.setEnabled (false);
    gainSlider.onValueChange = [this]
    {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int> (snapshot.items.size()))
            engine.getPlaylist().setItemGain (selectedIndex,
                                               static_cast<float> (gainSlider.getValue() / 100.0));
    };

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (totalLabel);
    addAndMakeVisible (memoryLabel);
    addAndMakeVisible (selectionLabel);
    addAndMakeVisible (gainLabel);
    addAndMakeVisible (addButton);
    addAndMakeVisible (removeButton);
    addAndMakeVisible (upButton);
    addAndMakeVisible (downButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (backingMuteButton);
    addAndMakeVisible (listBox);
    addAndMakeVisible (emptyLabel);
    addAndMakeVisible (gainSlider);

    juce::Component::SafePointer<PlaylistPanel> safeThis (this);

    loader.onFileStarted = [safeThis] (int index, int total, const juce::String& fileName)
    {
        juce::MessageManager::callAsync ([safeThis, index, total, fileName]
        {
            if (safeThis == nullptr)
                return;

            safeThis->loadingProgress = total > 0 ? static_cast<double> (index) / total : 0.0;
            safeThis->statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
            safeThis->statusLabel.setText ("Loading " + juce::String (index + 1)
                                               + "/" + juce::String (total) + ": " + fileName,
                                           juce::dontSendNotification);
            safeThis->repaint (safeThis->loadingBarArea);
        });
    };

    loader.onFileLoaded = [safeThis] (int index, int total, std::shared_ptr<PlaylistItem> item)
    {
        juce::MessageManager::callAsync ([safeThis, index, total, item]
        {
            if (safeThis != nullptr)
                safeThis->handleLoadedItem (item, index, total);
        });
    };

    loader.onFileFailed = [safeThis] (int index, int total, const juce::String& error)
    {
        juce::MessageManager::callAsync ([safeThis, index, total, error]
        {
            if (safeThis != nullptr)
                safeThis->handleFileFailed (index, total, error);
        });
    };

    loader.onFinished = [safeThis] (int loaded, int failed)
    {
        juce::MessageManager::callAsync ([safeThis, loaded, failed]
        {
            if (safeThis != nullptr)
                safeThis->handleLoadingFinished (loaded, failed);
        });
    };

    refreshSnapshot();
    startTimerHz (15);
}

PlaylistPanel::~PlaylistPanel()
{
    stopTimer();
}

void PlaylistPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    if (! filesPillArea.isEmpty())
    {
        DarkLookAndFeel::drawPill (g, filesPillArea,
                                   DarkLookAndFeel::panelAlt().withAlpha (0.72f),
                                   DarkLookAndFeel::outline(), 9.0f);
        DarkLookAndFeel::drawPill (g, memoryPillArea,
                                   DarkLookAndFeel::panelAlt().withAlpha (0.72f),
                                   DarkLookAndFeel::outline(), 9.0f);
    }

    DarkLookAndFeel::drawCard (g, listCardArea,
                               fileDragActive);
    DarkLookAndFeel::drawCard (g, bottomBarArea);

    if (snapshot.items.empty() && ! loading && ! emptyIconArea.isEmpty())
    {
        g.setColour (DarkLookAndFeel::panelAlt());
        g.fillEllipse (emptyIconArea.toFloat());
        g.setColour (DarkLookAndFeel::outline());
        g.drawEllipse (emptyIconArea.toFloat().reduced (0.5f), 1.0f);

        g.setColour (DarkLookAndFeel::accent());
        DarkLookAndFeel::drawIcon (g, "music", emptyIconArea.toFloat().reduced (16.0f),
                                   DarkLookAndFeel::accent(), 1.8f);
    }

    if (loading && ! loadingBarArea.isEmpty())
    {
        auto bar = loadingBarArea.toFloat();
        g.setColour (DarkLookAndFeel::panelHigh());
        g.fillRoundedRectangle (bar, 2.0f);

        const auto progress = juce::jlimit (0.0, 1.0, loadingProgress);
        const auto filled = bar.withWidth (bar.getWidth() * static_cast<float> (progress));

        if (filled.getWidth() > 1.0f)
        {
            g.setColour (DarkLookAndFeel::accent());
            g.fillRoundedRectangle (filled, 2.0f);
        }
    }

    if (fileDragActive && ! listCardArea.isEmpty())
    {
        g.setColour (DarkLookAndFeel::accent().withAlpha (0.06f));
        g.fillRoundedRectangle (listCardArea.toFloat().reduced (1.0f), 13.0f);
    }

    if (! bottomBarArea.isEmpty())
    {
        auto inner = bottomBarArea.reduced (20, 12);

        g.setColour (DarkLookAndFeel::mutedText());
        g.setFont (DarkLookAndFeel::kicker (10.0f));
        g.drawText ("SELECTED FILE", inner.getX(), inner.getY(),
                    280, 14, juce::Justification::centredLeft, false);

        g.setFont (DarkLookAndFeel::sans (11.0f));
        g.setColour (DarkLookAndFeel::mutedText().withAlpha (0.8f));
        g.drawText (snapshot.items.empty()
                        ? "Add files to create a backing cycle."
                        : "Gain is saved with the session and applies during playback.",
                    inner.getX(), bottomBarArea.getBottom() - 22,
                    juce::jmax (260, inner.getWidth() / 2), 16,
                    juce::Justification::centredLeft, false);
    }
}

void PlaylistPanel::resized()
{
    auto area = getLocalBounds().reduced (24);

    auto header = area.removeFromTop (54);
    auto titleArea = header.removeFromLeft (juce::jmax (320, header.getWidth() - 400));
    titleLabel.setBounds (titleArea.removeFromTop (32));
    subtitleLabel.setBounds (titleArea);

    memoryPillArea = header.removeFromRight (150).withSizeKeepingCentre (150, 30);
    header.removeFromRight (10);
    filesPillArea = header.removeFromRight (200).withSizeKeepingCentre (200, 30);
    totalLabel.setBounds (filesPillArea.reduced (10, 0));
    memoryLabel.setBounds (memoryPillArea.reduced (10, 0));

    area.removeFromTop (12);

    auto buttons = area.removeFromTop (38);
    addButton.setBounds (buttons.removeFromLeft (128));
    buttons.removeFromLeft (8);
    removeButton.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (8);
    upButton.setBounds (buttons.removeFromLeft (84));
    buttons.removeFromLeft (8);
    downButton.setBounds (buttons.removeFromLeft (92));
    buttons.removeFromLeft (8);
    clearButton.setBounds (buttons.removeFromLeft (94));
    backingMuteButton.setBounds (buttons.removeFromRight (160));

    area.removeFromTop (8);
    statusLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (8);

    bottomBarArea = area.removeFromBottom (84);
    area.removeFromBottom (10);
    listCardArea = area;

    listArea = listCardArea.reduced (8);
    listBox.setBounds (listArea);

    loadingBarArea = juce::Rectangle<int> (listCardArea.getX() + 12,
                                           listCardArea.getY() + 4,
                                           listCardArea.getWidth() - 24, 3);

    emptyIconArea = juce::Rectangle<int> (56, 56)
                        .withCentre ({ listCardArea.getCentreX(),
                                       listCardArea.getCentreY() - 28 });
    emptyLabel.setBounds (listCardArea.withTrimmedTop (listCardArea.getHeight() / 2 + 8)
                                      .withTrimmedBottom (20)
                                      .reduced (30, 0));

    auto inner = bottomBarArea.reduced (20, 12);
    inner.removeFromTop (16);

    auto left = inner.removeFromLeft (inner.getWidth() / 2);
    selectionLabel.setBounds (left.removeFromTop (24));

    auto right = inner;
    auto gainRow = right.removeFromTop (28);
    gainLabel.setBounds (gainRow.removeFromLeft (44));
    gainSlider.setBounds (gainRow.removeFromLeft (juce::jmin (280, gainRow.getWidth())));
}

int PlaylistPanel::getNumRows()
{
    return static_cast<int> (snapshot.items.size());
}

void PlaylistPanel::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                      int width, int height, bool rowIsSelected)
{
    if (! juce::isPositiveAndBelow (rowNumber, static_cast<int> (snapshot.items.size())))
        return;

    const auto& item = snapshot.items[static_cast<size_t> (rowNumber)];

    if (rowIsSelected)
    {
        g.setColour (DarkLookAndFeel::accent().withAlpha (0.14f));
        g.fillRoundedRectangle (juce::Rectangle<float> (6.0f, 2.0f,
                                                        static_cast<float> (width - 12),
                                                        static_cast<float> (height - 4)), 8.0f);
    }
    else if ((rowNumber % 2) == 1)
    {
        g.setColour (juce::Colours::white.withAlpha (0.012f));
        g.fillRect (6, 2, width - 12, height - 4);
    }

    g.setColour (rowIsSelected ? DarkLookAndFeel::accent()
                               : DarkLookAndFeel::mutedText());
    g.setFont (DarkLookAndFeel::mono (10.5f));
    g.drawText (juce::String (rowNumber + 1).paddedLeft ('0', 2),
                16, 0, 30, height, juce::Justification::centredLeft, false);

    const auto extension = item.name.fromLastOccurrenceOf (".", false, false).toUpperCase();
    const auto badge = juce::Rectangle<float> (54.0f,
                                               (static_cast<float> (height) - 20.0f) * 0.5f,
                                               30.0f, 20.0f);
    g.setColour (DarkLookAndFeel::accent().withAlpha (rowIsSelected ? 0.20f : 0.10f));
    g.fillRoundedRectangle (badge, 5.0f);
    g.setColour (DarkLookAndFeel::accent().withAlpha (rowIsSelected ? 1.0f : 0.8f));
    g.setFont (DarkLookAndFeel::mono (8.5f, true));
    g.drawText (extension.substring (0, 4), badge.toNearestInt(),
                juce::Justification::centred, false);

    g.setColour (rowIsSelected ? DarkLookAndFeel::text() : DarkLookAndFeel::text().withAlpha (0.86f));
    g.setFont (DarkLookAndFeel::sans (14.0f, rowIsSelected));
    g.drawFittedText (item.name, 96, 0, width - 300, height,
                      juce::Justification::centredLeft, 1, 0.92f);

    if (std::abs (item.gain - 1.0f) > 0.005f)
    {
        const auto gainText = juce::String (juce::roundToInt (item.gain * 100.0f)) + "%";
        auto gainArea = juce::Rectangle<int> (width - 196, 0, 64, height);

        g.setColour (DarkLookAndFeel::armed().withAlpha (0.12f));
        g.fillRoundedRectangle (gainArea.toFloat().withSizeKeepingCentre (54.0f, 18.0f), 4.0f);
        g.setColour (DarkLookAndFeel::armed());
        g.setFont (DarkLookAndFeel::mono (10.0f, true));
        g.drawText (gainText, gainArea, juce::Justification::centred, false);
    }

    g.setColour (DarkLookAndFeel::mutedText());
    g.setFont (DarkLookAndFeel::mono (11.5f));
    g.drawText (formatDuration (item.durationSeconds),
                width - 118, 0, 100, height,
                juce::Justification::centredRight, false);

    g.setColour (DarkLookAndFeel::outlineSoft().withAlpha (0.6f));
    g.fillRect (16, height - 1, width - 32, 1);
}

void PlaylistPanel::selectedRowsChanged (int lastRowSelected)
{
    selectedIndex = lastRowSelected;
    updateSelectionControls();
    updateButtons();
}

void PlaylistPanel::deleteKeyPressed (int lastRowSelected)
{
    selectedIndex = lastRowSelected;
    removeSelected();
}

bool PlaylistPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const auto extension = juce::File (path).getFileExtension().toLowerCase();

        if (extension == ".wav" || extension == ".flac" || extension == ".mp3"
            || extension == ".aiff" || extension == ".ogg")
            return true;
    }

    return false;
}

void PlaylistPanel::filesDropped (const juce::StringArray& files, int, int)
{
    fileDragActive = false;
    repaint (listArea);

    if (loading || ! engine.getTransport().isStopped())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Stop the transport before adding files.",
                             juce::dontSendNotification);
        return;
    }

    if (engine.hasLiveLayers())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Clear live layers before changing the playlist.",
                             juce::dontSendNotification);
        return;
    }

    juce::Array<juce::File> dropped;

    for (const auto& path : files)
        if (path.isNotEmpty())
            dropped.add (juce::File (path));

    beginLoading (dropped);
}

void PlaylistPanel::fileDragEnter (const juce::StringArray&, int, int)
{
    if (! fileDragActive)
    {
        fileDragActive = true;
        repaint (listArea);
    }
}

void PlaylistPanel::fileDragExit (const juce::StringArray&)
{
    if (fileDragActive)
    {
        fileDragActive = false;
        repaint (listArea);
    }
}

void PlaylistPanel::timerCallback()
{
    refreshSnapshot();
}

void PlaylistPanel::chooseFiles()
{
    if (loading || chooserOpen)
        return;

    if (! engine.getTransport().isStopped())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Stop the transport before adding or reordering files.",
                             juce::dontSendNotification);
        return;
    }

    if (engine.hasLiveLayers())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Clear live layers before changing the playlist.",
                             juce::dontSendNotification);
        return;
    }

    if (engine.getCurrentSampleRate() <= 0.0)
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        statusLabel.setText ("Start an audio device before loading files.",
                             juce::dontSendNotification);
        return;
    }

    chooserOpen = true;
    updateButtons();

    fileChooser = std::make_unique<juce::FileChooser> ("Add backing tracks",
                                                        lastDirectory,
                                                        "*.wav;*.flac;*.mp3;*.aiff;*.ogg");

    juce::Component::SafePointer<PlaylistPanel> safeThis (this);

    const auto browserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync (browserFlags, [safeThis] (const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        safeThis->chooserOpen = false;
        const auto results = chooser.getResults();

        if (results.isEmpty())
        {
            safeThis->updateButtons();
            return;
        }

        safeThis->lastDirectory = results[0].getParentDirectory();
        safeThis->beginLoading (results);
    });
}

void PlaylistPanel::beginLoading (const juce::Array<juce::File>& files)
{
    if (loading || files.isEmpty() || loader.isBusy())
        return;

    if (! engine.getTransport().isStopped())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Stop the transport before adding files.",
                             juce::dontSendNotification);
        return;
    }

    if (engine.hasLiveLayers())
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
        statusLabel.setText ("Clear live layers before changing the playlist.",
                             juce::dontSendNotification);
        return;
    }

    const auto sampleRate = engine.getCurrentSampleRate();

    if (sampleRate <= 0.0)
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        statusLabel.setText ("Start an audio device before loading files.",
                             juce::dontSendNotification);
        return;
    }

    loading = true;
    loadingProgress = 0.0;
    loadTotal = files.size();
    loadCompleted = 0;
    loadFailed = 0;

    emptyLabel.setVisible (false);
    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Loading 0/" + juce::String (loadTotal) + "...",
                         juce::dontSendNotification);

    updateButtons();
    repaint (loadingBarArea);

    loader.enqueueFiles (files, sampleRate);
}

void PlaylistPanel::handleLoadedItem (std::shared_ptr<PlaylistItem> item, int, int)
{
    if (item == nullptr)
        return;

    if (std::abs (item->sampleRate - engine.getCurrentSampleRate()) > 0.5)
    {
        ++loadFailed;
        loadingProgress = static_cast<double> (loadCompleted + loadFailed)
                        / juce::jmax (1, loadTotal);
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        statusLabel.setText ("Discarded " + item->name
                                 + ": the session sample rate changed while loading.",
                             juce::dontSendNotification);
        repaint (loadingBarArea);
        return;
    }

    engine.getPlaylist().addItem (item);
    engine.playlistChanged();

    ++loadCompleted;
    loadingProgress = static_cast<double> (loadCompleted + loadFailed)
                    / juce::jmax (1, loadTotal);

    selectedIndex = juce::jmax (0, lastItemCount + 1);
    refreshSnapshot();

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Loaded " + juce::String (loadCompleted)
                             + "/" + juce::String (loadTotal) + ": " + item->name,
                         juce::dontSendNotification);
    repaint (loadingBarArea);
}

void PlaylistPanel::handleFileFailed (int, int, const juce::String& error)
{
    ++loadFailed;
    loadingProgress = static_cast<double> (loadCompleted + loadFailed)
                    / juce::jmax (1, loadTotal);

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
    statusLabel.setText (error, juce::dontSendNotification);
    repaint (loadingBarArea);
}

void PlaylistPanel::handleLoadingFinished (int, int)
{
    loading = false;
    loadingProgress = 1.0;

    refreshSnapshot();

    if (loadFailed == 0)
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
        statusLabel.setText ("Loaded " + juce::String (loadCompleted) + " file(s).",
                             juce::dontSendNotification);
    }
    else
    {
        statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        statusLabel.setText ("Loaded " + juce::String (loadCompleted)
                                 + ", failed " + juce::String (loadFailed) + ".",
                             juce::dontSendNotification);
    }

    emptyLabel.setVisible (snapshot.items.empty());
    updateButtons();
    repaint (loadingBarArea);
}

void PlaylistPanel::refreshSnapshot()
{
    snapshot = engine.getPlaylist().getSnapshot();
    const auto count = static_cast<int> (snapshot.items.size());

    if (count != lastItemCount)
    {
        lastItemCount = count;

        if (count == 0)
            selectedIndex = -1;
        else if (selectedIndex < 0 || selectedIndex >= count)
            selectedIndex = count - 1;

        listBox.updateContent();

        if (selectedIndex >= 0)
            listBox.selectRow (selectedIndex, false, true);
        else
            listBox.deselectAllRows();
    }

    totalLabel.setText (juce::String (count)
                            + (count == 1 ? " file" : " files")
                            + "   |   " + formatDuration (snapshot.totalDurationSeconds),
                        juce::dontSendNotification);
    memoryLabel.setText (formatBytes (snapshot.memoryUsageBytes) + " RAM",
                         juce::dontSendNotification);

    emptyLabel.setVisible (snapshot.items.empty() && ! loading);

    updateSelectionControls();
    updateButtons();
}

void PlaylistPanel::updateButtons()
{
    const auto canEdit = engine.getTransport().isStopped()
                      && ! loading
                      && ! chooserOpen
                      && ! engine.hasLiveLayers();
    const auto count = static_cast<int> (snapshot.items.size());

    addButton.setEnabled (canEdit);
    removeButton.setEnabled (canEdit && selectedIndex >= 0);
    upButton.setEnabled (canEdit && selectedIndex > 0);
    downButton.setEnabled (canEdit && selectedIndex >= 0 && selectedIndex < count - 1);
    clearButton.setEnabled (canEdit && count > 0);

    if (backingMuteButton.getToggleState() != engine.isBackingMuted())
        backingMuteButton.setToggleState (engine.isBackingMuted(), juce::dontSendNotification);
}

void PlaylistPanel::updateSelectionControls()
{
    if (! juce::isPositiveAndBelow (selectedIndex, static_cast<int> (snapshot.items.size())))
    {
        selectionLabel.setText ("No file selected", juce::dontSendNotification);
        gainSlider.setEnabled (false);

        if (! gainSlider.isMouseButtonDown())
            gainSlider.setValue (100.0, juce::dontSendNotification);

        return;
    }

    const auto& item = snapshot.items[static_cast<size_t> (selectedIndex)];
    selectionLabel.setText (item.name, juce::dontSendNotification);
    gainSlider.setEnabled (true);

    if (! gainSlider.isMouseButtonDown())
        gainSlider.setValue (item.gain * 100.0, juce::dontSendNotification);
}

void PlaylistPanel::removeSelected()
{
    if (! engine.getTransport().isStopped() || loading || selectedIndex < 0)
        return;

    engine.getPlaylist().removeItem (selectedIndex);
    engine.playlistChanged();

    lastItemCount = -1;
    refreshSnapshot();

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Removed file from the playlist.", juce::dontSendNotification);
}

void PlaylistPanel::moveSelected (int delta)
{
    if (! engine.getTransport().isStopped() || loading || selectedIndex < 0)
        return;

    const auto newIndex = selectedIndex + delta;

    if (! juce::isPositiveAndBelow (newIndex, static_cast<int> (snapshot.items.size())))
        return;

    engine.getPlaylist().moveItem (selectedIndex, newIndex);
    engine.playlistChanged();

    selectedIndex = newIndex;
    lastItemCount = -1;
    refreshSnapshot();

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Playlist order updated.", juce::dontSendNotification);
}

void PlaylistPanel::clearPlaylist()
{
    if (! engine.getTransport().isStopped() || loading)
        return;

    engine.getPlaylist().clear();
    engine.playlistChanged();

    selectedIndex = -1;
    lastItemCount = -1;
    refreshSnapshot();

    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    statusLabel.setText ("Playlist cleared.", juce::dontSendNotification);
}

void PlaylistPanel::addFilesForTest (const juce::StringArray& paths)
{
    juce::Array<juce::File> files;

    for (const auto& path : paths)
        if (path.isNotEmpty())
            files.add (juce::File (path));

    beginLoading (files);
}

juce::String PlaylistPanel::formatDuration (double seconds)
{
    const auto totalMs = static_cast<juce::int64> (std::llround (juce::jmax (0.0, seconds) * 1000.0));
    const auto hours = totalMs / 3600000;
    const auto minutes = (totalMs / 60000) % 60;
    const auto secs = (totalMs / 1000) % 60;
    const auto ms = totalMs % 1000;

    if (hours > 0)
        return juce::String (hours) + ":"
             + juce::String (minutes).paddedLeft ('0', 2) + ":"
             + juce::String (secs).paddedLeft ('0', 2);

    return juce::String (minutes) + ":"
         + juce::String (secs).paddedLeft ('0', 2) + "."
         + juce::String (ms).paddedLeft ('0', 3);
}

juce::String PlaylistPanel::formatBytes (int bytes)
{
    if (bytes < 1024 * 1024)
        return juce::String (static_cast<double> (bytes) / 1024.0, 1) + " KB";

    return juce::String (static_cast<double> (bytes) / (1024.0 * 1024.0), 1) + " MB";
}

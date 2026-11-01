#include "AudioSettingsPanel.h"
#include "AudioEngine.h"
#include "AudioSetup.h"
#include "BufferSizeControl.h"
#include "app/DarkLookAndFeel.h"
#include "ui/LevelMeter.h"

#include <algorithm>
#include <cmath>

namespace
{
    juce::String formatSampleRate (double rate)
    {
        const auto kHz = rate / 1000.0;

        if (std::abs (kHz - std::round (kHz)) < 0.01)
            return juce::String (static_cast<int> (std::round (kHz))) + " kHz";

        return juce::String (kHz, 1) + " kHz";
    }

    void styleLabel (juce::Label& label, float height = 14.0f, bool bold = false)
    {
        DarkLookAndFeel::styleLabel (label, height, bold);
    }

    void styleFieldLabel (juce::Label& label)
    {
        label.setFont (DarkLookAndFeel::kicker (10.5f));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void styleGroup (juce::GroupComponent& group)
    {
        group.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
        group.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());
        group.setTextLabelPosition (juce::Justification::centredLeft);
        group.setOpaque (false);
    }
}

AudioSettingsPanel::AudioSettingsPanel (juce::AudioDeviceManager& manager,
                                        AudioEngine& audioEngine)
    : deviceManager (manager), engine (audioEngine)
{
    styleGroup (deviceGroup);
    styleGroup (mixGroup);

    styleLabel (titleLabel, 28.0f, true);
    styleLabel (subtitleLabel, 12.5f);
    titleLabel.setFont (DarkLookAndFeel::sans (28.0f, true)
                            .withExtraKerningFactor (0.02f));
    titleLabel.setText ("Audio", juce::dontSendNotification);
    subtitleLabel.setFont (DarkLookAndFeel::sans (12.5f));
    subtitleLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    subtitleLabel.setText ("Device, sample rate, latency and monitoring", juce::dontSendNotification);

    styleFieldLabel (backendLabel);
    styleFieldLabel (inputLabel);
    styleFieldLabel (outputLabel);
    styleFieldLabel (sampleRateLabel);
    styleFieldLabel (bufferSizeLabel);
    styleFieldLabel (inputGainLabel);
    styleFieldLabel (outputGainLabel);
    styleFieldLabel (latencyOffsetLabel);

    styleLabel (sampleRateHintLabel, 10.5f);
    styleLabel (bufferSizeHintLabel, 10.5f);
    sampleRateHintLabel.setFont (DarkLookAndFeel::mono (10.0f));
    bufferSizeHintLabel.setFont (DarkLookAndFeel::mono (10.0f));
    styleLabel (jackStatusLabel, 12.0f, true);
    styleLabel (statusLabel, 11.5f);
    statusLabel.setFont (DarkLookAndFeel::mono (11.0f));

    backendLabel.setText ("BACKEND", juce::dontSendNotification);
    inputLabel.setText ("INPUT DEVICE", juce::dontSendNotification);
    outputLabel.setText ("OUTPUT DEVICE", juce::dontSendNotification);
    sampleRateLabel.setText ("SAMPLE RATE", juce::dontSendNotification);
    sampleRateHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    sampleRateHintLabel.setJustificationType (juce::Justification::centredLeft);
    sampleRateHintLabel.setTooltip ("JACK owns its sample rate; choose it in the server or PipeWire settings.");

    bufferSizeLabel.setText ("BUFFER SIZE", juce::dontSendNotification);
    bufferSizeHintLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    bufferSizeHintLabel.setJustificationType (juce::Justification::centredLeft);
    bufferSizeHintLabel.setTooltip ("The buffer size is the number of samples in one audio callback period.");

    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    errorLabel.setFont (DarkLookAndFeel::mono (11.0f));
    errorLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
    errorLabel.setJustificationType (juce::Justification::centredLeft);
    errorLabel.setBorderSize (juce::BorderSize<int> (0, 10, 0, 10));

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (deviceGroup);
    addAndMakeVisible (mixGroup);
    addAndMakeVisible (backendLabel);
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (sampleRateLabel);
    addAndMakeVisible (sampleRateHintLabel);
    addAndMakeVisible (bufferSizeLabel);
    addAndMakeVisible (bufferSizeHintLabel);
    addAndMakeVisible (backendBox);
    addAndMakeVisible (inputBox);
    addAndMakeVisible (outputBox);
    addAndMakeVisible (sampleRateBox);
    addAndMakeVisible (bufferSizeBox);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (rescanButton);
    addAndMakeVisible (jackStatusLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (errorLabel);
    addAndMakeVisible (monitorButton);
    addAndMakeVisible (testToneButton);
    addAndMakeVisible (inputGainLabel);
    addAndMakeVisible (outputGainLabel);
    addAndMakeVisible (latencyOffsetLabel);
    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (latencyOffsetSlider);
    addAndMakeVisible (inputMeterLabel);
    addAndMakeVisible (outputMeterLabel);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);
    inputMeter.setOrientation (true);
    outputMeter.setOrientation (true);

    styleFieldLabel (inputGainLabel);
    styleFieldLabel (outputGainLabel);
    styleFieldLabel (latencyOffsetLabel);
    inputMeterLabel.setFont (DarkLookAndFeel::kicker (10.0f));
    outputMeterLabel.setFont (DarkLookAndFeel::kicker (10.0f));
    inputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());
    outputMeterLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::mutedText());

    inputGainLabel.setText ("INPUT GAIN", juce::dontSendNotification);
    outputGainLabel.setText ("OUTPUT GAIN", juce::dontSendNotification);
    latencyOffsetLabel.setText ("RECORD OFFSET", juce::dontSendNotification);
    inputMeterLabel.setText ("INPUT", juce::dontSendNotification);
    outputMeterLabel.setText ("OUTPUT", juce::dontSendNotification);

    for (auto* slider : { &inputGainSlider, &outputGainSlider })
    {
        slider->setSliderStyle (juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
        slider->setRange (-60.0, 12.0, 0.1);
        slider->setValue (0.0, juce::dontSendNotification);
        slider->setTextValueSuffix (" dB");
        slider->setColour (juce::Slider::textBoxTextColourId, DarkLookAndFeel::text());
    }

    latencyOffsetSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    latencyOffsetSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
    latencyOffsetSlider.setRange (-100.0, 100.0, 1.0);
    latencyOffsetSlider.setValue (engine.getLatencyOffsetMs(), juce::dontSendNotification);
    latencyOffsetSlider.setTextValueSuffix (" ms");
    latencyOffsetSlider.setTooltip ("Positive values compensate for more recording latency; negative for less.");
    latencyOffsetSlider.setColour (juce::Slider::textBoxTextColourId, DarkLookAndFeel::text());

    monitorButton.setToggleState (true, juce::dontSendNotification);
    engine.setMonitoringEnabled (true);
    engine.setInputGainDb (0.0f);
    engine.setOutputGainDb (0.0f);

    backendBox.onChange = [this]
    {
        if (updatingControls)
            return;

        const auto requestedType = backendBox.getText();
        if (requestedType.isEmpty() || requestedType == deviceManager.getCurrentAudioDeviceType())
            return;

        const auto previousType = deviceManager.getCurrentAudioDeviceType();
        const auto previousSetup = deviceManager.getAudioDeviceSetup();

        updatingControls = true;
        deviceManager.setCurrentAudioDeviceType (requestedType, true);
        updatingControls = false;

        if (deviceManager.getCurrentAudioDevice() == nullptr)
        {
            const auto error = "Could not open the " + requestedType + " backend.";

            if (previousType.isNotEmpty())
            {
                updatingControls = true;
                deviceManager.setCurrentAudioDeviceType (previousType, true);
                deviceManager.setAudioDeviceSetup (previousSetup, true);
                updatingControls = false;
            }

            showError (error);
            rebuildBackendList();
            rebuildDeviceLists();
            rebuildSampleRateList();
            rebuildBufferSizeList();
            updateStatus();
            return;
        }

        rebuildDeviceLists();
        rebuildSampleRateList();
        rebuildBufferSizeList();
        applyDeviceSelection();
    };

    applyButton.onClick = [this] { applyDeviceSelection(); };
    DarkLookAndFeel::setButtonVariant (applyButton, "primary");
    DarkLookAndFeel::setButtonIcon (applyButton, "check");
    DarkLookAndFeel::setButtonTextSize (applyButton, 12.5f);
    applyButton.setTooltip ("Apply the selected device, sample rate and buffer size");

    rescanButton.onClick = [this]
    {
        rebuildDeviceLists();
        rebuildSampleRateList();
        rebuildBufferSizeList();
        updateStatus();
    };
    DarkLookAndFeel::setButtonVariant (rescanButton, "ghost");
    DarkLookAndFeel::setButtonIcon (rescanButton, "loop");
    DarkLookAndFeel::setButtonTextSize (rescanButton, 12.5f);
    rescanButton.setTooltip ("Scan again for audio devices");

    monitorButton.onClick = [this]
    {
        engine.setMonitoringEnabled (monitorButton.getToggleState());
    };

    testToneButton.onClick = [this]
    {
        engine.setTestToneEnabled (testToneButton.getToggleState());
    };

    inputGainSlider.onValueChange = [this]
    {
        engine.setInputGainDb (static_cast<float> (inputGainSlider.getValue()));
    };

    outputGainSlider.onValueChange = [this]
    {
        engine.setOutputGainDb (static_cast<float> (outputGainSlider.getValue()));
    };

    latencyOffsetSlider.onValueChange = [this]
    {
        engine.setLatencyOffsetMs (static_cast<float> (latencyOffsetSlider.getValue()));
    };

    rebuildBackendList();
    rebuildDeviceLists();
    rebuildSampleRateList();
    rebuildBufferSizeList();
    updateStatus();

    startTimerHz (30);

   #if JUCE_DEBUG
    const auto testBufferSize = juce::SystemStats::getEnvironmentVariable ("PANLOOP_TEST_BUFFER_SIZE", {});

    if (testBufferSize.isNotEmpty())
    {
        juce::Component::SafePointer<AudioSettingsPanel> safeThis (this);

        juce::Timer::callAfterDelay (2000, [safeThis, testBufferSize]
        {
            if (safeThis == nullptr)
                return;

            const auto error = BufferSizeControl::apply (testBufferSize.getIntValue());
            juce::Logger::writeToLog ("Test buffer size " + testBufferSize + ": "
                                      + (error.isEmpty() ? "ok" : error));
            safeThis->rebuildBufferSizeList();
            safeThis->updateStatus();
        });
    }
   #endif
}

void AudioSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto header = area.removeFromTop (44);
    auto titleArea = header.removeFromLeft (juce::jmin (700, header.getWidth()));
    titleLabel.setBounds (titleArea.removeFromTop (30));
    subtitleLabel.setBounds (titleArea);

    area.removeFromTop (10);

    auto errorArea = area.removeFromBottom (24);
    errorLabel.setBounds (errorArea);
    area.removeFromBottom (8);

    const auto available = area.getHeight();
    const auto deviceHeight = juce::jlimit (206, 260,
                                            juce::roundToInt (static_cast<float> (available) * 0.50f));
    const auto mixHeight = juce::jmax (0, available - deviceHeight - 10);

    deviceGroup.setBounds (area.removeFromTop (deviceHeight));
    area.removeFromTop (10);
    mixGroup.setBounds (area.removeFromTop (mixHeight));

    {
        auto inner = deviceGroup.getLocalBounds().reduced (16, 14);
        inner.translate (deviceGroup.getX(), deviceGroup.getY());
        inner.removeFromTop (10);

        constexpr int rowsHeight = 110;
        constexpr int statusHeight = 50;
        const auto topPad = juce::jmax (0, (inner.getHeight() - rowsHeight - statusHeight - 8) / 2);
        inner.removeFromTop (topPad);

        auto columns = inner.removeFromTop (rowsHeight);
        auto left = columns.removeFromLeft (columns.getWidth() / 2 - 10);
        columns.removeFromLeft (20);
        auto right = columns;

        auto row = left.removeFromTop (30);
        backendLabel.setBounds (row.removeFromLeft (80));
        rescanButton.setBounds (row.removeFromRight (96));
        row.removeFromRight (8);
        backendBox.setBounds (row);

        left.removeFromTop (10);
        row = left.removeFromTop (30);
        inputLabel.setBounds (row.removeFromLeft (80));
        row.removeFromLeft (8);
        inputBox.setBounds (row);

        left.removeFromTop (10);
        row = left.removeFromTop (30);
        outputLabel.setBounds (row.removeFromLeft (80));
        row.removeFromLeft (8);
        outputBox.setBounds (row);

        row = right.removeFromTop (30);
        sampleRateLabel.setBounds (row.removeFromLeft (84));
        sampleRateBox.setBounds (row.removeFromLeft (juce::jmin (150, row.getWidth())));
        row.removeFromLeft (8);
        sampleRateHintLabel.setBounds (row);

        right.removeFromTop (10);
        row = right.removeFromTop (30);
        bufferSizeLabel.setBounds (row.removeFromLeft (84));
        bufferSizeBox.setBounds (row.removeFromLeft (juce::jmin (150, row.getWidth())));
        row.removeFromLeft (8);
        bufferSizeHintLabel.setBounds (row);

        right.removeFromTop (10);
        row = right.removeFromTop (30);
        applyButton.setBounds (row.removeFromRight (110));
        row.removeFromRight (8);
        jackStatusLabel.setBounds (row);

        inner.removeFromTop (8);
        statusLabel.setBounds (inner.removeFromTop (statusHeight));
    }

    {
        auto inner = mixGroup.getLocalBounds().reduced (16, 14);
        inner.translate (mixGroup.getX(), mixGroup.getY());
        inner.removeFromTop (10);

        auto meters = inner.removeFromRight (juce::jmin (190, inner.getWidth() / 3));
        inner.removeFromRight (24);

        auto controls = inner;
        constexpr int controlsHeight = 146;
        controls.removeFromTop (juce::jmax (0, (controls.getHeight() - controlsHeight) / 2));

        auto row = controls.removeFromTop (30);
        monitorButton.setBounds (row.removeFromLeft (juce::jmin (190, row.getWidth() / 2)));
        testToneButton.setBounds (row.removeFromLeft (juce::jmin (200, row.getWidth())));

        controls.removeFromTop (10);
        row = controls.removeFromTop (30);
        inputGainLabel.setBounds (row.removeFromLeft (96));
        inputGainSlider.setBounds (row.removeFromLeft (juce::jmin (360, row.getWidth())));

        controls.removeFromTop (8);
        row = controls.removeFromTop (30);
        outputGainLabel.setBounds (row.removeFromLeft (96));
        outputGainSlider.setBounds (row.removeFromLeft (juce::jmin (360, row.getWidth())));

        controls.removeFromTop (8);
        row = controls.removeFromTop (30);
        latencyOffsetLabel.setBounds (row.removeFromLeft (96));
        latencyOffsetSlider.setBounds (row.removeFromLeft (juce::jmin (360, row.getWidth())));

        const auto meterBlockHeight = juce::jmin (250, meters.getHeight());
        auto meterBlock = meters.withSizeKeepingCentre (meters.getWidth(), meterBlockHeight);
        auto labelsRow = meterBlock.removeFromTop (18);
        meterBlock.removeFromTop (6);

        constexpr int barWidth = 44;
        constexpr int barGap = 20;
        const auto barsWidth = barWidth * 2 + barGap;
        const auto barsX = meterBlock.getX() + (meterBlock.getWidth() - barsWidth) / 2;

        inputMeterLabel.setBounds (barsX, labelsRow.getY(), barWidth, 18);
        outputMeterLabel.setBounds (barsX + barWidth + barGap, labelsRow.getY(), barWidth, 18);
        inputMeter.setBounds (barsX, meterBlock.getY(), barWidth, meterBlock.getHeight());
        outputMeter.setBounds (barsX + barWidth + barGap, meterBlock.getY(),
                               barWidth, meterBlock.getHeight());
    }
}

void AudioSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());

    if (errorLabel.getText().isNotEmpty())
    {
        auto alert = errorLabel.getBounds().toFloat().expanded (4.0f, 2.0f);
        g.setColour (DarkLookAndFeel::recording().withAlpha (0.10f));
        g.fillRoundedRectangle (alert, 7.0f);
        g.setColour (DarkLookAndFeel::recording().withAlpha (0.30f));
        g.drawRoundedRectangle (alert.reduced (0.5f), 7.0f, 1.0f);
    }
}

void AudioSettingsPanel::refreshFromManager()
{
    rebuildBackendList();
    rebuildDeviceLists();
    rebuildSampleRateList();
    rebuildBufferSizeList();
    updateStatus();
}

void AudioSettingsPanel::timerCallback()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    const auto bufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;

    if (bufferSize != lastObservedBufferSize)
    {
        lastObservedBufferSize = bufferSize;
        rebuildBufferSizeList();
    }

    updateStatus();
    updateMeters();

    if (monitorButton.getToggleState() != engine.isMonitoringEnabled())
        monitorButton.setToggleState (engine.isMonitoringEnabled(), juce::dontSendNotification);

    if (engine.hasClipped())
    {
        clipHoldTicks = 60;
        engine.resetClipIndicator();
    }
    else if (clipHoldTicks > 0)
    {
        --clipHoldTicks;
    }

    if (clipHoldTicks > 0)
    {
        errorLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        errorLabel.setText ("CLIP - reduce input/output gain", juce::dontSendNotification);
    }
    else if (errorLabel.getText().startsWith ("CLIP"))
    {
        errorLabel.setText ({}, juce::dontSendNotification);
    }
}

void AudioSettingsPanel::rebuildBackendList()
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    backendBox.clear (juce::dontSendNotification);

    int itemId = 1;
    for (auto* type : deviceManager.getAvailableDeviceTypes())
        backendBox.addItem (type->getTypeName(), itemId++);

    auto currentType = deviceManager.getCurrentAudioDeviceType();

    if (currentType.isEmpty() && backendBox.getNumItems() > 0)
        currentType = backendBox.getItemText (0);

    backendBox.setText (currentType, juce::dontSendNotification);
}

void AudioSettingsPanel::rebuildDeviceLists()
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    auto* type = deviceManager.getCurrentDeviceTypeObject();

    if (type == nullptr)
    {
        inputBox.clear (juce::dontSendNotification);
        outputBox.clear (juce::dontSendNotification);
        inputBox.setEnabled (false);
        outputBox.setEnabled (false);
        return;
    }

    type->scanForDevices();

    const auto inputs = type->getDeviceNames (true);
    const auto outputs = type->getDeviceNames (false);
    const auto setup = deviceManager.getAudioDeviceSetup();

    inputBox.clear (juce::dontSendNotification);
    outputBox.clear (juce::dontSendNotification);

    for (int i = 0; i < inputs.size(); ++i)
        inputBox.addItem (inputs[i], i + 1);

    for (int i = 0; i < outputs.size(); ++i)
        outputBox.addItem (outputs[i], i + 1);

    auto inputName = setup.inputDeviceName;
    auto outputName = setup.outputDeviceName;

    if (inputName.isEmpty() && ! inputs.isEmpty())
        inputName = inputs[0];

    if (outputName.isEmpty() && ! outputs.isEmpty())
        outputName = outputs[0];

    inputBox.setText (inputName, juce::dontSendNotification);
    outputBox.setText (outputName, juce::dontSendNotification);

    inputBox.setEnabled (! inputs.isEmpty());
    outputBox.setEnabled (! outputs.isEmpty());
}

void AudioSettingsPanel::rebuildSampleRateList()
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    auto* device = deviceManager.getCurrentAudioDevice();
    const auto type = deviceManager.getCurrentAudioDeviceType();
    const auto isJack = type.equalsIgnoreCase ("JACK");

    juce::Array<double> availableRates;

    if (device != nullptr)
        availableRates = device->getAvailableSampleRates();

    if (availableRates.isEmpty())
    {
        for (const auto rate : { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
            availableRates.add (rate);
    }

    std::vector<double> uniqueRates;

    for (const auto rate : availableRates)
    {
        if (rate <= 0.0)
            continue;

        if (std::find (uniqueRates.begin(), uniqueRates.end(), rate) == uniqueRates.end())
            uniqueRates.push_back (rate);
    }

    std::sort (uniqueRates.begin(), uniqueRates.end());

    const auto setup = deviceManager.getAudioDeviceSetup();
    const auto currentRate = setup.sampleRate > 0.0
                               ? setup.sampleRate
                               : (device != nullptr ? device->getCurrentSampleRate() : 0.0);

    if (currentRate > 0.0
        && std::find (uniqueRates.begin(), uniqueRates.end(), currentRate) == uniqueRates.end())
    {
        uniqueRates.push_back (currentRate);
        std::sort (uniqueRates.begin(), uniqueRates.end());
    }

    sampleRates = uniqueRates;
    sampleRateBox.clear (juce::dontSendNotification);
    sampleRateBox.addItem ("Default", 1);

    int selectedId = 1;

    for (int i = 0; i < static_cast<int> (sampleRates.size()); ++i)
    {
        sampleRateBox.addItem (formatSampleRate (sampleRates[static_cast<size_t> (i)]), i + 2);

        if (std::abs (sampleRates[static_cast<size_t> (i)] - currentRate) < 0.5)
            selectedId = i + 2;
    }

    sampleRateBox.setSelectedId (selectedId, juce::dontSendNotification);
    sampleRateBox.setEnabled (! isJack && ! sampleRates.empty());

    if (isJack)
        sampleRateHintLabel.setText ("JACK-owned",
                                     juce::dontSendNotification);
    else if (device != nullptr)
        sampleRateHintLabel.setText ("Choose, then Apply",
                                     juce::dontSendNotification);
    else
        sampleRateHintLabel.setText ("Default rate",
                                     juce::dontSendNotification);
}

void AudioSettingsPanel::rebuildBufferSizeList()
{
    const juce::ScopedValueSetter<bool> guard (updatingControls, true);

    auto* device = deviceManager.getCurrentAudioDevice();
    const auto type = deviceManager.getCurrentAudioDeviceType();
    const auto isJack = type.equalsIgnoreCase ("JACK");
    const auto currentSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;

    bufferSizes.clear();
    bufferSizeBox.clear (juce::dontSendNotification);

    if (isJack)
    {
        const auto info = BufferSizeControl::query();
        std::vector<int> sizes = info.choices;

        if (currentSize > 0
            && std::find (sizes.begin(), sizes.end(), currentSize) == sizes.end())
            sizes.push_back (currentSize);

        std::sort (sizes.begin(), sizes.end());
        sizes.erase (std::unique (sizes.begin(), sizes.end()), sizes.end());
        bufferSizes = sizes;

        bufferSizeBox.addItem ("Auto (server)", 1);

        for (int i = 0; i < static_cast<int> (bufferSizes.size()); ++i)
            bufferSizeBox.addItem (juce::String (bufferSizes[static_cast<size_t> (i)])
                                     + " samples", i + 2);

        const auto target = currentSize > 0 ? currentSize : info.currentSamples;
        int selectedId = 1;

        for (int i = 0; i < static_cast<int> (bufferSizes.size()); ++i)
            if (bufferSizes[static_cast<size_t> (i)] == target)
                selectedId = i + 2;

        bufferSizeBox.setSelectedId (selectedId, juce::dontSendNotification);

        if (info.provider == "PipeWire")
        {
            bufferSizeBox.setEnabled (true);
            bufferSizeHintLabel.setText ("PipeWire quantum",
                                         juce::dontSendNotification);
        }
        else if (info.jackBufferToolAvailable)
        {
            bufferSizeBox.setEnabled (true);
            bufferSizeHintLabel.setText ("jack_bufsize",
                                         juce::dontSendNotification);
        }
        else
        {
            bufferSizeBox.setEnabled (false);
            bufferSizeHintLabel.setText ("Server-owned",
                                         juce::dontSendNotification);
        }
    }
    else
    {
        bufferSizeBox.addItem ("Default", 1);

        juce::Array<int> availableSizes;

        if (device != nullptr)
            availableSizes = device->getAvailableBufferSizes();

        if (availableSizes.isEmpty())
        {
            for (const auto size : { 64, 128, 256, 512, 1024, 2048, 4096 })
                availableSizes.add (size);
        }

        std::vector<int> sizes;

        for (const auto size : availableSizes)
            if (size > 0)
                sizes.push_back (size);

        if (currentSize > 0
            && std::find (sizes.begin(), sizes.end(), currentSize) == sizes.end())
            sizes.push_back (currentSize);

        std::sort (sizes.begin(), sizes.end());
        sizes.erase (std::unique (sizes.begin(), sizes.end()), sizes.end());
        bufferSizes = sizes;

        int selectedId = 1;

        for (int i = 0; i < static_cast<int> (bufferSizes.size()); ++i)
        {
            bufferSizeBox.addItem (juce::String (bufferSizes[static_cast<size_t> (i)])
                                     + " samples", i + 2);

            if (bufferSizes[static_cast<size_t> (i)] == currentSize)
                selectedId = i + 2;
        }

        bufferSizeBox.setSelectedId (selectedId, juce::dontSendNotification);
        bufferSizeBox.setEnabled (device != nullptr);
        bufferSizeHintLabel.setText (device != nullptr
                                       ? "Choose, then Apply"
                                       : "Default buffer",
                                     juce::dontSendNotification);
    }
}

void AudioSettingsPanel::applyDeviceSelection()
{
    const auto inputName = inputBox.getText();
    const auto outputName = outputBox.getText();

    if (inputName.isEmpty() && outputName.isEmpty())
    {
        showError ("Select an input and/or output device first.");
        return;
    }

    double requestedSampleRate = 0.0;
    const auto selectedRateId = sampleRateBox.getSelectedId();

    if (selectedRateId >= 2
        && selectedRateId - 2 < static_cast<int> (sampleRates.size()))
    {
        requestedSampleRate = sampleRates[static_cast<size_t> (selectedRateId - 2)];
    }

    int requestedBufferSize = 0;
    const auto selectedBufferId = bufferSizeBox.getSelectedId();

    if (selectedBufferId >= 2
        && selectedBufferId - 2 < static_cast<int> (bufferSizes.size()))
    {
        requestedBufferSize = bufferSizes[static_cast<size_t> (selectedBufferId - 2)];
    }

    const auto isJack = deviceManager.getCurrentAudioDeviceType().equalsIgnoreCase ("JACK");

    if (isJack)
    {
        // JACK owns its graph rate/buffer. On PipeWire the quantum can still be
        // changed through the settings metadata; native JACK uses jack_bufsize.
        if (bufferSizeBox.isEnabled())
        {
            const auto bufferError = BufferSizeControl::apply (requestedBufferSize);

            if (bufferError.isNotEmpty())
            {
                showError (bufferError);
                return;
            }
        }

        requestedSampleRate = 0.0;
        requestedBufferSize = 0;
    }

    const auto error = AudioSetup::applyStereoSetup (deviceManager,
                                                     inputName,
                                                     outputName,
                                                     requestedSampleRate,
                                                     requestedBufferSize);

    if (error.isNotEmpty())
        showError (error);
    else
        showError ({});

    updateStatus();
    rebuildDeviceLists();
    rebuildSampleRateList();
    rebuildBufferSizeList();
}

void AudioSettingsPanel::updateStatus()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    const auto type = deviceManager.getCurrentAudioDeviceType();

    if (device == nullptr)
    {
        jackStatusLabel.setText ("No audio device", juce::dontSendNotification);
        jackStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
        statusLabel.setText ("Start JACK or select another backend above.", juce::dontSendNotification);

        if (! errorLabel.getText().startsWith ("CLIP"))
            errorLabel.setText ("No audio device open.", juce::dontSendNotification);

        return;
    }

    if (type.equalsIgnoreCase ("JACK"))
    {
        jackStatusLabel.setText ("JACK connected", juce::dontSendNotification);
        jackStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::playing());
    }
    else
    {
        jackStatusLabel.setText ("JACK not selected - using " + type, juce::dontSendNotification);
        jackStatusLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::armed());
    }

    const auto sampleRate = device->getCurrentSampleRate();
    const auto blockSize = device->getCurrentBufferSizeSamples();
    const auto numInputs = device->getActiveInputChannels().countNumberOfSetBits();
    const auto numOutputs = device->getActiveOutputChannels().countNumberOfSetBits();
    const auto inputLatency = device->getInputLatencyInSamples();
    const auto outputLatency = device->getOutputLatencyInSamples();
    const auto xruns = deviceManager.getXRunCount();
    const auto cpu = deviceManager.getCpuUsage() * 100.0;

    juce::String text;
    text << device->getName() << "  |  " << type << "\n"
         << juce::String (sampleRate, 1) << " Hz   "
         << blockSize << " samples   "
         << numInputs << " in / " << numOutputs << " out\n"
         << "Latency " << inputLatency << " + " << outputLatency << " samples   "
         << "xruns " << xruns << "   "
         << "CPU " << juce::String (cpu, 1) << " %";

    statusLabel.setText (text, juce::dontSendNotification);

    auto deviceError = device->getLastError();

    if (deviceError.isNotEmpty() && ! errorLabel.getText().startsWith ("CLIP"))
        errorLabel.setText (deviceError, juce::dontSendNotification);
    else if (! errorLabel.getText().startsWith ("CLIP"))
        errorLabel.setText ({}, juce::dontSendNotification);
}

void AudioSettingsPanel::updateMeters()
{
    inputMeter.setLevel (engine.getInputLevel());
    outputMeter.setLevel (engine.getOutputLevel());
}

void AudioSettingsPanel::showError (const juce::String& message)
{
    if (message.isEmpty())
    {
        if (! errorLabel.getText().startsWith ("CLIP"))
            errorLabel.setText ({}, juce::dontSendNotification);

        return;
    }

    errorLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
    errorLabel.setText (message, juce::dontSendNotification);
    juce::Logger::writeToLog ("Audio UI error: " + message);
}

#include "AudioSettingsPanel.h"
#include "AudioEngine.h"
#include "AudioSetup.h"
#include "app/DarkLookAndFeel.h"
#include "ui/LevelMeter.h"

namespace
{
    void styleLabel (juce::Label& label, float height = 14.0f, bool bold = false)
    {
        label.setFont (juce::Font (juce::FontOptions (height,
                                                      bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, DarkLookAndFeel::text());
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void styleGroup (juce::GroupComponent& group)
    {
        group.setColour (juce::GroupComponent::outlineColourId, DarkLookAndFeel::outline());
        group.setColour (juce::GroupComponent::textColourId, DarkLookAndFeel::mutedText());
        group.setTextLabelPosition (juce::Justification::centredLeft);
    }
}

AudioSettingsPanel::AudioSettingsPanel (juce::AudioDeviceManager& manager,
                                        AudioEngine& audioEngine)
    : deviceManager (manager), engine (audioEngine)
{
    styleGroup (deviceGroup);
    styleGroup (mixGroup);

    styleLabel (backendLabel, 14.0f);
    styleLabel (inputLabel, 14.0f);
    styleLabel (outputLabel, 14.0f);
    styleLabel (jackStatusLabel, 14.0f, true);
    styleLabel (statusLabel, 13.0f, true);

    backendLabel.setText ("Backend", juce::dontSendNotification);
    inputLabel.setText ("Input device", juce::dontSendNotification);
    outputLabel.setText ("Output device", juce::dontSendNotification);

    statusLabel.setJustificationType (juce::Justification::topLeft);

    errorLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    errorLabel.setColour (juce::Label::textColourId, DarkLookAndFeel::recording());
    errorLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (deviceGroup);
    addAndMakeVisible (mixGroup);
    addAndMakeVisible (backendLabel);
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (backendBox);
    addAndMakeVisible (inputBox);
    addAndMakeVisible (outputBox);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (rescanButton);
    addAndMakeVisible (jackStatusLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (errorLabel);
    addAndMakeVisible (monitorButton);
    addAndMakeVisible (testToneButton);
    addAndMakeVisible (inputGainLabel);
    addAndMakeVisible (outputGainLabel);
    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (inputMeterLabel);
    addAndMakeVisible (outputMeterLabel);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    styleLabel (inputGainLabel, 13.0f);
    styleLabel (outputGainLabel, 13.0f);
    styleLabel (inputMeterLabel, 12.0f);
    styleLabel (outputMeterLabel, 12.0f);

    inputGainLabel.setText ("Input gain", juce::dontSendNotification);
    outputGainLabel.setText ("Output gain", juce::dontSendNotification);
    inputMeterLabel.setText ("IN", juce::dontSendNotification);
    outputMeterLabel.setText ("OUT", juce::dontSendNotification);

    for (auto* slider : { &inputGainSlider, &outputGainSlider })
    {
        slider->setSliderStyle (juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
        slider->setRange (-60.0, 12.0, 0.1);
        slider->setValue (0.0, juce::dontSendNotification);
        slider->setTextValueSuffix (" dB");
        slider->setColour (juce::Slider::textBoxTextColourId, DarkLookAndFeel::text());
    }

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
            updateStatus();
            return;
        }

        rebuildDeviceLists();
        applyDeviceSelection();
    };

    applyButton.onClick = [this] { applyDeviceSelection(); };

    rescanButton.onClick = [this]
    {
        rebuildDeviceLists();
        updateStatus();
    };

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

    rebuildBackendList();
    rebuildDeviceLists();
    updateStatus();

    startTimerHz (30);
}

void AudioSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (18);

    deviceGroup.setBounds (area.removeFromTop (252));
    area.removeFromTop (14);
    mixGroup.setBounds (area.removeFromTop (240));
    area.removeFromTop (10);
    errorLabel.setBounds (area.removeFromTop (30));

    {
        auto inner = deviceGroup.getLocalBounds().reduced (14);
        inner.translate (deviceGroup.getX(), deviceGroup.getY());
        inner.removeFromTop (10);

        auto row = inner.removeFromTop (30);
        backendLabel.setBounds (row.removeFromLeft (95));
        backendBox.setBounds (row.removeFromLeft (220));
        row.removeFromLeft (10);
        rescanButton.setBounds (row.removeFromLeft (100));

        inner.removeFromTop (10);
        row = inner.removeFromTop (30);
        inputLabel.setBounds (row.removeFromLeft (95));
        inputBox.setBounds (row.removeFromLeft (320));

        inner.removeFromTop (10);
        row = inner.removeFromTop (30);
        outputLabel.setBounds (row.removeFromLeft (95));
        outputBox.setBounds (row.removeFromLeft (320));

        inner.removeFromTop (10);
        row = inner.removeFromTop (30);
        jackStatusLabel.setBounds (row.removeFromLeft (320));
        row.removeFromRight (10);
        applyButton.setBounds (row.removeFromRight (110));

        inner.removeFromTop (8);
        statusLabel.setBounds (inner.removeFromTop (46));
    }

    {
        auto inner = mixGroup.getLocalBounds().reduced (14);
        inner.translate (mixGroup.getX(), mixGroup.getY());
        inner.removeFromTop (10);

        auto meters = inner.removeFromRight (280).reduced (8, 0);
        auto controls = inner;

        auto row = controls.removeFromTop (30);
        monitorButton.setBounds (row.removeFromLeft (190));
        testToneButton.setBounds (row.removeFromLeft (200));

        controls.removeFromTop (12);
        row = controls.removeFromTop (32);
        inputGainLabel.setBounds (row.removeFromLeft (90));
        inputGainSlider.setBounds (row.removeFromLeft (juce::jmin (360, row.getWidth())));

        controls.removeFromTop (8);
        row = controls.removeFromTop (32);
        outputGainLabel.setBounds (row.removeFromLeft (90));
        outputGainSlider.setBounds (row.removeFromLeft (juce::jmin (360, row.getWidth())));

        inputMeterLabel.setBounds (meters.removeFromTop (20));
        inputMeter.setBounds (meters.removeFromTop (24));
        meters.removeFromTop (10);
        outputMeterLabel.setBounds (meters.removeFromTop (20));
        outputMeter.setBounds (meters.removeFromTop (24));
    }
}

void AudioSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::background());
}

void AudioSettingsPanel::refreshFromManager()
{
    rebuildBackendList();
    rebuildDeviceLists();
    updateStatus();
}

void AudioSettingsPanel::timerCallback()
{
    updateStatus();
    updateMeters();

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

void AudioSettingsPanel::applyDeviceSelection()
{
    const auto inputName = inputBox.getText();
    const auto outputName = outputBox.getText();

    if (inputName.isEmpty() && outputName.isEmpty())
    {
        showError ("Select an input and/or output device first.");
        return;
    }

    const auto error = AudioSetup::applyStereoSetup (deviceManager, inputName, outputName);

    if (error.isNotEmpty())
        showError (error);
    else
        showError ({});

    updateStatus();
    rebuildDeviceLists();
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

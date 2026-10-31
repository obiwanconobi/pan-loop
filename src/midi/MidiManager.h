#pragma once

#include <JuceHeader.h>

#include "MidiMapping.h"

#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

class RawMidiInput;

/** Owns the selected MIDI input, MIDI Learn mappings and action dispatch. */
class MidiManager final : private juce::MidiInputCallback,
                          private juce::AsyncUpdater
{
public:
    explicit MidiManager (juce::AudioDeviceManager& deviceManager);
    ~MidiManager() override;

    void refreshDeviceList();
    void autoSelectLiveLogic();

    juce::Array<juce::MidiDeviceInfo> getAvailableDevices() const;
    juce::String getEnabledDeviceIdentifier() const;

    void setDevice (const juce::String& identifier);
    bool selectDeviceMatching (const juce::String& nameContains);

    void startLearning (MidiAction action);
    void cancelLearning();
    std::optional<MidiAction> getLearningAction() const;
    std::optional<MidiMapping> getMapping (MidiAction action) const;
    void clearMapping (MidiAction action);

    /** Session save/load helpers. Restored mappings are also persisted globally. */
    std::vector<MidiMapping> getMappingsForSession() const;
    void restoreMappingsForSession (const std::vector<MidiMapping>& newMappings);

    /** Called on the message thread when a mapped control is pressed. */
    std::function<void(MidiAction)> onAction;

    /** Development/test helper: feed a message through the normal learn/dispatch path. */
    void injectMidiMessageForTest (const juce::MidiMessage& message);

    struct Snapshot
    {
        juce::uint64 messageCount = 0;
        juce::String lastMessage;
        juce::String lastDevice;
        double lastMessageTimeMs = 0.0;
        juce::String logText;
    };

    Snapshot getSnapshot() const;

    static juce::String describeMessage (const juce::MidiMessage&);

private:
    void handleIncomingMidiMessage (juce::MidiInput* source,
                                    const juce::MidiMessage& message) override;
    void handleAsyncUpdate() override;

    static bool messageToMapping (const juce::MidiMessage& message, MidiMapping& mapping);
    std::optional<MidiAction> findActionForMessage (const juce::MidiMessage& message) const;
    void dispatchAction (MidiAction action);
    void setMapping (const MidiMapping& mapping);

    void processIncomingMidiMessage (const juce::String& deviceName,
                                     const juce::MidiMessage& message);
    bool openRawMidiDevice (const juce::String& identifier);
    void closeRawMidiDevice();

    static juce::File getMappingFile();
    void loadMappings();
    void saveMappings() const;

    juce::AudioDeviceManager& deviceManager;

    mutable juce::CriticalSection lock;
    juce::String enabledIdentifier;
    juce::Array<juce::MidiDeviceInfo> availableDevices;

    mutable juce::CriticalSection mappingLock;
    std::vector<MidiMapping> mappings;
    std::atomic<int> learningAction { -1 };
    std::atomic<int> pendingAction { -1 };
    std::atomic<bool> mappingDirty { false };
    std::array<double, numMidiActions> lastActionTimes {};

    juce::String lastMessage;
    juce::String lastDevice;
    double lastMessageTimeMs = 0.0;
    juce::StringArray logLines;
    juce::uint64 messageCount = 0;

    std::unique_ptr<RawMidiInput> rawMidiInput;
    juce::String rawMidiDeviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiManager)
};

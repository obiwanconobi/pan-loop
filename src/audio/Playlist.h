#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

/** One decoded backing file, stored as stereo float audio at the session sample rate. */
struct PlaylistItem
{
    juce::String name;
    juce::File sourceFile;
    juce::AudioBuffer<float> audio;
    double sampleRate = 48000.0;
    std::atomic<float> gain { 1.0f };

    int getNumSamples() const noexcept             { return audio.getNumSamples(); }
    int getNumChannels() const noexcept            { return audio.getNumChannels(); }

    double getDurationSeconds() const noexcept
    {
        return sampleRate > 0.0 ? static_cast<double> (audio.getNumSamples()) / sampleRate : 0.0;
    }

    int getMemoryUsageBytes() const noexcept
    {
        return audio.getNumSamples() * audio.getNumChannels() * static_cast<int> (sizeof (float));
    }
};

/**
    Ordered backing-file sequence.

    The playlist is concatenated sample-accurately and loops as one master cycle.
    Structural edits are intended to happen while the transport is stopped. The
    read/write lock protects the sequence from the audio callback if an edit and
    playback overlap.
*/
class Playlist
{
public:
    struct ItemInfo
    {
        juce::String name;
        double durationSeconds = 0.0;
        float gain = 1.0f;
        int memoryUsageBytes = 0;
    };

    struct Snapshot
    {
        std::vector<ItemInfo> items;
        juce::int64 totalLengthSamples = 0;
        double totalDurationSeconds = 0.0;
        int memoryUsageBytes = 0;
    };

    Playlist() = default;

    void addItem (std::shared_ptr<PlaylistItem> item);
    void removeItem (int index);
    void moveItem (int fromIndex, int toIndex);
    void clear();
    void setItemGain (int index, float gain);

    Snapshot getSnapshot() const;
    juce::int64 getTotalLengthSamples() const;
    bool isEmpty() const;

    /** Adds the playlist audio into a destination mix at the given cycle position.
        Called from the audio thread. The destination may have one or more channels. */
    void addTo (juce::AudioBuffer<float>& destination,
                int destStartSample,
                int numSamples,
                juce::int64 cyclePosition) const;

private:
    void recalculateTotals() noexcept;

    mutable juce::ReadWriteLock lock;
    std::vector<std::shared_ptr<PlaylistItem>> items;
    juce::int64 totalLengthSamples = 0;
    double totalDurationSeconds = 0.0;
    int memoryUsageBytes = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Playlist)
};

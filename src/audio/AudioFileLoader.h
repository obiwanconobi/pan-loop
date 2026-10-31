#pragma once

#include <JuceHeader.h>

#include "Playlist.h"

#include <atomic>
#include <functional>
#include <memory>

/**
    Decodes backing files on a background thread and resamples them to the
    session sample rate. Results and errors are delivered through callbacks;
    the UI is responsible for marshalling them back to the message thread.
*/
class AudioFileLoader final : private juce::Thread
{
public:
    AudioFileLoader();
    ~AudioFileLoader() override;

    void enqueueFiles (const juce::Array<juce::File>& files, double targetSampleRate);
    bool isBusy() const noexcept;

    /** Synchronous decoder used by session loading. */
    static std::shared_ptr<PlaylistItem> loadFileSynchronously (const juce::File& file,
                                                               double targetSampleRate,
                                                               juce::String& error);

    std::function<void(int index, int total, const juce::String& fileName)> onFileStarted;
    std::function<void(int index, int total, std::shared_ptr<PlaylistItem> item)> onFileLoaded;
    std::function<void(int index, int total, const juce::String& error)> onFileFailed;
    std::function<void(int loaded, int failed)> onFinished;

private:
    struct Request
    {
        juce::File file;
        double targetSampleRate = 48000.0;
        int index = 0;
        int total = 0;
    };

    void run() override;

    static std::shared_ptr<PlaylistItem> loadFile (const juce::File& file,
                                                   double targetSampleRate,
                                                   juce::String& error);

    juce::CriticalSection queueLock;
    std::vector<Request> queue;
    std::atomic<bool> busy { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileLoader)
};

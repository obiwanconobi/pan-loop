#include "AudioFileLoader.h"

#include <cmath>
#include <limits>

AudioFileLoader::AudioFileLoader()
    : juce::Thread ("Pan Loop audio file loader")
{
}

AudioFileLoader::~AudioFileLoader()
{
    stopThread (10000);
}

void AudioFileLoader::enqueueFiles (const juce::Array<juce::File>& files, double targetSampleRate)
{
    if (files.isEmpty() || targetSampleRate <= 0.0)
        return;

    {
        const juce::ScopedLock sl (queueLock);
        const auto total = files.size();

        for (int i = 0; i < files.size(); ++i)
            queue.push_back ({ files[i], targetSampleRate, i, total });
    }

    busy.store (true);

    if (! isThreadRunning())
        startThread();
}

bool AudioFileLoader::isBusy() const noexcept
{
    return busy.load (std::memory_order_relaxed);
}

std::shared_ptr<PlaylistItem> AudioFileLoader::loadFileSynchronously (const juce::File& file,
                                                                     double targetSampleRate,
                                                                     juce::String& error)
{
    return loadFile (file, targetSampleRate, error);
}

void AudioFileLoader::run()
{
    int loaded = 0;
    int failed = 0;

    for (;;)
    {
        Request request;

        {
            const juce::ScopedLock sl (queueLock);

            if (queue.empty() || threadShouldExit())
                break;

            request = queue.front();
            queue.erase (queue.begin());
        }

        if (onFileStarted != nullptr)
            onFileStarted (request.index, request.total, request.file.getFileName());

        juce::String error;
        auto item = loadFile (request.file, request.targetSampleRate, error);

        if (threadShouldExit())
            break;

        if (item != nullptr)
        {
            ++loaded;

            if (onFileLoaded != nullptr)
                onFileLoaded (request.index, request.total, std::move (item));
        }
        else
        {
            ++failed;

            if (onFileFailed != nullptr)
                onFileFailed (request.index, request.total, error);
        }
    }

    busy.store (false);
    juce::Logger::writeToLog ("Audio file loading finished: "
                              + juce::String (loaded) + " loaded, "
                              + juce::String (failed) + " failed");

    if (onFinished != nullptr)
        onFinished (loaded, failed);
}

std::shared_ptr<PlaylistItem> AudioFileLoader::loadFile (const juce::File& file,
                                                         double targetSampleRate,
                                                         juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr)
    {
        error = "Unsupported or unreadable file: " + file.getFileName();
        return {};
    }

    if (reader->lengthInSamples <= 0 || reader->numChannels == 0 || reader->sampleRate <= 0.0)
    {
        error = "File contains no audio: " + file.getFileName();
        return {};
    }

    if (reader->lengthInSamples > static_cast<juce::int64> (std::numeric_limits<int>::max()))
    {
        error = "File is too long to preload: " + file.getFileName();
        return {};
    }

    const auto sourceLength = static_cast<int> (reader->lengthInSamples);
    const auto sourceChannels = static_cast<int> (reader->numChannels);

    const auto estimatedBytes = static_cast<juce::int64> (sourceLength)
                              * static_cast<juce::int64> (sourceChannels)
                              * static_cast<juce::int64> (sizeof (float));

    if (estimatedBytes > static_cast<juce::int64> (2) * 1024 * 1024 * 1024)
    {
        error = "File is too large to preload: " + file.getFileName();
        return {};
    }

    juce::AudioBuffer<float> sourceBuffer (juce::jmin (2, sourceChannels), sourceLength);

    const auto readOk = sourceChannels == 1
                          ? reader->read (&sourceBuffer, 0, sourceLength, 0, true, false)
                          : reader->read (&sourceBuffer, 0, sourceLength, 0, true, true);

    if (! readOk)
    {
        error = "Could not decode audio: " + file.getFileName();
        return {};
    }

    juce::AudioBuffer<float> stereoSource;

    if (sourceChannels == 1)
    {
        stereoSource.setSize (2, sourceLength);
        stereoSource.copyFrom (0, 0, sourceBuffer, 0, 0, sourceLength);
        stereoSource.copyFrom (1, 0, sourceBuffer, 0, 0, sourceLength);
    }
    else
    {
        if (sourceChannels > 2)
            juce::Logger::writeToLog ("Multichannel file uses first two channels: "
                                      + file.getFileName());

        stereoSource = std::move (sourceBuffer);
    }

    auto item = std::make_shared<PlaylistItem>();
    item->name = file.getFileName();
    item->sourceFile = file;
    item->sampleRate = targetSampleRate;

    if (std::abs (reader->sampleRate - targetSampleRate) < 0.5)
    {
        item->audio = std::move (stereoSource);
    }
    else
    {
        const auto ratio = reader->sampleRate / targetSampleRate;
        const auto targetLength = juce::jmax (1, static_cast<int> (std::llround (sourceLength / ratio)));
        item->audio.setSize (2, targetLength);

        for (int channel = 0; channel < 2; ++channel)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.reset();
            interpolator.process (ratio,
                                  stereoSource.getReadPointer (channel),
                                  item->audio.getWritePointer (channel),
                                  targetLength,
                                  sourceLength,
                                  0);
        }

        juce::Logger::writeToLog ("Resampled " + file.getFileName()
                                  + " from " + juce::String (reader->sampleRate, 1)
                                  + " to " + juce::String (targetSampleRate, 1) + " Hz");
    }

    return item;
}

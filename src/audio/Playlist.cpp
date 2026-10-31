#include "Playlist.h"

void Playlist::addItem (std::shared_ptr<PlaylistItem> item)
{
    if (item == nullptr || item->getNumSamples() <= 0)
        return;

    const juce::ScopedWriteLock sl (lock);
    items.push_back (std::move (item));
    recalculateTotals();
}

void Playlist::removeItem (int index)
{
    const juce::ScopedWriteLock sl (lock);

    if (! juce::isPositiveAndBelow (index, static_cast<int> (items.size())))
        return;

    items.erase (items.begin() + index);
    recalculateTotals();
}

void Playlist::moveItem (int fromIndex, int toIndex)
{
    const juce::ScopedWriteLock sl (lock);

    if (! juce::isPositiveAndBelow (fromIndex, static_cast<int> (items.size()))
        || ! juce::isPositiveAndBelow (toIndex, static_cast<int> (items.size()))
        || fromIndex == toIndex)
        return;

    auto item = items[static_cast<size_t> (fromIndex)];
    items.erase (items.begin() + fromIndex);
    items.insert (items.begin() + toIndex, std::move (item));
}

void Playlist::clear()
{
    const juce::ScopedWriteLock sl (lock);
    items.clear();
    recalculateTotals();
}

void Playlist::setItemGain (int index, float gain)
{
    const juce::ScopedWriteLock sl (lock);

    if (juce::isPositiveAndBelow (index, static_cast<int> (items.size())))
        items[static_cast<size_t> (index)]->gain.store (juce::jlimit (0.0f, 4.0f, gain),
                                                        std::memory_order_relaxed);
}

Playlist::Snapshot Playlist::getSnapshot() const
{
    const juce::ScopedReadLock sl (lock);

    Snapshot snapshot;
    snapshot.items.reserve (items.size());
    snapshot.totalLengthSamples = totalLengthSamples;
    snapshot.totalDurationSeconds = totalDurationSeconds;
    snapshot.memoryUsageBytes = memoryUsageBytes;

    for (const auto& item : items)
    {
        if (item == nullptr)
            continue;

        snapshot.items.push_back ({ item->name,
                                     item->sourceFile,
                                     item->getDurationSeconds(),
                                     item->gain.load (std::memory_order_relaxed),
                                     item->getMemoryUsageBytes() });
    }

    return snapshot;
}

juce::int64 Playlist::getTotalLengthSamples() const
{
    const juce::ScopedReadLock sl (lock);
    return totalLengthSamples;
}

bool Playlist::isEmpty() const
{
    const juce::ScopedReadLock sl (lock);
    return items.empty();
}

void Playlist::addTo (juce::AudioBuffer<float>& destination,
                      int destStartSample,
                      int numSamples,
                      juce::int64 cyclePosition) const
{
    if (numSamples <= 0 || destination.getNumChannels() <= 0)
        return;

    const juce::ScopedReadLock sl (lock);

    if (items.empty() || totalLengthSamples <= 0)
        return;

    auto position = cyclePosition % totalLengthSamples;

    if (position < 0)
        position += totalLengthSamples;

    int itemIndex = 0;
    juce::int64 itemStart = 0;

    while (itemIndex < static_cast<int> (items.size()))
    {
        const auto itemLength = items[static_cast<size_t> (itemIndex)]->getNumSamples();

        if (position < itemStart + itemLength)
            break;

        itemStart += itemLength;
        ++itemIndex;
    }

    if (itemIndex >= static_cast<int> (items.size()))
    {
        itemIndex = 0;
        itemStart = 0;
        position = 0;
    }

    int written = 0;

    while (written < numSamples && ! items.empty())
    {
        const auto& item = *items[static_cast<size_t> (itemIndex)];
        const auto itemLength = item.getNumSamples();
        const auto offsetInItem = static_cast<int> (position - itemStart);
        const auto availableInItem = itemLength - offsetInItem;

        if (availableInItem <= 0)
        {
            itemStart += itemLength;
            ++itemIndex;

            if (itemIndex >= static_cast<int> (items.size()))
            {
                itemIndex = 0;
                itemStart = 0;
                position = 0;
            }

            continue;
        }

        const auto toCopy = juce::jmin (numSamples - written, availableInItem);
        const auto gain = item.gain.load (std::memory_order_relaxed);

        for (int channel = 0; channel < destination.getNumChannels(); ++channel)
        {
            const auto sourceChannel = juce::jmin (channel, item.getNumChannels() - 1);
            destination.addFrom (channel,
                                 destStartSample + written,
                                 item.audio,
                                 sourceChannel,
                                 offsetInItem,
                                 toCopy,
                                 gain);
        }

        written += toCopy;
        position += toCopy;

        if (position >= totalLengthSamples)
        {
            position = 0;
            itemIndex = 0;
            itemStart = 0;
        }
        else if (position >= itemStart + itemLength)
        {
            itemStart += itemLength;
            ++itemIndex;

            if (itemIndex >= static_cast<int> (items.size()))
            {
                itemIndex = 0;
                itemStart = 0;
                position = 0;
            }
        }
    }
}

void Playlist::recalculateTotals() noexcept
{
    totalLengthSamples = 0;
    totalDurationSeconds = 0.0;
    memoryUsageBytes = 0;

    for (const auto& item : items)
    {
        if (item == nullptr)
            continue;

        totalLengthSamples += item->getNumSamples();
        totalDurationSeconds += item->getDurationSeconds();
        memoryUsageBytes += item->getMemoryUsageBytes();
    }
}

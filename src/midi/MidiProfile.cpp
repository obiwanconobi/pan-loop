#include "MidiProfile.h"

#include <algorithm>

namespace MidiProfileStore
{
    namespace
    {
        constexpr int profileVersion = 1;
        constexpr const char* profileFormat = "pan-loop-midi-profiles";

        juce::String getString (const juce::var& value)
        {
            return value.isString() ? value.toString() : juce::String();
        }

        int getInt (const juce::var& value, int fallback)
        {
            return value.isInt() || value.isInt64()
                     ? static_cast<int> (value)
                     : fallback;
        }

        bool profilesMatch (const MidiProfile& profile, const juce::String& name)
        {
            return profile.name.equalsIgnoreCase (name);
        }
    }

    juce::String serialise (const std::vector<MidiProfile>& profiles)
    {
        juce::Array<juce::var> profileArray;

        for (const auto& profile : profiles)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty ("name", profile.name);
            object->setProperty ("deviceName", profile.deviceName);
            object->setProperty ("deviceIdentifier", profile.deviceIdentifier);

            juce::Array<juce::var> mappingArray;

            for (const auto& mapping : profile.mappings)
            {
                auto* mappingObject = new juce::DynamicObject();
                mappingObject->setProperty ("action", getMidiActionKey (mapping.action));
                mappingObject->setProperty ("type", midiMappingTypeToString (mapping.type));
                mappingObject->setProperty ("channel", mapping.channel);
                mappingObject->setProperty ("number", mapping.number);
                mappingArray.add (juce::var (mappingObject));
            }

            object->setProperty ("mappings", mappingArray);
            profileArray.add (juce::var (object));
        }

        auto* root = new juce::DynamicObject();
        root->setProperty ("format", profileFormat);
        root->setProperty ("version", profileVersion);
        root->setProperty ("profiles", profileArray);
        return juce::JSON::toString (juce::var (root), true);
    }

    std::vector<MidiProfile> parse (const juce::String& json)
    {
        std::vector<MidiProfile> profiles;
        const auto parsed = juce::JSON::parse (json);
        auto* root = parsed.getDynamicObject();

        if (root == nullptr
            || getString (root->getProperty ("format")) != profileFormat
            || getInt (root->getProperty ("version"), 0) != profileVersion)
            return profiles;

        auto* profileArray = root->getProperty ("profiles").getArray();

        if (profileArray == nullptr)
            return profiles;

        for (const auto& profileVar : *profileArray)
        {
            auto* object = profileVar.getDynamicObject();

            if (object == nullptr)
                continue;

            MidiProfile profile;
            profile.name = getString (object->getProperty ("name")).trim();
            profile.deviceName = getString (object->getProperty ("deviceName"));
            profile.deviceIdentifier = getString (object->getProperty ("deviceIdentifier"));

            if (profile.name.isEmpty())
                continue;

            if (auto* mappingArray = object->getProperty ("mappings").getArray())
            {
                for (const auto& mappingVar : *mappingArray)
                {
                    auto* mappingObject = mappingVar.getDynamicObject();

                    if (mappingObject == nullptr)
                        continue;

                    const auto action = midiActionFromKey (
                        getString (mappingObject->getProperty ("action")));
                    const auto type = midiMappingTypeFromString (
                        getString (mappingObject->getProperty ("type")));
                    const auto channel = getInt (mappingObject->getProperty ("channel"), 1);
                    const auto number = getInt (mappingObject->getProperty ("number"), -1);

                    if (! action.has_value()
                        || ! type.has_value()
                        || channel < 0 || channel > 16
                        || number < 0 || number > 127)
                        continue;

                    profile.mappings.push_back ({ *action, *type, channel, number });
                }
            }

            // Keep the last definition if a file contains duplicate names.
            profiles.erase (std::remove_if (profiles.begin(), profiles.end(),
                                            [&profile] (const MidiProfile& existing)
                                            {
                                                return profilesMatch (existing, profile.name);
                                            }),
                            profiles.end());

            profiles.push_back (std::move (profile));
        }

        return profiles;
    }

    std::vector<MidiProfile> load (const juce::File& file)
    {
        if (! file.existsAsFile())
            return {};

        return parse (file.loadFileAsString());
    }

    bool save (const juce::File& file, const std::vector<MidiProfile>& profiles)
    {
        file.getParentDirectory().createDirectory();
        return file.replaceWithText (serialise (profiles));
    }
}

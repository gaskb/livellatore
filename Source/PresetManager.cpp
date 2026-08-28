#include "PresetManager.h"
#include "Parameters.h"
#include "dsp/GainRider.h"

namespace livellatore
{

PresetManager::PresetManager (juce::File directoryToUse) : presetDirectory (std::move (directoryToUse))
{
    presetDirectory.createDirectory();
}

juce::File PresetManager::makeDefaultPresetDirectory (const juce::String& companyName,
                                                       const juce::String& productName)
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile (companyName)
        .getChildFile (productName)
        .getChildFile ("Presets");
}

juce::File PresetManager::fileForPreset (const juce::String& name) const
{
    return presetDirectory.getChildFile (name + ".xml");
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& file : presetDirectory.findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (file.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool PresetManager::savePreset (const juce::String& name, const juce::ValueTree& state) const
{
    if (name.isEmpty() || ! state.isValid())
        return false;

    if (auto xml = state.createXml())
        return xml->writeTo (fileForPreset (name));
    return false;
}

bool PresetManager::deletePreset (const juce::String& name) const
{
    return fileForPreset (name).deleteFile();
}

juce::ValueTree PresetManager::loadPreset (const juce::String& name) const
{
    if (auto xml = juce::XmlDocument::parse (fileForPreset (name)))
        return juce::ValueTree::fromXml (*xml);
    return {};
}

const std::vector<FactoryPreset>& PresetManager::getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        {
            "Default",
            {
                { ParamID::targetLufs, -16.0f },
                { ParamID::attack, 200.0f },
                { ParamID::release, 800.0f },
                { ParamID::gateThreshold, -60.0f },
            },
        },
        {
            "Voce",
            {
                { ParamID::targetLufs, -16.0f },
                { ParamID::attack, GainRider::presetVoice.attackMs },
                { ParamID::release, GainRider::presetVoice.releaseMs },
                { ParamID::gateThreshold, -50.0f },
            },
        },
        {
            "Master Bus",
            {
                { ParamID::targetLufs, -14.0f },
                { ParamID::attack, GainRider::presetMasterBus.attackMs },
                { ParamID::release, GainRider::presetMasterBus.releaseMs },
                { ParamID::gateThreshold, -70.0f },
            },
        },
    };
    return presets;
}

} // namespace livellatore

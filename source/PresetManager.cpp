#include "PresetManager.h"
#include "ParameterIDs.h"
#include "IntervalPresets.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : m_apvts (apvts)
{
    initFactoryPresets();
}

juce::File PresetManager::getPresetsDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Ament Audio")
               .getChildFile ("HALATION")
               .getChildFile ("Presets");
}

juce::File PresetManager::getPresetFile (const juce::String& name) const
{
    return getPresetsDirectory().getChildFile (name + ".xml");
}

void PresetManager::savePreset (const juce::String& name)
{
    auto dir = getPresetsDirectory();
    dir.createDirectory();

    auto state = m_apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->setAttribute ("presetName", name);
        xml->writeTo (getPresetFile (name));
        m_currentPresetName = name;
    }
}

void PresetManager::loadPreset (const juce::String& name)
{
    applyPreset (name);
}

void PresetManager::deletePreset (const juce::String& name)
{
    getPresetFile (name).deleteFile();
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getPresetsDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

void PresetManager::loadNextPreset()
{
    auto names = getPresetNames();
    if (names.isEmpty()) return;

    int idx = names.indexOf (m_currentPresetName);
    idx = (idx + 1) % names.size();
    loadPreset (names[idx]);
}

void PresetManager::loadPreviousPreset()
{
    auto names = getPresetNames();
    if (names.isEmpty()) return;

    int idx = names.indexOf (m_currentPresetName);
    idx = (idx - 1 + names.size()) % names.size();
    loadPreset (names[idx]);
}

void PresetManager::applyPreset (const juce::String& name)
{
    auto file = getPresetFile (name);
    if (! file.existsAsFile()) return;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            m_apvts.replaceState (state);
            m_currentPresetName = name;
        }
    }
}

void PresetManager::initFactoryPresets()
{
    auto dir = getPresetsDirectory();
    dir.createDirectory();

    // Only write factory presets if they don't already exist
    struct FactoryPreset
    {
        const char* name;
        int   numPaths;
        int   presetID;
        float bloomRate;
        float stagger;
        float spectralTilt;
        float damping;
        float chaos;
    };

    static const FactoryPreset kFactoryPresets[] =
    {
        { "Init",             4, 0, 0.0f, 0.5f,  0.0f, 0.4f, 0.0f  },
        { "Bloom - Clean",    4, 2, 0.0f, 0.8f,  0.0f, 0.3f, 0.05f },
        { "Wall of Fifths",   4, 2, 0.5f, 0.5f, -0.2f, 0.4f, 0.1f  },
        { "Shimmer Cloud",    8, 3, 0.65f,0.6f, -0.1f, 0.3f, 0.4f  },
        { "Dark Drone",       4, 1, 0.75f,0.4f, -0.8f, 0.85f,0.05f },
        { "Cluster Frost",    6, 4, 0.2f, 0.5f,  0.0f, 0.2f, 0.7f  },
    };

    for (auto& fp : kFactoryPresets)
    {
        auto file = getPresetFile (fp.name);
        if (file.existsAsFile()) continue;

        // Build a minimal XML state with the key parameters
        juce::ValueTree state (m_apvts.state.getType());
        state.setProperty ("presetName", juce::String (fp.name), nullptr);

        auto setParam = [&] (const juce::String& id, float value)
        {
            juce::ValueTree param ("PARAM");
            param.setProperty ("id", id, nullptr);
            param.setProperty ("value", value, nullptr);
            state.appendChild (param, nullptr);
        };

        setParam (ParameterIDs::globalNumPaths,       static_cast<float> (fp.numPaths));
        setParam (ParameterIDs::globalIntervalPreset, static_cast<float> (fp.presetID));
        setParam (ParameterIDs::globalBloomRate,      fp.bloomRate);
        setParam (ParameterIDs::globalStagger,        fp.stagger);
        setParam (ParameterIDs::globalSpectralTilt,   fp.spectralTilt);
        setParam (ParameterIDs::globalDamping,        fp.damping);
        setParam (ParameterIDs::globalChaos,          fp.chaos);
        setParam (ParameterIDs::globalMix,            0.7f);

        if (auto xml = state.createXml())
            xml->writeTo (file);
    }
}

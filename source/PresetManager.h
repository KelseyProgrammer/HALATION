#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

// Handles load/save/list of plugin presets.
// Presets are APVTS XML, saved to ~/Library/Audio/Presets/Ament Audio/HALATION/ on macOS.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    void savePreset    (const juce::String& name);
    void loadPreset    (const juce::String& name);
    void deletePreset  (const juce::String& name);

    juce::StringArray getPresetNames() const;

    void loadNextPreset();
    void loadPreviousPreset();

    juce::String getCurrentPresetName() const { return m_currentPresetName; }

    // Called once on construction — installs factory presets if absent.
    void initFactoryPresets();

private:
    juce::File getPresetsDirectory() const;
    juce::File getPresetFile (const juce::String& name) const;
    void       applyPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState& m_apvts;
    juce::String                        m_currentPresetName { "Init" };
};

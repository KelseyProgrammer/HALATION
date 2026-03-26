#pragma once

#include "HalationEngine.h"
#include "IntervalPresets.h"
#include "PresetManager.h"
#include "ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor,
                        public juce::AudioProcessorValueTreeState::Listener
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Called on message thread when a listened parameter changes
    void parameterChanged (const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState& getAPVTS()        { return m_apvts; }
    PresetManager&                      getPresetManager(){ return m_presetManager; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // Push the semitone array from a preset into the path params
    void applyIntervalPreset (halation::PresetID id);

    juce::AudioProcessorValueTreeState m_apvts;
    halation::HalationEngine           m_engine;
    PresetManager                      m_presetManager;

    // Guard against re-entrant parameterChanged calls when we update params programmatically
    bool m_ignoreParamChanges { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

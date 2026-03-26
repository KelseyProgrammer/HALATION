#pragma once

#include "HalationLookAndFeel.h"
#include "IntervalPresets.h"
#include "ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Radial bloom visualizer: one arm per active path, repaints at 30fps.
// Arm angle encodes path index, arm length encodes interval magnitude.
class BloomVisualizer : public juce::Component,
                        private juce::Timer
{
public:
    explicit BloomVisualizer (juce::AudioProcessorValueTreeState& apvts);
    ~BloomVisualizer() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& m_apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BloomVisualizer)
};

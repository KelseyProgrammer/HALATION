#pragma once

#include "HalationLookAndFeel.h"
#include "IntervalPresets.h"
#include "ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// One row in the path matrix: colored index dot, interval name, level fader, pan slider.
class PathRowComponent : public juce::Component,
                         public juce::AudioProcessorValueTreeState::Listener
{
public:
    PathRowComponent (int pathIndex, juce::AudioProcessorValueTreeState& apvts);
    ~PathRowComponent() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

    void parameterChanged (const juce::String& paramID, float newValue) override;

private:
    void updateSemitoneLabel (float semitones);

    int                                  m_pathIndex;
    juce::AudioProcessorValueTreeState&  m_apvts;

    juce::Label  m_intervalLabel;
    juce::Slider m_levelSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Slider m_panSlider   { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    std::unique_ptr<juce::SliderParameterAttachment> m_levelAttach;
    std::unique_ptr<juce::SliderParameterAttachment> m_panAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PathRowComponent)
};

#pragma once

#include "BloomVisualizer.h"
#include "HalationLookAndFeel.h"
#include "PathRowComponent.h"
#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <array>

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;
    void refreshPresetLabel();
    void refreshPathVisibility();

    PluginProcessor&    m_processorRef;
    HalationLookAndFeel m_lookAndFeel;

    // ── Path matrix ──────────────────────────────────────────────────────────
    std::array<std::unique_ptr<PathRowComponent>, 8> m_pathRows;
    juce::TextButton m_pathsDec { "-" };
    juce::TextButton m_pathsInc { "+" };
    juce::Label      m_pathsCountLabel;

    // ── Preset strip ─────────────────────────────────────────────────────────
    juce::TextButton m_presetPrev { "<" };
    juce::TextButton m_presetNext { ">" };
    juce::Label      m_presetLabel;

    // ── Bloom visualizer ─────────────────────────────────────────────────────
    BloomVisualizer m_bloomViz;

    // ── Global knobs (right panel) ────────────────────────────────────────────
    juce::Slider m_bloomKnob   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider m_staggerKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider m_tiltKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider m_dampingKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider m_chaosKnob   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label  m_bloomKnobLabel, m_staggerKnobLabel, m_tiltKnobLabel,
                 m_dampingKnobLabel, m_chaosKnobLabel;

    std::unique_ptr<juce::SliderParameterAttachment> m_bloomAttach;
    std::unique_ptr<juce::SliderParameterAttachment> m_staggerAttach;
    std::unique_ptr<juce::SliderParameterAttachment> m_tiltAttach;
    std::unique_ptr<juce::SliderParameterAttachment> m_dampingAttach;
    std::unique_ptr<juce::SliderParameterAttachment> m_chaosAttach;

    // ── Mix strip (bottom) ────────────────────────────────────────────────────
    juce::Slider m_mixSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Label  m_mixLabel;
    std::unique_ptr<juce::SliderParameterAttachment> m_mixAttach;

    // ── Melatonin inspector ───────────────────────────────────────────────────
    juce::TextButton m_inspectButton { "Inspect" };
    std::unique_ptr<melatonin::Inspector> m_inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

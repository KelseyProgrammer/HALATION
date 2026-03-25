#pragma once

#include "PluginProcessor.h"
#include "HalationLookAndFeel.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor&       m_processorRef;
    HalationLookAndFeel    m_lookAndFeel;

    juce::TextButton       m_inspectButton { "Inspect" };
    std::unique_ptr<melatonin::Inspector> m_inspector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

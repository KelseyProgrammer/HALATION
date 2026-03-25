#include "PluginEditor.h"
#include "HalationLookAndFeel.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), m_processorRef (p)
{
    setLookAndFeel (&m_lookAndFeel);

    addAndMakeVisible (m_inspectButton);
    m_inspectButton.onClick = [&]
    {
        if (! m_inspector)
        {
            m_inspector = std::make_unique<melatonin::Inspector> (*this);
            m_inspector->onClose = [this]() { m_inspector.reset(); };
        }
        m_inspector->setVisible (true);
    };

    setSize (680, 480);
    setResizable (false, false);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (HalationLookAndFeel::background());

    // Plugin name header
    g.setColour (HalationLookAndFeel::accentAmber());
    g.setFont (juce::Font (juce::FontOptions{}
                               .withName (juce::Font::getDefaultMonospacedFontName())
                               .withHeight (20.0f)
                               .withStyle ("Bold")));
    g.drawText ("HALATION", 20, 16, 200, 28, juce::Justification::centredLeft, false);

    // Subtitle
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (juce::Font (juce::FontOptions{}
                               .withName (juce::Font::getDefaultMonospacedFontName())
                               .withHeight (9.0f)));
    g.drawText ("FEEDBACK PITCH ECOSYSTEM", 20, 44, 260, 14,
                juce::Justification::centredLeft, false);

    // Company name (top-right)
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (juce::Font (juce::FontOptions{}
                               .withName (juce::Font::getDefaultMonospacedFontName())
                               .withHeight (9.0f)));
    g.drawText ("AMENT AUDIO", getWidth() - 120, 16, 100, 14,
                juce::Justification::centredRight, false);
    g.drawText ("v" + juce::String (VERSION), getWidth() - 120, 32, 100, 14,
                juce::Justification::centredRight, false);

    // Placeholder panel outlines (will be replaced in Phase 5 UI)
    g.setColour (HalationLookAndFeel::sectionBorder());
    // Left panel (path matrix)
    g.drawRect (12, 70, 240, 370, 1);
    // Centre panel (bloom visualizer)
    g.drawRect (260, 70, 280, 370, 1);
    // Right panel (global knobs)
    g.drawRect (548, 70, 120, 370, 1);

    // Centre placeholder text
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (9.0f);
    g.drawText ("BLOOM VISUALIZER", 260, 248, 280, 14, juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    m_inspectButton.setBounds (getWidth() - 70, getHeight() - 30, 60, 20);
}

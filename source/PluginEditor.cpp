#include "PluginEditor.h"
#include "HalationLookAndFeel.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), m_processorRef (p)
{
    setLookAndFeel (&m_lookAndFeel);

    // Preset navigation
    m_presetLabel.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (10.0f)));
    m_presetLabel.setColour (juce::Label::textColourId, HalationLookAndFeel::accentAmber());
    m_presetLabel.setJustificationType (juce::Justification::centred);
    refreshPresetLabel();

    m_presetPrev.onClick = [this]
    {
        m_processorRef.getPresetManager().loadPreviousPreset();
        refreshPresetLabel();
    };

    m_presetNext.onClick = [this]
    {
        m_processorRef.getPresetManager().loadNextPreset();
        refreshPresetLabel();
    };

    addAndMakeVisible (m_presetPrev);
    addAndMakeVisible (m_presetLabel);
    addAndMakeVisible (m_presetNext);

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

    // Left panel section label
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (juce::Font (juce::FontOptions{}
                               .withName (juce::Font::getDefaultMonospacedFontName())
                               .withHeight (9.0f)));
    g.drawText ("PATH MATRIX", 16, 76, 200, 12, juce::Justification::centredLeft, false);
    g.drawText ("PRESET", 16, 94, 200, 10, juce::Justification::centredLeft, false);

    // Centre placeholder text
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (9.0f);
    g.drawText ("BLOOM VISUALIZER", 260, 248, 280, 14, juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    // Preset nav strip — sits in the left panel just below the section header
    // Left panel: x=12, y=70, w=240
    m_presetPrev .setBounds (16,  100, 24, 22);
    m_presetLabel.setBounds (44,  100, 168, 22);
    m_presetNext .setBounds (216, 100, 24, 22);

    m_inspectButton.setBounds (getWidth() - 70, getHeight() - 30, 60, 20);
}

void PluginEditor::refreshPresetLabel()
{
    m_presetLabel.setText (m_processorRef.getPresetManager().getCurrentPresetName(),
                           juce::dontSendNotification);
}

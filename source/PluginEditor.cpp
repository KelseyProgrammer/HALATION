#include "PluginEditor.h"
#include "HalationLookAndFeel.h"

namespace
{
    // Right-panel knob layout helpers
    constexpr int kRightPanelX  = 548;
    constexpr int kKnobSlotH    = 62;   // knob(48) + label(14)
    constexpr int kKnobFirstY   = 92;

    juce::Rectangle<int> knobBounds (int slot)
    {
        return { kRightPanelX + 10, kKnobFirstY + slot * kKnobSlotH, 100, 48 };
    }

    juce::Rectangle<int> knobLabelBounds (int slot)
    {
        return { kRightPanelX, kKnobFirstY + slot * kKnobSlotH + 48, 120, 12 };
    }

    void styleKnobLabel (juce::Label& lbl, const juce::String& text)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setFont (juce::Font (juce::FontOptions{}
            .withName (juce::Font::getDefaultMonospacedFontName())
            .withHeight (8.0f)));
        lbl.setColour (juce::Label::textColourId, HalationLookAndFeel::mutedLabel());
        lbl.setJustificationType (juce::Justification::centred);
    }
}

//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      m_processorRef (p),
      m_bloomViz (p.getAPVTS())
{
    setLookAndFeel (&m_lookAndFeel);
    auto& apvts = p.getAPVTS();

    // ── Path rows ─────────────────────────────────────────────────────────────
    for (int i = 0; i < 8; ++i)
    {
        m_pathRows[static_cast<size_t> (i)] =
            std::make_unique<PathRowComponent> (i, apvts);
        addAndMakeVisible (*m_pathRows[static_cast<size_t> (i)]);
    }

    // ── Paths +/- ─────────────────────────────────────────────────────────────
    m_pathsCountLabel.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (9.0f)));
    m_pathsCountLabel.setColour (juce::Label::textColourId, HalationLookAndFeel::accentAmber());
    m_pathsCountLabel.setJustificationType (juce::Justification::centred);

    m_pathsDec.onClick = [this]
    {
        auto& av = m_processorRef.getAPVTS();
        if (auto* param = av.getParameter (ParameterIDs::globalNumPaths))
        {
            const int cur = static_cast<int> (*av.getRawParameterValue (ParameterIDs::globalNumPaths));
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (juce::jmax (2, cur - 1))));
        }
    };

    m_pathsInc.onClick = [this]
    {
        auto& av = m_processorRef.getAPVTS();
        if (auto* param = av.getParameter (ParameterIDs::globalNumPaths))
        {
            const int cur = static_cast<int> (*av.getRawParameterValue (ParameterIDs::globalNumPaths));
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (juce::jmin (8, cur + 1))));
        }
    };

    addAndMakeVisible (m_pathsDec);
    addAndMakeVisible (m_pathsCountLabel);
    addAndMakeVisible (m_pathsInc);

    // ── Preset nav ────────────────────────────────────────────────────────────
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

    // ── Bloom visualizer ─────────────────────────────────────────────────────
    addAndMakeVisible (m_bloomViz);

    // ── Global knobs ─────────────────────────────────────────────────────────
    styleKnobLabel (m_bloomKnobLabel,   "BLOOM");
    styleKnobLabel (m_staggerKnobLabel, "STAGGER");
    styleKnobLabel (m_tiltKnobLabel,    "TILT");
    styleKnobLabel (m_dampingKnobLabel, "DAMPING");
    styleKnobLabel (m_chaosKnobLabel,   "CHAOS");

    for (auto* knob : { &m_bloomKnob, &m_staggerKnob, &m_tiltKnob,
                        &m_dampingKnob, &m_chaosKnob })
    {
        knob->setColour (juce::Slider::rotarySliderFillColourId, HalationLookAndFeel::accentAmber());
        addAndMakeVisible (knob);
    }

    addAndMakeVisible (m_bloomKnobLabel);
    addAndMakeVisible (m_staggerKnobLabel);
    addAndMakeVisible (m_tiltKnobLabel);
    addAndMakeVisible (m_dampingKnobLabel);
    addAndMakeVisible (m_chaosKnobLabel);

    m_bloomAttach   = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalBloomRate),    m_bloomKnob);
    m_staggerAttach = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalStagger),      m_staggerKnob);
    m_tiltAttach    = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalSpectralTilt), m_tiltKnob);
    m_dampingAttach = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalDamping),      m_dampingKnob);
    m_chaosAttach   = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalChaos),        m_chaosKnob);

    // ── Mix strip ─────────────────────────────────────────────────────────────
    m_mixLabel.setText ("MIX", juce::dontSendNotification);
    m_mixLabel.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (8.0f)));
    m_mixLabel.setColour (juce::Label::textColourId, HalationLookAndFeel::mutedLabel());
    m_mixLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_mixLabel);

    m_mixSlider.setColour (juce::Slider::trackColourId,      HalationLookAndFeel::accentAmber().withAlpha (0.7f));
    m_mixSlider.setColour (juce::Slider::backgroundColourId, HalationLookAndFeel::mutedLabel().withAlpha (0.3f));
    m_mixSlider.setColour (juce::Slider::thumbColourId,      HalationLookAndFeel::accentAmber());
    addAndMakeVisible (m_mixSlider);
    m_mixAttach = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalMix), m_mixSlider);

    // ── Inspector ─────────────────────────────────────────────────────────────
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
    startTimerHz (10);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (HalationLookAndFeel::background());

    // ── Header ─────────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::accentAmber());
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (20.0f)
        .withStyle ("Bold")));
    g.drawText ("HALATION", 20, 16, 200, 28, juce::Justification::centredLeft, false);

    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (9.0f)));
    g.drawText ("FEEDBACK PITCH ECOSYSTEM", 20, 44, 260, 14,
                juce::Justification::centredLeft, false);

    g.setColour (HalationLookAndFeel::mutedLabel());
    g.drawText ("AMENT AUDIO", getWidth() - 120, 16, 100, 14,
                juce::Justification::centredRight, false);
    g.drawText ("v" + juce::String (VERSION), getWidth() - 120, 32, 100, 14,
                juce::Justification::centredRight, false);

    // ── Panel borders ────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::sectionBorder());
    g.drawRect (12,  70, 240, 370, 1);   // path matrix
    g.drawRect (260, 70, 280, 370, 1);   // bloom visualizer
    g.drawRect (548, 70, 120, 370, 1);   // global knobs

    // ── Section labels ───────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (8.0f)));
    g.drawText ("PATH MATRIX",  16,  76, 100, 10, juce::Justification::centredLeft,  false);
    g.drawText ("PRESET",       16,  96, 60,  10, juce::Justification::centredLeft,  false);
    g.drawText ("GLOBAL",       552, 76, 112, 10, juce::Justification::centred,       false);

    // ── Bottom strip ─────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::sectionBorder());
    g.drawRect (12, 442, 656, 28, 1);

    // Live readouts
    auto& apvts = m_processorRef.getAPVTS();
    const float bloom   = *apvts.getRawParameterValue (ParameterIDs::globalBloomRate);
    const float stagger = *apvts.getRawParameterValue (ParameterIDs::globalStagger);
    const float chaos   = *apvts.getRawParameterValue (ParameterIDs::globalChaos);

    g.setColour (HalationLookAndFeel::knobValue());
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (8.5f)));
    g.drawText ("BLOOM: "   + juce::String (bloom,   2), 244, 448, 120, 16,
                juce::Justification::centredLeft, false);
    g.drawText ("STAGGER: " + juce::String (stagger, 2), 370, 448, 130, 16,
                juce::Justification::centredLeft, false);
    g.drawText ("CHAOS: "   + juce::String (chaos,   2), 506, 448, 110, 16,
                juce::Justification::centredLeft, false);
}

//==============================================================================
void PluginEditor::resized()
{
    // ── Paths +/- (top-right of left panel) ──────────────────────────────────
    m_pathsDec       .setBounds (170, 74, 18, 16);
    m_pathsCountLabel.setBounds (190, 74, 20, 16);
    m_pathsInc       .setBounds (212, 74, 18, 16);

    // ── Preset nav ────────────────────────────────────────────────────────────
    m_presetPrev .setBounds (80,  93, 20, 18);
    m_presetLabel.setBounds (102, 93, 116, 18);
    m_presetNext .setBounds (220, 93, 20, 18);

    // ── Path rows (left panel, y=118 downward, 8 rows × 28px) ────────────────
    for (int i = 0; i < 8; ++i)
        m_pathRows[static_cast<size_t> (i)]->setBounds (16, 118 + i * 28, 228, 26);

    // ── Bloom visualizer ─────────────────────────────────────────────────────
    m_bloomViz.setBounds (261, 71, 278, 368);

    // ── Global knobs ─────────────────────────────────────────────────────────
    m_bloomKnob   .setBounds (knobBounds (0));
    m_staggerKnob .setBounds (knobBounds (1));
    m_tiltKnob    .setBounds (knobBounds (2));
    m_dampingKnob .setBounds (knobBounds (3));
    m_chaosKnob   .setBounds (knobBounds (4));

    m_bloomKnobLabel  .setBounds (knobLabelBounds (0));
    m_staggerKnobLabel.setBounds (knobLabelBounds (1));
    m_tiltKnobLabel   .setBounds (knobLabelBounds (2));
    m_dampingKnobLabel.setBounds (knobLabelBounds (3));
    m_chaosKnobLabel  .setBounds (knobLabelBounds (4));

    // ── Mix strip ─────────────────────────────────────────────────────────────
    m_mixLabel .setBounds (16,  448, 36, 18);
    m_mixSlider.setBounds (56,  450, 180, 14);

    // ── Inspector ─────────────────────────────────────────────────────────────
    m_inspectButton.setBounds (getWidth() - 70, getHeight() - 30, 60, 20);
}

//==============================================================================
void PluginEditor::timerCallback()
{
    refreshPathVisibility();
    repaint(); // refreshes live readouts in bottom strip
}

void PluginEditor::refreshPresetLabel()
{
    m_presetLabel.setText (m_processorRef.getPresetManager().getCurrentPresetName(),
                           juce::dontSendNotification);
}

void PluginEditor::refreshPathVisibility()
{
    auto& apvts   = m_processorRef.getAPVTS();
    const int n   = static_cast<int> (*apvts.getRawParameterValue (ParameterIDs::globalNumPaths));

    // Update paths count label
    m_pathsCountLabel.setText (juce::String (n), juce::dontSendNotification);

    for (int i = 0; i < 8; ++i)
        m_pathRows[static_cast<size_t> (i)]->setVisible (i < n);
}

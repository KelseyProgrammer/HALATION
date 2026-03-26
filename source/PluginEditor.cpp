#include "PluginEditor.h"
#include "HalationLookAndFeel.h"

namespace
{
    // Right panel layout constants
    constexpr int kRightPanelX  = 548;
    constexpr int kKnobSlotH    = 58;
    constexpr int kKnobFirstY   = 92;

    juce::Rectangle<int> knobBounds (int slot)
    {
        return { kRightPanelX + 10, kKnobFirstY + slot * kKnobSlotH, 100, 38 };
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
        .withHeight (10.0f)));
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

    // ── Interval preset dropdown ──────────────────────────────────────────────
    m_intervalPresetCombo.addItem ("Unison",          1);
    m_intervalPresetCombo.addItem ("Octaves",          2);
    m_intervalPresetCombo.addItem ("Fifths",           3);
    m_intervalPresetCombo.addItem ("Harmonic Series",  4);
    m_intervalPresetCombo.addItem ("Chromatic",        5);
    m_intervalPresetCombo.addItem ("Custom",           6);
    m_intervalPresetAttach = std::make_unique<juce::ComboBoxParameterAttachment> (
        *apvts.getParameter (ParameterIDs::globalIntervalPreset), m_intervalPresetCombo);
    addAndMakeVisible (m_intervalPresetCombo);

    // ── Bloom visualizer ─────────────────────────────────────────────────────
    addAndMakeVisible (m_bloomViz);

    // ── Global knobs — each with its own path color ───────────────────────────
    struct KnobSetup { juce::Slider* knob; int colorIndex; };
    for (auto [knob, ci] : { KnobSetup { &m_bloomKnob,   0 },
                              KnobSetup { &m_staggerKnob, 1 },
                              KnobSetup { &m_tiltKnob,    2 },
                              KnobSetup { &m_dampingKnob, 3 },
                              KnobSetup { &m_chaosKnob,   4 } })
    {
        const auto c = HalationLookAndFeel::pathColour (ci);
        knob->setColour (juce::Slider::rotarySliderFillColourId, c);
        knob->setColour (juce::Slider::thumbColourId,            c);
        addAndMakeVisible (knob);
    }

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

    const auto monoFont = [](float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions{}
            .withName (juce::Font::getDefaultMonospacedFontName())
            .withHeight (h)
            .withStyle (bold ? "Bold" : "Regular"));
    };

    // ── Header ────────────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::accentAmber());
    g.setFont (monoFont (20.0f, true));
    g.drawText ("HALATION", 20, 16, 200, 28, juce::Justification::centredLeft, false);

    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (monoFont (8.5f));
    g.drawText ("FEEDBACK PITCH ECOSYSTEM", 20, 43, 260, 14,
                juce::Justification::centredLeft, false);

    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (monoFont (8.0f));
    g.drawText ("AMENT AUDIO",        getWidth() - 120, 16, 108, 12,
                juce::Justification::centredRight, false);
    g.drawText ("v" + juce::String (VERSION), getWidth() - 120, 30, 108, 12,
                juce::Justification::centredRight, false);

    // ── Panel borders ─────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::sectionBorder());
    g.drawRect (12,  70, 240, 370, 1);   // path matrix
    g.drawRect (260, 70, 280, 370, 1);   // bloom visualizer
    g.drawRect (548, 70, 120, 370, 1);   // global knobs

    // ── Left panel labels ─────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (monoFont (8.0f));
    g.drawText ("PATHS", 18, 76, 60, 10, juce::Justification::centredLeft, false);

    // Column headers
    g.setFont (monoFont (7.5f));
    g.drawText ("semi",  44, 117, 38, 9, juce::Justification::centredLeft,   false);
    g.drawText ("level", 82, 117, 80, 9, juce::Justification::centredLeft,   false);
    g.drawText ("pan",  214, 117, 28, 9, juce::Justification::centredRight,  false);

    // ── Right panel: GLOBAL header ────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (monoFont (8.0f));
    g.drawText ("GLOBAL", 552, 76, 112, 10, juce::Justification::centred, false);

    // Knob labels and values (one per slot)
    const char* kKnobNames[]  = { "BLOOM", "STAGGER", "TILT", "DAMP", "CHAOS" };
    const juce::String kParamIDs[] = {
        ParameterIDs::globalBloomRate,
        ParameterIDs::globalStagger,
        ParameterIDs::globalSpectralTilt,
        ParameterIDs::globalDamping,
        ParameterIDs::globalChaos
    };

    auto& apvts = m_processorRef.getAPVTS();

    for (int i = 0; i < 5; ++i)
    {
        const int slotY = kKnobFirstY + i * kKnobSlotH;

        // Label name
        g.setColour (HalationLookAndFeel::mutedLabel());
        g.setFont (monoFont (7.5f));
        g.drawText (kKnobNames[i], kRightPanelX, slotY + 38, 120, 10,
                    juce::Justification::centred, false);

        // Current value
        const float val = *apvts.getRawParameterValue (kParamIDs[i]);
        g.setColour (HalationLookAndFeel::knobValue());
        g.setFont (monoFont (8.5f));
        g.drawText (juce::String (val, 2), kRightPanelX, slotY + 49, 120, 10,
                    juce::Justification::centred, false);
    }

    // Latency info
    const int latencySamples = m_processorRef.getLatencySamples();
    const float latencyMs    = (m_processorRef.getSampleRate() > 0.0)
                               ? static_cast<float> (latencySamples)
                                 / static_cast<float> (m_processorRef.getSampleRate()) * 1000.0f
                               : 46.4f;
    g.setColour (HalationLookAndFeel::mutedLabel());
    g.setFont (monoFont (7.5f));
    g.drawText ("LATENCY", kRightPanelX, 390, 120, 9, juce::Justification::centred, false);
    g.setColour (HalationLookAndFeel::knobValue());
    g.setFont (monoFont (8.5f));
    g.drawText (juce::String (static_cast<int> (std::round (latencyMs))) + "ms",
                kRightPanelX, 401, 120, 10, juce::Justification::centred, false);

    // ── Bottom strip ─────────────────────────────────────────────────────────
    g.setColour (HalationLookAndFeel::sectionBorder());
    g.drawRect (12, 442, 656, 28, 1);

    const float bloom   = *apvts.getRawParameterValue (ParameterIDs::globalBloomRate);
    const float stagger = *apvts.getRawParameterValue (ParameterIDs::globalStagger);
    const float chaos   = *apvts.getRawParameterValue (ParameterIDs::globalChaos);

    g.setColour (HalationLookAndFeel::knobValue());
    g.setFont (monoFont (8.5f));
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
    m_pathsDec       .setBounds (155, 73, 18, 16);
    m_pathsCountLabel.setBounds (175, 73, 20, 16);
    m_pathsInc       .setBounds (197, 73, 18, 16);

    // ── Interval preset dropdown ──────────────────────────────────────────────
    m_intervalPresetCombo.setBounds (16, 92, 228, 22);

    // ── Path rows (36px tall, 38px stride, start at y=128) ───────────────────
    for (int i = 0; i < 8; ++i)
        m_pathRows[static_cast<size_t> (i)]->setBounds (16, 128 + i * 38, 228, 36);

    // ── Bloom visualizer ─────────────────────────────────────────────────────
    m_bloomViz.setBounds (261, 71, 278, 368);

    // ── Global knobs ─────────────────────────────────────────────────────────
    m_bloomKnob  .setBounds (knobBounds (0));
    m_staggerKnob.setBounds (knobBounds (1));
    m_tiltKnob   .setBounds (knobBounds (2));
    m_dampingKnob.setBounds (knobBounds (3));
    m_chaosKnob  .setBounds (knobBounds (4));

    // ── Mix strip ─────────────────────────────────────────────────────────────
    m_mixLabel .setBounds (16,  450, 36, 16);
    m_mixSlider.setBounds (56,  452, 180, 12);

    // ── Inspector ─────────────────────────────────────────────────────────────
    m_inspectButton.setBounds (getWidth() - 70, getHeight() - 26, 58, 18);
}

//==============================================================================
void PluginEditor::timerCallback()
{
    refreshPathVisibility();
    repaint(); // redraws knob values and bottom strip readouts
}

void PluginEditor::refreshPathVisibility()
{
    auto& apvts = m_processorRef.getAPVTS();
    const int n = static_cast<int> (*apvts.getRawParameterValue (ParameterIDs::globalNumPaths));

    m_pathsCountLabel.setText (juce::String (n), juce::dontSendNotification);

    for (int i = 0; i < 8; ++i)
        m_pathRows[static_cast<size_t> (i)]->setVisible (i < n);
}

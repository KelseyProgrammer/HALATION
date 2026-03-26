#include "BloomVisualizer.h"

BloomVisualizer::BloomVisualizer (juce::AudioProcessorValueTreeState& apvts)
    : m_apvts (apvts)
{
    startTimerHz (30);
}

BloomVisualizer::~BloomVisualizer()
{
    stopTimer();
}

void BloomVisualizer::resized() {}

void BloomVisualizer::timerCallback()
{
    repaint();
}

void BloomVisualizer::paint (juce::Graphics& g)
{
    const auto  bounds    = getLocalBounds().toFloat();
    const float cx        = bounds.getCentreX();
    const float cy        = bounds.getCentreY();
    const float maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;

    g.fillAll (HalationLookAndFeel::background());

    const int   numPaths  = static_cast<int> (
        *m_apvts.getRawParameterValue (ParameterIDs::globalNumPaths));
    const float bloomRate = *m_apvts.getRawParameterValue (ParameterIDs::globalBloomRate);

    // Subtle outer glow ring
    {
        const float ringR = maxRadius * 1.06f;
        g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.05f + bloomRate * 0.10f));
        g.drawEllipse (cx - ringR, cy - ringR, ringR * 2.0f, ringR * 2.0f, 1.2f);
    }

    if (numPaths <= 0)
        return;

    // Angular half-width per wedge — fills ~70% of each slice
    const float halfAngle = juce::MathConstants<float>::pi
                            / static_cast<float> (numPaths) * 0.70f;

    for (int i = 0; i < numPaths; ++i)
    {
        const float semitones = *m_apvts.getRawParameterValue (ParameterIDs::pathSemitones (i));
        const float level     = *m_apvts.getRawParameterValue (ParameterIDs::pathLevel     (i));
        const auto  colour    = HalationLookAndFeel::pathColour (i);

        // Angle: evenly spaced, starting from top
        const float angle = -juce::MathConstants<float>::halfPi
                            + static_cast<float> (i)
                              * juce::MathConstants<float>::twoPi
                              / static_cast<float> (numPaths);

        // Arm length: semitones [-24..+24] → [20%..100%] of maxRadius
        const float normSemi = (semitones + 24.0f) / 48.0f;
        const float armLen   = maxRadius * (0.20f + normSemi * 0.80f);

        const float leftAngle  = angle - halfAngle;
        const float rightAngle = angle + halfAngle;

        // ── Filled wedge ─────────────────────────────────────────────────────
        juce::Path wedge;
        wedge.startNewSubPath (cx, cy);
        wedge.lineTo (cx + std::cos (leftAngle) * armLen,
                      cy + std::sin (leftAngle) * armLen);

        // Arc across the tip (12 segments)
        constexpr int kArcSegs = 12;
        for (int j = 1; j <= kArcSegs; ++j)
        {
            const float a = leftAngle + (rightAngle - leftAngle)
                            * static_cast<float> (j) / static_cast<float> (kArcSegs);
            wedge.lineTo (cx + std::cos (a) * armLen,
                          cy + std::sin (a) * armLen);
        }
        wedge.closeSubPath();

        const float fillAlpha = juce::jlimit (0.0f, 1.0f,
                                              0.22f + level * 0.18f + bloomRate * 0.12f);
        g.setColour (colour.withAlpha (fillAlpha));
        g.fillPath (wedge);

        // Soft inner glow on the wedge (brighter narrow fill near center)
        juce::Path innerWedge;
        const float innerLen = armLen * 0.55f;
        innerWedge.startNewSubPath (cx, cy);
        innerWedge.lineTo (cx + std::cos (leftAngle) * innerLen,
                           cy + std::sin (leftAngle) * innerLen);
        for (int j = 1; j <= kArcSegs; ++j)
        {
            const float a = leftAngle + (rightAngle - leftAngle)
                            * static_cast<float> (j) / static_cast<float> (kArcSegs);
            innerWedge.lineTo (cx + std::cos (a) * innerLen,
                               cy + std::sin (a) * innerLen);
        }
        innerWedge.closeSubPath();
        g.setColour (colour.withAlpha (fillAlpha * 0.35f));
        g.fillPath (innerWedge);

        // Tip dot
        const float tipX  = cx + std::cos (angle) * armLen;
        const float tipY  = cy + std::sin (angle) * armLen;
        const float dotR  = 4.0f + level * 3.0f + bloomRate * 2.0f;
        g.setColour (colour.withAlpha (0.85f));
        g.fillEllipse (tipX - dotR * 0.5f, tipY - dotR * 0.5f, dotR, dotR);

        // Interval name at tip
        const auto name = halation::IntervalPresets::semitoneToIntervalName (
            static_cast<int> (std::round (semitones)));
        g.setColour (colour.withAlpha (0.70f));
        g.setFont (juce::Font (juce::FontOptions{}
            .withName (juce::Font::getDefaultMonospacedFontName())
            .withHeight (8.5f)));
        g.drawText (name,
                    static_cast<int> (tipX) - 16,
                    static_cast<int> (tipY) - 10,
                    32, 11,
                    juce::Justification::centred, false);
    }

    // Center amber core — glows with bloom
    const float coreR = 6.0f + bloomRate * 6.0f;
    g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.20f + bloomRate * 0.25f));
    g.fillEllipse (cx - coreR, cy - coreR, coreR * 2.0f, coreR * 2.0f);

    g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.90f));
    g.fillEllipse (cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
}

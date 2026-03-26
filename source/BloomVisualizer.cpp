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

    // Outer glow ring — brightness tracks bloom rate
    {
        const float ringR = maxRadius * 1.08f;
        g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.06f + bloomRate * 0.14f));
        g.drawEllipse (cx - ringR, cy - ringR, ringR * 2.0f, ringR * 2.0f, 1.5f);
        const float ring2R = maxRadius * 0.6f;
        g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.04f + bloomRate * 0.06f));
        g.drawEllipse (cx - ring2R, cy - ring2R, ring2R * 2.0f, ring2R * 2.0f, 0.8f);
    }

    // Arms — one per active path
    for (int i = 0; i < numPaths; ++i)
    {
        const float semitones = *m_apvts.getRawParameterValue (ParameterIDs::pathSemitones (i));
        const float level     = *m_apvts.getRawParameterValue (ParameterIDs::pathLevel     (i));
        const auto  colour    = HalationLookAndFeel::pathColour (i);

        // Angle: evenly spaced, top-start
        const float angle = -juce::MathConstants<float>::halfPi
                            + static_cast<float> (i)
                              * juce::MathConstants<float>::twoPi
                              / static_cast<float> (numPaths);

        // Length: semitones [-24..+24] mapped to [18%..100%] of maxRadius
        const float normSemi = (semitones + 24.0f) / 48.0f;
        const float armLen   = maxRadius * (0.18f + normSemi * 0.82f);

        const float tipX = cx + std::cos (angle) * armLen;
        const float tipY = cy + std::sin (angle) * armLen;

        // Arm line — thicker at high bloom
        const float alpha = juce::jlimit (0.0f, 1.0f, 0.3f + level * 0.5f + bloomRate * 0.2f);
        g.setColour (colour.withAlpha (alpha));
        g.drawLine (cx, cy, tipX, tipY, 1.5f + bloomRate * 2.5f);

        // Tip glow dot
        const float dotR = 3.5f + level * 4.5f;
        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha + 0.15f)));
        g.fillEllipse (tipX - dotR * 0.5f, tipY - dotR * 0.5f, dotR, dotR);

        // Interval name at tip
        const auto name = halation::IntervalPresets::semitoneToIntervalName (
            static_cast<int> (std::round (semitones)));
        g.setColour (colour.withAlpha (0.65f));
        g.setFont (juce::Font (juce::FontOptions{}
            .withName (juce::Font::getDefaultMonospacedFontName())
            .withHeight (8.0f)));
        g.drawText (name,
                    static_cast<int> (tipX) - 14,
                    static_cast<int> (tipY) - 9,
                    28, 10,
                    juce::Justification::centred, false);
    }

    // Center amber core dot
    const float coreR = 5.0f + bloomRate * 5.0f;
    g.setColour (HalationLookAndFeel::accentAmber().withAlpha (0.9f));
    g.fillEllipse (cx - coreR * 0.5f, cy - coreR * 0.5f, coreR, coreR);
}

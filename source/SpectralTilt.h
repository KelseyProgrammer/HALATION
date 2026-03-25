#pragma once

#include <juce_dsp/juce_dsp.h>

namespace halation
{

// Shelving EQ applied in the feedback path per voice.
// Positive tilt = brighter per cycle, negative = darker.
// Separate damping parameter adds a 1-pole LP for tape-style rolloff.
class SpectralTilt
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Process a stereo pair of samples in place.
    void processSample (float& left, float& right);

    void setTilt    (float tilt);     // -1.0 to +1.0
    void setDamping (float damping);  // 0.0 to 1.0

private:
    void updateCoefficients();

    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using Coeffs    = juce::dsp::IIR::Coefficients<float>;

    IIRFilter m_highShelfL, m_highShelfR;
    IIRFilter m_lowShelfL,  m_lowShelfR;
    IIRFilter m_dampingL,   m_dampingR;

    double m_sampleRate { 44100.0 };
    float  m_tilt       { 0.0f };
    float  m_damping    { 0.0f };
};

} // namespace halation

#include "SpectralTilt.h"

namespace halation
{

void SpectralTilt::prepare (double sampleRate, int /*maxBlockSize*/)
{
    m_sampleRate = sampleRate;
    updateCoefficients();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 8192;
    spec.numChannels      = 1;

    m_highShelfL.prepare (spec);
    m_highShelfR.prepare (spec);
    m_lowShelfL.prepare (spec);
    m_lowShelfR.prepare (spec);
    m_dampingL.prepare (spec);
    m_dampingR.prepare (spec);

    reset();
}

void SpectralTilt::reset()
{
    m_highShelfL.reset();
    m_highShelfR.reset();
    m_lowShelfL.reset();
    m_lowShelfR.reset();
    m_dampingL.reset();
    m_dampingR.reset();
}

void SpectralTilt::setTilt (float tilt)
{
    m_tilt = tilt;
    updateCoefficients();
}

void SpectralTilt::setDamping (float damping)
{
    m_damping = damping;
    updateCoefficients();
}

void SpectralTilt::processSample (float& left, float& right)
{
    left  = m_highShelfL.processSample (left);
    right = m_highShelfR.processSample (right);
    left  = m_lowShelfL.processSample (left);
    right = m_lowShelfR.processSample (right);
    left  = m_dampingL.processSample (left);
    right = m_dampingR.processSample (right);
}

void SpectralTilt::updateCoefficients()
{
    if (m_sampleRate <= 0.0)
        return;

    // High shelf: boost/cut HF based on tilt sign
    const float tiltGainDb    = m_tilt * 6.0f;  // ±6 dB per cycle at extreme
    const float shelfFreqHz   = 4000.0f;

    auto highShelfCoeffs = Coeffs::makeHighShelf (m_sampleRate, shelfFreqHz, 0.707f,
                                                   juce::Decibels::decibelsToGain (tiltGainDb));
    *m_highShelfL.coefficients = *highShelfCoeffs;
    *m_highShelfR.coefficients = *highShelfCoeffs;

    // Low shelf: complementary cut/boost
    auto lowShelfCoeffs = Coeffs::makeLowShelf (m_sampleRate, 500.0f, 0.707f,
                                                 juce::Decibels::decibelsToGain (-tiltGainDb * 0.5f));
    *m_lowShelfL.coefficients = *lowShelfCoeffs;
    *m_lowShelfR.coefficients = *lowShelfCoeffs;

    // Damping: 1-pole LP, cutoff mapped from 20kHz (0) to ~200Hz (1)
    const float lpFreq = 20000.0f * std::pow (0.01f, m_damping);
    auto dampCoeffs = Coeffs::makeFirstOrderLowPass (m_sampleRate, lpFreq);
    *m_dampingL.coefficients = *dampCoeffs;
    *m_dampingR.coefficients = *dampCoeffs;
}

} // namespace halation

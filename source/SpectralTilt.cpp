#include "SpectralTilt.h"
#include <bit>
#include <cmath>
#include <cstdint>

namespace halation
{

namespace
{
    // Exact comparison for change detection — bitwise, so no -Wfloat-equal
    inline bool exactlyEqual (float a, float b) noexcept
    {
        return std::bit_cast<std::uint32_t> (a) == std::bit_cast<std::uint32_t> (b);
    }
}

void SpectralTilt::prepare (double sampleRate, int /*maxBlockSize*/)
{
    m_sampleRate = sampleRate;
    updateCoefficients();
    reset();
}

void SpectralTilt::reset()
{
    m_highShelfL.resetState();
    m_highShelfR.resetState();
    m_lowShelfL.resetState();
    m_lowShelfR.resetState();
    m_dampingL.resetState();
    m_dampingR.resetState();
}

void SpectralTilt::setTilt (float tilt)
{
    if (exactlyEqual (tilt, m_tilt))
        return;

    m_tilt = tilt;
    updateCoefficients();
}

void SpectralTilt::setDamping (float damping)
{
    if (exactlyEqual (damping, m_damping))
        return;

    m_damping = damping;
    updateCoefficients();
}

void SpectralTilt::processSample (float& left, float& right)
{
    left  = m_highShelfL.process (left);
    right = m_highShelfR.process (right);
    left  = m_lowShelfL.process (left);
    right = m_lowShelfR.process (right);
    left  = m_dampingL.process (left);
    right = m_dampingR.process (right);
}

void SpectralTilt::updateCoefficients()
{
    if (m_sampleRate <= 0.0)
        return;

    // High shelf: boost/cut HF based on tilt sign, ±6 dB per cycle at extreme
    const float tiltGainDb = m_tilt * 6.0f;
    setHighShelf (m_highShelfL, m_sampleRate, 4000.0f, tiltGainDb);
    m_highShelfL.copyCoeffsTo (m_highShelfR);

    // Low shelf: complementary cut/boost
    setLowShelf (m_lowShelfL, m_sampleRate, 500.0f, -tiltGainDb * 0.5f);
    m_lowShelfL.copyCoeffsTo (m_lowShelfR);

    // Damping: 1-pole LP, cutoff mapped from 20kHz (0) to ~200Hz (1)
    const float lpFreq = 20000.0f * std::pow (0.01f, m_damping);
    const float twoPi  = 6.283185307179586f;
    const float coeff  = 1.0f - std::exp (-twoPi * lpFreq
                                          / static_cast<float> (m_sampleRate));
    m_dampingL.coeff = coeff < 1.0f ? coeff : 1.0f;
    m_dampingR.coeff = m_dampingL.coeff;
}

// RBJ cookbook shelves, shelf slope S = 1 (matches the previous Q of 0.707)

void SpectralTilt::setHighShelf (Biquad& bq, double sampleRate, float freqHz, float gainDb)
{
    const float A     = std::pow (10.0f, gainDb / 40.0f);
    const float w0    = 6.283185307179586f * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (w0);
    const float sinw  = std::sin (w0);
    const float alpha = sinw * 0.5f * 1.4142135623730951f; // sqrt(2) for S = 1
    const float sqA2a = 2.0f * std::sqrt (A) * alpha;

    const float b0 =        A * ((A + 1.0f) + (A - 1.0f) * cosw + sqA2a);
    const float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
    const float b2 =        A * ((A + 1.0f) + (A - 1.0f) * cosw - sqA2a);
    const float a0 =             (A + 1.0f) - (A - 1.0f) * cosw + sqA2a;
    const float a1 =     2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
    const float a2 =             (A + 1.0f) - (A - 1.0f) * cosw - sqA2a;

    const float inv = 1.0f / a0;
    bq.b0 = b0 * inv; bq.b1 = b1 * inv; bq.b2 = b2 * inv;
    bq.a1 = a1 * inv; bq.a2 = a2 * inv;
}

void SpectralTilt::setLowShelf (Biquad& bq, double sampleRate, float freqHz, float gainDb)
{
    const float A     = std::pow (10.0f, gainDb / 40.0f);
    const float w0    = 6.283185307179586f * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (w0);
    const float sinw  = std::sin (w0);
    const float alpha = sinw * 0.5f * 1.4142135623730951f; // sqrt(2) for S = 1
    const float sqA2a = 2.0f * std::sqrt (A) * alpha;

    const float b0 =        A * ((A + 1.0f) - (A - 1.0f) * cosw + sqA2a);
    const float b1 =  2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
    const float b2 =        A * ((A + 1.0f) - (A - 1.0f) * cosw - sqA2a);
    const float a0 =             (A + 1.0f) + (A - 1.0f) * cosw + sqA2a;
    const float a1 =    -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
    const float a2 =             (A + 1.0f) + (A - 1.0f) * cosw - sqA2a;

    const float inv = 1.0f / a0;
    bq.b0 = b0 * inv; bq.b1 = b1 * inv; bq.b2 = b2 * inv;
    bq.a1 = a1 * inv; bq.a2 = a2 * inv;
}

} // namespace halation

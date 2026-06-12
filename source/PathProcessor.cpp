#include "PathProcessor.h"

namespace halation
{

PathProcessor::PathProcessor()
{
    m_delayLineL.fill (0.0f);
    m_delayLineR.fill (0.0f);
}

void PathProcessor::prepare (double sampleRate, int maxBlockSize)
{
    m_sampleRate = sampleRate;

    // DC blocker coefficient: R = 1 - (2π * 10 / sampleRate)
    m_dcCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 10.0f
                        / static_cast<float> (sampleRate));

    m_levelSmoothed.reset    (sampleRate, 0.02);
    m_panLeftSmoothed.reset  (sampleRate, 0.02);
    m_panRightSmoothed.reset (sampleRate, 0.02);
    m_delaySmoothed.reset    (sampleRate, 0.05);

    m_levelSmoothed.setCurrentAndTargetValue    (1.0f);
    m_panLeftSmoothed.setCurrentAndTargetValue  (1.0f);
    m_panRightSmoothed.setCurrentAndTargetValue (1.0f);
    m_delaySmoothed.setCurrentAndTargetValue (
        kBaseDelayMs * 0.001f * static_cast<float> (sampleRate));

    m_pitchShifterL.prepare (sampleRate, maxBlockSize);
    m_pitchShifterR.prepare (sampleRate, maxBlockSize);
    m_spectralTilt.prepare  (sampleRate, maxBlockSize);

    reset();
}

void PathProcessor::reset()
{
    m_delayLineL.fill (0.0f);
    m_delayLineR.fill (0.0f);
    m_writeHead    = 0;
    m_feedbackL    = 0.0f;
    m_feedbackR    = 0.0f;
    m_dcPrevInL    = 0.0f;
    m_dcPrevInR    = 0.0f;
    m_dcPrevOutL   = 0.0f;
    m_dcPrevOutR   = 0.0f;

    m_pitchShifterL.reset();
    m_pitchShifterR.reset();
    m_spectralTilt.reset();
}

std::pair<float, float> PathProcessor::process (float inputL, float inputR,
                                                 float bloomRate)
{
    const float delaySamples = juce::jlimit (1.0f,
                                             static_cast<float> (kMaxDelaySamples - 2),
                                             m_delaySmoothed.getNextValue());

    // Mix input with feedback, then pitch-shift
    const float shiftInL = inputL + m_feedbackL * bloomRate;
    const float shiftInR = inputR + m_feedbackR * bloomRate;

    const float shiftedL = m_pitchShifterL.processSample (shiftInL);
    const float shiftedR = m_pitchShifterR.processSample (shiftInR);

    // Write into delay line
    writeDelay (shiftedL, shiftedR);

    // Read delayed output (fractional, linear interpolation)
    float wetL = readDelayInterpolated (delaySamples, 0);
    float wetR = readDelayInterpolated (delaySamples, 1);

    // Feedback tap: spectral tilt, soft saturation, then DC blocking.
    // The tanh keeps runaway feedback musically bounded (tape-style
    // compression as the loop approaches unity gain) instead of leaving
    // it all to the output limiter.
    float fbL = wetL;
    float fbR = wetR;
    m_spectralTilt.processSample (fbL, fbR);
    fbL = std::tanh (fbL);
    fbR = std::tanh (fbR);
    m_feedbackL = dcBlock (fbL, m_dcPrevInL, m_dcPrevOutL);
    m_feedbackR = dcBlock (fbR, m_dcPrevInR, m_dcPrevOutR);

    // Apply smoothed level + pan to wet output
    const float level    = m_levelSmoothed.getNextValue();
    const float panLeft  = m_panLeftSmoothed.getNextValue();
    const float panRight = m_panRightSmoothed.getNextValue();

    return { wetL * level * panLeft, wetR * level * panRight };
}

void PathProcessor::setSemitones (float semitones)
{
    m_baseSemitones = semitones;
    updatePitchRatios();
}

void PathProcessor::setChaosOffsetCents (float centsL, float centsR)
{
    m_chaosOffCentsL = centsL;
    m_chaosOffCentsR = centsR;
    updatePitchRatios();
}

void PathProcessor::setLevel (float level)
{
    m_levelSmoothed.setTargetValue (level);
}

void PathProcessor::setPan (float pan)
{
    // Constant-power panning
    float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    m_panLeftSmoothed.setTargetValue  (std::cos (angle));
    m_panRightSmoothed.setTargetValue (std::sin (angle));
}

void PathProcessor::setSpectralTilt (float tilt)
{
    m_spectralTilt.setTilt (tilt);
}

void PathProcessor::setDamping (float damping)
{
    m_spectralTilt.setDamping (damping);
}

void PathProcessor::setDelaySamples (float delaySamples)
{
    m_delaySmoothed.setTargetValue (
        juce::jlimit (1.0f, static_cast<float> (kMaxDelaySamples - 2), delaySamples));
}

int PathProcessor::getLatencySamples() const
{
    return PitchShifter::kFFTSize;
}

// Private =====================================================================

float PathProcessor::readDelayInterpolated (float delaySamples, int channel) const
{
    const int   whole = static_cast<int> (delaySamples);
    const float frac  = delaySamples - static_cast<float> (whole);

    int read0 = m_writeHead - whole;
    if (read0 < 0)
        read0 += kMaxDelaySamples;

    int read1 = read0 - 1;
    if (read1 < 0)
        read1 += kMaxDelaySamples;

    const auto& line = (channel == 0) ? m_delayLineL : m_delayLineR;
    const float s0 = line[static_cast<size_t> (read0)];
    const float s1 = line[static_cast<size_t> (read1)];
    return s0 + frac * (s1 - s0);
}

void PathProcessor::writeDelay (float sampleL, float sampleR)
{
    m_delayLineL[static_cast<size_t> (m_writeHead)] = sampleL;
    m_delayLineR[static_cast<size_t> (m_writeHead)] = sampleR;
    ++m_writeHead;
    if (m_writeHead >= kMaxDelaySamples)
        m_writeHead = 0;
}

float PathProcessor::dcBlock (float x, float& prevIn, float& prevOut) const noexcept
{
    float y = x - prevIn + m_dcCoeff * prevOut;
    prevIn  = x;
    prevOut = y;
    return y;
}

void PathProcessor::updatePitchRatios()
{
    m_pitchShifterL.setPitchRatio (
        std::pow (2.0f, (m_baseSemitones + m_chaosOffCentsL * 0.01f) / 12.0f));
    m_pitchShifterR.setPitchRatio (
        std::pow (2.0f, (m_baseSemitones + m_chaosOffCentsR * 0.01f) / 12.0f));
}

} // namespace halation

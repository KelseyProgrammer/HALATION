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

    m_levelSmoothed.setCurrentAndTargetValue    (1.0f);
    m_panLeftSmoothed.setCurrentAndTargetValue  (1.0f);
    m_panRightSmoothed.setCurrentAndTargetValue (1.0f);

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
                                                 float bloomRate,
                                                 int   staggerDelaySamples)
{
    const int totalDelay = juce::jlimit (1, kMaxDelaySamples - 1,
                                         PitchShifter::kFFTSize + staggerDelaySamples);

    // Update pitch ratio each sample (chaos offset may change every block)
    updatePitchRatio();

    // Mix input with feedback, then pitch-shift
    const float shiftInL = inputL + m_feedbackL * bloomRate;
    const float shiftInR = inputR + m_feedbackR * bloomRate;

    const float shiftedL = m_pitchShifterL.processSample (shiftInL);
    const float shiftedR = m_pitchShifterR.processSample (shiftInR);

    // Write into delay line
    writeDelay (shiftedL, shiftedR);

    // Read delayed output
    float wetL = readDelay (totalDelay, 0);
    float wetR = readDelay (totalDelay, 1);

    // Apply spectral tilt + damping in the feedback tap, then DC-block
    float fbL = wetL;
    float fbR = wetR;
    m_spectralTilt.processSample (fbL, fbR);
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
    updatePitchRatio();
}

void PathProcessor::setChaosOffsetCents (float cents)
{
    m_chaosOffsetCents = cents;
    // ratio is updated per-sample inside process(), no need to call here
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

int PathProcessor::getLatencySamples() const
{
    return PitchShifter::kFFTSize;
}

// Private =====================================================================

float PathProcessor::readDelay (int delaySamples, int channel) const
{
    int readHead = m_writeHead - delaySamples;
    if (readHead < 0)
        readHead += kMaxDelaySamples;

    if (channel == 0)
        return m_delayLineL[static_cast<size_t> (readHead)];
    else
        return m_delayLineR[static_cast<size_t> (readHead)];
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

void PathProcessor::updatePitchRatio()
{
    const float totalSemitones = m_baseSemitones + m_chaosOffsetCents * 0.01f;
    const float ratio = std::pow (2.0f, totalSemitones / 12.0f);
    m_pitchShifterL.setPitchRatio (ratio);
    m_pitchShifterR.setPitchRatio (ratio);
}

} // namespace halation

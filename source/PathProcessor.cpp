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
    m_pitchShifterL.prepare (sampleRate, maxBlockSize);
    m_pitchShifterR.prepare (sampleRate, maxBlockSize);
    m_spectralTilt.prepare (sampleRate, maxBlockSize);
    reset();
}

void PathProcessor::reset()
{
    m_delayLineL.fill (0.0f);
    m_delayLineR.fill (0.0f);
    m_writeHead   = 0;
    m_feedbackL   = 0.0f;
    m_feedbackR   = 0.0f;
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

    // Mix input with feedback, then pitch-shift
    float shiftInL = inputL + m_feedbackL * bloomRate;
    float shiftInR = inputR + m_feedbackR * bloomRate;

    float shiftedL = m_pitchShifterL.processSample (shiftInL);
    float shiftedR = m_pitchShifterR.processSample (shiftInR);

    // Write into delay line
    writeDelay (shiftedL, shiftedR);

    // Read from delay line
    float wetL = readDelay (totalDelay, 0);
    float wetR = readDelay (totalDelay, 1);

    // Apply spectral tilt + damping in the feedback tap (not the output)
    float fbL = wetL;
    float fbR = wetR;
    m_spectralTilt.processSample (fbL, fbR);
    m_feedbackL = fbL;
    m_feedbackR = fbR;

    // Apply level + pan to the wet output
    float outL = wetL * m_level * m_panLeft;
    float outR = wetR * m_level * m_panRight;

    return { outL, outR };
}

void PathProcessor::setSemitones (float semitones)
{
    float ratio = std::pow (2.0f, semitones / 12.0f);
    m_pitchShifterL.setPitchRatio (ratio);
    m_pitchShifterR.setPitchRatio (ratio);
}

void PathProcessor::setLevel (float level)
{
    m_level = level;
}

void PathProcessor::setPan (float pan)
{
    // Constant-power panning
    float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    m_panLeft  = std::cos (angle);
    m_panRight = std::sin (angle);
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

} // namespace halation

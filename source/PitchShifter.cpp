#include "PitchShifter.h"

namespace halation
{

PitchShifter::PitchShifter()
{
    buildWindow();
}

void PitchShifter::prepare (double sampleRate, int /*maxBlockSize*/)
{
    m_sampleRate = static_cast<float> (sampleRate);
    reset();
}

void PitchShifter::reset()
{
    m_timeDomain.fill ({});
    m_freqDomain.fill ({});
    m_inputFifo.fill (0.0f);
    m_outputAccum.fill (0.0f);
    m_lastInputPhase.fill (0.0f);
    m_lastOutputPhase.fill (0.0f);
    m_fifoIndex = 0;
}

void PitchShifter::setPitchRatio (float ratio)
{
    m_pitchRatio = ratio;
}

void PitchShifter::buildWindow()
{
    for (auto i = 0; i < kFFTSize; ++i)
        m_window[static_cast<size_t> (i)] =
            0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                      * static_cast<float> (i)
                                      / static_cast<float> (kFFTSize)));
}

float PitchShifter::processSample (float input)
{
    const auto fi = static_cast<size_t> (m_fifoIndex);
    m_inputFifo[fi] = input;
    float output = m_outputAccum[fi];
    m_outputAccum[fi] = 0.0f;

    ++m_fifoIndex;
    if (m_fifoIndex >= kFFTSize)
        m_fifoIndex = 0;

    if ((m_fifoIndex % kHopSize) == 0)
        processFrame();

    return output;
}

void PitchShifter::processFrame()
{
    // Window input into time-domain complex buffer (imag = 0)
    for (auto i = 0; i < kFFTSize; ++i)
    {
        const auto src = static_cast<size_t> ((m_fifoIndex + i) % kFFTSize);
        m_timeDomain[static_cast<size_t> (i)] =
            Complex (m_inputFifo[src] * m_window[static_cast<size_t> (i)], 0.0f);
    }

    // Forward FFT
    m_fft.perform (m_timeDomain.data(), m_freqDomain.data(), false);

    // Zero the output phase accumulator for bins we write into
    std::array<Complex, kFFTSize> outputBins {};

    const float phaseExpected = juce::MathConstants<float>::twoPi
                                 * static_cast<float> (kHopSize)
                                 / static_cast<float> (kFFTSize);

    for (auto bin = 0; bin < kNumBins; ++bin)
    {
        const auto bIdx = static_cast<size_t> (bin);
        const Complex c = m_freqDomain[bIdx];
        const float   mag   = std::abs (c);
        const float   phase = std::arg (c);

        float delta = phase - m_lastInputPhase[bIdx]
                      - phaseExpected * static_cast<float> (bin);
        delta -= juce::MathConstants<float>::twoPi
                 * std::round (delta / juce::MathConstants<float>::twoPi);

        m_lastInputPhase[bIdx] = phase;

        // Map to target bin
        const int targetBin = juce::roundToInt (static_cast<float> (bin) * m_pitchRatio);
        if (targetBin >= 0 && targetBin < kNumBins)
        {
            const auto tIdx = static_cast<size_t> (targetBin);
            m_lastOutputPhase[tIdx] += phaseExpected * static_cast<float> (targetBin)
                                        + delta * m_pitchRatio;
            outputBins[tIdx] += Complex (mag * std::cos (m_lastOutputPhase[tIdx]),
                                         mag * std::sin (m_lastOutputPhase[tIdx]));
        }
    }

    // Mirror negative frequencies
    for (auto bin = kNumBins; bin < kFFTSize; ++bin)
        outputBins[static_cast<size_t> (bin)] =
            std::conj (outputBins[static_cast<size_t> (kFFTSize - bin)]);

    // Inverse FFT
    m_fft.perform (outputBins.data(), m_timeDomain.data(), true);

    // Overlap-add with normalisation
    const float scale = 1.0f / (static_cast<float> (kFFTSize) * 0.5f);
    for (auto i = 0; i < kFFTSize; ++i)
    {
        const auto dst = static_cast<size_t> ((m_fifoIndex + i) % kFFTSize);
        m_outputAccum[dst] +=
            m_timeDomain[static_cast<size_t> (i)].real()
            * m_window[static_cast<size_t> (i)]
            * scale;
    }
}

} // namespace halation

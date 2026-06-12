#include "PitchShifter.h"

namespace halation
{

namespace
{
    inline float wrapPhase (float phase) noexcept
    {
        return phase - juce::MathConstants<float>::twoPi
                         * std::round (phase / juce::MathConstants<float>::twoPi);
    }
}

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
    m_synthPhase.fill (0.0f);
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

    const float omega = juce::MathConstants<float>::twoPi
                        * static_cast<float> (kHopSize)
                        / static_cast<float> (kFFTSize);

    // ── Analysis: magnitude, phase and heterodyned phase increment per bin ──
    for (auto bin = 0; bin < kNumBins; ++bin)
    {
        const auto bIdx = static_cast<size_t> (bin);
        const Complex c = m_freqDomain[bIdx];

        m_mags[bIdx]   = std::abs (c);
        m_phases[bIdx] = std::arg (c);
        m_deltas[bIdx] = wrapPhase (m_phases[bIdx] - m_lastInputPhase[bIdx]
                                    - omega * static_cast<float> (bin));
        m_lastInputPhase[bIdx] = m_phases[bIdx];
    }

    // ── Peak picking: local maxima over two neighbours each side ────────────
    int numPeaks = 0;
    for (auto bin = 2; bin < kNumBins - 2; ++bin)
    {
        const auto b = static_cast<size_t> (bin);
        if (m_mags[b] >  m_mags[b - 1] && m_mags[b] >  m_mags[b - 2]
         && m_mags[b] >= m_mags[b + 1] && m_mags[b] >= m_mags[b + 2]
         && m_mags[b] > 1.0e-9f)
        {
            m_peakBins[static_cast<size_t> (numPeaks++)] = bin;
        }
    }

    std::array<Complex, kFFTSize> outputBins {};

    const auto synthesizeBin = [this, &outputBins] (int bin, float theta)
    {
        const int target = juce::roundToInt (static_cast<float> (bin) * m_pitchRatio);
        if (target >= 0 && target < kNumBins)
        {
            const auto bIdx = static_cast<size_t> (bin);
            outputBins[static_cast<size_t> (target)] +=
                Complex (m_mags[bIdx] * std::cos (theta),
                         m_mags[bIdx] * std::sin (theta));
        }
    };

    if (numPeaks == 0)
    {
        // No spectral structure (silence / noise floor): every bin advances
        // independently, as in a plain phase vocoder.
        for (auto bin = 0; bin < kNumBins; ++bin)
        {
            const auto bIdx = static_cast<size_t> (bin);
            const float theta = wrapPhase (m_synthPhase[bIdx]
                                           + (omega * static_cast<float> (bin)
                                              + m_deltas[bIdx]) * m_pitchRatio);
            m_synthPhase[bIdx] = theta;
            synthesizeBin (bin, theta);
        }
    }
    else
    {
        // Identity phase locking: each region around a peak rotates rigidly
        // with the peak's phase advance, preserving relative phases.
        for (int k = 0; k < numPeaks; ++k)
        {
            const int peak  = m_peakBins[static_cast<size_t> (k)];
            const auto pIdx = static_cast<size_t> (peak);

            const int regionStart = (k == 0)
                ? 0 : (m_peakBins[static_cast<size_t> (k - 1)] + peak + 1) / 2;
            const int regionEnd = (k + 1 < numPeaks)
                ? (peak + m_peakBins[static_cast<size_t> (k + 1)] + 1) / 2
                : kNumBins;

            const float thetaPeak = wrapPhase (m_synthPhase[pIdx]
                                               + (omega * static_cast<float> (peak)
                                                  + m_deltas[pIdx]) * m_pitchRatio);

            for (int bin = regionStart; bin < regionEnd; ++bin)
            {
                const auto bIdx = static_cast<size_t> (bin);
                const float theta = (bin == peak)
                    ? thetaPeak
                    : wrapPhase (thetaPeak + m_phases[bIdx] - m_phases[pIdx]);
                m_synthPhase[bIdx] = theta;
                synthesizeBin (bin, theta);
            }
        }
    }

    // Mirror negative frequencies
    for (auto bin = kNumBins; bin < kFFTSize; ++bin)
        outputBins[static_cast<size_t> (bin)] =
            std::conj (outputBins[static_cast<size_t> (kFFTSize - bin)]);

    // Inverse FFT
    m_fft.perform (outputBins.data(), m_timeDomain.data(), true);

    // Overlap-add normalisation: JUCE IFFT divides by N, analysis and synthesis
    // windows are both Hann. OLA sum of Hann² at 75% overlap = 1.5 exactly,
    // so scale = 2/3 for perfect reconstruction at unity pitch ratio.
    const float scale = 2.0f / 3.0f;
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

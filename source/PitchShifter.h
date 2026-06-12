#pragma once

#include <juce_dsp/juce_dsp.h>
#include <complex>
#include <array>

namespace halation
{

// Phase vocoder pitch shifter with identity phase locking.
// FFT size: 2048, hop: 512 (4x overlap), Hann window.
// Introduces kFFTSize samples of latency at any sample rate.
//
// Phase locking (Laroche/Dolson): spectral peaks get the standard phase
// vocoder phase advance; bins around each peak are locked to the peak's
// phase rotation, preserving vertical phase coherence between partials.
// This removes most of the metallic/phasey artifact of a plain vocoder —
// important here because artifacts compound every feedback cycle.
class PitchShifter
{
public:
    static constexpr int kFFTOrder = 11;           // 2^11 = 2048
    static constexpr int kFFTSize  = 1 << kFFTOrder;
    static constexpr int kHopSize  = kFFTSize / 4;
    static constexpr int kNumBins  = kFFTSize / 2 + 1;

    PitchShifter();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Push one sample in, get one sample out (latency = kFFTSize samples).
    float processSample (float input);

    void setPitchRatio (float ratio);   // 1.0 = unity, 2.0 = octave up

private:
    void processFrame();
    void buildWindow();

    using Complex = std::complex<float>;

    juce::dsp::FFT m_fft { kFFTOrder };

    std::array<Complex, kFFTSize> m_timeDomain  {};
    std::array<Complex, kFFTSize> m_freqDomain  {};
    std::array<float,   kFFTSize> m_window      {};
    std::array<float,   kFFTSize> m_inputFifo   {};
    std::array<float,   kFFTSize> m_outputAccum {};

    // Per-bin analysis scratch (members so the audio thread never grows the stack)
    std::array<float, kNumBins> m_lastInputPhase {};
    std::array<float, kNumBins> m_synthPhase     {};  // source-bin indexed
    std::array<float, kNumBins> m_mags           {};
    std::array<float, kNumBins> m_phases         {};
    std::array<float, kNumBins> m_deltas         {};
    std::array<int,   kNumBins> m_peakBins       {};

    int   m_fifoIndex   { 0 };
    float m_pitchRatio  { 1.0f };
    float m_sampleRate  { 44100.0f };
};

} // namespace halation

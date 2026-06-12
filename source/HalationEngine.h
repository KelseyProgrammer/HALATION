#pragma once

#include "PathProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace halation
{

// Central DSP orchestrator. The only DSP class PluginProcessor talks to.
// Owns 8 PathProcessors and manages global parameter distribution.
// All setters are called from the audio thread (processBlock) only.
class HalationEngine
{
public:
    static constexpr int kMaxPaths = 8;
    static constexpr int kMinPaths = 2;

    HalationEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setNumPaths     (int n);
    void setPathInterval (int path, float semitones);
    void setPathLevel    (int path, float level);
    void setPathPan      (int path, float pan);

    void setGlobalParameters (float bloomRate,
                              float stagger,
                              float spectralTilt,
                              float damping,
                              float chaos,
                              float mix);

    int getLatencySamples() const;

    // Smoothed per-path output level (post level/pan), safe to read from the
    // message thread — drives the bloom visualizer.
    float getPathOutputLevel (int path) const
    {
        if (path < 0 || path >= kMaxPaths)
            return 0.0f;
        return m_pathLevels[static_cast<size_t> (path)].load (std::memory_order_relaxed);
    }

private:
    void updateStaggerDelays();
    void updateChaosOffsets (int numSamples);

    std::array<PathProcessor, kMaxPaths> m_paths;
    std::array<float, kMaxPaths>         m_chaosLfoPhase {};

    // Per-path output meters, written on the audio thread, read by the UI
    std::array<std::atomic<float>, kMaxPaths> m_pathLevels {};

    int   m_numPaths { 4 };
    float m_stagger  { 0.5f };
    float m_chaos    { 0.15f };

    // Smoothed parameters (20ms ramp)
    juce::SmoothedValue<float> m_bloomSmoothed;
    juce::SmoothedValue<float> m_mixSmoothed;

    // Dry path delayed by the pitch shifter latency so dry and wet line up
    // with the latency reported to the host.
    std::array<float, PitchShifter::kFFTSize> m_dryDelayL {};
    std::array<float, PitchShifter::kFFTSize> m_dryDelayR {};
    int m_dryDelayHead { 0 };

    // Output safety limiter (-1 dBFS ceiling)
    juce::dsp::Limiter<float> m_limiter;

    double m_sampleRate { 44100.0 };
};

} // namespace halation

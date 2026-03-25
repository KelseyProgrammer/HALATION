#pragma once

#include "PathProcessor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>

namespace halation
{

// Central DSP orchestrator. The only DSP class PluginProcessor talks to.
// Owns 8 PathProcessors and manages global parameter distribution.
class HalationEngine
{
public:
    static constexpr int kMaxPaths = 8;
    static constexpr int kMinPaths = 2;

    HalationEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    // Thread-safe path count change (use a lock on message thread).
    void setNumPaths    (int n);
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

private:
    void updateStaggerDelays();

    std::array<PathProcessor, kMaxPaths> m_paths;
    std::array<int, kMaxPaths>           m_staggerDelaySamples {};
    std::array<float, kMaxPaths>         m_chaosLfoPhase {};

    int    m_numPaths    { 4 };
    float  m_bloomRate   { 0.3f };
    float  m_stagger     { 0.5f };
    float  m_chaos       { 0.15f };
    float  m_mix         { 0.7f };

    double m_sampleRate  { 44100.0 };

    juce::CriticalSection m_pathCountLock;
};

} // namespace halation

#pragma once

#include <juce_core/juce_core.h>

namespace ParameterIDs
{
    // Global parameters
    static const juce::String globalNumPaths       { "global_num_paths" };
    static const juce::String globalBloomRate      { "global_bloom_rate" };
    static const juce::String globalStagger        { "global_stagger" };
    static const juce::String globalSpectralTilt   { "global_spectral_tilt" };
    static const juce::String globalDamping        { "global_damping" };
    static const juce::String globalChaos          { "global_chaos" };
    static const juce::String globalMix            { "global_mix" };
    static const juce::String globalIntervalPreset { "global_interval_preset" };

    // Per-path parameters — use pathSemitones(N), pathLevel(N), pathPan(N)
    inline juce::String pathSemitones (int path) { return "path_" + juce::String (path) + "_semitones"; }
    inline juce::String pathLevel     (int path) { return "path_" + juce::String (path) + "_level"; }
    inline juce::String pathPan       (int path) { return "path_" + juce::String (path) + "_pan"; }
}

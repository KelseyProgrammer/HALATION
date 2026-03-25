#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace halation
{

enum class PresetID
{
    Unison         = 0,
    Octaves        = 1,
    Fifths         = 2,
    HarmonicSeries = 3,
    Chromatic      = 4,
    Custom         = 5
};

class IntervalPresets
{
public:
    static constexpr int kMaxPaths = 8;

    static std::array<float, kMaxPaths> getPreset (PresetID id);
    static juce::String getPresetName (PresetID id);
    static juce::String semitoneToIntervalName (int semitones);
};

} // namespace halation

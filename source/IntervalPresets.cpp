#include "IntervalPresets.h"

namespace halation
{

std::array<float, IntervalPresets::kMaxPaths> IntervalPresets::getPreset (PresetID id)
{
    switch (id)
    {
        case PresetID::Unison:
            return { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

        case PresetID::Octaves:
            return { 0.f, 12.f, -12.f, 24.f, -24.f, 12.f, 0.f, -12.f };

        case PresetID::Fifths:
            return { 0.f, 7.f, 12.f, 19.f, -5.f, 7.f, -12.f, 5.f };

        case PresetID::HarmonicSeries:
            return { 0.f, 12.f, 19.f, 24.f, 28.f, 31.f, 34.f, 36.f };

        case PresetID::Chromatic:
            return { 0.f, 1.f, 2.f, 3.f, -1.f, -2.f, -3.f, -4.f };

        case PresetID::Custom:
        default:
            return { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
    }
}

juce::String IntervalPresets::getPresetName (PresetID id)
{
    switch (id)
    {
        case PresetID::Unison:         return "Unison";
        case PresetID::Octaves:        return "Octaves";
        case PresetID::Fifths:         return "Fifths";
        case PresetID::HarmonicSeries: return "Harmonic Series";
        case PresetID::Chromatic:      return "Chromatic";
        case PresetID::Custom:         return "Custom";
        default:                       return "Unknown";
    }
}

juce::String IntervalPresets::semitoneToIntervalName (int semitones)
{
    static const std::map<int, juce::String> names =
    {
        {  0,  "P1"    }, {  1,  "m2"   }, {  2,  "M2"   }, {  3,  "m3"   },
        {  4,  "M3"    }, {  5,  "P4"   }, {  6,  "A4"   }, {  7,  "P5"   },
        {  8,  "m6"    }, {  9,  "M6"   }, { 10,  "m7"   }, { 11,  "M7"   },
        { 12,  "Oct\xe2\x86\x91" },  // Oct↑
        { -12, "Oct\xe2\x86\x93" },  // Oct↓
        { 19,  "12\xe2\x86\x91"  },  // 12↑
        { 24,  "2Oct\xe2\x86\x91"},  // 2Oct↑
        { -24, "2Oct\xe2\x86\x93"},  // 2Oct↓
        { -5,  "P4\xe2\x86\x93"  },  // P4↓
        { -7,  "P5\xe2\x86\x93"  },  // P5↓
    };

    auto it = names.find (semitones);
    if (it != names.end())
        return it->second;

    return (semitones > 0 ? "+" : "") + juce::String (semitones);
}

} // namespace halation

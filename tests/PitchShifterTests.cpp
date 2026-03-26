#include <PitchShifter.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{
    // Goertzel algorithm: magnitude of a single DFT bin (normalized by N).
    float goertzelMagnitude (const std::vector<float>& samples, float freqHz, float sampleRate)
    {
        const float omega = juce::MathConstants<float>::twoPi * freqHz / sampleRate;
        const float coeff = 2.0f * std::cos (omega);
        float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;

        for (float x : samples)
        {
            s0 = x + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }

        return std::sqrt (s1 * s1 + s2 * s2 - coeff * s1 * s2)
               / static_cast<float> (samples.size());
    }
}

TEST_CASE ("PitchShifter silence in produces silence out", "[pitchshifter]")
{
    halation::PitchShifter ps;
    ps.prepare (44100.0, 512);
    ps.setPitchRatio (1.0f);

    for (int i = 0; i < halation::PitchShifter::kFFTSize * 4; ++i)
        CHECK (ps.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-6f));
}

TEST_CASE ("PitchShifter unity ratio passes signal at original frequency", "[pitchshifter]")
{
    halation::PitchShifter ps;
    ps.prepare (44100.0, 512);
    ps.setPitchRatio (1.0f);

    const float sampleRate  = 44100.0f;
    const float inputFreqHz = 440.0f;
    const int   settleLen   = halation::PitchShifter::kFFTSize * 4; // generous settling
    const int   measureLen  = 8192;

    std::vector<float> settled;
    settled.reserve (static_cast<size_t> (measureLen));

    for (int i = 0; i < settleLen + measureLen; ++i)
    {
        float in  = std::sin (juce::MathConstants<float>::twoPi * inputFreqHz * float (i) / sampleRate);
        float out = ps.processSample (in);
        if (i >= settleLen)
            settled.push_back (out);
    }

    // With unity ratio the energy stays at the original frequency, not shifted up
    const float magAtInput  = goertzelMagnitude (settled, inputFreqHz,          sampleRate);
    const float magAtDouble = goertzelMagnitude (settled, inputFreqHz * 2.0f,   sampleRate);

    CHECK (magAtInput > magAtDouble);
}

TEST_CASE ("PitchShifter octave-up shift moves energy to double the frequency", "[pitchshifter]")
{
    halation::PitchShifter ps;
    ps.prepare (44100.0, 512);
    ps.setPitchRatio (2.0f); // octave up: 440 Hz -> 880 Hz

    const float sampleRate   = 44100.0f;
    const float inputFreqHz  = 440.0f;
    const float targetFreqHz = 880.0f;
    const int   settleLen    = halation::PitchShifter::kFFTSize * 2;
    const int   measureLen   = 8192;

    std::vector<float> settled;
    settled.reserve (static_cast<size_t> (measureLen));

    for (int i = 0; i < settleLen + measureLen; ++i)
    {
        float in  = std::sin (juce::MathConstants<float>::twoPi * inputFreqHz * float (i) / sampleRate);
        float out = ps.processSample (in);

        if (i >= settleLen)
            settled.push_back (out);
    }

    const float magAtInput  = goertzelMagnitude (settled, inputFreqHz,  sampleRate);
    const float magAtTarget = goertzelMagnitude (settled, targetFreqHz, sampleRate);

    // The shifted frequency should have more energy than the original
    CHECK (magAtTarget > magAtInput);
}

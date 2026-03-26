#include <SpectralTilt.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{
    // Run numSamples of a sine tone through the tilt filter and return output RMS.
    // Skips the first skipSamples to let the IIR filters settle.
    float measureToneRMS (halation::SpectralTilt& tilt,
                          float freqHz,
                          int   numSamples = 8192,
                          int   skipSamples = 2048,
                          float sampleRate  = 44100.0f)
    {
        double sum = 0.0;
        int measured = 0;
        const int total = skipSamples + numSamples;

        for (int n = 0; n < total; ++n)
        {
            float s = std::sin (juce::MathConstants<float>::twoPi * freqHz * float (n) / sampleRate);
            float l = s, r = s;
            tilt.processSample (l, r);

            if (n >= skipSamples)
            {
                sum += double (l) * double (l);
                ++measured;
            }
        }
        return static_cast<float> (std::sqrt (sum / measured));
    }
}

TEST_CASE ("SpectralTilt flat tilt produces near-unity gain at midrange", "[spectraltilt]")
{
    halation::SpectralTilt tilt;
    tilt.prepare (44100.0, 512);
    tilt.setTilt    (0.0f);
    tilt.setDamping (0.0f);

    // 2kHz is comfortably between both shelf frequencies (500Hz and 4kHz)
    const float inputRMS  = std::sqrt (0.5f); // theoretical sine RMS
    const float outputRMS = measureToneRMS (tilt, 2000.0f);

    // Allow ±2 dB (epsilon ~0.25 relative)
    CHECK (outputRMS == Catch::Approx (inputRMS).epsilon (0.25f));
}

TEST_CASE ("SpectralTilt positive tilt increases HF energy", "[spectraltilt]")
{
    halation::SpectralTilt flat, bright;
    flat  .prepare (44100.0, 512);  flat  .setTilt (0.0f);   flat  .setDamping (0.0f);
    bright.prepare (44100.0, 512);  bright.setTilt (1.0f);   bright.setDamping (0.0f);

    const float rmsFlat   = measureToneRMS (flat,   8000.0f);
    const float rmsBright = measureToneRMS (bright, 8000.0f);

    CHECK (rmsBright > rmsFlat);
}

TEST_CASE ("SpectralTilt negative tilt reduces HF energy", "[spectraltilt]")
{
    halation::SpectralTilt flat, dark;
    flat.prepare (44100.0, 512);  flat.setTilt (0.0f);    flat.setDamping (0.0f);
    dark.prepare (44100.0, 512);  dark.setTilt (-1.0f);   dark.setDamping (0.0f);

    const float rmsFlat = measureToneRMS (flat, 8000.0f);
    const float rmsDark = measureToneRMS (dark, 8000.0f);

    CHECK (rmsDark < rmsFlat);
}

TEST_CASE ("SpectralTilt damping attenuates high frequencies", "[spectraltilt]")
{
    halation::SpectralTilt open, damped;
    open  .prepare (44100.0, 512);  open  .setTilt (0.0f);  open  .setDamping (0.0f);
    damped.prepare (44100.0, 512);  damped.setTilt (0.0f);  damped.setDamping (0.8f);

    const float rmsOpen   = measureToneRMS (open,   8000.0f);
    const float rmsDamped = measureToneRMS (damped, 8000.0f);

    CHECK (rmsDamped < rmsOpen);
}

#include <HalationEngine.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE ("HalationEngine prepares and resets without crashing", "[engine]")
{
    halation::HalationEngine engine;
    REQUIRE_NOTHROW (engine.prepare (44100.0, 512));
    REQUIRE_NOTHROW (engine.reset());
}

TEST_CASE ("HalationEngine produces no NaN or inf at any feedback value", "[engine]")
{
    halation::HalationEngine engine;
    engine.prepare (44100.0, 512);

    for (float bloomRate : { 0.0f, 0.25f, 0.5f, 0.75f, 0.99f })
    {
        engine.reset();
        engine.setNumPaths (4);
        engine.setGlobalParameters (bloomRate, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

        juce::AudioBuffer<float> buffer (2, 512);

        for (int block = 0; block < 300; ++block)
        {
            // Alternating signal to exercise feedback accumulation
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* ptr = buffer.getWritePointer (ch);
                for (int n = 0; n < 512; ++n)
                    ptr[n] = (n % 2 == 0) ? 0.3f : -0.3f;
            }

            engine.process (buffer);

            INFO ("bloomRate=" << bloomRate << " block=" << block);
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* out = buffer.getReadPointer (ch);
                for (int n = 0; n < 512; ++n)
                    REQUIRE (std::isfinite (out[n]));
            }
        }
    }
}

TEST_CASE ("HalationEngine num_paths changes mid-stream produce no NaN", "[engine]")
{
    halation::HalationEngine engine;
    engine.prepare (44100.0, 512);
    engine.setGlobalParameters (0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

    juce::AudioBuffer<float> buffer (2, 512);

    auto fillAndProcess = [&]
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* ptr = buffer.getWritePointer (ch);
            for (int n = 0; n < 512; ++n)
                ptr[n] = 0.1f * std::sin (juce::MathConstants<float>::twoPi
                                          * 440.0f * float (n) / 44100.0f);
        }
        engine.process (buffer);

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* out = buffer.getReadPointer (ch);
            for (int n = 0; n < 512; ++n)
                REQUIRE (std::isfinite (out[n]));
        }
    };

    engine.setNumPaths (4);
    for (int i = 0; i < 10; ++i) fillAndProcess();

    engine.setNumPaths (8);
    for (int i = 0; i < 10; ++i) fillAndProcess();

    engine.setNumPaths (2);
    for (int i = 0; i < 10; ++i) fillAndProcess();

    engine.setNumPaths (4);
    for (int i = 0; i < 10; ++i) fillAndProcess();
}

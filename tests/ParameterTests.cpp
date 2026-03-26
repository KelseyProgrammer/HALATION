#include "helpers/test_helpers.h"
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Parameter IDs exist in APVTS", "[parameters]")
{
    PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    REQUIRE (apvts.getParameter (ParameterIDs::globalNumPaths)       != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalBloomRate)      != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalStagger)        != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalSpectralTilt)   != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalDamping)        != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalChaos)          != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalMix)            != nullptr);
    REQUIRE (apvts.getParameter (ParameterIDs::globalIntervalPreset) != nullptr);

    for (int i = 0; i < 8; ++i)
    {
        INFO ("path " << i);
        REQUIRE (apvts.getParameter (ParameterIDs::pathSemitones (i)) != nullptr);
        REQUIRE (apvts.getParameter (ParameterIDs::pathLevel     (i)) != nullptr);
        REQUIRE (apvts.getParameter (ParameterIDs::pathPan       (i)) != nullptr);
    }
}

TEST_CASE ("Global parameter defaults match spec", "[parameters]")
{
    PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalNumPaths)       == Catch::Approx (4.0f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalBloomRate)      == Catch::Approx (0.3f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalStagger)        == Catch::Approx (0.5f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalSpectralTilt)   == Catch::Approx (-0.2f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalDamping)        == Catch::Approx (0.4f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalChaos)          == Catch::Approx (0.15f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalMix)            == Catch::Approx (0.7f));
    CHECK (*apvts.getRawParameterValue (ParameterIDs::globalIntervalPreset) == Catch::Approx (2.0f));
}

TEST_CASE ("Per-path parameter defaults match spec", "[parameters]")
{
    PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();

    // Default preset is Fifths (index 2)
    const float fifthsSemitones[] = { 0.f, 7.f, 12.f, 19.f, -5.f, 7.f, -12.f, 5.f };

    for (int i = 0; i < 8; ++i)
    {
        INFO ("path " << i);
        CHECK (*apvts.getRawParameterValue (ParameterIDs::pathSemitones (i))
               == Catch::Approx (fifthsSemitones[i]));
        CHECK (*apvts.getRawParameterValue (ParameterIDs::pathLevel (i))
               == Catch::Approx (1.0f));

        const float expectedPan = -0.875f + static_cast<float> (i) * 0.25f;
        CHECK (*apvts.getRawParameterValue (ParameterIDs::pathPan (i))
               == Catch::Approx (expectedPan));
    }
}

#include "HalationEngine.h"
#include "IntervalPresets.h"

namespace halation
{

HalationEngine::HalationEngine()
{
    auto fifths = IntervalPresets::getPreset (PresetID::Fifths);
    for (auto i = 0; i < kMaxPaths; ++i)
    {
        const auto idx = static_cast<size_t> (i);
        m_paths[idx].setSemitones (fifths[idx]);
        m_paths[idx].setLevel (1.0f);

        // Spread pans evenly from -0.875 to +0.875
        float pan = -0.875f + static_cast<float> (i) * 0.25f;
        m_paths[idx].setPan (pan);
    }
}

void HalationEngine::prepare (double sampleRate, int maxBlockSize)
{
    m_sampleRate = sampleRate;
    for (auto& path : m_paths)
        path.prepare (sampleRate, maxBlockSize);
    updateStaggerDelays();
}

void HalationEngine::reset()
{
    for (auto& path : m_paths)
        path.reset();
}

void HalationEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2)
        return;

    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getWritePointer (1);

    const float dryGain = 1.0f - m_mix;
    const float wetGain = m_mix;

    const float chaosRate = juce::MathConstants<float>::twoPi * 0.3f
                            / static_cast<float> (m_sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        const float dryL = dataL[s];
        const float dryR = dataR[s];

        float wetL = 0.0f;
        float wetR = 0.0f;

        {
            juce::ScopedTryLock lock (m_pathCountLock);
            const int n = m_numPaths;

            for (auto i = 0; i < n; ++i)
            {
                const auto idx = static_cast<size_t> (i);

                float chaosCents = std::sin (m_chaosLfoPhase[idx]) * m_chaos * 15.0f;
                m_chaosLfoPhase[idx] += chaosRate * (1.0f + static_cast<float> (i) * 0.17f);
                if (m_chaosLfoPhase[idx] > juce::MathConstants<float>::twoPi)
                    m_chaosLfoPhase[idx] -= juce::MathConstants<float>::twoPi;

                juce::ignoreUnused (chaosCents); // applied in Phase 2 per-path pitch offset

                auto [pathWetL, pathWetR] = m_paths[idx].process (dryL, dryR,
                                                                    m_bloomRate,
                                                                    m_staggerDelaySamples[idx]);
                wetL += pathWetL;
                wetR += pathWetR;
            }
        }

        const float norm = (m_numPaths > 0) ? (1.0f / static_cast<float> (m_numPaths)) : 1.0f;
        dataL[s] = dryL * dryGain + wetL * wetGain * norm;
        dataR[s] = dryR * dryGain + wetR * wetGain * norm;
    }
}

void HalationEngine::setNumPaths (int n)
{
    juce::ScopedLock lock (m_pathCountLock);
    m_numPaths = juce::jlimit (kMinPaths, kMaxPaths, n);
    updateStaggerDelays();
}

void HalationEngine::setPathInterval (int path, float semitones)
{
    if (path >= 0 && path < kMaxPaths)
        m_paths[static_cast<size_t> (path)].setSemitones (semitones);
}

void HalationEngine::setPathLevel (int path, float level)
{
    if (path >= 0 && path < kMaxPaths)
        m_paths[static_cast<size_t> (path)].setLevel (level);
}

void HalationEngine::setPathPan (int path, float pan)
{
    if (path >= 0 && path < kMaxPaths)
        m_paths[static_cast<size_t> (path)].setPan (pan);
}

void HalationEngine::setGlobalParameters (float bloomRate, float stagger, float spectralTilt,
                                           float damping, float chaos, float mix)
{
    m_bloomRate = bloomRate;
    m_mix       = mix;
    m_chaos     = chaos;
    m_stagger   = stagger;

    updateStaggerDelays();

    for (auto& path : m_paths)
    {
        path.setSpectralTilt (spectralTilt);
        path.setDamping (damping);
    }
}

int HalationEngine::getLatencySamples() const
{
    return PitchShifter::kFFTSize;
}

void HalationEngine::updateStaggerDelays()
{
    const float srMs = static_cast<float> (m_sampleRate) / 1000.0f;
    for (auto i = 0; i < kMaxPaths; ++i)
    {
        const auto idx = static_cast<size_t> (i);
        float delayMs  = static_cast<float> (i) * m_stagger * PathProcessor::kMaxStaggerMs;
        m_staggerDelaySamples[idx] = static_cast<int> (delayMs * srMs);
    }
}

} // namespace halation

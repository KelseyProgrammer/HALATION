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

        float pan = -0.875f + static_cast<float> (i) * 0.25f;
        m_paths[idx].setPan (pan);
    }
}

void HalationEngine::prepare (double sampleRate, int maxBlockSize)
{
    m_sampleRate = sampleRate;

    for (auto& path : m_paths)
        path.prepare (sampleRate, maxBlockSize);

    m_bloomSmoothed.reset (sampleRate, 0.02);
    m_bloomSmoothed.setCurrentAndTargetValue (0.3f);

    m_mixSmoothed.reset (sampleRate, 0.02);
    m_mixSmoothed.setCurrentAndTargetValue (0.7f);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
    spec.numChannels      = 2;
    m_limiter.prepare (spec);
    m_limiter.setThreshold (-1.0f);
    m_limiter.setRelease (200.0f);

    updateStaggerDelays();
}

void HalationEngine::reset()
{
    for (auto& path : m_paths)
        path.reset();

    for (auto& level : m_pathLevels)
        level.store (0.0f, std::memory_order_relaxed);

    m_dryDelayL.fill (0.0f);
    m_dryDelayR.fill (0.0f);
    m_dryDelayHead = 0;

    m_limiter.reset();
}

void HalationEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getWritePointer (1);

    // Chaos LFOs advance once per block — the phase vocoder only samples the
    // pitch ratio at hop boundaries, so per-sample updates were wasted work.
    updateChaosOffsets (numSamples);

    const int   n    = m_numPaths;
    // Equal-power path summing: 1/sqrt(n) keeps perceived level steady as
    // paths are added instead of getting quieter (1/n).
    const float norm = 1.0f / std::sqrt (static_cast<float> (n > 0 ? n : 1));

    std::array<float, kMaxPaths> sumSquares {};

    for (int s = 0; s < numSamples; ++s)
    {
        const float inL = dataL[s];
        const float inR = dataR[s];

        // Delay the dry path by the pitch shifter latency so dry and wet are
        // aligned with the latency reported to the host.
        const auto dh = static_cast<size_t> (m_dryDelayHead);
        const float dryL = m_dryDelayL[dh];
        const float dryR = m_dryDelayR[dh];
        m_dryDelayL[dh] = inL;
        m_dryDelayR[dh] = inR;
        if (++m_dryDelayHead >= PitchShifter::kFFTSize)
            m_dryDelayHead = 0;

        // Advance smoothed parameters one sample
        const float bloom = m_bloomSmoothed.getNextValue();
        const float mix   = m_mixSmoothed.getNextValue();

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (auto i = 0; i < n; ++i)
        {
            const auto idx = static_cast<size_t> (i);
            auto [pathWetL, pathWetR] = m_paths[idx].process (inL, inR, bloom);
            wetL += pathWetL;
            wetR += pathWetR;
            sumSquares[idx] += pathWetL * pathWetL + pathWetR * pathWetR;
        }

        // Equal-power dry/wet crossfade
        const float mixAngle = mix * juce::MathConstants<float>::halfPi;
        const float dryGain  = std::cos (mixAngle);
        const float wetGain  = std::sin (mixAngle);

        dataL[s] = dryL * dryGain + wetL * wetGain * norm;
        dataR[s] = dryR * dryGain + wetR * wetGain * norm;
    }

    // Update per-path meters: fast attack, slow release
    const float invN = 1.0f / static_cast<float> (numSamples);
    for (auto i = 0; i < kMaxPaths; ++i)
    {
        const auto idx = static_cast<size_t> (i);
        const float rms  = (i < n) ? std::sqrt (sumSquares[idx] * 0.5f * invN) : 0.0f;
        const float prev = m_pathLevels[idx].load (std::memory_order_relaxed);
        const float next = rms > prev ? rms : prev * 0.85f;
        m_pathLevels[idx].store (next, std::memory_order_relaxed);
    }

    // Output limiter — prevents runaway feedback from clipping
    juce::dsp::AudioBlock<float>          block  (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    m_limiter.process (ctx);
}

void HalationEngine::setNumPaths (int n)
{
    const int clamped = juce::jlimit (kMinPaths, kMaxPaths, n);
    const int prev    = m_numPaths;
    m_numPaths = clamped;

    // Reset paths that just became inactive to clear stale feedback
    if (clamped < prev)
    {
        for (int i = clamped; i < prev; ++i)
            m_paths[static_cast<size_t> (i)].reset();
    }
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

void HalationEngine::setGlobalParameters (float bloomRate, float stagger,
                                           float spectralTilt, float damping,
                                           float chaos, float mix)
{
    m_bloomSmoothed.setTargetValue (bloomRate);
    m_mixSmoothed.setTargetValue   (mix);
    m_chaos = chaos;

    if (! juce::exactlyEqual (stagger, m_stagger))
    {
        m_stagger = stagger;
        updateStaggerDelays();
    }

    // SpectralTilt setters detect unchanged values internally — no
    // coefficient recomputation happens unless a knob actually moved.
    for (auto& path : m_paths)
    {
        path.setSpectralTilt (spectralTilt);
        path.setDamping      (damping);
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
        const float delayMs = PathProcessor::kBaseDelayMs
                              + static_cast<float> (i) * m_stagger
                                * PathProcessor::kMaxStaggerMs;
        m_paths[static_cast<size_t> (i)].setDelaySamples (delayMs * srMs);
    }
}

void HalationEngine::updateChaosOffsets (int numSamples)
{
    const float baseRate = juce::MathConstants<float>::twoPi * 0.3f
                           / static_cast<float> (m_sampleRate);

    for (auto i = 0; i < m_numPaths; ++i)
    {
        const auto idx = static_cast<size_t> (i);

        // L/R read the LFO at slightly different phases so each voice drifts
        // independently across the stereo field.
        const float depth  = m_chaos * 15.0f;
        const float centsL = std::sin (m_chaosLfoPhase[idx]) * depth;
        const float centsR = std::sin (m_chaosLfoPhase[idx] + 1.9f) * depth;
        m_paths[idx].setChaosOffsetCents (centsL, centsR);

        m_chaosLfoPhase[idx] += baseRate
                                * (1.0f + static_cast<float> (i) * 0.17f)
                                * static_cast<float> (numSamples);
        while (m_chaosLfoPhase[idx] > juce::MathConstants<float>::twoPi)
            m_chaosLfoPhase[idx] -= juce::MathConstants<float>::twoPi;
    }
}

} // namespace halation

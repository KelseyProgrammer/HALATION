#pragma once

#include "PitchShifter.h"
#include "SpectralTilt.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>

namespace halation
{

// Handles one parallel feedback voice.
// Owns a circular delay buffer, a PitchShifter, a SpectralTilt, and a feedback tap.
class PathProcessor
{
public:
    static constexpr int    kMaxDelaySamples { 192000 }; // ~2 sec at 96kHz
    static constexpr float  kBaseDelayMs     { 20.0f };
    static constexpr float  kMaxStaggerMs    { 80.0f };

    PathProcessor();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Process one stereo frame. Returns the wet output pair.
    // bloomRate: smoothed feedback gain. staggerDelaySamples: per-path offset.
    std::pair<float, float> process (float inputL, float inputR,
                                     float bloomRate,
                                     int   staggerDelaySamples);

    void setSemitones        (float semitones);
    void setChaosOffsetCents (float cents);      // added to base semitones before each frame
    void setLevel            (float level);
    void setPan              (float pan);         // -1.0 left … +1.0 right
    void setSpectralTilt     (float tilt);
    void setDamping          (float damping);

    int getLatencySamples() const;

private:
    float readDelay  (int delaySamples, int channel) const;
    void  writeDelay (float sampleL, float sampleR);

    // Simple 1-pole DC blocking highpass at ~10 Hz
    float dcBlock (float x, float& prevIn, float& prevOut) const noexcept;

    void updatePitchRatio();

    PitchShifter m_pitchShifterL;
    PitchShifter m_pitchShifterR;
    SpectralTilt m_spectralTilt;

    std::array<float, kMaxDelaySamples> m_delayLineL {};
    std::array<float, kMaxDelaySamples> m_delayLineR {};
    int m_writeHead { 0 };

    float m_feedbackL { 0.0f };
    float m_feedbackR { 0.0f };

    // DC blocker state
    float m_dcPrevInL  { 0.0f };
    float m_dcPrevInR  { 0.0f };
    float m_dcPrevOutL { 0.0f };
    float m_dcPrevOutR { 0.0f };
    float m_dcCoeff    { 0.9997f }; // updated in prepare()

    // Pitch
    float m_baseSemitones      { 0.0f };
    float m_chaosOffsetCents   { 0.0f };

    // Level and pan with 20ms smoothing
    juce::SmoothedValue<float> m_levelSmoothed;
    juce::SmoothedValue<float> m_panLeftSmoothed;
    juce::SmoothedValue<float> m_panRightSmoothed;

    double m_sampleRate { 44100.0 };
};

} // namespace halation

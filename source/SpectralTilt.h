#pragma once

namespace halation
{

// Shelving EQ applied in the feedback path per voice.
// Positive tilt = brighter per cycle, negative = darker.
// Separate damping parameter adds a 1-pole LP for tape-style rolloff.
//
// Implemented with raw RBJ biquads so coefficient updates never allocate —
// safe to call setTilt/setDamping from the audio thread. Coefficients are
// only recomputed when a setter receives a genuinely new value.
class SpectralTilt
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Process a stereo pair of samples in place.
    void processSample (float& left, float& right);

    void setTilt    (float tilt);     // -1.0 to +1.0
    void setDamping (float damping);  // 0.0 to 1.0

private:
    struct Biquad
    {
        float b0 { 1.0f }, b1 { 0.0f }, b2 { 0.0f }, a1 { 0.0f }, a2 { 0.0f };
        float x1 { 0.0f }, x2 { 0.0f }, y1 { 0.0f }, y2 { 0.0f };

        float process (float x) noexcept
        {
            const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }

        void resetState() noexcept { x1 = x2 = y1 = y2 = 0.0f; }
        void copyCoeffsTo (Biquad& other) const noexcept
        {
            other.b0 = b0; other.b1 = b1; other.b2 = b2;
            other.a1 = a1; other.a2 = a2;
        }
    };

    struct OnePoleLowPass
    {
        float coeff { 1.0f };
        float state { 0.0f };

        float process (float x) noexcept
        {
            state += coeff * (x - state);
            return state;
        }

        void resetState() noexcept { state = 0.0f; }
    };

    void updateCoefficients();
    static void setHighShelf (Biquad& bq, double sampleRate, float freqHz, float gainDb);
    static void setLowShelf  (Biquad& bq, double sampleRate, float freqHz, float gainDb);

    Biquad m_highShelfL, m_highShelfR;
    Biquad m_lowShelfL,  m_lowShelfR;
    OnePoleLowPass m_dampingL, m_dampingR;

    double m_sampleRate { 44100.0 };
    float  m_tilt       { 0.0f };
    float  m_damping    { 0.0f };
};

} // namespace halation

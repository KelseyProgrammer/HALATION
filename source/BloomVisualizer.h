#pragma once

#include "HalationLookAndFeel.h"
#include "IntervalPresets.h"
#include "ParameterIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Radial bloom visualizer: one arm per active path, repaints at 30fps.
// Arm angle encodes path index, arm length encodes interval magnitude.
// Tip dots are draggable — dragging inward/outward changes path semitones.
class BloomVisualizer : public juce::Component,
                        private juce::Timer
{
public:
    explicit BloomVisualizer (juce::AudioProcessorValueTreeState& apvts);
    ~BloomVisualizer() override;

    void paint      (juce::Graphics&) override;
    void resized    () override;
    void mouseMove  (const juce::MouseEvent&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // Geometry helpers (all coordinates in local space)
    float    getMaxRadius() const;
    float    getArmAngle  (int pathIndex, int numPaths) const;
    float    getArmLength (float semitones, float maxRadius) const;
    float    semitoneFromProjection (float projLen, float maxRadius) const;
    juce::Point<float> getTipPosition (int pathIndex, int numPaths,
                                       float maxRadius) const;
    // Returns path index whose tip dot contains p, or -1
    int      hitTestTip (juce::Point<float> p) const;

    juce::AudioProcessorValueTreeState& m_apvts;

    int   m_draggingPath  = -1;   // path index being dragged, -1 = none
    int   m_hoveredPath   = -1;   // path index mouse is hovering over, -1 = none

    static constexpr float kHitRadius = 12.0f;  // px around tip dot to start drag

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BloomVisualizer)
};

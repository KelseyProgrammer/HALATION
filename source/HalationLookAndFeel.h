#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Custom LookAndFeel for HALATION.
// Implements arc-style rotary knobs, path-colored sliders, and the overall dark aesthetic.
class HalationLookAndFeel : public juce::LookAndFeel_V4
{
public:
    //==========================================================================
    // Brand colors
    static juce::Colour background()      { return juce::Colour (0xff12111a); }
    static juce::Colour accentAmber()     { return juce::Colour (0xffe8a84a); }
    static juce::Colour mutedLabel()      { return juce::Colour (0xff5a5070); }
    static juce::Colour mutedLabelAlt()   { return juce::Colour (0xff6a5f7a); }
    static juce::Colour knobValue()       { return juce::Colour (0xff9a8868); }
    static juce::Colour sectionBorder()   { return juce::Colour (0x1ec8a050); }

    // Per-path colors (index 0–7)
    static juce::Colour pathColour (int index);

    //==========================================================================
    HalationLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           juce::Slider::SliderStyle style,
                           juce::Slider& slider) override;

    juce::Font getLabelFont (juce::Label& label) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalationLookAndFeel)
};

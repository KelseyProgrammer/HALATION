#include "HalationLookAndFeel.h"

juce::Colour HalationLookAndFeel::pathColour (int index)
{
    static const juce::Colour kPathColours[8] =
    {
        juce::Colour (0xffe8a84a),  // 0: burnt amber
        juce::Colour (0xffc4707a),  // 1: rose
        juce::Colour (0xff8870c0),  // 2: violet
        juce::Colour (0xff4aa4b8),  // 3: teal
        juce::Colour (0xffd46a30),  // 4: orange
        juce::Colour (0xff7a9e3a),  // 5: olive green
        juce::Colour (0xffb05090),  // 6: magenta
        juce::Colour (0xff5a7ac4),  // 7: periwinkle
    };

    return kPathColours[juce::jlimit (0, 7, index)];
}

HalationLookAndFeel::HalationLookAndFeel()
{
    // Window / widget backgrounds
    setColour (juce::ResizableWindow::backgroundColourId, background());
    setColour (juce::DocumentWindow::backgroundColourId,  background());

    // Sliders
    setColour (juce::Slider::rotarySliderOutlineColourId, mutedLabel());
    setColour (juce::Slider::rotarySliderFillColourId,    accentAmber());
    setColour (juce::Slider::thumbColourId,               accentAmber());

    // Labels
    setColour (juce::Label::textColourId, mutedLabel());

    // TextButton
    setColour (juce::TextButton::buttonColourId,   background().brighter (0.08f));
    setColour (juce::TextButton::textColourOffId,  accentAmber().withAlpha (0.7f));
    setColour (juce::TextButton::textColourOnId,   accentAmber());

    // ComboBox
    setColour (juce::ComboBox::backgroundColourId,        background().brighter (0.12f));
    setColour (juce::ComboBox::textColourId,              accentAmber().withAlpha (0.9f));
    setColour (juce::ComboBox::outlineColourId,           accentAmber().withAlpha (0.35f));
    setColour (juce::ComboBox::focusedOutlineColourId,    accentAmber().withAlpha (0.7f));
    setColour (juce::ComboBox::arrowColourId,             accentAmber());

    // PopupMenu
    setColour (juce::PopupMenu::backgroundColourId,              background().brighter (0.08f));
    setColour (juce::PopupMenu::textColourId,                    juce::Colours::white.withAlpha (0.75f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId,   accentAmber().withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId,         accentAmber());
}

void HalationLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float sliderPosProportional,
                                             float rotaryStartAngle,
                                             float rotaryEndAngle,
                                             juce::Slider& slider)
{
    const float radius  = static_cast<float> (juce::jmin (width, height)) * 0.5f - 4.0f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width)  * 0.5f;
    const float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float angle   = rotaryStartAngle
                          + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Track arc (background)
    {
        juce::Path trackArc;
        trackArc.addArc (centreX - radius, centreY - radius,
                         radius * 2.0f, radius * 2.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (mutedLabel().withAlpha (0.35f));
        g.strokePath (trackArc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // Fill arc (value)
    {
        juce::Path fillArc;
        fillArc.addArc (centreX - radius, centreY - radius,
                        radius * 2.0f, radius * 2.0f,
                        rotaryStartAngle, angle, true);
        auto fillColour = slider.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (fillColour);
        g.strokePath (fillArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Indicator dot at tip
    {
        const float dotRadius = 3.0f;
        const float dotX = centreX + (radius - 2.0f)
                           * std::cos (angle - juce::MathConstants<float>::halfPi);
        const float dotY = centreY + (radius - 2.0f)
                           * std::sin (angle - juce::MathConstants<float>::halfPi);
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }
}

void HalationLookAndFeel::drawLinearSlider (juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float sliderPos,
                                             float /*minSliderPos*/,
                                             float /*maxSliderPos*/,
                                             juce::Slider::SliderStyle style,
                                             juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        const float trackY      = static_cast<float> (y + height / 2);
        const float trackLeft   = static_cast<float> (x);
        const float trackRight  = static_cast<float> (x + width);
        const float thumbRadius = 5.0f;

        // Track
        g.setColour (mutedLabel().withAlpha (0.25f));
        g.drawLine (trackLeft, trackY, trackRight, trackY, 1.5f);

        // Fill to position
        auto fillColour = slider.findColour (juce::Slider::trackColourId);
        if (fillColour == juce::Colours::transparentBlack)
            fillColour = slider.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (fillColour);
        g.drawLine (trackLeft, trackY, sliderPos, trackY, 2.0f);

        // Thumb dot
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        g.fillEllipse (sliderPos - thumbRadius, trackY - thumbRadius,
                       thumbRadius * 2.0f, thumbRadius * 2.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                          sliderPos, 0.0f, 1.0f, style, slider);
    }
}

void HalationLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour& backgroundColour,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto base = backgroundColour.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

    g.setColour (shouldDrawButtonAsDown    ? base.brighter (0.25f) :
                 shouldDrawButtonAsHighlighted ? base.brighter (0.1f) : base);
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (accentAmber().withAlpha (0.28f));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
}

void HalationLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                         bool /*isButtonDown*/,
                                         int /*buttonX*/, int /*buttonY*/,
                                         int /*buttonW*/, int /*buttonH*/,
                                         juce::ComboBox& /*box*/)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (background().brighter (0.12f));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (accentAmber().withAlpha (0.35f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    // Arrow
    const float arrowX = (float) width - 14.0f;
    const float arrowY = (float) height * 0.5f;
    const float arrowW = 6.0f;
    const float arrowH = 4.0f;

    juce::Path arrow;
    arrow.addTriangle (arrowX - arrowW * 0.5f, arrowY - arrowH * 0.4f,
                       arrowX + arrowW * 0.5f, arrowY - arrowH * 0.4f,
                       arrowX,                  arrowY + arrowH * 0.6f);
    g.setColour (accentAmber().withAlpha (0.8f));
    g.fillPath (arrow);
}

void HalationLookAndFeel::drawPopupMenuItem (juce::Graphics& g,
                                              const juce::Rectangle<int>& area,
                                              bool /*isSeparator*/, bool isActive,
                                              bool isHighlighted, bool isTicked,
                                              bool /*hasSubMenu*/,
                                              const juce::String& text,
                                              const juce::String& /*shortcutKeyText*/,
                                              const juce::Drawable* /*icon*/,
                                              const juce::Colour* /*textColour*/)
{
    if (isHighlighted && isActive)
    {
        g.setColour (accentAmber().withAlpha (0.16f));
        g.fillRect (area);
    }

    const auto colour = isTicked    ? accentAmber() :
                        isActive    ? juce::Colours::white.withAlpha (0.75f)
                                    : mutedLabel();
    g.setColour (colour);
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (9.5f)));
    g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft, true);
}

juce::Font HalationLookAndFeel::getLabelFont (juce::Label& /*label*/)
{
    return juce::Font (juce::FontOptions{}
                           .withName (juce::Font::getDefaultMonospacedFontName())
                           .withHeight (9.0f));
}

#include "PathRowComponent.h"

PathRowComponent::PathRowComponent (int pathIndex, juce::AudioProcessorValueTreeState& apvts)
    : m_pathIndex (pathIndex), m_apvts (apvts)
{
    const auto colour = HalationLookAndFeel::pathColour (pathIndex);

    m_intervalLabel.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (10.0f)));
    m_intervalLabel.setColour (juce::Label::textColourId, colour);
    m_intervalLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_intervalLabel);

    m_levelSlider.setColour (juce::Slider::trackColourId,      colour.withAlpha (0.75f));
    m_levelSlider.setColour (juce::Slider::backgroundColourId, HalationLookAndFeel::mutedLabel().withAlpha (0.2f));
    m_levelSlider.setColour (juce::Slider::thumbColourId,      colour);
    addAndMakeVisible (m_levelSlider);

    m_panSlider.setColour (juce::Slider::trackColourId,      colour.withAlpha (0.5f));
    m_panSlider.setColour (juce::Slider::backgroundColourId, HalationLookAndFeel::mutedLabel().withAlpha (0.2f));
    m_panSlider.setColour (juce::Slider::thumbColourId,      colour.withAlpha (0.9f));
    addAndMakeVisible (m_panSlider);

    m_levelAttach = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::pathLevel (pathIndex)), m_levelSlider);
    m_panAttach = std::make_unique<juce::SliderParameterAttachment> (
        *apvts.getParameter (ParameterIDs::pathPan (pathIndex)), m_panSlider);

    apvts.addParameterListener (ParameterIDs::pathSemitones (pathIndex), this);

    const float initSemi = *apvts.getRawParameterValue (ParameterIDs::pathSemitones (pathIndex));
    updateSemitoneLabel (initSemi);
}

PathRowComponent::~PathRowComponent()
{
    m_apvts.removeParameterListener (ParameterIDs::pathSemitones (m_pathIndex), this);
}

void PathRowComponent::paint (juce::Graphics& g)
{
    const auto  colour = HalationLookAndFeel::pathColour (m_pathIndex);
    const float h      = static_cast<float> (getHeight());

    // Color dot
    g.setColour (colour);
    g.fillEllipse (2.0f, (h - 5.0f) * 0.5f, 5.0f, 5.0f);

    // Path number
    g.setColour (colour.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (9.0f)));
    g.drawText (juce::String (m_pathIndex + 1), 9, 0, 12, getHeight(),
                juce::Justification::centredLeft, false);
}

void PathRowComponent::resized()
{
    // dot(7) + number(14) + interval(40) + level(130) + gap(6) + pan(24)
    m_intervalLabel.setBounds (23,  0,  40, getHeight());
    m_levelSlider  .setBounds (65,  8, 130, getHeight() - 16);
    m_panSlider    .setBounds (199, 8,  24, getHeight() - 16);
}

void PathRowComponent::parameterChanged (const juce::String&, float newValue)
{
    juce::MessageManager::callAsync ([this, newValue]
    {
        updateSemitoneLabel (newValue);
    });
}

void PathRowComponent::updateSemitoneLabel (float semitones)
{
    m_intervalLabel.setText (
        halation::IntervalPresets::semitoneToIntervalName (
            static_cast<int> (std::round (semitones))),
        juce::dontSendNotification);
}

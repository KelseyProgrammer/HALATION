#include "PathRowComponent.h"

PathRowComponent::PathRowComponent (int pathIndex, juce::AudioProcessorValueTreeState& apvts)
    : m_pathIndex (pathIndex), m_apvts (apvts)
{
    const auto colour = HalationLookAndFeel::pathColour (pathIndex);

    m_intervalLabel.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (9.0f)));
    m_intervalLabel.setColour (juce::Label::textColourId, colour);
    m_intervalLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_intervalLabel);

    m_levelSlider.setColour (juce::Slider::trackColourId,      colour.withAlpha (0.8f));
    m_levelSlider.setColour (juce::Slider::backgroundColourId, HalationLookAndFeel::mutedLabel().withAlpha (0.3f));
    m_levelSlider.setColour (juce::Slider::thumbColourId,      colour);
    addAndMakeVisible (m_levelSlider);

    m_panSlider.setColour (juce::Slider::trackColourId,      colour.withAlpha (0.5f));
    m_panSlider.setColour (juce::Slider::backgroundColourId, HalationLookAndFeel::mutedLabel().withAlpha (0.3f));
    m_panSlider.setColour (juce::Slider::thumbColourId,      colour.withAlpha (0.8f));
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
    const auto colour = HalationLookAndFeel::pathColour (m_pathIndex);

    // Color dot
    g.setColour (colour);
    g.fillEllipse (2.0f, (static_cast<float> (getHeight()) - 6.0f) * 0.5f, 6.0f, 6.0f);

    // Path number
    g.setColour (colour.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions{}
        .withName (juce::Font::getDefaultMonospacedFontName())
        .withHeight (8.0f)));
    g.drawText (juce::String (m_pathIndex + 1), 10, 0, 12, getHeight(),
                juce::Justification::centredLeft, false);
}

void PathRowComponent::resized()
{
    // dot(8) + index(14) + intervalLabel(42) + levelSlider(90) + gap(4) + panSlider(52)
    m_intervalLabel.setBounds (24,  0,  42, getHeight());
    m_levelSlider  .setBounds (68,  5,  90, getHeight() - 10);
    m_panSlider    .setBounds (162, 5,  52, getHeight() - 10);
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

#include "LevelSliderComponent.h"

namespace livellatore
{

LevelSliderComponent::LevelSliderComponent (juce::AudioProcessorValueTreeState& state,
                                             const juce::String& paramID,
                                             const juce::String& displayName)
{
    nameLabel.setText (displayName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (nameLabel);

    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, paramID, slider);
}

void LevelSliderComponent::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds (bounds.removeFromTop (18));
    slider.setBounds (bounds);
}

} // namespace livellatore

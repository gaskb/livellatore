#include "PluginEditor.h"

namespace livellatore
{

LivellatoreAudioProcessorEditor::LivellatoreAudioProcessorEditor (LivellatoreAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      inputGainSlider (p.apvts, ParamID::inputGain, "Input Gain"),
      targetLevelSlider (p.apvts, ParamID::targetLufs, "Target Level"),
      attackSlider (p.apvts, ParamID::attack, "Attack"),
      releaseSlider (p.apvts, ParamID::release, "Release"),
      gateThresholdSlider (p.apvts, ParamID::gateThreshold, "Gate Threshold"),
      limiterSlider (p.apvts, ParamID::limiter, "Limiter"),
      outputGainSlider (p.apvts, ParamID::outputGain, "Output Gain")
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);
    addAndMakeVisible (riderActivityMeter);

    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (targetLevelSlider);
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (releaseSlider);
    addAndMakeVisible (gateThresholdSlider);
    addAndMakeVisible (limiterSlider);
    addAndMakeVisible (outputGainSlider);

    setSize (520, 420);
    startTimerHz (30);
}

LivellatoreAudioProcessorEditor::~LivellatoreAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LivellatoreAudioProcessorEditor::timerCallback()
{
    auto& engine = processorRef.getEngine();
    inputMeter.setLevelDb (engine.getInputLevelDb());
    outputMeter.setLevelDb (engine.getOutputLevelDb());
    riderActivityMeter.setLevelDb (engine.getRiderGainDb());
}

void LivellatoreAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (LivellatoreLookAndFeel::backgroundColour);
}

void LivellatoreAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    auto meterArea = bounds.removeFromRight (150);
    inputMeter.setBounds (meterArea.removeFromLeft (40).reduced (4));
    riderActivityMeter.setBounds (meterArea.removeFromLeft (60).reduced (4));
    outputMeter.setBounds (meterArea.removeFromLeft (40).reduced (4));

    bounds.removeFromRight (16);

    const int rowHeight = 48;
    inputGainSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    targetLevelSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    attackSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    releaseSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    gateThresholdSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    limiterSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    outputGainSlider.setBounds (bounds.removeFromTop (rowHeight));
}

} // namespace livellatore

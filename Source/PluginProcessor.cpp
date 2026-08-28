#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace livellatore
{

LivellatoreAudioProcessor::LivellatoreAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void LivellatoreAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    engine.prepare (spec);
    updateEngineParametersFromState();

    // Il limiter con lookahead (#9) introduce latenza: va dichiarata
    // all'host per un plugin delay compensation corretto.
    setLatencySamples (engine.getLatencySamples());
}

void LivellatoreAudioProcessor::releaseResources()
{
}

bool LivellatoreAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto set = layouts.getMainOutputChannelSet();
    return (set == mono || set == stereo) && layouts.getMainInputChannelSet() == set;
}

void LivellatoreAudioProcessor::applyFactoryPreset (int index)
{
    const auto& factoryPresets = PresetManager::getFactoryPresets();
    if (index < 0 || (size_t) index >= factoryPresets.size())
        return;

    for (const auto& [paramID, value] : factoryPresets[(size_t) index].paramValues)
        apvts.getParameterAsValue (paramID) = value;
}

bool LivellatoreAudioProcessor::loadUserPreset (const juce::String& name)
{
    auto state = presetManager.loadPreset (name);
    if (! state.isValid())
        return false;

    apvts.replaceState (state);
    return true;
}

bool LivellatoreAudioProcessor::saveCurrentStateAsPreset (const juce::String& name)
{
    return presetManager.savePreset (name, apvts.copyState());
}

void LivellatoreAudioProcessor::updateEngineParametersFromState()
{
    engine.setInputGainDb (apvts.getRawParameterValue (ParamID::inputGain)->load());
    engine.setTargetLufs (apvts.getRawParameterValue (ParamID::targetLufs)->load());
    engine.setAttackMs (apvts.getRawParameterValue (ParamID::attack)->load());
    engine.setReleaseMs (apvts.getRawParameterValue (ParamID::release)->load());
    engine.setGateThresholdLufs (apvts.getRawParameterValue (ParamID::gateThreshold)->load());
    engine.setLimiterThresholdDb (apvts.getRawParameterValue (ParamID::limiter)->load());
    engine.setOutputGainDb (apvts.getRawParameterValue (ParamID::outputGain)->load());
}

void LivellatoreAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateEngineParametersFromState();
    engine.process (buffer);
}

juce::AudioProcessorEditor* LivellatoreAudioProcessor::createEditor()
{
    return new LivellatoreAudioProcessorEditor (*this);
}

void LivellatoreAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void LivellatoreAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace livellatore

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new livellatore::LivellatoreAudioProcessor();
}

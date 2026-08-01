#include "LevelerEngine.h"

namespace livellatore
{

void LevelerEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    loudnessMeter.prepare (spec.sampleRate, (int) spec.numChannels);
    gainRider.prepare (spec.sampleRate);
    limiter.prepare (spec);

    reset();
}

void LevelerEngine::reset()
{
    loudnessMeter.reset();
    gainRider.reset();
    limiter.reset();
    inputLevelDb = -100.0f;
    outputLevelDb = -100.0f;
}

void LevelerEngine::process (juce::AudioBuffer<float>& buffer)
{
    // 1. Input gain
    buffer.applyGain (juce::Decibels::decibelsToGain (inputGainDb));
    inputLevelDb = buffer.getMagnitude (0, buffer.getNumSamples()) > 0.0f
                       ? juce::Decibels::gainToDecibels (buffer.getMagnitude (0, buffer.getNumSamples()))
                       : -100.0f;

    // 2. Misura loudness sul segnale post input-gain
    loudnessMeter.pushBlock (buffer);

    // 3. Gain rider: calcola e applica la correzione verso il target
    const double deltaTimeSeconds = (double) buffer.getNumSamples() / sampleRate;
    const float riderGainDb = gainRider.computeGainDb (loudnessMeter.getLoudnessLufs(), deltaTimeSeconds);
    buffer.applyGain (juce::Decibels::decibelsToGain (riderGainDb));

    // 4. Output (makeup) gain, indipendente dal rider
    buffer.applyGain (juce::Decibels::decibelsToGain (outputGainDb));

    // 5. Limiter di sicurezza sui transienti residui
    limiter.process (buffer);

    outputLevelDb = buffer.getMagnitude (0, buffer.getNumSamples()) > 0.0f
                         ? juce::Decibels::gainToDecibels (buffer.getMagnitude (0, buffer.getNumSamples()))
                         : -100.0f;
}

} // namespace livellatore

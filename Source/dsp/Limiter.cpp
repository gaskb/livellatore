#include "Limiter.h"

namespace livellatore
{

void Limiter::prepare (const juce::dsp::ProcessSpec& spec)
{
    limiter.prepare (spec);
}

void Limiter::reset()
{
    limiter.reset();
    currentGainReductionDb = 0.0f;
}

void Limiter::setThresholdDb (float db)
{
    limiter.setThreshold (db);
}

void Limiter::setReleaseMs (float ms)
{
    limiter.setRelease (ms);
}

void Limiter::process (juce::AudioBuffer<float>& buffer)
{
    const float peakBefore = buffer.getMagnitude (0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    limiter.process (context);

    const float peakAfter = buffer.getMagnitude (0, buffer.getNumSamples());

    if (peakBefore > 1.0e-6f && peakAfter > 1.0e-6f)
        currentGainReductionDb = juce::Decibels::gainToDecibels (peakAfter / peakBefore);
    else
        currentGainReductionDb = 0.0f;
}

} // namespace livellatore

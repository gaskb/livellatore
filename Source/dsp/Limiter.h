#pragma once

#include <juce_dsp/juce_dsp.h>

namespace livellatore
{

/**
 * Limiter di sicurezza a valle del gain rider: taglia i transienti troppo
 * rapidi perché il rider (che lavora con tempi di attack/release "musicali",
 * non a livello di sample) riesca a gestirli. Wrapper sottile su
 * juce::dsp::Limiter con metering della gain reduction.
 */
class Limiter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setThresholdDb (float db);
    void setReleaseMs (float ms);

    /** Processa in-place e aggiorna la stima di gain reduction. */
    void process (juce::AudioBuffer<float>& buffer);

    float getGainReductionDb() const noexcept { return currentGainReductionDb; }

private:
    juce::dsp::Limiter<float> limiter;
    float currentGainReductionDb = 0.0f;
};

} // namespace livellatore

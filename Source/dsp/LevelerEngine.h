#pragma once

#include "LoudnessMeter.h"
#include "GainRider.h"
#include "Limiter.h"
#include <juce_dsp/juce_dsp.h>

namespace livellatore
{

/**
 * Orchestratore della catena DSP: input gain -> misura loudness -> gain
 * rider -> output (makeup) gain -> limiter. Non conosce APVTS/JUCE plugin
 * wrapper: riceve solo parametri "grezzi" e buffer audio, per restare
 * testabile in isolamento (vedi Tests/).
 */
class LevelerEngine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setInputGainDb (float db) noexcept { inputGainDb = db; }
    void setTargetLufs (float lufs) noexcept { gainRider.setTargetLufs (lufs); }
    void setAttackMs (float ms) noexcept { gainRider.setAttackMs (ms); }
    void setReleaseMs (float ms) noexcept { gainRider.setReleaseMs (ms); }
    void setGateThresholdLufs (float lufs) noexcept
    {
        gainRider.setGateThresholdLufs (lufs);
        loudnessMeter.setGateThresholdLufs (lufs); // stessa soglia, vedi Dialogue Mode
    }
    /** "Dialogue Mode" (nato da domanda utente su parlato con pause): la
     * misura di loudness ignora i tratti sotto la soglia di gate invece
     * di lasciare che le pause diluiscano la media, vedi LoudnessMeter.h. */
    void setDialogueMode (bool enabled) noexcept { loudnessMeter.setGatingEnabled (enabled); }
    void setOutputGainDb (float db) noexcept { outputGainDb = db; }
    void setLimiterThresholdDb (float db) noexcept { limiter.setThresholdDb (db); }

    void process (juce::AudioBuffer<float>& buffer);

    // --- Metering per la GUI ---
    float getInputLevelDb() const noexcept { return inputLevelDb; }
    float getOutputLevelDb() const noexcept { return outputLevelDb; }
    float getRiderGainDb() const noexcept { return gainRider.getCurrentGainDb(); }
    float getLimiterGainReductionDb() const noexcept { return limiter.getGainReductionDb(); }
    /** Latenza introdotta dal lookahead del limiter: va riportata
     * all'host per il plugin delay compensation (issue #9). */
    int getLatencySamples() const noexcept { return limiter.getLatencySamples(); }
    float getCurrentLoudnessLufs() const noexcept { return loudnessMeter.getLoudnessLufs(); }

private:
    double sampleRate = 44100.0;

    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;

    LoudnessMeter loudnessMeter;
    GainRider gainRider;
    Limiter limiter;

    std::atomic<float> inputLevelDb { -100.0f };
    std::atomic<float> outputLevelDb { -100.0f };
};

} // namespace livellatore

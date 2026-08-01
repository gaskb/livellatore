#pragma once

#include <juce_dsp/juce_dsp.h>
#include <deque>

namespace livellatore
{

/**
 * Misura la loudness percepita in LUFS usando il K-weighting definito in
 * ITU-R BS.1770 (pre-filter shelving + RLB high-pass) e una finestra
 * scorrevole rettangolare (default 3s, stile "short-term loudness").
 *
 * Non implementa il gating relativo/assoluto della loudness integrata:
 * qui serve solo una stima continua per pilotare il gain rider, non una
 * misura conforme per la compliance broadcast.
 */
class LoudnessMeter
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    void setWindowSeconds (float seconds);

    /** Processa un blocco e aggiorna la stima di loudness. Non modifica il buffer. */
    void pushBlock (const juce::AudioBuffer<float>& buffer);

    /** Loudness corrente in LUFS. Ritorna un pavimento (-100 LUFS) in assenza di segnale. */
    float getLoudnessLufs() const noexcept { return currentLufs; }

private:
    struct BlockEnergy
    {
        double sumOfSquares;
        int numSamples;
    };

    void makeKWeightingFilters (double sampleRate);

    double sampleRate = 44100.0;
    int numChannels = 2;
    float windowSeconds = 3.0f;

    std::vector<juce::dsp::IIR::Filter<float>> preFilters;   // stage 1: shelving
    std::vector<juce::dsp::IIR::Filter<float>> rlbFilters;   // stage 2: high-pass
    juce::dsp::IIR::Coefficients<float>::Ptr preCoeffs;
    juce::dsp::IIR::Coefficients<float>::Ptr rlbCoeffs;

    std::deque<BlockEnergy> window;
    double windowSumSquares = 0.0;
    long long windowNumSamples = 0;

    float currentLufs = -100.0f;
};

} // namespace livellatore

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
 * Decisione (issue #1): niente finestra "momentary" (400ms) separata.
 * Quella distinzione serve per la loudness integrata da compliance
 * broadcast (dove il momentary alimenta il gating relativo); qui il solo
 * scopo è pilotare in continuo il gain rider, per cui la finestra singola
 * configurabile (setWindowSeconds) basta — un valore più corto si comporta
 * già come una stima "momentary-like" senza bisogno di una seconda finestra
 * parallela. Se in futuro servisse comunque (es. per un secondo meter in
 * GUI più reattivo), va riaperta come nuovo item di backlog.
 *
 * "Dialogue Mode" (setGatingEnabled): applica lo stesso principio del
 * gating relativo di BS.1770/EBU R128 (quello usato per misurare la
 * loudness del solo dialogo, es. "dialnorm"), adattato a una finestra
 * continua invece che a un intero programma. Con il gating disattivato
 * (default, comportamento invariato) la media della finestra include
 * anche i tratti sotto soglia (silenzio, pause): su musica va bene, su
 * parlato con pause fa scendere molto la loudness misurata ogni volta che
 * c'è una pausa, anche se il parlato stesso non e' cambiato di livello —
 * il rider si ritrova a inseguire una lettura "diluita" dal silenzio
 * invece che il livello reale del parlato. Con il gating attivo, i
 * mini-blocchi (un pushBlock() per volta, non i 400ms del BS.1770
 * originale) sotto la soglia di gate vengono esclusi dalla media: la
 * lettura riflette solo il livello del parlato vero e proprio, restando
 * stabile durante le pause invece di scendere verso il pavimento.
 */
class LoudnessMeter
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    void setWindowSeconds (float seconds);

    /** "Dialogue Mode": esclude dalla media della finestra i mini-blocchi
     * sotto setGateThresholdLufs(). Default disattivo (comportamento
     * invariato, adatto a musica). */
    void setGatingEnabled (bool enabled) noexcept { gatingEnabled = enabled; }
    void setGateThresholdLufs (float lufs) noexcept { gateThresholdLufs = lufs; }

    /** Processa un blocco e aggiorna la stima di loudness. Non modifica il buffer. */
    void pushBlock (const juce::AudioBuffer<float>& buffer);

    /** Loudness corrente in LUFS. Ritorna un pavimento (-100 LUFS) in assenza di segnale. */
    float getLoudnessLufs() const noexcept { return currentLufs; }

private:
    struct BlockEnergy
    {
        double sumOfSquares;
        int numSamples;
        bool includedInGatedSum;
    };

    void makeKWeightingFilters (double sampleRate);

    double sampleRate = 44100.0;
    int numChannels = 2;
    float windowSeconds = 3.0f;

    bool gatingEnabled = false;
    float gateThresholdLufs = -60.0f;

    std::vector<juce::dsp::IIR::Filter<float>> preFilters;   // stage 1: shelving
    std::vector<juce::dsp::IIR::Filter<float>> rlbFilters;   // stage 2: high-pass
    juce::dsp::IIR::Coefficients<float>::Ptr preCoeffs;
    juce::dsp::IIR::Coefficients<float>::Ptr rlbCoeffs;

    std::deque<BlockEnergy> window;
    double windowSumSquares = 0.0;
    long long windowNumSamples = 0;

    // Somme "gated" (Dialogue Mode): stessi dati, ma solo dei mini-blocchi
    // sopra soglia. Tenute in parallelo invece di ricalcolate da zero ad
    // ogni pushBlock, per restare O(1) per campione come il path esistente.
    double gatedSumSquares = 0.0;
    long long gatedNumSamples = 0;

    float currentLufs = -100.0f;
};

} // namespace livellatore

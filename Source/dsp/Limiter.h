#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace livellatore
{

/**
 * Limiter di sicurezza a valle del gain rider: true-peak limiter con
 * lookahead (issue #9), non piu' un wrapper su juce::dsp::Limiter.
 *
 * Decisione (issue #9), presa con dati misurati invece che per supposizione
 * (vedi Tests/LimiterTests.cpp, "Sonda: overshoot..."): juce::dsp::Limiter
 * NON e' adatto al ruolo di rete di sicurezza di questa catena. La sua
 * doc comment lo dichiara esplicitamente ("hard clipper at 0 dB"): il
 * parametro "soglia" regola solo dove inizia la compressione, ma il vero
 * tetto e' un clip fisso a 0dBFS, non alla soglia impostata dall'utente.
 * Misurato: un impulso a 0dBFS con soglia a -6dB attraversava il vecchio
 * wrapper del tutto inalterato (0dB di riduzione, non -6dB) per transienti
 * larghi fino ad almeno 100 campioni (~2ms a 48kHz) — cioe' esattamente il
 * caso d'uso dichiarato di questo limiter (transienti piu' rapidi di
 * quanto il gain rider possa seguire, vedi LevelerEngine.cpp).
 *
 * Implementazione: finestra di lookahead scorrevole (default 5ms, vedi
 * .cpp). Il guadagno necessario a riportare il picco della finestra sotto
 * soglia si applica al campione ritardato di quella stessa finestra, cosi'
 * la riduzione e' gia' pronta PRIMA che il transiente raggiunga l'uscita
 * (attack effettivamente istantaneo entro il lookahead), senza affidarsi a
 * un hard clip a valle. Il rilascio verso guadagno 1.0 resta invece
 * graduale secondo il release time impostato, per non introdurre pumping
 * udibile. Stereo-linked: usa il picco piu' alto fra i canali come
 * rilevatore comune, per non spostare l'immagine stereo.
 *
 * Il lookahead introduce latenza (vedi getLatencySamples()): va riportata
 * all'host per il plugin delay compensation, non e' piu' "a costo zero"
 * come il vecchio wrapper senza lookahead.
 */
class Limiter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setThresholdDb (float db) noexcept { thresholdDb = db; }
    void setReleaseMs (float ms) noexcept { releaseMs = juce::jmax (1.0f, ms); }

    /** Processa in-place e aggiorna la stima di gain reduction. */
    void process (juce::AudioBuffer<float>& buffer);

    /** Gain reduction sample-accurate: il minimo (piu' negativo) applicato
     * nel blocco appena processato, non un'approssimazione a picco di
     * blocco come nella versione precedente. */
    float getGainReductionDb() const noexcept { return currentGainReductionDb; }

    /** Latenza introdotta dal lookahead, in campioni: va riportata
     * all'host (AudioProcessor::setLatencySamples) per un plugin delay
     * compensation corretto. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

private:
    void allocateBuffers();

    double sampleRate = 44100.0;
    float thresholdDb = -0.3f;
    float releaseMs = 100.0f;

    int lookaheadSamples = 1;
    int numChannels = 2;

    // Buffer circolare di ritardo per canale (riproduce in uscita il
    // campione di `lookaheadSamples` fa) e cronologia dei picchi
    // (stereo-linked, comune a tutti i canali) sulla stessa finestra.
    std::vector<std::vector<float>> delayBuffers; // [channel][lookaheadSamples]
    std::vector<float> peakHistory;                // [lookaheadSamples]
    int writeIndex = 0;

    float currentGainLinear = 1.0f;
    float currentGainReductionDb = 0.0f;
};

} // namespace livellatore

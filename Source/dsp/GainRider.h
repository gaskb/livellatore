#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace livellatore
{

/**
 * Calcola il gain correttivo (in dB) da applicare al segnale per portare
 * la loudness misurata verso il target impostato.
 *
 * "Attack" e "release" non agiscono sul segnale audio direttamente (come in
 * un compressore) ma sulla velocità con cui il gain correttivo si muove:
 *  - attack: quanto velocemente il gain si allontana da 0 quando serve una
 *    correzione maggiore di quella attuale.
 *  - release: quanto velocemente il gain torna verso 0 (livello originale)
 *    quando la correzione necessaria si riduce.
 *
 * Gate (vedi issue #2): sotto la soglia impostata il rider smette di
 * inseguire il target e rilassa verso 0, per non amplificare il rumore di
 * fondo in assenza di segnale utile. Usa un trigger di Schmitt (apre/chiude
 * a soglie leggermente diverse) invece di un confronto a soglia singola,
 * per evitare "chattering" quando il segnale oscilla proprio intorno alla
 * soglia. Non esiste un controllo "Gate Speed" separato in questa versione:
 * la velocità di rilascio quando il gate chiude è derivata dal release
 * esistente (decisione presa per non aggiungere un settimo parametro senza
 * un'evidenza concreta che serva davvero — se in ascolto risultasse troppo
 * lento/veloce, va riaperta come nuovo item di backlog).
 */
class GainRider
{
public:
    void prepare (double sampleRate);
    void reset();

    void setTargetLufs (float lufs) noexcept { targetLufs = lufs; }
    void setAttackMs (float ms) noexcept { attackMs = juce::jmax (1.0f, ms); }
    void setReleaseMs (float ms) noexcept { releaseMs = juce::jmax (1.0f, ms); }
    void setMaxCorrectionDb (float db) noexcept { maxCorrectionDb = db; }
    void setGateThresholdLufs (float lufs) noexcept { gateThresholdLufs = lufs; }

    /**
     * Da chiamare una volta per blocco con la loudness corrente misurata e
     * la durata del blocco in secondi (i coefficienti di attack/release
     * sono ricalcolati per-call per restare corretti anche con block size
     * variabile).
     */
    float computeGainDb (float currentLufs, double deltaTimeSeconds) noexcept;

    float getCurrentGainDb() const noexcept { return smoothedGainDb; }
    bool isGateOpen() const noexcept { return gateOpen; }

private:
    double sampleRate = 44100.0;

    float targetLufs = -16.0f;
    float attackMs = 200.0f;
    float releaseMs = 800.0f;
    float maxCorrectionDb = 24.0f;
    float gateThresholdLufs = -60.0f;

    // Larghezza dell'isteresi del gate (schmitt trigger): apre sopra
    // (soglia + metà banda), chiude sotto (soglia - metà banda).
    static constexpr float gateHysteresisDb = 3.0f;

    bool gateOpen = true;
    float smoothedGainDb = 0.0f;
};

} // namespace livellatore

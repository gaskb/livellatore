#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace livellatore
{

/**
 * Meter verticale generico, riusato per input, output e "attività" del
 * gain rider (quanto sta correggendo in questo momento). Il livello viene
 * fornito dall'esterno (poll via Timer sul processor) invece che letto
 * direttamente dal motore audio, per non toccare dati audio-thread dal
 * message thread.
 *
 * Modalità "bipolar" (issue #4): il gain del rider può essere sia positivo
 * (boost) che negativo (taglio), a differenza di input/output level che
 * sono sempre >= 0. Un meter che riempie dal basso come per un livello
 * assoluto renderebbe un +3dB e un -3dB come "quanto sta lavorando" uguale
 * in altezza ma la stessa direzione visiva: fuorviante. In modalità
 * bipolar il riempimento parte da una linea centrale (0dB) verso l'alto
 * per correzioni positive e verso il basso per quelle negative, con range
 * simmetrico attorno allo zero.
 */
class VuMeterComponent : public juce::Component, private juce::Timer
{
public:
    VuMeterComponent (juce::String label, float rangeMinDb, float rangeMaxDb, bool bipolar = false);

    /** Thread-safe: chiamabile dall'audio thread. */
    void setLevelDb (float db) noexcept { targetLevelDb.store (db); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    juce::String labelText;
    float rangeMinDb, rangeMaxDb;
    bool bipolar;

    std::atomic<float> targetLevelDb { -100.0f };
    float displayedLevelDb = -100.0f;
};

} // namespace livellatore

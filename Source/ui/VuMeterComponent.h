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
 */
class VuMeterComponent : public juce::Component, private juce::Timer
{
public:
    VuMeterComponent (juce::String label, float rangeMinDb, float rangeMaxDb);

    /** Thread-safe: chiamabile dall'audio thread. */
    void setLevelDb (float db) noexcept { targetLevelDb.store (db); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    juce::String labelText;
    float rangeMinDb, rangeMaxDb;

    std::atomic<float> targetLevelDb { -100.0f };
    float displayedLevelDb = -100.0f;
};

} // namespace livellatore

# Livellatore

Plugin audio (VST3/AU) di auto-leveling, ispirato a Sonic Anomaly TriLeveler:
misura la loudness in ingresso e alza/abbassa automaticamente il gain per
mantenere l'uscita vicina a un target impostabile (in LUFS), con limiter di
sicurezza sui transienti.

## Controlli

- **Input Gain** — corregge il livello di ingresso prima della misura.
- **Target Level** — loudness desiderata in uscita (LUFS).
- **Attack** — velocità con cui il gain rider aumenta la correzione quando serve.
- **Release** — velocità con cui il gain rider torna verso 0 (livello originale) quando la correzione non serve più.
- **Limiter** — soglia del limiter di sicurezza a valle, per i transienti troppo rapidi per il rider.
- **Output Gain** — makeup gain finale, indipendente dal rider.

## Struttura del progetto

```
Source/
  PluginProcessor.*   Wrapper JUCE AudioProcessor, APVTS, ponte verso l'engine DSP
  PluginEditor.*       GUI
  Parameters.h         Definizione parametri APVTS
  dsp/
    LoudnessMeter.*     Loudness K-weighted (ITU-R BS.1770) a finestra scorrevole
    GainRider.*         Calcolo del gain correttivo con attack/release
    Limiter.*           Wrapper su juce::dsp::Limiter
    LevelerEngine.*     Orchestrazione della catena (indipendente da JUCE plugin wrapper)
  ui/
    LivellatoreLookAndFeel.*  Stile grafico (slider, colori)
    VuMeterComponent.*        Meter verticale riusabile (input/output/rider)
    LevelSliderComponent.*    Slider orizzontale con label + attachment APVTS
Tests/
  DspTests.cpp          Unit test JUCE UnitTest sulla parte DSP (engine-agnostic)
```

La logica DSP (`Source/dsp/`) non dipende dal wrapper plugin JUCE: prende
buffer e parametri "grezzi", il che la rende testabile in isolamento (vedi
`Tests/`).

## Build

Richiede CMake ≥ 3.22 e una toolchain C++20. JUCE viene scaricato
automaticamente al primo configure via `FetchContent` (serve rete).

```sh
cmake -B build
cmake --build build --config Release
ctest --test-dir build
```

## Stato

Progetto agli inizi: la struttura è uno scheletro funzionante (compila,
processa audio, ha GUI e test) ma diversi punti sono volutamente
semplificati per iterare via backlog — vedi le issue su GitLab
(`gas/livellatore`) per il dettaglio.

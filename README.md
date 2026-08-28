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

## Packaging e distribuzione

Il build locale (`cmake --build build --config Release`) produce VST3, AU
(solo macOS, via `COPY_PLUGIN_AFTER_BUILD`) e Standalone, installati
automaticamente in `~/Library/Audio/Plug-Ins/` — con firma **ad-hoc**,
sufficiente per sviluppare sulla propria macchina ma non per condividere il
plugin con altri (Gatekeeper la rifiuterebbe su un'altra macchina).

- **AU validato con `auval`**: `auval -v aufx Lvlr Gasx` — PASS (tipo
  `aufx`/Effect, non `aumf`: essendo un plugin senza MIDI, JUCE lo registra
  come Effect semplice, non Music Effect).
- **VST3 validato con [pluginval](https://github.com/Tracktion/pluginval)**
  (`brew install --cask pluginval`): `pluginval --strictness-level 10
  --validate build/Livellatore_artefacts/VST3/Livellatore.vst3` — SUCCESS,
  incluso il fuzzing dei parametri, su 44.1/48/96kHz e block size 64-1024.
- **Pacchetto ad-hoc**: `./scripts/package_macos.sh` crea uno zip con
  VST3+AU+Standalone+README, utilizzabile per condividere il plugin (chi lo
  riceve dovrà comunque approvarlo a mano su Gatekeeper — clic destro >
  Apri, o `xattr -dr com.apple.quarantine`).
- **Firma con Developer ID reale + notarization Apple**: template in
  `scripts/notarize_macos.sh`, MAI eseguito in automatico — richiede un
  account Apple Developer Program a pagamento e le credenziali personali
  dell'utente, che non vanno mai condivise con un LLM. Va lanciato a mano
  quando si arriva al punto di distribuire pubblicamente il plugin.
- **Installer .pkg**: non ancora fatto; per un plugin agli inizi uno zip
  con istruzioni è sufficiente, un installer vero e proprio (`pkgbuild`/
  `productbuild`) è rimandato a quando servirà davvero.

### Windows e Linux

Il `CMakeLists.txt` è predisposto per essere cross-platform (formato AU
incluso solo su Apple via `if (APPLE)`; VST3 e Standalone sono già
platform-agnostic in JUCE), ma **non è stato compilato né testato** su
Windows o Linux: questo repository è stato sviluppato solo su macOS e non
c'è a disposizione una macchina/toolchain Windows o Linux per verificarlo.
Su Linux, JUCE richiede in più i pacchetti di sviluppo di sistema (tipici:
`libasound2-dev`, `libx11-dev`, `libfreetype-dev`, `libfontconfig1-dev`,
`libcurl4-openssl-dev` — quest'ultimo non serve qui, `JUCE_USE_CURL=0` è
già impostato). Su Windows serve Visual Studio (MSVC) con il workload
"Desktop development with C++". Se/quando si builda su una di queste
piattaforme, va aggiornato questo paragrafo con l'esito reale, non un
"dovrebbe funzionare".

## Stato

Progetto agli inizi: la struttura è uno scheletro funzionante (compila,
processa audio, ha GUI e test) ma diversi punti sono volutamente
semplificati per iterare via backlog — vedi le issue su GitLab
(`gas/livellatore`) per il dettaglio.

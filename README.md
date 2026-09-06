# Livellatore

*[Italiano sotto / Italian below](#livellatore-italiano)*

Auto-leveling audio plugin (VST3/AU), inspired by Sonic Anomaly TriLeveler: measures input loudness and automatically raises/lowers gain to keep the output close to a settable target (in LUFS), with a safety limiter on transients.

## Controls

- **Input Gain** — corrects the input level before measurement.
- **Target Level** — desired output loudness (LUFS).
- **Attack** — how fast the gain rider increases the correction when needed.
- **Release** — how fast the gain rider returns toward 0 (original level) once the correction is no longer needed.
- **Max Correction** (0–20dB) + **Limit Rider Range** — caps the rider's correction to +/- the set value instead of the default safety ceiling; off by default (no explicit limit).
- **Gate Threshold** (LUFS) — below this threshold the rider stops chasing the target and relaxes toward 0, so it doesn't amplify background noise when there's no useful signal (3dB hysteresis to avoid chattering).
- **Dialogue Mode** — excludes below-gate-threshold portions from the loudness measurement, so pauses in speech don't drag the reading down (relative gating in the style of BS.1770/EBU R128, adapted to a rolling window). Off by default (suited to music); turn it on for speech/podcasts with pauses.
- **Limiter** — downstream safety limiter threshold (true 5ms lookahead, sample-accurate gain reduction), for transients too fast for the rider to follow.
- **Output Gain** — final makeup gain, independent of the rider.

## Project layout

```
Source/
  PluginProcessor.*   JUCE AudioProcessor wrapper, APVTS, bridge to the DSP engine
  PluginEditor.*       GUI
  Parameters.h         APVTS parameter definitions
  dsp/
    LoudnessMeter.*     K-weighted loudness (ITU-R BS.1770) over a rolling window
    GainRider.*         Corrective gain calculation with attack/release
    Limiter.*           Wrapper around juce::dsp::Limiter
    LevelerEngine.*     Chain orchestration (independent of the JUCE plugin wrapper)
  ui/
    LivellatoreLookAndFeel.*  Visual style (sliders, colors)
    VuMeterComponent.*        Reusable vertical meter (input/output/rider/limiter GR)
    LevelSliderComponent.*    Horizontal slider with label + APVTS attachment
Tests/
  DspTests.cpp          JUCE UnitTest unit tests on the DSP side (engine-agnostic)
```

The DSP logic (`Source/dsp/`) doesn't depend on the JUCE plugin wrapper: it
takes raw buffers and parameters, which makes it testable in isolation (see
`Tests/`).

## Build

Requires CMake ≥ 3.22 and a C++20 toolchain. JUCE is downloaded automatically
on first configure via `FetchContent` (needs network access).

```sh
cmake -B build
cmake --build build --config Release
ctest --test-dir build
```

## Packaging and distribution

A local build (`cmake --build build --config Release`) produces VST3, AU
(macOS only, via `COPY_PLUGIN_AFTER_BUILD`) and Standalone, automatically
installed into `~/Library/Audio/Plug-Ins/` — with an **ad-hoc** signature,
enough for development on your own machine but not for sharing the plugin
with others (Gatekeeper would reject it on another machine).

- **AU validated with `auval`**: `auval -v aufx Lvlr Gasx` — PASS (type
  `aufx`/Effect, not `aumf`: since it's a plugin with no MIDI, JUCE
  registers it as a plain Effect, not a Music Effect).
- **VST3 validated with [pluginval](https://github.com/Tracktion/pluginval)**
  (`brew install --cask pluginval`): `pluginval --strictness-level 10
  --validate build/Livellatore_artefacts/VST3/Livellatore.vst3` — SUCCESS,
  including parameter fuzzing, across 44.1/48/96kHz and block sizes 64-1024.
- **Ad-hoc package**: `./scripts/package_macos.sh` creates a zip with
  VST3+AU+Standalone+README, usable to share the plugin (whoever receives
  it will still need to approve it manually past Gatekeeper — right-click >
  Open, or `xattr -dr com.apple.quarantine`).
- **Signing with a real Developer ID + Apple notarization**: template in
  `scripts/notarize_macos.sh`, NEVER run automatically — it requires a paid
  Apple Developer Program account and the user's personal credentials,
  which should never be shared with an LLM. Run it by hand once you're
  ready to distribute the plugin publicly.
- **.pkg installer**: not done yet; for an early-stage plugin a zip with
  instructions is enough, a proper installer (`pkgbuild`/`productbuild`) is
  left for whenever it's actually needed.
- **Installed-version check**: `./scripts/check_version.sh` compares the
  version declared in `project(Livellatore VERSION X.Y.Z)` with the one
  actually installed in `~/Library/Audio/Plug-Ins/` (the one a DAW sees).
  Born from a real incident: `cmake -B build` immediately regenerates the
  Info.plist/moduleinfo.json inside the build folder for every target
  (a configure step), but the installed copy only updates once that target
  is actually rebuilt — a `cmake --build build --target <only-some-targets>`
  can leave the source already updated while the installed plugin stays on
  the previous version, with no error at all. **After every version bump,
  do a full build with `cmake --build build --config Release`** (not
  partial targets) and then run this script to confirm.

### Windows and Linux

`CMakeLists.txt` is set up to be cross-platform (the AU format is only
included on Apple via `if (APPLE)`; VST3 and Standalone are already
platform-agnostic in JUCE), but it **has not been built or tested** on
Windows or Linux: this repository has only been developed on macOS, and
there's no Windows/Linux machine or toolchain available to verify it. On
Linux, JUCE additionally needs system development packages (typically
`libasound2-dev`, `libx11-dev`, `libfreetype-dev`, `libfontconfig1-dev`,
`libcurl4-openssl-dev` — the last one isn't needed here, `JUCE_USE_CURL=0`
is already set). On Windows, Visual Studio (MSVC) with the "Desktop
development with C++" workload is required. If/when this gets built on one
of these platforms, this paragraph should be updated with the real outcome,
not a "should work."

## Status

Early-stage project: the structure is a working skeleton (compiles,
processes audio, has a GUI and tests) but several parts are deliberately
simplified to be iterated on via the backlog — see the issues on GitLab
(`gas/music/livellatore`) for details.

## License

This project is distributed under the **GNU Affero General Public License
v3.0** (see [LICENSE](LICENSE)) — chosen to stay compliant with JUCE's
license terms without a paid commercial license: distributing a binary
built with JUCE requires either a commercial JUCE license or using its
AGPLv3 option, and since this repository is fully open source anyway,
AGPLv3 is the natural fit, at no cost and with no JUCE account to set up.

AGPLv3 is a strong copyleft license: anyone distributing a modified
version of this code (including running it as a network service) must in
turn make its source available under the same terms.

Copyright (C) 2026 Gas

Livellatore is free. If you find it useful, feel free to [buy me a coffee on Ko-fi](https://ko-fi.com/gaskb) — no obligation at all.

---

# Livellatore (Italiano)

Plugin audio (VST3/AU) di auto-leveling, ispirato a Sonic Anomaly TriLeveler:
misura la loudness in ingresso e alza/abbassa automaticamente il gain per
mantenere l'uscita vicina a un target impostabile (in LUFS), con limiter di
sicurezza sui transienti.

## Controlli

- **Input Gain** — corregge il livello di ingresso prima della misura.
- **Target Level** — loudness desiderata in uscita (LUFS).
- **Attack** — velocità con cui il gain rider aumenta la correzione quando serve.
- **Release** — velocità con cui il gain rider torna verso 0 (livello originale) quando la correzione non serve più.
- **Max Correction** (0–20dB) + **Limit Rider Range** — limita la correzione del rider a +/- il valore impostato invece del tetto di sicurezza di default; disattivato di default (nessun limite esplicito).
- **Gate Threshold** (LUFS) — sotto questa soglia il rider smette di inseguire il target e rilassa verso 0, per non amplificare il rumore di fondo in assenza di segnale utile (isteresi di 3dB per evitare chattering).
- **Dialogue Mode** — esclude dalla misura di loudness i tratti sotto la soglia di gate, così le pause nel parlato non fanno scendere la lettura (gating relativo in stile BS.1770/EBU R128, adattato a una finestra continua). Disattivato di default (adatto alla musica); attivarlo per parlato/podcast con pause.
- **Limiter** — soglia del limiter di sicurezza a valle (vero lookahead di 5ms, gain reduction sample-accurate), per i transienti troppo rapidi per il rider.
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
    VuMeterComponent.*        Meter verticale riusabile (input/output/rider/limiter GR)
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
- **Verifica versione installata**: `./scripts/check_version.sh` confronta
  la versione dichiarata in `project(Livellatore VERSION X.Y.Z)` con
  quella effettivamente installata in `~/Library/Audio/Plug-Ins/` (quella
  che vede un DAW). Nato da un caso reale: `cmake -B build` rigenera
  subito l'Info.plist/moduleinfo.json nella cartella di build per ogni
  target (è un passo di configure), ma la copia installata si aggiorna
  solo quando quel target viene davvero ricompilato — un
  `cmake --build build --target <solo-alcuni-target>` può lasciare il
  sorgente già aggiornato ma il plugin installato ancora fermo alla
  versione precedente, senza nessun errore. **Dopo ogni bump di versione,
  buildare per intero con `cmake --build build --config Release`** (non
  target parziali) e poi lanciare questo script per conferma.

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
(`gas/music/livellatore`) per il dettaglio.

## Licenza

Questo progetto è distribuito sotto **GNU Affero General Public License
v3.0** (vedi [LICENSE](LICENSE)) — scelta per restare conforme ai termini
di licenza di JUCE senza una licenza commerciale a pagamento: distribuire
un binario costruito con JUCE richiede o una licenza commerciale JUCE o
l'uso della sua opzione AGPLv3, e dato che questo repository è comunque
interamente open source l'AGPLv3 è la scelta naturale, senza costi né
account JUCE da configurare.

L'AGPLv3 è una licenza copyleft forte: chiunque distribuisca una versione
modificata di questo codice (incluso l'uso come servizio di rete) deve a
sua volta renderne disponibile il sorgente sotto gli stessi termini.

Copyright (C) 2026 Gas

Livellatore è gratis. Se ti torna utile, sentiti libero di [offrirmi un caffè su Ko-fi](https://ko-fi.com/gaskb) — nessun obbligo.

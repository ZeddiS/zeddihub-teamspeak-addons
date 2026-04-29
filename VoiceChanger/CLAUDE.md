# Voice Changer — Plugin notes for Claude

> Kontextová paměť pro Claude napříč konverzacemi ohledně **Voice Changer**
> pluginu. Před editací čti vždy **TENTO soubor**.

## Co Voice Changer dělá

TS3 plugin. Hookne se do `ts3plugin_onEditCapturedVoiceDataEvent` a aplikuje
real-time DSP efekty na výchozí mikrofonní stream **předtím** než ho TS3 pošle
na server.

**Top menu** (PLUGIN_MENU_TYPE_GLOBAL — Plugins menu v top baru):
1. `Voice Changer Settings...` — otevře Qt dialog (combobox hlasů, Echo group, Enable/Disable buttons)
2. `Enable Voice Changer` — quick-toggle ON
3. `Disable Voice Changer` — quick-toggle OFF

**Chat commands**: `/voicechanger on|off|settings`

### Voice presets

| Preset | DSP | Ratio / param |
|---|---|---|
| Off | passthrough | — (samples nezměněny, edited=0) |
| Helium | pitch shift | +6 půltónů (ratio 2^(6/12) ≈ 1.414) |
| Chipmunk | pitch shift | +12 půltónů (ratio 2.0) |
| Deep | pitch shift | -4 půltóny (ratio ≈ 0.794) |
| Demon | pitch shift | -7 půltónů (ratio ≈ 0.667) |
| Robot | ring modulation | sin(2π × 30Hz × t) — bzučivý robot |
| Echo | delay buffer | default 200ms delay, 30% feedback, do 500ms |
| Custom | pitch shift | -12 .. +12 půltónů, slider v dialogu |

## Architektura

```
VoiceChanger/
├── CMakeLists.txt              ← Qt 5.15 + ${VC_API_VERSION}
├── CLAUDE.md                   ← tento soubor
├── vendor/
│   └── smb_pitch_shift.h       ← phase-vocoder pitch shifter (Bernsee, PD)
└── src/
    ├── plugin.cpp              ← TS3 exporty, top menu, audio callback hook
    ├── voice_engine.h/.cpp     ← VoiceEngine: DSP dispatch, mutex pro config
    ├── settings_dialog.h       ← run(VoiceEngine&) entry point
    └── settings_dialog.cpp     ← Qt dialog s TS3 dark theme + Live update
```

### Klíčové třídy

| Symbol | Soubor | Účel |
|---|---|---|
| `VoicePreset` enum | voice_engine.h | Off/Helium/Chipmunk/Demon/Deep/Robot/Echo/Custom |
| `VoiceConfig` | voice_engine.h | preset, customSemitones, echoDelay, echoFeedback, robotHz, enabled |
| `VoiceEngine` | voice_engine.cpp | `processSamples()` na audio thread, `setConfig()` thread-safe (mutex) |
| `pitchshift::PitchShifter` | vendor/smb_pitch_shift.h | Phase vocoder, FFT 2048, oversampling 4 |
| `settings_dialog::run()` | settings_dialog.cpp | Modální Qt dialog, live update zatím co je open |

### Audio callback flow

```
TS3 audio thread:
  ts3plugin_onEditCapturedVoiceDataEvent(schid, samples, count, channels, &edited)
    → VoiceEngine::processSamples(samples, count, channels)
      → if !enabled or preset==Off: return false
      → mutex lock pro snapshot config
      → int16 → float32 (scale 1/32768)
      → switch(preset):
          Helium/Demon/etc → applyPitchShift(buf, n, ratio)
                            → pitchShifter_.process(ratio, n, 48000, in, out)
          Robot → applyRobot — ring mod sin(2π*30Hz*t)
          Echo → applyEcho — circular buffer 500ms max
      → float32 → int16 (clamped ±32767)
      → return true
    → if returned true: edited = 1
```

### Thread safety

- **Audio thread** volá `processSamples()` ~100×/sec (10ms chunks).
- **UI thread** volá `setConfig()` při klikatkách v dialogu.
- Sdílený `VoiceConfig` chráněn `std::mutex mu_`.
- `enabled` jako `std::atomic<bool>` pro fast-path bez locku.
- `processSamples()` snapshotuje config pod lockem, pak DSP bez locku.

### Pitch shifter detaily

Bernsee phase-vocoder:
- **fftFrameSize = 2048** (43ms @ 48kHz)
- **osamp = 4** → hop size 512 (10.7ms)
- **inFifoLatency = 1536** samples (32ms latency cost)
- Magnitude-only spectral shift, zachovává formant rough
- Cca **40ms latence** — slyšitelné ale ne otravné

## Build

API přes `-DVC_API_VERSION=23|24|25|26` (default 26).

```powershell
& $cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 `
         -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" -DVC_API_VERSION=25
& $cmake --build build_api25 --config Release
```

Output: `build_api25/Release/voicechanger_api25_win64.dll` (~150-300 KB).

## Pitfalls / gotchas

- **Sample rate fixed at 48000** — TS3 capture je vždy 48kHz mono. Pokud by se to změnilo, `kSampleRate` v voice_engine.h by se musel přepočítat (a pitch shifter by potřeboval reset).
- **`edited` flag** — TS3 pošle modifikované samples na server JEN pokud nastavíš `*edited = 1`. Bez toho TS3 použije ORIGINÁLNÍ samples a tvoje práce se zahodí.
- **Statické pole v PitchShifter** — `MAX_FRAME_LENGTH = 8192`. Float pole = 8192 × 4 B × 8 polí ≈ 256 KB per instance. OK pro single instance, problém pro per-channel pitch (nemáme).
- **PitchShifter::reset()** — volat při změně presetu, jinak FFT state způsobí klikací artefakty.
- **Echo buffer 500ms hard max** — `echoBuf_` allokované na 500ms × 48kHz × 4 B = 96 KB. Delší delay = silent clamp.
- **Robot phase wrap** — `robotPhase_` se reset každou sekundu (sample count) aby float precision neutekla.
- **PLUGIN_MENU_TYPE_GLOBAL** — top menu items, NE pravý klik. Nezaměňovat s CLIENT type.
- **`edited = nullptr`** — některé starší TS3 verze volají s null pointerem. Engine to checkuje (`if (edited) *edited = 1`).
- **Multi-instance** — TS3 má jeden capture stream → jeden VoiceEngine instance. Nemixovat per-server.

## TS3 SDK funkce, které používáme

```c
// Audio callback (přijímáme volání):
ts3plugin_onEditCapturedVoiceDataEvent(schid, samples, sampleCount, channels, &edited)

// Volané:
ts3Functions.getCurrentServerConnectionHandlerID()
ts3Functions.printMessage(schid, text, target)
ts3Functions.logMessage(msg, level, channel, schid)
ts3Functions.freeMemory(ptr)
```

## Verze

- 1.0.0 (2026-04-30) — Initial release: Helium/Chipmunk/Deep/Demon (pitch),
  Robot (ring mod), Echo (delay), Custom (slider). Phase-vocoder pitch shift.

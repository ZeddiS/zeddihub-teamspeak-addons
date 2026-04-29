# Poke Bot — Plugin notes for Claude

> Tento soubor je kontextová paměť pro Claude Code napříč konverzacemi
> ohledně **Poke Bot** pluginu. Před editací čti vždy **TENTO soubor**
> a teprve pak explorace.

## Co Poke Bot dělá

TS3 plugin. Pravým klikem na uživatele → preset poke kampaně + Custom dialog.

**Menu items** (PLUGIN_MENU_TYPE_CLIENT):
1. `Poke Bot: Wake-up CZ` — burst 20 českých "vstávej" hlášek (~250ms)
2. `Poke Bot: Halt! (interval)` — 10× v intervalu 3-6s, stop-fráze
3. `Poke Bot: Symbol Storm` — burst symbolů, chunked po 100 znacích
4. `Poke Bot: Silent poke` — 8× tichý poke v intervalu 2-4.5s
5. `Poke Bot: MAX SPAM` — hard cap 500× s 50ms delay (anti-flood floor) — RISK kick
6. `Poke Bot: Custom...` — Qt dialog (Burst/Schedule mode toggle, slidery, TS3 dark theme)
7. `Poke Bot: STOP` — okamžité přerušení běžící kampaně

**Chat command**: `/zhpoke status`, `/zhpoke stop`

## Architektura

```
pokebot/
├── CMakeLists.txt              ← build s Qt 5.15 + ${POKEBOT_API_VERSION}
├── CLAUDE.md                   ← tento soubor
├── BUILD.md                    ← user-facing build návod
├── README.md                   ← user-facing přehled
├── src/
│   ├── plugin.cpp              ← TS3 SDK exporty + menu dispatch
│   ├── poke_engine.h/.cpp      ← worker thread, Burst/Schedule timing, safety brakes
│   ├── presets.h               ← 5 preset PokeJob configs (header-only inline funkce)
│   └── custom_dialog.h/.cpp    ← Qt 5 dialog s TS3 dark theme + paired slider/spinbox
└── ts3sdk/                     ← shared SDK na úrovni rootu (../ts3sdk/)
```

### Klíčové třídy/funkce

| Symbol | Soubor | Účel |
|---|---|---|
| `PokeEngine` | poke_engine.h | Worker thread, drží `std::atomic<bool> running_`, `std::condition_variable` pro cancellation, hard-cap 500 poke, min 50ms delay |
| `PokeJob` | poke_engine.h | POD struct: schid, clientID, mode (Burst/Schedule), messages, count, intervalMin/Max, burstDelayMs |
| `presets::wakeupCz/halt/symbolStorm/silent/maxSpam` | presets.h | Preset job factories |
| `presets::chunk(s, n=100)` | presets.h | Helper pro split textu na 100-znakové části |
| `custom_dialog::run(schid, clientID, engine&)` | custom_dialog.cpp | Otevře Qt modální dialog, na accept volá engine.start() |
| `kStyleSheet` | custom_dialog.cpp | TS3 dark theme — bg #2b2d31, accent #5865f2 |
| `ts3plugin_*` | plugin.cpp | TS3 SDK exporty — name/version/apiVersion/init/shutdown/menu/freeMemory/requestAutoload/processCommand |

### TS3 plugin SDK exporty (musí všechny existovat)
- `ts3plugin_name/version/apiVersion/author/description`
- `ts3plugin_setFunctionPointers/init/shutdown`
- `ts3plugin_registerPluginID`
- `ts3plugin_freeMemory` ⭐ **bez tohoto TS3 nezobrazí menu**
- `ts3plugin_requestAutoload` (vrací 0)
- `ts3plugin_commandKeyword/processCommand` — `/zhpoke ...` chat příkazy
- `ts3plugin_initMenus/onMenuItemEvent`

## Build

API verze přes CMake flag `-DPOKEBOT_API_VERSION=23|24|25|26` (default 26).

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$qt = "C:\Qt\5.15.2\msvc2019_64"

& $cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 `
         -DCMAKE_PREFIX_PATH=$qt -DPOKEBOT_API_VERSION=25
& $cmake --build build_api25 --config Release
```

Output: `build_api25/Release/zeddihub_pokebot_api25_win64.dll` (~80-200 KB s Qt linkováním).

Nebo top-level `build_all.ps1` zbuildí všechny 4 API verze do `dist/pokebot/`.

## Pitfalls / gotchas

- **TS3 vyžaduje `ts3plugin_freeMemory`.** Bez něj plugin se zapne, ale menu se nezobrazí.
- **Qt verze musí přesně sedět** (5.15.2 MSVC 2019 64-bit). Jiná verze = ABI crash při loadu.
- **Burst delay < 50ms** dostane uživatele kicknutého anti-floodem. Engine vynucuje min 50ms hard floor.
- **Hard cap 500 poke** v `kHardCapPokes` v poke_engine.cpp. Změna vyžaduje rebuild.
- **TS3 process locks DLL** — pokud TS3 běží, Copy-Item DLL do plugins folderu selže silently. Vždy zavřít TS3 nebo unload plugin.
- **`POKEBOT_API_VERSION` cache cleanout** — když měníš API ver, smaž `build_apiXX/` jinak CMake cachuje starou hodnotu.
- **GetDlgItem** (ne `GetDlgItemW`) — ale s Qt už neaktuální.

## API verze

| API | Klient | DLL název |
|---|---|---|
| 23 | TS3 ~3.5.0 | `zeddihub_pokebot_api23_win64.dll` |
| 24 | TS3 3.5.1-3.5.5 | `zeddihub_pokebot_api24_win64.dll` |
| 25 | **TS3 3.5.6** | `zeddihub_pokebot_api25_win64.dll` |
| 26 | TS3 3.6.x+ | `zeddihub_pokebot_api26_win64.dll` |

## Verze

- 1.0.0 (2026-04-29) — Initial release, Win32 dialog
- 1.0.1 (2026-04-29) — Configurable API version (25/26)
- 1.1.0 (2026-04-29) — Renamed to "Poke Bot", added freeMemory + requestAutoload
- 1.2.0 (2026-04-30) — Qt 5 dialog s TS3 dark theme, MAX SPAM preset, multi-API support (23/24/25/26)

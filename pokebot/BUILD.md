# Build návod — ZeddiHub Pokebot

Plugin existuje ve dvou variantách:

| Varianta | DLL | Pro TS3 klient |
|---|---|---|
| **API 25** | `zeddihub_pokebot_api25_win64.dll` | **3.5.6** a starší (max API 25) |
| **API 26** | `zeddihub_pokebot_api26_win64.dll` | 3.6.x a novější |

Vyber si tu, která odpovídá tvému klientu (Settings → About → ukáže verzi;
nebo zkus načíst plugin a TS3 napíše "minimum X, current Y" pokud nesedí).

## Požadavky

| Komponenta | Verze | Poznámka |
|---|---|---|
| Visual Studio | 2019 nebo 2022 (BuildTools stačí) | C++ Desktop workload + MSVC v142/v143 |
| CMake | ≥ 3.16 | VS BuildTools jej obsahuje (`Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) |
| TS3 Plugin SDK | 3.3.0+ | Viz [`ts3sdk/PLACE_SDK_HERE.md`](ts3sdk/PLACE_SDK_HERE.md) |

> **Žádná Qt dependency.** Custom dialog je psaný v Win32 (windows.h + comctl32),
> takže build vyžaduje jen MSVC + Windows SDK (oboje součástí VS BuildTools).

## Instalace závislostí (jednorázově)

### 1. Visual Studio Build Tools 2022
- https://visualstudio.microsoft.com/downloads/  → "Tools for Visual Studio" → **Build Tools for Visual Studio 2022**
- V instalátoru zaškrtni: **Desktop development with C++**
  - obsahuje: MSVC compiler, Windows SDK, CMake

### 2. TS3 Plugin SDK
```powershell
cd C:\Users\12voj\Documents\zeddihub-teamspeak-addons\pokebot
git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone
Move-Item ts3sdk_clone\include ts3sdk\include
Remove-Item -Recurse -Force ts3sdk_clone
```

Po této operaci musí existovat soubor:
`pokebot\ts3sdk\include\plugin_definitions.h`

## Build

### API verze přes CMake flag

API verze se ovládá CMake parametrem `-DPOKEBOT_API_VERSION=25` (nebo `26`).
Default = 26.

### Z Developer PowerShell for VS 2022

```powershell
cd C:\Users\12voj\Documents\zeddihub-teamspeak-addons\pokebot

# API 25 (TS3 3.5.6)
cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 -DPOKEBOT_API_VERSION=25
cmake --build build_api25 --config Release

# API 26 (TS3 3.6.x+)
cmake -S . -B build_api26 -G "Visual Studio 17 2022" -A x64 -DPOKEBOT_API_VERSION=26
cmake --build build_api26 --config Release
```

### Z normálního PowerShellu (s plnou cestou ke cmake)

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$proj  = "C:\Users\12voj\Documents\zeddihub-teamspeak-addons\pokebot"

foreach ($api in 25, 26) {
    & $cmake -S $proj -B "$proj\build_api$api" -G "Visual Studio 17 2022" -A x64 -DPOKEBOT_API_VERSION=$api
    & $cmake --build "$proj\build_api$api" --config Release
}
```

Výstup:
```
build_api25\Release\zeddihub_pokebot_api25_win64.dll  (~40 KB, pro TS3 3.5.6)
build_api26\Release\zeddihub_pokebot_api26_win64.dll  (~40 KB, pro TS3 3.6.x+)
```

## Instalace pluginu

### Manuálně
Zkopíruj **správnou variantu** do:
```
%APPDATA%\TS3Client\plugins\
```
(typicky `C:\Users\<USER>\AppData\Roaming\TS3Client\plugins\`)

> **Důležité:** instaluj **jen jednu variantu** (API 25 nebo API 26).
> Ne obě zároveň — TS3 by načetl obě a měl jsi v menu duplicitní položky.

### Jednořádkově (API 25 — pro TS3 3.5.6)
```powershell
Copy-Item ".\build_api25\Release\zeddihub_pokebot_api25_win64.dll" `
          "$env:APPDATA\TS3Client\plugins\" -Force
```

### Jednořádkově (API 26 — pro TS3 3.6.x+)
```powershell
Copy-Item ".\build_api26\Release\zeddihub_pokebot_api26_win64.dll" `
          "$env:APPDATA\TS3Client\plugins\" -Force
```

### Aktivace
1. Restartuj TS3 klienta (pokud běžel)
2. **Settings → Plugins** → najdi "ZeddiHub Pokebot"
3. Zaškrtni **Enabled**

## Použití

1. Pravým na uživatele → vyber jednu z položek `ZH Pokebot: ...`:
   - **Wake-up CZ** — burst 20 různých českých hlášek (~250ms mezi)
   - **Halt!** — 10 poke se stop-frází, interval 3-6s
   - **Symbol Storm** — burst dlouhého řetězce symbolů, sekáno na chunky 100 znaků
   - **Silent poke** — 8 tichých poke (mezera), interval 2-4.5s
   - **Custom...** — Win32 dialog: vlastní text, count, interval, mode
   - **STOP** — okamžité přerušení běžící kampaně

2. CLI alternativa v chat tabu:
   ```
   /zhpoke status
   /zhpoke stop
   ```

## Bezpečnostní brzdy

- **Hard cap:** max 500 poke per kampaň (změň v `poke_engine.cpp` → `kHardCapPokes`)
- **Min delay:** 50ms mezi poke (anti-flood; TS3 servery často kickují při <50ms)
- **Single campaign:** start nové kampaně auto-stopne tu předchozí

## Troubleshooting

| Symptom | Příčina | Fix |
|---|---|---|
| Plugin v Settings → Plugins není | Špatná lokace DLL | Ověř `%APPDATA%\TS3Client\plugins\zeddihub_pokebot_win64.dll` |
| Plugin viditelný, ale zašedlý ("API not compatible: X, current Y") | API version mismatch | Použij DLL odpovídající tvému klientu — API 25 pro 3.5.6, API 26 pro 3.6.x+. Build s `-DPOKEBOT_API_VERSION=<X>` |
| "requestClientPoke failed" v TS logu | Anti-flood server | Zvýš `burstDelayMs` v custom dialogu (200ms+) |
| Server kickne uživatele při burst | Anti-flood server | Použij Schedule mode místo Burst, nebo delší delay |
| Czech znaky se zobrazují špatně | UTF-8 v MSVC | Build už zapíná `/utf-8`. Zkontroluj že soubory jsou ve UTF-8 BOM. |

## Verze

- 1.0.0 (2026-04-29) — initial release (Win32 dialogs, no Qt)
- 1.0.1 (2026-04-29) — `POKEBOT_API_VERSION` CMake flag, dual-build pro API 25 + API 26

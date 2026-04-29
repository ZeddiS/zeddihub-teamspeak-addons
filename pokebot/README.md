# ZeddiHub Pokebot

TeamSpeak 3 plugin pro pokovací kampaně. Buildujeme **dvě varianty**:

- `zeddihub_pokebot_api25_win64.dll` — pro **TS3 3.5.6** a starší (max API 25)
- `zeddihub_pokebot_api26_win64.dll` — pro TS3 3.6.x a novější (API 26)
Pravým klikem na uživatele → vybereš preset nebo otevřeš Qt dialog
pro vlastní poke (text + interval + count + mode).

## Co umí

- **4 presety** v context menu:
  - 🇨🇿 *Wake-up CZ* — 20× burst různých českých "vstávej"
  - ⛔ *Halt!* — 10× v intervalu 3-6s, stop-fráze
  - 💢 *Symbol Storm* — dlouhý spam symbolů automaticky sekaný na 100-znakové chunky
  - 🤫 *Silent poke* — 8× tichý poke v intervalu 2-4.5s
- **Custom poke dialog** (Win32 native):
  - Mode: Burst (rychle za sebou) nebo Schedule (1× za interval)
  - Vlastní text (volitelně auto-chunked po 100 znacích)
  - Count: 1-500
  - Interval min/max ms (Schedule)
  - Burst delay ms
- **STOP** v context menu — okamžité přerušení běžící kampaně
- Bezpečnostní brzdy: max 500 poke/kampaň, min 50ms mezi poke

## Instalace

Viz [`BUILD.md`](BUILD.md). Stručně:

1. Naklonuj TS3 plugin SDK do `ts3sdk/`
2. Build (API 25 pro 3.5.6):
   `cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 -DPOKEBOT_API_VERSION=25 && cmake --build build_api25 --config Release`
3. Zkopíruj `build_api25\Release\zeddihub_pokebot_api25_win64.dll` do `%APPDATA%\TS3Client\plugins\`
4. Restart TS3 → Settings → Plugins → enable

> **Žádná Qt dependency** — Custom dialog je Win32 native, build potřebuje jen MSVC + Windows SDK (VS BuildTools 2022).

## Použití

1. Připoj se na TS server
2. **Right-click** na jakéhokoli uživatele
3. V kontextovém menu klikni na `ZH Pokebot: <preset>` nebo `Custom...`
4. Pro zastavení: pravým na uživatele → `ZH Pokebot: STOP`,
   nebo do chatu napiš `/zhpoke stop`

## Příkazy v chatu

```
/zhpoke status   # ukáže kolik poke odesláno z aktuální kampaně
/zhpoke stop     # zastaví běžící kampaň
```

## Struktura projektu

```
pokebot/
├── CMakeLists.txt
├── BUILD.md           ← detailní build návod
├── README.md          ← tento soubor
├── src/
│   ├── plugin.cpp        ← TS3 plugin entry, menu, callback dispatch
│   ├── poke_engine.h/cpp ← worker thread, scheduling, safety brakes
│   ├── presets.h         ← 4 preset PokeJob configs
│   └── custom_dialog.h/cpp ← Win32 dialog pro custom poke
└── ts3sdk/            ← TS3 plugin SDK headers (gitignored)
    └── PLACE_SDK_HERE.md
```

## Roadmap (možná)

- [ ] Per-user historie kampaní
- [ ] Settings dialog v TS3 plugin manageru (Qt)
- [ ] Per-preset editovatelné texty (uložené v JSON)
- [ ] Multi-target (poke skupiny uživatelů zároveň)
- [ ] Integrace s ZeddiHub admin telemetrii (volitelný opt-in)

## Etická poznámka

Pokebot je nástroj pro **vlastní servery** a **se souhlasem** uživatelů.
Nepoužívej proti lidem co si to nepřejí — to je důvod k banu na většině
serverů a typicky porušení TS3 ToS.

---

Verze: 1.0.0 · 2026-04-29 · ZeddiHub

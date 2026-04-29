# ZeddiHub TeamSpeak Addons

Kolekce 4 TeamSpeak 3 pluginů — práská, sleduje, hrá si s hlasem. Všechno
napsané v C++17 + Qt 5.15.2, postavené pro **API verze 23, 24, 25, 26**
(klient TS3 ~3.5.0 až 3.6.x+).

## Pluginy

| Plugin | Co dělá | Menu |
|---|---|---|
| **[Poke Bot](pokebot/)** | Pravým na uživatele → preset/custom poke kampaně. Burst i Schedule mode, MAX SPAM hard-cap 500, custom Qt dialog s TS3 dark theme. | Right-click client |
| **[Follow](Follow/)** | Pravým na uživatele → Start Following → automaticky tě přesune do jeho kanálu když ho změní. Auto-stop při no-perms. | Right-click client |
| **[MoveSpam](MoveSpam/)** | Pravým na uživatele → Basic (current↔default) nebo Custom dialog (zadat channel name/ID + interval slider). | Right-click client |
| **[Voice Changer](VoiceChanger/)** | Plugins top menu → Settings dialog. DSP efekty na mikrofon: Helium, Demon, Robot, Echo, Custom pitch slider. Phase-vocoder pitch shift. | Plugins top menu |

## Stažení

Jdi na **[Releases](../../releases)** a stáhni .zip podle své TS3 verze:

| TS3 client | Plugin API | Stáhni zip |
|---|---|---|
| 3.5.0 | 23 | `*_api23.zip` |
| 3.5.1 - 3.5.5 | 24 | `*_api24.zip` |
| **3.5.6** | 25 | `*_api25.zip` |
| 3.6.x+ | 26 | `*_api26.zip` |

Každý zip obsahuje DLL soubor pro daný plugin a danou API verzi. Rozbal a
zkopíruj DLL do:

```
%APPDATA%\TS3Client\plugins\
```

(typicky `C:\Users\<USER>\AppData\Roaming\TS3Client\plugins\`)

Pak v TS3: **Settings → Plugins → Reload All → zaškrtni Enabled**.

## Instalační poznámky

- Stahuj **jen jednu API verzi** každého pluginu — TS3 by jinak načetl
  duplicity a viděl bys menu items dvakrát.
- Pokud TS3 hlásí "API not compatible: X, current Y", máš špatnou variantu —
  stáhni `*_apiY.zip`.
- Voice Changer aplikuje efekty na **odchozí** mikrofon (slyší tě jiní
  s hlasem). Nehraje to lokálně tobě.

## Build ze zdrojáků

Viz per-plugin BUILD.md, nebo top-level `build_all.ps1` pro batch:

```powershell
# Setup (jednorázově)
git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone
Move-Item ts3sdk_clone/include ts3sdk/include
Remove-Item -Recurse -Force ts3sdk_clone

# Qt 5.15.2
py -m pip install aqtinstall
py -m aqt install-qt windows desktop 5.15.2 win64_msvc2019_64 -O C:\Qt --archives qtbase

# Build all
.\build_all.ps1
```

Po dokončení v `dist/<plugin>/api<NN>/` najdeš všechny DLL.

### Závislosti

| | |
|---|---|
| Visual Studio 2022 Build Tools | C++ Desktop workload + MSVC v143 + Windows SDK |
| CMake 3.16+ | Bundled ve VS BuildTools |
| Qt 5.15.2 MSVC 2019 64-bit | Pro UI dialogy (PokeBot, MoveSpam, VoiceChanger) |
| TS3 Plugin SDK | https://github.com/TeamSpeak-Systems/ts3client-pluginsdk |

## Struktura repa

```
zeddihub-teamspeak-addons/
├── README.md                    ← tento soubor
├── build_all.ps1                ← batch build script (16 DLLs)
├── ts3sdk/                      ← TS3 plugin SDK (gitignored, stáhneš sám)
├── pokebot/
│   ├── CLAUDE.md                ← per-plugin context paměť
│   ├── CMakeLists.txt
│   ├── README.md / BUILD.md
│   └── src/
├── Follow/
├── MoveSpam/
├── VoiceChanger/
│   └── vendor/smb_pitch_shift.h ← phase-vocoder pitch shifter (PD)
└── dist/                        ← gitignored, výstup build_all.ps1
    ├── pokebot/api23/*.dll
    ├── pokebot/api24/*.dll
    ├── ...
    └── VoiceChanger/api26/*.dll
```

## Etická poznámka

PokeBot, MoveSpam — jsou to **harassment tools**. Používej **jen na
serverech kde máš oprávnění** a **se souhlasem** uživatelů. Tyhle pluginy
ti udělají kick/ban na 99% veřejných serverů.

Follow + Voice Changer jsou OK i pro běžné použití (následování kamaráda,
fun voice changer). Pořád ale platí, že je to tvoje zodpovědnost.

## License

Source code: MIT. Vendored knihovny mají vlastní licence:
- `VoiceChanger/vendor/smb_pitch_shift.h` — public domain (Stephan M. Bernsee)

## Verze

- **v1.0.0** (2026-04-30) — Initial release: PokeBot, Follow, MoveSpam, Voice Changer.
  Multi-API build (23/24/25/26).

# ZeddiHub TeamSpeak Addons

Kolekce TeamSpeak 3 plug-inů od ZeddiHub. **Každý addon má svůj GitHub repo** s vlastním
README, CHANGELOG a releases. Tento repo je **index / collection / build infrastructure**.

[**ZeddiHub.eu**](https://zeddihub.eu) · [**ZeddiHub Tools**](https://zeddihub.eu/tools/) · [**zeddis.xyz**](https://zeddis.xyz) · © 2026 ZeddiHub.eu

---

## Plugins

| Plugin | Kategorie | Repo | Latest |
|---|---|---|---|
| **Poke Bot** | 😈 prank | [zeddihub-teamspeak-pokebot](https://github.com/ZeddiS/zeddihub-teamspeak-pokebot) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-pokebot?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-pokebot/releases/latest) |
| **Follow** | 🔧 utility | [zeddihub-teamspeak-follow](https://github.com/ZeddiS/zeddihub-teamspeak-follow) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-follow?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-follow/releases/latest) |
| **MoveSpam** | 😈 prank | [zeddihub-teamspeak-movespam](https://github.com/ZeddiS/zeddihub-teamspeak-movespam) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-movespam?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-movespam/releases/latest) |
| **Voice Changer** | 🎭 fun | [zeddihub-teamspeak-voicechanger](https://github.com/ZeddiS/zeddihub-teamspeak-voicechanger) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-voicechanger?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-voicechanger/releases/latest) |
| **AutoReconnect** | 🔧 utility | [zeddihub-teamspeak-autoreconnect](https://github.com/ZeddiS/zeddihub-teamspeak-autoreconnect) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-autoreconnect?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-autoreconnect/releases/latest) |
| **Greeting Bot** | 😈 prank | [zeddihub-teamspeak-greetingbot](https://github.com/ZeddiS/zeddihub-teamspeak-greetingbot) | [![](https://img.shields.io/github/v/release/ZeddiS/zeddihub-teamspeak-greetingbot?label=release)](https://github.com/ZeddiS/zeddihub-teamspeak-greetingbot/releases/latest) |

## Stažení & instalace

Otevři repo libovolného pluginu výše a v jeho **Releases**:

### A) Installer (.exe) — doporučené
Stáhni **`<plugin>-Setup-vX.Y.Z.exe`** a spusť.

Wizard:
1. Detekuje TS3 verzi a vybere správnou API DLL (23 / 24 / 25 / 26)
2. Detekuje běžící TS3, nabídne uzavření
3. Nainstaluje DLL do `%APPDATA%\TS3Client\plugins\` (per-user, bez admin)
4. Zaregistruje uninstaller v Add/Remove Programs

### B) Manuální (.zip)
Vyber zip podle své TS3 verze:
- `*-TS3-3.5.0-api23.zip` — TS3 3.5.0
- `*-TS3-3.5.1-3.5.5-api24.zip` — TS3 3.5.1 až 3.5.5
- **`*-TS3-3.5.6-api25.zip`** — TS3 3.5.6 ⭐ nejčastější
- `*-TS3-3.6+-api26.zip` — TS3 3.6.0+

Rozbal a zkopíruj DLL do `%APPDATA%\TS3Client\plugins\`. V TS3 → Settings → Plugins → Reload All → zaškrtni Enabled.

## Repo struktura

```
zeddihub-teamspeak-addons/   ← TENTO repo, build infrastructure + collection index
├── pokebot/                 ← lokální zdroj (per-plugin repos jsou srcd z toho)
├── Follow/
├── MoveSpam/
├── VoiceChanger/
├── AutoReconnect/
├── GreetingBot/
├── common/zh_brand.h        ← author/copyright sdílený header
├── installer/template.iss   ← Inno Setup šablona
├── build_all.ps1            ← build 24 DLLs (6 plugins × 4 APIs)
├── package_zips.ps1         ← package release zips
├── build_installers.ps1     ← Inno Setup -> .exe per plugin
├── refresh_per_plugin_repos.ps1  ← sync čistého obsahu do per-plugin repos
└── PLUGIN_IDEAS.md          ← backlog dalších pluginů
```

## Build infrastruktura

```powershell
# Setup (jednorázově)
py -m pip install aqtinstall
py -m aqt install-qt windows desktop 5.12.12 win64_msvc2017_64 -O C:\Qt --archives qtbase
git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone
Move-Item ts3sdk_clone/include ts3sdk/include
winget install JRSoftware.InnoSetup

# Plný release pipeline
.\build_all.ps1                  # 24 DLLs do dist/
.\package_zips.ps1               # 28 zipů do release/
.\build_installers.ps1           # 6 installerů .exe do installer/output/
.\refresh_per_plugin_repos.ps1   # push čistého obsahu do per-plugin repos
```

### Závislosti

| Tool | Verze | Účel |
|---|---|---|
| Visual Studio 2022 BuildTools | C++ Desktop workload | Compiler + Windows SDK |
| CMake | 3.16+ | Bundled ve VS BuildTools |
| **Qt 5.12.12 MSVC 2017 64-bit** | LTS | TS3 ABI compat (TS3 3.5.6 má Qt 5.12.3) |
| Inno Setup | 6.0+ | Build .exe installerů |
| TS3 Plugin SDK | 3.3.0+ | https://github.com/TeamSpeak-Systems/ts3client-pluginsdk |

> **Důležité:** Qt 5.12.12 (ne 5.15.x). Plugin built proti 5.15+ může selhat
> při loadu do TS3 3.5.6 kvůli chybějícím symbolům (např. `QPushButton::hitButton`
> přibyl až v Qt 5.13). 5.12 LTS je forward-compatible s novějšími TS3 verzemi.

## Plugin nápady (backlog)

Viz [PLUGIN_IDEAS.md](PLUGIN_IDEAS.md). Top picks pro v1.2.0:
- 🟢⭐ **Channel Notifier** — toast když přátelé připojí
- 🟡⭐ **Recording Indicator** — varování když někdo nahrává
- 🟢🔧 **Quick Channel Switcher** — hotkey jump
- 🟢😈 **Mirror Bot** — echo target's chat zpět jen jemu
- 🟡😈 **Random Mover Chaos** — MoveSpam s random destination

## Etická poznámka

PokeBot, MoveSpam, GreetingBot — **harassment tools**. Používej **jen kde
máš oprávnění** a **se souhlasem** uživatelů. Na 99% public TS3 serverů ti
za ně budou kickovat / banovat. Tvoje zodpovědnost.

Follow, AutoReconnect, Voice Changer — užitečné pro běžné použití.

## License

MIT pro vlastní kód. Vendored knihovny mají vlastní licence:
- `VoiceChanger/vendor/smb_pitch_shift.h` — public domain (Stephan M. Bernsee)

---

## Links

- 🏠 **Web**: [zeddihub.eu](https://zeddihub.eu)
- 🔧 **ZeddiHub Tools**: [zeddihub.eu/tools](https://zeddihub.eu/tools/)
- 👤 **Author**: [zeddis.xyz](https://zeddis.xyz)

© 2026 [ZeddiHub.eu](https://zeddihub.eu) · zeddis.xyz

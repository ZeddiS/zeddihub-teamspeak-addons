# ZeddiHub TeamSpeak Addons

Kolekce TeamSpeak 3 pluginů od ZeddiHub. Každý plugin má **vlastní GitHub
repo** s vlastními releases. Tento repo je **index / collection / build
infrastructure**.

🌐 [zeddihub.eu](https://zeddihub.eu) · 👤 [zeddis.xyz](https://zeddis.xyz) · © 2026 ZeddiHub.eu

## Pluginy

| Plugin | Kategorie | Repo | Co dělá |
|---|---|---|---|
| **Poke Bot** | 😈 prank | [ts3-pokebot](https://github.com/ZeddiS/ts3-pokebot) | Preset/custom poke kampaně, Burst+Schedule, Qt dark theme, MAX SPAM hard-cap |
| **Follow** | 🔧 utility | [ts3-follow](https://github.com/ZeddiS/ts3-follow) | Single-target auto-follow, auto-stop on errors |
| **MoveSpam** | 😈 prank | [ts3-movespam](https://github.com/ZeddiS/ts3-movespam) | Alternuje target mezi 2 channels — Basic + Custom Qt dialog |
| **Voice Changer** | 🎭 fun | [ts3-voicechanger](https://github.com/ZeddiS/ts3-voicechanger) | Real-time DSP: Helium / Demon / Robot / Echo / Custom pitch |
| **AutoReconnect** | 🔧 utility | [ts3-autoreconnect](https://github.com/ZeddiS/ts3-autoreconnect) | Auto-reconnect po disconnectu s exponential backoff |
| **Greeting Bot** | 😈 prank | [ts3-greetingbot](https://github.com/ZeddiS/ts3-greetingbot) | Auto-poke každého kdo vstoupí do tvého kanálu |

## Rychlá instalace

Každý plugin má **vlastní release** na svém repu. Zhruba:

1. Jdi na repo pluginu (viz tabulka)
2. **Releases** → najdi nejnovější verzi
3. Stáhni zip podle své TS3 verze:
   - `*-TS3-3.5.0-api23.zip` — TS3 3.5.0
   - `*-TS3-3.5.1-3.5.5-api24.zip` — TS3 3.5.1 až 3.5.5
   - `*-TS3-3.5.6-api25.zip` — TS3 3.5.6 ⭐ nejčastější
   - `*-TS3-3.6+-api26.zip` — TS3 3.6.0 a novější
4. Rozbal zip → zkopíruj DLL do `%APPDATA%\TS3Client\plugins\`
5. V TS3 → Settings → Plugins → Reload All → zaškrtni Enabled

## Build infrastructure

Tento repo obsahuje:

- `build_all.ps1` — buildí všech 6 pluginů × 4 API verze = **24 DLLs** najednou
- `package_zips.ps1` — balí `dist/*.dll` do `release/*.zip` s human-readable názvy
- `split_repos.ps1` — splitne každý plugin do vlastního GH repa s release
- `common/zh_brand.h` — sdílený author/copyright header
- `ts3sdk/` — TeamSpeak Plugin SDK (gitignored, stáhneš sám)
- `PLUGIN_IDEAS.md` — backlog dalších pluginů
- per-plugin folders pro local development

### Závislosti

| Tool | Verze | Účel |
|---|---|---|
| Visual Studio 2022 BuildTools | C++ Desktop workload | Compiler + Windows SDK |
| CMake | 3.16+ | Bundled ve VS BuildTools |
| **Qt 5.12.12 MSVC 2017 64-bit** | LTS | TS3 ABI compat (TS3 3.5.6 má Qt 5.12.3) |
| TS3 Plugin SDK | 3.3.0+ | https://github.com/TeamSpeak-Systems/ts3client-pluginsdk |

> **Důležité:** Qt 5.12.12 (ne 5.15.x). Plugin built proti 5.15+ může selhat
> při loadu do TS3 3.5.6 kvůli chybějícím symbolům (např. `QPushButton::hitButton`
> přibyl až v Qt 5.13). 5.12 LTS je forward-compatible s novějšími TS3 verzemi.

### Setup (jednorázově)

```powershell
# Qt 5.12.12 přes aqtinstall
py -m pip install aqtinstall
py -m aqt install-qt windows desktop 5.12.12 win64_msvc2017_64 -O C:\Qt --archives qtbase

# TS3 plugin SDK
git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone
Move-Item ts3sdk_clone/include ts3sdk/include
Remove-Item -Recurse -Force ts3sdk_clone

# Build vše
.\build_all.ps1
.\package_zips.ps1
```

## Architektura — jak to funguje

Každý plugin je samostatná Windows DLL:
- TS3 SDK exporty (`ts3plugin_*` C functions)
- Qt 5 dialogy pro UI (kde je potřeba)
- Worker threads pro background DSP/scheduling/poke
- PLUGIN_MENU_TYPE_CLIENT (right-click) i PLUGIN_MENU_TYPE_GLOBAL (top bar)

DLL se buildí v 4 variantách per plugin (API 23/24/25/26), takže každý
TS3 client od 3.5.0 po 3.6+ je pokryt.

Per-plugin `CLAUDE.md` (kontextová paměť pro Claude Code) má detaily
o každé architecture.

## Plugin ideas backlog

Viz [PLUGIN_IDEAS.md](PLUGIN_IDEAS.md) — náměty na další užitečné a prank
pluginy. PRs welcome!

## Verze

- **v1.1.0** (2026-04-30) — 6 pluginů, multi-API (23-26), Qt 5.12.12 ABI fix,
  per-plugin GH repos, author/copyright info, global menu items
- **v1.0.0** (2026-04-29) — Initial: 4 pluginů (PokeBot/Follow/MoveSpam/VoiceChanger)

## License

MIT pro vlastní kód. Vendored knihovny mají vlastní licence:
- `VoiceChanger/vendor/smb_pitch_shift.h` — public domain (Stephan M. Bernsee)

## Etická poznámka

PokeBot, MoveSpam, GreetingBot — **harassment tools**. Používej **jen kde
máš oprávnění** a **se souhlasem** uživatelů. Na 99% public TS3 serverů ti
za ně budou kickovat / banovat. Tvoje zodpovědnost.

Follow, AutoReconnect, Voice Changer — užitečné pro běžné použití.

---

zeddis.xyz · © 2026 ZeddiHub.eu · Built with Claude Code

# ZeddiHub TeamSpeak Addons

Curated TeamSpeak 3 plugin collection by ZeddiHub. **Each plugin lives in
its own GitHub repo** with releases, README, and CHANGELOG. This repo is
the **build infrastructure** and collection index.

[**ZeddiHub.eu**](https://zeddihub.eu) · [**ZeddiHub Tools**](https://zeddihub.eu/tools/) · [**zeddis.xyz**](https://zeddis.xyz) · © 2026 ZeddiHub.eu

---

## Plugins (4)

| Plugin | What it does | Repo |
|---|---|---|
| **Poke Bot** | Right-click client → preset (CZ, Symbol Storm, Silent, MAX) or custom poke campaign | [zeddihub-teamspeak-pokebot](https://github.com/ZeddiS/zeddihub-teamspeak-pokebot) |
| **Follow** | Auto-follow another client (polling, never auto-stops) | [zeddihub-teamspeak-follow](https://github.com/ZeddiS/zeddihub-teamspeak-follow) |
| **MoveSpam** | Repeatedly move target between two channels | [zeddihub-teamspeak-movespam](https://github.com/ZeddiS/zeddihub-teamspeak-movespam) |
| **Soundboard** | Play .wav files into your microphone with hotkeys | [zeddihub-teamspeak-soundboard](https://github.com/ZeddiS/zeddihub-teamspeak-soundboard) |

## Installation

Open the repo of any plugin above and grab the latest release.

### Recommended: TS3 native install (.ts3_plugin)
Download the file matching your TS3 version, double-click it, and TS3
opens its native install dialog. Click **Yes** to install.

| TS3 client | API | File |
|---|---|---|
| 3.5.0 | 23 | `*-TS3-3.5.0-api23.ts3_plugin` |
| 3.5.1 - 3.5.5 | 24 | `*-TS3-3.5.1-3.5.5-api24.ts3_plugin` |
| **3.5.6** ⭐ | **25** | `*-TS3-3.5.6-api25.ts3_plugin` |
| 3.6.x and newer | 26 | `*-TS3-3.6+-api26.ts3_plugin` |

### Manual: raw .dll
Download the matching `<plugin>_apiNN_win64.dll` and copy to
`%APPDATA%\TS3Client\plugins\`. Then in TS3 → Settings → Plugins → Reload All → tick Enabled.

## Theme

All plugin dialogs use **TS3 client's native theme**. Whatever skin you
have configured in TS3 (default, dark, custom) is what the plugin
windows will look like — no Discord-style overrides.

## Build infrastructure

```powershell
# Setup (one-time)
py -m pip install aqtinstall
py -m aqt install-qt windows desktop 5.12.12 win64_msvc2017_64 -O C:\Qt --archives qtbase
git clone https://github.com/TeamSpeak-Systems/ts3client-pluginsdk.git ts3sdk_clone
Move-Item ts3sdk_clone/include ts3sdk/include

# Full release pipeline
.\build_all.ps1                     # 16 DLLs (4 plugins x 4 APIs) -> dist/
.\package_ts3_plugins.ps1           # 16 .ts3_plugin packages -> ts3_plugins/
.\refresh_per_plugin_repos.ps1      # push minimal trees to per-plugin repos
.\reorganize_releases.ps1           # create per-plugin GH releases
```

### Tooling

| Tool | Version | Why |
|---|---|---|
| Visual Studio 2022 BuildTools | C++ Desktop workload | MSVC + Windows SDK |
| CMake | 3.16+ | Bundled in VS BuildTools |
| Qt 5.12.12 MSVC 2017 64-bit | LTS | TS3 3.5.6 ships Qt 5.12.3 -- ABI compat |
| TS3 Plugin SDK | 3.3.0+ | https://github.com/TeamSpeak-Systems/ts3client-pluginsdk |

> **Important:** Qt 5.12 (not 5.15+). Plugin built against 5.13+ may fail
> to load on TS3 3.5.6 due to missing symbols (e.g. `QPushButton::hitButton`
> added in Qt 5.13). 5.12 LTS is forward-compatible with newer TS3 versions.

## Repo structure

```
zeddihub-teamspeak-addons/
├── pokebot/        ← Poke Bot source (builds zeddihub_pokebot_*.dll)
├── Follow/         ← Follow source
├── MoveSpam/       ← MoveSpam source
├── Soundboard/     ← Soundboard source (with vendored audio decoder)
├── common/zh_brand.h        ← Shared author/copyright header
├── ts3sdk/                  ← TS3 plugin SDK (gitignored)
├── build_all.ps1            ← Build 16 DLLs
├── package_ts3_plugins.ps1  ← Wrap DLLs into .ts3_plugin packages
├── refresh_per_plugin_repos.ps1  ← Sync README/CHANGELOG to per-plugin repos
├── reorganize_releases.ps1  ← Recreate releases with .ts3_plugin + .dll assets
└── PLUGIN_IDEAS.md          ← Backlog of future plugin ideas
```

## Deprecated repos (archived)

The following plugins were experiments that didn't make the final cut.
Repos are archived (read-only) but still accessible:

- ~~zeddihub-teamspeak-voicechanger~~ (real-time DSP voice effects)
- ~~zeddihub-teamspeak-autoreconnect~~ (auto-reconnect on disconnect)
- ~~zeddihub-teamspeak-greetingbot~~ (auto-poke on channel join)

## Plugin ideas backlog

See [PLUGIN_IDEAS.md](PLUGIN_IDEAS.md) for proposed future plugins.

## Ethical note

Poke Bot, MoveSpam — **harassment tools**. Use only on servers where you
have permission and target consents. Most public servers will kick/ban
you for using them.

Follow, Soundboard — useful for everyday TS3 use. Soundboard especially
is a fun social tool when used responsibly.

## License

MIT for own code. Vendored libraries have their own licenses.

---

## Links

- 🏠 **Web**: [zeddihub.eu](https://zeddihub.eu)
- 🔧 **ZeddiHub Tools**: [zeddihub.eu/tools](https://zeddihub.eu/tools/)
- 👤 **Author**: [zeddis.xyz](https://zeddis.xyz)

© 2026 [ZeddiHub.eu](https://zeddihub.eu) · zeddis.xyz

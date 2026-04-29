# Follow — Plugin notes for Claude

> Kontextová paměť pro Claude napříč konverzacemi ohledně **Follow**
> pluginu. Před editací čti vždy **TENTO soubor**.

## Co Follow dělá

TS3 plugin. Pravým klikem na uživatele → **Start Following**. Když ten
uživatel přepne kanál, plugin automaticky přesune i tebe do nového kanálu.
**Stop Following** kampaň zastaví.

**Single target mode + auto-stop on error**:
- Sleduji jen jednoho člověka. Start na druhém = první se zastaví.
- Pokud server odmítne move (no rights / channel password / kicked) →
  follow se sám zastaví + chat notifikace.
- Disconnect / timeout sledovaného → auto-stop.

**Menu items** (PLUGIN_MENU_TYPE_CLIENT):
1. `Follow Bot: Start Following` — začne sledovat, hned přesune do jeho aktuálního kanálu
2. `Follow Bot: Stop Following` — zastaví sledování

**Chat command**: `/follow status`, `/follow stop`

## Architektura

```
Follow/
├── CMakeLists.txt              ← build s ${FOLLOW_API_VERSION}, žádný Qt (jen menu)
├── CLAUDE.md                   ← tento soubor
└── src/
    └── plugin.cpp              ← VŠE: TS3 SDK exporty, menu, atomic state, callbacky
```

**Žádný Qt** — Follow nemá dialogy, jen menu items + onClientMoveEvent callback.

### Klíčové globální state

```cpp
std::atomic<bool>     g_active;        // currently following?
std::atomic<uint64>   g_followSchid;   // server connection ID
std::atomic<anyID>    g_followClientID;// target's client ID
```

Atomic protože callbacky (`onClientMoveEvent`) jdou z TS3 audio/network threadu, ne main.

### Klíčové funkce

| Funkce | Účel |
|---|---|
| `moveSelfTo(schid, channelID)` | Získá vlastní clientID přes `getClientID()`, volá `requestClientMove()` |
| `getClientName(schid, clientID, out&)` | Helper přes `getClientVariableAsString(CLIENT_NICKNAME)` |
| `stopFollowing(schid, reason)` | Atomicky vypne tracking + chat notify s důvodem |
| `ts3plugin_onClientMoveEvent` | TS3 callback pro každý move libovolného klienta — filtrujeme target |
| `ts3plugin_onClientMoveTimeoutEvent` | Target timeoutoval → auto-stop |
| `ts3plugin_onServerErrorEvent` | Move odmítnut serverem → auto-stop pokud je to permission/channel error |

### Move detection logic

```
onClientMoveEvent(schid, clientID, oldCh, newCh, vis, msg):
    if not g_active: return
    if schid != g_followSchid: return
    if clientID != g_followClientID: return
    if newCh == 0:
        stopFollowing("user disconnected")
    else if not moveSelfTo(schid, newCh):
        stopFollowing("no permission to target channel")
```

## Build

API přes `-DFOLLOW_API_VERSION=23|24|25|26` (default 26).

```powershell
& $cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 -DFOLLOW_API_VERSION=25
& $cmake --build build_api25 --config Release
```

Output: `build_api25/Release/follow_api25_win64.dll` (~30-50 KB).

## Pitfalls / gotchas

- **TS3 callback thread** — `onClientMoveEvent` se NEvolá z main UI thread. Nepoužívat Qt UI z těchto callbacků! Atomic state stačí pro jednoduchost.
- **schid filter** — pokud jsi připojený na 2+ servery zároveň, sledování platí jen na ten server kde jsi spustil. Pokud target není na stejném schid, ignoruj.
- **Vlastní move** — `requestClientMove(schid, ownID, ...)` triggers ANOTHER `onClientMoveEvent` pro nás. Nezpůsobuje smyčku, protože filter `clientID != target` to odfiltruje.
- **Disconnect detection** — TS3 posílá `onClientMoveEvent` s `newCh == 0` při disconnectu. Auto-stop logika tohle řeší.
- **`ts3plugin_freeMemory`** — required, jinak menu nezobrazí.
- **Ban/kick z target's channel** — pokud server kickne za try-to-enter, dostaneme `onServerErrorEvent` s `ERROR_permissions` → auto-stop.

## TS3 SDK funkce, které používáme

```c
ts3Functions.getClientID(schid, &myID)
ts3Functions.getChannelOfClient(schid, clientID, &channelID)
ts3Functions.getClientVariableAsString(schid, clientID, CLIENT_NICKNAME, &name)
ts3Functions.requestClientMove(schid, clientID, channelID, password, returnCode)
ts3Functions.printMessage(schid, text, target)
ts3Functions.freeMemory(ptr)
```

## Verze

- 1.0.0 (2026-04-30) — Initial release, single target + auto-stop

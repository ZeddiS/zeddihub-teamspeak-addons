# MoveSpam — Plugin notes for Claude

> Kontextová paměť pro Claude napříč konverzacemi ohledně **MoveSpam**
> pluginu. Před editací čti vždy **TENTO soubor**.

## Co MoveSpam dělá

TS3 plugin. Pravým klikem na uživatele → opakovaně přesouvá toho
uživatele tam-zpět mezi dvěma kanály.

**Menu items** (PLUGIN_MENU_TYPE_CLIENT):
1. `MoveSpam: Basic (current ↔ default)` — alternuje target mezi jeho aktuálním kanálem a server-default channelem
2. `MoveSpam: Custom...` — Qt dialog: zadáš channel name nebo ID + interval slider
3. `MoveSpam: STOP` — zastaví běžící kampaň

**Chat command**: `/movespam status`, `/movespam stop`

## Architektura

```
MoveSpam/
├── CMakeLists.txt              ← Qt 5.15 + ${MOVESPAM_API_VERSION}
├── CLAUDE.md                   ← tento soubor
└── src/
    ├── plugin.cpp              ← TS3 exporty + menu + channel resolution helpers
    ├── move_engine.h/.cpp      ← worker thread, alternuje moves, 3-error backoff
    ├── custom_dialog.h         ← Result struct pro dialog
    └── custom_dialog.cpp       ← Qt dialog s TS3 dark theme + 2 paired slidery
```

### Klíčové třídy

| Symbol | Soubor | Účel |
|---|---|---|
| `MoveJob` | move_engine.h | POD: schid, clientID, channelA, channelB, intervalMs, maxMoves |
| `MoveEngine` | move_engine.cpp | Worker thread, alternuje A↔B, **3 consecutive errors → stop** |
| `getDefaultChannel(schid)` | plugin.cpp | Iteruje channelList, hledá `CHANNEL_FLAG_DEFAULT`. Fallback = lowest ID |
| `findChannelByName(schid, name)` | plugin.cpp | Case-insensitive substring match přes `CHANNEL_NAME` |
| `custom_dialog::runMoveSpamDialog()` | custom_dialog.cpp | Qt dialog → Result {destChannelID, destName, intervalMs, maxMoves} |

### Channel resolution flow (Custom mode)

```
1. User zadá text v dialogu
2. Try parse jako uint64 → destChannelID
3. Pokud parsování selže → uložit jako destName
4. plugin.cpp: pokud destChannelID == 0, volá findChannelByName()
5. Ne-found → notify chat + abort
```

### MoveEngine worker logic

```cpp
for i = 0; i < maxMoves || maxMoves == 0; ++i:
    if stopRequested: break
    dest = toggle ? channelB : channelA
    err = requestClientMove(schid, clientID, dest, "", null)
    if err == ERROR_ok:
        sent++
        consecutiveErrors = 0
    else:
        if ++consecutiveErrors >= 3: break   # back off
    sleep(intervalMs)  # condition_variable.wait_for, interruptible
```

## Build

API přes `-DMOVESPAM_API_VERSION=23|24|25|26` (default 26).

```powershell
& $cmake -S . -B build_api25 -G "Visual Studio 17 2022" -A x64 `
         -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" -DMOVESPAM_API_VERSION=25
& $cmake --build build_api25 --config Release
```

Output: `build_api25/Release/movespam_api25_win64.dll` (~80-150 KB).

## Pitfalls / gotchas

- **Min interval 200 ms** v `kMinIntervalMs` — TS3 server flood-kicks pod tím.
- **Default channel detection** — některé servery nemají žádný kanál s `CHANNEL_FLAG_DEFAULT` set. Fallback je lowest channel ID. Může to být něco jiného než user očekává.
- **`maxMoves == 0`** v dialogu = nekonečno. Custom dialog ukazuje "∞" symbol pro hodnotu 0 (`QSpinBox::specialValueText`).
- **Move selhání** — pokud target nemá oprávnění do destination kanálu (např. password protected, locked), volání selže. Engine počítá 3 consecutive errors a pak zastaví automaticky.
- **CHANNEL_NAME memory** — `getChannelVariableAsString` alokuje string, **vždy `freeMemory(ptr)`** po použití. Bez toho memory leak.
- **getChannelList free** — pole `uint64*` ukončené nulou. **Vždy `freeMemory(channelList)`**.
- **Substring match** — `findChannelByName` dělá case-insensitive substring. Více kanálů se shodným názvem? Vrátí PRVNÍ.
- **Kanál target vs current** — `MENU_ID_BASIC` zjišťuje aktuální kanál target přes `getChannelOfClient`. Pokud je 0 (chyba), abort.

## TS3 SDK funkce, které používáme

```c
ts3Functions.getChannelOfClient(schid, clientID, &channelID)
ts3Functions.getChannelList(schid, &channelList)
ts3Functions.getChannelVariableAsInt(schid, channelID, CHANNEL_FLAG_DEFAULT, &val)
ts3Functions.getChannelVariableAsString(schid, channelID, CHANNEL_NAME, &str)
ts3Functions.requestClientMove(schid, clientID, channelID, password, returnCode)
ts3Functions.printMessage(schid, text, target)
ts3Functions.freeMemory(ptr)
```

## Verze

- 1.0.0 (2026-04-30) — Initial release, Basic + Custom mode

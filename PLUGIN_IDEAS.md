# Plugin Ideas — backlog pro budoucí rozvoj

Návrhy dalších TS3 pluginů k ZeddiHub kolekci. Označeno podle náročnosti
implementace (komplexita) a hodnoty (užitek pro běžné/pokročilé uživatele).

Legend: 🟢 simple · 🟡 medium · 🔴 complex · ⭐ high value · 🔧 utility · 😈 prank

---

## Užitečné

### 🟢 ⭐ Channel Notifier
Desktop toast (Windows 10/11) když přátelé z whitelistu přijdou na server.
Subscribe to `onClientMoveEvent`, hooknout Win32 ToastNotification API.
Konfigurace: list nicknamů + per-user toggle.

### 🟡 ⭐ Recording Indicator
Podle `CLIENT_IS_RECORDING` flagu zobrazit blikající ikonu / poke-warn
když někdo v tvém kanálu nahrává. Bezpečnost > "kdo to nahrává".

### 🟢 🔧 Quick Channel Switcher
Definuj 5-10 oblíbených kanálů, hotkey → instant move. TS3 hotkey API
(`onHotkeyEvent`) + Qt config dialog. Ušetří klikání.

### 🟡 🔧 Sticky Status
Nastav si trvalé "I'm AFK / Working" status který se obnoví po každém
re-connectu. TS3 to ne-pamatuje — náš plugin ano.

### 🟡 ⭐ Server Time Logger
Zaloguj kolik času jsi strávil v každém kanálu na každém serveru. CSV
export, grafy. Pro hardcore TS3 uživatele.

### 🔴 🔧 Voice Activation Logger
Detekuj a loguj kdo mluví v aktuálním kanálu (timestamp + duration). Skvělé
pro "kdo to řekl" debaty po několika měsících.

### 🟡 🔧 Auto-AFK Mover
Po X minutách bez mluvení → move sebe do AFK channel (definovatelný).
Po promluvení zpět → move zpět. Detekce via `onTalkStatusChange`.

### 🟡 ⭐ Bookmark Sync
Sync TS3 bookmarks přes JSON soubor v cloudu (Dropbox/OneDrive). Mezi PC.

### 🟢 🔧 Channel Description Toaster
Hover nad kanálem → zobrazí description v TS3 client tooltipu. Vyžaduje
custom Qt widget overlay. Streamlined navigace.

### 🔴 🔧 Unread Indicator
Per-channel badge zobrazující nepřečtené zprávy. TS3 nemá nativně —
uchováváme last-read timestamp lokálně.

---

## Pranky

### 🟢 😈 Mirror Bot
Když target user napíše do chatu, plugin pošle stejnou zprávu zpátky jen
jemu (private message). Začne pochybovat o realitě. ~~Zlé~~ vtipné.

### 🟡 😈 Random Mover (chaos mode)
Like MoveSpam ale destination = náhodný channel ze server tree. Cesta
nemá ani logiku ani pattern. Nesnesitelné UX pro target.

### 🟢 😈 Subtitle Spam
Tvoje vlastní chat zprávy se posílají word-by-word s 500ms intervalem.
Vypadá to jak streaming subtitle. Pro tebe samotného.

### 🟡 😈 Status Cycler
Tvůj status (online/away/dnd) cykluje každých X sekund. Ostatní v
kanálu vidí pulsující notifikace. Annoying bez actual harm.

### 🟡 😈 ChatBomb
Když target napíše JAKOUKOLI chat zprávu, plugin pošle 5-10 random response
zpráv z poolu (configurable). "Imagine being you", "🤡", "this is fine 🔥".

### 🔴 😈 Soundboard Bot
Predefinovaný pool zvuků (mp3/wav). Hotkey → play do mikrofonu (mix
s capture audio). Vyžaduje audio mixing přes `onEditCapturedVoiceData`.

### 🟢 😈 Welcome Spam
GreetingBot rozšíření: každý uživatel co vstoupí do tvého kanálu dostane
**tři** poke různými hláškami v 5s rozestupech.

### 🟡 😈 Random Talk Power Toggle
Pokud máš grant move power, target user dostane talk power na 1s, pak
revoke, pak grant... Konstantní notifikace zvuku.

### 🔴 😈 Voice Distortion Per-User
VoiceChanger varianta — aplikuje DSP efekty JEN na hlas konkrétního
uživatele (slyšíš ho jako démon, ostatní ho slyší normálně). Hooknout
`onEditPlaybackVoiceDataEvent` per-clientID.

---

## Pokročilé / experimentální

### 🔴 ⭐ Server Stats Dashboard
Plugin window: real-time bandwidth, latency, packet loss, ping graph
za posledních 60 minut. Charts (Qt QChartView).

### 🔴 ⭐ AI Voice Assistant
Hotkey → record 5s → speech-to-text → forward to GPT API → TTS odpoví.
"Hey TS3, who's in channel General?". Internet/API dependency.

### 🔴 🔧 Multi-Server Bridge
Forwarduj chat zprávy mezi 2 TS3 servery (přes 2 schid). "Bridge" pro
komunity rozdělené na více serverů.

### 🔴 ⭐ Voice Snippets Recorder
Tlačítko → uloží posledních 30 sekund audio z kanálu jako .wav. Pro
zachycení vtipů / důkazu / archivace. Circular buffer audio capture.

### 🟡 ⭐ Channel Auto-Joiner
"Pokud lobby > 5 lidí, automaticky tě tam přesune". Configurable rules.

### 🔴 🔧 Smart Mute Manager
Auto-mute / unmute lidí podle pravidel (např. "mute všechny kdo nemají
voice avatar nastavený"). Pro velké servery.

### 🟡 ⭐ Friend Map
Mapa kdo s kým je v jakém kanálu, s historií. "Jak dlouho jsou A a B v
channel General?". Konfigurovatelné friends.

---

## Recommended priority pro v1.2.0

1. **Channel Notifier** (🟢⭐) — high value, low effort
2. **Recording Indicator** (🟡⭐) — privacy/security
3. **Quick Channel Switcher** (🟢🔧) — daily QoL
4. **Mirror Bot** (🟢😈) — fun prank
5. **Random Mover chaos** (🟡😈) — variant of MoveSpam

---

ZeddiHub TeamSpeak Addons | zeddis.xyz | © 2026 ZeddiHub.eu

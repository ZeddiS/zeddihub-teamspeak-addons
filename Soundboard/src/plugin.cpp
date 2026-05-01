// Soundboard - TeamSpeak 3 plugin
//
// Plugins menu (top bar) -> "Open Soundboard" opens a Qt grid where user
// adds sound slots, browses for .wav files and assigns hotkeys. When a
// hotkey fires (or user clicks Play in dialog), the sound is mixed into
// the outgoing microphone stream so other clients hear it.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "plugin_definitions.h"
#include "ts3_functions.h"

#include "../../common/zh_brand.h"
#include "audio_converter.h"
#include "audio_engine.h"
#include "sound_slot.h"
#include "soundboard_dialog.h"

#ifndef PLUGIN_API_VERSION
#define PLUGIN_API_VERSION 26
#endif

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s((dest), (destSize), (src))
#else
#define _strcpy(dest, destSize, src)                  \
    do {                                               \
        std::strncpy((dest), (src), (destSize) - 1);   \
        (dest)[(destSize) - 1] = '\0';                 \
    } while (0)
#endif

namespace {

struct TS3Functions ts3Functions;
char* g_pluginID = nullptr;
std::unique_ptr<AudioEngine> g_engine;
std::vector<SoundSlot> g_slots;

// VAD bypass state — when sounds are queued we disable TS3's voice
// activation so the mix is unconditionally transmitted. Saved value is
// restored once playback finishes (or plugin unloads).
std::string g_savedVad;
bool g_vadOverridden = false;
uint64 g_vadSchid = 0;

constexpr int kMaxHotkeySlots = 32;

enum MenuID : int {
    MENU_ID_OPEN = 1,
    MENU_ID_STOP_ALL,
    MENU_ID_ABOUT,
};

void notifyTab(uint64 schid, const char* text) {
    if (ts3Functions.printMessage && schid != 0) {
        ts3Functions.printMessage(schid, text, PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "Soundboard", 0);
    }
}

PluginMenuItem* makeMenuItem(PluginMenuType type, int id, const char* text) {
    auto* m = static_cast<PluginMenuItem*>(std::malloc(sizeof(PluginMenuItem)));
    m->type = type;
    m->id = id;
    _strcpy(m->text, PLUGIN_MENU_BUFSZ, text);
    _strcpy(m->icon, PLUGIN_MENU_BUFSZ, "");
    return m;
}

uint64 currentSchid() {
    if (!ts3Functions.getCurrentServerConnectionHandlerID) return 0;
    return ts3Functions.getCurrentServerConnectionHandlerID();
}

// Disable TS3 voice activation detection so the mic mix is transmitted
// unconditionally. Saves current VAD state to restore later.
void overrideVadOff(uint64 schid) {
    if (g_vadOverridden) return;
    if (schid == 0) return;
    if (!ts3Functions.getPreProcessorConfigValue || !ts3Functions.setPreProcessorConfigValue) return;
    char* cur = nullptr;
    if (ts3Functions.getPreProcessorConfigValue(schid, "vad", &cur) == ERROR_ok && cur) {
        g_savedVad = cur;
        ts3Functions.freeMemory(cur);
    } else {
        g_savedVad = "true";  // sane default to restore later
    }
    ts3Functions.setPreProcessorConfigValue(schid, "vad", "false");
    g_vadOverridden = true;
    g_vadSchid = schid;
    char buf[160];
    std::snprintf(buf, sizeof(buf),
        "[SoundBoard] VAD temporarily disabled (was '%s') to ensure transmission of mixed audio.",
        g_savedVad.c_str());
    logMsg(buf);
}

void restoreVad() {
    if (!g_vadOverridden) return;
    if (g_vadSchid != 0 && ts3Functions.setPreProcessorConfigValue) {
        ts3Functions.setPreProcessorConfigValue(g_vadSchid, "vad", g_savedVad.c_str());
        char buf[120];
        std::snprintf(buf, sizeof(buf),
            "[SoundBoard] VAD restored to '%s'.", g_savedVad.c_str());
        logMsg(buf);
    }
    g_vadOverridden = false;
    g_vadSchid = 0;
    g_savedVad.clear();
}

void playSlot(int id) {
    if (!g_engine) return;
    for (auto& s : g_slots) {
        if (s.id != id) continue;
        char buf[256];
        if (s.filePath.empty()) {
            std::snprintf(buf, sizeof(buf), "[SoundBoard] Slot %d has no file.", id);
            logMsg(buf, LogLevel_WARNING);
            return;
        }
        if (!s.decoded) {
            s.decoded = g_engine->loadFile(s.filePath);
        }
        if (!s.decoded || !s.decoded->ok) {
            std::snprintf(buf, sizeof(buf),
                "[SoundBoard] Failed to decode slot %d (file: %s). "
                "Use 16/24/32-bit PCM or 32-bit float WAV.",
                id, s.filePath.c_str());
            logMsg(buf, LogLevel_WARNING);
            return;
        }

        // 1. Queue for mic-stream mix (others in channel hear it)
        g_engine->play(s.decoded, s.volume);

        // 1b. Disable VAD so the mix is transmitted no matter how quiet the
        //     user's actual mic is. Restored when active queue empties.
        overrideVadOff(currentSchid());

        // 2. Local monitor playback. Try TS3 first (mixes with TS3 audio
        //    output device), fall back to Win32 PlaySound (default Windows
        //    playback device). Win32 fallback works even when offline.
        bool localOk = false;
#ifdef _WIN32
        unsigned int ts3err = ERROR_ok;
        if (ts3Functions.playWaveFile && currentSchid() != 0) {
            ts3err = ts3Functions.playWaveFile(currentSchid(), s.filePath.c_str());
            if (ts3err == ERROR_ok) localOk = true;
        }
        if (!localOk) {
            // Fallback: Win32 PlaySound (async, default playback device).
            // Each call replaces previous SoundBoard playback (single-stream).
            BOOL sndOk = PlaySoundA(s.filePath.c_str(), NULL,
                                    SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
            if (sndOk) localOk = true;
        }
#endif

        std::snprintf(buf, sizeof(buf),
            "[SoundBoard] Playing slot %d (%zu samples, ~%.1fs). "
            "Local monitor: %s.",
            id, s.decoded->samples.size(),
            (double)s.decoded->samples.size() / 48000.0,
            localOk ? "OK" : "FAILED (file path issue?)");
        logMsg(buf);
        return;
    }
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "SoundBoard"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() { return "1.2.2"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_apiVersion() { return PLUGIN_API_VERSION; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_author() { return ZH_AUTHOR; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_description() {
    return ZH_DESC(
        "Soundboard - mix .wav sounds into your microphone stream. "
        "Plugins menu -> Open Soundboard. Bind hotkeys in TS3 Settings.");
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_init() {
    g_engine = std::make_unique<AudioEngine>();
    if (!audio_converter::init()) {
        logMsg("Audio converter init failed - import will only accept WAV", LogLevel_WARNING);
    }
    soundboard_dialog::loadSlots(g_slots);
    logMsg("SoundBoard plugin loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    restoreVad();
    if (g_engine) {
        g_engine->stopAll();
        g_engine.reset();
    }
    g_slots.clear();
    audio_converter::shutdown();
    if (g_pluginID) {
        std::free(g_pluginID);
        g_pluginID = nullptr;
    }
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_registerPluginID(const char* id) {
    const std::size_t sz = std::strlen(id) + 1;
    g_pluginID = static_cast<char*>(std::malloc(sz));
    _strcpy(g_pluginID, sz, id);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_freeMemory(void* data) { std::free(data); }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_requestAutoload() { return 0; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_commandKeyword() { return "soundboard"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command || !g_engine) return 1;
    if (std::strcmp(command, "open") == 0) {
        soundboard_dialog::open(g_slots, *g_engine, &playSlot);
        return 0;
    }
    if (std::strcmp(command, "stop") == 0) {
        g_engine->stopAll();
        notifyTab(schid, "[Soundboard] All sounds stopped.");
        return 0;
    }
    if (std::strncmp(command, "play ", 5) == 0) {
        int id = std::atoi(command + 5);
        if (id > 0) playSlot(id);
        return 0;
    }
    notifyTab(schid,
        "[Soundboard] Commands: /soundboard open | stop | play <slotId>");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * 4));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_OPEN,
                                   "Soundboard: Open Soundboard");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_STOP_ALL,
                                   "Soundboard: Stop All Sounds");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_ABOUT,
                                   "Soundboard: About...");
    (*menuItems)[3] = nullptr;

    *menuIcon = static_cast<char*>(std::malloc(PLUGIN_MENU_BUFSZ));
    _strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "icon.png");
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onMenuItemEvent(uint64 schid,
                               enum PluginMenuType type,
                               int menuItemID,
                               uint64 selectedItemID) {
    (void)selectedItemID;
    if (type != PLUGIN_MENU_TYPE_GLOBAL) return;
    if (!g_engine) return;
    if (schid == 0) schid = currentSchid();

    switch (menuItemID) {
        case MENU_ID_OPEN:
            soundboard_dialog::open(g_slots, *g_engine, &playSlot);
            break;
        case MENU_ID_STOP_ALL:
            g_engine->stopAll();
            notifyTab(schid, "[Soundboard] All sounds stopped.");
            break;
        case MENU_ID_ABOUT:
            notifyTab(schid,
                "[Soundboard] " ZH_AUTHOR " | " ZH_COPYRIGHT
                " | https://github.com/ZeddiS/zeddihub-teamspeak-soundboard");
            break;
    }
}

// Pre-register hotkeys for 32 slot triggers + a "Stop All" + "Open" hotkey.
// User binds them in TS3 Settings -> Hotkeys.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
    constexpr int kCount = kMaxHotkeySlots + 2;  // 32 slots + Stop All + Open
    *hotkeys = static_cast<PluginHotkey**>(
        std::malloc(sizeof(PluginHotkey*) * (kCount + 1)));
    int idx = 0;

    auto mk = [](const char* keyword, const char* desc) {
        auto* h = static_cast<PluginHotkey*>(std::malloc(sizeof(PluginHotkey)));
        _strcpy(h->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
        _strcpy(h->description, PLUGIN_HOTKEY_BUFSZ, desc);
        return h;
    };

    for (int i = 1; i <= kMaxHotkeySlots; ++i) {
        char kw[64], desc[96];
        std::snprintf(kw, sizeof(kw), "soundboard_play_%d", i);
        std::snprintf(desc, sizeof(desc), "Play soundboard slot %d", i);
        (*hotkeys)[idx++] = mk(kw, desc);
    }
    (*hotkeys)[idx++] = mk("soundboard_stop_all", "Stop all soundboard sounds");
    (*hotkeys)[idx++] = mk("soundboard_open", "Open soundboard window");
    (*hotkeys)[idx] = nullptr;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onHotkeyEvent(const char* keyword) {
    if (!keyword || !g_engine) return;
    if (std::strncmp(keyword, "soundboard_play_", 16) == 0) {
        int id = std::atoi(keyword + 16);
        if (id > 0) playSlot(id);
        return;
    }
    if (std::strcmp(keyword, "soundboard_stop_all") == 0) {
        g_engine->stopAll();
        return;
    }
    if (std::strcmp(keyword, "soundboard_open") == 0) {
        soundboard_dialog::open(g_slots, *g_engine, &playSlot);
        return;
    }
}

// THE audio callback - mix soundboard audio into outgoing mic stream.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onEditCapturedVoiceDataEvent(uint64 schid,
                                            short* samples,
                                            int sampleCount,
                                            int channels,
                                            int* edited) {
    (void)schid;
    static int s_callbackCount = 0;
    static bool s_loggedFirst = false;
    if (!s_loggedFirst) {
        s_loggedFirst = true;
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "[SoundBoard] Audio callback active: sampleCount=%d, channels=%d. "
            "Plugin is wired correctly.", sampleCount, channels);
        logMsg(buf);
    }
    if (!g_engine || !samples || sampleCount <= 0) return;
    if (!g_engine->hasActive()) {
        // No more sounds queued: restore VAD if we previously bypassed it
        if (g_vadOverridden) restoreVad();
        return;
    }

    bool modified = g_engine->mixIntoCapture(samples, sampleCount, channels);
    if (modified && edited) {
        *edited = 1;
        if (++s_callbackCount % 100 == 0) {
            char buf[80];
            std::snprintf(buf, sizeof(buf),
                "[SoundBoard] mixed %d frames so far.", s_callbackCount);
            logMsg(buf);
        }
    }
}

}  // extern "C"

// Voice Changer — TeamSpeak 3 plugin
//
// Plugins menu (top bar) → Voice Changer:
//   - Settings...
//   - Enable Voice Changer
//   - Disable Voice Changer
//
// Hooks ts3plugin_onEditCapturedVoiceDataEvent and applies DSP to the
// outgoing microphone stream before TS3 sends it to the server.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "plugin_definitions.h"
#include "ts3_functions.h"

#include "../../common/zh_brand.h"
#include "settings_dialog.h"
#include "voice_engine.h"

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
std::unique_ptr<VoiceEngine> g_engine;

enum MenuID : int {
    // Per-preset shortcuts (one click = pick & enable)
    MENU_ID_PRESET_OFF        = 1,
    MENU_ID_PRESET_HELIUM,
    MENU_ID_PRESET_CHIPMUNK,
    MENU_ID_PRESET_DEEP,
    MENU_ID_PRESET_DEMON,
    MENU_ID_PRESET_ROBOT,
    MENU_ID_PRESET_ECHO,
    MENU_ID_PRESET_DISTORTION,
    MENU_ID_PRESET_WHISPER,
    MENU_ID_PRESET_TELEPHONE,
    MENU_ID_PRESET_UNDERWATER,
    MENU_ID_PRESET_MEGAPHONE,
    MENU_ID_PRESET_VOLBOOST,
    // Generic actions
    MENU_ID_SETTINGS = 100,
    MENU_ID_ENABLE,
    MENU_ID_DISABLE,
    MENU_ID_TOGGLE_COMPAT,
    MENU_ID_ABOUT,
};

VoicePreset menuToPreset(int menuItemID) {
    switch (menuItemID) {
        case MENU_ID_PRESET_OFF:        return VoicePreset::Off;
        case MENU_ID_PRESET_HELIUM:     return VoicePreset::Helium;
        case MENU_ID_PRESET_CHIPMUNK:   return VoicePreset::Chipmunk;
        case MENU_ID_PRESET_DEEP:       return VoicePreset::Deep;
        case MENU_ID_PRESET_DEMON:      return VoicePreset::Demon;
        case MENU_ID_PRESET_ROBOT:      return VoicePreset::Robot;
        case MENU_ID_PRESET_ECHO:       return VoicePreset::Echo;
        case MENU_ID_PRESET_DISTORTION: return VoicePreset::Distortion;
        case MENU_ID_PRESET_WHISPER:    return VoicePreset::Whisper;
        case MENU_ID_PRESET_TELEPHONE:  return VoicePreset::Telephone;
        case MENU_ID_PRESET_UNDERWATER: return VoicePreset::Underwater;
        case MENU_ID_PRESET_MEGAPHONE:  return VoicePreset::Megaphone;
        case MENU_ID_PRESET_VOLBOOST:   return VoicePreset::VolumeBoost;
        default:                        return VoicePreset::Off;
    }
}

const char* presetName(VoicePreset p) {
    switch (p) {
        case VoicePreset::Off:        return "Off";
        case VoicePreset::Helium:     return "Helium";
        case VoicePreset::Chipmunk:   return "Chipmunk";
        case VoicePreset::Deep:       return "Deep";
        case VoicePreset::Demon:      return "Demon";
        case VoicePreset::Robot:      return "Robot";
        case VoicePreset::Echo:       return "Echo";
        case VoicePreset::Custom:     return "Custom";
        case VoicePreset::Distortion: return "Distortion";
        case VoicePreset::Whisper:    return "Whisper";
        case VoicePreset::Telephone:  return "Telephone";
        case VoicePreset::Underwater: return "Underwater";
        case VoicePreset::Megaphone:  return "Megaphone";
        case VoicePreset::VolumeBoost:return "VolumeBoost";
        default:                      return "?";
    }
}

void notifyTab(uint64 schid, const char* text) {
    if (ts3Functions.printMessage && schid != 0) {
        ts3Functions.printMessage(schid, text, PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "VoiceChanger", 0);
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

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "Voice Changer"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() { return "1.2.4"; }

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
        "Voice Changer - real-time DSP effects on outgoing microphone "
        "audio (Helium, Demon, Robot, Echo, Custom pitch). Plugins menu -> Voice Changer.");
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
    g_engine = std::make_unique<VoiceEngine>();
    logMsg("Voice Changer plugin loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    if (g_engine) {
        g_engine->setEnabled(false);
        g_engine.reset();
    }
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
const char* ts3plugin_commandKeyword() { return "voicechanger"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command || !g_engine) return 1;
    if (std::strcmp(command, "on") == 0 || std::strcmp(command, "enable") == 0) {
        VoiceConfig c = g_engine->config();
        c.enabled = true;
        g_engine->setConfig(c);
        notifyTab(schid, "[VoiceChanger] Enabled.");
        return 0;
    }
    if (std::strcmp(command, "off") == 0 || std::strcmp(command, "disable") == 0) {
        VoiceConfig c = g_engine->config();
        c.enabled = false;
        g_engine->setConfig(c);
        notifyTab(schid, "[VoiceChanger] Disabled.");
        return 0;
    }
    if (std::strcmp(command, "settings") == 0) {
        settings_dialog::run(*g_engine);
        return 0;
    }
    return 1;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    struct Spec { int id; const char* text; };
    constexpr Spec items[] = {
        // Per-preset shortcuts — one-click pick & enable
        { MENU_ID_PRESET_OFF,        "VC: Off (no effect)" },
        { MENU_ID_PRESET_VOLBOOST,   "VC: Volume Boost (test, 1.5x)" },
        { MENU_ID_PRESET_HELIUM,     "VC: Helium (+6 semitones)" },
        { MENU_ID_PRESET_CHIPMUNK,   "VC: Chipmunk (+12 semitones)" },
        { MENU_ID_PRESET_DEEP,       "VC: Deep (-4 semitones)" },
        { MENU_ID_PRESET_DEMON,      "VC: Demon (-7 + grit)" },
        { MENU_ID_PRESET_ROBOT,      "VC: Robot (AM)" },
        { MENU_ID_PRESET_ECHO,       "VC: Echo (delay)" },
        { MENU_ID_PRESET_DISTORTION, "VC: Distortion (raspy)" },
        { MENU_ID_PRESET_WHISPER,    "VC: Whisper (breathy)" },
        { MENU_ID_PRESET_TELEPHONE,  "VC: Telephone (300-3400 Hz)" },
        { MENU_ID_PRESET_UNDERWATER, "VC: Underwater (muffled)" },
        { MENU_ID_PRESET_MEGAPHONE,  "VC: Megaphone (PA)" },
        // Actions
        { MENU_ID_SETTINGS,          "VC: Settings..." },
        { MENU_ID_ENABLE,            "VC: Enable" },
        { MENU_ID_DISABLE,           "VC: Disable" },
        { MENU_ID_TOGGLE_COMPAT,     "VC: Toggle Compat Mode (no edited flag)" },
        { MENU_ID_ABOUT,             "VC: About..." },
    };
    constexpr int kCount = sizeof(items) / sizeof(items[0]);

    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * (kCount + 1)));
    for (int i = 0; i < kCount; ++i) {
        (*menuItems)[i] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL,
                                       items[i].id,
                                       items[i].text);
    }
    (*menuItems)[kCount] = nullptr;

    *menuIcon = static_cast<char*>(std::malloc(PLUGIN_MENU_BUFSZ));
    _strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "");
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

    // Preset shortcuts — one click sets preset and enables.
    if (menuItemID >= MENU_ID_PRESET_OFF && menuItemID <= MENU_ID_PRESET_VOLBOOST) {
        VoiceConfig c = g_engine->config();
        c.preset = menuToPreset(menuItemID);
        c.enabled = (c.preset != VoicePreset::Off);
        g_engine->setConfig(c);
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "[VoiceChanger] %s (%s).",
                      presetName(c.preset),
                      c.enabled ? "ENABLED" : "disabled");
        notifyTab(schid, buf);
        return;
    }

    switch (menuItemID) {
        case MENU_ID_SETTINGS:
            settings_dialog::run(*g_engine);
            break;
        case MENU_ID_ENABLE: {
            VoiceConfig c = g_engine->config();
            c.enabled = true;
            g_engine->setConfig(c);
            notifyTab(schid, "[VoiceChanger] Enabled.");
            break;
        }
        case MENU_ID_DISABLE: {
            VoiceConfig c = g_engine->config();
            c.enabled = false;
            g_engine->setConfig(c);
            notifyTab(schid, "[VoiceChanger] Disabled.");
            break;
        }
        case MENU_ID_TOGGLE_COMPAT: {
            VoiceConfig c = g_engine->config();
            c.compatMode = !c.compatMode;
            g_engine->setConfig(c);
            char buf[200];
            std::snprintf(buf, sizeof(buf),
                "[VoiceChanger] Compat mode %s. %s",
                c.compatMode ? "ON" : "OFF",
                c.compatMode
                    ? "Plugin no longer flags audio as edited - mic should work but voice changes won't transmit."
                    : "Normal mode - voice changes transmit but mic may be blocked.");
            notifyTab(schid, buf);
            break;
        }
        case MENU_ID_ABOUT:
            notifyTab(schid,
                "[VoiceChanger] " ZH_AUTHOR " | " ZH_COPYRIGHT
                " | https://github.com/ZeddiS/zeddihub-teamspeak-voicechanger");
            break;
    }
}

// THE audio callback. TS3 calls us before sending captured mic data to
// the server. We modify samples in place and set *edited = 1.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onEditCapturedVoiceDataEvent(uint64 schid,
                                            short* samples,
                                            int sampleCount,
                                            int channels,
                                            int* edited) {
    (void)schid;
    if (!g_engine || !samples || sampleCount <= 0) return;
    bool modified = g_engine->processSamples(samples, sampleCount, channels);
    if (modified && edited && !g_engine->config().compatMode) {
        *edited = 1;
    }
}

}  // extern "C"

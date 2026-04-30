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
    MENU_ID_SETTINGS = 1,
    MENU_ID_ENABLE,
    MENU_ID_DISABLE,
};

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
const char* ts3plugin_version() { return "1.2.0"; }

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
        "Voice Changer - DSP efekty pro mikrofon (Helium, Demon, "
        "Robot, Echo, Custom pitch). Plugins menu -> Voice Changer.");
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
        notifyTab(schid, "[VoiceChanger] Zapnuto.");
        return 0;
    }
    if (std::strcmp(command, "off") == 0 || std::strcmp(command, "disable") == 0) {
        VoiceConfig c = g_engine->config();
        c.enabled = false;
        g_engine->setConfig(c);
        notifyTab(schid, "[VoiceChanger] Vypnuto.");
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
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * 4));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SETTINGS,
                                   "Voice Changer Settings...");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_ENABLE,
                                   "Enable Voice Changer");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_DISABLE,
                                   "Disable Voice Changer");
    (*menuItems)[3] = nullptr;

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

    switch (menuItemID) {
        case MENU_ID_SETTINGS:
            settings_dialog::run(*g_engine);
            break;
        case MENU_ID_ENABLE: {
            VoiceConfig c = g_engine->config();
            c.enabled = true;
            g_engine->setConfig(c);
            notifyTab(schid, "[VoiceChanger] Zapnuto.");
            break;
        }
        case MENU_ID_DISABLE: {
            VoiceConfig c = g_engine->config();
            c.enabled = false;
            g_engine->setConfig(c);
            notifyTab(schid, "[VoiceChanger] Vypnuto.");
            break;
        }
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
    if (modified && edited) {
        *edited = 1;
    }
}

}  // extern "C"

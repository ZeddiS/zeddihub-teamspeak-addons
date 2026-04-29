// ZeddiHub Pokebot — TeamSpeak 3 plugin (client 3.5.6 / API v26)
//
// Right-click a client in TS3 → context menu items run preset poke campaigns
// or open a Qt dialog for a custom campaign. Stop interrupts current campaign.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "plugin_definitions.h"
#include "ts3_functions.h"

#include "../../common/zh_brand.h"
#include "custom_dialog.h"
#include "poke_engine.h"
#include "presets.h"

// PLUGIN_API_VERSION is supplied by CMake (-DPLUGIN_API_VERSION=25|26).
// Default to 26 if no define passed (matches newer TS3 clients).
#ifndef PLUGIN_API_VERSION
#define PLUGIN_API_VERSION 26
#endif

#define PATH_BUFSIZE 512

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s((dest), (destSize), (src))
#else
#define _strcpy(dest, destSize, src)         \
    do {                                      \
        std::strncpy((dest), (src), (destSize) - 1); \
        (dest)[(destSize) - 1] = '\0';        \
    } while (0)
#endif

namespace {

struct TS3Functions ts3Functions;
char* g_pluginID = nullptr;
std::unique_ptr<PokeEngine> g_engine;

enum MenuID : int {
    MENU_ID_WAKEUP_CZ = 1,
    MENU_ID_HALT,
    MENU_ID_SYMBOL_STORM,
    MENU_ID_SILENT,
    MENU_ID_MAX_SPAM,
    MENU_ID_CUSTOM,
    MENU_ID_STOP,
    MENU_ID_GLOBAL_STOP,
    MENU_ID_GLOBAL_STATUS,
    MENU_ID_GLOBAL_ABOUT,
};

struct MenuSpec {
    MenuID id;
    const char* text;
};

constexpr MenuSpec kMenuSpecs[] = {
    { MENU_ID_WAKEUP_CZ,    "Poke Bot: Wake-up CZ" },
    { MENU_ID_HALT,         "Poke Bot: Halt! (interval)" },
    { MENU_ID_SYMBOL_STORM, "Poke Bot: Symbol Storm" },
    { MENU_ID_SILENT,       "Poke Bot: Silent poke" },
    { MENU_ID_MAX_SPAM,     "Poke Bot: MAX SPAM" },
    { MENU_ID_CUSTOM,       "Poke Bot: Custom..." },
    { MENU_ID_STOP,         "Poke Bot: STOP" },
};

PluginMenuItem* makeMenuItem(PluginMenuType type, int id, const char* text) {
    auto* m = static_cast<PluginMenuItem*>(std::malloc(sizeof(PluginMenuItem)));
    m->type = type;
    m->id = id;
    _strcpy(m->text, PLUGIN_MENU_BUFSZ, text);
    _strcpy(m->icon, PLUGIN_MENU_BUFSZ, "");
    return m;
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "PokeBot", 0);
    }
}

void notifyTab(uint64 schid, const std::string& text) {
    if (ts3Functions.printMessage) {
        ts3Functions.printMessage(schid, text.c_str(), PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() {
    return "Poke Bot";
}

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() {
    return "1.0.0";
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_apiVersion() {
    return PLUGIN_API_VERSION;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_author() {
    return ZH_AUTHOR;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_description() {
    return ZH_DESC(
        "Poke Bot - pravym klikem na uzivatele vyber preset poke kampan "
        "nebo otevri custom dialog (burst / schedule mode).");
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
    g_engine = std::make_unique<PokeEngine>(&ts3Functions);
    logMsg("Poke Bot loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    if (g_engine) {
        g_engine->stop();
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

// REQUIRED: TS3 calls this to free any heap memory the plugin returned
// (commandKeyword string, menu items array, hotkeys array, etc.).
// Without this export, TS3 silently skips the plugin's menu callbacks.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_freeMemory(void* data) {
    std::free(data);
}

// Tell TS3 not to auto-enable the plugin on first load (user opts in).
#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_requestAutoload() {
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_commandKeyword() {
    return "zhpoke";
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command) return 1;
    if (std::strcmp(command, "stop") == 0) {
        if (g_engine) g_engine->stop();
        notifyTab(schid, "[Poke Bot] Kampaň zastavena.");
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        if (g_engine && g_engine->isRunning()) {
            char buf[128];
            std::snprintf(buf, sizeof(buf),
                          "[Poke Bot] Bezi: %d / %d poke odeslano.",
                          g_engine->sent(), g_engine->total());
            notifyTab(schid, buf);
        } else {
            notifyTab(schid, "[Poke Bot] Idle.");
        }
        return 0;
    }
    return 1;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    constexpr int kClientCount = sizeof(kMenuSpecs) / sizeof(kMenuSpecs[0]);
    constexpr int kGlobalCount = 3;
    constexpr int kTotal = kClientCount + kGlobalCount;
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * (kTotal + 1)));
    int idx = 0;
    for (int i = 0; i < kClientCount; ++i) {
        (*menuItems)[idx++] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT,
                                           kMenuSpecs[i].id,
                                           kMenuSpecs[i].text);
    }
    (*menuItems)[idx++] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL,
                                       MENU_ID_GLOBAL_STOP,
                                       "Poke Bot: STOP campaign");
    (*menuItems)[idx++] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL,
                                       MENU_ID_GLOBAL_STATUS,
                                       "Poke Bot: Status");
    (*menuItems)[idx++] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL,
                                       MENU_ID_GLOBAL_ABOUT,
                                       "Poke Bot: About...");
    (*menuItems)[idx] = nullptr;

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
    if (!g_engine) return;

    // Global menu (Plugins top bar) ----------------------------------
    if (type == PLUGIN_MENU_TYPE_GLOBAL) {
        if (schid == 0 && ts3Functions.getCurrentServerConnectionHandlerID) {
            schid = ts3Functions.getCurrentServerConnectionHandlerID();
        }
        switch (menuItemID) {
            case MENU_ID_GLOBAL_STOP:
                g_engine->stop();
                notifyTab(schid, "[Poke Bot] Kampaň zastavena.");
                return;
            case MENU_ID_GLOBAL_STATUS: {
                char buf[160];
                std::snprintf(buf, sizeof(buf),
                              "[Poke Bot] %s — %d / %d poke odeslano.",
                              g_engine->isRunning() ? "běží" : "idle",
                              g_engine->sent(), g_engine->total());
                notifyTab(schid, buf);
                return;
            }
            case MENU_ID_GLOBAL_ABOUT:
                notifyTab(schid,
                    "[Poke Bot] " ZH_AUTHOR " — " ZH_COPYRIGHT
                    " — https://github.com/ZeddiS/zeddihub-teamspeak-addons");
                return;
        }
        return;
    }

    if (type != PLUGIN_MENU_TYPE_CLIENT) return;
    const anyID clientID = static_cast<anyID>(selectedItemID);

    auto runJob = [&](PokeJob job) {
        g_engine->start(job);
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "[Poke Bot] '%s' spusten — %d poke, mod=%s.",
                      job.label.c_str(),
                      job.count,
                      job.mode == PokeMode::Burst ? "Burst" : "Schedule");
        notifyTab(schid, buf);
    };

    switch (menuItemID) {
        case MENU_ID_WAKEUP_CZ:
            runJob(presets::wakeupCz(schid, clientID));
            break;
        case MENU_ID_HALT:
            runJob(presets::halt(schid, clientID));
            break;
        case MENU_ID_SYMBOL_STORM:
            runJob(presets::symbolStorm(schid, clientID));
            break;
        case MENU_ID_SILENT:
            runJob(presets::silent(schid, clientID));
            break;
        case MENU_ID_MAX_SPAM:
            runJob(presets::maxSpam(schid, clientID));
            break;
        case MENU_ID_CUSTOM:
            custom_dialog::run(schid, clientID, *g_engine);
            break;
        case MENU_ID_STOP:
            g_engine->stop();
            notifyTab(schid, "[Poke Bot] Kampaň zastavena.");
            break;
        default:
            break;
    }
}

}  // extern "C"

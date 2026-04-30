// MoveSpam — TeamSpeak 3 plugin
//
// Right-click client → MoveSpam: Basic / Custom... / STOP.
// Basic:  alternate target between his current channel and server-default.
// Custom: Qt dialog — channel name/ID + interval slider.

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
#include "move_engine.h"

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
std::unique_ptr<MoveEngine> g_engine;

enum MenuID : int {
    MENU_ID_BASIC = 1,
    MENU_ID_CUSTOM,
    MENU_ID_STOP,
    MENU_ID_GLOBAL_STOP,
    MENU_ID_GLOBAL_STATUS,
    MENU_ID_GLOBAL_ABOUT,
};

void notifyTab(uint64 schid, const std::string& text) {
    if (ts3Functions.printMessage) {
        ts3Functions.printMessage(schid, text.c_str(), PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "MoveSpam", 0);
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

uint64 getCurrentChannelOf(uint64 schid, anyID clientID) {
    uint64 ch = 0;
    if (ts3Functions.getChannelOfClient(schid, clientID, &ch) != ERROR_ok) return 0;
    return ch;
}

// Resolve the server's default channel via CHANNEL_FLAG_DEFAULT. Falls
// back to the channel with the lowest ID (typically the lobby).
uint64 getDefaultChannel(uint64 schid) {
    uint64* channelList = nullptr;
    if (ts3Functions.getChannelList(schid, &channelList) != ERROR_ok || !channelList) {
        return 1;
    }

    uint64 chosen = 0;
    uint64 lowest = 0;
    for (uint64* p = channelList; *p; ++p) {
        if (lowest == 0 || *p < lowest) lowest = *p;
        int isDefault = 0;
        if (ts3Functions.getChannelVariableAsInt(
                schid, *p, CHANNEL_FLAG_DEFAULT, &isDefault) == ERROR_ok &&
            isDefault) {
            chosen = *p;
            break;
        }
    }
    ts3Functions.freeMemory(channelList);
    return chosen ? chosen : (lowest ? lowest : 1);
}

// Resolve channel ID by name (case-insensitive substring match).
uint64 findChannelByName(uint64 schid, const char* name) {
    if (!name || !*name) return 0;

    uint64* channelList = nullptr;
    if (ts3Functions.getChannelList(schid, &channelList) != ERROR_ok || !channelList) {
        return 0;
    }

    std::string needle(name);
    for (auto& c : needle) c = (char)std::tolower((unsigned char)c);

    uint64 found = 0;
    for (uint64* p = channelList; *p; ++p) {
        char* chName = nullptr;
        if (ts3Functions.getChannelVariableAsString(
                schid, *p, CHANNEL_NAME, &chName) == ERROR_ok && chName) {
            std::string hay(chName);
            ts3Functions.freeMemory(chName);
            std::string hayLow = hay;
            for (auto& c : hayLow) c = (char)std::tolower((unsigned char)c);
            if (hayLow.find(needle) != std::string::npos) {
                found = *p;
                break;
            }
        }
    }
    ts3Functions.freeMemory(channelList);
    return found;
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "MoveSpam"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() { return "1.0.0"; }

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
        "MoveSpam - repeatedly move target between two channels.");
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
    g_engine = std::make_unique<MoveEngine>(&ts3Functions);
    logMsg("MoveSpam plugin loaded.");
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
const char* ts3plugin_commandKeyword() { return "movespam"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command) return 1;
    if (std::strcmp(command, "stop") == 0) {
        if (g_engine) g_engine->stop();
        notifyTab(schid, "[MoveSpam] Stopped.");
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
                      "[MoveSpam] %s - %d moves sent.",
                      (g_engine && g_engine->isRunning()) ? "running" : "idle",
                      g_engine ? g_engine->sent() : 0);
        notifyTab(schid, buf);
        return 0;
    }
    return 1;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * 7));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_BASIC,
                                   "MoveSpam: Basic (current <-> default)");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CUSTOM,
                                   "MoveSpam: Custom...");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_STOP,
                                   "MoveSpam: STOP");
    (*menuItems)[3] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_STOP,
                                   "MoveSpam: STOP campaign");
    (*menuItems)[4] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_STATUS,
                                   "MoveSpam: Status");
    (*menuItems)[5] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_ABOUT,
                                   "MoveSpam: About...");
    (*menuItems)[6] = nullptr;

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

    if (type == PLUGIN_MENU_TYPE_GLOBAL) {
        if (schid == 0 && ts3Functions.getCurrentServerConnectionHandlerID) {
            schid = ts3Functions.getCurrentServerConnectionHandlerID();
        }
        switch (menuItemID) {
            case MENU_ID_GLOBAL_STOP:
                g_engine->stop();
                notifyTab(schid, "[MoveSpam] Stopped.");
                return;
            case MENU_ID_GLOBAL_STATUS: {
                char buf[160];
                std::snprintf(buf, sizeof(buf),
                              "[MoveSpam] %s - %d moves sent.",
                              g_engine->isRunning() ? "running" : "idle",
                              g_engine->sent());
                notifyTab(schid, buf);
                return;
            }
            case MENU_ID_GLOBAL_ABOUT:
                notifyTab(schid,
                    "[MoveSpam] " ZH_AUTHOR " | " ZH_COPYRIGHT
                    " | https://github.com/ZeddiS/zeddihub-teamspeak-movespam");
                return;
        }
        return;
    }

    if (type != PLUGIN_MENU_TYPE_CLIENT) return;
    const anyID clientID = static_cast<anyID>(selectedItemID);

    switch (menuItemID) {
        case MENU_ID_BASIC: {
            uint64 cur = getCurrentChannelOf(schid, clientID);
            uint64 def = getDefaultChannel(schid);
            if (cur == 0 || def == 0 || cur == def) {
                notifyTab(schid, "[MoveSpam] Could not resolve channels "
                                 "(target or default missing, or they are the same).");
                return;
            }
            MoveJob j;
            j.schid = schid;
            j.clientID = clientID;
            j.channelA = cur;
            j.channelB = def;
            j.intervalMs = 1500;
            j.maxMoves = 200;
            g_engine->start(j);
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "[MoveSpam] Basic mode started (cur=%llu <-> def=%llu).",
                          (unsigned long long)cur, (unsigned long long)def);
            notifyTab(schid, buf);
            break;
        }
        case MENU_ID_CUSTOM: {
            uint64 cur = getCurrentChannelOf(schid, clientID);
            auto r = custom_dialog::runMoveSpamDialog(cur);
            if (!r.accepted) return;

            uint64 dest = r.destChannelID;
            if (dest == 0 && r.destName[0] != '\0') {
                dest = findChannelByName(schid, r.destName);
            }
            if (dest == 0 || cur == 0 || cur == dest) {
                notifyTab(schid, "[MoveSpam] Target channel not resolved "
                                 "or same as current.");
                return;
            }
            MoveJob j;
            j.schid = schid;
            j.clientID = clientID;
            j.channelA = cur;
            j.channelB = dest;
            j.intervalMs = r.intervalMs;
            j.maxMoves = r.maxMoves;
            g_engine->start(j);
            notifyTab(schid, "[MoveSpam] Custom mode started.");
            break;
        }
        case MENU_ID_STOP:
            g_engine->stop();
            notifyTab(schid, "[MoveSpam] Stopped.");
            break;
    }
}

}  // extern "C"

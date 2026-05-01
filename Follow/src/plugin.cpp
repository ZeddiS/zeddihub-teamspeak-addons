// Follow — TeamSpeak 3 plugin (v1.2.0 polling-based)
//
// Right-click any client → "Follow Bot: Start Following".
// A worker thread polls the target's channel every 1 second and moves us
// in if we're not already there. The plugin NEVER auto-stops on errors —
// only manual STOP or plugin shutdown ends the follow.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "plugin_definitions.h"
#include "ts3_functions.h"

#include "../../common/zh_brand.h"

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

// Tracking state ----------------------------------------------------------
std::atomic<bool>     g_active{false};
std::atomic<uint64>   g_followSchid{0};
std::atomic<anyID>    g_followClientID{0};

std::thread g_worker;
std::atomic<bool> g_workerStop{false};
std::condition_variable g_workerCv;
std::mutex g_workerMu;

enum MenuID : int {
    MENU_ID_START = 1,
    MENU_ID_STOP,
    MENU_ID_GLOBAL_STOP,
    MENU_ID_GLOBAL_STATUS,
    MENU_ID_GLOBAL_ABOUT,
};

void notifyTab(uint64 schid, const std::string& text) {
    if (ts3Functions.printMessage && schid != 0) {
        ts3Functions.printMessage(schid, text.c_str(), PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "Follow", 0);
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

void getClientName(uint64 schid, anyID clientID, std::string& out) {
    char* name = nullptr;
    if (ts3Functions.getClientVariableAsString(
            schid, clientID, CLIENT_NICKNAME, &name) == ERROR_ok && name) {
        out.assign(name);
        ts3Functions.freeMemory(name);
    } else {
        out.clear();
    }
}

void stopFollowingInternal(uint64 schid, const char* reason) {
    if (!g_active.exchange(false)) return;
    g_followSchid.store(0);
    g_followClientID.store(0);
    g_workerCv.notify_all();
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "[Follow] Following stopped%s%s.",
                  (reason && *reason) ? ": " : "",
                  reason ? reason : "");
    notifyTab(schid, buf);
}

// Polling worker -----------------------------------------------------------
// Every poll interval, check target's current channel and our own. If
// different, request move. Never stops itself on errors — only when
// g_active is set false externally.
void workerLoop() {
    constexpr auto kPollInterval = std::chrono::milliseconds(1000);
    int silentTicks = 0;

    while (!g_workerStop.load()) {
        std::unique_lock<std::mutex> lk(g_workerMu);
        g_workerCv.wait_for(lk, kPollInterval, [] {
            return g_workerStop.load() || !g_active.load();
        });
        if (g_workerStop.load()) break;
        if (!g_active.load()) continue;

        const uint64 schid = g_followSchid.load();
        const anyID  target = g_followClientID.load();
        if (schid == 0 || target == 0) continue;

        // Get target channel
        uint64 targetCh = 0;
        unsigned int rc = ts3Functions.getChannelOfClient(schid, target, &targetCh);
        if (rc != ERROR_ok || targetCh == 0) {
            // Target invisible / left briefly — keep waiting (10s grace)
            if (++silentTicks > 10) {
                stopFollowingInternal(schid, "target unreachable for 10s+");
            }
            continue;
        }
        silentTicks = 0;

        // Get our channel
        anyID myID = 0;
        if (ts3Functions.getClientID(schid, &myID) != ERROR_ok || myID == 0) continue;
        uint64 myCh = 0;
        if (ts3Functions.getChannelOfClient(schid, myID, &myCh) != ERROR_ok) continue;

        if (myCh == targetCh) continue;  // already following

        // Try to move. Ignore return value — server will fail silently if
        // we lack permission, and we'll just keep polling.
        ts3Functions.requestClientMove(schid, myID, targetCh, "", nullptr);
    }
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "Follow"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() { return "1.3.0"; }

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
        "Follow another client (polling-based, never auto-stops). "
        "Right-click -> Start Following.");
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
    g_workerStop.store(false);
    g_worker = std::thread(workerLoop);
    logMsg("Follow plugin loaded (v1.2.0 polling).");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    g_active.store(false);
    g_workerStop.store(true);
    g_workerCv.notify_all();
    if (g_worker.joinable()) g_worker.join();
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
const char* ts3plugin_commandKeyword() { return "follow"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command) return 1;
    if (std::strcmp(command, "stop") == 0) {
        stopFollowingInternal(schid, "manual /follow stop");
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        if (g_active.load()) {
            anyID t = g_followClientID.load();
            std::string name;
            getClientName(g_followSchid.load(), t, name);
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "[Follow] Active - tracking '%s' (id=%u). Polling every 1s.",
                          name.c_str(), t);
            notifyTab(schid, buf);
        } else {
            notifyTab(schid, "[Follow] Idle.");
        }
        return 0;
    }
    return 1;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * 6));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_START,
                                   "Follow Bot: Start Following");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_STOP,
                                   "Follow Bot: Stop Following");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_STOP,
                                   "Follow Bot: Stop Following");
    (*menuItems)[3] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_STATUS,
                                   "Follow Bot: Status");
    (*menuItems)[4] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_ABOUT,
                                   "Follow Bot: About...");
    (*menuItems)[5] = nullptr;

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
    if (type == PLUGIN_MENU_TYPE_GLOBAL) {
        if (schid == 0 && ts3Functions.getCurrentServerConnectionHandlerID) {
            schid = ts3Functions.getCurrentServerConnectionHandlerID();
        }
        switch (menuItemID) {
            case MENU_ID_GLOBAL_STOP:
                stopFollowingInternal(schid, "manual stop");
                return;
            case MENU_ID_GLOBAL_STATUS: {
                if (g_active.load()) {
                    anyID t = g_followClientID.load();
                    std::string name;
                    getClientName(g_followSchid.load(), t, name);
                    char buf[256];
                    std::snprintf(buf, sizeof(buf),
                                  "[Follow] Active - tracking '%s' (id=%u).",
                                  name.c_str(), t);
                    notifyTab(schid, buf);
                } else {
                    notifyTab(schid, "[Follow] Idle.");
                }
                return;
            }
            case MENU_ID_GLOBAL_ABOUT:
                notifyTab(schid,
                    "[Follow] " ZH_AUTHOR " | " ZH_COPYRIGHT
                    " | https://github.com/ZeddiS/zeddihub-teamspeak-follow");
                return;
        }
        return;
    }

    if (type != PLUGIN_MENU_TYPE_CLIENT) return;
    const anyID clientID = static_cast<anyID>(selectedItemID);

    if (menuItemID == MENU_ID_START) {
        // Capture target. Worker will poll and move.
        g_followSchid.store(schid);
        g_followClientID.store(clientID);
        g_active.store(true);
        g_workerCv.notify_all();

        std::string name;
        getClientName(schid, clientID, name);
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "[Follow] Tracking '%s' (id=%u). Polling every 1s. "
                      "Stop via right-click menu or /follow stop.",
                      name.c_str(), clientID);
        notifyTab(schid, buf);
    } else if (menuItemID == MENU_ID_STOP) {
        stopFollowingInternal(schid, nullptr);
    }
}

}  // extern "C"

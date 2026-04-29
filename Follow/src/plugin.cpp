// Follow — TeamSpeak 3 plugin
//
// Right-click any client → "Follow Bot: Start Following".
// When that client changes channel, this plugin auto-moves us into the
// same channel. "Follow Bot: Stop Following" cancels.
//
// Single-target mode with auto-stop on move error.

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "teamspeak/public_definitions.h"
#include "teamspeak/public_errors.h"
#include "plugin_definitions.h"
#include "ts3_functions.h"

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

enum MenuID : int {
    MENU_ID_START = 1,
    MENU_ID_STOP,
};

void notifyTab(uint64 schid, const std::string& text) {
    if (ts3Functions.printMessage) {
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

// Move our own client into the same channel as target -------------------
bool moveSelfTo(uint64 schid, uint64 channelID) {
    anyID myID = 0;
    if (ts3Functions.getClientID(schid, &myID) != ERROR_ok) return false;
    if (myID == 0) return false;
    unsigned int err = ts3Functions.requestClientMove(
        schid, myID, channelID, "", nullptr);
    return err == ERROR_ok;
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

void stopFollowing(uint64 schid, const char* reason) {
    if (!g_active.exchange(false)) return;
    g_followSchid.store(0);
    g_followClientID.store(0);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "[Follow] Sledování zastaveno%s%s.",
                  (reason && *reason) ? ": " : "",
                  reason ? reason : "");
    notifyTab(schid, buf);
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
const char* ts3plugin_version() { return "1.0.0"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_apiVersion() { return PLUGIN_API_VERSION; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_author() { return "Follow"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_description() {
    return "Follow another client into channels — right-click → Start Following.";
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
    logMsg("Follow plugin loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    g_active.store(false);
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
        stopFollowing(schid, "manual /follow stop");
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        if (g_active.load()) {
            anyID t = g_followClientID.load();
            std::string name;
            getClientName(g_followSchid.load(), t, name);
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "[Follow] Aktivní — sleduji '%s' (id=%u).",
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
        std::malloc(sizeof(PluginMenuItem*) * 3));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_START,
                                   "Follow Bot: Start Following");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_STOP,
                                   "Follow Bot: Stop Following");
    (*menuItems)[2] = nullptr;

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
    if (type != PLUGIN_MENU_TYPE_CLIENT) return;
    const anyID clientID = static_cast<anyID>(selectedItemID);

    if (menuItemID == MENU_ID_START) {
        // Get target's current channel and follow there immediately
        uint64 targetCh = 0;
        if (ts3Functions.getChannelOfClient(schid, clientID, &targetCh) == ERROR_ok &&
            targetCh != 0) {
            moveSelfTo(schid, targetCh);
        }
        g_followSchid.store(schid);
        g_followClientID.store(clientID);
        g_active.store(true);

        std::string name;
        getClientName(schid, clientID, name);
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "[Follow] Sleduji '%s' (id=%u). Stop přes pravé menu nebo /follow stop.",
                      name.c_str(), clientID);
        notifyTab(schid, buf);
    } else if (menuItemID == MENU_ID_STOP) {
        stopFollowing(schid, nullptr);
    }
}

// onClientMoveEvent — fires when ANY client (including us, others) moves
// between channels. We filter for our target and react.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onClientMoveEvent(uint64 schid,
                                 anyID clientID,
                                 uint64 oldChannelID,
                                 uint64 newChannelID,
                                 int visibility,
                                 const char* moveMessage) {
    (void)oldChannelID;
    (void)visibility;
    (void)moveMessage;

    if (!g_active.load()) return;
    if (g_followSchid.load() != schid) return;
    if (g_followClientID.load() != clientID) return;
    if (newChannelID == 0) {
        // Target disconnected
        stopFollowing(schid, "uživatel se odpojil");
        return;
    }

    if (!moveSelfTo(schid, newChannelID)) {
        stopFollowing(schid, "nemám oprávnění do cílového kanálu");
    }
}

// onClientMoveTimeoutEvent — target was disconnected by timeout.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onClientMoveTimeoutEvent(uint64 schid,
                                        anyID clientID,
                                        uint64 oldChannelID,
                                        uint64 newChannelID,
                                        int visibility,
                                        const char* timeoutMessage) {
    (void)oldChannelID;
    (void)newChannelID;
    (void)visibility;
    (void)timeoutMessage;

    if (!g_active.load()) return;
    if (g_followSchid.load() != schid) return;
    if (g_followClientID.load() == clientID) {
        stopFollowing(schid, "uživatel timeoutoval");
    }
}

// Server told us we couldn't perform an action. If it's our move that
// failed, stop following.
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onServerErrorEvent(uint64 schid,
                                  const char* errorMessage,
                                  unsigned int error,
                                  const char* returnCode,
                                  const char* extraMessage) {
    (void)errorMessage;
    (void)returnCode;
    (void)extraMessage;
    if (!g_active.load()) return;
    if (error != ERROR_ok) {
        // Only auto-stop on clear move/permission failures.
        if (error == ERROR_channel_invalid_password ||
            error == ERROR_channel_invalid_id ||
            error == ERROR_permissions ||
            error == ERROR_permissions_client_insufficient) {
            stopFollowing(schid, "server odmítl move");
        }
    }
}

}  // extern "C"

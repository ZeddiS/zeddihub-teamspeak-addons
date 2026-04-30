// GreetingBot — TeamSpeak 3 plugin (prank/utility)
//
// When a user joins YOUR current channel, automatically poke them with a
// configurable greeting message. Optional pseudo-personalization with
// "{name}" placeholder.
//
// Plugins menu (top bar):
//   - Greeting Bot: Toggle ON/OFF
//   - Greeting Bot: Status
//   - Greeting Bot: About...

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

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

std::atomic<bool> g_enabled{false};
std::mutex g_cfgMu;
std::string g_greeting = "Welcome {name}! :)";

enum MenuID : int {
    MENU_ID_TOGGLE = 1,
    MENU_ID_SET_GREETING,
    MENU_ID_STATUS,
    MENU_ID_ABOUT,
};

void notifyTab(uint64 schid, const char* text) {
    if (ts3Functions.printMessage && schid != 0) {
        ts3Functions.printMessage(schid, text, PLUGIN_MESSAGE_TARGET_SERVER);
    }
}

void logMsg(const char* msg, LogLevel level = LogLevel_INFO) {
    if (ts3Functions.logMessage) {
        ts3Functions.logMessage(msg, level, "GreetingBot", 0);
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

uint64 getMyChannel(uint64 schid) {
    anyID myID = 0;
    if (ts3Functions.getClientID(schid, &myID) != ERROR_ok) return 0;
    uint64 ch = 0;
    if (ts3Functions.getChannelOfClient(schid, myID, &ch) != ERROR_ok) return 0;
    return ch;
}

std::string clientName(uint64 schid, anyID clientID) {
    char* n = nullptr;
    if (ts3Functions.getClientVariableAsString(schid, clientID, CLIENT_NICKNAME, &n) ==
            ERROR_ok &&
        n) {
        std::string s(n);
        ts3Functions.freeMemory(n);
        return s;
    }
    return "?";
}

std::string substituteName(const std::string& tmpl, const std::string& name) {
    std::string out = tmpl;
    auto pos = out.find("{name}");
    while (pos != std::string::npos) {
        out.replace(pos, 6, name);
        pos = out.find("{name}", pos + name.size());
    }
    if (out.size() > 100) out.resize(100);  // TS3 poke limit
    return out;
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "Greeting Bot"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_version() { return "1.0.1"; }

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
        "GreetingBot - automatically pokes any user that joins your "
        "current channel. Plugins menu -> Greeting Bot.");
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
    logMsg("GreetingBot plugin loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    g_enabled.store(false);
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
const char* ts3plugin_commandKeyword() { return "greet"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command) return 1;
    if (std::strcmp(command, "on") == 0) {
        g_enabled.store(true);
        notifyTab(schid, "[GreetingBot] Enabled.");
        return 0;
    }
    if (std::strcmp(command, "off") == 0) {
        g_enabled.store(false);
        notifyTab(schid, "[GreetingBot] Disabled.");
        return 0;
    }
    if (std::strncmp(command, "set ", 4) == 0) {
        std::lock_guard<std::mutex> lk(g_cfgMu);
        g_greeting = command + 4;
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "[GreetingBot] Greeting set: '%s'", g_greeting.c_str());
        notifyTab(schid, buf);
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        std::lock_guard<std::mutex> lk(g_cfgMu);
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "[GreetingBot] %s. Greeting: '%s'",
                      g_enabled.load() ? "ON" : "OFF", g_greeting.c_str());
        notifyTab(schid, buf);
        return 0;
    }
    notifyTab(schid,
        "[GreetingBot] Commands: /greet on | off | set <text> | status. "
        "Use {name} for nickname placeholder.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    *menuItems = static_cast<PluginMenuItem**>(
        std::malloc(sizeof(PluginMenuItem*) * 5));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_TOGGLE,
                                   "Greeting Bot: Toggle ON/OFF");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_STATUS,
                                   "Greeting Bot: Status");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_ABOUT,
                                   "Greeting Bot: About...");
    (*menuItems)[3] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SET_GREETING,
                                   "Greeting Bot: How to set greeting");
    (*menuItems)[4] = nullptr;

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
    if (schid == 0 && ts3Functions.getCurrentServerConnectionHandlerID) {
        schid = ts3Functions.getCurrentServerConnectionHandlerID();
    }

    switch (menuItemID) {
        case MENU_ID_TOGGLE: {
            bool newState = !g_enabled.load();
            g_enabled.store(newState);
            notifyTab(schid, newState ? "[GreetingBot] Enabled."
                                     : "[GreetingBot] Disabled.");
            break;
        }
        case MENU_ID_STATUS: {
            std::lock_guard<std::mutex> lk(g_cfgMu);
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "[GreetingBot] %s. Greeting: '%s'",
                          g_enabled.load() ? "ON" : "OFF", g_greeting.c_str());
            notifyTab(schid, buf);
            break;
        }
        case MENU_ID_SET_GREETING:
            notifyTab(schid,
                "[GreetingBot] In chat type: /greet set <text>. "
                "Use {name} as nickname placeholder.");
            break;
        case MENU_ID_ABOUT:
            notifyTab(schid,
                "[GreetingBot] " ZH_AUTHOR " | " ZH_COPYRIGHT
                " | https://github.com/ZeddiS/zeddihub-teamspeak-greetingbot");
            break;
    }
}

// onClientMoveEvent: detect users entering my current channel.
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

    if (!g_enabled.load()) return;
    if (newChannelID == 0) return;  // disconnecting

    // Skip self
    anyID myID = 0;
    if (ts3Functions.getClientID(schid, &myID) != ERROR_ok || myID == clientID) {
        return;
    }

    uint64 myCh = getMyChannel(schid);
    if (myCh == 0 || myCh != newChannelID) return;

    std::string name = clientName(schid, clientID);
    std::string msg;
    {
        std::lock_guard<std::mutex> lk(g_cfgMu);
        msg = substituteName(g_greeting, name);
    }
    if (msg.empty()) msg = "Welcome!";

    ts3Functions.requestClientPoke(schid, clientID, msg.c_str(), nullptr);
}

}  // extern "C"

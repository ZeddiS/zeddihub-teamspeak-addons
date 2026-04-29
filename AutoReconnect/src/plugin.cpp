// AutoReconnect — TeamSpeak 3 plugin
//
// Detects unexpected disconnects (network drop, kicks aside) and
// automatically reconnects with exponential backoff.
//
// Plugins menu (top bar): "AutoReconnect Settings...", Enable/Disable, Status.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

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

// Captured connection params per server connection handler
struct ConnState {
    std::string host;
    unsigned int port = 0;
    std::string nickname;
    std::string identity;
    std::string defaultChannel;
    std::string serverPassword;  // empty if user didn't supply
    int retries = 0;
    std::chrono::steady_clock::time_point nextAttempt{};
};

std::mutex g_mu;
std::unordered_map<uint64, ConnState> g_states;

std::atomic<bool> g_enabled{true};
std::atomic<int>  g_maxRetries{10};
std::atomic<int>  g_baseDelayMs{2000};

std::thread g_worker;
std::atomic<bool> g_workerStop{false};
std::condition_variable g_workerCv;
std::mutex g_workerMu;

enum MenuID : int {
    MENU_ID_TOGGLE = 1,
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
        ts3Functions.logMessage(msg, level, "AutoReconnect", 0);
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

std::string getStringVar(uint64 schid, std::size_t flag, bool client) {
    char* v = nullptr;
    unsigned int err = client
        ? ts3Functions.getClientSelfVariableAsString(schid, (int)flag, &v)
        : ts3Functions.getServerVariableAsString(schid, (int)flag, &v);
    if (err == ERROR_ok && v) {
        std::string s(v);
        ts3Functions.freeMemory(v);
        return s;
    }
    return "";
}

void captureConnection(uint64 schid) {
    std::lock_guard<std::mutex> lk(g_mu);
    ConnState& s = g_states[schid];

    char host[256] = {0};
    unsigned short port = 0;
    char password[256] = {0};
    if (ts3Functions.getServerConnectInfo(schid, host, &port, password, sizeof(host)) == ERROR_ok) {
        s.host = host;
        s.port = port;
        // serverPassword may be empty
        if (password[0]) s.serverPassword = password;
    }
    s.nickname = getStringVar(schid, CLIENT_NICKNAME, true);
    // Identity is per-client TS3 identity string
    s.identity = getStringVar(schid, CLIENT_UNIQUE_IDENTIFIER, true);
    s.retries = 0;
    s.nextAttempt = {};
}

bool attemptReconnect(uint64 schid, ConnState& s) {
    if (s.host.empty() || s.port == 0) return false;

    // Use the same identity string TS3 currently uses (don't pass null)
    char* defaultArr[2] = { (char*)"", nullptr };
    unsigned int err = ts3Functions.startConnection(
        schid,
        s.identity.c_str(),
        s.host.c_str(),
        s.port,
        s.nickname.c_str(),
        (const char**)defaultArr,
        "",                     // default channel password
        s.serverPassword.c_str()
    );
    return err == ERROR_ok;
}

void workerLoop() {
    while (!g_workerStop.load()) {
        std::unique_lock<std::mutex> wlk(g_workerMu);
        g_workerCv.wait_for(wlk, std::chrono::seconds(1),
                            [] { return g_workerStop.load(); });
        if (g_workerStop.load()) break;

        if (!g_enabled.load()) continue;

        auto now = std::chrono::steady_clock::now();
        std::vector<uint64> due;
        {
            std::lock_guard<std::mutex> lk(g_mu);
            for (auto& [schid, s] : g_states) {
                if (s.nextAttempt.time_since_epoch().count() != 0 &&
                    now >= s.nextAttempt &&
                    s.retries < g_maxRetries.load()) {
                    due.push_back(schid);
                }
            }
        }

        for (uint64 schid : due) {
            // Re-check we're actually disconnected (avoid race with manual reconnect)
            int status = 0;
            ts3Functions.getConnectionStatus(schid, &status);
            if (status != STATUS_DISCONNECTED) continue;

            std::lock_guard<std::mutex> lk(g_mu);
            ConnState& s = g_states[schid];
            s.retries++;
            int delay = g_baseDelayMs.load() *
                        (1 << std::min(s.retries, 6));  // cap exp at 64x
            s.nextAttempt = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(delay);

            char buf[200];
            std::snprintf(buf, sizeof(buf),
                          "[AutoReconnect] Attempt %d (next in %dms) ...",
                          s.retries, delay);
            logMsg(buf);

            if (attemptReconnect(schid, s)) {
                // Connection initiated; wait for status callback to confirm
            }
        }
    }
}

void scheduleReconnect(uint64 schid) {
    if (!g_enabled.load()) return;
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_states.find(schid);
    if (it == g_states.end()) return;
    if (it->second.host.empty()) return;

    if (it->second.retries < g_maxRetries.load()) {
        int delay = g_baseDelayMs.load() * (1 << std::min(it->second.retries, 6));
        it->second.nextAttempt = std::chrono::steady_clock::now() +
                                 std::chrono::milliseconds(delay);
    }
    g_workerCv.notify_all();
}

}  // namespace

extern "C" {

#ifdef _WIN32
__declspec(dllexport)
#endif
const char* ts3plugin_name() { return "AutoReconnect"; }

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
        "AutoReconnect - on disconnect, auto-reconnect with exponential "
        "backoff. Plugins menu -> AutoReconnect.");
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
    logMsg("AutoReconnect plugin loaded.");
    return 0;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_shutdown() {
    g_workerStop.store(true);
    g_workerCv.notify_all();
    if (g_worker.joinable()) g_worker.join();
    {
        std::lock_guard<std::mutex> lk(g_mu);
        g_states.clear();
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
const char* ts3plugin_commandKeyword() { return "autoreconnect"; }

#ifdef _WIN32
__declspec(dllexport)
#endif
int ts3plugin_processCommand(uint64 schid, const char* command) {
    if (!command) return 1;
    if (std::strcmp(command, "on") == 0 || std::strcmp(command, "enable") == 0) {
        g_enabled.store(true);
        notifyTab(schid, "[AutoReconnect] Enabled.");
        return 0;
    }
    if (std::strcmp(command, "off") == 0 || std::strcmp(command, "disable") == 0) {
        g_enabled.store(false);
        notifyTab(schid, "[AutoReconnect] Disabled.");
        return 0;
    }
    if (std::strcmp(command, "status") == 0) {
        char buf[200];
        std::lock_guard<std::mutex> lk(g_mu);
        std::snprintf(buf, sizeof(buf),
                      "[AutoReconnect] %s, %zu servers tracked, max %d retries.",
                      g_enabled.load() ? "ON" : "OFF",
                      g_states.size(),
                      g_maxRetries.load());
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
        std::malloc(sizeof(PluginMenuItem*) * 4));
    (*menuItems)[0] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_TOGGLE,
                                   "AutoReconnect: Toggle ON/OFF");
    (*menuItems)[1] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_STATUS,
                                   "AutoReconnect: Status");
    (*menuItems)[2] = makeMenuItem(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_ABOUT,
                                   "AutoReconnect: About...");
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
    if (schid == 0 && ts3Functions.getCurrentServerConnectionHandlerID) {
        schid = ts3Functions.getCurrentServerConnectionHandlerID();
    }

    switch (menuItemID) {
        case MENU_ID_TOGGLE: {
            bool newState = !g_enabled.load();
            g_enabled.store(newState);
            notifyTab(schid, newState ? "[AutoReconnect] Enabled."
                                     : "[AutoReconnect] Disabled.");
            break;
        }
        case MENU_ID_STATUS: {
            char buf[200];
            std::lock_guard<std::mutex> lk(g_mu);
            std::snprintf(buf, sizeof(buf),
                          "[AutoReconnect] %s, %zu servers tracked.",
                          g_enabled.load() ? "ON" : "OFF",
                          g_states.size());
            notifyTab(schid, buf);
            break;
        }
        case MENU_ID_ABOUT:
            notifyTab(schid,
                "[AutoReconnect] " ZH_AUTHOR " - " ZH_COPYRIGHT
                " - https://github.com/ZeddiS/zeddihub-teamspeak-addons");
            break;
    }
}

// Connection state changed.
//   newStatus: STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED, ...
//   errorNumber: 0 if user-initiated; non-zero if server/network forced
#ifdef _WIN32
__declspec(dllexport)
#endif
void ts3plugin_onConnectStatusChangeEvent(uint64 schid,
                                          int newStatus,
                                          unsigned int errorNumber) {
    if (newStatus == STATUS_CONNECTED ||
        newStatus == STATUS_CONNECTION_ESTABLISHED) {
        captureConnection(schid);
        return;
    }

    if (newStatus == STATUS_DISCONNECTED) {
        if (errorNumber != ERROR_ok &&
            errorNumber != ERROR_client_not_logged_in /* user kicked */) {
            // Unexpected disconnect; schedule reconnect.
            scheduleReconnect(schid);
        }
    }
}

}  // extern "C"

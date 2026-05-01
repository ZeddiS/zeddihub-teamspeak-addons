#pragma once

#include <string>
#include <vector>

#include "poke_engine.h"

namespace presets {

inline std::vector<std::string> chunk(const std::string& s, std::size_t n = 100) {
    std::vector<std::string> out;
    if (s.empty()) {
        out.push_back(" ");
        return out;
    }
    for (std::size_t i = 0; i < s.size(); i += n) {
        out.push_back(s.substr(i, n));
    }
    return out;
}

inline PokeJob cz(uint64 schid, anyID clientID) {
    PokeJob j;
    j.label = "CZ";
    j.schid = schid;
    j.clientID = clientID;
    j.mode = PokeMode::Burst;
    j.messages = {
        "AHOJ KAMARADE",
        "VSTAVEJ!",
        "POZOR!",
        "HALO HALO",
        "PROBUD SE",
        "SLYSIS ME?",
        "OZVI SE",
        "TUK TUK TUK",
        "TADY ZEDDIHUB",
        "POJD SEM"
    };
    j.count = 20;
    j.burstDelayMs = 250;
    return j;
}

inline PokeJob symbolStorm(uint64 schid, anyID clientID) {
    PokeJob j;
    j.label = "Symbol Storm";
    j.schid = schid;
    j.clientID = clientID;
    j.mode = PokeMode::Burst;

    std::string spam;
    const char* segments[] = {
        "!!!##@@$$%%^^&&**(()(()()##!!@@..,,??!!??",
        "########################################",
        "!@#$%^&*()_+!@#$%^&*()_+!@#$%^&*()_+!@#$",
        "................????????????????????????",
        "**********!!!!!!!!!!**********!!!!!!!!!!",
        "<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>"
    };
    for (int round = 0; round < 6; ++round) {
        for (const char* s : segments) {
            spam += s;
        }
    }
    j.messages = chunk(spam, 100);
    j.count = static_cast<int>(j.messages.size());
    if (j.count > 40) j.count = 40;
    j.burstDelayMs = 220;
    return j;
}

inline PokeJob silent(uint64 schid, anyID clientID) {
    PokeJob j;
    j.label = "Silent";
    j.schid = schid;
    j.clientID = clientID;
    j.mode = PokeMode::Schedule;
    j.messages = { " " };
    j.count = 8;
    j.intervalMinMs = 2000;
    j.intervalMaxMs = 4500;
    return j;
}

// MAX SPAM — hard-cap burst, 50ms anti-flood floor, long symbol storm
// chunked to 100-char pieces. Risk of server kick is HIGH; use only on
// servers where you have permission.
inline PokeJob maxSpam(uint64 schid, anyID clientID) {
    PokeJob j;
    j.label = "MAX";
    j.schid = schid;
    j.clientID = clientID;
    j.mode = PokeMode::Burst;

    std::string spam;
    const char* segments[] = {
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@",
        "##################################################################################################",
        "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",
        "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
        "**************************************************************************************************",
    };
    for (int round = 0; round < 80; ++round) {
        spam += segments[round % (sizeof(segments) / sizeof(segments[0]))];
    }
    j.messages = chunk(spam, 100);
    j.count = 500;            // hard cap
    j.burstDelayMs = 50;      // anti-flood floor
    return j;
}

}  // namespace presets

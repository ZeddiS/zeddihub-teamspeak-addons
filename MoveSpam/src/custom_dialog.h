#pragma once

#include <cstdint>
using anyID = unsigned short;
using uint64 = unsigned long long;

class MoveEngine;

namespace custom_dialog {

struct Result {
    bool accepted = false;
    uint64 destChannelID = 0;     // resolved channel ID (0 if user typed name)
    char destName[128] = {0};     // user-typed channel name (if no ID)
    int intervalMs = 1500;
    int maxMoves = 200;
};

Result runMoveSpamDialog(uint64 currentChannel);

}  // namespace custom_dialog

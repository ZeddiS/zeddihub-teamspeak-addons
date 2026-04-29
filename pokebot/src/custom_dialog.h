#pragma once

#include <cstdint>

using anyID = unsigned short;
using uint64 = unsigned long long;

class PokeEngine;

namespace custom_dialog {

void run(uint64 schid, anyID clientID, PokeEngine& engine);

}  // namespace custom_dialog

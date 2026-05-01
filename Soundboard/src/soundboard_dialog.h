#pragma once

#include <string>
#include <vector>

#include "sound_slot.h"

class AudioEngine;

namespace soundboard_dialog {

// Callback invoked when user clicks Play (or right-click Play) on a tile.
// Plugin should both mix the sound into mic stream AND play it locally.
using PlayCallback = void (*)(int slotId);

// Shows the soundboard window. Modal-ish (single instance kept open while
// running). Reads/writes slot config to %APPDATA%/TS3Client/plugins/soundboard.json.
void open(std::vector<SoundSlot>& slots, AudioEngine& engine, PlayCallback playCb);

// Persistence helpers (used by both dialog and hotkey path).
void loadSlots(std::vector<SoundSlot>& slots);
void saveSlots(const std::vector<SoundSlot>& slots);

}  // namespace soundboard_dialog

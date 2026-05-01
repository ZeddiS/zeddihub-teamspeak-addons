#pragma once

#include <string>
#include <vector>

#include "sound_slot.h"

class AudioEngine;

namespace soundboard_dialog {

// Shows the soundboard window. Modal-ish (single instance kept open while
// running). Reads/writes slot config to %APPDATA%/TS3Client/plugins/soundboard.json.
void open(std::vector<SoundSlot>& slots, AudioEngine& engine);

// Persistence helpers (used by both dialog and hotkey path).
void loadSlots(std::vector<SoundSlot>& slots);
void saveSlots(const std::vector<SoundSlot>& slots);

}  // namespace soundboard_dialog

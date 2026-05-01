#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Decoded sound buffer at 48kHz mono int16 PCM (matches TS3 capture format).
struct DecodedSound {
    std::vector<short> samples;
    bool ok = false;
};

// One soundboard slot (a button in the grid).
struct SoundSlot {
    int  id = 0;                 // 1..N stable identifier (matches hotkey number)
    std::string name;            // user-visible label
    std::string filePath;        // absolute path to .wav (or .mp3)
    float volume = 1.0f;         // 0..2
    std::shared_ptr<DecodedSound> decoded;  // lazily decoded on first play
};

// Active playback session (one per concurrent sound).
struct PlaybackSession {
    std::shared_ptr<DecodedSound> buffer;
    std::size_t position = 0;
    float volume = 1.0f;
};

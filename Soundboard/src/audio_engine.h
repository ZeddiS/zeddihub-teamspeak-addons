#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "sound_slot.h"

class AudioEngine {
public:
    AudioEngine();

    // Decode a sound file (WAV / MP3 if vendored) into 48kHz mono int16 PCM.
    // Returns null on failure.
    std::shared_ptr<DecodedSound> loadFile(const std::string& path);

    // Queue a sound for playback (mixed into next captured-voice callbacks).
    void play(std::shared_ptr<DecodedSound> sound, float volume = 1.0f);

    // Stop all currently playing sounds.
    void stopAll();

    // Hook for ts3plugin_onEditCapturedVoiceDataEvent. Mixes any active
    // playback sessions into the buffer in place. Returns true if buffer
    // was modified (and TS3 should set edited=1).
    bool mixIntoCapture(short* samples, int sampleCount, int channels);

    bool hasActive() const;

    // Master volume (0..2). Default 1.0.
    void setMasterVolume(float v) { masterVolume_.store(v); }
    float masterVolume() const { return masterVolume_.load(); }

private:
    std::shared_ptr<DecodedSound> decodeWav(const std::string& path);
    std::shared_ptr<DecodedSound> decodeMp3(const std::string& path);
    std::vector<short> resampleToMono48k(const std::vector<float>& src,
                                          int srcRate, int srcChannels);

    mutable std::mutex mu_;
    std::vector<PlaybackSession> active_;
    std::atomic<float> masterVolume_{1.0f};

    static constexpr int kSampleRate = 48000;
};

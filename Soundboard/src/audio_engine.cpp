#include "audio_engine.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>

namespace {

bool readU32LE(std::ifstream& in, uint32_t& v) {
    unsigned char b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    if (!in) return false;
    v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return true;
}

bool readU16LE(std::ifstream& in, uint16_t& v) {
    unsigned char b[2];
    in.read(reinterpret_cast<char*>(b), 2);
    if (!in) return false;
    v = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    return true;
}

bool tagEquals(const char tag[4], const char* expected) {
    return std::memcmp(tag, expected, 4) == 0;
}

bool endsWithCI(const std::string& s, const char* suffix) {
    std::size_t n = std::strlen(suffix);
    if (s.size() < n) return false;
    for (std::size_t i = 0; i < n; ++i) {
        char a = (char)std::tolower((unsigned char)s[s.size() - n + i]);
        char b = (char)std::tolower((unsigned char)suffix[i]);
        if (a != b) return false;
    }
    return true;
}

}  // namespace

AudioEngine::AudioEngine() = default;

std::shared_ptr<DecodedSound> AudioEngine::loadFile(const std::string& path) {
    if (endsWithCI(path, ".mp3")) return decodeMp3(path);
    return decodeWav(path);
}

std::shared_ptr<DecodedSound> AudioEngine::decodeWav(const std::string& path) {
    auto out = std::make_shared<DecodedSound>();
    std::ifstream in(path, std::ios::binary);
    if (!in) return out;

    char riff[4]; in.read(riff, 4);
    if (!in || !tagEquals(riff, "RIFF")) return out;
    uint32_t size; readU32LE(in, size);
    char wave[4]; in.read(wave, 4);
    if (!in || !tagEquals(wave, "WAVE")) return out;

    // Find fmt and data chunks
    uint16_t audioFormat = 0, numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    std::vector<unsigned char> dataBytes;

    while (in.good()) {
        char tag[4]; in.read(tag, 4);
        if (!in) break;
        uint32_t chunkSize; if (!readU32LE(in, chunkSize)) break;

        if (tagEquals(tag, "fmt ")) {
            uint16_t blockAlign; uint32_t byteRate;
            readU16LE(in, audioFormat);
            readU16LE(in, numChannels);
            readU32LE(in, sampleRate);
            readU32LE(in, byteRate);
            readU16LE(in, blockAlign);
            readU16LE(in, bitsPerSample);
            // skip rest of chunk if any
            std::size_t consumed = 16;
            if (chunkSize > consumed) in.seekg(chunkSize - consumed, std::ios::cur);
        } else if (tagEquals(tag, "data")) {
            dataBytes.resize(chunkSize);
            in.read(reinterpret_cast<char*>(dataBytes.data()), chunkSize);
            break;
        } else {
            // skip unknown chunk
            in.seekg(chunkSize, std::ios::cur);
        }
    }

    // Supported formats:
    // - PCM (1): 8-bit unsigned, 16-bit signed, 24-bit signed, 32-bit signed
    // - IEEE float (3): 32-bit float
    bool isPcm = (audioFormat == 1);
    bool isFloat = (audioFormat == 3);
    bool ok = (isPcm || isFloat) &&
              numChannels >= 1 && numChannels <= 8 &&
              sampleRate > 0 &&
              !dataBytes.empty();
    if (isPcm && bitsPerSample != 8 && bitsPerSample != 16 &&
        bitsPerSample != 24 && bitsPerSample != 32) ok = false;
    if (isFloat && bitsPerSample != 32) ok = false;
    if (!ok) return out;

    // Decode samples to float [-1, 1] interleaved
    std::vector<float> floatData;
    if (isPcm && bitsPerSample == 16) {
        const int n = (int)(dataBytes.size() / 2);
        floatData.resize(n);
        for (int i = 0; i < n; ++i) {
            int16_t s = (int16_t)((uint16_t)dataBytes[i * 2] |
                                  ((uint16_t)dataBytes[i * 2 + 1] << 8));
            floatData[i] = (float)s / 32768.0f;
        }
    } else if (isPcm && bitsPerSample == 8) {
        floatData.resize(dataBytes.size());
        for (std::size_t i = 0; i < dataBytes.size(); ++i) {
            floatData[i] = ((float)dataBytes[i] - 128.0f) / 128.0f;
        }
    } else if (isPcm && bitsPerSample == 24) {
        const int n = (int)(dataBytes.size() / 3);
        floatData.resize(n);
        for (int i = 0; i < n; ++i) {
            int32_t s = (int32_t)dataBytes[i * 3] |
                        ((int32_t)dataBytes[i * 3 + 1] << 8) |
                        ((int32_t)dataBytes[i * 3 + 2] << 16);
            // sign-extend 24-bit
            if (s & 0x800000) s |= 0xFF000000;
            floatData[i] = (float)s / 8388608.0f;
        }
    } else if (isPcm && bitsPerSample == 32) {
        const int n = (int)(dataBytes.size() / 4);
        floatData.resize(n);
        for (int i = 0; i < n; ++i) {
            int32_t s = (int32_t)((uint32_t)dataBytes[i * 4] |
                                  ((uint32_t)dataBytes[i * 4 + 1] << 8) |
                                  ((uint32_t)dataBytes[i * 4 + 2] << 16) |
                                  ((uint32_t)dataBytes[i * 4 + 3] << 24));
            floatData[i] = (float)s / 2147483648.0f;
        }
    } else if (isFloat && bitsPerSample == 32) {
        const int n = (int)(dataBytes.size() / 4);
        floatData.resize(n);
        std::memcpy(floatData.data(), dataBytes.data(), n * 4);
    }

    out->samples = resampleToMono48k(floatData, (int)sampleRate, (int)numChannels);
    out->ok = !out->samples.empty();
    return out;
}

std::shared_ptr<DecodedSound> AudioEngine::decodeMp3(const std::string& path) {
    // MP3 not supported in v1. User should convert to WAV first.
    (void)path;
    return std::make_shared<DecodedSound>();
}

std::vector<short> AudioEngine::resampleToMono48k(const std::vector<float>& src,
                                                   int srcRate, int srcChannels) {
    if (src.empty() || srcRate <= 0 || srcChannels <= 0) return {};

    // Step 1: downmix to mono
    const int frames = (int)(src.size() / srcChannels);
    std::vector<float> mono(frames);
    if (srcChannels == 1) {
        std::memcpy(mono.data(), src.data(), frames * sizeof(float));
    } else {
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < srcChannels; ++c) sum += src[i * srcChannels + c];
            mono[i] = sum / (float)srcChannels;
        }
    }

    // Step 2: linear-interpolation resample to 48000 Hz
    if (srcRate == kSampleRate) {
        std::vector<short> out(frames);
        for (int i = 0; i < frames; ++i) {
            float v = mono[i] * 32767.0f;
            if (v > 32767.0f) v = 32767.0f;
            else if (v < -32768.0f) v = -32768.0f;
            out[i] = (short)v;
        }
        return out;
    }

    const double ratio = (double)kSampleRate / (double)srcRate;
    const int outFrames = (int)((double)frames * ratio);
    std::vector<short> out((std::size_t)outFrames);

    for (int i = 0; i < outFrames; ++i) {
        double srcIdx = (double)i / ratio;
        int idx0 = (int)srcIdx;
        if (idx0 >= frames - 1) idx0 = frames - 2;
        if (idx0 < 0) idx0 = 0;
        int idx1 = idx0 + 1;
        if (idx1 >= frames) idx1 = frames - 1;
        double frac = srcIdx - (double)idx0;
        float v = (float)((1.0 - frac) * mono[idx0] + frac * mono[idx1]);
        v *= 32767.0f;
        if (v > 32767.0f) v = 32767.0f;
        else if (v < -32768.0f) v = -32768.0f;
        out[i] = (short)v;
    }
    return out;
}

void AudioEngine::play(std::shared_ptr<DecodedSound> sound, float volume) {
    if (!sound || !sound->ok || sound->samples.empty()) return;
    PlaybackSession s;
    s.buffer = std::move(sound);
    s.position = 0;
    s.volume = volume;
    std::lock_guard<std::mutex> lk(mu_);
    active_.push_back(std::move(s));
}

void AudioEngine::stopAll() {
    std::lock_guard<std::mutex> lk(mu_);
    active_.clear();
}

bool AudioEngine::hasActive() const {
    std::lock_guard<std::mutex> lk(mu_);
    return !active_.empty();
}

bool AudioEngine::mixIntoCapture(short* samples, int sampleCount, int channels) {
    if (!samples || sampleCount <= 0) return false;
    std::lock_guard<std::mutex> lk(mu_);
    if (active_.empty()) return false;

    const float master = masterVolume_.load();
    constexpr float kMixBoost = 4.0f;
    // PRNG state for VAD-trigger noise burst.
    static unsigned int rng = 0x12345u;

    for (int frame = 0; frame < sampleCount; ++frame) {
        int monoMix = 0;
        for (auto& s : active_) {
            if (s.position >= s.buffer->samples.size()) continue;
            short sb = s.buffer->samples[s.position];
            monoMix += (int)((float)sb * s.volume * master * kMixBoost);
            s.position++;

            // VAD-trigger burst at sound start: ~50ms of moderate
            // broadband noise mixed in. Triggers TS3's VAD reliably
            // even when user is silent. Hangover keeps VAD active for
            // the rest of the sound.
            if (s.triggerSamplesLeft > 0) {
                rng = rng * 1664525u + 1013904223u;
                int noise = (int)((rng >> 16) & 0x7fff) - 16383;  // -16k..+16k
                // Fade in/out of trigger to avoid click
                float fade = 1.0f;
                if (s.triggerSamplesLeft < 480) {
                    fade = s.triggerSamplesLeft / 480.0f;  // last 10ms fade out
                }
                monoMix += (int)((float)noise * fade);
                s.triggerSamplesLeft--;
            }
        }

        for (int ch = 0; ch < channels; ++ch) {
            const int idx = frame * channels + ch;
            int mixed = (int)samples[idx] + monoMix;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32768) mixed = -32768;
            samples[idx] = (short)mixed;
        }
    }

    // Drop finished sessions
    active_.erase(
        std::remove_if(active_.begin(), active_.end(),
                       [](const PlaybackSession& s) {
                           return s.position >= s.buffer->samples.size();
                       }),
        active_.end());

    return true;
}

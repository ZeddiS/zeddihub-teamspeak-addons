// Windows Media Foundation-based audio converter.
// Decodes any Windows-supported audio format (WAV, MP3, AAC, M4A, FLAC, WMA, ...)
// and writes a canonical 48kHz, 16-bit, mono PCM WAV file.
//
// Used at import time so SoundBoard always plays from a known-good WAV.

#pragma once

#include <string>

namespace audio_converter {

// One-time process init / shutdown.
// init() must be called before any conversion; shutdown() at plugin unload.
bool init();
void shutdown();

// Decode `srcPath` (any format Windows supports) and write canonical
// 48kHz / 16-bit signed / mono WAV to `dstPath`. Returns true on success.
// On failure, errMsg holds a human-readable reason.
bool convertToCanonicalWav(const std::string& srcPath,
                            const std::string& dstPath,
                            std::string& errMsg);

}  // namespace audio_converter

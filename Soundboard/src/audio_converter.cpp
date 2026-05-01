#include "audio_converter.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Ole32.lib")

namespace {

constexpr int kSampleRate = 48000;
constexpr int kBitsPerSample = 16;
constexpr int kChannels = 1;

bool s_initialized = false;

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

// Write a minimal PCM WAV file (RIFF/WAVE/fmt/data) to disk.
bool writeWav(const std::string& dstPath, const std::vector<int16_t>& pcm) {
    std::ofstream out(dstPath, std::ios::binary);
    if (!out) return false;

    const uint32_t dataBytes = (uint32_t)(pcm.size() * sizeof(int16_t));
    const uint32_t fmtBytes = 16;
    const uint32_t riffBytes = 4 + (8 + fmtBytes) + (8 + dataBytes);

    auto w32 = [&](uint32_t v) {
        unsigned char b[4] = { (unsigned char)(v & 0xFF),
                                (unsigned char)((v >> 8) & 0xFF),
                                (unsigned char)((v >> 16) & 0xFF),
                                (unsigned char)((v >> 24) & 0xFF) };
        out.write(reinterpret_cast<char*>(b), 4);
    };
    auto w16 = [&](uint16_t v) {
        unsigned char b[2] = { (unsigned char)(v & 0xFF),
                                (unsigned char)((v >> 8) & 0xFF) };
        out.write(reinterpret_cast<char*>(b), 2);
    };

    out.write("RIFF", 4); w32(riffBytes); out.write("WAVE", 4);
    out.write("fmt ", 4); w32(fmtBytes);
    w16(1);                                  // PCM
    w16((uint16_t)kChannels);
    w32((uint32_t)kSampleRate);
    w32((uint32_t)(kSampleRate * kChannels * kBitsPerSample / 8));  // byte rate
    w16((uint16_t)(kChannels * kBitsPerSample / 8));                // block align
    w16((uint16_t)kBitsPerSample);
    out.write("data", 4); w32(dataBytes);
    out.write(reinterpret_cast<const char*>(pcm.data()), dataBytes);
    return out.good();
}

}  // namespace

namespace audio_converter {

bool init() {
    if (s_initialized) return true;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    s_initialized = true;
    return true;
}

void shutdown() {
    if (!s_initialized) return;
    MFShutdown();
    CoUninitialize();
    s_initialized = false;
}

bool convertToCanonicalWav(const std::string& srcPath,
                            const std::string& dstPath,
                            std::string& errMsg) {
    errMsg.clear();
    if (!s_initialized) {
        errMsg = "audio_converter not initialized";
        return false;
    }

    std::wstring wsrc = utf8ToWide(srcPath);

    IMFSourceReader* reader = nullptr;
    HRESULT hr = MFCreateSourceReaderFromURL(wsrc.c_str(), nullptr, &reader);
    if (FAILED(hr)) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "MFCreateSourceReaderFromURL failed (hr=0x%08lx)", (long)hr);
        errMsg = buf;
        return false;
    }

    // Disable all streams except the first audio stream
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

    // Request canonical PCM 48kHz mono 16-bit
    IMFMediaType* outType = nullptr;
    hr = MFCreateMediaType(&outType);
    if (SUCCEEDED(hr)) hr = outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) hr = outType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, kBitsPerSample);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, kSampleRate);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, kChannels);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, kChannels * kBitsPerSample / 8);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
                                                kSampleRate * kChannels * kBitsPerSample / 8);
    if (SUCCEEDED(hr)) hr = outType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    if (FAILED(hr)) {
        if (outType) outType->Release();
        reader->Release();
        errMsg = "Failed to create output media type";
        return false;
    }

    hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, outType);
    outType->Release();
    if (FAILED(hr)) {
        reader->Release();
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "Decoder cannot output 48kHz/16-bit/mono PCM (hr=0x%08lx). "
            "Format may not be supported on this Windows.", (long)hr);
        errMsg = buf;
        return false;
    }

    // Read decoded samples
    std::vector<int16_t> pcm;
    pcm.reserve(48000 * 30);  // hint: 30s of audio

    while (true) {
        IMFSample* sample = nullptr;
        DWORD streamFlags = 0;
        hr = reader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0, nullptr, &streamFlags, nullptr, &sample);
        if (FAILED(hr)) {
            if (sample) sample->Release();
            reader->Release();
            errMsg = "ReadSample failed";
            return false;
        }
        if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
            if (sample) sample->Release();
            break;
        }
        if (!sample) continue;

        IMFMediaBuffer* mb = nullptr;
        if (SUCCEEDED(sample->ConvertToContiguousBuffer(&mb)) && mb) {
            BYTE* data = nullptr;
            DWORD len = 0;
            if (SUCCEEDED(mb->Lock(&data, nullptr, &len))) {
                const int16_t* src = reinterpret_cast<const int16_t*>(data);
                const std::size_t n = len / sizeof(int16_t);
                pcm.insert(pcm.end(), src, src + n);
                mb->Unlock();
            }
            mb->Release();
        }
        sample->Release();
    }

    reader->Release();

    if (pcm.empty()) {
        errMsg = "Decoder produced no samples (empty / corrupt audio)";
        return false;
    }

    if (!writeWav(dstPath, pcm)) {
        errMsg = "Failed to write canonical WAV to disk";
        return false;
    }
    return true;
}

}  // namespace audio_converter

// smbPitchShift — phase-vocoder pitch shifter.
//
// Based on Stephan M. Bernsee's public-domain reference implementation
// (http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/).
// Wrapped here in a class so multiple instances can be used.

#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>

namespace pitchshift {

class PitchShifter {
public:
    static constexpr int MAX_FRAME_LENGTH = 8192;

    PitchShifter() { reset(); }

    void reset() {
        std::memset(gInFIFO, 0, sizeof(gInFIFO));
        std::memset(gOutFIFO, 0, sizeof(gOutFIFO));
        std::memset(gFFTworksp, 0, sizeof(gFFTworksp));
        std::memset(gLastPhase, 0, sizeof(gLastPhase));
        std::memset(gSumPhase, 0, sizeof(gSumPhase));
        std::memset(gOutputAccum, 0, sizeof(gOutputAccum));
        std::memset(gAnaFreq, 0, sizeof(gAnaFreq));
        std::memset(gAnaMagn, 0, sizeof(gAnaMagn));
        std::memset(gSynFreq, 0, sizeof(gSynFreq));
        std::memset(gSynMagn, 0, sizeof(gSynMagn));
        gRover = 0;
    }

    // pitchShift: ratio (1.0 = no change, 2.0 = +1 octave, 0.5 = -1 octave)
    // sampleRate: e.g. 48000
    // Processes numSamples in place.
    void process(float pitchShift,
                 long numSamples,
                 float sampleRate,
                 float* indata,
                 float* outdata) {
        constexpr long fftFrameSize = 2048;
        constexpr long osamp = 4;

        const long fftFrameSize2 = fftFrameSize / 2;
        const long stepSize = fftFrameSize / osamp;
        const float freqPerBin = sampleRate / (float)fftFrameSize;
        const float expct = 2.0f * (float)M_PI * (float)stepSize / (float)fftFrameSize;
        const long inFifoLatency = fftFrameSize - stepSize;

        if (gRover == 0) gRover = inFifoLatency;

        for (long i = 0; i < numSamples; ++i) {
            gInFIFO[gRover] = indata[i];
            outdata[i] = gOutFIFO[gRover - inFifoLatency];
            ++gRover;

            if (gRover >= fftFrameSize) {
                gRover = inFifoLatency;

                // Window + interleave for FFT
                for (long k = 0; k < fftFrameSize; ++k) {
                    float window = -0.5f * std::cos(2.0f * (float)M_PI * (float)k / (float)fftFrameSize) + 0.5f;
                    gFFTworksp[2 * k]     = gInFIFO[k] * window;
                    gFFTworksp[2 * k + 1] = 0.0f;
                }

                smbFft(gFFTworksp, fftFrameSize, -1);

                // Analysis
                for (long k = 0; k <= fftFrameSize2; ++k) {
                    float real = gFFTworksp[2 * k];
                    float imag = gFFTworksp[2 * k + 1];
                    float magn = 2.0f * std::sqrt(real * real + imag * imag);
                    float phase = std::atan2(imag, real);
                    float tmp = phase - gLastPhase[k];
                    gLastPhase[k] = phase;
                    tmp -= (float)k * expct;
                    long qpd = (long)(tmp / (float)M_PI);
                    if (qpd >= 0) qpd += qpd & 1;
                    else qpd -= qpd & 1;
                    tmp -= (float)M_PI * (float)qpd;
                    tmp = (float)osamp * tmp / (2.0f * (float)M_PI);
                    tmp = (float)k * freqPerBin + tmp * freqPerBin;
                    gAnaMagn[k] = magn;
                    gAnaFreq[k] = tmp;
                }

                // Pitch shift in freq domain
                std::memset(gSynMagn, 0, fftFrameSize * sizeof(float));
                std::memset(gSynFreq, 0, fftFrameSize * sizeof(float));
                for (long k = 0; k <= fftFrameSize2; ++k) {
                    long index = (long)(k * pitchShift);
                    if (index <= fftFrameSize2) {
                        gSynMagn[index] += gAnaMagn[k];
                        gSynFreq[index] = gAnaFreq[k] * pitchShift;
                    }
                }

                // Synthesis
                for (long k = 0; k <= fftFrameSize2; ++k) {
                    float magn = gSynMagn[k];
                    float tmp = gSynFreq[k];
                    tmp -= (float)k * freqPerBin;
                    tmp /= freqPerBin;
                    tmp = 2.0f * (float)M_PI * tmp / (float)osamp;
                    tmp += (float)k * expct;
                    gSumPhase[k] += tmp;
                    float phase = gSumPhase[k];
                    gFFTworksp[2 * k]     = magn * std::cos(phase);
                    gFFTworksp[2 * k + 1] = magn * std::sin(phase);
                }
                for (long k = fftFrameSize + 2; k < 2 * fftFrameSize; ++k) {
                    gFFTworksp[k] = 0.0f;
                }

                smbFft(gFFTworksp, fftFrameSize, 1);

                for (long k = 0; k < fftFrameSize; ++k) {
                    float window = -0.5f * std::cos(2.0f * (float)M_PI * (float)k / (float)fftFrameSize) + 0.5f;
                    gOutputAccum[k] += 2.0f * window * gFFTworksp[2 * k] / ((float)fftFrameSize2 * (float)osamp);
                }

                for (long k = 0; k < stepSize; ++k) {
                    gOutFIFO[k] = gOutputAccum[k];
                }

                std::memmove(gOutputAccum, gOutputAccum + stepSize,
                             fftFrameSize * sizeof(float));

                for (long k = 0; k < inFifoLatency; ++k) {
                    gInFIFO[k] = gInFIFO[k + stepSize];
                }
            }
        }
    }

private:
    // Standard radix-2 FFT (in-place).
    static void smbFft(float* fftBuffer, long fftFrameSize, long sign) {
        for (long i = 2; i < 2 * fftFrameSize - 2; i += 2) {
            long j = 0;
            for (long bitm = 2; bitm < 2 * fftFrameSize; bitm <<= 1) {
                if (i & bitm) j++;
                j <<= 1;
            }
            if (i < j) {
                std::swap(fftBuffer[i], fftBuffer[j]);
                std::swap(fftBuffer[i + 1], fftBuffer[j + 1]);
            }
        }

        long k = (long)(std::log((float)fftFrameSize) / std::log(2.0f) + 0.5f);
        long le = 2;
        for (long s = 1; s <= k; ++s) {
            le <<= 1;
            long le2 = le >> 1;
            float ur = 1.0f;
            float ui = 0.0f;
            float arg = (float)M_PI / (float)(le2 >> 1);
            float wr = std::cos(arg);
            float wi = (float)sign * std::sin(arg);
            for (long j = 0; j < le2; j += 2) {
                for (long i = j; i < 2 * fftFrameSize; i += le) {
                    long ip = i + le2;
                    float tr = fftBuffer[ip] * ur - fftBuffer[ip + 1] * ui;
                    float ti = fftBuffer[ip] * ui + fftBuffer[ip + 1] * ur;
                    fftBuffer[ip]     = fftBuffer[i] - tr;
                    fftBuffer[ip + 1] = fftBuffer[i + 1] - ti;
                    fftBuffer[i]     += tr;
                    fftBuffer[i + 1] += ti;
                }
                float tr = ur * wr - ui * wi;
                ui = ur * wi + ui * wr;
                ur = tr;
            }
        }
    }

    float gInFIFO[MAX_FRAME_LENGTH];
    float gOutFIFO[MAX_FRAME_LENGTH];
    float gFFTworksp[2 * MAX_FRAME_LENGTH];
    float gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
    float gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
    float gOutputAccum[2 * MAX_FRAME_LENGTH];
    float gAnaFreq[MAX_FRAME_LENGTH];
    float gAnaMagn[MAX_FRAME_LENGTH];
    float gSynFreq[MAX_FRAME_LENGTH];
    float gSynMagn[MAX_FRAME_LENGTH];
    long  gRover = 0;
};

}  // namespace pitchshift

#include "audio_spectrum.h"

#include <cmath>
#include <cstring>

#include "config.h"

namespace {
constexpr float PI_F = 3.14159265358979323846f;

void swapf(float &a, float &b) {
    float t = a;
    a = b;
    b = t;
}
}  // namespace

bool AudioSpectrum::begin() {
    // M5Stack Unit Mini PDM: Data -> G1, Clock -> G2, 5V/GND from Grove.
    i2s_.setTimeout(50);
    i2s_.setPinsPdmRx(cfg::PIN_PDM_CLK, cfg::PIN_PDM_DATA);
    if (!i2s_.begin(I2S_MODE_PDM_RX, static_cast<uint32_t>(SAMPLE_RATE), I2S_DATA_BIT_WIDTH_16BIT,
                    I2S_SLOT_MODE_MONO)) {
        Serial.println("[AUDIO] PDM mic init failed");
        return false;
    }

    for (int i = 0; i < FFT_SIZE; ++i) {
        window_[i] = 0.5f * (1.0f - cosf(2.0f * PI_F * i / (FFT_SIZE - 1)));
    }
    ready_ = true;
    Serial.println("[AUDIO] PDM mic ready");
    return true;
}

void AudioSpectrum::loop() {
    if (!ready_) return;

    int16_t samples[FFT_SIZE];
    size_t got = i2s_.readBytes(reinterpret_cast<char *>(samples), sizeof(samples));
    if (got < sizeof(samples)) return;
    analyze(samples, FFT_SIZE);
}

void AudioSpectrum::fft(float *re, float *im, int n) {
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            swapf(re[i], re[j]);
            swapf(im[i], im[j]);
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI_F / static_cast<float>(len);
        float wlenRe = cosf(ang);
        float wlenIm = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float wRe = 1.0f;
            float wIm = 0.0f;
            for (int j = 0; j < len / 2; ++j) {
                int u = i + j;
                int v = i + j + len / 2;
                float vr = re[v] * wRe - im[v] * wIm;
                float vi = re[v] * wIm + im[v] * wRe;
                re[v] = re[u] - vr;
                im[v] = im[u] - vi;
                re[u] += vr;
                im[u] += vi;
                float nextRe = wRe * wlenRe - wIm * wlenIm;
                wIm = wRe * wlenIm + wIm * wlenRe;
                wRe = nextRe;
            }
        }
    }
}

void AudioSpectrum::analyze(const int16_t *samples, size_t count) {
    if (count < FFT_SIZE) return;

    float mean = 0.0f;
    float sumSq = 0.0f;
    for (int i = 0; i < FFT_SIZE; ++i) {
        mean += samples[i];
        sumSq += static_cast<float>(samples[i]) * samples[i];
    }
    mean /= FFT_SIZE;
    float rms = sqrtf(sumSq / FFT_SIZE) / 32768.0f;
    if (rms > 0.003f) signal_ = true;

    for (int i = 0; i < FFT_SIZE; ++i) {
        re_[i] = (static_cast<float>(samples[i]) - mean) / 32768.0f * window_[i];
        im_[i] = 0.0f;
    }
    fft(re_, im_, FFT_SIZE);

    // 12 logarithmic bands from 60 Hz to 7 kHz.
    const float ratio = powf(7000.0f / 60.0f, 1.0f / BAND_COUNT);
    for (int band = 0; band < BAND_COUNT; ++band) {
        float f0 = 60.0f * powf(ratio, static_cast<float>(band));
        float f1 = 60.0f * powf(ratio, static_cast<float>(band + 1));
        int k0 = static_cast<int>(f0 * FFT_SIZE / SAMPLE_RATE);
        int k1 = static_cast<int>(f1 * FFT_SIZE / SAMPLE_RATE);
        if (k0 < 1) k0 = 1;
        if (k1 <= k0) k1 = k0 + 1;
        if (k1 > FFT_SIZE / 2 - 1) k1 = FFT_SIZE / 2 - 1;

        float peak = 0.0f;
        for (int k = k0; k <= k1; ++k) {
            float mag = sqrtf(re_[k] * re_[k] + im_[k] * im_[k]);
            if (mag > peak) peak = mag;
        }

        // Normalize FFT magnitude and map roughly -65..0 dB to 0..8.
        float mag = peak / (FFT_SIZE / 2.0f);
        float db = 20.0f * log10f(mag + 1e-7f);
        float norm = (db + 65.0f) / 65.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        uint8_t target = static_cast<uint8_t>(norm * 8.0f + 0.5f);
        if (target > levels_[band]) {
            levels_[band] = target;
        } else if (levels_[band] > 0) {
            --levels_[band];
        }
    }
}

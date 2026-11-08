#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>

class AudioSpectrum {
public:
    bool begin();
    void loop();
    bool ready() const { return ready_; }
    bool signal() const { return signal_; }
    const uint8_t *levels() const { return levels_; }

private:
    static constexpr int FFT_SIZE = 256;
    static constexpr int BAND_COUNT = 12;
    static constexpr float SAMPLE_RATE = 16000.0f;

    void analyze(const int16_t *samples, size_t count);
    static void fft(float *re, float *im, int n);

    I2SClass i2s_;
    bool ready_ = false;
    bool signal_ = false;
    float re_[FFT_SIZE] = {};
    float im_[FFT_SIZE] = {};
    float window_[FFT_SIZE] = {};
    uint8_t levels_[BAND_COUNT] = {};
};

#pragma once

#include <Arduino.h>

// Receives 12 band levels from the PC companion app over USB CDC serial.
// Frame: A5 5A | 12 levels | checksum
// checksum = 0x5A ^ XOR(levels)
class UsbSpectrum {
public:
    void loop();
    bool ready() const;
    const uint8_t *levels() const { return levels_; }

private:
    uint8_t levels_[12] = {};
    uint8_t frame_[12] = {};
    uint8_t count_ = 0;
    uint8_t checksum_ = 0;
    uint8_t state_ = 0;
    uint32_t lastFrameMs_ = 0;
};

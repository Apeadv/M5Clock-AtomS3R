#include "usb_spectrum.h"

void UsbSpectrum::loop() {
    while (Serial.available() > 0) {
        uint8_t b = static_cast<uint8_t>(Serial.read());

        switch (state_) {
            case 0:
                if (b == 0xA5) state_ = 1;
                break;
            case 1:
                if (b == 0x5A) {
                    state_ = 2;
                    count_ = 0;
                } else if (b != 0xA5) {
                    state_ = 0;
                }
                break;
            case 2:
                frame_[count_++] = b;
                if (count_ >= 12) {
                    state_ = 3;
                    checksum_ = 0x5A;
                    for (int i = 0; i < 12; ++i) checksum_ ^= frame_[i];
                }
                break;
            case 3:
                if (b == checksum_) {
                    for (int i = 0; i < 12; ++i) {
                        if (frame_[i] > 8) frame_[i] = 8;
                        levels_[i] = frame_[i];
                    }
                    lastFrameMs_ = millis();
                }
                state_ = 0;
                break;
        }
    }
}

bool UsbSpectrum::ready() const {
    return lastFrameMs_ != 0 && (millis() - lastFrameMs_ < 1000);
}

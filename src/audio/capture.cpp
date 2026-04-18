#include "capture.hpp"
#include "feedback.hpp"
#include "../utils/logger.hpp"
#include <vector>

// static const std::string DEVICE_HW = "plughw:CARD=Audio,DEV=0";
static const std::string DEVICE_DEFAULT = "default";

namespace audio_capture {
    FilePtr open_stream(StreamMode mode) {
        std::string capture_cmd;
        std::string base_record = 
            "arecord -D " + DEVICE_DEFAULT +
            " -r 16000 -f S16_LE -c 1 -t raw -q"
            " --buffer-size=8192" // delay selama 8192/16000 = 512 ms
            " 2>/dev/null";

        if (mode == StreamMode::IDLE) {
            // Murni raw tanpa SoX untuk Neural Network
            capture_cmd = base_record;
        } else {
            // Pipeline dengan SoX untuk Vosk
            capture_cmd = 
                base_record + 
                " | sox -t raw -r 16000 -e signed -b 16 -c 1 - "
                "        -t raw -r 16000 -e signed -b 16 -c 1 - "
                "        highpass 100 lowpass 4000 " // Filter frekuensi manusia
                "        compand 0.05,0.2 6:-70,-70,-60,-20,0,0 "
                "        2>/dev/null";
        }

        FILE* pipe = popen(capture_cmd.c_str(), "r");
        return FilePtr(pipe);
    }

    bool smoke_test(FILE* stream) {
        char test_buf[1024];
        if (std::fread(test_buf, 1, sizeof(test_buf), stream) == 0) {
            logger::err("MAIN", "Stream kosong — mic mungkin sibuk atau tidak terdeteksi.");
            audio::feedback(audio::Event::MicError);
            return false;
        }
        return true;
    }
}
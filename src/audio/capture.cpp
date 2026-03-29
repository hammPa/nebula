#include "capture.hpp"
#include "feedback.hpp"
#include "../utils/logger.hpp"
#include <vector>

namespace audio_capture {
    FilePtr open_stream(const std::string& noise_prof) {
        const std::string capture_cmd =
            "arecord -D plughw:CARD=Audio,DEV=0 -r 16000 -f S16_LE -c 1 -t raw -q 2>/dev/null"
            " | sox -t raw -r 16000 -e signed -b 16 -c 1 - "
            "        -t raw -r 16000 -e signed -b 16 -c 1 - "
            "        noisered " + noise_prof + " 0.21 2>/dev/null";

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
#include "capture.hpp"
#include "feedback.hpp"
#include "../utils/logger.hpp"

static constexpr int SAMPLE_RATE = 16000;
static constexpr int BIT_DEPTH = 16;
static constexpr int CHANNELS = 1;
static constexpr int BUFFER_SIZE = 4096;

static std::string BASE_RECORD = 
    "arecord -D default"
    " -r " + std::to_string(SAMPLE_RATE) +
    " -f S16_LE"
    " -c " + std::to_string(CHANNELS) +
    " -t raw -q"
    " --buffer-size=" + std::to_string(BUFFER_SIZE) +
    " 2>/dev/null";

static std::string SOX_FILTER = 
    BASE_RECORD +
    " | sox -t raw -r " + std::to_string(SAMPLE_RATE) +
    " -e signed -b " + std::to_string(BIT_DEPTH) +
    " -c " + std::to_string(CHANNELS) + " - "
    "       -t raw -r " + std::to_string(SAMPLE_RATE) +
    " -e signed -b " + std::to_string(BIT_DEPTH) +
    " -c " + std::to_string(CHANNELS) + " - "
    "       highpass 100 lowpass 4000"
    "       compand 0.05,0.3 6:-60,-50,0,0"
    " 2>/dev/null";



namespace audio_capture {
    FilePtr open_stream(StreamMode mode) {
        const std::string capture_cmd = (mode == StreamMode::IDLE)
            ? BASE_RECORD // Murni raw tanpa SoX untuk Neural Network
            : SOX_FILTER; // Pipeline dengan SoX untuk Vosk

        FILE* pipe = popen(capture_cmd.c_str(), "r");
        if (!pipe) {
            logger::err("MAIN", "popen gagal — tidak bisa membuka stream audio.");
            audio::feedback(audio::Event::MicError);
            return FilePtr(nullptr);
        }

        return FilePtr(pipe);
    }

    bool smoke_test(FILE* stream, std::vector<char>& out_buffer) {
        out_buffer.resize(1024);
        std::size_t n = std::fread(out_buffer.data(), 1, sizeof(out_buffer.data()), stream);
        if (n == 0) {
            logger::err("MAIN", "Stream kosong — mic mungkin sibuk atau tidak terdeteksi.");
            audio::feedback(audio::Event::MicError);
            out_buffer.clear();
            return false;
        }

        out_buffer.resize(n);
        return true;
    }
}
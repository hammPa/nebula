#include "capture.hpp"
#include "feedback.hpp"
#include "../utils/logger.hpp"

static constexpr int SAMPLE_RATE = 16000;
static constexpr int BIT_DEPTH = 16;
static constexpr int CHANNELS = 1;
static constexpr int BUFFER_SIZE = 4096;

enum class AudioBackend { PipeWire, PulseAudio, ALSA };

static AudioBackend detect_backend() {
    // pw-record tersedia  →  PipeWire (modern, full sharing)
    if (std::system("which pw-record > /dev/null 2>&1") == 0)
        return AudioBackend::PipeWire;
 
    // pacat tersedia  →  PulseAudio (sharing via daemon)
    if (std::system("which pacat > /dev/null 2>&1") == 0)
        return AudioBackend::PulseAudio;
 
    // Fallback: ALSA langsung (exclusive access, tanpa sharing)
    return AudioBackend::ALSA;
}

static const AudioBackend BACKEND = detect_backend();

static void log_backend() {
    switch (BACKEND) {
        case AudioBackend::PipeWire:
            logger::info("AUDIO", "Backend: PipeWire (pw-record) — sharing dengan app lain OK");
            break;
        case AudioBackend::PulseAudio:
            logger::info("AUDIO", "Backend: PulseAudio (pacat) — sharing dengan app lain OK");
            break;
        case AudioBackend::ALSA:
            logger::warn("AUDIO", "Backend: ALSA langsung — mic mungkin exclusive, app lain bisa conflict");
            break;
    }
}


static std::string build_raw_cmd() {
    const std::string rate = std::to_string(SAMPLE_RATE);
    const std::string ch   = std::to_string(CHANNELS);
    const std::string buf  = std::to_string(BUFFER_SIZE);
 
    switch (BACKEND) {
        case AudioBackend::PipeWire:
            // pw-record output langsung PCM s16le ke stdout
            return "pw-record"
                   " --rate="     + rate +
                   " --channels=" + ch   +
                   " --format=s16"
                   " - 2>/dev/null";
 
        case AudioBackend::PulseAudio:
            // pacat: baca dari source default, raw s16le
            return "pacat"
                   " --record"
                   " --rate="    + rate +
                   " --channels=" + ch  +
                   " --format=s16le"
                   " --latency-msec=100"
                   " 2>/dev/null";
 
        case AudioBackend::ALSA:
        default:
            return "arecord -D default"
                   " -r " + rate +
                   " -f S16_LE"
                   " -c " + ch +
                   " -t raw -q"
                   " --buffer-size=" + buf +
                   " 2>/dev/null";
    }
}

static std::string build_sox_cmd(const std::string& raw_cmd) {
    const std::string rate  = std::to_string(SAMPLE_RATE);
    const std::string depth = std::to_string(BIT_DEPTH);
    const std::string ch    = std::to_string(CHANNELS);
 
    return raw_cmd +
           " | sox -t raw -r " + rate +
           " -e signed -b "    + depth +
           " -c "              + ch + " - "
           " -t raw -r "       + rate +
           " -e signed -b "    + depth +
           " -c "              + ch + " - "
           " highpass 100 lowpass 4000"
           " compand 0.05,0.3 6:-60,-50,0,0"
           " 2>/dev/null";
}


static const std::string BASE_RECORD = build_raw_cmd();
static const std::string SOX_FILTER  = build_sox_cmd(BASE_RECORD);


namespace audio_capture {
    FilePtr open_stream(StreamMode mode) {
        // Cetak backend sekali pada pemanggilan pertama
        static bool logged = false;
        if (!logged) { log_backend(); logged = true; }
    
        const std::string& capture_cmd =
            (mode == StreamMode::IDLE)
                ? BASE_RECORD   // Murni raw — untuk Neural Network
                : SOX_FILTER;   // Pipeline SoX — untuk Vosk
    
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
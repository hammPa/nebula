#include "nebula_engine.hpp"
#include "../config/config.hpp"
#include "../state/state.hpp"
#include "../audio/feedback.hpp"
#include "../waybar/waybar.hpp"
#include "../actions/actions.hpp"
#include "../utils/logger.hpp"
#include "../audio/capture.hpp"
#include <csignal>
#include <nlohmann/json.hpp>


extern volatile std::sig_atomic_t g_running;

static constexpr const int UNLOAD_TIMEOUT = 300;
// static constexpr const char* MFCC_MODEL     = "mfcc_preprocessor.onnx";
// static constexpr const char* WAKEWORD_MODEL = "nebula_wake_word.onnx";
// static constexpr const char* WAKEWORD_MODEL = "nebula_wake_word_int8.onnx";
static constexpr const char* WAKEWORD_MODEL = "nebula_full.onnx";

NebulaEngine::NebulaEngine(){}

bool NebulaEngine::init(const std::string& config_path) {
    if (!config_load(config_path)) {
        logger::err("MAIN", "Gagal memuat config: " + config_path);
        return false;
    }

    waybar::set_idle();

    // load vosk diubah jadi saat pertama membaca dari NN, tdi salah comment
    
    // load wakeword
    wakeword_ready_ = wakeword_.init(WAKEWORD_MODEL);
    if (!wakeword_ready_) {
        // Tidak fatal — fallback ke perilaku lama (Vosk selalu aktif)
        logger::err("MAIN", "Wake word model gagal dimuat — fallback ke Vosk penuh.");
        logger::err("MAIN", "Pastikan '" + std::string(WAKEWORD_MODEL) + "' ada di direktori build.");
    } else {
        logger::info("MAIN", "Wake word NN aktif — mode hemat daya.");
    }

    stream = StreamController(wakeword_ready_);
    return true;
}

void NebulaEngine::handle_result(const char* result) {
    try {
        actions::process(result);
    } catch (const nlohmann::json::parse_error& e) {
        logger::err("MAIN", std::string("JSON parse error: ") + e.what());
    }
}

// Fase IDLE, hanya wake word NN yang jalan
void NebulaEngine::run_idle_phase(const char* buf, int n) {
    // Cast buf ke int16 untuk dikirim ke wake word detector
    // buf berasal dari arecord S16_LE sehingga sudah int16
    const int16_t* pcm      = reinterpret_cast<const int16_t*>(buf);
    int n_samples = n / 2;   // 2 bytes per sample (int16)
 
    if (wakeword_.feed(pcm, n_samples)) {
        // Wake word terdeteksi oleh NN
        audio::feedback(audio::Event::Wake);
        waybar::set_listening();

        if (!vosk.load()) {
            logger::err("MAIN", "Gagal load Vosk setelah wake word.");
            return;
        }

        state::set(state::State::Listening);
        state::reset_timeout();
        logger::info("MAIN", "Wake word NN: terdeteksi → Listening");
    }
}
 
// Fase LISTENING, Vosk aktif untuk ASR perintah
void NebulaEngine::run_listening_phase(const char* buf, int n) {
    if (vosk.accept(buf, n)) {
        handle_result(vosk.result());
    }
}

void NebulaEngine::check_timeouts() {
    // kembali ke idle setelah 10 detik
    if (state::is_timed_out()) {
        state::set(state::State::Idle);
        state::reset_timeout(); // kalau udah timeout maka kan idle, kalau lebih 30 detik unload vosk
        audio::feedback(audio::Event::Timeout);
        waybar::set_idle();
        wakeword_.reset_buffer();
        logger::info("MAIN", "Timeout — kembali ke Standby.");
    }

    // unload setelah 5 menit idle
    if (vosk.is_loaded() && state::is_idle_timed_out(UNLOAD_TIMEOUT)) {
        vosk.unload();
        logger::info("MAIN", "Vosk di unload setelah 5 menit idle.");
        // Tidak perlu ubah state, tetap Idle.
    }
}

void NebulaEngine::run() {
    if(!stream.open()) return;

    // detect wakeword
    logger::info("MAIN", wakeword_ready_
        ? "Nebula siap [mode NN]. Ucapkan \"nebula\"..."
        : "Nebula siap [mode Vosk]. Ucapkan \"nebula\"...");
    logger::info("MAIN", "Noise reduction aktif.");
    
    int check_counter = 0;
    char buf[3200];
    while (g_running) {
        // LOGIKA SWITCHING STREAM (RAW vs FILTER)
        audio_capture::StreamMode target_mode = (state::get() == state::State::Idle && wakeword_ready_)
            ? audio_capture::StreamMode::IDLE
            : audio_capture::StreamMode::LISTENING;
    
        // mode stream diganti, mulai baca dari awal
        if(stream.switch_to(target_mode)) continue; 


        const size_t n = stream.read(buf, sizeof(buf));
        if (n == 0) break;

        // cek timeout setiap 1 detik
        if(++check_counter >= 10){
            check_counter = 0;
            check_timeouts();
        }

        // logger::info("ntahlah", wakeword_ready_ ? "ready oi": "no redy");
        // not ready -> vosk langsung
        if (!wakeword_ready_) {
            run_listening_phase(buf, static_cast<int>(n));
            continue;
        }

        // ready -> NN --> accept -> vosk
        // MODE BARU: dua fase
        if (state::get() == state::State::Idle)
            run_idle_phase(buf, static_cast<int>(n));
        else
            run_listening_phase(buf, static_cast<int>(n));
    }
}

void NebulaEngine::cleanup() {
    logger::info("MAIN", "Membersihkan resource dan keluar...");
    waybar::set_idle();
    std::system("pkill arecord; pkill sox");
}
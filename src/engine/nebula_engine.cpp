#include "nebula_engine.hpp"
#include "../config/config.hpp"
#include "../state/state.hpp"
#include "../audio/feedback.hpp"
#include "../waybar/waybar.hpp"
#include "../actions/actions.hpp"
#include "../utils/logger.hpp"
#include "../audio/capture.hpp"
#include <csignal>
#include <malloc.h>

extern volatile std::sig_atomic_t g_running;

static constexpr const int UNLOAD_TIMEOUT = 10;
static int check_counter = 0;
static constexpr const char* MFCC_MODEL     = "mfcc_preprocessor.onnx";
// static constexpr const char* WAKEWORD_MODEL = "nebula_wake_word.onnx";
static constexpr const char* WAKEWORD_MODEL = "nebula_wake_word_int8.onnx";

NebulaEngine::NebulaEngine() : model(nullptr, vosk_model_free), rec(nullptr, vosk_recognizer_free) {}

bool NebulaEngine::load_vosk() {
    if (vosk_loaded_) return true;

    model.reset(vosk_model_new("model"));
    if (!model) {
        logger::err("MAIN", "Folder 'model' vosk tidak ditemukan!");
        return false;
    }

    const std::string grammar = config_build_grammar();
    rec.reset(vosk_recognizer_new_grm(model.get(), 16000.0f, grammar.c_str()));
    if (!rec) {
        logger::err("MAIN", "Gagal inisialisasi Recognizer.");
        return false;
    }

    vosk_recognizer_set_max_alternatives(rec.get(), 0);
    vosk_recognizer_set_words(rec.get(), 1);

    vosk_loaded_ = true;
    logger::info("MAIN", "Vosk dimuat ke RAM.");
    return true;
}

void NebulaEngine::unload_vosk() {
    if (!vosk_loaded_) return;
    rec.reset();
    model.reset();
    vosk_loaded_ = false;
    malloc_trim(0);
    logger::info("MAIN", "Vosk diturunkan dari RAM.");
}

bool NebulaEngine::init(const std::string& config_path) {
    if (!config_load(config_path)) {
        logger::err("MAIN", "Gagal memuat config: " + config_path);
        return false;
    }

    waybar::set_idle();

    // load vosk diubah jadi saat pertama run
    
    // load wakeword
    wakeword_ready_ = wakeword_.init(MFCC_MODEL, WAKEWORD_MODEL);
    if (!wakeword_ready_) {
        // Tidak fatal — fallback ke perilaku lama (Vosk selalu aktif)
        logger::err("MAIN", "Wake word model gagal dimuat — fallback ke Vosk penuh.");
        logger::err("MAIN", "Pastikan '" + std::string(WAKEWORD_MODEL) + "' ada di direktori build.");
    } else {
        logger::info("MAIN", "Wake word NN aktif — mode hemat daya.");
    }

    return true;
}

void NebulaEngine::handle_result(const char* result) {
    try {
        const std::string raw(result);
        if (json::parse(result).value("text", "").empty()) return;
        actions::process(raw);
    } catch (const json::parse_error& e) {
        logger::err("MAIN", std::string("JSON parse error: ") + e.what());
    }
}

// Fase IDLE, hanya wake word NN yang jalan
void NebulaEngine::run_idle_phase(const char* buf, int n) {
    // Cast buf ke int16 untuk dikirim ke wake word detector
    // buf berasal dari arecord S16_LE sehingga sudah int16
    const int16_t* pcm      = reinterpret_cast<const int16_t*>(buf);
    int            n_samples = n / 2;   // 2 bytes per sample (int16)
 
    if (wakeword_.feed(pcm, n_samples)) {
        // Wake word terdeteksi oleh NN
        // wakeword_.reset_buffer();

        if (!load_vosk()) {
            logger::err("MAIN", "Gagal load Vosk setelah wake word.");
            return;
        }

        state::set(state::State::Listening);
        state::reset_timeout();
        audio::feedback(audio::Event::Wake);
        waybar::set_listening();
        logger::info("MAIN", "Wake word NN: terdeteksi → Listening");
    }
}
 
// Fase LISTENING, Vosk aktif untuk ASR perintah
void NebulaEngine::run_listening_phase(const char* buf, int n) {
    if (vosk_recognizer_accept_waveform(rec.get(), buf, n)) {
        handle_result(vosk_recognizer_result(rec.get()));
    }
}

void NebulaEngine::run() {
    audio_capture::StreamMode current_mode = wakeword_ready_ 
        ? audio_capture::StreamMode::IDLE 
        : audio_capture::StreamMode::LISTENING;

    auto capture = audio_capture::open_stream(current_mode);

    if (!capture || !audio_capture::smoke_test(capture.get())) {
        if (!capture) logger::err("MAIN", "Mic tidak bisa diakses!");
        return;
    }

    // detect wakeword
    if (wakeword_ready_) {
        logger::info("MAIN", "Nebula siap [mode NN]. Ucapkan \"nebula\"...");
    } else {
        logger::info("MAIN", "Nebula siap [mode Vosk]. Ucapkan \"nebula\"...");
    }
    logger::info("MAIN", "Noise reduction aktif: ");

    char buf[3200];
    while (g_running) {
        // LOGIKA SWITCHING STREAM (RAW vs FILTER)
        audio_capture::StreamMode target_mode = (state::get() == state::State::Idle && wakeword_ready_)
            ? audio_capture::StreamMode::IDLE 
            : audio_capture::StreamMode::LISTENING;

        if (current_mode != target_mode) {
            logger::info("MAIN", target_mode == audio_capture::StreamMode::IDLE 
                ? "Mematikan filter SoX (Pindah ke Raw Audio)" 
                : "Menghidupkan filter SoX (Pindah ke Vosk)");
            
            capture.reset(); // Tutup stream 'arecord' yang lama
            capture = audio_capture::open_stream(target_mode); // Buka yang baru
            current_mode = target_mode;
            
            if (!capture) {
                logger::err("MAIN", "Gagal switch stream mic!");
                break;
            }
            continue; // Skip sisa loop ini untuk langsung baca dari stream yang baru
        }


        const size_t n = std::fread(buf, 1, sizeof(buf), capture.get());
        if (n == 0) break;


        if(++check_counter >= 10){
            check_counter = 0;
            if (state::is_timed_out()) {
                state::set(state::State::Idle);
                state::reset_timeout(); // kalau udah timeout maka kan idle, kalau lebih 30 detik unload vosk
                audio::feedback(audio::Event::Timeout);
                waybar::set_idle();
                wakeword_.reset_buffer();
                logger::info("MAIN", "Timeout — kembali ke Standby.");
            }
    
            if (state::is_idle_timed_out(UNLOAD_TIMEOUT) && vosk_loaded_) {
                unload_vosk();
                logger::info("MAIN", "Vosk di unload setelah 5 menit idle.");
                // Tidak perlu ubah state, tetap Idle.
            }
        }

        // logger::info("ntahlah", wakeword_ready_ ? "ready oi": "no redy");
        if (wakeword_ready_) {
            // MODE BARU: dua fase
            if (state::get() == state::State::Idle) {
                run_idle_phase(buf, static_cast<int>(n));
            } else {
                run_listening_phase(buf, static_cast<int>(n));
            }
        } else {
            run_listening_phase(buf, static_cast<int>(n));
        }
    }
}

void NebulaEngine::cleanup() {
    logger::info("MAIN", "Membersihkan resource dan keluar...");
    waybar::set_idle();
    std::system("pkill arecord; pkill sox");
}
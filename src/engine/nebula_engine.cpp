#include "nebula_engine.hpp"
#include "../config/config.hpp"
#include "../state/state.hpp"
#include "../audio/feedback.hpp"
#include "../waybar/waybar.hpp"
#include "../actions/actions.hpp"
#include "../utils/logger.hpp"
#include "../audio/capture.hpp"
#include <csignal>

extern volatile std::sig_atomic_t g_running;

NebulaEngine::NebulaEngine() : model(nullptr, vosk_model_free), rec(nullptr, vosk_recognizer_free) {}

bool NebulaEngine::init(const std::string& config_path) {
    if (!config_load(config_path)) {
        logger::err("MAIN", "Gagal memuat config: " + config_path);
        return false;
    }

    waybar::set_idle();

    model.reset(vosk_model_new("model"));
    if (!model) {
        logger::err("MAIN", "Folder 'model' tidak ditemukan!");
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
    
    return true;
}

void NebulaEngine::handle_result(const char* result) {
    try {
        if (json::parse(result).value("text", "").empty()) return;
        actions::process(result);
    } catch (const json::parse_error& e) {
        logger::err("MAIN", std::string("JSON parse error: ") + e.what());
    }
}

void NebulaEngine::run() {
    const std::string noise_prof = "src/nebula_noise.prof";
    auto capture = audio_capture::open_stream(noise_prof);

    if (!capture || !audio_capture::smoke_test(capture.get())) {
        if (!capture) logger::err("MAIN", "Mic tidak bisa diakses!");
        return;
    }

    logger::info("MAIN", "Nebula siap. Ucapkan \"nebula\"...");
    logger::info("MAIN", "Noise reduction aktif: " + noise_prof);

    char buf[3200];
    while (g_running) {
        if (state::is_timed_out()) {
            state::set(state::State::Idle);
            audio::feedback(audio::Event::Timeout);
            waybar::set_idle();
            logger::info("MAIN", "Timeout — kembali ke Standby.");
        }

        const size_t n = std::fread(buf, 1, sizeof(buf), capture.get());
        if (n == 0) break;

        if (vosk_recognizer_accept_waveform(rec.get(), buf, static_cast<int>(n))) {
            handle_result(vosk_recognizer_result(rec.get()));
        }
    }
}

void NebulaEngine::cleanup() {
    logger::info("MAIN", "Membersihkan resource dan keluar...");
    waybar::set_idle();
    std::system("pkill arecord; pkill sox");
}
#include <iostream>
#include <csignal>
#include <malloc.h>
#include <nlohmann/json.hpp>
#include "vosk_api.h"

#include "config/config.hpp"
#include "state/state.hpp"
#include "audio/feedback.hpp"
#include "waybar/waybar.hpp"
#include "actions/actions.hpp"
#include "utils/logger.hpp"

using json = nlohmann::json;

using VoskModelPtr = std::unique_ptr<VoskModel,      decltype(&vosk_model_free)>;
using VoskRecPtr   = std::unique_ptr<VoskRecognizer, decltype(&vosk_recognizer_free)>;
using FilePtr      = std::unique_ptr<FILE,           decltype(&pclose)>;

volatile std::sig_atomic_t g_running = 1;
static void signal_handler(int) { g_running = 0; }

int main(int argc, char* argv[]) {
    mallopt(M_TRIM_THRESHOLD, 128 * 1024);
    mallopt(M_MMAP_THRESHOLD, 64 * 1024);

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    logger::banner();

    const std::string config_path = (argc > 1) ? argv[1] : "commands.json";
    if (!config_load(config_path)) {
        logger::err("MAIN", "Gagal memuat config: " + config_path);
        return 1;
    }

    waybar::set_idle();

    VoskModelPtr model(vosk_model_new("model"), vosk_model_free);
    if (!model) { logger::err("MAIN", "Folder 'model' tidak ditemukan!"); return 1; }

    const std::string grammar = config_build_grammar();
    VoskRecPtr rec(vosk_recognizer_new_grm(model.get(), 16000.0f, grammar.c_str()),
                   vosk_recognizer_free);
    if (!rec) { logger::err("MAIN", "Gagal inisialisasi Recognizer."); return 1; }

    FilePtr capture(
        popen("arecord -D plughw:CARD=Audio,DEV=0 -r 16000 -f S16_LE -c 1 -t raw -q 2>/dev/null", "r"),
        pclose);
    if (!capture) { logger::err("MAIN", "Mic tidak bisa diakses!"); return 1; }

    // Smoke-test: pastikan stream benar-benar jalan
    char test_buf[1024];
    if (std::fread(test_buf, 1, sizeof(test_buf), capture.get()) == 0) {
        logger::err("MAIN", "Stream kosong — mic mungkin sibuk atau tidak terdeteksi.");
        audio::feedback(audio::Event::MicError);
        return 1;
    }

    vosk_recognizer_set_max_alternatives(rec.get(), 0);
    vosk_recognizer_set_words(rec.get(), 1);

    logger::info("MAIN", "Nebula siap. Ucapkan \"nebula\"...");

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
            const char* result = vosk_recognizer_result(rec.get());
            try {
                if (json::parse(result).value("text", "").empty()) continue;
                actions::process(result);
            } catch (const json::parse_error& e) {
                logger::err("MAIN", std::string("JSON parse error: ") + e.what());
            }
        }
    }

    logger::info("MAIN", "Membersihkan resource dan keluar...");
    waybar::set_idle();
    std::system("pkill arecord");
    return 0;
}
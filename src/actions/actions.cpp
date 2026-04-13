#include "actions.hpp"
#include "../state/state.hpp"
#include "../config/config.hpp"
#include "../audio/feedback.hpp"
#include "../waybar/waybar.hpp"
#include "../utils/shell.hpp"
#include "../utils/logger.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace actions {

static constexpr float MIN_CONF_COMMAND = 0.9f;

// String helpers ────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    auto end   = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Confidence helpers ────────────────────────────────────────────────────────

// Ambil confidence terendah untuk kata tertentu di hasil Vosk
static float word_min_confidence(const json& j, const std::string& target) {
    if (!j.contains("result") || !j["result"].is_array()) return 1.0f;
    float lowest = 1.0f;
    bool  found  = false;
    for (const auto& w : j["result"]) {
        if (w.value("word", "") == target) {
            lowest = std::min(lowest, w.value("conf", 1.0f));
            found  = true;
        }
    }
    return found ? lowest : 1.0f;
}

// Cek apakah semua kata pada wake word memenuhi threshold-nya
static bool wakeword_passes(const json& j, const WakeWord& ww) {
    std::istringstream ss(ww.word);
    std::string token;
    float min_conf = 1.0f;
    bool  found    = false;
    while (ss >> token) {
        min_conf = std::min(min_conf, word_min_confidence(j, token));
        found = true;
    }
    if (!found || min_conf < ww.confidence) {
        logger::info("ACTIONS", "Wake word \"" + ww.word + "\" ditolak — conf "
                  + std::to_string(min_conf) + " < " + std::to_string(ww.confidence));
        return false;
    }
    return true;
}

// Cek apakah semua kata dalam utterance memenuhi threshold command
static bool command_passes(const json& j) {
    if (!j.contains("result") || !j["result"].is_array()) return true;
    for (const auto& w : j["result"]) {
        float conf = w.value("conf", 1.0f);
        if (conf < MIN_CONF_COMMAND) {
            logger::info("ACTIONS", "Confidence rendah (" + std::to_string(conf)
                      + ") pada \"" + w.value("word", "?") + "\" — diabaikan");
            return false;
        }
    }
    return true;
}

// Window helpers ────────────────────────────────────────────────────────────

static void close_window_by_class(const std::string& wclass) {
    try {
        auto active = json::parse(shell::read("hyprctl activewindow -j"));
        if (!active.is_null() && active.value("class", "") == wclass) {
            shell::run_bg("hyprctl dispatch killactive");
            return;
        }
        auto clients = json::parse(shell::read("hyprctl clients -j"));
        for (const auto& c : clients) {
            if (c.value("class", "") == wclass) {
                shell::run_bg("hyprctl dispatch closewindow address:" + c.value("address", ""));
                return;
            }
        }
    } catch (...) {
        logger::err("ACTIONS", "Gagal memproses JSON Hyprland");
    }
}

// Command runner ────────────────────────────────────────────────────────────

static void run_command(const Command& cmd) {
    static constexpr auto CLOSE_PREFIX = "__close_class__ ";
    if (cmd.shell.rfind(CLOSE_PREFIX, 0) == 0)
        close_window_by_class(cmd.shell.substr(std::strlen(CLOSE_PREFIX)));
    else
        shell::run(cmd.shell);

    if (!cmd.notify.empty())
        shell::run_bg("notify-send 'Nebula' '" + cmd.notify + "' -t 1500");
}

static bool contains_whole_word(const std::string& text, const std::string& word) {
    size_t pos = text.find(word);
    while (pos != std::string::npos) {
        bool left_ok  = (pos == 0 || !std::isalpha(text[pos - 1]));
        bool right_ok = (pos + word.size() >= text.size() || !std::isalpha(text[pos + word.size()]));
        if (left_ok && right_ok) return true;
        pos = text.find(word, pos + 1);
    }
    return false;
}

// Handlers per state ────────────────────────────────────────────────────────

// Kembalikan true jika wake word ditemukan dan diproses
static bool handle_wakeword(const json& j, const std::string& text) {
    for (const auto& ww : g_wakeWords) {
        if (!contains_whole_word(text, ww.word)) continue;
        if (!wakeword_passes(j, ww)) return true; // ada, tapi conf kurang → tolak & stop
        state::set(state::State::Listening);
        state::reset_timeout();
        audio::feedback(audio::Event::Wake);
        waybar::set_listening();
        return true;
    }
    return false;
}

static bool handle_stopword(const std::string& text) {
    for (const auto& sw : g_stopWords) {
        if (text != sw) continue;
        state::set(state::State::Idle);
        audio::feedback(audio::Event::Idle);
        waybar::set_idle();
        return true;
    }
    return false;
}

static bool handle_command(const std::string& text) {
    for (const auto& cmd : g_commands) {
        for (const auto& phrase : cmd.phrases) {
            if (!contains_whole_word(text, phrase)) continue;
            waybar::set_transcript(text);
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            run_command(cmd);
            audio::feedback(audio::Event::Action);
            waybar::set_action(phrase);
            state::reset_timeout();
            return true;
        }
    }
    return false;
}

// Entry point ───────────────────────────────────────────────────────────────

void process(const std::string& raw_json) {
    json j;
    try { j = json::parse(raw_json); }
    catch (...) { return; }

    const std::string text = trim(j.value("text", ""));
    if (text.empty()) return;

    logger::info("ACTIONS", "Mendengar: [" + text + "]");

    // Resolve timeout lebih awal
    if (state::is_timed_out()) {
        state::set(state::State::Idle);
        audio::feedback(audio::Event::Timeout);
        waybar::set_idle();
    }

    if (handle_wakeword(j, text)) return;
    if (state::get() != state::State::Listening) return;
    if (!command_passes(j)) return;
    if (handle_stopword(text)) return;
    if (handle_command(text)) return;

    // Tidak ada yang cocok — tampilkan teks & reset timer
    waybar::set_transcript(text);
    state::reset_timeout();
}

} // namespace actions
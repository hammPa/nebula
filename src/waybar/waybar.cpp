#include "waybar.hpp"
#include "../utils/shell.hpp"
#include "../utils/logger.hpp"
#include <fstream>
#include <csignal>

namespace waybar {

static constexpr auto JSON_PATH  = "/tmp/nebula_waybar.json";
static constexpr auto PROC_NAME  = "waybar";
static constexpr int  SIGNAL_NUM = 34 + 8; // SIGRTMIN+8

static int find_pid() {
    const std::string out = shell::read("pgrep -x " + std::string(PROC_NAME));
    try { return std::stoi(out); }
    catch (...) { return -1; }
}

static void signal_refresh() {
    int pid = find_pid();
    if (pid > 0) kill(pid, SIGNAL_NUM);
}

// Bangun JSON dengan escaping minimal — cukup untuk teks normal
static std::string make_json(const std::string& text,
                              const std::string& tooltip,
                              const std::string& css_class) {
    return "{\"text\":\"" + text + "\","
           "\"tooltip\":\"" + tooltip + "\","
           "\"class\":\"" + css_class + "\"}\n";
}

static void write_and_refresh(const std::string& text,
                               const std::string& tooltip,
                               const std::string& css_class) {
    std::ofstream f(JSON_PATH, std::ios::trunc);
    if (!f) { logger::err("WAYBAR", "Gagal menulis ke " + std::string(JSON_PATH)); return; }
    f << make_json(text, tooltip, css_class);
    signal_refresh();
}

// ── API publik ────────────────────────────────────────────────────────────────

void set_idle() {
    write_and_refresh("󰍮 ", "Nebula: Standby", "nebula-idle");
}

void set_listening() {
    write_and_refresh("󰍬 ", "Nebula: Mendengarkan...", "nebula-listening");
}

void set_transcript(const std::string& spoken) {
    write_and_refresh("󰍬 " + spoken, "Nebula: " + spoken, "nebula-listening");
}

void set_action(const std::string& action_name) {
    write_and_refresh("󱐋 ", "Nebula: " + action_name, "nebula-action");
}

} // namespace waybar
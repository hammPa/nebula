#include <fstream>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include "config.hpp"

using json = nlohmann::json;

// Global variables
std::vector<WakeWord> g_wakeWords;
std::vector<std::string> g_stopWords;
std::vector<Command> g_commands;
int g_listenTimeoutSec = 10;

bool config_load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    try {
        auto j = json::parse(f);

        // ── Wake words dengan per-word confidence ──────────────────────────
        g_wakeWords.clear();
        for (auto& w : j.at("wake_words")) {
            WakeWord ww;
            // Support dua format:
            //   string lama  → { "word": w, confidence: default }
            //   object baru  → { "word": "nebula", "confidence": 0.95 }
            if (w.is_string()) {
                ww.word       = w.get<std::string>();
                ww.confidence = 0.95f;
            } else {
                ww.word       = w.value("word", "");
                ww.confidence = w.value("confidence", 0.95f);
            }
            if (!ww.word.empty()){
                std::istringstream ss(ww.word);
                std::string token;
                while(ss >> token){
                    ww.tokens.push_back(token);
                }
                g_wakeWords.push_back(ww);
            }
        }

        // ── Stop words ─────────────────────────────────────────────────────
        g_stopWords.clear();
        for (auto& s : j.at("stop_words"))
            g_stopWords.push_back(s.get<std::string>());

        g_listenTimeoutSec = j.value("listen_timeout_sec", 5);

        // ── Commands ───────────────────────────────────────────────────────
        g_commands.clear();
        for (auto& c : j.at("commands")) {
            Command cmd;
            cmd.shell  = c.value("shell", "");
            cmd.notify = c.value("notify", "");
            for (auto& p : c.at("phrases"))
                cmd.phrases.push_back(p.get<std::string>());
            g_commands.push_back(cmd);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[CONFIG] Parse error: " << e.what() << "\n";
        return false;
    }
}

std::string config_build_grammar() {
    json arr = json::array();
    for (auto& w : g_wakeWords) arr.push_back(w.word); // ← .word ditambah
    for (auto& w : g_stopWords) arr.push_back(w);
    for (auto& cmd : g_commands)
        for (auto& p : cmd.phrases) arr.push_back(p);
    // arr.push_back("[unk]");
    return arr.dump();
}
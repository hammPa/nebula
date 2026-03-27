// src/config/config.hpp
#pragma once
#include <string>
#include <vector>

struct WakeWord {
    std::string word;
    float confidence = 0.95f; // default
};

struct Command {
    std::vector<std::string> phrases;
    std::string shell;
    std::string notify;
};

// --- DEKLARASI EXTERN (Janji ke file lain) ---
extern std::vector<WakeWord> g_wakeWords;
extern std::vector<std::string> g_stopWords;
extern std::vector<Command> g_commands;
extern int g_listenTimeoutSec;

bool config_load(const std::string& path);
std::string config_build_grammar();
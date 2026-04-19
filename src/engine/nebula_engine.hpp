#pragma once
#include "../audio/wakeword.hpp"
#include "./vosk_manager.hpp"
#include "./stream_controller.hpp"
#include <string>

class NebulaEngine {
public:
    NebulaEngine();
    bool init(const std::string& config_path);
    void run();
    void cleanup();

private:
    VoskManager vosk;
    StreamController stream{false};
    WakeWordDetector wakeword_;
    bool wakeword_ready_ = false;

    // fase run:
    // idle: wakeword
    // listening: vosk
    void run_idle_phase(const char* buf, int n);
    void run_listening_phase(const char* buf, int n);
    void handle_result(const char* result);
    void check_timeouts();
};
#pragma once
#include "vosk_api.h" // asumsikan alias VoskModelPtr dsb ada di sini atau didefinisikan ulang
#include <nlohmann/json.hpp>
#include <memory>
#include "../audio/wakeword.hpp"

using json = nlohmann::json;
using VoskModelPtr = std::unique_ptr<VoskModel, decltype(&vosk_model_free)>;
using VoskRecPtr   = std::unique_ptr<VoskRecognizer, decltype(&vosk_recognizer_free)>;

class NebulaEngine {
public:
    NebulaEngine();
    bool init(const std::string& config_path);
    void run();
    void cleanup();

private:
    // saat listening
    VoskModelPtr model;
    VoskRecPtr rec;
    void handle_result(const char* result);

    bool vosk_loaded_ = false;
    bool load_vosk();
    void unload_vosk();

    // wake word, selalu aktif
    WakeWordDetector wakeword_;
    bool wakeword_ready_ = false;

    // fase run:
    // idle: wakeword
    // listening: vosk
    void run_idle_phase(const char* buf, int n);
    void run_listening_phase(const char* buf, int n);
};
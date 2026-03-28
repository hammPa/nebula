#pragma once
#include "vosk_api.h" // asumsikan alias VoskModelPtr dsb ada di sini atau didefinisikan ulang
#include <nlohmann/json.hpp>
#include <memory>

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
    VoskModelPtr model;
    VoskRecPtr rec;
    void handle_result(const char* result);
};
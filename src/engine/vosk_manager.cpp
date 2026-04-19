#include "vosk_manager.hpp"
#include "../config/config.hpp"
#include "../utils/logger.hpp"
#include <malloc.h>

VoskManager::VoskManager()
    : model(nullptr, vosk_model_free),
      rec(nullptr, vosk_recognizer_free)
{}

bool VoskManager::load() {
    if (loaded_) return true;

    model.reset(vosk_model_new("model"));
    if (!model) {
        logger::err("VOSK", "Folder 'model' tidak ditemukan!");
        return false;
    }

    const std::string grammar = config_build_grammar();
    rec.reset(vosk_recognizer_new_grm(model.get(), 16000.0f, grammar.c_str()));
    if (!rec) {
        logger::err("VOSK", "Gagal inisialisasi Recognizer.");
        model.reset();
        return false;
    }

    vosk_recognizer_set_max_alternatives(rec.get(), 0);
    vosk_recognizer_set_words(rec.get(), 1);

    loaded_ = true;
    logger::info("VOSK", "Model dimuat ke RAM.");
    return true;
}

void VoskManager::unload() {
    if (!loaded_) return;
    rec.reset();
    model.reset();
    loaded_ = false;
    malloc_trim(0);
    logger::info("VOSK", "Model diturunkan dari RAM.");
}

bool VoskManager::accept(const char* buf, int n) {
    if (!loaded_) return false;
    return vosk_recognizer_accept_waveform(rec.get(), buf, n);
}

const char* VoskManager::result() {
    if (!loaded_) return "{}";
    return vosk_recognizer_result(rec.get());
}
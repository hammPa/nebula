#pragma once
#include "vosk_api.h"
#include <memory>
#include <string>

using VoskModelPtr = std::unique_ptr<VoskModel, decltype(&vosk_model_free)>;
using VoskRecPtr   = std::unique_ptr<VoskRecognizer, decltype(&vosk_recognizer_free)>;

class VoskManager {
public:
    VoskManager();

    // Load model + recognizer ke RAM. No-op jika sudah loaded.
    bool load();

    // Unload dan trim heap. No-op jika belum loaded.
    void unload();

    bool is_loaded() const { return loaded_; }

    // Umpan satu chunk PCM. Return true jika satu utterance selesai dikenali.
    bool accept(const char* buf, int n);

    // Ambil hasil JSON terakhir (valid setelah accept() return true).
    const char* result();

private:
    VoskModelPtr model;
    VoskRecPtr rec;
    bool loaded_ = false;
};
#include "stream_controller.hpp"
#include "../utils/logger.hpp"
#include "../audio/capture.hpp"
#include <cstring>

StreamController::StreamController(bool wakeword_ready)
    : current_mode(wakeword_ready
        ? audio_capture::StreamMode::IDLE
        : audio_capture::StreamMode::LISTENING)
{}

bool StreamController::open() {
    capture = audio_capture::open_stream(current_mode);
    if (!capture){
        logger::err("STREAM", "Mic tidak bisa diakses!");
        return false;
    }

    std::vector<char> prefetch;
    if (!audio_capture::smoke_test(capture.get(), prefetch)) {
        capture.reset();
        return false;
    }

    prefetch_buf = std::move(prefetch);
    return true;
}

bool StreamController::switch_to(audio_capture::StreamMode target) {
    if (current_mode == target) return false;

    logger::info("STREAM", target == audio_capture::StreamMode::IDLE
        ? "Mematikan filter SoX (Pindah ke Raw Audio)"
        : "Menghidupkan filter SoX (Pindah ke Vosk)");

    capture.reset();
    prefetch_buf.clear();
    capture = audio_capture::open_stream(target);
    current_mode = target;

    if (!capture) logger::err("STREAM", "Gagal switch stream mic!");
    return true;
}

size_t StreamController::read(char* buf, size_t size) {
    if (!capture) return 0;

    // keluarkan prefetch kalau masih ada
    if(!prefetch_buf.empty()){
        size_t n = std::min(size, prefetch_buf.size());
        std::memcpy(buf, prefetch_buf.data(), n);
        prefetch_buf.erase(prefetch_buf.begin(), prefetch_buf.begin() + n);
        return n;
    }
    return std::fread(buf, 1, size, capture.get());
}
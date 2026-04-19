#pragma once
#include <memory>
#include <string>
#include <cstdio>
#include <vector>

// using FilePtr = std::unique_ptr<FILE, decltype(&pclose)>;
struct PipeDeleter {
    void operator()(FILE* f) const { if (f) pclose(f); }
};
using FilePtr = std::unique_ptr<FILE, PipeDeleter>;


namespace audio_capture {
    enum class StreamMode {
        IDLE,       // Raw (untuk Wake Word CRNN)
        LISTENING   // Difilter SoX (untuk Vosk)
    };
    // Membungkus logic pembuatan command pipe sox
    FilePtr open_stream(StreamMode mode);
    bool smoke_test(FILE* stream, std::vector<char>& prefetch);
}
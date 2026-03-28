#pragma once
#include <memory>
#include <string>
#include <cstdio>

using FilePtr = std::unique_ptr<FILE, decltype(&pclose)>;

namespace audio_capture {
    // Membungkus logic pembuatan command pipe sox
    FilePtr open_stream(const std::string& noise_prof);
    bool smoke_test(FILE* stream);
}
#pragma once
#include "../audio/capture.hpp"
#include <memory>
#include <cstdio>
#include <vector>

class StreamController {
public:
    explicit StreamController(bool wakeword_ready);

    // Buka stream pertama kali. Return false jika mic tidak bisa diakses.
    bool open();

    // Switch ke mode baru jika berbeda dari mode saat ini.
    // Return true jika switch terjadi (caller harus `continue` loop-nya).
    // Return false jika mode sama, tidak ada perubahan.
    bool switch_to(audio_capture::StreamMode target);

    // Baca audio ke buf. Return jumlah byte terbaca (0 = stream mati).
    size_t read(char* buf, size_t size);

    bool is_open() const { return capture != nullptr; }

private:
    audio_capture::StreamMode current_mode;
    FilePtr capture{nullptr};
    std::vector<char> prefetch_buf;
};
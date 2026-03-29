#pragma once
#include <string>
#include <memory>
#include <cstdio>

namespace shell {

// Jalankan perintah, kembalikan output stdout-nya
inline std::string read(const std::string& cmd) {
    std::string result;
    char buf[128];

    struct PipeDeleter {
        void operator()(FILE* f) const { if (f) pclose(f); }
    };
    std::unique_ptr<FILE, PipeDeleter> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) return "";
    while (fgets(buf, sizeof(buf), pipe.get()))
        result += buf;
    return result;
}

// Jalankan tanpa ambil output (fire & forget, non-blocking via &)
inline void run(const std::string& cmd) {
    std::system(cmd.c_str());
}

inline void run_bg(const std::string& cmd) {
    std::system((cmd + " &").c_str());
}

} // namespace shell
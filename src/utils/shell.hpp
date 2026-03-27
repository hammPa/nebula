#pragma once
#include <string>
#include <memory>
#include <cstdio>

namespace shell {

// Jalankan perintah, kembalikan output stdout-nya
inline std::string read(const std::string& cmd) {
    std::string result;
    char buf[128];
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
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
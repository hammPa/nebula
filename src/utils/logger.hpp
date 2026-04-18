#pragma once
#include <iostream>
#include <string>

namespace logger {

#define C_RESET  "\033[0m"
#define C_CYAN   "\033[96m"
#define C_YELLOW "\033[93m"
#define C_BOLD   "\033[1m"
#define C_RED    "\033[91m"

inline void info(const std::string& module, const std::string& msg) {
    std::cout << C_CYAN << "[" << module << "] " << C_RESET << msg << "\n";
}

inline void warn(const std::string& module, const std::string& msg) {
    std::cerr << C_YELLOW << "[" << module << "] " << msg << C_RESET << "\n";
}

inline void err(const std::string& module, const std::string& msg) {
    std::cerr << C_RED << "[" << module << "] ERROR: " << msg << C_RESET << "\n";
}

inline void banner() {
    std::cout << C_BOLD << C_CYAN
              << "╔══════════════════════════════════╗\n"
              << "║     NEBULA V10.1  —  HYPRLAND    ║\n"
              << "║   Neural Network Merged Session  ║\n"
              << "╚══════════════════════════════════╝\n"
              << C_RESET;
}

} // namespace log
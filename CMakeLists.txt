cmake_minimum_required(VERSION 3.10)
project(Nebula VERSION 8.0)
set(CMAKE_CXX_STANDARD 17)

# ── Source files (semua .cpp, tanpa .h/.hpp) ──────────────────────────────────
set(NEBULA_SOURCES
    src/main.cpp
    src/config/config.cpp
    src/state/state.cpp
    src/audio/feedback.cpp
    src/waybar/waybar.cpp
    src/actions/actions.cpp
)

add_executable(nebula ${NEBULA_SOURCES})

# ── Include paths ─────────────────────────────────────────────────────────────
# Vosk header di root project (vosk_api.h)
target_include_directories(nebula PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(nebula
    ${CMAKE_CURRENT_SOURCE_DIR}/libvosk.so
    -lpthread
    -ldl
)

# ── Copy commands.json ke build dir otomatis ──────────────────────────────────
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/commands.json
    ${CMAKE_CURRENT_BINARY_DIR}/commands.json
    COPYONLY
)

# ── Pesan setelah build ───────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "  Nebula V8 siap di-build.")
message(STATUS "  Jalankan: ./nebula [path/ke/commands.json]")
message(STATUS "  Default config: commands.json di direktori yang sama dengan binary.")
message(STATUS "")
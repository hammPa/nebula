cmake_minimum_required(VERSION 3.10)
project(Nebula VERSION 10.4)
set(CMAKE_CXX_STANDARD 17)

# lokasi output binary
set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# ── Source files (semua .cpp, tanpa .h/.hpp) ──────────────────────────────────
set(NEBULA_SOURCES
    src/main.cpp
    src/config/config.cpp
    src/state/state.cpp
    src/audio/feedback.cpp
    src/engine/nebula_engine.cpp
    src/engine/vosk_manager.cpp
    src/engine/stream_controller.cpp
    src/audio/capture.cpp
    src/audio/wakeword.cpp
    src/waybar/waybar.cpp
    src/actions/actions.cpp
)

add_executable(nebula ${NEBULA_SOURCES})

# ── Konfigurasi Mode Build (Debug vs Production) ──────────────────────────────
# Jika tidak ada flag yang diberikan saat cmake, jadikan "Release" (Production) sebagai default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Pilih mode build: Debug atau Release" FORCE)
endif()

# Terapkan flag compiler berdasarkan mode
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "[Nebula] Mode Build : DEBUG (Tanpa optimasi, mudah di-debug)")
    # -g untuk memunculkan simbol debug (bisa dibaca gdb/valgrind)
    # -O0 mematikan optimasi compiler
    target_compile_options(nebula PRIVATE -g -O0 -Wall)
    
    # Opsional: Bisa tambahkan macro C++ khusus debug
    target_compile_definitions(nebula PRIVATE DEBUG_MODE)
else()
    message(STATUS "[Nebula] Mode Build : PRODUCTION / RELEASE (Kinerja maksimal)")
    # Optimasi -O3, instruksi CPU lokal, dan optimasi matematika cepat
    target_compile_options(nebula PRIVATE -O3 -march=native -ffast-math)
    
    # -DNDEBUG mematikan assert() di C++ yang bisa memakan CPU
    target_compile_definitions(nebula PRIVATE NDEBUG)

    # strip agar ukuran kecil
    if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_custom_command(TARGET nebula POST_BUILD
        COMMAND strip $<TARGET_FILE:nebula>
        COMMENT "[Nebula] Membuang simbol debug (stripping) untuk mengecilkan ukuran file..."
    )
    endif()
endif()

# ── Include paths ─────────────────────────────────────────────────────────────
# Vosk header di root project (vosk_api.h)
target_include_directories(nebula PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/include
    /usr/local/include/onnxruntime
)


# ── Cari ONNX Runtime library ─────────────────────────────────────────────────
# Cari di /usr/local/lib dulu (hasil install manual), lalu /usr/lib
find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    HINTS /usr/local/lib /usr/lib
)
 
if(NOT ONNXRUNTIME_LIB)
    message(FATAL_ERROR
        "\n[Nebula] libonnxruntime tidak ditemukan!\n"
        "Install dengan:\n"
        "  cd ~/Downloads\n"
        "  wget https://github.com/microsoft/onnxruntime/releases/download/v1.17.3/onnxruntime-linux-x64-1.17.3.tgz\n"
        "  tar -xzf onnxruntime-linux-x64-1.17.3.tgz\n"
        "  sudo cp -r onnxruntime-linux-x64-1.17.3/include/* /usr/local/include/\n"
        "  sudo cp -r onnxruntime-linux-x64-1.17.3/lib/*     /usr/local/lib/\n"
        "  sudo ldconfig\n"
    )
endif()
 
message(STATUS "[Nebula] ONNX Runtime: ${ONNXRUNTIME_LIB}")


# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(nebula
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/libvosk.so
    ${ONNXRUNTIME_LIB}
    -lpthread
    -ldl
)

# ── Copy commands.json ke build dir otomatis ──────────────────────────────────
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/commands.json
    ${CMAKE_CURRENT_BINARY_DIR}/commands.json
    COPYONLY
)



# ── Copy model ONNX (nebula_full.onnx) ───────────────────────────────────────
set(ONNX_FULL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/nebula_full.onnx)
if(EXISTS ${ONNX_FULL_SRC})
    configure_file(${ONNX_FULL_SRC} ${CMAKE_CURRENT_BINARY_DIR}/nebula_full.onnx COPYONLY)
    message(STATUS "[Nebula] nebula_full.onnx ditemukan.")
else()
    message(WARNING "[Nebula] nebula_full.onnx tidak ditemukan di root project.")
endif()


# ── Pesan setelah build ───────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "  Nebula V9 siap di-build.")
message(STATUS "  Jalankan: ${OUTPUT_DIR}/nebula [path/ke/commands.json]")
message(STATUS "  Default config: commands.json di direktori yang sama dengan main.cpp.")
message(STATUS "")
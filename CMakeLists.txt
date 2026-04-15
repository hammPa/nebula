cmake_minimum_required(VERSION 3.10)
project(Nebula VERSION 9.0)
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
    src/audio/capture.cpp
    src/audio/wakeword.cpp
    src/waybar/waybar.cpp
    src/actions/actions.cpp
    third_party/kissfft/kiss_fft.c
)

add_executable(nebula ${NEBULA_SOURCES})


set_source_files_properties(
    third_party/kissfft/kiss_fft.c
    PROPERTIES COMPILE_FLAGS "-w"
)

# ── Include paths ─────────────────────────────────────────────────────────────
# Vosk header di root project (vosk_api.h)
target_include_directories(nebula PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/include
    /usr/local/include/onnxruntime
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/kissfft
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



# Copy model ONNX ke build dir kalau sudah ada
set(ONNX_MODEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/nebula_wake_word.onnx)
set(ONNX_DATA_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/nebula_wake_word.onnx.data)
 
if(EXISTS ${ONNX_MODEL_SRC})
    configure_file(${ONNX_MODEL_SRC} ${CMAKE_CURRENT_BINARY_DIR}/nebula_wake_word.onnx COPYONLY)
    message(STATUS "[Nebula] Model ONNX ditemukan dan akan di-copy ke build dir.")
else()
    message(WARNING
        "[Nebula] nebula_wake_word.onnx tidak ditemukan di root project.\n"
        "Letakkan file ini di: ${CMAKE_CURRENT_SOURCE_DIR}/nebula_wake_word.onnx"
    )
endif()
 
# Copy .data juga kalau ada (model besar biasanya punya file .onnx.data)
if(EXISTS ${ONNX_DATA_SRC})
    configure_file(${ONNX_DATA_SRC} ${CMAKE_CURRENT_BINARY_DIR}/nebula_wake_word.onnx.data COPYONLY)
    message(STATUS "[Nebula] Model data file (.onnx.data) ditemukan.")
endif()


# ── Pesan setelah build ───────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "  Nebula V9 siap di-build.")
message(STATUS "  Jalankan: ${OUTPUT_DIR}/nebula [path/ke/commands.json]")
message(STATUS "  Default config: commands.json di direktori yang sama dengan main.cpp.")
message(STATUS "")
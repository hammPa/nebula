#pragma once

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <array>

// Konfigurasi harus sama persis dengan Python torchaudio
static constexpr int   MFCC_BINS       = 40;
static constexpr int   MFCC_FRAMES     = 148;
static constexpr int   SAMPLE_RATE     = 16000;
static constexpr float TARGET_DURATION = 1.5f;
static constexpr int   TARGET_SAMPLES  = static_cast<int>(SAMPLE_RATE * TARGET_DURATION); // 24000
static constexpr int   HOP_LENGTH      = 160;

// Threshold deteksi
static constexpr float DETECT_THRESHOLD = 0.85f;  
static constexpr float SILENCE_THRESHOLD_SQ = 0.035f * 0.035f; // 0.0004f

class WakeWordDetector {
public:
    WakeWordDetector();
    ~WakeWordDetector() = default;

    // Load DUA model ONNX (MFCC dan CRNN)
    bool init(const std::string& mfcc_model_path, const std::string& crnn_model_path);

    // Terima audio PCM int16, return true kalau wake word terdeteksi
    bool feed(const int16_t* pcm, int num_samples);

    // Reset buffer setelah wake word terdeteksi
    void reset_buffer();

    bool is_ready() const { return ready_; }

private:
    // Bagian ONNX Runtime
    Ort::Env                        env_;
    Ort::SessionOptions             session_opts_;
    std::unique_ptr<Ort::Session>   session_mfcc_;
    std::unique_ptr<Ort::Session>   session_crnn_;
    Ort::MemoryInfo                 mem_info_;

    bool ready_ = false;
    int detection_count_ = 0;

    // Ring buffer audio
    std::vector<float> audio_buf_;   
    int                buf_pos_ = 0; 

    // Akumulator agar inferensi tidak jalan di setiap frame yang terlalu kecil
    int accumulated_samples_ = 0;
    static constexpr int INFER_STRIDE_SAMPLES = 8000; // Inferensi setiap 500ms

    std::vector<float> linear_buf_;


    // ini semua dijadikan variabel private agar tidak di alokasi terus per500ms pada while utama, jdi tinggal reuse
    // Ort::RunOptions run_opts{nullptr};
    std::array<int64_t, 2> pcm_shape;
    std::array<int64_t, 4> crnn_shape;
};
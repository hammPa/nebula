#pragma once

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <array>

// Konfigurasi harus sama persis dengan Python torchaudio
// Arsitektur internal model ONNX:
// Input : raw PCM 24000 sampel (1.5s @ 16kHz)
// MFCC  : 40 bins, hop 160, → 148 frames
// Output: skor float [0,1]
static constexpr int   SAMPLE_RATE     = 16000;
static constexpr float TARGET_DURATION = 1.5f;
static constexpr int   TARGET_SAMPLES  = static_cast<int>(SAMPLE_RATE * TARGET_DURATION); // 24000

// Threshold deteksi
static constexpr float DETECT_THRESHOLD = 0.85f;  
static constexpr float SILENCE_THRESHOLD_SQ = 1e-4f; // percakapan normal
// static constexpr float SILENCE_THRESHOLD_SQ = 1e-3f;
    
static constexpr int INFER_STRIDE_SAMPLES = 8000; // Inferensi setiap 500ms

class WakeWordDetector {
public:
    WakeWordDetector();
    ~WakeWordDetector() = default;

    // Load DUA model ONNX (MFCC dan CRNN)
    bool init(const std::string& model_path);

    // Terima audio PCM int16, return true kalau wake word terdeteksi
    bool feed(const int16_t* pcm, int num_samples);

    // Reset buffer setelah wake word terdeteksi
    void reset_buffer();

    bool is_ready() const { return ready_; }

private:
    bool check_energy(const int16_t* pcm, int num_samples);
    bool prepare_window(); // extract + DC removal + full RMS
    bool run_inference(); // ONNX + debounce
    
    // Bagian ONNX Runtime
    Ort::Env                        env_;
    Ort::SessionOptions             session_opts_;
    std::unique_ptr<Ort::Session>   session_;
    Ort::MemoryInfo                 mem_info_;
    Ort::RunOptions run_opts{nullptr};
    Ort::Value pcm_tensor{nullptr};
    
    // Ring buffer audio - float[-1,1]
    std::vector<float> audio_buf_;   
    std::vector<float> linear_buf_;
    int buf_pos_ = 0; 
    bool ready_ = false;
    int detection_count_ = 0;

    // Akumulator agar inferensi tidak jalan di setiap frame yang terlalu kecil
    int accumulated_samples_ = 0;

    // ini semua dijadikan variabel private agar tidak di alokasi terus per500ms pada while utama, jdi tinggal reuse
    std::array<int64_t, 2> pcm_shape;
};
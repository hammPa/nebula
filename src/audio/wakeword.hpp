#pragma once

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <array>
#include "kiss_fft.h"

// Konfigurasi MFCC, harus sama dengan yang dipakai saat training di Python
static constexpr int   MFCC_BINS       = 40;
static constexpr int   MFCC_FRAMES     = 98;
static constexpr int   SAMPLE_RATE     = 16000;
static constexpr float TARGET_DURATION = 1.0f;
static constexpr int   TARGET_SAMPLES  = static_cast<int>(SAMPLE_RATE * TARGET_DURATION); // 24000
static constexpr int   HOP_LENGTH      = 160;
static constexpr int   N_FFT           = 400;
static constexpr float DETECT_THRESHOLD = 0.83f;
static constexpr float SILENCE_THRESHOLD = 0.015f; // RMS minimum

class WakeWordDetector {
public:
    WakeWordDetector();
    ~WakeWordDetector();

    // Load model ONNX, return false kalau gagal
    bool init(const std::string& model_path);

    // Terima audio PCM int16, simpan ke buffer internal
    // Return true kalau wake word terdeteksi
    bool feed(const int16_t* pcm, int num_samples);

    // Reset buffer setelah wake word terdeteksi
    void reset_buffer();

    bool is_ready() const { return ready_; }

private:
    // Bagian ONNX Runtime
    Ort::Env                        env_;
    Ort::SessionOptions             session_opts_;
    std::unique_ptr<Ort::Session>   session_;
    Ort::MemoryInfo                 mem_info_;

    bool ready_ = false;

    // Buffer audio (ring buffer)
    // Menyimpan 1.5 detik audio terakhir
    std::vector<float> audio_buf_;   // nilai float [-1.0, 1.0]
    int                buf_pos_ = 0; // posisi tulis berikutnya

    // Helper untuk MFCC
    // Hanning window untuk N_FFT
    std::array<float, N_FFT> hann_window_;

    void   init_hann_window();
    float  dot(const float* a, const float* b, int n);
    void   compute_mfcc(const float* audio, int n_samples,
                        std::vector<float>& out_mfcc);

    // Mel filterbank, diinisialisasi sekali saat init
    std::vector<std::vector<float>> mel_filters_;  // (N_MELS x N_FFT/2+1)
    void init_mel_filters(int n_mels, int n_fft, int sr,
                          float fmin, float fmax);
    static float hz_to_mel(float hz);
    static float mel_to_hz(float mel);

    // DCT untuk MFCC
    void apply_dct(const std::vector<float>& log_mel,
                   int n_mfcc, std::vector<float>& out);

    // Jalankan inferensi model
    float infer(const std::vector<float>& mfcc_flat);

    kiss_fft_cfg fft_cfg_ = nullptr;
    int feed_counter_ = 0;
    static constexpr int INFER_EVERY_N = 3; // inferensi tiap 3 chunk
};
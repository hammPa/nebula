#include "wakeword.hpp"
#include "../utils/logger.hpp"

#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WakeWordDetector::WakeWordDetector()
    : env_(ORT_LOGGING_LEVEL_WARNING, "nebula_wakeword"),
      mem_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    audio_buf_.assign(TARGET_SAMPLES, 0.0f);
    init_hann_window();
    fft_cfg_ = kiss_fft_alloc(N_FFT, 0, nullptr, nullptr); // 0 = forward FFT
}

WakeWordDetector::~WakeWordDetector() {
    if (fft_cfg_) kiss_fft_free(fft_cfg_);
}

// init
bool WakeWordDetector::init(const std::string& model_path) {
    try {
        session_opts_.SetIntraOpNumThreads(1);
        session_opts_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        session_ = std::make_unique<Ort::Session>(
            env_, model_path.c_str(), session_opts_
        );

        init_mel_filters(MFCC_BINS, N_FFT, SAMPLE_RATE, 0.0f, 8000.0f);

        ready_ = true;
        logger::info("WAKEWORD", "Model dimuat: " + model_path);
        return true;
    } catch (const Ort::Exception& e) {
        logger::err("WAKEWORD", std::string("Gagal load ONNX: ") + e.what());
        return false;
    }
}

// feed — terima PCM, isi ring buffer, inferensi tiap hop
bool WakeWordDetector::feed(const int16_t* pcm, int num_samples) {
    if (!ready_) return false;

    // Konversi int16 → float [-1.0, 1.0] dan tulis ke ring buffer
    for (int i = 0; i < num_samples; ++i) {
        audio_buf_[buf_pos_] = static_cast<float>(pcm[i]) / 32768.0f;
        buf_pos_ = (buf_pos_ + 1) % TARGET_SAMPLES;
    }

    // skip inferensi kalau belum waktunya
    feed_counter_ = (feed_counter_ + 1) % INFER_EVERY_N;
    if (feed_counter_ != 0) return false;


    // cek energi audio, skip kalau senyap
    float rms = 0.0f;
    for (int i = 0; i < TARGET_SAMPLES; ++i) {
        rms += audio_buf_[i] * audio_buf_[i];
    }
    rms = std::sqrt(rms / TARGET_SAMPLES);
    if (rms < SILENCE_THRESHOLD) return false;

    // Susun audio linear dari ring buffer (urut dari paling lama ke baru)
    std::vector<float> linear(TARGET_SAMPLES);
    for (int i = 0; i < TARGET_SAMPLES; ++i) {
        linear[i] = audio_buf_[(buf_pos_ + i) % TARGET_SAMPLES];
    }

    // Hitung MFCC
    std::vector<float> mfcc_flat;
    compute_mfcc(linear.data(), TARGET_SAMPLES, mfcc_flat);

    // Inferensi
    float score = infer(mfcc_flat);

    if (score > 0.0f) {  // lihat SEMUA skor termasuk saat bicara
        logger::info("WAKEWORD_DEBUG", "Skor: " + std::to_string(score) 
                + " | RMS: " + std::to_string(rms));
    }

    if (score > DETECT_THRESHOLD) {
        logger::info("WAKEWORD", "Terdeteksi! score=" + std::to_string(score));
        return true;
    }
    return false;
}

void WakeWordDetector::reset_buffer() {
    std::fill(audio_buf_.begin(), audio_buf_.end(), 0.0f);
    buf_pos_ = 0;
}

// Hanning window
void WakeWordDetector::init_hann_window() {
    for (int i = 0; i < N_FFT; ++i) {
        hann_window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (N_FFT - 1)));
    }
}

// Mel filterbank
float WakeWordDetector::hz_to_mel(float hz) {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

float WakeWordDetector::mel_to_hz(float mel) {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

void WakeWordDetector::init_mel_filters(int n_mels, int n_fft,
                                         int sr, float fmin, float fmax) {
    int n_freqs = n_fft / 2 + 1;
    mel_filters_.assign(n_mels, std::vector<float>(n_freqs, 0.0f));

    float mel_min = hz_to_mel(fmin);
    float mel_max = hz_to_mel(fmax);

    // n_mels + 2 titik mel yang merata
    std::vector<float> mel_pts(n_mels + 2);
    for (int i = 0; i < n_mels + 2; ++i) {
        mel_pts[i] = mel_to_hz(mel_min + (mel_max - mel_min) * i / (n_mels + 1));
    }

    // Frekuensi tiap bin FFT
    std::vector<float> freq_bins(n_freqs);
    for (int i = 0; i < n_freqs; ++i) {
        freq_bins[i] = static_cast<float>(i) * sr / n_fft;
    }

    for (int m = 0; m < n_mels; ++m) {
        float f_left   = mel_pts[m];
        float f_center = mel_pts[m + 1];
        float f_right  = mel_pts[m + 2];

        for (int k = 0; k < n_freqs; ++k) {
            float f = freq_bins[k];
            if (f >= f_left && f <= f_center) {
                mel_filters_[m][k] = (f - f_left) / (f_center - f_left + 1e-8f);
            } else if (f > f_center && f <= f_right) {
                mel_filters_[m][k] = (f_right - f) / (f_right - f_center + 1e-8f);
            }
        }
    }
}

// orthonormal DCT-II, sama persis dengan torchaudio
void WakeWordDetector::apply_dct(const std::vector<float>& log_mel,
                                  int n_mfcc, std::vector<float>& out) {
    int n = static_cast<int>(log_mel.size());
    out.resize(n_mfcc);

    // scale[0] = sqrt(1/N), scale[k>0] = sqrt(2/N) — orthonormal
    float scale0 = std::sqrt(1.0f / n);
    float scaleK = std::sqrt(2.0f / n);

    for (int k = 0; k < n_mfcc; ++k) {
        float sum = 0.0f;
        for (int n_ = 0; n_ < n; ++n_) {
            sum += log_mel[n_] * std::cos(M_PI * k * (2.0f * n_ + 1.0f) / (2.0f * n));
        }
        out[k] = sum * (k == 0 ? scale0 : scaleK);
    }
}

// compute_mfcc
// Menghasilkan vektor flat (MFCC_BINS x MFCC_FRAMES) — sama persis dengan
// torchaudio.transforms.MFCC yang dipakai saat training
void WakeWordDetector::compute_mfcc(const float* audio, int n_samples, std::vector<float>& out_mfcc) {
    int n_freqs  = N_FFT / 2 + 1;
    int n_frames = (n_samples - N_FFT) / HOP_LENGTH + 1;
    
    // 1. SIAPKAN BUFFER KOSONG UKURAN (40 x 98)
    // Gunakan 98 karena itu hasil center=False 16kHz
    const int FINAL_FRAMES = 98; 
    out_mfcc.assign(MFCC_BINS * FINAL_FRAMES, 0.0f);

    std::vector<float> frame(N_FFT, 0.0f);
    std::vector<kiss_fft_cpx> in_cpx(N_FFT), out_cpx(N_FFT);

    for (int t = 0; t < std::min(n_frames, FINAL_FRAMES); ++t) {
        int offset = t * HOP_LENGTH;

        // Windowing
        for (int i = 0; i < N_FFT; ++i) {
            frame[i] = audio[offset + i] * hann_window_[i];
            in_cpx[i].r = frame[i];
            in_cpx[i].i = 0.0f;
        }

        kiss_fft(fft_cfg_, in_cpx.data(), out_cpx.data());

        // Mel Energy & DCT
        std::vector<float> mel_energy(MFCC_BINS, 0.0f);
        for (int m = 0; m < MFCC_BINS; ++m) {
            for (int k = 0; k < n_freqs; ++k) {
                float mag_sq = out_cpx[k].r * out_cpx[k].r + out_cpx[k].i * out_cpx[k].i;
                mel_energy[m] += mel_filters_[m][k] * mag_sq;
            }
            mel_energy[m] = std::log(mel_energy[m] + 1e-7f);
        }

        std::vector<float> mfcc_frame;
        apply_dct(mel_energy, MFCC_BINS, mfcc_frame);

        // 2. MASUKKAN KE BARIS YANG BENAR (Penting!)
        for (int m = 0; m < MFCC_BINS; ++m) {
            out_mfcc[m * FINAL_FRAMES + t] = mfcc_frame[m];
        }
    }

    // 3. NORMALISASI (Perbaiki rumus std_dev kamu yang ada -1 nya)
    for (int m = 0; m < MFCC_BINS; ++m) {
        int row_start = m * FINAL_FRAMES;
        float sum = 0.0f, sq_sum = 0.0f;
        for (int t = 0; t < FINAL_FRAMES; ++t) {
            float val = out_mfcc[row_start + t];
            sum += val;
            sq_sum += val * val;
        }
        float mean = sum / FINAL_FRAMES;
        float var = (sq_sum / FINAL_FRAMES) - (mean * mean);
        float std_dev = std::sqrt(std::max(var, 1e-7f));

        for (int t = 0; t < FINAL_FRAMES; ++t) {
            out_mfcc[row_start + t] = (out_mfcc[row_start + t] - mean) / std_dev;
        }
    }
}

// infer
float WakeWordDetector::infer(const std::vector<float>& mfcc_flat) {
    // Shape: (1, 1, MFCC_BINS, MFCC_FRAMES)
    std::array<int64_t, 4> shape = {1, 1, MFCC_BINS, MFCC_FRAMES};

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem_info_,
        const_cast<float*>(mfcc_flat.data()),
        mfcc_flat.size(),
        shape.data(), shape.size()
    );

    const char* input_names[]  = {"input"};
    const char* output_names[] = {"output"};

    try {
        auto results = session_->Run(
            Ort::RunOptions{nullptr},
            input_names,  &input_tensor, 1,
            output_names, 1
        );
        float* out_data = results[0].GetTensorMutableData<float>();
        return out_data[0];
    } catch (const Ort::Exception& e) {
        logger::err("WAKEWORD", std::string("Inferensi gagal: ") + e.what());
        return 0.0f;
    }
}
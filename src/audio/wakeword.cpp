#include "wakeword.hpp"
#include "../utils/logger.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

WakeWordDetector::WakeWordDetector()
    : env_(ORT_LOGGING_LEVEL_WARNING, "nebula_wakeword"),
      mem_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      pcm_shape({1, TARGET_SAMPLES})

{
    audio_buf_.assign(TARGET_SAMPLES, 0.0f);
    linear_buf_.assign(TARGET_SAMPLES, 0.0f);
}

bool WakeWordDetector::init(const std::string& model_path) {
    try {
        session_opts_.SetIntraOpNumThreads(1);
        session_opts_.SetInterOpNumThreads(1);

        // matikan spinwait, hemat CPU saat idle
        session_opts_.AddConfigEntry("session.intra_op.allow_spinning", "0");
        session_opts_.AddConfigEntry("session.inter_op.allow_spinning", "0");
        session_opts_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_opts_);

        ready_ = true;
        logger::info("WAKEWORD", "Model MFCC & CRNN ONNX berhasil dimuat.");
        return true;
    } catch (const Ort::Exception& e) {
        logger::err("WAKEWORD", std::string("Gagal load ONNX: ") + e.what());
        return false;
    }
}

bool WakeWordDetector::feed(const int16_t* pcm, int num_samples) {
    if (!ready_) return false;

    float chunk_energy = 0.0f; // Menyimpan total energi dari potongan audio saat ini

    // 1. Tulis ke ring buffer dan konversi int16 ke float [-1.0, 1.0]
    for (int i = 0; i < num_samples; ++i) {
        float val = static_cast<float>(pcm[i]) / 32768.0f;
        audio_buf_[buf_pos_] = val;
        buf_pos_ = (buf_pos_ + 1) % TARGET_SAMPLES;

        chunk_energy += val * val;
    }

    // Hitung rata-rata energi (RMS kuadrat) khusus untuk potongan (chunk) terbaru ini
    float current_chunk_rms_sq = chunk_energy / num_samples;

    // 2. Akumulasi sampel (Inferensi tiap 8000 sampel / 500ms)
    accumulated_samples_ += num_samples;
    if (accumulated_samples_ < INFER_STRIDE_SAMPLES) return false; 
    accumulated_samples_ = 0;

    // Cek Keheningan Awal (Early Exit)
    // Jika 500ms terakhir ini hening, batalkan semua proses berat di bawahnya.
    if (current_chunk_rms_sq < SILENCE_THRESHOLD_SQ) {
        // CPU Anda terbebas dari alokasi memori dan inferensi di sini!
        // logger::info("WAKEWORD_DEBUG", "SKIP (hening) rms_sq=" + std::to_string(current_chunk_rms_sq));
        return false; 
    }

    // 3. Ekstrak audio dari ring buffer menjadi linear (oldest -> newest)
    float dc_mean = 0.0f;
    for (int i = 0; i < TARGET_SAMPLES; ++i) {
        float val = audio_buf_[(buf_pos_ + i) % TARGET_SAMPLES];
        linear_buf_[i] = val;
        dc_mean += val; 
    }

    // 4. Hilangkan DC Offset & Hitung RMS untuk cek keheningan
    dc_mean /= TARGET_SAMPLES;
    float rms = 0.0f;
    for (int i = 0; i < TARGET_SAMPLES; ++i) {
        linear_buf_[i] -= dc_mean; 
        rms += linear_buf_[i] * linear_buf_[i];
    }
    float rms_sq = rms / TARGET_SAMPLES; // rms² tanpa sqrt
    if (rms_sq < SILENCE_THRESHOLD_SQ) return false;

    try {
        static const char* in_names[]  = {"mfcc_pcm_audio"};
        static const char* out_names[] = {"output"};

        Ort::RunOptions run_opts{nullptr};
        Ort::Value pcm_tensor = Ort::Value::CreateTensor<float>(
            mem_info_, linear_buf_.data(), linear_buf_.size(),
            pcm_shape.data(), pcm_shape.size()
        );

        auto results = session_->Run(
            run_opts, in_names, &pcm_tensor, 1, out_names, 1
        );

        float score = results[0].GetTensorMutableData<float>()[0];

        // LOG DEBUG: Buka comment ini jika ingin melihat pergerakan skor di terminal
        // logger::info("WAKEWORD_DEBUG", "Skor: " + std::to_string(score) + " | RMS: " + std::to_string(rms));

        if (score > DETECT_THRESHOLD) {
            detection_count_++;
            // Opsional: print log untuk melihat proses debouncing
            logger::info("WAKEWORD_DEBUG", "Frame terdeteksi: " + std::to_string(detection_count_) + "/2");

            // Harus 2 frame berturut-turut tembus threshold
            if (detection_count_ >= 2) {
                logger::info("WAKEWORD", "Terdeteksi VALID! score=" + std::to_string(score));
                reset_buffer(); 
                return true;
            }
        } else {
            // Jika skor tiba-tiba turun (false positive sesaat), reset hitungan ke 0
            detection_count_ = 0;
        }

    } catch (const Ort::Exception& e) {
        logger::err("WAKEWORD", std::string("Error saat inferensi: ") + e.what());
    }

    return false;
}

void WakeWordDetector::reset_buffer() {
    std::fill(audio_buf_.begin(), audio_buf_.end(), 0.0f);
    buf_pos_ = 0;
    detection_count_ = 0;
}
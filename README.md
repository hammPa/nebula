# Nebula: Offline Voice Assistant

**Nebula** adalah asisten suara yang berjalan sepenuhnya secara offline. Dibangun menggunakan **C++** dan ditenagai oleh **Vosk API** serta **ONNX Runtime**, Nebula dirancang khusus untuk pengguna Linux (dioptimalkan untuk **Hyprland**) yang mengutamakan privasi, efisiensi daya, dan performa tinggi tanpa ketergantungan pada koneksi internet.

## Fitur Utama

- **Offline Recognition:** Pemrosesan suara 100% lokal tanpa mengirim data ke *cloud*.
- **C++ Performance:** Arsitektur yang ringan dengan manajemen memori yang dioptimalkan untuk penggunaan sistem jangka panjang.
- **Hyprland Integration:** Kontrol penuh terhadap *window manager* melalui perintah suara (pindah *workspace*, buka aplikasi, tutup jendela, dll).
- **Custom Command Mapping:** Pemetaan perintah suara ke *shell script* atau perintah terminal dengan mudah.

## Arsitektur Baru: Mode Dua Fase & Optimasi Memori
Nebula sekarang menggunakan arsitektur **Sistem Dua Fase (Two-Phase System)** untuk menjamin performa yang cepat tanpa menguras *resource* komputer:

1. **Fase Idle (Wake Word NN):** Menggunakan model Neural Network (ONNX) yang sangat ringan sebagai "penjaga gerbang". Sistem ini akan terus mendengarkan kata panggil tanpa membebani CPU.
2. **Fase Listening (Vosk ASR):** Mesin pengenal suara utama (Vosk) hanya akan mengambil alih tugas saat *wake word* terdeteksi, memastikan terjemahan perintah yang akurat.
3. **Smart Memory Management:** Nebula dilengkapi fitur *auto-unload*. Jika sistem diam selama lebih dari 30 detik, model Vosk (yang memakan RAM ~170MB) akan dilepas dari memori dan baru dimuat kembali saat dibutuhkan.
4. **Sistem Fallback:** Jika file model *wake word* gagal dimuat, Nebula akan otomatis beroperasi dalam mode aman (Vosk selalu aktif).

## Tech Stack

- **Language:** C++ (Standard 17/20)
- **Speech Engine:** [Vosk API](https://alphacephei.com/vosk/)
- **Neural Network Runtime:** [ONNX Runtime](https://onnxruntime.ai/) (untuk *wake word*)
- **Audio I/O:** PortAudio atau ALSA
- **System Control:** Shell integration (execv/system calls)

## Prasyarat

Pastikan sistem Anda memiliki dependensi berikut sebelum melakukan kompilasi:

- **Compiler:** GCC atau Clang yang mendukung C++17+.
- **Vosk SDK:** Library pengembangan Vosk.
- **ONNX Runtime:** Library `libonnxruntime.so` terinstal di sistem (misal di `/usr/local/lib`).
- **PortAudio:** Untuk menangkap input mikrofon secara real-time.
- **Hyprland:** (Opsional) Untuk fitur integrasi *window manager*.

## Instalasi & Setup

### 1. Klon Repositori
```bash
git clone [https://github.com/hammPa/nebula.git](https://github.com/hammPa/nebula.git)
cd nebula
```

### 2. Download Vosk
```bash
mkdir model
# Download model bahasa Inggris (Vosk Small)
wget [https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip](https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip)
unzip vosk-model-small-en-us-0.15.zip -d model/
```

### 3. Latih Model Wake Word ONNX dengan CNN
Pastikan file model nebula_wake_word.onnx sudah berada di direktori root proyek atau di dalam folder build nantinya agar sistem tidak memicu mode fallback. Atau gunakan contoh model pada repository ini.

### 4. Kompilasi
```bash
mkdir build
cd build
cmake ..
make
```

### 5. Eksekusi
```bash
# Pastikan berada di direktori yang terdapat config dan model (disarankan root)
./nebula commands.json
```
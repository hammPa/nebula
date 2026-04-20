# Nebula: Offline Voice Assistant

**Nebula** adalah asisten suara yang berjalan sepenuhnya secara offline. Dibangun menggunakan **C++** dan ditenagai oleh **Vosk API** serta **ONNX Runtime**, Nebula dirancang khusus untuk pengguna Linux (dioptimalkan untuk **Hyprland**) yang mengutamakan privasi, efisiensi daya, dan performa tinggi tanpa ketergantungan pada koneksi internet.

## Fitur Utama

- **Offline Recognition:** Pemrosesan suara 100% lokal tanpa mengirim data ke *cloud*.
- **C++ Performance:** Arsitektur yang ringan dengan manajemen memori yang dioptimalkan untuk penggunaan sistem jangka panjang.
- **Hyprland Integration:** Kontrol penuh terhadap *window manager* melalui perintah suara (pindah *workspace*, buka aplikasi, tutup jendela, dll).
- **Custom Command Mapping:** Pemetaan perintah suara ke *shell script* atau perintah terminal dengan mudah.

## Pembaruan

1. **WakeWord Optimation:** Menaikkan silence threshold dan memodularkan fungsi feed menjadi1. **WakeWord Optimization:** Memecah fungsi `feed()` menjadi 3 fungsi private terpisah berdasarkan tanggung jawab.

   **Sebelum:** Semua logika (tulis ring buffer, DC removal, inferensi ONNX, debounce) berada di dalam satu fungsi `feed()`.

   **Sesudah:**
   - `check_energy()` — menulis ring buffer, stride gate, dan cek RMS chunk 500ms
   - `prepare_window()` — ekstraksi window 1.5 detik, DC removal, dan cek RMS penuh
   - `run_inference()` — inferensi ONNX dan debounce 2 frame berturut-turut.
2. **NebulaEngine Refactor:** Memecah logika `run()` menjadi method-method yang lebih terfokus.

   **Sebelum:** Loop utama `run()` menangani stream switching, routing phase, dan counter management secara bersamaan dalam satu blok.

   **Sesudah:**
   - `process_audio()` — routing audio ke fase yang tepat (idle/listening) berdasarkan state dan ketersediaan wake word
   - `current_stream_mode()` — menentukan `StreamMode` yang dibutuhkan berdasarkan state saat ini
   - `run()` — hanya bertanggung jawab atas loop utama, baca buffer, dan memanggil method di atas
3. **Pipeline Fusion: MFCC + CRNN → nebula_full.onnx**
   Sebelum: Pipeline inferensi terbagi dua, preprocessing MFCC dilakukan manual di C++ (DC removal, normalisasi per-frekuensi), lalu hasilnya dikirim ke model CRNN sebagai tensor [1, 1, 40, 148]. Ini menyebabkan mismatch karena nebula_full.onnx sudah mengandung MFCC preprocessor di dalamnya, sehingga MFCC dihitung dua kali.
   Sesudah: wakeword.cpp hanya melakukan konversi int16 → - float / 32768.0f, tanpa normalisasi manual
   - Input tensor berubah dari [1, 1, 40, 148] menjadi [1, 24000] (raw float audio)
   - Input name diperbarui dari "input" menjadi "mfcc_pcm_audio" sesuai nama node di fused model
   - Stride inferensi diperkecil dari 8000 → 4000 sampel (500ms → 250ms) agar wake word pendek tidak luput antar window
   - Verifikasi nama input/output ONNX ditambahkan saat init() untuk memudahkan debugging

## Tech Stack

- **Language:** C++ (Standard 17/20)
- **Speech Engine:** [Vosk API](https://alphacephei.com/vosk/)
- **Neural Network Runtime:** [ONNX Runtime](https://onnxruntime.ai/) (single merged session: MFCC preprocessor + CRNN wake word, INT8 quantized)
- **Audio Pipeline:** SoX (Sound eXchange)
- **Audio Server:** PipeWire / PulseAudio
- **System Control:** Shell integration (execv/system calls)
- **OS Target:** Linux Mint Hyprland (Full Feature), ZorinOs (Partial Feature)

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

### 3. Latih Model Wake Word ONNX
```bash
Pastikan file model nebula_wake_word.onnx sudah berada di direktori root proyek agar sistem tidak memicu mode fallback.
Model `nebula_full.onnx` adalah hasil penggabungan dua graph ONNX:
- `mfcc_preprocessor.onnx` — preprocessing audio ke fitur MFCC
- `nebula_wake_word_int8.onnx` — CRNN classifier (INT8 quantized)
Atau gunakan `nebula_full.onnx` yang sudah tersedia di repositori.
```

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
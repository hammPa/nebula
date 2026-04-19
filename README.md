# Nebula: Offline Voice Assistant

**Nebula** adalah asisten suara yang berjalan sepenuhnya secara offline. Dibangun menggunakan **C++** dan ditenagai oleh **Vosk API** serta **ONNX Runtime**, Nebula dirancang khusus untuk pengguna Linux (dioptimalkan untuk **Hyprland**) yang mengutamakan privasi, efisiensi daya, dan performa tinggi tanpa ketergantungan pada koneksi internet.

## Fitur Utama

- **Offline Recognition:** Pemrosesan suara 100% lokal tanpa mengirim data ke *cloud*.
- **C++ Performance:** Arsitektur yang ringan dengan manajemen memori yang dioptimalkan untuk penggunaan sistem jangka panjang.
- **Hyprland Integration:** Kontrol penuh terhadap *window manager* melalui perintah suara (pindah *workspace*, buka aplikasi, tutup jendela, dll).
- **Custom Command Mapping:** Pemetaan perintah suara ke *shell script* atau perintah terminal dengan mudah.

## Pembaruan

1. **Fork Feedback Audio Optimation:** Mengubah implementasi feedback menjadi 1 fork yang awalnya 2 fork.
2. **Capture Config Optimation:** Menjadikan perintah `arecord` dan `sox` sebagai `constexpr` agar tidak ada alokasi heap saat `open_stream()` dipanggil.
3. **Nebula Engine Modularization:** Memecah `nebula_engine` menjadi 3 file terpisah berdasarkan tanggung jawab.

   **Sebelum:** Semua logika (Vosk lifecycle, stream switching, timeout, loop utama) ada di dalam satu class `NebulaEngine`.

   **Sesudah:**
   - `vosk_manager.hpp/.cpp` — mengenkapsulasi semua urusan Vosk: load/unload model, build grammar, accept waveform, dan return hasil JSON. Termasuk guard: jika `rec` gagal dibuat saat `load()`, `model` otomatis di-reset.
   - `stream_controller.hpp/.cpp` — mengenkapsulasi buka/tutup stream `arecord`/`sox`, switch mode IDLE↔LISTENING, dan baca audio.
   - `nebula_engine.hpp/.cpp` — kini hanya berperan sebagai orkestrator: memanggil `VoskManager` dan `StreamController`, dispatch ke fase yang sesuai, dan menangani timeout lewat `check_timeouts()`.

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
# Nebula: Offline Voice Assistant

**Nebula** adalah asisten suara yang berjalan sepenuhnya secara offline. Dibangun menggunakan **C++** dan ditenagai oleh **Vosk API** serta **ONNX Runtime**, Nebula dirancang khusus untuk pengguna Linux (dioptimalkan untuk **Hyprland**) yang mengutamakan privasi, efisiensi daya, dan performa tinggi tanpa ketergantungan pada koneksi internet.

## Fitur Utama

- **Offline Recognition:** Pemrosesan suara 100% lokal tanpa mengirim data ke *cloud*.
- **C++ Performance:** Arsitektur yang ringan dengan manajemen memori yang dioptimalkan untuk penggunaan sistem jangka panjang.
- **Hyprland Integration:** Kontrol penuh terhadap *window manager* melalui perintah suara (pindah *workspace*, buka aplikasi, tutup jendela, dll).
- **Custom Command Mapping:** Pemetaan perintah suara ke *shell script* atau perintah terminal dengan mudah.

## Pembaruan

1. **Nebula kini tidak lagi menggunakan akses hardware eksklusif (plughw):** Dengan integrasi -D default, Nebula bisa berjalan bersamaan dengan Discord, Spotify, atau Zoom. Anda bisa memberikan perintah suara tanpa harus mematikan musik atau keluar dari voice chat.
2. **Menggunakan algoritma EMA:** Nebula menggunakan algoritma Exponential Moving Average (EMA) untuk mempelajari tingkat kebisingan ruangan Anda secara real-time.
3. **Highpass/Lowpass Filter:** Menghapus frekuensi rendah (deru mesin) dan frekuensi tinggi (noise elektrik).
4. **Ultra-Efficient INT8 Inference:** Model wake-word telah dikuantisasi ke presisi INT8, mengurangi ukuran model dan beban CPU secara drastis, sehingga sangat aman untuk berjalan secara terus-menerus (always-on) di background.


## Tech Stack

- **Language:** C++ (Standard 17/20)
- **Speech Engine:** [Vosk API](https://alphacephei.com/vosk/)
- **Neural Network Runtime:** [ONNX Runtime](https://onnxruntime.ai/) (untuk *wake word*)
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
# Nebula: Offline Voice Assistant

**Nebula** adalah asisten suara yang berjalan sepenuhnya secara offline. Dibangun menggunakan **C++** dan ditenagai oleh **Vosk API**, Nebula dirancang baru untuk pengguna Linux (dioptimalkan untuk **Hyprland**) yang mengutamakan privasi dan performa tinggi tanpa ketergantungan pada koneksi internet.

## Fitur Utama

- **Offline Recognition:** Menggunakan Vosk API untuk pemrosesan suara lokal tanpa mengirim data ke cloud.
- **C++ Performance:** Arsitektur yang ringan dengan manajemen memori yang dioptimalkan untuk penggunaan jangka panjang.
- **Hyprland Integration:** Kontrol penuh terhadap window manager melalui perintah suara (pindah workspace, buka aplikasi, dll).
- **Wake-word Detection:** Siap mendengarkan perintah hanya saat dipicu (Always-listening mode).
- **Custom Command Mapping:** Pemetaan perintah suara ke shell script atau perintah terminal dengan mudah.

## Tech Stack

- **Language:** C++ (Standard 17/20)
- **Speech Engine:** [Vosk API](https://alphacephei.com/vosk/)
- **Audio I/O:** PortAudio atau ALSA (tergantung konfigurasi)
- **System Control:** Shell integration (execv/system calls)

## Prasyarat

Pastikan sistem Anda memiliki dependensi berikut:

- **Compiler:** GCC atau Clang yang mendukung C++17+.
- **Vosk SDK:** Library pengembangan Vosk.
- **Vosk Model:** Model bahasa (misalnya: `vosk-model-small-en-us-0.15`).
- **PortAudio:** Untuk menangkap input mikrofon secara real-time.
- **Hyprland:** (Opsional) Untuk fitur integrasi window manager.

## Instalasi & Setup

### 1. Klon Repositori
```bash
git clone [https://github.com/hammPa/nebula.git](https://github.com/hammPa/nebula.git)
cd nebula
```
### 2. Download Model (https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip)
```bash
mkdir model
# Contoh untuk model bahasa Inggris kecil
wget [https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip]
unzip vosk-model-small-en-us-0.15.zip -d model/
```

### 3. Kompilasi
```bash
mkdir build
cd build
cmake ..
make
```

### 4. Eksekusi
```bash
./nebula commands.json
```
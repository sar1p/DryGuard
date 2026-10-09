# Setup, build, dan upload

## PlatformIO pada Windows

Prasyarat: Python 3.12, Git, serta akses internet untuk mengambil dependency. Jalankan dari root repository pada PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
Copy-Item firmware\jemuran_otomatis\src\Secrets.example.h firmware\jemuran_otomatis\src\Secrets.h
```

Isi `Secrets.h` sebelum membuat firmware untuk perangkat:

```cpp
#define BLYNK_TEMPLATE_ID "TMPL_MILIK_ANDA"
#define BLYNK_TEMPLATE_NAME "Jemuran Otomatis"
#define BLYNK_AUTH_TOKEN "TOKEN_PERANGKAT_ANDA"
#define WIFI_SSID "NAMA_WIFI_ANDA"
#define WIFI_PASSWORD "PASSWORD_WIFI_ANDA"
```

File ini harus tetap lokal. `git check-ignore firmware/jemuran_otomatis/src/Secrets.h` memastikan aturan ignore aktif. Bila file belum disiapkan, project tetap dapat dikompilasi dengan contoh kredensial kosong; Wi-Fi/Blynk dinonaktifkan saat runtime.

Build terlebih dahulu:

```powershell
.\.venv\Scripts\python.exe -m platformio run -e esp32dev
```

Hasil yang diharapkan adalah `SUCCESS`, dengan firmware pada `.pio/build/esp32dev/`. Upload hanya setelah board, sambungan, dan posisi awal telah diperiksa. Ganti `COM5` dengan port perangkat yang benar:

```powershell
.\.venv\Scripts\python.exe -m platformio device list
.\.venv\Scripts\python.exe -m platformio run -e esp32dev -t upload --upload-port COM5
.\.venv\Scripts\python.exe -m platformio device monitor --port COM5 --baud 115200
```

Target `esp32dev` ditujukan ke ESP32 klasik dua core. Sebutan "ESP32D" belum menunjukkan nomor board lengkap; cocokan tipe board sebelum upload. Perangkat ESP32 dengan core/pin berbeda membutuhkan konfigurasi tersendiri.

## Arduino IDE

1. Pasang board package **esp32 by Espressif Systems**; build PlatformIO dipatok pada core Arduino ESP32 2.0.17. Untuk pemeriksaan pertama, gunakan versi yang sama.
2. Pasang library **Blynk 1.3.2** dan **AccelStepper 1.64.0**.
3. Buka `firmware/jemuran_otomatis/jemuran_otomatis.ino`. Arduino akan mengompilasi file `.cpp` di `src/` secara rekursif.
4. Siapkan `src/Secrets.h`, pilih ESP32 Dev Module dan port yang benar, lalu Verify sebelum Upload.

Sketch `.ino` sengaja singkat karena `setup()` dan `loop()` berada di `src/main.cpp`. Hindari menyalin dua sketch legacy ke dalam folder sketch baru, karena masing-masing mempunyai entry point sendiri.

## Dashboard Blynk

Buat datastream berikut pada template perangkat dan hubungkan widget ke virtual pin yang sesuai:

| Virtual pin | Jenis | Fungsi / widget |
|---|---|---|
| V0 | Integer, 0–1 | Power, switch |
| V1 | Integer, 0–1 | Mode automatic, switch |
| V2 | Integer, 0–1 | Mode manual, switch |
| V3 | Integer, 0–1 | Masuk, tombol push |
| V4 | Integer, 0–1 | Keluar, tombol push |
| V5 | String | Cuaca: HUJAN/KERING/OFF/MEMBACA |
| V6 | String | Cahaya: GELAP/TERANG/-/MEMBACA |
| V7 | String | Posisi/gerak: DALAM/LUAR/TENGAH/KELUAR/ MASUK/SISTEM MATI |

State perangkat menjadi acuan saat reconnect. Pada boot pertama, migrasi, atau checkpoint tidak valid, V0 dipublikasikan OFF. Power ON mengaktifkan mode yang tersimpan; jika mode Idle, pilih V1 atau V2/V3/V4.

V3/V4 memilih mode manual secara otomatis. Perintah manual tetap bisa mengeluarkan jemuran ketika hujan; pilih V1 untuk mengaktifkan kembali perlindungan otomatis berdasarkan sensor.

## Kalibrasi dan sambungan

Pin dan nilai awal tersedia di `src/config/HardwareConfig.h`. Pantau nilai ADC pada log `[SENSOR]`, lalu cocokkan ambang dengan kondisi sensor yang nyata. Polaritas dari sketch lama dipertahankan: nilai hujan kecil berarti hujan, nilai cahaya besar berarti gelap.

ULN2003/motor menggunakan suplai 5 V sesuai spesifikasi perangkat, dengan ground bersama ESP32. Tegangan pada input ADC ESP32 harus sesuai batas perangkat ESP32; keluaran analog modul sensor tidak boleh diasumsikan sama dengan tegangan suplai modul. Pengujian catu daya dan mekanisme dijelaskan dalam panduan troubleshooting.

## Rujukan

- [PlatformIO — ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [Arduino — sketch specification](https://docs.arduino.cc/arduino-cli/sketch-specification/)
- [Espressif — Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)
- [AccelStepper — class reference](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html)
- [Blynk — batas dan rekomendasi pengiriman data](https://docs.blynk.io/en/blynk-library-firmware-api/limitations-and-recommendations)

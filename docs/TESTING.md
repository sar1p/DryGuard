# Pengujian

## Pengujian otomatis

```powershell
python tools/run_tests.py
```

Runner membutuhkan GCC atau Clang pada PATH. Bila compiler berada di lokasi lain, atur `CXX` ke path executable compiler. Tes mengompilasi **kode C++ yang sama dengan firmware**, bukan implementasi ulang dalam Python.

Tes mencakup startup OFF, cuaca otomatis, hysteresis, ADC di luar rentang, pergantian mode, override manual, penolakan perintah saat OFF, pause/resume saat reboot, target penuh `4000`, checkpoint posisi hold, record korup, dan debounce saat `millis()` melingkar.

GitHub Actions menjalankan dua pemeriksaan:

1. **Core C++ tests**: kompilasi dengan C++11, warning sebagai error, dan menjalankan tes regresi.
2. **ESP32 firmware build**: build PlatformIO menggunakan dependency yang dipatok, lalu memeriksa konfigurasi tanpa jaringan dan konfigurasi jaringan dummy melalui header lokal.

`.\.venv\Scripts\python.exe tools/check_configured_build.py` memeriksa varian jaringan aktif dengan kredensial dummy pada Windows setelah setup PlatformIO. Tool ini menolak menimpa `Secrets.h` yang sudah ada dan menghapus header dummy buatannya setelah build. Binary yang dihasilkan memakai data dummy; build ulang dengan kredensial perangkat sendiri sebelum upload.

Runner native tidak menyimulasikan library Blynk, RTOS scheduler, flash ESP32, suplai daya, atau mekanisme motor. Build firmware memeriksa kompatibilitas compile/link; kedua pemeriksaan tersebut belum membuktikan perilaku hardware.

## Matriks uji perangkat

Seluruh baris berikut memerlukan pengujian fisik; jangan menandainya lulus hanya karena CI berhasil.

| Skenario | Hasil yang diperiksa |
|---|---|
| Boot pertama, jemuran berada di dalam | Estimasi 0 dan sistem OFF; V0 ON lalu V1 menjalankan automatic |
| Kering dan terang | Target luar, gerak selesai tanpa stall atau reset |
| Hujan saat bergerak keluar | Target berubah ke dalam; tidak terus melewati batas mekanik |
| Gelap | Automatic mengarah ke dalam |
| Nilai sensor dekat ambang | Tidak berganti arah berulang pada noise kecil |
| Mode manual dan V3/V4 | Mode benar, tombol kembali nol, arah sesuai wiring |
| V0 OFF di tengah gerak | Gerak berhenti, posisi disimpan setelah hold diakui |
| V0 ON setelah pause manual | Tujuan sebelumnya dilanjutkan, estimasi cocok dengan posisi nyata |
| Reboot di tengah gerak | Checkpoint dibaca; ukur selisih estimasi terhadap posisi fisik |
| Wi-Fi/Blynk terputus lalu tersambung | Logika lokal tetap berjalan, dashboard diperbarui saat reconnect |
| Beban ringan hingga beban penggunaan | Catat reset reason, suplai, suhu driver/motor, dan langkah terlewat |

Kenaikan beban dilakukan bertahap sesuai kemampuan mekanisme. Bila estimasi berbeda dari posisi nyata, hentikan pengujian dan cocokkan kembali posisi sebelum melanjutkan. Limit switch/encoder dan prosedur homing belum tersedia di proyek ini.

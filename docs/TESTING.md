# Pengujian

## Pengujian otomatis

```powershell
python tools/run_tests.py
```

Runner membutuhkan GCC atau Clang pada PATH. Bila compiler berada di lokasi lain, atur `CXX` ke path executable compiler. Tes mengompilasi **kode C++ yang sama dengan firmware**, bukan implementasi ulang dalam Python.

Tes core mencakup startup/restart OFF, cuaca otomatis, hysteresis termasuk pemulihan setelah ADC invalid, pergantian mode, override manual, penolakan perintah saat OFF, pause/resume setelah enable, target penuh `4000`, checkpoint posisi hold, record korup, dan debounce saat `millis()` melingkar.

Tes gateway mengompilasi `BlynkGateway.cpp` yang sama dengan firmware bersama controller asli. Stub API Arduino, Wi-Fi, dan Blynk mencatat nilai yang dipublikasikan untuk memeriksa koreksi switch yang ditolak saat OFF, interval pengiriman, input callback, dan refresh setelah reconnect. Salinan sumber dan header dummy dibuat hanya di `build/native/gateway/`; runner tidak membaca atau menimpa `Secrets.h` pribadi.

GitHub Actions menjalankan dua pemeriksaan:

1. **Native C++ tests**: kompilasi dengan C++11, warning sebagai error, dan menjalankan tes core serta gateway.
2. **ESP32 firmware build**: build PlatformIO menggunakan dependency yang dipatok, lalu memeriksa konfigurasi tanpa jaringan dan konfigurasi jaringan dummy melalui header lokal.

`.\.venv\Scripts\python.exe tools/check_configured_build.py` memeriksa varian jaringan aktif dengan kredensial dummy pada Windows setelah setup PlatformIO. Tool ini menolak menimpa `Secrets.h` yang sudah ada dan menghapus header dummy buatannya setelah build. Binary yang dihasilkan memakai data dummy; build ulang dengan kredensial perangkat sendiri sebelum upload.

Stub gateway memeriksa logika firmware terhadap API pengganti yang minimal; perilaku library/server Blynk asli, RTOS scheduler, flash ESP32, suplai daya, dan mekanisme motor tetap memerlukan pengujian tersendiri. Build firmware memeriksa kompatibilitas compile/link; kedua pemeriksaan tersebut belum membuktikan perilaku hardware.

## Matriks uji perangkat

Seluruh baris berikut memerlukan pengujian fisik; jangan menandainya lulus hanya karena CI berhasil.

| Skenario | Hasil yang diperiksa |
|---|---|
| Boot pertama, jemuran berada di dalam | Estimasi 0 dan sistem OFF; V0 ON lalu V1 menjalankan automatic |
| Kering dan terang | Target luar, gerak selesai tanpa stall atau reset |
| Hujan saat bergerak keluar | Target berubah ke dalam; tidak terus melewati batas mekanik |
| Gelap | Automatic mengarah ke dalam |
| Nilai sensor dekat ambang | Tidak berganti arah berulang pada noise kecil |
| V1/V2 ON saat V0 OFF | Motor tetap hold dan switch mode kembali OFF pada jadwal dashboard berikutnya |
| Mode manual dan V3/V4 | Mode benar, tombol kembali nol, arah sesuai wiring |
| V0 OFF di tengah gerak | Gerak berhenti, posisi disimpan setelah hold diakui |
| V0 ON setelah pause manual | Tujuan sebelumnya dilanjutkan, estimasi cocok dengan posisi nyata |
| Reboot di tengah gerak | Checkpoint dibaca dan sistem OFF; ukur selisih estimasi terhadap posisi fisik sebelum V0 ON |
| Wi-Fi/Blynk terputus lalu tersambung | Logika lokal tetap berjalan, dashboard diperbarui saat reconnect |
| Beban ringan hingga beban penggunaan | Catat reset reason, suplai, suhu driver/motor, dan langkah terlewat |

Kenaikan beban dilakukan bertahap sesuai kemampuan mekanisme. Bila estimasi berbeda dari posisi nyata, hentikan pengujian dan cocokkan kembali posisi sebelum melanjutkan. Limit switch/encoder dan prosedur homing belum tersedia di proyek ini.

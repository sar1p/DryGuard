# Menelusuri restart dan gangguan gerak

## Ambil bukti dari Serial Monitor

Buka Serial Monitor pada **115200 baud** sebelum mengulang kondisi yang menimbulkan restart. Firmware baru menampilkan format berikut; ini contoh format, bukan hasil pengukuran alat:

```text
[BOOT] Jemuran modular | reset=<ALASAN> (<KODE>) | SDK=<VERSI>
[DIAG] heap_free=<BYTES> bytes | heap_min=<BYTES> bytes
[STATE] estimate=<LANGKAH> target=<LANGKAH> power=<0/1> mode=<MODE>
```

Simpan log sebelum restart, termasuk pesan panic/backtrace bila ada, dan baris boot sesudahnya. Alasan reset dilaporkan melalui `esp_reset_reason()` menurut [dokumentasi Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html#reset-reason).

| Alasan | Arti awal dan pemeriksaan berikutnya |
|---|---|
| `BROWNOUT` | Detektor brownout melaporkan tegangan rendah; periksa suplai, kabel, ground, dan tegangan saat motor berbeban |
| `TASK_WATCHDOG` / `INTERRUPT_WATCHDOG` | Watchdog terpicu; ambil backtrace dan periksa fungsi/task yang menahan eksekusi |
| `PANIC` | Ambil seluruh pesan error dan backtrace; cocokkan dengan ELF dari build yang benar-benar diunggah |
| `POWER_ON` / `EXTERNAL_RESET` | Periksa hilangnya daya, konektor, atau sinyal reset; alasan ini belum menentukan penyebab lengkap |
| `SOFTWARE_RESET` / `UNKNOWN` | Periksa log sebelum reset; kode baru tidak menjadwalkan reboot untuk reconnect Wi-Fi |

Log alasan reset membantu penyelidikan, tetapi bukan pengukuran tegangan atau bukti tunggal bahwa komponen tertentu rusak.

## Motor stepper 5 V dan ULN2003

Konfigurasi pin serta kecepatan awal mengikuti proyek lama. Bila motor kehilangan langkah ketika diberi beban, uji kecepatan/percepatan lebih rendah pada `HardwareConfig.h`, periksa hambatan mekanik, dan cocokkan estimasi posisi terhadap posisi fisik. Pencacahan langkah software tidak mendeteksi stall.

`kMotorMaxSpeed` adalah batas kecepatan yang dikonfigurasi. Kecepatan aktual juga bergantung pada frekuensi pemanggilan `run()` dan penjadwalan task; ukur waktu perjalanan pada perangkat sebelum menganggap nilai itu tercapai.

Pastikan motor/driver memperoleh suplai 5 V sesuai spesifikasinya; motor tidak disuplai dari GPIO ESP32. ESP32 dan driver memerlukan ground bersama. Bandingkan kondisi tanpa gerak motor, gerak tanpa beban, dan beban penggunaan, lalu ukur tegangan saat gejala muncul. Mengubah kode tidak dapat memperbaiki suplai yang turun atau mekanisme yang macet.

## Dashboard tidak tersambung

- Periksa apakah `src/Secrets.h` tersedia dan token/SSID telah diisi. Contoh kosong sengaja menonaktifkan jaringan.
- Cocokkan template ID, token perangkat, virtual pin, serta jaringan Wi-Fi dengan konfigurasi Blynk.
- Pastikan Serial menampilkan koneksi Wi-Fi lalu Blynk; retry Blynk berjalan setiap lima detik dengan timeout terbatas.
- Sesudah reconnect, state perangkat dikirim ulang. State power OFF tidak digantikan oleh switch cloud yang sebelumnya ON.

## Posisi atau resume tidak sesuai

Checkpoint bergerak tidak dibuat pada setiap langkah. Pemadaman mendadak dapat menghilangkan perubahan sejak checkpoint terakhir; pergeseran mekanik saat mati dan langkah terlewat juga tidak terdeteksi. Cocokkan posisi secara fisik, lalu perbaiki kalibrasi atau tambahkan mekanisme homing pada pengembangan berikutnya.

Pada setiap boot/restart, firmware menunggu V0 ON; baca estimasi `[STATE]` dan cocokkan dengan posisi fisik terlebih dahulu. Namespace lama tetap tersimpan agar dapat ditelusuri, tetapi rollback firmware lama mungkin membaca checkpoint lamanya sendiri yang sudah tidak mengikuti gerak pada firmware baru.

# Jemuran Otomatis — ESP32 dan Blynk

Firmware jemuran otomatis dengan sensor hujan, sensor cahaya, motor stepper 5 V, dan driver ULN2003. Mode otomatis memasukkan jemuran ketika hujan atau gelap, lalu mengeluarkannya ketika kering dan terang. Mode manual menyediakan perintah masuk/keluar melalui Blynk.

Versi ini memisahkan kode berdasarkan tanggung jawab, memakai antrean FreeRTOS untuk kontrol motor, menyimpan checkpoint melalui Preferences/NVS, dan menyediakan build serta pengujian otomatis. Dua sketch sebelumnya tersedia di [`legacy/`](legacy/README.md) sebagai rujukan.

## Perangkat yang dituju

- ESP32 klasik dengan dua core; target build `esp32dev` / ESP32 Dev Module. Sebutan perangkat pengguna adalah "ESP32D"; nomor board lengkap belum dikonfirmasi.
- Stepper empat fase 5 V dengan driver ULN2003, sesuai perangkat proyek sebelumnya.
- Sensor hujan analog dan sensor cahaya analog, masing-masing dibaca sebagai ADC 12 bit.
- Blynk dengan virtual pin V0–V7.

| Perangkat | GPIO |
|---|---:|
| ULN2003 IN1 | 13 |
| ULN2003 IN2 | 12 |
| ULN2003 IN3 | 14 |
| ULN2003 IN4 | 27 |
| Sensor hujan — analog | 34 |
| Sensor cahaya — analog | 35 |

Urutan constructor motor tetap **IN1, IN3, IN2, IN4**, sesuai sketch awal. Posisi dalam `0`, posisi luar `4000`, ambang hujan `< 2800`, dan ambang gelap `> 3400`. Konfigurasi ini adalah nilai awal dari kode proyek, bukan hasil kalibrasi ulang mekanisme.

## Struktur proyek

```text
firmware/jemuran_otomatis/
  jemuran_otomatis.ino      # entry sketch Arduino IDE
  src/
    main.cpp               # setup/loop singkat
    Secrets.example.h      # contoh konfigurasi jaringan
    config/                # pin, kalibrasi, interval, ukuran task
    core/                  # logika murni C++ dan format checkpoint
    app/                   # orkestrasi aplikasi
    motor/                 # satu task pemilik AccelStepper
    sensors/               # pembacaan ADC dan filter
    storage/               # Preferences/NVS dan migrasi
    iot/                   # Wi-Fi, Blynk, dashboard
    diagnostics/           # alasan reset dan kondisi heap
tests/                     # tes regresi core C++
tools/                     # runner pengujian
docs/                      # setup, arsitektur, pengujian, troubleshooting
legacy/                    # dua program asli
.github/workflows/ci.yml    # build ESP32 dan tes pada GitHub Actions
platformio.ini             # versi platform/library yang dipatok
```

## Mulai menggunakan

1. Salin `firmware/jemuran_otomatis/src/Secrets.example.h` menjadi `Secrets.h` di folder yang sama, lalu isi template Blynk, token, SSID, dan password Wi-Fi. `Secrets.h` diabaikan Git.
2. Siapkan dashboard sesuai tabel [virtual pin Blynk](docs/SETUP.md#dashboard-blynk).
3. Build menggunakan PlatformIO atau buka `firmware/jemuran_otomatis/jemuran_otomatis.ino` melalui Arduino IDE. Tidak perlu mengganti ekstensi file `.cpp`.
4. Pada **setiap boot/restart**, sistem mulai **OFF** sambil mempertahankan tujuan yang tersimpan. Cocokkan posisi fisik dengan estimasi yang ditampilkan Serial Monitor sebelum menyalakan V0.
5. Pilih otomatis melalui V1, atau manual melalui V2/V3/V4.

Panduan lengkap: [setup dan build](docs/SETUP.md), [arsitektur](docs/ARCHITECTURE.md), [pengujian](docs/TESTING.md), dan [penelusuran restart](docs/TROUBLESHOOTING.md).

## Perbaikan utama

- Objek motor diakses hanya oleh task motor; task aplikasi mengirim target/hold melalui queue dan membaca snapshot.
- Power OFF menghentikan gerak sambil mempertahankan tujuan yang tertunda. Tujuan resume tidak ditimpa kembali oleh proses boot; setelah restart, perjalanan dilanjutkan melalui V0 ON setelah posisi diperiksa.
- Mode automatic/manual menggunakan satu enum, sehingga keduanya tidak aktif bersamaan.
- Pembacaan median dan hysteresis mengurangi pergantian arah akibat noise di sekitar ambang sensor.
- Penyimpanan memakai satu record dengan schema, checksum, dan validasi rentang. Checkpoint bergerak dibuat paling cepat setiap lima detik, selain perubahan perintah/status dan akhir gerak.
- Dashboard dikirim saat nilai berubah, dengan interval satu detik. State perangkat dipublikasikan kembali saat reconnect.
- Boot mencetak alasan reset; diagnostik heap dan sensor dicetak setiap menit.

## Batas pengujian dan posisi

Firmware menghitung langkah; proyek saat ini tidak mempunyai input limit switch atau encoder di kode. Estimasi posisi dapat meleset akibat langkah terlewat, gerakan saat listrik mati, atau hilangnya langkah sejak checkpoint terakhir. Resume tidak menggantikan homing fisik.

Build dan tes logika tidak membuktikan bahwa restart saat beban penuh telah hilang. Pengujian catu daya, driver, mekanisme, dan firmware pada perangkat mengikuti [panduan pengujian](docs/TESTING.md). CI memeriksa konfigurasi kosong dan jaringan dummy. Artefak CI memakai kredensial dummy; buat build sendiri dengan `Secrets.h` untuk koneksi Blynk.

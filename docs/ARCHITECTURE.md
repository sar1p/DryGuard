# Arsitektur dan aliran data

## Pemilik masing-masing komponen

| Modul | Tanggung jawab | Pemilik saat runtime |
|---|---|---|
| `app/Application` | Orkestrasi, penerimaan perintah, penjadwalan checkpoint | task aplikasi, core 0 |
| `core/Controller` | Power, mode, tujuan motor, pause/resume | task aplikasi |
| `core/SensorPolicy` | Ambang dan hysteresis sensor | task aplikasi |
| `core/StateCodec` | Encode/decode, schema, checksum, validasi record | task aplikasi / tes native |
| `motor/MotorService` | AccelStepper, pembatasan target, snapshot posisi | task motor, core 1 |
| `sensors/SensorService` | Median lima sampel ADC, pembacaan 200 ms | task aplikasi |
| `storage/StateStore` | Membaca/mengubah Preferences/NVS | task aplikasi |
| `iot/BlynkGateway` | Wi-Fi, reconnect, callback Blynk, virtualWrite | task aplikasi |
| `diagnostics/Diagnostics` | Alasan reset dan statistik heap | setup / task aplikasi |

Setelah inisialisasi, hanya task motor yang memanggil metode objek AccelStepper. Aplikasi mengirim `MotionRequest` melalui queue satu slot: pesan terbaru berisi seluruh tujuan yang berlaku, sehingga perintah lama tidak menumpuk. Snapshot dibagikan melalui queue terpisah, tanpa mengakses objek motor dari core lain.

Queue menyimpan kondisi akhir yang diminta. Perintah yang segera digantikan sebelum diterima task motor dapat dilewati; perubahan OFF lalu ON bukan dua event yang wajib dijalankan berurutan.

Setiap perintah motor membawa nomor urut. Aplikasi menunggu snapshot yang mengakui nomor tersebut sebelum membuat checkpoint perubahan perintah. Dengan demikian, Power OFF menyimpan posisi setelah task motor menerima hold, bukan posisi yang dibaca sebelum motor berhenti.

## Power dan mode

- `Automatic`: hujan **atau** gelap mengarah ke dalam; kering **dan** terang mengarah ke luar. Pembacaan sensor tidak valid mengarah ke dalam.
- `Manual`: V3/V4 memilih tujuan secara eksplisit. Perintah manual mengabaikan aturan cuaca sampai mode automatic dipilih kembali.
- `Idle`: tidak ada gerak yang diminta.
- Power OFF terpisah dari mode. Mode dan tujuan yang tertunda dipertahankan, tetapi permintaan motor menjadi hold. Ketika power kembali ON dalam mode automatic, cuaca terkini dievaluasi terlebih dahulu.
- Setiap boot mulai OFF, termasuk ketika record menyimpan power ON. Mode/tujuan tetap dipertahankan, tetapi V0 harus dinyalakan kembali setelah posisi fisik diperiksa. Tanpa feedback posisi, firmware tidak dapat membuktikan lokasi motor sesudah reset mendadak.
- V0 mengaktifkan/menonaktifkan logika gerak; hold tidak memutus suplai driver atau otomatis melepas arus holding coil.
- Memilih mode manual melalui V2 membuat motor hold. Mematikan mode yang aktif juga membuat hold.

Hold diterapkan pada posisi aktual di task motor. Angka snapshot yang mungkin sudah terlambat beberapa langkah tidak digunakan sebagai tujuan untuk menghentikan motor.

## Penyimpanan dan migrasi

Namespace baru adalah `jemuran_v3`, dengan satu key `state` berisi record 32 byte. Record menyimpan posisi, target, mode, power, dan niat gerak; versi format serta checksum divalidasi sebelum digunakan. Ini menghindari record campuran dari penulisan banyak key terpisah, tetapi ketepatan posisi tetap bergantung pada estimasi langkah dan interval checkpoint.

Data valid dari `jemuran_v2` atau `jemuran` dapat diimpor saat record baru belum tersedia. Flag resume Preferences milik versi eksperimen ikut diperiksa. Flag EEPROM dari versi awal tidak diimpor. Namespace lama tidak dihapus. Hasil migrasi mulai OFF karena kedua sketch lama tidak menyimpan state power secara konsisten.

Record baru yang tidak valid menyebabkan startup OFF; data legacy tidak dipakai sebagai pengganti checkpoint baru yang korup. Pada boot pertama, estimasi awal adalah posisi dalam `0`; cocokkan kondisi mekanis sebelum menyalakan sistem.

Flash ditulis oleh task aplikasi saat state berubah, motor selesai, atau checkpoint bergerak memenuhi interval lima detik dan perubahan minimal 50 langkah. Record yang sama tidak ditulis ulang. Operasi flash ESP32 masih dapat memengaruhi timing sistem; pemisahan task bukan jaminan gerak tanpa jitter.

## Jaringan dan dashboard

Wi-Fi dicoba kembali setiap 10 detik, dan Blynk setiap lima detik dengan budget `connect(250)`. Tidak ada `delay(1000)` pada jalur reconnect aplikasi. Panggilan koneksi Blynk tetap dapat menunggu hingga timeout tersebut; task motor berjalan terpisah.

Semua virtual pin dipublikasikan dari satu lokasi, paling banyak delapan nilai aplikasi per interval satu detik, termasuk reset tombol. Reconnect menghapus cache sehingga dashboard kembali menampilkan state perangkat. Nilai V0 lama di cloud tidak otomatis mengaktifkan perangkat yang sudah OFF.

Callback power/mode menandai V0–V2 untuk dikirim ulang pada jadwal publikasi berikutnya, walaupun state lokal tidak berubah. Dengan demikian, perintah V1/V2 saat OFF yang ditolak controller dikoreksi menjadi nol pada dashboard. Flag koreksi tetap tertunda ketika belum tersambung atau interval satu detik belum lewat.

Pin, kecepatan, percepatan, interval, dan kalibrasi dapat diubah pada `src/config/HardwareConfig.h`. Perubahan harus disertai build dan pengujian yang sesuai.

# Alur perubahan kode

Buat perubahan pada branch sendiri, lalu kirim pull request agar kode dan hasil tes dapat diperiksa bersama:

```powershell
git switch main
git pull --ff-only
git switch -c feat/nama-perubahan
git status
```

Setelah mengedit, jalankan tes core dan build firmware sesuai `docs/TESTING.md`. Stage file yang memang berubah, cek diff, lalu commit dan push:

```powershell
git add firmware tests docs
git diff --cached
git commit -m "Describe the concrete change"
git push -u origin feat/nama-perubahan
```

Jelaskan masalah, perubahan perilaku, dan hasil pengujian pada pull request. Bedakan tes software dari tes fisik ESP32. Tunggu hasil GitHub Actions sebelum merge.

`Secrets.h`, `.pio`, `.venv`, hasil build, dan log lokal diabaikan Git. Simpan data jaringan hanya dalam konfigurasi lokal. Hindari mengubah isi `legacy/` saat memperbaiki firmware aktif; arsip tersebut menjadi pembanding terhadap versi sebelumnya.

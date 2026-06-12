# 💳 Smart Daily Finance Manager

Aplikasi manajemen keuangan harian berbasis web dengan backend C++ dan frontend HTML/CSS/JS murni. Dirancang untuk mencatat pemasukan, pengeluaran, dan melacak progress tabungan secara real-time.

## ✨ Fitur

- 📥 **Catat Pemasukan** — dengan sistem alokasi otomatis 60% kebutuhan / 40% tabungan, atau 100% ke tabungan
- 📤 **Catat Pengeluaran** — otomatis ambil dari saldo kebutuhan, fallback ke tabungan jika kurang
- 🎯 **Target Tabungan** — atur target dan pantau progress dengan visual lingkaran
- 📊 **Statistik** — pie chart & bar chart distribusi transaksi
- 🔍 **Cari Transaksi** — pencarian real-time via modal
- 💾 **Persistensi Data** — tersimpan di file lokal (`riwayat_transaksi.txt`)
- 📱 **Responsive** — tampilan menyesuaikan mobile & desktop

## 🛠️ Teknologi

| Layer | Teknologi |
|-------|-----------|
| Frontend | HTML5, CSS3, Vanilla JavaScript |
| Backend | C++17 + [cpp-httplib](https://github.com/yhirose/cpp-httplib) |
| Charts | Canvas API (custom, tanpa library eksternal) |

## 📁 Struktur File
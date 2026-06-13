//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

// ============================================================
// [4] NAMESPACE — semua logika utama dibungkus namespace Finance
// ============================================================
namespace Finance {

// ============================================================
// [1] STRUCT — entitas utama data transaksi
// ============================================================
struct Transaksi {
    string tipe;
    string kategori;
    string keterangan;
    double jumlah;
    string tanggal;
};

// ============================================================
// [6] INLINE FUNCTION — fungsi pendek, dioptimasi compiler
// ============================================================
inline string formatRupiah(double value) {
    long long angka = (long long)value;
    string hasil = to_string(angka);
    for (int i = (int)hasil.length() - 3; i > 0; i -= 3)
        hasil.insert(i, ".");
    return "Rp " + hasil;
}

// Inline: cek apakah transaksi bertipe pemasukan
inline bool isPemasukan(const Transaksi& t) {
    return t.tipe == "Pemasukan";
}

// Inline: cek apakah transaksi bertipe pengeluaran
inline bool isPengeluaran(const Transaksi& t) {
    return t.tipe == "Pengeluaran";
}

// ============================================================
// [6] DEFAULT ARGUMENT — nilai default jika tidak diisi
// ============================================================
string buatRingkasanTeks(const vector<Transaksi>& data,
                          const string& judul = "Ringkasan Transaksi",
                          int limit = 5) {
    ostringstream oss;
    oss << judul << " (maks " << limit << " data):\n";
    int tampil = 0;
    for (const auto& t : data) {
        if (tampil >= limit) break;
        oss << "- [" << t.tipe << "] " << t.keterangan
            << " : " << formatRupiah(t.jumlah) << "\n";
        tampil++;
    }
    return oss.str();
}

// ============================================================
// [2] REFERENCES (&) — modifikasi langsung via referensi
// ============================================================
void updateSaldo(const Transaksi& t, double& saldoKebutuhan, double& saldoTabungan) {
    if (t.tipe == "Pemasukan") {
        saldoKebutuhan += t.jumlah * 0.6;
        saldoTabungan  += t.jumlah * 0.4;
    } else if (t.tipe == "Tabungan") {
        saldoTabungan  += t.jumlah;
    } else if (t.tipe == "Pengeluaran") {
        if (t.jumlah <= saldoKebutuhan) {
            saldoKebutuhan -= t.jumlah;
        } else {
            double sisa = t.jumlah - saldoKebutuhan;
            saldoKebutuhan = 0;
            saldoTabungan -= sisa;
        }
    }
}

// Reset saldo via referensi ke 0
void resetSaldo(double& saldoKebutuhan, double& saldoTabungan, double& targetTabungan) {
    saldoKebutuhan = 0;
    saldoTabungan  = 0;
    targetTabungan = 0;
}

// ============================================================
// [8] EXCEPTION HANDLING — validasi input dengan try-catch-throw
// ============================================================
double validasiNominal(const string& inputStr) {
    string input = inputStr;
    input.erase(remove(input.begin(), input.end(), '.'), input.end());
    input.erase(remove(input.begin(), input.end(), ','), input.end());
    try {
        double nilai = stod(input);
        if (nilai <= 0)
            throw invalid_argument("Nominal harus lebih dari 0!");
        return nilai;
    } catch (const invalid_argument& e) {
        throw invalid_argument(string("Input tidak valid: ") + e.what());
    } catch (...) {
        throw runtime_error("Terjadi kesalahan saat memproses nominal!");
    }
}

double konversiRupiah(string input) {
    input.erase(remove(input.begin(), input.end(), '.'), input.end());
    input.erase(remove(input.begin(), input.end(), ','), input.end());
    try { return stod(input); } catch (...) { return 0; }
}

// ============================================================
// [5] CALLBACK FUNCTION — fungsi menerima fungsi lain sebagai parameter
// ============================================================
void prosesTransaksi(const Transaksi& t,
                     function<void(const Transaksi&)> onBerhasil) {
    onBerhasil(t);
}

vector<Transaksi> filterTransaksi(const vector<Transaksi>& data,
                                   function<bool(const Transaksi&)> kondisi) {
    vector<Transaksi> hasil;
    for (const auto& t : data) {
        if (kondisi(t)) hasil.push_back(t);
    }
    return hasil;
}

// ============================================================
// [7] FUNCTION OVERLOADING — buatTransaksi dengan parameter berbeda
// ============================================================

// Versi 1: semua field lengkap
Transaksi buatTransaksi(const string& tipe, const string& kategori,
                        const string& keterangan, double jumlah,
                        const string& tanggal) {
    return {tipe, kategori, keterangan, jumlah, tanggal};
}

// Versi 2: tanpa tanggal → default "N/A"
Transaksi buatTransaksi(const string& tipe, const string& kategori,
                        const string& keterangan, double jumlah) {
    return {tipe, kategori, keterangan, jumlah, "N/A"};
}

// Versi 3: tipe, jumlah, tanggal → kategori & keterangan default
Transaksi buatTransaksi(const string& tipe, double jumlah, const string& tanggal) {
    return {tipe, "Umum", "Tanpa Keterangan", jumlah, tanggal};
}

// Versi 4: paling minimal, hanya tipe & jumlah
Transaksi buatTransaksi(const string& tipe, double jumlah) {
    return {tipe, "Umum", "Tanpa Keterangan", jumlah, "N/A"};
}

// JSON helpers
string escapeJson(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

string transaksiToJson(const Transaksi& t) {
    ostringstream oss;
    oss << "{"
        << "\"tipe\":\""        << escapeJson(t.tipe)        << "\","
        << "\"kategori\":\""    << escapeJson(t.kategori)    << "\","
        << "\"keterangan\":\""  << escapeJson(t.keterangan)  << "\","
        << "\"jumlah\":"        << (long long)t.jumlah        << ","
        << "\"tanggal\":\""     << escapeJson(t.tanggal)     << "\""
        << "}";
    return oss.str();
}

string parseJsonField(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        size_t end = json.find_first_of(",}", pos);
        return json.substr(pos, end - pos);
    }
}

} // namespace Finance

// ============================================================
// PENGELOLA KEUANGAN
// ============================================================
class PengelolaKeuangan {
private:
    // [9] STL VECTOR — container utama daftar transaksi
    vector<Finance::Transaksi> daftarTransaksi;

    double saldoKebutuhan = 0;
    double saldoTabungan  = 0;
    double targetTabungan = 0;
    const string FILE_RIWAYAT = "riwayat_transaksi.txt";

    // [10] FILE HANDLING — simpan transaksi ke file .txt
    void simpanKeFile(const Finance::Transaksi& t) {
        ofstream file(FILE_RIWAYAT, ios::app);
        if (file.is_open()) {
            file << t.tipe       << "|"
                 << t.kategori   << "|"
                 << t.keterangan << "|"
                 << t.jumlah     << "|"
                 << t.tanggal    << "\n";
        }
    }

    // [10] FILE HANDLING — baca transaksi dari file .txt
    void muatDariFile() {
        ifstream file(FILE_RIWAYAT);
        if (!file) return;
        string line;

        // [2] REFERENCES — reset saldo via referensi
        Finance::resetSaldo(saldoKebutuhan, saldoTabungan, targetTabungan);
        daftarTransaksi.clear();

        while (getline(file, line)) {
            istringstream ss(line);
            Finance::Transaksi t;
            string jumlahStr;
            getline(ss, t.tipe,        '|');
            getline(ss, t.kategori,    '|');
            getline(ss, t.keterangan,  '|');
            getline(ss, jumlahStr,     '|');
            getline(ss, t.tanggal);
            t.jumlah = Finance::konversiRupiah(jumlahStr);
            daftarTransaksi.push_back(t);

            // [2] REFERENCES — update saldo via referensi
            Finance::updateSaldo(t, saldoKebutuhan, saldoTabungan);
        }

        ifstream ft("target.txt");
        if (ft) ft >> targetTabungan;
    }

    // Helper: build JSON array dari vector transaksi
    string transaksiVectorToJson(const vector<Finance::Transaksi>& data) {
        ostringstream oss;
        oss << "[";
        // [9] ITERATOR — menelusuri vector dengan iterator
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it != data.begin()) oss << ",";
            // [3] POINTER — akses elemen via pointer
            const Finance::Transaksi* pt = &(*it);
            oss << Finance::transaksiToJson(*pt);
        }
        oss << "]";
        return oss.str();
    }

public:
    PengelolaKeuangan() { muatDariFile(); }

    // GET /api/saldo
    string getSaldo() {
        // [3] POINTER — baca saldo via pointer
        double* pK = &saldoKebutuhan;
        double* pT = &saldoTabungan;
        double  total = *pK + *pT;

        ostringstream oss;
        oss << "{"
            << "\"saldoKebutuhan\":" << (long long)(*pK)          << ","
            << "\"saldoTabungan\":"  << (long long)(*pT)          << ","
            << "\"totalSaldo\":"     << (long long)total            << ","
            << "\"targetTabungan\":" << (long long)targetTabungan
            << "}";
        return oss.str();
    }

    // GET /api/transaksi
    string getTransaksi() {
        return transaksiVectorToJson(daftarTransaksi);
    }

    // POST /api/pemasukan
    string tambahPemasukan(const string& body) {
        string sumber  = Finance::parseJsonField(body, "sumber");
        string nomStr  = Finance::parseJsonField(body, "nominal");
        string tipeStr = Finance::parseJsonField(body, "tipe");
        string tanggal = Finance::parseJsonField(body, "tanggal");
        string tgl     = tanggal.empty() ? "N/A" : tanggal;

        double nominal = 0;

        // [8] EXCEPTION HANDLING — validasi nominal
        try {
            nominal = Finance::validasiNominal(nomStr);
        } catch (const exception& e) {
            return "{\"status\":\"error\",\"pesan\":\"" +
                   string(e.what()) + "\"}";
        }

        int tipe = tipeStr == "1" ? 1 : 2;

        // [3] POINTER — update saldo via pointer
        double* pK = &saldoKebutuhan;
        double* pT = &saldoTabungan;

        Finance::Transaksi t;
        if (tipe == 1) {
            *pK += nominal * 0.6;
            *pT += nominal * 0.4;
            // [7] OVERLOADING versi 1 (semua field lengkap)
            t = Finance::buatTransaksi("Pemasukan", "Pemasukan", sumber, nominal, tgl);
        } else {
            *pT += nominal;
            // [7] OVERLOADING versi 3 (tipe, jumlah, tanggal)
            t = Finance::buatTransaksi("Tabungan", nominal, tgl);
            t.keterangan = sumber.empty() ? "Tanpa Keterangan" : sumber;
        }

        // [5] CALLBACK FUNCTION — simpan ke vector & file via callback
        Finance::prosesTransaksi(t, [&](const Finance::Transaksi& trx) {
            daftarTransaksi.push_back(trx);
            simpanKeFile(trx);
        });

        return "{\"status\":\"ok\",\"pesan\":\"Pemasukan berhasil ditambahkan\"}";
    }

    // POST /api/pengeluaran
    string tambahPengeluaran(const string& body) {
        string ket     = Finance::parseJsonField(body, "keterangan");
        string jmlStr  = Finance::parseJsonField(body, "jumlah");
        string tanggal = Finance::parseJsonField(body, "tanggal");
        string tgl     = tanggal.empty() ? "N/A" : tanggal;

        double jumlah = 0;

        // [8] EXCEPTION HANDLING — validasi jumlah
        try {
            jumlah = Finance::validasiNominal(jmlStr);
            if (jumlah > saldoKebutuhan + saldoTabungan)
                throw runtime_error("Saldo tidak mencukupi!");
        } catch (const exception& e) {
            return "{\"status\":\"error\",\"pesan\":\"" +
                   string(e.what()) + "\"}";
        }

        // [2] REFERENCES — kurangi saldo via referensi
        Finance::Transaksi dummy = {"Pengeluaran", "Umum", ket, jumlah, tgl};
        Finance::updateSaldo(dummy, saldoKebutuhan, saldoTabungan);

        // [7] OVERLOADING versi 1 (semua field)
        Finance::Transaksi t = Finance::buatTransaksi("Pengeluaran", "Umum", ket, jumlah, tgl);

        // [5] CALLBACK FUNCTION — simpan via callback
        Finance::prosesTransaksi(t, [&](const Finance::Transaksi& trx) {
            daftarTransaksi.push_back(trx);
            simpanKeFile(trx);
        });

        return "{\"status\":\"ok\",\"pesan\":\"Pengeluaran berhasil ditambahkan\"}";
    }

    // POST /api/target
    string aturTarget(const string& body) {
        string tStr = Finance::parseJsonField(body, "target");
        double target = 0;

        // [8] EXCEPTION HANDLING — validasi target
        try {
            target = Finance::validasiNominal(tStr);
        } catch (const exception& e) {
            return "{\"status\":\"error\",\"pesan\":\"" +
                   string(e.what()) + "\"}";
        }

        // [3] POINTER — set target via pointer
        double* pTarget = &targetTabungan;
        *pTarget = target;

        // [10] FILE HANDLING — simpan target ke file
        ofstream ft("target.txt");
        ft << *pTarget;

        return "{\"status\":\"ok\",\"pesan\":\"Target berhasil diatur\"}";
    }

    // GET /api/statistik
    string getStatistik() {
        // [9] STL COUNT_IF + LAMBDA — hitung jumlah tiap tipe transaksi
        int ctrMasuk = count_if(
            daftarTransaksi.begin(), daftarTransaksi.end(),
            [](const Finance::Transaksi& t) { return t.tipe == "Pemasukan"; }
        );
        int ctrKeluar = count_if(
            daftarTransaksi.begin(), daftarTransaksi.end(),
            [](const Finance::Transaksi& t) { return t.tipe == "Pengeluaran"; }
        );
        int ctrTabungan = count_if(
            daftarTransaksi.begin(), daftarTransaksi.end(),
            [](const Finance::Transaksi& t) { return t.tipe == "Tabungan"; }
        );

        // [5] CALLBACK — hitung total nominal via lambda sebagai callback
        auto hitungTotal = [](const vector<Finance::Transaksi>& data,
                               function<bool(const Finance::Transaksi&)> filter) -> double {
            double total = 0;
            for (const auto& t : data)
                if (filter(t)) total += t.jumlah;
            return total;
        };

        double totalMasuk  = hitungTotal(daftarTransaksi,
            [](const Finance::Transaksi& t){ return t.tipe == "Pemasukan"; });
        double totalKeluar = hitungTotal(daftarTransaksi,
            [](const Finance::Transaksi& t){ return t.tipe == "Pengeluaran"; });

        double progress = targetTabungan > 0
            ? min((saldoTabungan / targetTabungan) * 100.0, 100.0) : 0;

        ostringstream oss;
        oss << "{"
            << "\"totalPemasukan\":"    << (long long)totalMasuk   << ","
            << "\"totalPengeluaran\":"  << (long long)totalKeluar  << ","
            << "\"jumlahPemasukan\":"   << ctrMasuk                << ","
            << "\"jumlahPengeluaran\":" << ctrKeluar               << ","
            << "\"jumlahTabungan\":"    << ctrTabungan             << ","
            << "\"progressTarget\":"    << (int)progress
            << "}";
        return oss.str();
    }

    // GET /api/cari?q=keyword
    string cariTransaksi(const string& keyword) {
        string kw = keyword;
        transform(kw.begin(), kw.end(), kw.begin(), ::tolower);

        // [5] CALLBACK + [11] LAMBDA — filter via filterTransaksi
        vector<Finance::Transaksi> hasil = Finance::filterTransaksi(
            daftarTransaksi,
            [&kw](const Finance::Transaksi& t) {
                string ket = t.keterangan;
                transform(ket.begin(), ket.end(), ket.begin(), ::tolower);
                return ket.find(kw) != string::npos;
            }
        );

        return transaksiVectorToJson(hasil);
    }

    // GET /api/sort?by=jumlah|tipe
    string getSortedTransaksi(const string& by = "jumlah") {
        vector<Finance::Transaksi> sorted = daftarTransaksi;

        // [9] STL SORT + [11] LAMBDA
        if (by == "tipe") {
            sort(sorted.begin(), sorted.end(),
                [](const Finance::Transaksi& a, const Finance::Transaksi& b) {
                    return a.tipe < b.tipe;
                }
            );
        } else {
            sort(sorted.begin(), sorted.end(),
                [](const Finance::Transaksi& a, const Finance::Transaksi& b) {
                    return a.jumlah > b.jumlah;
                }
            );
        }

        return transaksiVectorToJson(sorted);
    }

    // GET /api/ringkasan — pakai default argument
    string getRingkasan(int limit = 5) {
        // [6] DEFAULT ARGUMENT — buatRingkasanTeks dengan nilai default
        string teks = Finance::buatRingkasanTeks(
            daftarTransaksi, "Ringkasan Transaksi", limit
        );

        int total = (int)daftarTransaksi.size();

        // [9] STL COUNT_IF + [6] INLINE FUNCTION sebagai kondisi
        int jmlMasuk = count_if(daftarTransaksi.begin(), daftarTransaksi.end(),
            [](const Finance::Transaksi& t){ return Finance::isPemasukan(t); });
        int jmlKeluar = count_if(daftarTransaksi.begin(), daftarTransaksi.end(),
            [](const Finance::Transaksi& t){ return Finance::isPengeluaran(t); });

        ostringstream oss;
        oss << "{"
            << "\"jumlahTotal\":"       << total      << ","
            << "\"jumlahPemasukan\":"   << jmlMasuk   << ","
            << "\"jumlahPengeluaran\":" << jmlKeluar  << ","
            << "\"ringkasanTeks\":\""   << Finance::escapeJson(teks) << "\""
            << "}";
        return oss.str();
    }

    // DELETE /api/reset
    string resetData() {
        daftarTransaksi.clear();

        // [2] REFERENCES — reset semua saldo via referensi
        Finance::resetSaldo(saldoKebutuhan, saldoTabungan, targetTabungan);

        // [10] FILE HANDLING — hapus file data
        remove(FILE_RIWAYAT.c_str());
        remove("target.txt");

        return "{\"status\":\"ok\",\"pesan\":\"Data berhasil direset\"}";
    }
};

// ============================================================
// MAIN — HTTP SERVER
// ============================================================
int main() {
    PengelolaKeuangan app;
    httplib::Server svr;

    // [11] LAMBDA — setCORS sebagai lambda tanpa nama
    auto setCORS = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Content-Type", "application/json");
    };

    svr.Options(".*", [&](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.status = 204;
    });

    svr.Get("/api/saldo", [&](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.getSaldo(), "application/json");
    });

    svr.Get("/api/transaksi", [&](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.getTransaksi(), "application/json");
    });

    svr.Get("/api/statistik", [&](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.getStatistik(), "application/json");
    });

    // [6] DEFAULT ARGUMENT — sort by "jumlah" jika param tidak ada
    svr.Get("/api/sort", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        string by = req.has_param("by") ? req.get_param_value("by") : "jumlah";
        res.set_content(app.getSortedTransaksi(by), "application/json");
    });

    // [6] DEFAULT ARGUMENT — limit 5 jika param tidak ada
    svr.Get("/api/ringkasan", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        int limit = req.has_param("limit")
            ? stoi(req.get_param_value("limit")) : 5;
        res.set_content(app.getRingkasan(limit), "application/json");
    });

    svr.Get("/api/cari", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        string kw = req.has_param("q") ? req.get_param_value("q") : "";
        res.set_content(app.cariTransaksi(kw), "application/json");
    });

    svr.Post("/api/pemasukan", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.tambahPemasukan(req.body), "application/json");
    });

    svr.Post("/api/pengeluaran", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.tambahPengeluaran(req.body), "application/json");
    });

    svr.Post("/api/target", [&](const httplib::Request& req, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.aturTarget(req.body), "application/json");
    });

    svr.Delete("/api/reset", [&](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.set_content(app.resetData(), "application/json");
    });

    cout << "====================================\n";
    cout << " Smart Finance Backend berjalan...\n";
    cout << " http://localhost:8080\n";
    cout << "====================================\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
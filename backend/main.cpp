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

// =======================
// NAMESPACE FINANCE
// =======================
namespace Finance {

struct Transaksi {
    string tipe;
    string kategori;
    string keterangan;
    double jumlah;
    string tanggal;
};

inline string formatRupiah(double value) {
    long long angka = (long long)value;
    string hasil = to_string(angka);
    for (int i = (int)hasil.length() - 3; i > 0; i -= 3)
        hasil.insert(i, ".");
    return "Rp " + hasil;
}

double konversiRupiah(string input) {
    input.erase(remove(input.begin(), input.end(), '.'), input.end());
    input.erase(remove(input.begin(), input.end(), ','), input.end());
    try { return stod(input); } catch (...) { return 0; }
}

string escapeJson(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

string transaksiToJson(const Transaksi& t) {
    ostringstream oss;
    oss << "{"
        << "\"tipe\":\"" << escapeJson(t.tipe) << "\","
        << "\"kategori\":\"" << escapeJson(t.kategori) << "\","
        << "\"keterangan\":\"" << escapeJson(t.keterangan) << "\","
        << "\"jumlah\":" << (long long)t.jumlah << ","
        << "\"tanggal\":\"" << escapeJson(t.tanggal) << "\""
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

// =======================
// PENGELOLA KEUANGAN
// =======================
class PengelolaKeuangan {
private:
    vector<Finance::Transaksi> daftarTransaksi;
    double saldoKebutuhan = 0;
    double saldoTabungan  = 0;
    double targetTabungan = 0;
    const string FILE_RIWAYAT = "riwayat_transaksi.txt";

    void simpanKeFile(const Finance::Transaksi& t) {
        ofstream file(FILE_RIWAYAT, ios::app);
        if (file.is_open()) {
            file << t.tipe << "|" << t.kategori << "|"
                 << t.keterangan << "|" << t.jumlah << "|" << t.tanggal << "\n";
        }
    }

    void muatDariFile() {
        ifstream file(FILE_RIWAYAT);
        if (!file) return;
        string line;
        saldoKebutuhan = 0;
        saldoTabungan  = 0;
        daftarTransaksi.clear();
        while (getline(file, line)) {
            istringstream ss(line);
            Finance::Transaksi t;
            string jumlahStr;
            getline(ss, t.tipe,      '|');
            getline(ss, t.kategori,  '|');
            getline(ss, t.keterangan,'|');
            getline(ss, jumlahStr,   '|');
            getline(ss, t.tanggal);
            t.jumlah = Finance::konversiRupiah(jumlahStr);
            daftarTransaksi.push_back(t);
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
        ifstream ft("target.txt");
        if (ft) ft >> targetTabungan;
    }

public:
    PengelolaKeuangan() { muatDariFile(); }

    string getSaldo() {
        double total = saldoKebutuhan + saldoTabungan;
        ostringstream oss;
        oss << "{"
            << "\"saldoKebutuhan\":" << (long long)saldoKebutuhan << ","
            << "\"saldoTabungan\":"  << (long long)saldoTabungan  << ","
            << "\"totalSaldo\":"     << (long long)total           << ","
            << "\"targetTabungan\":" << (long long)targetTabungan
            << "}";
        return oss.str();
    }

    string getTransaksi() {
        ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < daftarTransaksi.size(); i++) {
            if (i > 0) oss << ",";
            oss << Finance::transaksiToJson(daftarTransaksi[i]);
        }
        oss << "]";
        return oss.str();
    }

    string tambahPemasukan(const string& body) {
        string sumber  = Finance::parseJsonField(body, "sumber");
        string nomStr  = Finance::parseJsonField(body, "nominal");
        string tipeStr = Finance::parseJsonField(body, "tipe");
        string tanggal = Finance::parseJsonField(body, "tanggal");

        double nominal = Finance::konversiRupiah(nomStr);
        int tipe = tipeStr == "1" ? 1 : 2;

        if (nominal <= 0)
            return "{\"status\":\"error\",\"pesan\":\"Nominal harus lebih dari 0\"}";

        Finance::Transaksi t;
        t.tanggal = tanggal.empty() ? "N/A" : tanggal;

        if (tipe == 1) {
            saldoKebutuhan += nominal * 0.6;
            saldoTabungan  += nominal * 0.4;
            t = {"Pemasukan", "Pemasukan", sumber, nominal, t.tanggal};
        } else {
            saldoTabungan += nominal;
            t = {"Tabungan", "Umum", sumber.empty() ? "Tanpa Keterangan" : sumber, nominal, t.tanggal};
        }

        daftarTransaksi.push_back(t);
        simpanKeFile(t);
        return "{\"status\":\"ok\",\"pesan\":\"Pemasukan berhasil ditambahkan\"}";
    }

    string tambahPengeluaran(const string& body) {
        string ket     = Finance::parseJsonField(body, "keterangan");
        string jmlStr  = Finance::parseJsonField(body, "jumlah");
        string tanggal = Finance::parseJsonField(body, "tanggal");
        double jumlah  = Finance::konversiRupiah(jmlStr);

        if (jumlah <= 0)
            return "{\"status\":\"error\",\"pesan\":\"Jumlah harus lebih dari 0\"}";
        if (jumlah > saldoKebutuhan + saldoTabungan)
            return "{\"status\":\"error\",\"pesan\":\"Saldo tidak mencukupi\"}";

        if (jumlah <= saldoKebutuhan) {
            saldoKebutuhan -= jumlah;
        } else {
            double sisa = jumlah - saldoKebutuhan;
            saldoKebutuhan = 0;
            saldoTabungan -= sisa;
        }

        Finance::Transaksi t = {"Pengeluaran", "Umum", ket, jumlah,
                                 tanggal.empty() ? "N/A" : tanggal};
        daftarTransaksi.push_back(t);
        simpanKeFile(t);
        return "{\"status\":\"ok\",\"pesan\":\"Pengeluaran berhasil ditambahkan\"}";
    }

    string aturTarget(const string& body) {
        string tStr = Finance::parseJsonField(body, "target");
        double target = Finance::konversiRupiah(tStr);
        if (target <= 0)
            return "{\"status\":\"error\",\"pesan\":\"Target harus lebih dari 0\"}";
        targetTabungan = target;
        ofstream ft("target.txt");
        ft << targetTabungan;
        return "{\"status\":\"ok\",\"pesan\":\"Target berhasil diatur\"}";
    }

    string getStatistik() {
        double totalMasuk  = 0;
        double totalKeluar = 0;
        int ctrMasuk = 0, ctrKeluar = 0, ctrTabungan = 0;

        for (auto& t : daftarTransaksi) {
            if (t.tipe == "Pemasukan")  { totalMasuk  += t.jumlah; ctrMasuk++; }
            if (t.tipe == "Pengeluaran"){ totalKeluar += t.jumlah; ctrKeluar++; }
            if (t.tipe == "Tabungan")   { ctrTabungan++; }
        }

        double progress = targetTabungan > 0
            ? min((saldoTabungan / targetTabungan) * 100.0, 100.0) : 0;

        ostringstream oss;
        oss << "{"
            << "\"totalPemasukan\":"   << (long long)totalMasuk   << ","
            << "\"totalPengeluaran\":" << (long long)totalKeluar  << ","
            << "\"jumlahPemasukan\":"  << ctrMasuk                << ","
            << "\"jumlahPengeluaran\":" << ctrKeluar              << ","
            << "\"jumlahTabungan\":"   << ctrTabungan             << ","
            << "\"progressTarget\":"   << (int)progress
            << "}";
        return oss.str();
    }

    string cariTransaksi(const string& keyword) {
        ostringstream oss;
        oss << "[";
        bool first = true;
        for (auto& t : daftarTransaksi) {
            string ket = t.keterangan;
            string kw  = keyword;
            transform(ket.begin(), ket.end(), ket.begin(), ::tolower);
            transform(kw.begin(),  kw.end(),  kw.begin(),  ::tolower);
            if (ket.find(kw) != string::npos) {
                if (!first) oss << ",";
                oss << Finance::transaksiToJson(t);
                first = false;
            }
        }
        oss << "]";
        return oss.str();
    }

    string resetData() {
        daftarTransaksi.clear();
        saldoKebutuhan = 0;
        saldoTabungan  = 0;
        targetTabungan = 0;
        remove(FILE_RIWAYAT.c_str());
        remove("target.txt");
        return "{\"status\":\"ok\",\"pesan\":\"Data berhasil direset\"}";
    }
};

// =======================
// MAIN
// =======================
int main() {
    PengelolaKeuangan app;
    httplib::Server svr;

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
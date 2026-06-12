// =======================
// CONFIG
// =======================
const API = 'http://localhost:8080/api';

// =======================
// STATE LOKAL
// =======================
let allTransaksi = [];
let saldoData    = { saldoKebutuhan:0, saldoTabungan:0, totalSaldo:0, targetTabungan:0 };
let statistikData = {};

// =======================
// UTILITAS
// =======================
function formatRupiah(value) {
  return 'Rp ' + Number(value).toLocaleString('id-ID');
}

function getTanggalHariIni() {
  return new Date().toLocaleDateString('id-ID', {
    weekday:'long', year:'numeric', month:'long', day:'numeric'
  });
}

function getTanggalISO() {
  return new Date().toLocaleDateString('id-ID', {
    day:'2-digit', month:'short', year:'numeric'
  });
}

// =======================
// TOAST NOTIFIKASI
// =======================
let toastTimer = null;
function showToast(msg, type = 'success') {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.className = `toast ${type}`;
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.className = 'toast hidden'; }, 3200);
}

// =======================
// MODAL
// =======================
function showModal(title, bodyHTML, buttons) {
  document.getElementById('modalTitle').textContent = title;
  document.getElementById('modalBody').innerHTML = bodyHTML;
  const footer = document.getElementById('modalFooter');
  footer.innerHTML = '';
  buttons.forEach(b => {
    const btn = document.createElement('button');
    btn.textContent = b.label;
    btn.className = b.cls || 'btn-confirm';
    btn.onclick = () => { closeModal(); b.action && b.action(); };
    footer.appendChild(btn);
  });
  document.getElementById('modalOverlay').classList.remove('hidden');
}

function closeModal() {
  document.getElementById('modalOverlay').classList.add('hidden');
}

document.getElementById('modalClose').onclick = closeModal;
document.getElementById('modalOverlay').addEventListener('click', e => {
  if (e.target === document.getElementById('modalOverlay')) closeModal();
});

// =======================
// SERVER STATUS CHECK
// =======================
async function checkServer() {
  const el = document.getElementById('serverStatus');
  try {
    const res = await fetch(`${API}/saldo`, { signal: AbortSignal.timeout(2000) });
    if (res.ok) {
      el.textContent = '✅ Server Online';
      el.className = 'server-status online';
      return true;
    }
  } catch (_) {}
  el.textContent = '❌ Server Offline';
  el.className = 'server-status offline';
  return false;
}

// =======================
// FETCH DATA
// =======================
async function fetchSaldo() {
  try {
    const res = await fetch(`${API}/saldo`);
    saldoData = await res.json();
    renderSaldo();
  } catch (e) { console.error('fetchSaldo gagal:', e); }
}

async function fetchTransaksi() {
  try {
    const res = await fetch(`${API}/transaksi`);
    allTransaksi = await res.json();
    renderTransaksiDashboard();
    renderTransaksiFull();
  } catch (e) { console.error('fetchTransaksi gagal:', e); }
}

async function fetchStatistik() {
  try {
    const res = await fetch(`${API}/statistik`);
    statistikData = await res.json();
    renderStatistik();
    renderPieChart();
  } catch (e) { console.error('fetchStatistik gagal:', e); }
}

async function refreshSemua() {
  await checkServer();
  await Promise.all([fetchSaldo(), fetchTransaksi(), fetchStatistik()]);
}

// =======================
// RENDER SALDO
// =======================
function renderSaldo() {
  const { saldoKebutuhan, saldoTabungan, totalSaldo, targetTabungan } = saldoData;
  document.getElementById('saldoKebutuhan').textContent = formatRupiah(saldoKebutuhan);
  document.getElementById('saldoTabungan').textContent  = formatRupiah(saldoTabungan);
  document.getElementById('totalSaldo').textContent     = formatRupiah(totalSaldo);

  const pctK = totalSaldo > 0 ? Math.round((saldoKebutuhan / totalSaldo) * 100) : 0;
  const pctT = totalSaldo > 0 ? Math.round((saldoTabungan  / totalSaldo) * 100) : 0;
  document.getElementById('pctKebutuhan').textContent = `${pctK}% dari total`;
  document.getElementById('pctTabungan').textContent  = `${pctT}% dari total`;
  document.getElementById('barKebutuhan').style.width = `${pctK}%`;
  document.getElementById('barTabungan').style.width  = `${pctT}%`;

  document.getElementById('infoSaldoKebutuhan').textContent = formatRupiah(saldoKebutuhan);
  document.getElementById('infoSaldoTabungan').textContent  = formatRupiah(saldoTabungan);

  const progress = targetTabungan > 0
    ? Math.min(Math.round((saldoTabungan / targetTabungan) * 100), 100) : 0;

  document.getElementById('targetAmt').textContent   = targetTabungan > 0 ? formatRupiah(targetTabungan) : 'Belum diatur';
  document.getElementById('tabunganAmt').textContent = formatRupiah(saldoTabungan);
  document.getElementById('targetPct').textContent   = `${progress}%`;
  document.getElementById('barTarget').style.width   = `${progress}%`;

  const motivasi = progress >= 100 ? '🎉 Target tercapai! Luar biasa!'
    : progress >= 75 ? '🔥 Hampir sampai! Tetap semangat!'
    : progress >= 50 ? '💪 Sudah lebih dari setengah jalan!'
    : progress >= 25 ? '🌱 Bagus! Terus konsisten ya!'
    : targetTabungan > 0 ? '🚀 Yuk mulai menabung lebih giat!'
    : 'Atur target tabunganmu sekarang! 🎯';
  document.getElementById('motivasiText').textContent = motivasi;

  document.getElementById('targetPctPage').textContent = `${progress}%`;
  document.getElementById('statTarget').textContent    = targetTabungan > 0 ? formatRupiah(targetTabungan) : 'Belum diatur';
  document.getElementById('statTerkumpul').textContent = formatRupiah(saldoTabungan);
  const sisa = targetTabungan > 0 ? Math.max(targetTabungan - saldoTabungan, 0) : 0;
  document.getElementById('statSisa').textContent = targetTabungan > 0 ? formatRupiah(sisa) : '-';

  const circle = document.getElementById('targetCircle');
  if (circle) {
    const circ   = 2 * Math.PI * 54;
    const offset = circ - (progress / 100) * circ;
    circle.style.strokeDasharray  = circ.toFixed(1);
    circle.style.strokeDashoffset = offset.toFixed(1);
  }
}

// =======================
// RENDER TRANSAKSI DASHBOARD
// =======================
function renderTransaksiDashboard() {
  const tbody  = document.getElementById('transaksiBodyDashboard');
  const recent = [...allTransaksi].reverse().slice(0, 5);
  if (recent.length === 0) {
    tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;color:gray;padding:20px">Belum ada transaksi</td></tr>';
    return;
  }
  tbody.innerHTML = recent.map((t, i) => {
    const cls = t.tipe === 'Pemasukan' ? 'income-text' : t.tipe === 'Pengeluaran' ? 'expense-text' : 'saving-text';
    return `<tr>
      <td>${i + 1}</td><td>${t.tanggal}</td>
      <td class="${cls}">${t.tipe}</td>
      <td>${t.keterangan}</td>
      <td class="${cls}">${formatRupiah(t.jumlah)}</td>
    </tr>`;
  }).join('');
}

// =======================
// RENDER TRANSAKSI FULL
// =======================
function renderTransaksiFull(data = null) {
  const tbody = document.getElementById('transaksiBodyFull');
  const empty = document.getElementById('transaksiEmpty');
  const list  = data !== null ? data : [...allTransaksi].reverse();
  if (list.length === 0) {
    tbody.innerHTML = '';
    empty.classList.remove('hidden');
    return;
  }
  empty.classList.add('hidden');
  tbody.innerHTML = list.map((t, i) => {
    const cls = t.tipe === 'Pemasukan' ? 'income-text' : t.tipe === 'Pengeluaran' ? 'expense-text' : 'saving-text';
    return `<tr>
      <td>${i + 1}</td><td>${t.tanggal}</td>
      <td class="${cls}">${t.tipe}</td>
      <td>${t.keterangan}</td>
      <td class="${cls}">${formatRupiah(t.jumlah)}</td>
    </tr>`;
  }).join('');
}

// =======================
// FILTER TRANSAKSI
// =======================
function filterTransaksi() {
  const kw   = document.getElementById('searchInput').value.toLowerCase();
  const tipe = document.getElementById('filterTipe').value;
  let filtered = [...allTransaksi].reverse();
  if (kw)   filtered = filtered.filter(t => t.keterangan.toLowerCase().includes(kw) || t.tipe.toLowerCase().includes(kw));
  if (tipe) filtered = filtered.filter(t => t.tipe === tipe);
  renderTransaksiFull(filtered);
}

// =======================
// RENDER STATISTIK
// =======================
function renderStatistik() {
  const s     = statistikData;
  const total = saldoData.saldoKebutuhan + saldoData.saldoTabungan;
  document.getElementById('statPemasukan').textContent      = formatRupiah(s.totalPemasukan   || 0);
  document.getElementById('statPengeluaran').textContent    = formatRupiah(s.totalPengeluaran || 0);
  document.getElementById('statCtrPemasukan').textContent   = `${s.jumlahPemasukan || 0} transaksi`;
  document.getElementById('statCtrPengeluaran').textContent = `${s.jumlahPengeluaran || 0} transaksi`;
  document.getElementById('statCtrTabunganAmt').textContent = s.jumlahTabungan || 0;
  document.getElementById('statBersih').textContent         = formatRupiah(total);
  document.getElementById('totalPemasukan').textContent     = formatRupiah(s.totalPemasukan   || 0);
  document.getElementById('totalPengeluaran').textContent   = formatRupiah(s.totalPengeluaran || 0);
  renderBarChart();
}

// =======================
// PIE CHART
// =======================
function renderPieChart() {
  const canvas = document.getElementById('pieCanvas');
  if (!canvas) return;
  const ctx    = canvas.getContext('2d');
  const masuk  = statistikData.totalPemasukan  || 0;
  const keluar = statistikData.totalPengeluaran || 0;
  const total  = masuk + keluar;
  ctx.clearRect(0, 0, 160, 160);
  if (total === 0) {
    ctx.beginPath();
    ctx.arc(80, 80, 64, 0, 2 * Math.PI);
    ctx.fillStyle = '#ececec';
    ctx.fill();
    ctx.fillStyle = '#aaa';
    ctx.font = '12px Poppins';
    ctx.textAlign = 'center';
    ctx.fillText('Belum ada data', 80, 85);
    return;
  }
  let start = -Math.PI / 2;
  [{ value: masuk, color: '#2ecc71' }, { value: keluar, color: '#ff6b9a' }].forEach(s => {
    if (s.value === 0) return;
    const angle = (s.value / total) * 2 * Math.PI;
    ctx.beginPath();
    ctx.moveTo(80, 80);
    ctx.arc(80, 80, 64, start, start + angle);
    ctx.closePath();
    ctx.fillStyle = s.color;
    ctx.fill();
    start += angle;
  });
  ctx.beginPath();
  ctx.arc(80, 80, 34, 0, 2 * Math.PI);
  ctx.fillStyle = 'white';
  ctx.fill();
}

// =======================
// BAR CHART
// =======================
function renderBarChart() {
  const canvas = document.getElementById('barCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const W   = canvas.offsetWidth || 500;
  canvas.width  = W;
  canvas.height = 200;
  ctx.clearRect(0, 0, W, 200);
  const s      = statistikData;
  const labels = ['Pemasukan', 'Pengeluaran', 'Tabungan'];
  const values = [s.jumlahPemasukan || 0, s.jumlahPengeluaran || 0, s.jumlahTabungan || 0];
  const colors = ['#2ecc71', '#ff4d6d', '#9b59b6'];
  const maxVal = Math.max(...values, 1);
  const gap    = W / labels.length;
  const barW   = gap * 0.4;
  values.forEach((v, i) => {
    const barH = (v / maxVal) * 150;
    const x    = gap * i + gap * 0.3;
    const y    = 175 - barH;
    ctx.fillStyle = colors[i];
    ctx.beginPath();
    const r = Math.min(barW / 2, 8);
    ctx.roundRect(x, y, barW, barH, [r, r, 0, 0]);
    ctx.fill();
    ctx.fillStyle = '#2d3436';
    ctx.font = '13px Poppins';
    ctx.textAlign = 'center';
    ctx.fillText(labels[i], x + barW / 2, 195);
    ctx.fillText(v, x + barW / 2, y - 5);
  });
}

// =======================
// SUBMIT PEMASUKAN
// =======================
async function submitPemasukan() {
  const sumber  = document.getElementById('inSumber').value.trim();
  const nominal = parseFloat(document.getElementById('inNominal').value);
  const tipe    = document.querySelector('input[name="tipeAlokasi"]:checked')?.value || '1';
  if (!sumber)              return showToast('Isi sumber pemasukan!', 'error');
  if (!nominal || nominal <= 0) return showToast('Nominal harus lebih dari 0!', 'error');
  try {
    const res  = await fetch(`${API}/pemasukan`, {
      method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ sumber, nominal, tipe, tanggal: getTanggalISO() })
    });
    const data = await res.json();
    if (data.status === 'ok') {
      showToast('✅ ' + data.pesan);
      document.getElementById('inSumber').value  = '';
      document.getElementById('inNominal').value = '';
      await refreshSemua();
    } else { showToast('❌ ' + data.pesan, 'error'); }
  } catch (e) { showToast('❌ Gagal terhubung ke server!', 'error'); }
}

// =======================
// SUBMIT PENGELUARAN
// =======================
async function submitPengeluaran() {
  const ket    = document.getElementById('inKeterangan').value.trim();
  const jumlah = parseFloat(document.getElementById('inJumlah').value);
  if (!ket)                  return showToast('Isi keterangan!', 'error');
  if (!jumlah || jumlah <= 0) return showToast('Jumlah harus lebih dari 0!', 'error');
  if (jumlah > saldoData.saldoKebutuhan + saldoData.saldoTabungan)
    return showToast('❌ Saldo tidak mencukupi!', 'error');
  if (jumlah > saldoData.saldoKebutuhan) {
    showModal('⚠️ Konfirmasi',
      `<p>Saldo kebutuhan tidak mencukupi.<br>Gunakan saldo tabungan untuk menutup kekurangannya?</p>`,
      [
        { label:'Batal', cls:'btn-cancel' },
        { label:'Ya, Lanjutkan', cls:'btn-confirm', action: () => kirimPengeluaran(ket, jumlah) }
      ]
    );
    return;
  }
  await kirimPengeluaran(ket, jumlah);
}

async function kirimPengeluaran(ket, jumlah) {
  try {
    const res  = await fetch(`${API}/pengeluaran`, {
      method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ keterangan: ket, jumlah, tanggal: getTanggalISO() })
    });
    const data = await res.json();
    if (data.status === 'ok') {
      showToast('✅ ' + data.pesan);
      document.getElementById('inKeterangan').value = '';
      document.getElementById('inJumlah').value     = '';
      await refreshSemua();
    } else { showToast('❌ ' + data.pesan, 'error'); }
  } catch (e) { showToast('❌ Gagal terhubung ke server!', 'error'); }
}

// =======================
// SUBMIT TARGET
// =======================
async function submitTarget() {
  const target = parseFloat(document.getElementById('inTarget').value);
  if (!target || target <= 0) return showToast('Masukkan target yang valid!', 'error');
  try {
    const res  = await fetch(`${API}/target`, {
      method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ target })
    });
    const data = await res.json();
    if (data.status === 'ok') {
      showToast('✅ ' + data.pesan);
      document.getElementById('inTarget').value = '';
      await refreshSemua();
    } else { showToast('❌ ' + data.pesan, 'error'); }
  } catch (e) { showToast('❌ Gagal terhubung ke server!', 'error'); }
}

// =======================
// CARI TRANSAKSI (MODAL)
// =======================
async function cariTransaksiModal() {
  showModal('🔍 Cari Transaksi',
    `<input type="text" id="modalSearch" placeholder="Ketik kata kunci..."
      style="width:100%;border:2px solid #e9e9e9;border-radius:12px;padding:12px 14px;font-size:14px;font-family:inherit;outline:none"
      oninput="this.style.borderColor='#2ecc71'" />
    <div id="searchResult" style="margin-top:16px"></div>`,
    [{ label:'Tutup', cls:'btn-cancel' }]
  );
  setTimeout(() => {
    const inp = document.getElementById('modalSearch');
    if (inp) {
      inp.focus();
      inp.addEventListener('input', async () => {
        const kw = inp.value.trim();
        if (!kw) { document.getElementById('searchResult').innerHTML = ''; return; }
        try {
          const res  = await fetch(`${API}/cari?q=${encodeURIComponent(kw)}`);
          const list = await res.json();
          const el   = document.getElementById('searchResult');
          if (!list.length) { el.innerHTML = '<p style="color:gray;font-size:13px">Tidak ditemukan.</p>'; return; }
          el.innerHTML = list.map(t => {
            const cls = t.tipe === 'Pemasukan' ? '#2ecc71' : t.tipe === 'Pengeluaran' ? '#ff4d6d' : '#9b59b6';
            return `<div style="border-top:1px solid #f1f1f1;padding:10px 0;font-size:13px">
              <b style="color:${cls}">${t.tipe}</b> — ${t.keterangan}<br>
              <span style="color:gray">${t.tanggal}</span>
              <span style="float:right;font-weight:600;color:${cls}">${formatRupiah(t.jumlah)}</span>
            </div>`;
          }).join('');
        } catch (_) {}
      });
    }
  }, 100);
}

// =======================
// RESET DATA
// =======================
function resetDataConfirm() {
  showModal('⚠️ Reset Data',
    `<p>Semua transaksi dan saldo akan dihapus permanen. Yakin?</p>`,
    [
      { label:'Batal', cls:'btn-cancel' },
      { label:'Ya, Reset', cls:'btn-danger', action: async () => {
        try {
          const res  = await fetch(`${API}/reset`, { method:'DELETE' });
          const data = await res.json();
          showToast(data.pesan, data.status === 'ok' ? 'success' : 'error');
          await refreshSemua();
        } catch (e) { showToast('❌ Gagal terhubung ke server!', 'error'); }
      }}
    ]
  );
}

// =======================
// NAVIGASI HALAMAN
// =======================
function switchPage(page) {
  document.querySelectorAll('.page').forEach(p => {
    p.classList.remove('active');
    p.classList.add('hidden');
  });
  const el = document.getElementById(`page-${page}`);
  if (el) { el.classList.remove('hidden'); el.classList.add('active'); }
  document.querySelectorAll('.menu li').forEach(li => {
    li.classList.toggle('active', li.dataset.page === page);
  });
  if (page === 'statistik') fetchStatistik();
  if (page === 'transaksi') renderTransaksiFull();
}

// =======================
// EVENT LISTENERS
// =======================
document.querySelectorAll('.menu li').forEach(li => {
  li.addEventListener('click', e => {
    e.preventDefault();
    const page = li.dataset.page;
    if (page) switchPage(page);
  });
});

document.getElementById('btnPemasukan').onclick  = () => switchPage('pemasukan');
document.getElementById('btnPengeluaran').onclick = () => switchPage('pengeluaran');
document.getElementById('btnTarget').onclick      = () => switchPage('target');
document.getElementById('btnCari').onclick        = cariTransaksiModal;
document.getElementById('btnReset').onclick       = resetDataConfirm;

document.getElementById('menuToggle').addEventListener('click', () => {
  document.getElementById('sidebar').classList.toggle('open');
});

window.addEventListener('resize', () => {
  renderBarChart();
  renderPieChart();
});

// =======================
// INISIALISASI
// =======================
document.getElementById('tanggalHeader').textContent = getTanggalHariIni();

(async () => {
  await refreshSemua();
})();
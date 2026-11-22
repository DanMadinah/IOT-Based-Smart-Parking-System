const $ = i => document.getElementById(i);

const D = {
  en: {
    title: 'Smart Parking', live: 'Live', off: 'Offline',
    k_cars: 'Cars in lot', k_free: 'Free spaces', h_reserve: 'Reserve a space',
    payNote: 'Flat fee RM2. Demo payment: no real money is used.',
    m1: 'Card', m2: 'E-wallet', m3: 'Online banking',
    payBtn: 'Pay RM2 and reserve', arm: "I'm at the gate", cancel: 'Cancel',
    codeBtn: 'Submit code', h_spaces: 'Parking spaces', h_read: 'Live readings',
    k_gate: 'Gate', k_mode: 'Mode', k_entry: 'Entry sensor', k_exit: 'Exit sensor',
    k_w1: 'Wall sensor 1', k_w2: 'Wall sensor 2', h_today: 'Today',
    k_in: 'Cars entered', k_out: 'Cars left', k_peak: 'Peak cars at once',
    h_occ: 'Occupancy (last 30 minutes)', h_health: 'System health',
    h_events: 'Recent events', foot: 'Updates every few seconds',
    open: 'OPEN: {n} free', full: 'LOT FULL', fullr: 'LOT FULL ({n} reserved)',
    manual: 'MANUAL MODE', alert: 'SECURITY ALERT',
    sfree: 'Free', staken: 'Taken', sres: 'Reserved',
    gopen: 'Open', gclosed: 'Closed', det: 'Car detected', clear: 'Clear',
    noecho: 'No echo', msimple: 'Simple', msecure: 'Secure (LCD code)',
    mmanual: 'Manual', hok: 'All systems OK', hwarn: 'Attention',
    ticket: 'Ticket', proc: 'Processing payment...', paid: 'Payment received',
    hot: 'Hotspot',
    tm_res: "Space reserved. {t} left. Drive to the gate, then tap I'm at the gate.",
    tm_arm: 'Gate armed. Drive to the entry sensor now ({s} s).',
    tm_code: 'Type the 4-digit code shown on the LCD.',
    tm_ent: 'Gate opening. Drive in.',
    tm_used: 'You are in. Thank you!',
    tm_exp: 'Reservation expired. Payment refunded (demo).',
    tm_can: 'Cancelled. Payment refunded (demo).',
    tm_atgate: "Your car is at the gate. Tap I'm at the gate now.",
    tm_locked: 'Ticket locked. Try again in {s} s.',
    collect: 'Collecting data...'
  },
  ms: {
    title: 'Parkir Pintar', live: 'Langsung', off: 'Luar talian',
    k_cars: 'Kereta di lot', k_free: 'Ruang kosong', h_reserve: 'Tempah ruang',
    payNote: 'Bayaran tetap RM2. Pembayaran demo: tiada wang sebenar digunakan.',
    m1: 'Kad', m2: 'E-dompet', m3: 'Perbankan dalam talian',
    payBtn: 'Bayar RM2 dan tempah', arm: 'Saya di pintu pagar', cancel: 'Batal',
    codeBtn: 'Hantar kod', h_spaces: 'Ruang parkir', h_read: 'Bacaan langsung',
    k_gate: 'Pintu pagar', k_mode: 'Mod', k_entry: 'Sensor masuk', k_exit: 'Sensor keluar',
    k_w1: 'Sensor dinding 1', k_w2: 'Sensor dinding 2', h_today: 'Hari ini',
    k_in: 'Kereta masuk', k_out: 'Kereta keluar', k_peak: 'Kereta tertinggi serentak',
    h_occ: 'Penghunian (30 minit lepas)', h_health: 'Kesihatan sistem',
    h_events: 'Peristiwa terkini', foot: 'Dikemas kini setiap beberapa saat',
    open: 'BUKA: {n} kosong', full: 'LOT PENUH', fullr: 'LOT PENUH ({n} ditempah)',
    manual: 'MOD MANUAL', alert: 'AMARAN KESELAMATAN',
    sfree: 'Kosong', staken: 'Diisi', sres: 'Ditempah',
    gopen: 'Buka', gclosed: 'Tutup', det: 'Kereta dikesan', clear: 'Kosong',
    noecho: 'Tiada gema', msimple: 'Mudah', msecure: 'Selamat (kod LCD)',
    mmanual: 'Manual', hok: 'Semua sistem OK', hwarn: 'Perhatian',
    ticket: 'Tiket', proc: 'Memproses bayaran...', paid: 'Bayaran diterima',
    hot: 'Hotspot',
    tm_res: 'Ruang ditempah. {t} lagi. Pandu ke pintu pagar, kemudian tekan Saya di pintu pagar.',
    tm_arm: 'Pintu pagar bersedia. Pandu ke sensor masuk sekarang ({s} s).',
    tm_code: 'Taip kod 4 digit pada LCD.',
    tm_ent: 'Pintu pagar dibuka. Sila masuk.',
    tm_used: 'Anda sudah masuk. Terima kasih!',
    tm_exp: 'Tempahan tamat. Bayaran dikembalikan (demo).',
    tm_can: 'Dibatalkan. Bayaran dikembalikan (demo).',
    tm_atgate: 'Kereta anda di pintu pagar. Tekan Saya di pintu pagar sekarang.',
    tm_locked: 'Tiket dikunci. Cuba lagi dalam {s} s.',
    collect: 'Mengumpul data...'
  }
};

let lang = localStorage.getItem('lang') || 'en';
const T = (k, p) => {
  let s = (D[lang] && D[lang][k]) || D.en[k] || k;
  if (p) for (const x in p) s = s.replace('{' + x + '}', p[x]);
  return s;
};

function applyLang() {
  document.documentElement.lang = lang;
  document.querySelectorAll('[data-k]').forEach(e => { e.textContent = T(e.dataset.k); });
  $('lang').textContent = lang === 'en' ? 'BM' : 'EN';
}

$('lang').onclick = () => {
  lang = lang === 'en' ? 'ms' : 'en';
  localStorage.setItem('lang', lang);
  applyLang();
  tick();
  refreshTickets();
};
applyLang();

const pref = localStorage.getItem('dark');
if (pref === '1' || (pref === null && matchMedia('(prefers-color-scheme: dark)').matches)) {
  document.body.classList.add('dark');
}
$('theme').onclick = () => {
  document.body.classList.toggle('dark');
  localStorage.setItem('dark', document.body.classList.contains('dark') ? '1' : '0');
};

const shown = {};
function count(id, to) {
  const el = $(id);
  const from = (shown[id] === undefined) ? to : shown[id];
  shown[id] = to;
  if (from === to) { el.textContent = to; return; }
  const t0 = performance.now();
  const step = t => {
    const p = Math.min((t - t0) / 500, 1);
    el.textContent = Math.round(from + (to - from) * p);
    if (p < 1) requestAnimationFrame(step);
  };
  requestAnimationFrame(step);
}

let toastT;
function toast(m) {
  const t = $('toast');
  t.textContent = m;
  t.classList.add('show');
  clearTimeout(toastT);
  toastT = setTimeout(() => t.classList.remove('show'), 3200);
}

function cm(v) { return v > 0 ? v.toFixed(0) + ' cm' : T('noecho'); }
function mmss(s) { return Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0'); }
function up(s) { return Math.floor(s / 3600) + 'h ' + Math.floor(s % 3600 / 60) + 'm'; }

async function api(p) {
  try {
    return await (await fetch(p + (p.includes('?') ? '&' : '?') + '_=' + Date.now(), { cache: 'no-store' })).json();
  } catch (e) {
    return { ok: false, msg: 'Connection problem' };
  }
}

const CAR = '<svg viewBox="0 0 40 18" width="34" height="15"><rect x="0" y="6" width="40" height="9" rx="3" fill="currentColor"/><rect x="9" y="0" width="20" height="9" rx="3" fill="currentColor"/></svg>';

let tickets = [];
try { tickets = JSON.parse(localStorage.getItem('tix') || '[]'); } catch (e) { tickets = []; }
const oldTok = localStorage.getItem('tok');
if (oldTok && !tickets.some(x => x.t === oldTok)) {
  tickets.push({ t: oldTok, n: 'Car' });
  localStorage.removeItem('tok');
}
function save() { localStorage.setItem('tix', JSON.stringify(tickets)); }
const esc = s => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

function okAnim() {
  const o = $('ok');
  $('okt').textContent = T('paid');
  o.classList.remove('show');
  void o.offsetWidth;
  o.classList.add('show');
  setTimeout(() => o.classList.remove('show'), 2600);
}

$('payBtn').onclick = async () => {
  $('rmsg').textContent = T('proc');
  const nm = ($('cname').value || '').trim() || ('Car ' + (tickets.length + 1));
  const r = await api('/api/pay?method=' + encodeURIComponent($('method').value) + '&name=' + encodeURIComponent(nm));
  $('rmsg').textContent = r.ok ? '' : (r.msg || '');
  if (!r.ok) toast(r.msg || 'Payment failed');
  if (r.ok) {
    tickets.push({ t: r.token, n: nm });
    save();
    $('cname').value = '';
    okAnim();
    refreshTickets();
  }
};

async function act(kind, t) {
  $('rmsg').textContent = '...';
  const r = await api('/api/' + kind + '?t=' + t);
  $('rmsg').textContent = r.msg || '';
  refreshTickets();
}

async function sendCode(t) {
  const el = $('cd' + t);
  const r = await api('/api/code?t=' + t + '&c=' + encodeURIComponent(el ? el.value : ''));
  $('rmsg').textContent = r.msg || '';
  if (el) el.value = '';
  refreshTickets();
}

function line(x) {
  const S = x.state, id = "'" + x.t + "'";
  let m = '', b = '';
  if (S === 'reserved') {
    m = x.atgate ? T('tm_atgate') : T('tm_res', { t: mmss(x.left) });
    b = '<button class="primary" onclick="act(\'arm\',' + id + ')">' + T('arm') + '</button>' +
        '<button onclick="act(\'cancel\',' + id + ')">' + T('cancel') + '</button>';
  } else if (S === 'armed') {
    m = x.locked > 0 ? T('tm_locked', { s: x.locked }) : (x.needCode ? T('tm_code') : T('tm_arm', { s: x.left }));
    if (x.needCode) {
      b = '<input id="cd' + x.t + '" inputmode="numeric" maxlength="4" placeholder="1234">' +
          '<button class="primary" onclick="sendCode(' + id + ')">' + T('codeBtn') + '</button>';
    }
    b += '<button onclick="act(\'cancel\',' + id + ')">' + T('cancel') + '</button>';
  } else if (S === 'entering') m = T('tm_ent');
  else if (S === 'parked') {
    m = "Parked. Tap I'm exiting when you leave.";
    b = '<button class="primary" onclick="act(\'exitarm\',' + id + ')">I\'m exiting</button>';
  } else if (S === 'exiting') m = 'Exit armed (' + x.left + ' s). Drive to the exit sensor now.';
  else if (S === 'left') m = 'This car has left. Thank you!';
  else if (S === 'used') m = T('tm_used');
  else if (S === 'expired') m = T('tm_exp');
  else if (S === 'cancelled') m = T('tm_can');

  return '<div style="border-top:1px solid var(--line);padding:10px 0">' +
         '<div class="v">' + esc(x.n) + ' &middot; ' + x.ticket + '</div>' +
         '<div class="sub">' + m + '</div>' +
         '<div style="margin-top:6px">' + b + '</div></div>';
}

let lastTix = '';
async function refreshTickets() {
  const box = $('tickets');
  if (!tickets.length) { box.innerHTML = ''; return; }
  if (document.activeElement && document.activeElement.tagName === 'INPUT' && box.contains(document.activeElement)) return;
  const r = await api('/api/tickets?t=' + tickets.map(x => x.t).join(','));
  if (!Array.isArray(r)) return;
  const fin = ['left', 'used', 'expired', 'cancelled'];
  r.sort((a, b) => fin.includes(a.state) - fin.includes(b.state));
  const live = new Set();
  let h = '';
  r.forEach(x => { live.add(x.t); h += line(x); });
  const before = tickets.length;
  tickets = tickets.filter(x => live.has(x.t));
  if (tickets.length !== before) save();
  if (h !== lastTix) { box.innerHTML = h; lastTix = h; }
}
let tm2 = setInterval(refreshTickets, 2500);
refreshTickets();

const slotX = [140, 75, 10, -70, -135];
const carQ = [0, 1, 2, 3, 4].map(i => $('car' + i));
carQ.forEach((el, k) => { el.style.transform = 'translateX(' + slotX[k] + 'px)'; });
let prevCars = null, prevOpen = false, prevTail = false, ran = false;

function advance() {
  const el = carQ.shift();
  $('scene').classList.add('go');
  el.style.transition = 'transform 2.4s ease-in';
  el.style.transform = 'translateX(400px)';
  carQ.forEach((c, k) => {
    c.style.transition = 'transform 1.2s ease-in-out';
    c.style.transform = 'translateX(' + slotX[k] + 'px)';
  });
  setTimeout(() => {
    el.style.transition = 'none';
    el.style.transform = 'translateX(' + slotX[carQ.length] + 'px)';
    carQ.push(el);
    setTimeout(() => $('scene').classList.remove('go'), 600);
  }, 2600);
}

function sceneUpdate(s) {
  const open = s.gate === 'Open';
  const top = s.events.length ? s.events[0].m : '';
  const leaving = /leav|exit|left/i.test(top);
  $('scene').classList.toggle('tail', !!s.tail);
  if (prevCars !== null) {
    if (open && !prevOpen && !leaving) { ran = true; advance(); }
    else if (s.cars > prevCars) { if (!ran) advance(); ran = false; }
    if (s.tail && !prevTail) setTimeout(advance, 900);
  }
  prevOpen = open; prevTail = !!s.tail; prevCars = s.cars;
}

let lastEv = '', firstLoad = true, lastSlots = '';
async function tick() {
  try {
    const s = await (await fetch('/api/status?_=' + Date.now(), { cache: 'no-store' })).json();
    $('dot').className = 'dot';
    $('conn').textContent = T('live');
    count('cars', s.cars);
    $('max').textContent = s.max;
    count('free', s.free);
    const sc = $('scene');
    sc.classList.toggle('open', s.gate === 'Open');
    sc.classList.toggle('full', s.free <= 0);
    sceneUpdate(s);
    $('gled').style.opacity = s.free > 0 ? 1 : .2;
    $('rled').style.opacity = (s.free <= 0 || s.unpaid) ? 1 : .2;

    const b = $('banner');
    if (s.alert) { b.textContent = T('alert'); b.style.background = 'var(--red)'; }
    else if (s.manual) { b.textContent = T('manual'); b.style.background = 'var(--amber)'; }
    else if (s.free <= 0) { b.textContent = s.reserved > 0 ? T('fullr', { n: s.reserved }) : T('full'); b.style.background = 'var(--red)'; }
    else { b.textContent = T('open', { n: s.free }); b.style.background = 'var(--green)'; }
    b.classList.toggle('flash', !!(s.unpaid || s.tail || s.warnNow || s.alert));

    let h = '';
    for (let i = 0; i < s.max; i++) {
      let c = 'free', t = T('sfree'), ic = '';
      if (i < s.cars) { c = 'taken'; t = T('staken'); ic = CAR; }
      else if (i < s.cars + s.reserved) { c = 'res'; t = T('sres'); }
      h += '<div class="slot ' + c + '">P' + (i + 1) + ic + '<small>' + t + '</small></div>';
    }
    if (h !== lastSlots) { $('slots').innerHTML = h; lastSlots = h; }

    $('gate').textContent = s.gate === 'Open' ? T('gopen') : T('gclosed');
    $('mode').textContent = s.manual ? T('mmanual') : (s.mode === 'secure' ? T('msecure') : T('msimple'));
    $('entry').textContent = s.entry ? T('det') : T('clear');
    $('exit').textContent = s.exit ? T('det') : T('clear');
    $('w1').textContent = cm(s.d1);
    $('w2').textContent = cm(s.d2);
    count('sin', s.in);
    count('sout', s.out);
    count('speak', s.peak);

    const net = s.ap ? T('hot') : 'Wi-Fi ' + s.rssi + ' dBm';
    if (s.warns.length) {
      $('health').textContent = T('hwarn');
      $('health').style.color = 'var(--amber)';
      $('hdetail').textContent = s.warns.join(' | ') + '  (uptime ' + up(s.uptime) + ')';
    } else {
      $('health').textContent = T('hok');
      $('health').style.color = 'var(--green)';
      $('hdetail').textContent = net + ' | uptime ' + up(s.uptime);
    }

    const top = s.events.length ? s.events[0].t + s.events[0].m : '';
    if (!firstLoad && top && top !== lastEv) toast(s.events[0].m);
    lastEv = top; firstLoad = false;
    $('log').innerHTML = s.events.map(e => '<li><b>' + e.t + '</b>&nbsp; ' + e.m + '</li>').join('') || '<li>-</li>';
  } catch (e) {
    $('dot').className = 'dot off';
    $('conn').textContent = T('off');
  }
}
let tm1 = setInterval(tick, 4000);
tick();

async function histo() {
  try {
    const h = await (await fetch('/api/history?_=' + Date.now(), { cache: 'no-store' })).json();
    const n = h.h.length;
    if (n < 2) {
      $('chart').innerHTML = '<text x="10" y="55" font-size="11" fill="gray">' + T('collect') + '</text>';
      return;
    }
    const W = 300, H = 100, m = Math.max(h.max, 1);
    const y = v => (H - 6 - (v / m) * (H - 16)).toFixed(1);
    const pts = h.h.map((v, i) => (i * W / (n - 1)).toFixed(1) + ',' + y(v)).join(' ');
    $('chart').innerHTML =
      '<line x1="0" y1="' + y(0) + '" x2="300" y2="' + y(0) + '" stroke="gray" stroke-opacity=".4"/>' +
      '<line x1="0" y1="' + y(m) + '" x2="300" y2="' + y(m) + '" stroke="gray" stroke-opacity=".4" stroke-dasharray="3"/>' +
      '<text x="2" y="' + (y(m) - 3) + '" font-size="9" fill="gray">max ' + m + '</text>' +
      '<polygon fill="#1faa6b" fill-opacity=".15" points="0,' + y(0) + ' ' + pts + ' 300,' + y(0) + '"/>' +
      '<polyline fill="none" stroke="#1faa6b" stroke-width="2.5" stroke-linejoin="round" points="' + pts + '"/>';
  } catch (e) {}
}
let tm3 = setInterval(histo, 30000);
histo();

function stopAll() { clearInterval(tm1); clearInterval(tm2); clearInterval(tm3); }
document.querySelectorAll('a[href="/admin"],a[href="/about"]').forEach(a => {
  a.onclick = e => {
    e.preventDefault();
    stopAll();
    const u = a.getAttribute('href');
    document.body.style.opacity = '.6';
    location.href = u;
    setTimeout(() => { window.stop(); location.href = u + '?r=1'; }, 7000);
    setTimeout(() => { window.stop(); location.href = u + '?r=2'; }, 14000);
  };
});

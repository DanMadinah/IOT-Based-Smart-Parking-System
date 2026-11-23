/**
 * Interactive ESP32 Simulator for GitHub Pages / local browser previews.
 * Excluded automatically from firmware/smart_parking/web_pages.h by build_web_header.py.
 */
(function () {
  const isDemo =
    location.protocol === 'file:' ||
    location.hostname.endsWith('github.io') ||
    location.hostname.endsWith('vercel.app') ||
    location.hostname.endsWith('netlify.app') ||
    location.hostname === 'localhost' ||
    location.hostname === '127.0.0.1' ||
    new URLSearchParams(location.search).get('demo') === '1';

  if (!isDemo) return;

  const KEY = 'sp_esp32_demo_v1';
  const START = Date.now();

  function defaultState() {
    return {
      cars: 2,
      max: 4,
      mode: 'simple',
      manual: false,
      muted: false,
      gate: 'Closed',
      entry: false,
      exit: false,
      d1: 12.4,
      d2: -1,
      rssi: -58,
      ap: false,
      inCount: 4,
      outCount: 2,
      peak: 3,
      paid: 4,
      ref: 0,
      ticketNo: 2,
      wrong: 0,
      unpaidUntil: 0,
      alertUntil: 0,
      tailUntil: 0,
      ac: [1, 0, 0, 0, 0, 0],
      tickets: [],
      activeCode: '',
      codeFor: '',
      history: [1, 1, 2, 2, 3, 3, 2, 2, 1, 2, 2],
      events: [
        { t: '+105s', m: 'Car entered (P-0002). Cars: 2' },
        { t: '+86s', m: 'Payment RM2 (Card, demo): P-0002 Myvi reserved' },
        { t: '+63s', m: 'Unpaid car at gate' },
        { t: '+20s', m: 'System started' }
      ],
      alerts: [
        { t: '+63s', m: 'Unpaid car at gate' }
      ]
    };
  }

  function load() {
    try {
      const raw = sessionStorage.getItem(KEY);
      if (raw) return JSON.parse(raw);
    } catch (e) {}
    const s = defaultState();
    save(s);
    return s;
  }

  function save(s) {
    try { sessionStorage.setItem(KEY, JSON.stringify(s)); } catch (e) {}
  }

  function stamp() {
    const d = new Date();
    return d.toTimeString().slice(0, 8);
  }

  function addEvent(s, m) {
    s.events.unshift({ t: stamp(), m });
    if (s.events.length > 25) s.events.pop();
  }

  function addAlert(s, m, kind) {
    addEvent(s, m);
    s.alerts.unshift({ t: stamp(), m });
    if (s.alerts.length > 10) s.alerts.pop();
    if (kind >= 0 && kind < 6) s.ac[kind]++;
    s.alertUntil = Date.now() + 5000;
  }

  function reservedCount(s) {
    return s.tickets.filter(x => ['reserved', 'armed', 'entering'].includes(x.state)).length;
  }

  function freeSpaces(s) {
    return Math.max(0, s.max - s.cars - reservedCount(s));
  }

  function padTicket(n) {
    return 'P-' + String(n).padStart(4, '0');
  }

  function updateDemoBanner(s) {
    const el = document.getElementById('demo-lcd');
    if (!el) return;
    if (s.activeCode) {
      el.innerHTML = 'Simulated LCD: <b>Code: ' + s.activeCode + '</b>';
      el.style.display = 'inline-block';
    } else {
      el.style.display = 'none';
    }
  }

  function completeEntry(token) {
    const s = load();
    const tk = s.tickets.find(x => x.t === token);
    if (!tk) return;
    tk.state = 'entering';
    s.gate = 'Open';
    s.entry = true;
    s.activeCode = '';
    s.codeFor = '';
    addEvent(s, 'Paid car ' + tk.ticket + ' (' + tk.n + '): gate opening');
    save(s);
    updateDemoBanner(s);

    setTimeout(() => {
      const s2 = load();
      const tk2 = s2.tickets.find(x => x.t === token);
      s2.gate = 'Closed';
      s2.entry = false;
      s2.cars = Math.min(s2.max, s2.cars + 1);
      s2.inCount++;
      if (s2.cars > s2.peak) s2.peak = s2.cars;
      s2.history.push(s2.cars);
      if (s2.history.length > 40) s2.history.shift();
      if (tk2) tk2.state = 'parked';
      addEvent(s2, 'Car entered (' + (tk2 ? tk2.ticket : '') + '). Cars: ' + s2.cars);
      save(s2);
    }, 2600);
  }

  function completeExit(token) {
    const s = load();
    const tk = s.tickets.find(x => x.t === token);
    if (!tk) return;
    s.gate = 'Open';
    s.exit = true;
    addEvent(s, 'Car leaving: ' + tk.n);
    save(s);

    setTimeout(() => {
      const s2 = load();
      const tk2 = s2.tickets.find(x => x.t === token);
      s2.gate = 'Closed';
      s2.exit = false;
      if (s2.cars > 0) {
        s2.cars--;
        s2.outCount++;
      }
      s2.history.push(s2.cars);
      if (s2.history.length > 40) s2.history.shift();
      if (tk2) tk2.state = 'left';
      addEvent(s2, 'Car left (' + (tk2 ? tk2.n : '') + '). Cars: ' + s2.cars);
      save(s2);
    }, 2200);
  }

  function handleApi(urlStr) {
    const u = new URL(urlStr, location.origin);
    const p = u.pathname;
    const q = u.searchParams;
    const s = load();
    const now = Date.now();

    // Expire old reservations
    s.tickets.forEach(tk => {
      if (tk.state === 'reserved' && now - tk.created > 60000) {
        tk.state = 'expired';
        s.ref++;
        addEvent(s, 'Reservation expired (demo refund): ' + tk.ticket);
      }
    });

    if (p.endsWith('/api/status')) {
      save(s);
      updateDemoBanner(s);
      return {
        cars: s.cars, max: s.max, free: freeSpaces(s), reserved: reservedCount(s),
        mode: s.mode, manual: s.manual, muted: s.muted, gate: s.gate,
        entry: s.entry, exit: s.exit, d1: s.d1, d2: s.d2, rssi: s.rssi, ap: s.ap,
        uptime: Math.floor((now - START) / 1000) + 120,
        in: s.inCount, out: s.outCount, peak: s.peak,
        unpaid: now < s.unpaidUntil, alert: now < s.alertUntil,
        warnNow: false, tail: now < s.tailUntil,
        warns: [], locked: 0, events: s.events.slice(0, 12)
      };
    }

    if (p.endsWith('/api/history')) {
      return { max: s.max, step: 20, h: s.history };
    }

    if (p.endsWith('/api/pay')) {
      if (freeSpaces(s) <= 0) return { ok: false, msg: 'Lot full, try again later' };
      const method = q.get('method') || 'Card';
      const name = (q.get('name') || 'Car').slice(0, 14);
      s.ticketNo++;
      s.paid++;
      const token = Math.random().toString(16).slice(2, 10);
      const ticket = padTicket(s.ticketNo);
      s.tickets.push({ t: token, n: name, ticket, state: 'reserved', created: now, wrong: 0 });
      addEvent(s, 'Payment RM2 (' + method + ', demo): ' + ticket + ' ' + name + ' reserved');
      save(s);
      return { ok: true, token, msg: 'Paid. Space reserved for 1 minute.' };
    }

    if (p.endsWith('/api/tickets')) {
      const ids = (q.get('t') || '').split(',').filter(Boolean);
      const out = [];
      ids.forEach(id => {
        const tk = s.tickets.find(x => x.t === id);
        if (!tk) return;
        let left = 0, total = 60, needCode = false;
        if (tk.state === 'reserved') {
          left = Math.max(0, Math.ceil((60000 - (now - tk.created)) / 1000));
        } else if (tk.state === 'armed') {
          total = 20;
          left = Math.max(0, Math.ceil((20000 - (now - (tk.armedAt || now))) / 1000));
          needCode = s.mode === 'secure' && s.codeFor === tk.t;
        }
        out.push({
          t: tk.t, n: tk.n, ticket: tk.ticket, state: tk.state,
          left, total, locked: 0, atgate: false, needCode
        });
      });
      save(s);
      return out;
    }

    if (p.endsWith('/api/arm')) {
      const tk = s.tickets.find(x => x.t === q.get('t'));
      if (!tk) return { ok: false, msg: 'Unknown ticket' };
      tk.state = 'armed';
      tk.armedAt = now;
      addEvent(s, "Driver tapped I'm at the gate: " + tk.ticket);
      if (s.mode === 'secure') {
        s.activeCode = String(Math.floor(1000 + Math.random() * 9000));
        s.codeFor = tk.t;
        addEvent(s, 'LCD code shown for ' + tk.ticket);
        save(s);
        updateDemoBanner(s);
        return { ok: true, msg: 'Gate armed. Enter the 4-digit code shown on the simulated LCD above.' };
      }
      save(s);
      setTimeout(() => completeEntry(tk.t), 700);
      return { ok: true, msg: 'Gate armed. Opening gate...' };
    }

    if (p.endsWith('/api/code')) {
      const tk = s.tickets.find(x => x.t === q.get('t'));
      const c = q.get('c') || '';
      if (!tk) return { ok: false, msg: 'No valid ticket' };
      if (s.codeFor === tk.t && c === s.activeCode) {
        save(s);
        setTimeout(() => completeEntry(tk.t), 300);
        return { ok: true, msg: 'Code accepted. Drive in.' };
      }
      s.wrong++;
      tk.wrong = (tk.wrong || 0) + 1;
      save(s);
      return { ok: false, msg: 'Wrong code (check simulated LCD banner at top)' };
    }

    if (p.endsWith('/api/cancel')) {
      const tk = s.tickets.find(x => x.t === q.get('t'));
      if (!tk) return { ok: false, msg: 'Nothing to cancel' };
      tk.state = 'cancelled';
      s.ref++;
      if (s.codeFor === tk.t) { s.activeCode = ''; s.codeFor = ''; }
      addEvent(s, 'Reservation cancelled (demo refund): ' + tk.ticket);
      save(s);
      updateDemoBanner(s);
      return { ok: true, msg: 'Cancelled. Payment refunded (demo).' };
    }

    if (p.endsWith('/api/exitarm')) {
      const tk = s.tickets.find(x => x.t === q.get('t'));
      if (!tk || tk.state !== 'parked') return { ok: false, msg: 'This car is not parked' };
      tk.state = 'exiting';
      addEvent(s, "Driver tapped I'm exiting: " + tk.ticket + ' (' + tk.n + ')');
      save(s);
      setTimeout(() => completeExit(tk.t), 700);
      return { ok: true, msg: 'Exit armed. Opening gate...' };
    }

    if (p.endsWith('/api/alerts')) {
      return { c: s.ac, wrong: s.wrong, paid: s.paid, ref: s.ref, list: s.alerts };
    }

    if (p.endsWith('/api/admin')) {
      const cmd = q.get('cmd');
      const v = parseInt(q.get('v') || '0', 10);
      let msg = 'Done';
      if (cmd === 'manual') {
        s.manual = v === 1;
        addEvent(s, s.manual ? 'Admin: manual mode ON' : 'Admin: manual mode OFF');
        msg = s.manual ? 'Manual mode on. Gate is yours.' : 'Manual mode off. Automatic again.';
      } else if (cmd === 'gate') {
        if (!s.manual) return { ok: false, msg: 'Turn manual mode on first' };
        s.gate = v === 1 ? 'Open' : 'Closed';
        addEvent(s, v === 1 ? 'Admin: gate opened' : 'Admin: gate closed');
        msg = v === 1 ? 'Gate opened.' : 'Gate closed.';
      } else if (cmd === 'reset') {
        s.cars = 0; addEvent(s, 'Admin: car count reset to 0'); msg = 'Car count reset to 0';
      } else if (cmd === 'clearstats') {
        s.inCount = 0; s.outCount = 0; s.peak = s.cars; s.paid = 0; s.ref = 0;
        addEvent(s, 'Admin: statistics cleared'); msg = 'Statistics cleared';
      } else if (cmd === 'clearalerts') {
        s.ac = [0, 0, 0, 0, 0, 0]; s.wrong = 0; s.alerts = [];
        addEvent(s, 'Admin: alerts cleared'); msg = 'Alerts cleared';
      } else if (cmd === 'cleartail') {
        s.tailUntil = 0; addEvent(s, 'Admin: tailgating alarm cleared'); msg = 'Tailgating alarm cleared';
      } else if (cmd === 'approveexit') {
        addEvent(s, 'Admin: exit approved for the next car (30 s)'); msg = 'Next car may leave within 30 s';
      } else if (cmd === 'mode') {
        s.mode = v === 1 ? 'secure' : 'simple';
        addEvent(s, v === 1 ? 'Admin: secure mode on (LCD code)' : 'Admin: simple mode on');
        msg = v === 1 ? 'Secure mode on' : 'Simple mode on';
      } else if (cmd === 'mute') {
        s.muted = v === 1;
        addEvent(s, s.muted ? 'Admin: buzzer muted' : 'Admin: buzzer on');
        msg = s.muted ? 'Buzzer muted' : 'Buzzer on';
      } else if (cmd === 'cap') {
        if (v < 1 || v > 12) return { ok: false, msg: 'Capacity must be 1 to 12' };
        s.max = v; addEvent(s, 'Admin: capacity set to ' + v); msg = 'Capacity is now ' + v;
      } else if (cmd === 'buzz') {
        msg = 'Buzzer test triggered (simulated)';
      } else if (cmd === 'sim') {
        if (v === 1 && freeSpaces(s) > 0) {
          s.cars++; s.inCount++; s.peak = Math.max(s.peak, s.cars);
          s.history.push(s.cars); addEvent(s, 'Demo: paid car entered. Cars: ' + s.cars);
          msg = 'Simulated paid car entering.';
        } else if (v === 2) {
          s.unpaidUntil = now + 4000; addAlert(s, 'Demo: unpaid car at gate', 0);
          msg = 'Simulated unpaid car alert.';
        } else if (v === 3 && s.cars > 0) {
          s.cars--; s.outCount++; s.history.push(s.cars);
          addEvent(s, 'Demo: car left. Cars: ' + s.cars);
          msg = 'Simulated car leaving.';
        }
      } else if (cmd === 'demo') {
        addEvent(s, 'Demo: simulated full sequence'); msg = 'Demo event logged (use dashboard to test full flow).';
      } else if (cmd === 'restart') {
        const fresh = defaultState();
        fresh.cars = 0; fresh.inCount = 0; fresh.outCount = 0; fresh.peak = 0;
        fresh.paid = 0; fresh.ref = 0; fresh.alerts = []; fresh.ac = [0, 0, 0, 0, 0, 0];
        fresh.events = [{ t: stamp(), m: 'Admin: everything restarted' }];
        save(fresh);
        return { ok: true, msg: 'Everything restarted to the beginning' };
      }
      save(s);
      return { ok: true, msg };
    }

    return { ok: false, msg: 'Unknown endpoint' };
  }

  // Intercept /api/* fetch calls
  const origFetch = window.fetch.bind(window);
  window.fetch = function (input, init) {
    const url = typeof input === 'string' ? input : (input && input.url) || '';
    if (url.includes('/api/')) {
      const data = handleApi(url);
      return Promise.resolve(
        new Response(JSON.stringify(data), {
          status: 200,
          headers: { 'Content-Type': 'application/json' }
        })
      );
    }
    return origFetch(input, init);
  };

  // UI helpers for demo mode (banner, relative links, CSV download, About placeholders)
  document.addEventListener('DOMContentLoaded', () => {
    const bar = document.createElement('div');
    bar.style.cssText =
      'background:#14202b;color:#eaf1f7;font:13px/1.4 system-ui,sans-serif;' +
      'padding:8px 14px;text-align:center;border-bottom:1px solid rgba(255,255,255,.15);' +
      'display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:center;';
    bar.innerHTML =
      '<span><b>Interactive Demo Mode</b> — simulated ESP32 backend</span>' +
      '<span id="demo-lcd" style="display:none;background:#1faa6b;color:#fff;padding:2px 8px;border-radius:6px"></span>' +
      '<a href="index.html" style="color:#74c0fc">Dashboard</a>' +
      '<a href="admin.html" style="color:#74c0fc">Admin</a>' +
      '<a href="about.html" style="color:#74c0fc">About</a>';
    document.body.prepend(bar);
    updateDemoBanner(load());

    // Rewrite absolute ESP32 links to relative files for GitHub Pages
    const map = { '/': 'index.html', '/admin': 'admin.html', '/about': 'about.html' };
    document.querySelectorAll('a[href]').forEach(a => {
      const h = a.getAttribute('href');
      if (map[h]) a.setAttribute('href', map[h]);
      if (h === '/download.csv') {
        a.onclick = e => {
          e.preventDefault();
          const s = load();
          let csv = 'time,event\n' + s.events.map(ev => ev.t + ',' + ev.m.replace(/,/g, ';')).join('\n');
          const blob = new Blob([csv], { type: 'text/csv' });
          const link = document.createElement('a');
          link.href = URL.createObjectURL(blob);
          link.download = 'parking-log.csv';
          link.click();
        };
      }
    });

    // Replace ESP32 template placeholders on about.html
    if (document.body.innerHTML.includes('%TITLE%')) {
      document.body.innerHTML = document.body.innerHTML
        .replace(/%TITLE%/g, 'Smart Parking System')
        .replace(/%NAME%/g, 'Muhammad Abdullahi Bukar')
        .replace(/%SUP%/g, 'Mohd Zaki Shahabuddin');
    }
  });
})();

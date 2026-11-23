/**
 * Live Firebase Realtime Database bridge for Vercel / GitHub Pages.
 * Connects the public web dashboard directly to your physical ESP32 hardware
 * (zero demo simulator). Excluded from ESP32 PROGMEM builds automatically.
 */
(function () {
  const dbUrl = (window.FIREBASE_DB_URL || '').replace(/\/+$/, '');
  if (!dbUrl) return; // When empty or running directly on ESP32, use native /api/*

  const origFetch = window.fetch.bind(window);

  async function fbGet(path) {
    const res = await origFetch(dbUrl + path + '.json', { cache: 'no-store' });
    if (!res.ok) throw new Error('Firebase read failed');
    return await res.json();
  }

  async function fbPut(path, body) {
    const res = await origFetch(dbUrl + path + '.json', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    if (!res.ok) throw new Error('Firebase write failed');
    return await res.json();
  }

  async function fbDelete(path) {
    try { await origFetch(dbUrl + path + '.json', { method: 'DELETE' }); } catch (e) {}
  }

  async function sendCommand(action, arg1, arg2) {
    const id = 'c' + Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
    await fbPut('/parking/command', { id, action, arg1: String(arg1 || ''), arg2: String(arg2 || '') });

    const deadline = Date.now() + 7000;
    while (Date.now() < deadline) {
      await new Promise((r) => setTimeout(r, 200));
      const resp = await fbGet('/parking/responses/' + id);
      if (resp && typeof resp === 'object') {
        fbDelete('/parking/responses/' + id);
        return resp;
      }
    }
    return { ok: false, msg: 'ESP32 did not respond (is it powered on & connected to Wi-Fi?)' };
  }

  function jsonResp(data, status = 200) {
    return new Response(JSON.stringify(data), {
      status,
      headers: { 'Content-Type': 'application/json' },
    });
  }

  window.fetch = async function (input, init) {
    const urlStr = typeof input === 'string' ? input : (input && input.url) || '';
    if (!urlStr.includes('/api/')) return origFetch(input, init);

    const u = new URL(urlStr, location.origin);
    const p = u.pathname;
    const q = u.searchParams;

    try {
      if (p.endsWith('/api/status')) {
        const s = await fbGet('/parking/status');
        if (!s) throw new Error('No ESP32 status yet');
        s.warns = Array.isArray(s.warns) ? s.warns : [];
        s.events = Array.isArray(s.events) ? s.events : [];
        return jsonResp(s);
      }

      if (p.endsWith('/api/history')) {
        const h = (await fbGet('/parking/history')) || { max: 4, step: 20, h: [] };
        h.h = Array.isArray(h.h) ? h.h : [];
        return jsonResp(h);
      }

      if (p.endsWith('/api/alerts')) {
        const a = (await fbGet('/parking/alerts')) || { c: [0, 0, 0, 0, 0, 0], wrong: 0, paid: 0, ref: 0, list: [] };
        a.c = Array.isArray(a.c) ? a.c : [0, 0, 0, 0, 0, 0];
        a.list = Array.isArray(a.list) ? a.list : [];
        return jsonResp(a);
      }

      if (p.endsWith('/api/tickets')) {
        const all = (await fbGet('/parking/tickets')) || {};
        const ids = (q.get('t') || '').split(',').filter(Boolean);
        const list = ids.map((id) => all[id]).filter(Boolean);
        return jsonResp(list);
      }

      if (p.endsWith('/api/pay')) {
        return jsonResp(await sendCommand('pay', q.get('method') || 'Card', q.get('name') || 'Car'));
      }
      if (p.endsWith('/api/arm')) {
        return jsonResp(await sendCommand('arm', q.get('t') || '', ''));
      }
      if (p.endsWith('/api/cancel')) {
        return jsonResp(await sendCommand('cancel', q.get('t') || '', ''));
      }
      if (p.endsWith('/api/code')) {
        return jsonResp(await sendCommand('code', q.get('t') || '', q.get('c') || ''));
      }
      if (p.endsWith('/api/exitarm')) {
        return jsonResp(await sendCommand('exitarm', q.get('t') || '', ''));
      }
      if (p.endsWith('/api/admin')) {
        return jsonResp(await sendCommand('admin', q.get('cmd') || '', q.get('v') || '0'));
      }
    } catch (err) {
      return jsonResp({ ok: false, msg: 'ESP32 offline or Firebase unreachable' }, 503);
    }

    return origFetch(input, init);
  };

  document.addEventListener('DOMContentLoaded', () => {
    const map = { '/': 'index.html', '/admin': 'admin.html', '/about': 'about.html' };
    document.querySelectorAll('a[href]').forEach((a) => {
      const h = a.getAttribute('href');
      if (map[h]) a.setAttribute('href', map[h]);
      if (h === '/download.csv') {
        a.onclick = async (e) => {
          e.preventDefault();
          const s = (await fbGet('/parking/status')) || { events: [] };
          const csv = 'time,event\n' + (s.events || []).map((ev) => ev.t + ',' + ev.m.replace(/,/g, ';')).join('\n');
          const blob = new Blob([csv], { type: 'text/csv' });
          const link = document.createElement('a');
          link.href = URL.createObjectURL(blob);
          link.download = 'parking-log.csv';
          link.click();
        };
      }
    });

    if (document.body.innerHTML.includes('%TITLE%')) {
      document.body.innerHTML = document.body.innerHTML
        .replace(/%TITLE%/g, 'Smart Parking System')
        .replace(/%NAME%/g, 'Muhammad Abdullahi Bukar')
        .replace(/%SUP%/g, 'Mohd Zaki Shahabuddin');
    }
  });
})();

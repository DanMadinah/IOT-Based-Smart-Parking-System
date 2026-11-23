/**
 * Live Firebase Realtime Database bridge for Vercel / GitHub Pages.
 * Connects the public web dashboard directly to your physical ESP32 hardware
 * with ESP32-verified Admin Login (zero passwords stored in frontend JS).
 */
(function () {
  const dbUrl = (window.FIREBASE_DB_URL || '').replace(/\/+$/, '');
  if (!dbUrl) return;

  const origFetch = window.fetch.bind(window);
  const AUTH_KEY = 'sp_admin_pass_v1';

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

  async function sendCommand(action, arg1, arg2, auth) {
    const id = 'c' + Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
    await fbPut('/parking/command', {
      id,
      action,
      arg1: String(arg1 || ''),
      arg2: String(arg2 || ''),
      auth: String(auth || ''),
    });

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
        return jsonResp(await sendCommand('pay', q.get('method') || 'Card', q.get('name') || 'Car', ''));
      }
      if (p.endsWith('/api/arm')) {
        return jsonResp(await sendCommand('arm', q.get('t') || '', '', ''));
      }
      if (p.endsWith('/api/cancel')) {
        return jsonResp(await sendCommand('cancel', q.get('t') || '', '', ''));
      }
      if (p.endsWith('/api/code')) {
        return jsonResp(await sendCommand('code', q.get('t') || '', q.get('c') || '', ''));
      }
      if (p.endsWith('/api/exitarm')) {
        return jsonResp(await sendCommand('exitarm', q.get('t') || '', '', ''));
      }
      if (p.endsWith('/api/admin')) {
        const pass = sessionStorage.getItem(AUTH_KEY) || '';
        return jsonResp(await sendCommand('admin', q.get('cmd') || '', q.get('v') || '0', pass));
      }
    } catch (err) {
      return jsonResp({ ok: false, msg: 'ESP32 offline or Firebase unreachable' }, 503);
    }

    return origFetch(input, init);
  };

  function setupAdminGate() {
    const isAdminPage = /\/admin(\.html)?$/i.test(location.pathname);
    if (!isAdminPage) return;

    const main = document.querySelector('main');
    const saved = sessionStorage.getItem(AUTH_KEY);

    function addLogoutButton() {
      const h1 = document.querySelector('h1');
      if (!h1 || document.getElementById('sp-logout')) return;
      const btn = document.createElement('button');
      btn.id = 'sp-logout';
      btn.textContent = 'Log out';
      btn.style.cssText = 'float:right;font-size:13px;padding:6px 12px;margin:0';
      btn.onclick = () => {
        sessionStorage.removeItem(AUTH_KEY);
        location.reload();
      };
      h1.appendChild(btn);
    }

    if (saved) {
      addLogoutButton();
      return;
    }

    if (main) main.style.display = 'none';

    const box = document.createElement('div');
    box.id = 'sp-login-gate';
    box.style.cssText =
      'max-width:380px;margin:80px auto;padding:24px;border-radius:16px;' +
      'background:var(--card);border:1px solid var(--line);box-shadow:0 12px 32px rgba(0,0,0,.15);';
    box.innerHTML =
      '<h2 style="margin:0 0 6px;font-size:20px;color:var(--text)">Admin Login</h2>' +
      '<p class="sub" style="margin:0 0 16px">Sign in with your ESP32 admin credentials.</p>' +
      '<form id="sp-login-form">' +
      '<label class="sub" style="display:block;margin-bottom:4px">Username</label>' +
      '<input id="sp-user" value="admin" required style="width:100%;padding:10px;border-radius:10px;border:1px solid var(--line);background:var(--bg);color:var(--text);margin-bottom:12px;font-size:15px">' +
      '<label class="sub" style="display:block;margin-bottom:4px">Password</label>' +
      '<input id="sp-pass" type="password" placeholder="Enter admin password" required autofocus style="width:100%;padding:10px;border-radius:10px;border:1px solid var(--line);background:var(--bg);color:var(--text);margin-bottom:14px;font-size:15px">' +
      '<button type="submit" class="g" style="width:100%;margin:0">Sign in</button>' +
      '<div id="sp-err" class="sub" style="margin-top:10px;color:var(--red)"></div>' +
      '</form>' +
      '<p class="sub" style="margin-top:16px"><a href="index.html">Back to dashboard</a></p>';

    document.body.prepend(box);

    document.getElementById('sp-login-form').onsubmit = async (e) => {
      e.preventDefault();
      const u = (document.getElementById('sp-user').value || '').trim();
      const pw = document.getElementById('sp-pass').value || '';
      const err = document.getElementById('sp-err');
      if (u !== 'admin') {
        err.textContent = 'Username must be admin';
        return;
      }
      err.style.color = 'var(--sub)';
      err.textContent = 'Verifying with ESP32...';
      const r = await sendCommand('authcheck', '', '', pw);
      if (r && r.ok) {
        sessionStorage.setItem(AUTH_KEY, pw);
        box.remove();
        if (main) main.style.display = '';
        addLogoutButton();
      } else {
        err.style.color = 'var(--red)';
        err.textContent = (r && r.msg) || 'Invalid admin password';
      }
    };
  }

  document.addEventListener('DOMContentLoaded', () => {
    setupAdminGate();

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

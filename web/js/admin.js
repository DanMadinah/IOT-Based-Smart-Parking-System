const $ = i => document.getElementById(i);
let cap = 4;

async function go(c, v) {
  $('msg').textContent = 'Sending...';
  try {
    const r = await (await fetch('/api/admin?cmd=' + c + '&v=' + v + '&_=' + Date.now(), { cache: 'no-store' })).json();
    $('msg').textContent = r.msg || 'Done';
  } catch (e) {
    $('msg').textContent = 'Connection problem. Try again.';
  }
  st();
  al();
}

async function st() {
  try {
    const s = await (await fetch('/api/status?_=' + Date.now(), { cache: 'no-store' })).json();
    cap = s.max;
    $('capt').textContent = 'Capacity: ' + s.max;
    $('st').innerHTML =
      'Cars <b>' + s.cars + '/' + s.max + '</b> &middot; ' +
      'Reserved <b>' + s.reserved + '</b> &middot; ' +
      'Gate <b>' + s.gate + '</b><br>' +
      'Control: <b>' + (s.manual ? 'MANUAL' : 'Automatic') + '</b> &middot; ' +
      'Entry mode: <b>' + (s.mode === 'secure' ? 'Secure' : 'Simple') + '</b> &middot; ' +
      'Buzzer: <b>' + (s.muted ? 'MUTED' : 'on') + '</b>';
  } catch (e) {
    $('st').textContent = 'Cannot reach the lot';
  }
}

async function al() {
  try {
    const a = await (await fetch('/api/alerts?_=' + Date.now(), { cache: 'no-store' })).json();
    const c = a.c;
    $('rev').innerHTML =
      'Revenue <b>RM ' + (Math.max(a.paid - a.ref, 0) * 2).toFixed(2) + '</b> &middot; ' +
      'Payments <b>' + a.paid + '</b> &middot; ' +
      'Refunds <b>' + a.ref + '</b>';
    $('al').innerHTML =
      'Unpaid cars <b>' + c[0] + '</b> &middot; ' +
      'Not confirmed <b>' + c[1] + '</b> &middot; ' +
      'Ticket locks <b>' + c[2] + '</b> &middot; ' +
      'Invalid attempts <b>' + c[3] + '</b> &middot; ' +
      'Blocked tickets <b>' + c[4] + '</b> &middot; ' +
      'Tailgating <b>' + c[5] + '</b> &middot; ' +
      'Wrong codes <b>' + a.wrong + '</b>';
    $('alist').innerHTML =
      a.list.map(e => '<li><b>' + e.t + '</b>&nbsp; ' + e.m + '</li>').join('') || '<li>No alerts</li>';
  } catch (e) {}
}

setInterval(st, 5000);
setInterval(al, 6000);
st();
al();

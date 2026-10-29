// Checks the web page's known-CAN-ID decoder (pulled straight out of web_ui.h) against the shared
// test frames in known_ids_vectors.json. The Android decoder (KnownIds.kt) must give the same text.
// Run: node test/test_known_ids.js
const fs = require('fs');
const path = require('path');
const here = __dirname;
const ui = fs.readFileSync(path.join(here, '../firmware/obduke/web_ui.h'), 'utf8');
const a = ui.indexOf('// ---------- known KTM CAN IDs');
const b = ui.indexOf('let snOpen');
if (a < 0 || b < 0) { console.error('FAIL: could not find the known-ID block in web_ui.h'); process.exit(1); }
const lib = new Function(ui.slice(a, b) + '\nreturn {KIDS,kinfo,kval,ksig};')();
const vec = JSON.parse(fs.readFileSync(path.join(here, 'known_ids_vectors.json'), 'utf8'));
let fails = 0, checks = 0;
const bad = m => { fails++; console.log('FAIL: ' + m); };

for (const v of vec.decode) {
  const e = lib.kinfo(v.id);
  if (!e) { bad(v.id + ' not in table'); continue; }
  const d = v.data.split(' ').map(h => parseInt(h, 16));
  for (const x of v.expect) {
    checks++;
    const f = e.fields[x.f];
    const r = lib.kval(f, d);
    if (x.t === null) { if (r !== null) bad(`${v.id} f${x.f} should be null, got ${JSON.stringify(r)}`); continue; }
    if (!r) { bad(`${v.id} f${x.f} (${f.name}) gave null for ${v.data}`); continue; }
    if (r.t !== x.t || r.ok !== x.ok) bad(`${v.id} f${x.f} (${f.name}) ${v.data}: got "${r.t}" ok=${r.ok}, want "${x.t}" ok=${x.ok}`);
  }
}
for (const s of vec.signal) {
  checks++;
  const e = lib.kinfo(s.id), line = lib.ksig(e, e.fields[s.f]);
  if (line !== s.line) bad(`signal ${s.id} f${s.f}: got "${line}", want "${s.line}"`);
}
for (const l of vec.lookup) {
  checks++;
  const e = lib.kinfo(l.id);
  const name = e ? e.name : null;
  if (name !== l.name) bad(`lookup ${l.id}: got ${name}, want ${l.name}`);
}
// every table entry must be well formed enough to render
for (const e of lib.KIDS.ids) {
  for (const f of e.fields || []) {
    checks++;
    if (f.text) continue;
    const r = lib.kval(f, [0, 0, 0, 0, 0, 0, 0, 0]);
    if (!r || typeof r.t !== 'string') bad(`${e.id} ${f.name}: no value for an all-zero frame`);
  }
}
console.log(`${fails} failure(s) in ${checks} checks`);
process.exit(fails ? 1 : 0);

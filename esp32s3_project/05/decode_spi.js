const fs = require('fs');
const path = require('path');

const base = process.argv[2];
const FS = 5000000;
const BLK = 1048576;

function loadCh(ch) {
  const dir = path.join(base, String(ch));
  const ini = fs.readFileSync(path.join(dir, 'channel.ini'), 'utf8')
    .split(/\r?\n/).map(s => s.replace(/^\uFEFF/, '').trim());
  const total = parseInt(ini[2], 10) || 0;
  let bitmap = 0n;
  for (const l of ini) {
    const m = l.match(/^(\d+),(\d+)$/);
    if (m) { bitmap = BigInt(m[1]); break; }
  }
  const present = new Set(fs.readdirSync(dir)
    .filter(f => /^0-\d+\.bin$/.test(f))
    .map(f => parseInt(f.match(/0-(\d+)\.bin/)[1], 10)));
  let maxIdx = present.size ? Math.max(...present) : -1;
  for (let b = 0; b < 64; b++) if ((bitmap >> BigInt(b)) & 1n) maxIdx = Math.max(maxIdx, b);
  if (total > 0) maxIdx = Math.max(maxIdx, Math.ceil(total / (BLK * 8)) - 1);
  const zero = Buffer.alloc(BLK);
  const parts = [];
  for (let i = 0; i <= maxIdx; i++) {
    parts.push(present.has(i) ? fs.readFileSync(path.join(dir, `0-${i}.bin`)) : zero);
  }
  let buf = Buffer.concat(parts);
  if (total > 0) {
    const bytes = Math.ceil(total / 8);
    if (buf.length > bytes) buf = buf.subarray(0, bytes);
  }
  return { buf, total: total || buf.length * 8, blocks: maxIdx + 1, present: present.size };
}

const ch = [0, 1, 2, 3].map(loadCh);
console.log('channels:', ch.map(c => `${c.total} samples / ${c.blocks} blocks (${c.present} stored)`).join(' | '));
const bit = (b, s) => (b[s >> 3] >> (s & 7)) & 1;
const N = Math.min(...ch.map(c => c.total));

let prev = 0;
const edges = [];
for (let s = 0; s < N; s++) {
  const clk = bit(ch[1].buf, s);
  if (clk && !prev) edges.push(s);
  prev = clk;
}
console.log('CLK rising edges:', edges.length);
if (!edges.length) process.exit(0);
const t0 = edges[0];
console.log('first edge t =', (t0 / FS).toFixed(6), 's');

const out = [];
for (let k = 0; k + 8 <= edges.length && out.length < 160; k += 8) {
  let v = 0;
  for (let j = 0; j < 8; j++) v = (v << 1) | bit(ch[2].buf, edges[k + j]);
  const dc = bit(ch[3].buf, edges[k]);
  const cs = bit(ch[0].buf, edges[k]);
  out.push({ t: (edges[k] - t0) / FS * 1000, v, dc, cs });
}
for (const b of out) {
  console.log(`${b.t.toFixed(4)}ms  cs=${b.cs} dc=${b.dc}  0x${b.v.toString(16).padStart(2, '0')}`);
}

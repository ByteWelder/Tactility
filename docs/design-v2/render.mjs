// Renders every screen at 1x/2x, the contact sheet, and dumps layout checks + font sizes.
// Usage: PW=<node_modules dir that has playwright> node render.mjs
import { createRequire } from 'node:module';
import { writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const here = path.dirname(fileURLToPath(import.meta.url));
const { chromium } = createRequire(path.join(process.env.PW, 'x.js'))('playwright');
const url = 'file://' + path.join(here, 'src/index.html');
const browser = await chromium.launch(process.env.CHROME ? { executablePath: process.env.CHROME } : {});

for (const scale of [1, 2]) {
  const page = await browser.newPage({ deviceScaleFactor: scale, viewport: { width: 2400, height: 420 } });
  await page.goto(url);
  await page.evaluate(() => document.fonts.ready);
  for (const id of ['home', 'lock', 'quick', 'recents', 'settings', 'wifi', 'files', 'dialog', 'keyboard'])
    await page.locator('#s-' + id).screenshot({ path: `${here}/png/${scale}x/${id}.png` });
  await page.locator('main.board').screenshot({ path: `${here}/png/contact-sheet${scale === 2 ? '@2x' : ''}.png` });

  if (scale === 1) {
    const report = await page.evaluate(() => {
      const box = (el) => el.getBoundingClientRect();
      const out = {};
      // grid labels: width and gap to the neighbour on the same row
      const labels = [...document.querySelectorAll('#s-home .app .t12')].map((el) => ({ text: el.textContent, w: el.scrollWidth, shown: box(el).width, l: box(el).left, r: box(el).right }));
      out.gridLabels = labels.map((a, i) => ({ ...a, truncated: a.w > a.shown + 0.5, gapRight: i % 4 < 3 ? +(labels[i + 1].l - a.r).toFixed(1) : null }));
      const o = (sel) => { const s = document.querySelector(sel); const b = box(s), r = box(s.closest('.scr')); return { x: +(b.left - r.left).toFixed(1), y: +(b.top - r.top).toFixed(1), w: +b.width.toFixed(1), h: +b.height.toFixed(1) }; };
      out.grid = o('#s-home .grid'); out.dock = o('#s-home .dock');
      out.recentsX = o('#s-recents .x'); out.recentsList = o('#s-recents .rc');
      out.key = o('#s-keyboard .k'); out.keyboard = o('#s-keyboard .kb');
      out.dialogBtn = o('#s-dialog > .dlg .btn'); out.dialog = o('#s-dialog > .dlg');
      out.qtile = o('#s-quick .qt'); out.slider = o('#s-quick .sl'); out.thumb = o('#s-quick .thumb');
      out.overflow = [...document.querySelectorAll('.ell')].filter((e) => e.scrollWidth > e.clientWidth).map((e) => e.textContent);
      // dialog title must fit on one line
      out.dialogTitleH = box(document.querySelector('#s-dialog > .dlg .t22')).height;

      // LVGL 4 bpp bitmap estimate: per glyph ceil(w*h/2) bytes + 8 B glyph descriptor, + ~1 KB cmap/header
      const ascii = [...Array(95)].map((_, i) => String.fromCharCode(32 + i));
      const c = document.createElement('canvas').getContext('2d');
      const est = (weight, size, chars) => {
        c.font = `${weight} ${size}px Inter`;
        let bytes = 0;
        for (const ch of chars) {
          const m = c.measureText(ch);
          const w = Math.ceil(m.actualBoundingBoxLeft + m.actualBoundingBoxRight), h = Math.ceil(m.actualBoundingBoxAscent + m.actualBoundingBoxDescent);
          bytes += Math.ceil((w * h) / 2) + 8;
        }
        return Math.round(bytes + 1024);
      };
      out.fonts = {
        'Regular 12 ASCII': est(400, 12, ascii),
        'Medium 13 ASCII': est(500, 13, ascii),
        'Medium 14 ASCII': est(500, 14, ascii),
        'Medium 15 ASCII': est(500, 15, ascii),
        'Medium 22 ASCII': est(500, 22, ascii),
        'Medium 48 "0123456789:"': est(500, 48, [...'0123456789:']),
      };
      return out;
    });
    writeFileSync(`${here}/png/checks.json`, JSON.stringify(report, null, 2));
  }
  await page.close();
}
await browser.close();

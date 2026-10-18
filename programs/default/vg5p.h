// =====================================================================
// vg5p.h — the Set 5 pen-plot engine (by Dewey)
//
// After di_diff_world's pen plots with airbrush: every surface is drawn as a
// flat tone covered in a fine square mesh of pen lines, and shading is a
// soft airbrushed darkening (deeper = darker). This file has the shared
// palette and the face drawing used by SHAFT, PIPEWORKS, PLOTTER and
// GREEBLE (Set 5, slots 4-7).
//
// Palette: two tone ramps.
//   0-127   ramp A: ink (0) → paper (127), through an optional tint
//   128-255 ramp B: the same for a second, coloured light (used for
//           coloured shadows and lit faces in some presets)
// Pen lines on a face are the face's own tone made darker by `p5_ink`.
// =====================================================================
#ifndef VG5P_H
#define VG5P_H
#include "vg3.h"
#include "vg5fx.h"

struct P5Look { uint8_t ink[3], mid[3], paper[3], inkB[3], midB[3], paperB[3]; };
static int p5_ink = 46;                              // how much darker the pen lines are than their face

// build both ramps: ink → mid → paper (gamma bends the ramp: <1 = lighter mid-tones)
static const P5Look* p5_palWas = 0; static float p5_gammaWas = -1; static int p5_palAge = 0;
static void p5_paletteDirty() { p5_palWas = 0; }
static void p5_palette(const P5Look& L, float gamma) {
  if (!fx5_palReset && p5_palWas == &L && p5_gammaWas == gamma && ++p5_palAge < 120) return;   // (always rebuilt while the colour knob is working on it)   // only rebuilt when it changes (and every 2 s, to be safe)
  p5_palWas = &L; p5_gammaWas = gamma; p5_palAge = 0;
  for (int i = 0; i < 128; i++) {
    float u = powf(i / 127.0f, gamma);
    int k = (int)(u * 512);
    for (int r = 0; r < 2; r++) {
      const uint8_t* a = r ? L.inkB : L.ink; const uint8_t* m = r ? L.midB : L.mid; const uint8_t* p = r ? L.paperB : L.paper;
      uint8_t c[3];
      for (int q = 0; q < 3; q++) c[q] = k < 256 ? v3_mix8(a[q], m[q], k) : v3_mix8(m[q], p[q], k - 256);
      display.setColor((uint8_t)(r * 128 + i), c[0], c[1], c[2]);
    }
  }
}
static inline uint8_t p5_tone(int t, int ramp) { return (uint8_t)(ramp * 128 + v3_clamp(t, 0, 127)); }
static inline uint8_t p5_line(int t, int ramp) { return (uint8_t)(ramp * 128 + v3_clamp(t - p5_ink, 0, 127)); }

// a filled convex polygon (n ≤ 8), corners in 1/16 px. Walks each edge once with a
// fixed-point step (one 32-bit divide per edge), so it is much faster on the board than v3_poly.
static int16_t p5_xl[H], p5_xr[H];
static void p5_fill(const int32_t* xs0, const int32_t* ys0, int n, uint8_t c) {
  if (n < 3 || n > 8) return;
  int32_t xs[8], ys[8], ymin = 1 << 30, ymax = -(1 << 30);
  for (int i = 0; i < n; i++) {
    xs[i] = xs0[i] < -8000 ? -8000 : (xs0[i] > 13000 ? 13000 : xs0[i]);
    ys[i] = ys0[i] < -8000 ? -8000 : (ys0[i] > 12000 ? 12000 : ys0[i]);
    if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i];
  }
  int r0 = (int)((ymin + 7) >> 4), r1 = (int)((ymax - 8) >> 4);       // rows whose centre (y·16+8) is inside
  if (r0 < 0) r0 = 0; if (r1 > H - 1) r1 = H - 1;
  if (r0 > r1) return;
  for (int y = r0; y <= r1; y++) { p5_xl[y] = 32767; p5_xr[y] = -32768; }
  for (int i = 0; i < n; i++) {
    int j = i + 1 == n ? 0 : i + 1;
    int32_t xa = xs[i], ya = ys[i], xb = xs[j], yb = ys[j];
    if (ya == yb) continue;
    if (ya > yb) { int32_t t = xa; xa = xb; xb = t; t = ya; ya = yb; yb = t; }
    int e0 = (int)((ya + 7) >> 4), e1 = (int)((yb - 8) >> 4);          // rows this edge crosses (centre in [ya, yb))
    if (e0 < r0) e0 = r0; if (e1 > r1) e1 = r1;
    if (e0 > e1) continue;
    int32_t step = (xb - xa) * 4096 / (yb - ya);                     // x per row, in 1/256 of 1/16 px
    int32_t x8 = xa * 256 + (((int32_t)e0 * 16 + 8 - ya) * step >> 4);
    int32_t rowStep = step * 16 >> 4;                                 // per whole row, in 1/256 units
    for (int y = e0; y <= e1; y++, x8 += rowStep) {
      int32_t x16 = x8 >> 8;
      int xl = (int)((x16 + 8) >> 4), xr = (int)((x16 - 8) >> 4);
      if (xl < p5_xl[y]) p5_xl[y] = (int16_t)xl;
      if (xr > p5_xr[y]) p5_xr[y] = (int16_t)xr;
    }
  }
  for (int y = r0; y <= r1; y++) {
    int a = p5_xl[y], b = p5_xr[y];
    if (a < 0) a = 0; if (b > W - 1) b = W - 1;
    if (a <= b) memset(v3_buf + y * W + a, c, b - a + 1);
  }
}
static inline void p5_quad(const int32_t* xs, const int32_t* ys, uint8_t c) { p5_fill(xs, ys, 4, c); }
// a line clipped to the screen first, then drawn straight into the buffer (no per-pixel checks)
static void p5_seg(int x0, int y0, int x1, int y1, uint8_t c) {
  // Cohen-Sutherland clip to 0 … W-1, 0 … H-1
  for (int it = 0; it < 4; it++) {
    int o0 = (x0 < 0) | ((x0 > W - 1) << 1) | ((y0 < 0) << 2) | ((y0 > H - 1) << 3);
    int o1 = (x1 < 0) | ((x1 > W - 1) << 1) | ((y1 < 0) << 2) | ((y1 > H - 1) << 3);
    if (!(o0 | o1)) break;
    if (o0 & o1) return;
    int o = o0 ? o0 : o1, x, y;
    int dx = x1 - x0, dy = y1 - y0;
    if (o & 4)      { y = 0;     x = x0 + (int)((int32_t)dx * (0 - y0) / dy); }
    else if (o & 8) { y = H - 1; x = x0 + (int)((int32_t)dx * (H - 1 - y0) / dy); }
    else if (o & 1) { x = 0;     y = y0 + (int)((int32_t)dy * (0 - x0) / dx); }
    else            { x = W - 1; y = y0 + (int)((int32_t)dy * (W - 1 - x0) / dx); }
    if (o == o0) { x0 = x; y0 = y; } else { x1 = x; y1 = y; }
    if (it == 3 && ((unsigned)x0 >= W || (unsigned)y0 >= H || (unsigned)x1 >= W || (unsigned)y1 >= H)) return;
  }
  if ((unsigned)x0 >= W || (unsigned)y0 >= H || (unsigned)x1 >= W || (unsigned)y1 >= H) return;
  int dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -W : W;
  uint8_t* p = v3_buf + y0 * W + x0;
  if (adx >= ady) { int e = adx >> 1; for (int i = 0; i <= adx; i++) { *p = c; p += sx; e -= ady; if (e < 0) { e += adx; p += sy; } } }
  else            { int e = ady >> 1; for (int i = 0; i <= ady; i++) { *p = c; p += sy; e -= adx; if (e < 0) { e += ady; p += sx; } } }
}
// a quad covered in a mesh: nu lines one way, nv the other (corners in order 0-1-2-3, 1/16 px)
static void p5_mesh(const int32_t* xs, const int32_t* ys, int nu, int nv, uint8_t line) {
  if (nu > 60) nu = 60; if (nv > 60) nv = 60;
  if (nu > 1) {                                        // lines from edge 0-3 to edge 1-2, stepped (no divides per line)
    int32_t ax = xs[0] * 256, ay = ys[0] * 256, bx = xs[3] * 256, by = ys[3] * 256;
    int32_t dax = (xs[1] - xs[0]) * 256 / nu, day = (ys[1] - ys[0]) * 256 / nu, dbx = (xs[2] - xs[3]) * 256 / nu, dby = (ys[2] - ys[3]) * 256 / nu;
    for (int k = 1; k < nu; k++) { ax += dax; ay += day; bx += dbx; by += dby; p5_seg(ax >> 12, ay >> 12, bx >> 12, by >> 12, line); }
  }
  if (nv > 1) {                                        // lines from edge 0-1 to edge 3-2
    int32_t ax = xs[0] * 256, ay = ys[0] * 256, bx = xs[1] * 256, by = ys[1] * 256;
    int32_t dax = (xs[3] - xs[0]) * 256 / nv, day = (ys[3] - ys[0]) * 256 / nv, dbx = (xs[2] - xs[1]) * 256 / nv, dby = (ys[2] - ys[1]) * 256 / nv;
    for (int k = 1; k < nv; k++) { ax += dax; ay += day; bx += dbx; by += dby; p5_seg(ax >> 12, ay >> 12, bx >> 12, by >> 12, line); }
  }
}
// the outline of a quad
static void p5_outline(const int32_t* xs, const int32_t* ys, uint8_t c) {
  for (int i = 0; i < 4; i++) { int j = (i + 1) & 3; p5_seg(xs[i] >> 4, ys[i] >> 4, xs[j] >> 4, ys[j] >> 4, c); }
}
// darken a pixel by `amt` tone steps, staying in its ramp (an airbrush pass)
static inline void p5_darken(int x, int y, int amt) {
  if ((unsigned)x >= W || (unsigned)y >= H) return;
  uint8_t* p = v3_buf + y * W + x;
  int r = *p & 128, t = (*p & 127) - amt;
  *p = (uint8_t)(r | (t < 0 ? 0 : t));
}
// an airbrush spray over a whole row span: darker towards one end (soft, dithered)
static void p5_sprayRow(int y, int x0, int x1, int a0, int a1, uint32_t seed) {
  if ((unsigned)y >= H) return;
  if (x0 < 0) x0 = 0; if (x1 > W - 1) x1 = W - 1;
  int n = x1 - x0; if (n <= 0) return;
  uint8_t* p = v3_buf + y * W;
  uint32_t h = seed ^ (uint32_t)(y * 2654435761u);
  for (int x = x0; x <= x1; x++) {
    h ^= h << 13; h ^= h >> 17; h ^= h << 5;
    int a = a0 + (a1 - a0) * (x - x0) / n + (int)(h & 7) - 3;
    if (a <= 0) continue;
    int r = p[x] & 128, t = (p[x] & 127) - a;
    p[x] = (uint8_t)(r | (t < 0 ? 0 : t));
  }
}
#endif

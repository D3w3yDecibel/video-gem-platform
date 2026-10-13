// =====================================================================
// vg3.h — shared helpers for the Set 3 programs on slots 3-11 (by Dewey)
//
// Only one program runs at a time, so these programs share one block of
// working memory (v3_scratch) instead of each keeping its own — that is how
// nine programs fit in the ~20 KB of RAM Set 3 had left. A program must set
// up everything it keeps in the scratch block in its _init (or whenever it
// changes preset), and never assume what another program left there.
//
// Also here: knob smoothing, frame timing, random numbers, integer trig,
// palette builders and drawing on the 8-bit display buffer. Most programs
// draw pixels as (level << 4) | hue: 16 hues × 16 brightness levels.
// =====================================================================
#ifndef VG3_H
#define VG3_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "vg_sin.h"

// ─── Shared working memory ────────────────────────────────────────────
#define V3_SCRATCH 14336
static uint8_t v3_scratch[V3_SCRATCH] __attribute__((aligned(4)));

// ─── Small maths ──────────────────────────────────────────────────────
static inline int v3_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float v3_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int v3_abs(int v) { return v < 0 ? -v : v; }
static uint32_t v3_seed = 0x2545F491u;
static inline uint32_t v3_rand() { uint32_t x = v3_seed; x ^= x << 13; x ^= x >> 17; x ^= x << 5; v3_seed = x; return x; }
static inline int v3_rnd(int n) { return n > 0 ? (int)((v3_rand() >> 8) % (uint32_t)n) : 0; }
static inline int v3_rndr(int lo, int hi) { return lo + v3_rnd(hi - lo + 1); }
static inline float v3_rndf() { return (v3_rand() >> 8) * (1.0f / 16777216.0f); }
static inline uint32_t v3_hash(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x;
}
// sines: angle 1024 steps per turn, result Q14 (16384 = 1.0)
static inline int32_t v3_sin(int a) { return vg_sin1024[a & 1023]; }
static inline int32_t v3_cos(int a) { return vg_sin1024[(a + 256) & 1023]; }
static inline uint32_t v3_isqrt(uint32_t v) {
  uint32_t r = 0, b = 1u << 30;
  while (b > v) b >>= 2;
  while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
  return r;
}
// angle of (dx, dy): 0 … 1023 per turn, integers only (error < 0.3°)
static inline int v3_atan(int32_t dy, int32_t dx) {
  int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax == 0 && ay == 0) return 0;
  int32_t z = ax > ay ? (ay << 12) / ax : (ax << 12) / ay;
  int32_t t = 8192 + ((2848 * (4096 - z)) >> 12);
  int a = (int)((z * t) >> 18);
  if (ay > ax) a = 256 - a;
  if (dx < 0) a = 512 - a;
  if (dy < 0) a = 1024 - a;
  return a & 1023;
}
// smooth 1-D value noise: t in 1/256 steps, result −16384 … 16384
static inline int32_t v3_noise1(uint32_t t, uint32_t seed) {
  uint32_t i = t >> 8; int32_t f = t & 255;
  int32_t a = (int32_t)(v3_hash(i * 0x9E3779B1u + seed) & 32767) - 16384;
  int32_t b = (int32_t)(v3_hash((i + 1) * 0x9E3779B1u + seed) & 32767) - 16384;
  int32_t s = (f * f * (768 - 2 * f)) >> 16;                      // smoothstep, 0 … 256
  return a + (((b - a) * s) >> 8);
}

// ─── Frame timing + knobs ─────────────────────────────────────────────
// call v3_frame() once at the top of every draw: it measures dt and smooths the knobs
static float v3_sm[16];
static bool v3_smReady = false;
static float v3_dt = 0.016f;
static unsigned long v3_lastMs = 0;
static inline void v3_reset() { v3_smReady = false; }
static void v3_frame() {
  unsigned long now = millis();
  float dt = (now - v3_lastMs) / 1000.0f;
  v3_lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  v3_dt = dt;
  float a = dt / 0.12f; if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!v3_smReady) v3_sm[i] = (float)pots[i];
    else v3_sm[i] += (pots[i] - v3_sm[i]) * a;
  }
  v3_smReady = true;
}
static inline int v3_k(int i) { int v = (int)v3_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
static inline float v3_u(int i) { return v3_k(i) / 1023.0f; }                       // 0 … 1
static inline bool v3_centre(int i) { int k = v3_k(i); return k >= 440 && k < 600; } // the "preset's own" band
static inline float v3_bi(int i) { return (v3_k(i) - 512) / 512.0f; }               // −1 … 1
static inline float v3_rel(int i, float own, float range) { return own * powf(range, v3_bi(i)); }
// a choice knob: centre band = the preset's own choice, otherwise one of n spread across the dial
static inline int v3_pick(int i, int own, int n) {
  if (v3_centre(i)) return own;
  return v3_clamp(v3_k(i) * n / 1024, 0, n - 1);
}
// a speed knob: far left freezes, centre = 1, right = up to `top`
static inline float v3_speed(int i, float top) {
  float k = v3_u(i);
  if (k < 0.03f) return 0.0f;
  return powf(top, (k - 0.5f) * 2.0f);
}

// ─── Palettes ─────────────────────────────────────────────────────────
// level × hue: entry (l << 4) | h = colour h at brightness l (0 = black)
static inline uint8_t v3_mix8(int a, int b, int w) { return (uint8_t)(a + (((b - a) * w) >> 8)); }
static int16_t v3_levK[16];
static float v3_levG = -1; static int v3_levF = -1;
static void v3_setLH(int h, int r, int g, int b, float gamma, int floor256) {
  if (gamma != v3_levG || floor256 != v3_levF) {                  // brightness steps, worked out once per gamma
    v3_levG = gamma; v3_levF = floor256;
    for (int l = 1; l < 16; l++) v3_levK[l] = (int16_t)(floor256 + (int)((256 - floor256) * powf(l / 15.0f, gamma)));
  }
  display.setColor((uint8_t)h, 0, 0, 0);
  for (int l = 1; l < 16; l++) {
    int k = v3_levK[l];
    display.setColor((uint8_t)((l << 4) | h), v3_clamp(r * k >> 8, 0, 255), v3_clamp(g * k >> 8, 0, 255), v3_clamp(b * k >> 8, 0, 255));
  }
}
static void v3_palLH(const uint8_t (*cols)[3], int nh, float gamma, int floor256) {
  for (int h = 0; h < nh && h < 16; h++) v3_setLH(h, cols[h][0], cols[h][1], cols[h][2], gamma, floor256);
}
// a smooth gradient through `n` colour stops into entries first … first+count−1
static void v3_palGrad(const uint8_t (*stops)[3], int n, int first, int count, bool wrap) {
  for (int i = 0; i < count; i++) {
    int seg = wrap ? n : n - 1;
    int32_t p = (int32_t)i * seg * 256 / (wrap ? count : (count > 1 ? count - 1 : 1));
    int s0 = (p >> 8) % n, s1 = (s0 + 1) % n, f = p & 255;
    if (!wrap && s0 >= n - 1) { s0 = n - 1; s1 = n - 1; }
    display.setColor((uint8_t)(first + i), v3_mix8(stops[s0][0], stops[s1][0], f), v3_mix8(stops[s0][1], stops[s1][1], f), v3_mix8(stops[s0][2], stops[s1][2], f));
  }
}
// HSV → RGB, h 0 … 1535 (6 × 256), s and v 0 … 255
static void v3_hsv(int h, int s, int v, uint8_t* out) {
  h %= 1536; if (h < 0) h += 1536;
  int seg = h >> 8, f = h & 255;
  int p = v * (255 - s) / 255, q = v * (255 - s * f / 255) / 255, t = v * (255 - s * (255 - f) / 255) / 255;
  int r, g, b;
  switch (seg) { case 0: r = v; g = t; b = p; break; case 1: r = q; g = v; b = p; break; case 2: r = p; g = v; b = t; break;
                 case 3: r = p; g = q; b = v; break; case 4: r = t; g = p; b = v; break; default: r = v; g = p; b = q; break; }
  out[0] = r; out[1] = g; out[2] = b;
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* v3_buf;
static inline void v3_put(int x, int y, uint8_t c) { if ((unsigned)x < W && (unsigned)y < H) v3_buf[y * W + x] = c; }
static inline void v3_putMax(int x, int y, int lev, int hue) {     // brightest wins
  if ((unsigned)x >= W || (unsigned)y >= H || lev <= 0) return;
  if (lev > 15) lev = 15;
  uint8_t* p = v3_buf + y * W + x;
  if ((*p >> 4) <= lev) *p = (uint8_t)((lev << 4) | hue);
}
static void v3_hspan(int x0, int x1, int y, uint8_t c) {
  if ((unsigned)y >= H) return;
  if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
  if (x0 < 0) x0 = 0; if (x1 > W - 1) x1 = W - 1;
  if (x0 <= x1) memset(v3_buf + y * W + x0, c, x1 - x0 + 1);
}
static void v3_rect(int x0, int y0, int x1, int y1, uint8_t c) { for (int y = y0; y <= y1; y++) v3_hspan(x0, x1, y, c); }
static void v3_line(int x0, int y0, int x1, int y1, uint8_t c) {
  int dx = v3_abs(x1 - x0), dy = -v3_abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
  if (dx > 2000 || dy < -2000) return;
  for (;;) {
    v3_put(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * e;
    if (e2 >= dy) { e += dy; x0 += sx; }
    if (e2 <= dx) { e += dx; y0 += sy; }
  }
}
// anti-aliased line, ends in 1/8 px, brightest wins
static void v3_lineAA(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (lev <= 0) return;
  int dx = x1 - x0, dy = y1 - y0;
  int adx = v3_abs(dx), ady = v3_abs(dy);
  if (adx > 8 * 1000 || ady > 8 * 1000) return;
  if (adx < 8 && ady < 8) { v3_putMax((x0 + 4) >> 3, (y0 + 4) >> 3, lev, hue); return; }
  if (adx >= ady) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dy = -dy; }
    int32_t grad = (int32_t)dy * 65536 / (x1 - x0);
    int xs = (x0 + 7) >> 3, xe = x1 >> 3;
    if (xs < 0) xs = 0; if (xe > W - 1) xe = W - 1;
    int32_t y = y0 * 65536 + (int32_t)(xs * 8 - x0) * grad, step = grad * 8;
    for (int x = xs; x <= xe; x++, y += step) {
      int yy = (int)(y >> 16), yi = yy >> 3, f = yy & 7;
      v3_putMax(x, yi, (lev * (8 - f) + 4) >> 3, hue);
      if (f) v3_putMax(x, yi + 1, (lev * f + 4) >> 3, hue);
    }
  } else {
    if (y1 < y0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dx = -dx; }
    int32_t grad = (int32_t)dx * 65536 / (y1 - y0);
    int ys = (y0 + 7) >> 3, ye = y1 >> 3;
    if (ys < 0) ys = 0; if (ye > H - 1) ye = H - 1;
    int32_t x = x0 * 65536 + (int32_t)(ys * 8 - y0) * grad, step = grad * 8;
    for (int y = ys; y <= ye; y++, x += step) {
      int xx = (int)(x >> 16), xi = xx >> 3, f = xx & 7;
      v3_putMax(xi, y, (lev * (8 - f) + 4) >> 3, hue);
      if (f) v3_putMax(xi + 1, y, (lev * f + 4) >> 3, hue);
    }
  }
}
// a filled disc (whole pixels)
static void v3_disc(int cx, int cy, int r, uint8_t c) {
  if (r <= 0) { v3_put(cx, cy, c); return; }
  int r2 = r * r + r;
  for (int dy = -r; dy <= r; dy++) {
    int w = (int)v3_isqrt((uint32_t)(r2 - dy * dy));
    v3_hspan(cx - w, cx + w, cy + dy, c);
  }
}
// a filled polygon, corners in 1/16 px (n ≤ 16); even-odd rule, pixel centres
static void v3_poly(const int32_t* xs, const int32_t* ys, int n, uint8_t c) {
  if (n < 3 || n > 16) return;
  int32_t ymin = ys[0], ymax = ys[0];
  for (int i = 1; i < n; i++) { if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i]; }
  int y0 = (int)((ymin + 8) >> 4), y1 = (int)((ymax - 8) >> 4);
  if (y0 < 0) y0 = 0; if (y1 > H - 1) y1 = H - 1;
  for (int y = y0; y <= y1; y++) {
    int32_t sy = y * 16 + 8, xsx[16]; int m = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
      int32_t ya = ys[j], yb = ys[i];
      if ((ya <= sy && yb > sy) || (yb <= sy && ya > sy))
        xsx[m++] = xs[j] + (int32_t)((int64_t)(sy - ya) * (xs[i] - xs[j]) / (yb - ya));
    }
    for (int a = 1; a < m; a++) { int32_t v = xsx[a]; int b = a - 1; while (b >= 0 && xsx[b] > v) { xsx[b + 1] = xsx[b]; b--; } xsx[b + 1] = v; }
    for (int a = 0; a + 1 < m; a += 2) {
      int xa = (int)((xsx[a] + 8) >> 4), xb = (int)((xsx[a + 1] - 8) >> 4);
      if (xb >= xa) v3_hspan(xa, xb, y, c);
    }
  }
}
// ─── A small font (5×7, rows) ─────────────────────────────────────────
static const char v3_fontChars[] = "ABCDEGHILMNOPRSTUVY0123456789:-> ";
static const uint8_t v3_font[][7] = {
  {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
  {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},
  {17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},
  {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,10,4,4,4,4},
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{31,2,4,2,1,17,14},{2,6,10,18,31,2,2},
  {31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
  {0,12,12,0,12,12,0},{0,0,0,31,0,0,0},{8,4,2,1,2,4,8},{0,0,0,0,0,0,0},
};
static int v3_glyph(char c) { for (int i = 0; v3_fontChars[i]; i++) if (v3_fontChars[i] == c) return i; return 32; }
// draw text straight into the picture: top-left (x, y), each font pixel sc × sc
static void v3_text(int x, int y, const char* s, int sc, uint8_t c) {
  for (int i = 0; s[i]; i++) {
    const uint8_t* g = v3_font[v3_glyph(s[i])];
    for (int r = 0; r < 7; r++) for (int b = 0; b < 5; b++) if (g[r] & (16 >> b)) v3_rect(x + (i * 6 + b) * sc, y + r * sc, x + (i * 6 + b) * sc + sc - 1, y + r * sc + sc - 1, c);
  }
}
// fade every pixel's level by `amt` (hue kept; level 0 = black). Buffer is word-aligned.
static void v3_fadeLev(int amt) {
  if (amt <= 0) return;
  if (amt > 15) amt = 15;
  uint32_t* p = (uint32_t*)v3_buf;
  uint32_t sub = (uint32_t)amt * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) {
    uint32_t v = p[i];
    uint32_t lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
    uint32_t d = (lev | 0x10101010u) - sub;                         // bit 4 of each byte set ⇔ no borrow
    uint32_t keep = (d >> 4) & 0x01010101u; keep *= 0x0Fu;
    p[i] = ((d & keep) << 4) | hue;
  }
}
#endif

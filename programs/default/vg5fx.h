// =====================================================================
// vg5fx.h — Set 5's live performance knobs (by Dewey)
//
// The same four knobs on every Set 5 program, applied to the finished
// picture. Each is OFF in its centre band and does something different
// each way, growing stronger the further you turn — and each one moves on
// its own, so turning a knob always adds motion:
//
//   p12 WARP    ◄ RIPPLE: rows wobble like liquid      ZOOM ►: the picture breathes in and out
//   p13 MIRROR  ◄ MIRROR: 2-way, then 4-way             TILES ►: 2×2, then 4×4 mirrored tiles, sliding
//   p14 COLOUR  ◄ CYCLE: hues rotate (faster further)   FLASH ►: the palette flashes and jumps on a beat
//   p15 GLITCH  ◄ TEAR: bands of the picture rip sideways, stuttering    PIXELS ►: pixelate, pulsing
//
// How a program uses it: call fx5_take() right after v3_frame() — it reads
// p12-p15 and hands the program's own code the centre value for them (so
// those knobs keep the preset's look) — and fx5_apply() when the picture is
// finished. Colour works on the palette, so programs rebuild their palettes
// every frame while it is on (vg5p.h's cache checks fx5_palReset).
// =====================================================================
#ifndef VG5FX_H
#define VG5FX_H
#include "vg3.h"

static float fx5_amt[4];                             // −1 … 1 for p12 … p15 (0 = off)
static float fx5_saved[4];
static bool fx5_took = false, fx5_palReset = false, fx5_colWas = false;
static float fx5_t = 0;
static uint8_t fx5_row[W];

// read a knob as off in the centre band, −1 … 1 outside it
static inline float fx5_knob(int i) {
  float k = v3_sm[i];
  if (k >= 440 && k < 600) return 0.0f;
  if (k < 440) return -(440.0f - k) / 440.0f;
  return (k - 600.0f) / 423.0f;
}
static void fx5_take() {
  for (int i = 0; i < 4; i++) { fx5_amt[i] = v3_clampf(fx5_knob(12 + i), -1.0f, 1.0f); fx5_saved[i] = v3_sm[12 + i]; v3_sm[12 + i] = 512.0f; }
  bool col = fx5_amt[2] != 0.0f;
  fx5_palReset = col || fx5_colWas;                  // rebuild once more after it's turned off, to undo it
  fx5_colWas = col;
  fx5_t += v3_dt;
  if (fx5_t > 3600.0f) fx5_t -= 3600.0f;
  fx5_took = true;
}

// a row turned round by dx pixels (wrapping, so no black edges)
static void fx5_rotRow(uint8_t* r, int dx) {
  dx %= W; if (dx < 0) dx += W;
  if (!dx) return;
  memcpy(fx5_row, r + W - dx, dx);
  memmove(r + dx, r, W - dx);
  memcpy(r, fx5_row, dx);
}
// ── WARP ──
static void fx5_ripple(float a) {
  int amp = (int)(a * 46), t1 = (int)(fx5_t * 260), t2 = (int)(fx5_t * 170);
  int f1 = 9 + (int)(a * 10), f2 = 23;
  for (int y = 0; y < H; y++) {
    int dx = (v3_sin(y * f1 + t1) * amp + v3_sin(y * f2 - t2) * (amp / 2)) >> 14;
    fx5_rotRow(v3_buf + y * W, dx);
  }
}
static void fx5_zoom(float a) {
  float s = 1.0f + a * 1.6f * (0.5f + 0.5f * sinf(fx5_t * 2.1f)) + a * 0.15f;   // breathing, never below 1
  int32_t inv = (int32_t)(65536.0f / s);
  const int cx = W / 2, cy = H / 2;
  static int16_t xs[W];                               // source column for each screen column
  for (int x = 0; x < W; x++) xs[x] = (int16_t)(cx + (((x - cx) * inv) >> 16));
  // rows from the edges inwards: each reads a row nearer the middle, not yet overwritten
  for (int k = 0; k < cy; k++) {
    for (int side = 0; side < 2; side++) {
      int y = side ? H - 1 - k : k;
      int sy = cy + (((y - cy) * inv) >> 16);
      memcpy(fx5_row, v3_buf + sy * W, W);
      uint8_t* d = v3_buf + y * W;
      for (int x = 0; x < W; x++) d[x] = fx5_row[xs[x]];
    }
  }
}
// ── MIRROR ──
static void fx5_mirror(float a) {
  for (int y = 0; y < H; y++) { uint8_t* r = v3_buf + y * W; for (int x = 0; x < W / 2; x++) r[W - 1 - x] = r[x]; }
  if (a > 0.5f) for (int y = 0; y < H / 2; y++) memcpy(v3_buf + (H - 1 - y) * W, v3_buf + y * W, W);
}
static void fx5_shrinkTile(int w, int h) {         // the w × h top-left block → half size, mirrored into its 4 quarters
  int hw = w / 2, hh = h / 2;
  for (int y = 0; y < hh; y++) { uint8_t* d = v3_buf + y * W; const uint8_t* s = v3_buf + (2 * y) * W; for (int x = 0; x < hw; x++) d[x] = s[2 * x]; }
  for (int y = 0; y < hh; y++) { uint8_t* r = v3_buf + y * W; for (int x = 0; x < hw; x++) r[w - 1 - x] = r[x]; }
  for (int y = 0; y < hh; y++) memcpy(v3_buf + (h - 1 - y) * W, v3_buf + y * W, w);
}
static void fx5_tiles(float a) {
  fx5_shrinkTile(W, H);
  if (a > 0.5f) {                                    // 4 × 4: shrink the top-left tile again, copy it across
    fx5_shrinkTile(W / 2, H / 2);
    for (int y = 0; y < H / 2; y++) { uint8_t* r = v3_buf + y * W; memcpy(r + W / 2, r, W / 2); }
    for (int y = 0; y < H / 2; y++) memcpy(v3_buf + (y + H / 2) * W, v3_buf + y * W, W);
  }
  int dx = (int)(fx5_t * 18 * (0.5f + a));          // the tiles slide sideways …
  int wob = (int)(a * 10);
  for (int y = 0; y < H; y++) fx5_rotRow(v3_buf + y * W, dx + ((v3_sin(y * 4 + (int)(fx5_t * 200)) * wob) >> 14));   // … with a slow wave
}
// ── COLOUR (on the palette) ──
static void fx5_colour(float a) {
  float ang, inv = 0.0f;
  if (a < 0) { ang = fx5_t * (0.4f + 5.0f * (-a) * (-a)); }                      // cycle: faster the further left
  else {                                                                        // flash: a beat, 70 … 170 bpm
    float bps = (70.0f + 100.0f * a) / 60.0f, beat = fx5_t * bps;
    float ph = beat - floorf(beat);
    inv = (1.0f - ph) * (1.0f - ph) * (1.0f - ph) * (0.35f + 0.65f * a);
    ang = floorf(beat) * 2.1f * a + fx5_t * 0.3f;
  }
  float c = cosf(ang), s = sinf(ang);
  int32_t m[9] = {                                    // hue rotation round the grey axis, Q12
    (int32_t)((0.299f + 0.701f * c + 0.168f * s) * 4096), (int32_t)((0.587f - 0.587f * c + 0.330f * s) * 4096), (int32_t)((0.114f - 0.114f * c - 0.497f * s) * 4096),
    (int32_t)((0.299f - 0.299f * c - 0.328f * s) * 4096), (int32_t)((0.587f + 0.413f * c + 0.035f * s) * 4096), (int32_t)((0.114f - 0.114f * c + 0.292f * s) * 4096),
    (int32_t)((0.299f - 0.300f * c + 1.250f * s) * 4096), (int32_t)((0.587f - 0.588f * c - 1.050f * s) * 4096), (int32_t)((0.114f + 0.886f * c - 0.203f * s) * 4096) };
  int iv = (int)(inv * 256);
  // a tint that travels round the colour wheel too, so grey pictures (SHAFT, PLOTTER …) get colour as well
  int ta = (int)(fabsf(a) * 150);
  int tr = 128 + (int)(127 * cosf(ang * 1.3f)), tg = 128 + (int)(127 * cosf(ang * 1.3f - 2.09f)), tb = 128 + (int)(127 * cosf(ang * 1.3f + 2.09f));
  for (int i = 0; i < 256; i++) {
    uint16_t v = display.getColor((uint8_t)i);
    int r = ((v >> 11) & 31) * 255 / 31, g = ((v >> 5) & 63) * 255 / 63, b = (v & 31) * 255 / 31;
    int r2 = (m[0] * r + m[1] * g + m[2] * b) >> 12, g2 = (m[3] * r + m[4] * g + m[5] * b) >> 12, b2 = (m[6] * r + m[7] * g + m[8] * b) >> 12;
    r2 = v3_clamp(r2, 0, 255); g2 = v3_clamp(g2, 0, 255); b2 = v3_clamp(b2, 0, 255);
    int lum = (r2 * 77 + g2 * 150 + b2 * 29) >> 8;
    r2 += ((lum * tr / 128 - r2) * ta) >> 8; g2 += ((lum * tg / 128 - g2) * ta) >> 8; b2 += ((lum * tb / 128 - b2) * ta) >> 8;
    r2 = v3_clamp(r2, 0, 255); g2 = v3_clamp(g2, 0, 255); b2 = v3_clamp(b2, 0, 255);
    if (iv) { r2 += ((255 - 2 * r2) * iv) >> 8; g2 += ((255 - 2 * g2) * iv) >> 8; b2 += ((255 - 2 * b2) * iv) >> 8; }
    display.setColor((uint8_t)i, (uint8_t)r2, (uint8_t)g2, (uint8_t)b2);
  }
}
// ── GLITCH ──
#define FX5_BANDS 14
static int16_t fx5_by[FX5_BANDS], fx5_bh[FX5_BANDS], fx5_bdx[FX5_BANDS];
static uint8_t fx5_bsmear[FX5_BANDS];
static float fx5_gNext = 0;
static void fx5_tear(float a) {
  int n = 1 + (int)(a * (FX5_BANDS - 1));
  if (fx5_t >= fx5_gNext || fx5_t + 1.0f < fx5_gNext) {           // new tears, a few times a second (stuttering)
    fx5_gNext = fx5_t + 0.06f + v3_rndf() * (0.25f - 0.15f * a);
    for (int i = 0; i < FX5_BANDS; i++) {
      fx5_by[i] = (int16_t)v3_rnd(H); fx5_bh[i] = (int16_t)(2 + v3_rnd(6 + (int)(a * 40)));
      fx5_bdx[i] = (int16_t)(v3_rndr(-1, 1) * (8 + v3_rnd(20 + (int)(a * 120))));
      fx5_bsmear[i] = (uint8_t)(v3_rnd(100) < (int)(a * 40));
    }
  }
  for (int i = 0; i < n; i++) {
    int y0 = fx5_by[i], y1 = y0 + fx5_bh[i]; if (y1 > H) y1 = H;
    for (int y = y0; y < y1; y++) {
      if (fx5_bsmear[i] && y > y0) memcpy(v3_buf + y * W, v3_buf + y0 * W, W);   // smeared: one row stretched down
      else fx5_rotRow(v3_buf + y * W, fx5_bdx[i]);
    }
  }
}
static void fx5_pixels(float a) {
  int b = 2 + (int)(a * 14 * (0.6f + 0.4f * sinf(fx5_t * 3.0f)));   // block size, pulsing
  if (b < 2) return;
  for (int by = 0; by < H; by += b) {
    uint8_t* src = v3_buf + by * W;
    for (int bx = 0; bx < W; bx += b) { uint8_t c = src[bx]; int w = bx + b > W ? W - bx : b; memset(src + bx, c, w); }
    for (int y = by + 1; y < by + b && y < H; y++) memcpy(v3_buf + y * W, src, W);
  }
}
// everything, in order: warp, glitch, mirror, colour — then the program's own knob values are put back
static void fx5_apply() {
  if (!fx5_took) return;
  fx5_took = false;
  for (int i = 0; i < 4; i++) v3_sm[12 + i] = fx5_saved[i];
  if (!v3_buf) return;
  float w = fx5_amt[0], m = fx5_amt[1], c = fx5_amt[2], g = fx5_amt[3];
  if (w < 0) fx5_ripple(-w); else if (w > 0) fx5_zoom(w);
  if (g < 0) fx5_tear(-g); else if (g > 0) fx5_pixels(g);
  if (m < 0) fx5_mirror(-m); else if (m > 0) fx5_tiles(m);
  if (c != 0) fx5_colour(c);
}
static const char* const fx5_labels[4] = { "Warp: Ripple|Zoom", "Mirror|Tiles", "Colour: Cycle|Flash", "Glitch: Tear|Pixels" };
#endif

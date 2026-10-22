// =====================================================================
// PROGRAM: TRIAD — endless triangle fractals (by Dewey) — Set 6, slot 7
//
// The Sierpinski triangle and its cousins, falling forever inward. On a
// triangular grid, a spot belongs to Sierpinski's triangle exactly when its
// two grid numbers share no binary 1s — so one AND per pixel draws a
// fractal, and every halving of scale looks the same, so the zoom never
// ends. Six copies round the centre make it a hexagonal snowflake of
// triangles; other bit tricks weave lace, rings of gaskets, wheels and stars. Each depth of the fractal gets its own colour, flowing.
//
// Presets: k0 Sierpinski Star · k1 Pascal · k2 XOR Weave · k3 Hex Carpet ·
//          k4 Lace · k5 Ember Gasket · k6 Crystal · k7 Woven Mandala ·
//          k8 Deep Dive · k9 Opal Fractal · k10 Spectrum · k11 Tour
//   k12 — LEAP: dive two levels deeper at once
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Rule (gasket · lace · rings · wheel · star; centre = preset) · p6 Dive (◄ out | in ►)
//   p7 Spin · p8 Depth (how many levels light up) · p9 Sectors (1 · 3 · 6)
//   p10 Glow · p11 Shimmer · p12-p15 live FX
// =====================================================================
#include "vg6.h"

enum { TR_AND, TR_LACE, TR_RINGS, TR_MANDALA, TR_STAR, TR_NRULE };
struct TrLook { uint8_t rule, sect, pal, dive, spin, depth, glow, shim, flow, spread; };
static const TrLook tr_looks[11] = {
  // rule    sect pal dive spin depth glow shim flow spread
  { TR_AND,     6,  0, 40, 10, 60, 50, 30, 5, 70 },   // Sierpinski Star
  { TR_AND,     1,  9, 35,  0, 70, 40, 20, 3, 50 },   // Pascal
  { TR_MANDALA, 6,  4, 30, 12, 60, 50, 50, 6, 80 },   // Woven Wheel
  { TR_RINGS,   6,  2, 35,  8, 70, 50, 30, 5, 70 },   // Hex Rings
  { TR_LACE,    6,  7, 30, 10, 80, 30, 20, 4, 60 },   // Lace
  { TR_AND,     6,  5, 50, 15, 50, 70, 40, 7, 60 },   // Ember Gasket
  { TR_STAR,    6,  6, 30, 10, 95, 95, 30, 5, 70 },   // Crystal Star
  { TR_MANDALA, 3,  8, 35, 20, 60, 60, 60, 6, 90 },   // Triple Mandala
  { TR_AND,     6, 10, 90, 25, 70, 50, 40, 8, 80 },   // Deep Dive
  { TR_LACE,    3,  3, 30, 6,  60, 40, 30, 4, 60 },   // Opal Lace
  { TR_RINGS,   6, 11, 40, 15, 70, 60, 50, 9,100 },   // Spectrum
};
static const char* const tr_names[12] = { "Sierpinski Star", "Pascal", "Woven Wheel", "Hex Rings", "Lace", "Ember Gasket", "Crystal Star", "Triple Mandala", "Deep Dive", "Opal Lace", "Spectrum", "Tour" };
static const char* const tr_lbl[7] = { "Rule", "Dive", "Spin", "Depth", "Sectors", "Glow", "Shimmer" };
const char* prog_triad_name() { return "TRIAD"; }
const char* prog_triad_character() { return "Endless triangle fractals: Sierpinski's gasket and bitwise lace, diving inward forever"; }
const char* prog_triad_presetName(int p) { return (p >= 0 && p < 12) ? tr_names[p] : NULL; }
const char* prog_triad_potLabel(int p, int pot) { (void)p; return v6_label(pot, tr_lbl); }
uint8_t prog_triad_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// colour by level: [rule result's top 8 bits] → level and hue (rebuilt each frame)
static uint8_t* tr_lut;
static void tr_mem() { tr_lut = v6_mem; }

// ─── The hot loop ─────────────────────────────────────────────────────
// A, B: the point's two triangular-grid numbers (fixed point, `sh` fraction bits).
// sect: 1 = one 60° wedge (the rest mirrored), 3, or 6 copies round the centre.
// Into the wedge a ≥ 0, b ≥ 0: six copies are turns of 60°, and which turn is
// needed follows from the signs of a, b and a + b.
#define TR_SIX(a, b) do { int32_t s_ = a + b, t_; \
    if (a >= 0) { if (b < 0) { if (s_ >= 0) { a = -b; b = s_; } else { t_ = a; a = -s_; b = t_; } } } \
    else if (b >= 0) { if (s_ >= 0) { t_ = a; a = s_; b = -t_; } else { t_ = a; a = b; b = -s_; } } \
    else { a = -a; b = -b; } } while (0)
#define TR_RULE(R, ia, ib) ((R) == TR_AND ? ((ia) & (ib)) : (R) == TR_LACE ? (((ia) ^ (ib)) & ((ia) + (ib))) : \
    (R) == TR_RINGS ? (((ia) | (ib)) & ((ia) + (ib))) : (R) == TR_MANDALA ? (((ia) + 2 * (ib)) & ((ib) + 2 * (ia))) : \
    (((ia) & (ib)) | ((ia) & ((ia) + (ib))) | ((ib) & ((ia) + (ib)))))
// one loop per rule, so the rule isn't decided again for every pixel
#define TR_LOOP(R) __attribute__((optimize("O2"))) static void tr_row##R(uint8_t* row, int32_t A, int32_t B, int32_t dA, int32_t dB, int sect, int sh) { \
  const uint8_t* lut = tr_lut; \
  if (sect == 6) { \
    for (int x = 0; x < W; x++, A += dA, B += dB) { int32_t a = A, b = B; TR_SIX(a, b); \
      uint32_t ia = (uint32_t)a >> sh, ib = (uint32_t)b >> sh, m = TR_RULE(R, ia, ib); row[x] = lut[(m & 0xFF) ? (m & 0xFF) : ((m >> 8) ? 255 : 0)]; } \
  } else { \
    for (int x = 0; x < W; x++, A += dA, B += dB) { int32_t a = A, b = B; \
      if (sect == 3) { for (int k = 0; k < 2 && (a < 0 || b < 0); k++) { int32_t t = a; a = -b - a; b = t; } } \
      if (a < 0) a = -a; if (b < 0) b = -b; \
      uint32_t ia = (uint32_t)a >> sh, ib = (uint32_t)b >> sh, m = TR_RULE(R, ia, ib); row[x] = lut[(m & 0xFF) ? (m & 0xFF) : ((m >> 8) ? 255 : 0)]; } \
  } }
TR_LOOP(0) TR_LOOP(1) TR_LOOP(2) TR_LOOP(3) TR_LOOP(4)
static void tr_row(uint8_t* row, int32_t A, int32_t B, int32_t dA, int32_t dB, int rule, int sect, int sh) {
  switch (rule) {
    case 0: tr_row0(row, A, B, dA, dB, sect, sh); break;
    case 1: tr_row1(row, A, B, dA, dB, sect, sh); break;
    case 2: tr_row2(row, A, B, dA, dB, sect, sh); break;
    case 3: tr_row3(row, A, B, dA, dB, sect, sh); break;
    default: tr_row4(row, A, B, dA, dB, sect, sh); break;
  }
}

static float tr_dive, tr_spin, tr_depth, tr_glow, tr_shim;
static float tr_z = 0, tr_ang = 0, tr_leap = 0, tr_hueT = 0;
static int tr_ruleNow = 0, tr_sectNow = 6;

void prog_triad_init() { tr_mem(); v6_init(); tr_z = 0; tr_ang = 0; }

void prog_triad_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  tr_mem();
  const TrLook& L = tr_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  int rule = v3_centre(5) ? L.rule : v3_clamp(v3_k(5) * TR_NRULE / 1024, 0, TR_NRULE - 1);
  float tDive = v3_centre(6) ? L.dive / 100.0f : v3_bi(6) * 1.5f;
  float tSpin = v3_centre(7) ? L.spin / 100.0f : v3_bi(7);
  float tDepth = v3_centre(8) ? L.depth / 100.0f : v3_u(8);
  int sect = v3_centre(9) ? L.sect : (v3_k(9) < 341 ? 1 : (v3_k(9) < 682 ? 3 : 6));
  float tGlow = v3_centre(10) ? L.glow / 100.0f : v3_u(10);
  float tShim = v3_centre(11) ? L.shim / 100.0f : v3_u(11);
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&tr_dive, tDive, rate); v6_ease(&tr_spin, tSpin, rate); v6_ease(&tr_depth, tDepth, rate);
  v6_ease(&tr_glow, tGlow, rate); v6_ease(&tr_shim, tShim, rate);
  if (v6_first || v6_fade > 0.5f || !v3_centre(5)) tr_ruleNow = rule;
  if (v6_first || v6_fade > 0.5f || !v3_centre(9)) tr_sectNow = sect;
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 130);

  // ── the dive: scale doubles every so often; the colours shift one level each time so nothing jumps ──
  if (v6_k12hit) tr_leap = 2.0f;
  float leapV = tr_leap > 0 ? tr_leap * 2.5f : 0.0f;
  tr_leap -= dt * 2.5f; if (tr_leap < 0) tr_leap = 0;
  tr_z += sdt * tr_dive * 0.22f + dt * leapV * 0.4f;
  tr_ang += sdt * tr_spin * 0.15f;
  tr_hueT += sdt * tr_shim * 1.5f;
  int lvlShift = (int)floorf(tr_z);                                 // whole levels dived so far
  float fz = tr_z - lvlShift;                                       // 0 … 1 within this level
  float scale = powf(2.0f, -fz) / v6_zoom(1.0f);                    // grid units per pixel shrink as we dive

  // ── colours: each level of the fractal its own hue, deeper = dimmer unless Glow ──
  int depthN = 2 + (int)(tr_depth * 6);                             // levels that light up
  for (int m = 0; m < 256; m++) {
    int lev, hue;
    if (m == 0) { lev = 15; hue = (int)(tr_hueT) & 15; }            // in the set: the brightest
    else {
      int top = 0; while ((m >> (top + 1)) != 0) top++;             // highest bit: the level the spot was cut out at
      int lv = 7 - top;                                             // 0 = cut out first (biggest holes)
      if (lv >= depthN + 1) { lev = 1; hue = 0; }
      else {
        float lvc = lv - fz;                                        // (continuous across each doubling, so nothing jumps)
        lev = 3 + (int)((12 - lvc * (12.0f / depthN)) * (0.5f + tr_glow * 0.5f));
        hue = (lv * 3 + lvlShift * 3 + (int)tr_hueT) & 15;
      }
    }
    tr_lut[m] = (uint8_t)((v3_clamp(lev, 0, 15) << 4) | (hue & 15));
  }

  // ── pixels: screen → triangular grid (a along 0°, b along 60°) ──
  const int sh = 16;
  float k = scale * 65536.0f / 4.0f;                                // grid spacing 4 px at scale 1
  float ca = cosf(tr_ang), sa = sinf(tr_ang);
  // a = x' − y'/√3, b = 2y'/√3 for the turned point (x', y')
  float ax = ca + sa * 0.5773503f, ay = sa - ca * 0.5773503f;        // ∂a/∂x, ∂a/∂y
  float bx = -sa * 1.1547005f, by = ca * 1.1547005f;                 // ∂b/∂x, ∂b/∂y
  int32_t dAx = (int32_t)(ax * k), dAy = (int32_t)(ay * k), dBx = (int32_t)(bx * k), dBy = (int32_t)(by * k);
  // the gasket's corner sits a hair off the centre, so the middle pixel isn't on a seam
  int32_t A0 = (int32_t)(0.37f * k), B0 = (int32_t)(0.29f * k);
  for (int y = 0; y < H; y++) {
    int32_t A = A0 - 160 * dAx + (y - 120) * dAy, B = B0 - 160 * dBx + (y - 120) * dBy;
    tr_row(v3_buf + y * W, A, B, dAx, dBx, tr_ruleNow, tr_sectNow, sh);
  }
  v6_end();
}

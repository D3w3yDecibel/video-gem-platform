// =====================================================================
// vg7op.h — the Set 7 "Op-Art" engine (by Dewey)
//
// Set 7 is twelve programs of optical art: Bridget Riley's waves, Victor
// Vasarely's bulging grids, Soto's vibrating lines, Agam's changing
// pictures, Albers' squares and the classic optical illusions. Built on the
// Set 6 engine (vg6.h): same tour, same knob behaviour — plus what op art
// needs:
//
//   • INK AND PAPER — every pixel is (shade << 4) | hue. Shade 0 is "ink",
//     shade 15 is "paper", and the shades between are a blend of the two —
//     so edges can be anti-aliased with in-between shades, which keeps fine
//     stripes crisp instead of jagged. Hues 0-14 are a slice of one of
//     twelve palettes (each has its own paper colours and ink colours);
//     hue 15 is always neutral: black → grey → white.
//   • COMMON KNOBS on all twelve programs:
//       p0 Palette (centre = the preset's own)   p1 Speed (far left = freeze)
//       p2 Zoom (centre = preset)                p3 Colour Flow (◄ backwards | forwards ►)
//       p4 Hue Spread                            p11 COLOUR: far left = pure black & white,
//                                                    centre = the preset's own, right = full colour
//       p12 Warp and p14 Colour: the live FX from vg5fx.h
//       p13 RANDOMNESS: how much the patterns wander on their own (centre = normal,
//           far left = steady and regular, right = restless)
//       p15 LIQUID FLOW: the whole picture flows slowly like liquid (centre = a gentle
//           flow, far left = still, right = a strong flow)
//     (Set 7 has no mirror or glitch FX.)
//     p1 and p12-p15 do nothing until you actually turn them.
//   • CALM AND RANDOM — Set 7 runs at a slower pace than earlier sets, and its
//     movements wander: most motions follow slow random curves rather than
//     repeating sine waves, and each preset's own settings drift a little
//     (about ±20 %) over half a minute or so, so the picture never quite repeats.
//     Every power-up starts from a different random point.
//   • VIBRANT — the palettes are made richer than they are written (the colours are pushed
//     away from grey, most of all in the in-between shades), and the colours always drift.
//   • LIQUID — after each picture is drawn, its rows are slid sideways and up and down by
//     slow, smooth random amounts, so straight lines bend and flow (p15).
//   • TOUR — preset k11 on every program moves through presets k0-k10 by
//     itself, about every 20 seconds.
//   • STRIPES — o7_band makes a stripe profile with soft, anti-aliased edges
//     of a chosen width; o7_comb adds a hue for each stripe, so a whole
//     stripe field is one table lookup per pixel.
//
// Speed: every program draws with table lookups and integer steps only —
// no floating point and no division per pixel.
// =====================================================================
#ifndef VG7OP_H
#define VG7OP_H
#include "vg6.h"

// ─── Palettes: six paper colours (they wrap round) and how the ink is made ─
// ink colour = the palette's colour a little further round (inkOff, 1536 = once round) × inkBr / 255
struct O7Pal { uint8_t c[6][3]; uint16_t inkOff; uint8_t inkBr; };
#define O7_NPAL 12
static const O7Pal o7_pals[O7_NPAL] = {
  { {{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255}},   0,   0 },   //  0 Black & White
  { {{ 40,190,180},{235,110,140},{215,160, 50},{120, 80,180},{ 70,170, 90},{ 50, 90,200}}, 768, 255 },   //  1 Ra (Riley's coloured stripes)
  { {{ 40, 60,220},{140, 40,200},{230, 40, 60},{240,120, 40},{250,220, 60},{ 40,180,220}}, 512, 110 },   //  2 Vega (Vasarely)
  { {{250,220, 80},{245,170, 40},{220,110, 30},{180, 60, 30},{240,200,120},{200,140, 60}}, 256, 200 },   //  3 Albers
  { {{230, 30, 40},{ 20,180, 80},{ 40, 60,230},{240,120, 20},{140, 30,200},{ 30,200,200}}, 768, 230 },   //  4 Anuszkiewicz: vibrating complements
  { {{255, 40, 40},{255,200,  0},{ 40,220, 60},{  0,180,255},{ 60, 60,255},{220, 40,220}}, 768, 255 },   //  5 Agam: the spectrum
  { {{240,240,235},{ 30, 80,200},{240,210, 30},{240,240,235},{200, 40, 40},{ 30, 80,200}},   0,   0 },   //  6 Soto: blue, yellow, red on black
  { {{ 30, 60,220},{250,220, 40},{ 30, 60,220},{250,220, 40},{255,255,255},{ 60, 60, 60}}, 768, 255 },   //  7 Kitaoka: blue against yellow
  { {{230, 30, 30},{250,250,245},{ 30, 50,180},{250,250,245},{250,210, 20},{250,250,245}},   0,   0 },   //  8 De Stijl
  { {{255, 40,180},{  0,240,255},{180,255,  0},{255,120,  0},{140, 60,255},{  0,255,150}},   0,   0 },   //  9 Neon on black
  { {{245,235,210},{240,225,195},{235,215,180},{245,235,210},{230,210,170},{240,228,200}},   0,  45 },   // 10 Ink & Cream
  { {{255, 30, 30},{255,220,  0},{ 30,255, 60},{  0,200,255},{ 40, 60,255},{230,  0,255}}, 768, 120 },   // 11 Spectrum
};
static const char* const o7_palNames[O7_NPAL] = { "Black & White", "Ra", "Vega", "Albers", "Anuszkiewicz", "Agam", "Soto", "Kitaoka", "De Stijl", "Neon", "Ink & Cream", "Spectrum" };
enum { O7P_BW, O7P_RA, O7P_VEGA, O7P_ALBERS, O7P_ANUSZ, O7P_AGAM, O7P_SOTO, O7P_KITAOKA, O7P_STIJL, O7P_NEON, O7P_INK, O7P_SPECTRUM };
#define O7C(s, h) ((uint8_t)(((s) << 4) | (h)))
#define O7_GREY 15                                                      // the neutral hue

// ─── Small shared tables (worked out once) ────────────────────────────
static uint8_t o7_op[4][256];                     // two shades → one: 0 XOR, 1 AND (multiply), 2 OR (screen), 3 difference
static uint8_t o7_mod15[64];                      // n → n mod 15 (hues 0-14)
static bool o7_ready = false;
static int o7_frame = 0;
static uint32_t o7_nseed = 0x9E3779B9u;           // where the random wandering starts (new every power-up)
static float o7_rnd = 1.0f;                       // randomness, 0 … 2 (p13; 1 = normal)
static float o7_vt = 0;                           // the slow clock the settings drift on
static float o7_liq = 1.0f, o7_lt = 0;            // liquid flow, 0 … 2 (p15; 1 = gentle), and its clock
static float o7_liqK = 1.0f; static bool o7_liqC = true;   // a program's own amount (used while p15 is in the centre)
static float o7_tint = 0;                         // 0 … 1: how much the neutral hue (15) takes the palette's colour (a program may set it)
#define O7_SLOW 0.5f                              // Set 7's pace, compared with earlier sets
enum { O7_XOR, O7_AND, O7_OR, O7_DIFF };
static void o7_setup() {
  if (o7_ready) return;
  for (int a = 0; a < 16; a++) for (int b = 0; b < 16; b++) {
    int x = (a * (15 - b) + b * (15 - a) + 7) / 15;                      // soft XOR: paper where exactly one is paper
    o7_op[O7_XOR][a * 16 + b] = (uint8_t)x;
    o7_op[O7_AND][a * 16 + b] = (uint8_t)((a * b + 7) / 15);
    o7_op[O7_OR][a * 16 + b] = (uint8_t)(15 - ((15 - a) * (15 - b) + 7) / 15);
    o7_op[O7_DIFF][a * 16 + b] = (uint8_t)(a > b ? a - b : b - a);
  }
  for (int i = 0; i < 64; i++) o7_mod15[i] = (uint8_t)(i % 15);
  // a different start every power-up: the clock and the knobs' noise
  uint32_t mix = (uint32_t)micros() * 2654435761u;
  for (int i = 0; i < 16; i++) mix = (mix ^ (uint32_t)pots[i]) * 16777619u;
  v3_seed ^= mix | 1; o7_nseed = v3_hash(mix + 12345u);
  o7_ready = true;
}
// ─── Random wandering ─────────────────────────────────────────────────
// smooth random −1 … 1 (value noise), mixed with a sine so it still swings both ways;
// how much is random follows the Randomness knob
static inline float o7_noise(float x, uint32_t salt) {
  uint32_t t = (uint32_t)(int32_t)(x * 256.0f) + (1u << 24);
  return v3_noise1(t, o7_nseed ^ salt) * (1.0f / 16384.0f);
}
static inline float o7_nsin(float x) {
  float r = v3_clampf(o7_rnd * 0.7f, 0.0f, 1.0f);
  return sinf(x) * (1.0f - r) + o7_noise(x * 0.33f, (uint32_t)(x > 0 ? 1 : 2)) * r * 1.3f;
}
static inline float o7_ncos(float x) {
  float r = v3_clampf(o7_rnd * 0.7f, 0.0f, 1.0f);
  return cosf(x) * (1.0f - r) + o7_noise(x * 0.33f + 517.0f, 7u) * r * 1.3f;
}
// a preset's own setting, drifting slowly by up to ±30 % (more or less with Randomness)
static inline float o7_vary(int pot, float own) {
  return own * (1.0f + 0.22f * o7_rnd * o7_noise(o7_vt + pot * 37.0f, 0xA5u + pot));
}

// ─── Palette ──────────────────────────────────────────────────────────
static inline void o7_grad(const O7Pal& P, int p, int* o) {
  p %= 1536; if (p < 0) p += 1536;
  int i = p >> 8, f = p & 255, j = i == 5 ? 0 : i + 1;
  for (int c = 0; c < 3; c++) o[c] = P.c[i][c] + (((P.c[j][c] - P.c[i][c]) * f) >> 8);
}
// richer colour: push each colour away from its own grey (v256: 256 = as written)
static inline void o7_vib(int* c, int v256) {
  int l = (c[0] * 77 + c[1] * 150 + c[2] * 29) >> 8;
  for (int q = 0; q < 3; q++) { int x = l + (((c[q] - l) * v256) >> 8); c[q] = x < 0 ? 0 : (x > 255 ? 255 : x); }
}
#define O7_VIB 330                                                      // ≈ 1.3 × as colourful
// all 256 entries: 15 hues × (ink → paper), then hue 15 = black → white.
// spread256: 256 = the 15 hues span the palette once. sat256: 0 = black & white, 256 = full colour.
static bool o7_invert = false;                                          // swap ink and paper (a program may set it)
static void o7_palette(int spread256, int sat256) {
  int base = (int)(v6_flow * 1536.0f), xf = (int)(v6_palX * 256);
  const O7Pal& A = o7_pals[v6_palA]; const O7Pal& B = o7_pals[v6_palB];
  for (int h = 0; h < 15; h++) {
    int p = base + (h * spread256 * 6) / 15;
    int pa[3], pb[3], ia[3], ib[3], paper[3], ink[3];
    o7_grad(A, p, pa); o7_grad(B, p, pb); o7_grad(A, p + A.inkOff, ia); o7_grad(B, p + B.inkOff, ib);
    for (int q = 0; q < 3; q++) {
      int ca = (ia[q] * A.inkBr) >> 8, cb = (ib[q] * B.inkBr) >> 8;
      paper[q] = pa[q] + (((pb[q] - pa[q]) * xf) >> 8);
      ink[q] = ca + (((cb - ca) * xf) >> 8);
      paper[q] = 255 + (((paper[q] - 255) * sat256) >> 8);            // less colour: paper → white, ink → black
      ink[q] = (ink[q] * sat256) >> 8;
    }
    if (o7_invert) for (int q = 0; q < 3; q++) { int t = paper[q]; paper[q] = ink[q]; ink[q] = t; }
    int vb = (O7_VIB * sat256) >> 8; if (vb < 256) vb = 256;
    o7_vib(paper, vb); o7_vib(ink, vb);
    for (int s = 0; s < 16; s++) {
      int k = s * 273, c[3];                                            // s / 15, ×4096
      for (int q = 0; q < 3; q++) c[q] = ink[q] + (((paper[q] - ink[q]) * k) >> 12);
      o7_vib(c, 256 + ((s * (15 - s) * 3 * sat256) >> 8));             // the in-between shades: kept from going grey
      display.setColor((uint8_t)((s << 4) | h), (uint8_t)c[0], (uint8_t)c[1], (uint8_t)c[2]);
    }
  }
  // hue 15: neutral black → white (or, with o7_tint, a two-tone of the palette's ink and paper)
  int tk = (int)(o7_tint * sat256);                                     // 0 … 256
  int tp[3] = { 255, 255, 255 }, ti[3] = { 0, 0, 0 };
  if (tk > 0) {
    int p = base + 1536 / 4, a[3], b[3];
    o7_grad(A, p, a); o7_grad(A, p + 768, b);
    for (int q = 0; q < 3; q++) { tp[q] = 255 + (((a[q] - 255) * tk) >> 8); ti[q] = (b[q] * tk * 3) >> 10; }   // paper: a colour; ink: the opposite colour, dark
    o7_vib(tp, O7_VIB); o7_vib(ti, O7_VIB);
  }
  for (int s = 0; s < 16; s++) {
    int k = (o7_invert ? 15 - s : s) * 273, c[3];
    for (int q = 0; q < 3; q++) c[q] = ti[q] + (((tp[q] - ti[q]) * k) >> 12);
    display.setColor((uint8_t)((s << 4) | 15), (uint8_t)c[0], (uint8_t)c[1], (uint8_t)c[2]);
  }
}
// palette, colour flow, hue spread and colour amount from the common knobs (the look's own in the centre bands)
static float o7_sat = 1.0f;
static void o7_colours(int ownPal, float ownFlow, float ownSpread, float ownSat) {
  int pal = v3_centre(0) ? ownPal : v3_clamp(v3_k(0) * O7_NPAL / 1024, 0, O7_NPAL - 1);
  if (v6_first) { v6_palA = v6_palB = pal; v6_palX = 1.0f; }
  if (pal != v6_palB) { v6_palA = v6_palB; v6_palB = pal; v6_palX = 0; }
  if (v6_palX < 1.0f) { v6_palX += v3_dt / 1.6f; if (v6_palX >= 1.0f) { v6_palX = 1.0f; v6_palA = v6_palB; } }
  float own = ownFlow * 0.8f; if (own < 0.012f && ownSat > 0) own = 0.012f;          // (colour always drifts a little)
  float fl = v3_centre(3) ? own : v3_bi(3) * fabsf(v3_bi(3)) * 0.6f;
  v6_f.flowRate = fl;
  v6_flow += fl * v3_dt * (v6_f.speed > 0 ? 1.0f : 0.0f);
  v6_flow -= floorf(v6_flow);
  float sp = v3_centre(4) ? ownSpread : 0.08f + v3_u(4) * 1.4f;
  v6_f.spread = (int)(sp * 256);
  v6_ease(&o7_sat, v3_centre(11) ? ownSat : v3_u(11), 4.0f);
  o7_palette(v6_f.spread, (int)(o7_sat * 256));
}

// ─── Every frame ──────────────────────────────────────────────────────
static int o7_begin(int preset) {
  o7_setup();
  int look = v6_begin(preset, 11);
  // the slower pace: the program clock and its step
  float full = v6_f.sdt;
  v6_f.sdt = full * O7_SLOW; v6_t -= full * (1.0f - O7_SLOW);
  // no mirror and no glitch in Set 7; p13 becomes Randomness (its value was put aside by fx5_take)
  float k13 = fx5_saved[1], k15 = fx5_saved[3];                          // 512 until the knob is turned
  fx5_amt[1] = 0.0f; fx5_amt[3] = 0.0f;
  float rk = (k13 >= 440 && k13 < 600) ? 1.0f : k13 / 512.0f;
  v6_ease(&o7_rnd, v3_clampf(rk, 0.0f, 2.0f), 2.0f);
  float lk = (k15 >= 440 && k15 < 600) ? 1.0f : k15 / 512.0f;            // p15: liquid flow (p13, p15 use the FX knobs' "asleep until turned")
  o7_liqC = k15 >= 440 && k15 < 600; o7_liqK = 1.0f;
  v6_ease(&o7_liq, v3_clampf(lk, 0.0f, 2.0f), 2.0f);
  o7_lt += v6_f.sdt * (0.6f + 0.4f * o7_rnd);
  if (o7_lt > 5000.0f) o7_lt -= 5000.0f;
  o7_vt += v3_dt * 0.06f * (v6_f.speed > 0 ? 1.0f : 0.0f);
  if (o7_vt > 10000.0f) o7_vt -= 10000.0f;
  o7_frame++;
  o7_invert = false; o7_tint = 0;
  return look;
}
// ─── Liquid flow ──────────────────────────────────────────────────────
// Each row of the finished picture is taken from a row a little above or below (up to 11 rows) and
// slid sideways, by two slow, smooth random curves down the screen. The rows that are about to be
// written over are kept in a small ring of rows, so it all happens in place.
#define O7_RING 12
static uint8_t o7_ring[O7_RING * W];
static int16_t o7_ldx[H]; static int8_t o7_ldy[H];
static void o7_liquid() {
  static float k = 1.0f;
  v6_ease(&k, o7_liqC ? o7_liqK : 1.0f, 2.0f);                           // (a program's own amount, eased: no jumps)
  float a = o7_liq * k;
  if (a < 0.03f || !v3_buf) return;
  float ax = a <= 1.0f ? 7.0f * a : 7.0f + 21.0f * (a - 1.0f);           // sideways (px): gentle 7, strong 28
  float ay = a <= 1.0f ? 4.0f * a : 4.0f + 6.5f * (a - 1.0f);            // up and down (px): at most 10.5
  float t = o7_lt;
  // the two curves, worked out every 8th row (in 1/256 px) and joined by straight steps
  int32_t px0 = 0, py0 = 0;
  for (int y = 0; y <= H; y += 8) {
    float fy = (float)y;
    float n1 = o7_noise(fy * 0.010f + t * 0.35f, 0x51u), n2 = o7_noise(fy * 0.026f - t * 0.55f, 0x52u);
    float n3 = o7_noise(fy * 0.008f - t * 0.27f, 0x53u), n4 = o7_noise(fy * 0.021f + t * 0.42f, 0x54u);
    int32_t px1 = (int32_t)(256.0f * ax * (n1 * 0.75f + n2 * 0.35f)), py1 = (int32_t)(256.0f * ay * (n3 * 0.75f + n4 * 0.35f));
    if (y) for (int k = 0; k < 8; k++) {
      int yy = y - 8 + k;
      o7_ldx[yy] = (int16_t)((px0 * (8 - k) + px1 * k) / 2048);
      int dy = (py0 * (8 - k) + py1 * k) / 2048;
      o7_ldy[yy] = (int8_t)(dy < -(O7_RING - 1) ? -(O7_RING - 1) : (dy > O7_RING - 1 ? O7_RING - 1 : dy));
    }
    px0 = px1; py0 = py1;
  }
  uint8_t* buf = v3_buf;
  for (int y = 0; y < H; y++) {
    int s = y + o7_ldy[y]; if (s < 0) s = 0; if (s > H - 1) s = H - 1;
    uint8_t* out = buf + y * W;
    memcpy(o7_ring + (y % O7_RING) * W, out, W);                          // keep this row as it was drawn
    const uint8_t* src = s > y ? buf + s * W : o7_ring + (s % O7_RING) * W; // (rows above are already done: from the ring)
    int dx = o7_ldx[y] % W; if (dx < 0) dx += W;
    if (s == y && !dx) continue;
    memcpy(out + dx, src, W - dx);                                          // slid round (no black edges)
    if (dx) memcpy(out, src + W - dx, dx);
  }
}
static inline void o7_end() { o7_liquid(); fx5_apply(); }

// ─── Stripes ──────────────────────────────────────────────────────────
// A stripe profile over one period of n table steps: paper for the first `duty256`/256 of the
// period, ink for the rest, with edges that ramp over `edge16`/16 steps (anti-aliasing: make
// that about one pixel's worth of phase). Result: shades 0-15.
static void o7_band(uint8_t* lut, int n, int duty256, int edge16) {
  if (edge16 < 4) edge16 = 4;
  int D = (duty256 * n * 16) >> 8, N = n * 16;
  for (int i = 0; i < n; i++) {
    int p = i * 16 + 8, d;
    if (D <= 0) d = -N; else if (D >= N) d = N;
    else if (p < D) d = p < D - p ? p : D - p;
    else { int a = p - D, b = N - p; d = -(a < b ? a : b); }
    int s = 15 + (d * 30) / edge16;                                     // ×2: 15 = the half-way shade
    s = (s + 1) >> 1;
    lut[i] = (uint8_t)(s < 0 ? 0 : (s > 15 ? 15 : s));
  }
}
// a whole stripe field in one table: index = (stripe number & 15) << 8 | position in stripe (0-255)
// → (shade << 4) | hue, the hue stepping by `hueStep` each stripe (hueStep 0 → all one hue)
static void o7_comb(uint8_t* comb, const uint8_t* band, int hueBase, int hueStep) {
  for (int j = 0; j < 16; j++) {
    int h = hueStep < 0 ? O7_GREY : o7_mod15[(hueBase + j * hueStep) % 60];
    uint8_t* d = comb + (j << 8);
    for (int i = 0; i < 256; i++) d[i] = (uint8_t)((band[i] << 4) | h);
  }
}
// the anti-aliasing width (in 1/16 table steps) for stripes `periodPx` pixels wide, `soft` 0 … 1
static inline int o7_edge(float periodPx, int n, float soft) {
  float px = 1.25f + soft * soft * 14.0f;                                 // edge width in pixels (a little soft: steadier when moving)
  int e = (int)(px * n * 16.0f / (periodPx > 0.5f ? periodPx : 0.5f));
  return e < 4 ? 4 : e;
}
// four bytes at once into a word-aligned row
#define O7_PUT4(p, a, b, c, d) (*(uint32_t*)(p) = (uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

// ─── Labels ───────────────────────────────────────────────────────────
static const char* o7_label(int pot, const char* const* own) {
  if (pot >= 0 && pot <= 4) return v6_commonLbl[pot];
  if (pot == 13) return "Randomness";
  if (pot == 15) return "Liquid Flow";
  if (pot >= 12 && pot <= 15) return fx5_labels[pot - 12];
  if (pot >= 5 && pot <= 10) return own[pot - 5];
  if (pot == 11) return "Colour: B&W|Full";
  return "";
}
static inline float o7_kn(int pot, float own) { return v3_centre(pot) ? v3_clampf(o7_vary(pot, own), 0.0f, 1.0f) : v3_u(pot); }    // 0 … 1
static inline float o7_kb(int pot, float own) { return v3_centre(pot) ? v3_clampf(o7_vary(pot, own), -1.0f, 1.0f) : v3_bi(pot); }   // −1 … 1
// (o7_kn / o7_kb: the preset's own value drifts a little; for counts and switches use o7_pick or v3_centre directly)
static inline int o7_pick(int pot, int own, int n) { return v3_centre(pot) ? own : v3_clamp(v3_k(pot) * n / 1024, 0, n - 1); }
#endif

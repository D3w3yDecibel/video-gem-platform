// =====================================================================
// PROGRAM: QUASI — quasicrystals (by Dewey) — Set 6, slot 9
//
// Lay five, seven or twelve sets of straight waves across each other, all
// at even angles, and they make a pattern that is perfectly ordered yet
// never repeats: a quasicrystal, like a Penrose tiling or the tiles of a
// mosque in Isfahan. With three sets it locks into a honeycomb; with six or
// twelve, star flowers and rings of ripples. As the waves flow, rosettes are born
// at every point and dissolve into others, endlessly.
//
// How it works: for every pixel, a handful of sine waves are added up (one
// table lookup each), worked out at half resolution — the patterns are smooth.
//
// Presets: k0 Penrose (5) · k1 Honeycomb (3) · k2 Star Flowers (6) · k3 Heptagram (7) ·
//          k4 Octagram (8) · k5 Ripple Rose (12) · k6 Op Art (5, black & white) ·
//          k7 Liquid (7, warped) · k8 Contours (9) · k9 Moiré Lotus (6, two scales) ·
//          k10 Spectrum (11) · k11 Tour
//   k12 — SCATTER: the waves slip out of step, then slide back into a crystal
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Waves (3 … 13; centre = preset) · p6 Wavelength · p7 Bands (◄ soft | sharp ►)
//   p8 Spin · p9 Flow · p10 Warp · p11 Contrast
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

struct QuLook { uint8_t n, pal, wl, bands, spin, flow, warp, contrast, two, cflow, spread; };
static const QuLook qu_looks[11] = {
  // n pal wl bands spin flow warp contr two cflow spread
  {  5,  0, 40, 50, 6, 50,  0, 50, 0, 5, 70 },   // Penrose
  {  3,  9, 45, 60, 5, 40,  0, 60, 0, 4, 50 },   // Honeycomb
  {  6,  2, 40, 50, 6, 45,  0, 50, 0, 5, 70 },   // Star Flowers
  {  7,  4, 35, 55, 8, 50,  0, 50, 0, 6, 80 },   // Heptagram
  {  8,  3, 35, 50, 6, 50,  0, 50, 0, 5, 70 },   // Octagram
  { 12,  8, 30, 50, 5, 40,  0, 55, 0, 6, 90 },   // Ripple Rose
  {  5,  7, 75, 95, 4, 50,  0, 60, 0, 3, 20 },   // Op Art
  {  7,  5, 45, 40, 6, 55, 60, 50, 0, 7, 70 },   // Liquid
  {  9,  6, 30, 80, 4, 45,  0, 60, 0, 5, 80 },   // Contours
  {  6, 10, 40, 50, 6, 45,  0, 50, 1, 6, 90 },   // Moiré Lotus
  { 11, 11, 30, 55, 8, 50, 20, 55, 0, 9,100 },   // Spectrum
};
static const char* const qu_names[12] = { "Penrose", "Honeycomb", "Star Flowers", "Heptagram", "Octagram", "Ripple Rose", "Op Art", "Liquid", "Contours", "Moire Lotus", "Spectrum", "Tour" };
static const char* const qu_lbl[7] = { "Waves", "Wavelength", "Bands", "Spin", "Flow", "Warp", "Contrast" };
const char* prog_quasi_name() { return "QUASI"; }
const char* prog_quasi_character() { return "Quasicrystals: 3 to 13 sets of waves crossing into rosettes that never repeat"; }
const char* prog_quasi_presetName(int p) { return (p >= 0 && p < 12) ? qu_names[p] : NULL; }
const char* prog_quasi_potLabel(int p, int pot) { (void)p; return v6_label(pot, qu_lbl); }
uint8_t prog_quasi_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define QU_MAXN 14
static int8_t* qu_sin;                     // 256-step sine, ±127
static uint8_t* qu_lut;                    // summed waves → colour
static int16_t* qu_sum;                    // one half-resolution row of sums
static void qu_mem() { qu_sin = (int8_t*)v6_mem; qu_lut = v6_mem + 256; qu_sum = (int16_t*)(v6_mem + 256 + 4096); }

// add one wave across a row: phase p (1/256 of a sine step), step dp per (half-res) pixel
__attribute__((optimize("O2"))) static void qu_wave(int16_t* sum, int32_t p, int32_t dp, int n, int amp) {
  const int8_t* S = qu_sin;
  if (amp == 256) for (int x = 0; x < n; x++, p += dp) sum[x] += S[(p >> 8) & 255];
  else for (int x = 0; x < n; x++, p += dp) sum[x] += (S[(p >> 8) & 255] * amp) >> 8;
}
// a row of sums → colours, each one 2 × 2 pixels
__attribute__((optimize("O2"))) static void qu_out(uint8_t* row, const int16_t* sum, const uint8_t* lut, int off) {
  for (int x = 0; x < W / 2; x++) { uint8_t c = lut[sum[x] + off]; row[2 * x] = c; row[2 * x + 1] = c; }
  memcpy(row + W, row, W);
}

static float qu_ang = 0, qu_ph = 0, qu_wl, qu_bands, qu_spin, qu_flow, qu_warp, qu_contr, qu_nf = 5, qu_scat = 0;
static float qu_off[QU_MAXN];

void prog_quasi_init() {
  qu_mem(); v6_init();
  for (int i = 0; i < 256; i++) qu_sin[i] = (int8_t)((v3_sin(i * 4) * 127) / 16384);
  for (int i = 0; i < QU_MAXN; i++) qu_off[i] = 0;
}

void prog_quasi_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  qu_mem();
  const QuLook& L = qu_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  float tN = v3_centre(5) ? L.n : 3.0f + v3_u(5) * 10.0f;
  float tWl = v3_centre(6) ? L.wl / 100.0f : v3_u(6);
  float tBands = v3_centre(7) ? L.bands / 100.0f : v3_u(7);
  float tSpin = v3_centre(8) ? L.spin / 100.0f : v3_bi(8);
  float tFlow = v3_centre(9) ? L.flow / 100.0f : v3_bi(9);
  float tWarp = v3_centre(10) ? L.warp / 100.0f : v3_u(10);
  float tContr = v3_centre(11) ? L.contrast / 100.0f : v3_u(11);
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&qu_wl, tWl, rate); v6_ease(&qu_bands, tBands, rate); v6_ease(&qu_spin, tSpin, rate); v6_ease(&qu_flow, tFlow, rate);
  v6_ease(&qu_warp, tWarp, rate); v6_ease(&qu_contr, tContr, rate); v6_ease(&qu_nf, tN, rate * 0.6f);
  v6_colours(L.pal, L.cflow / 100.0f, L.spread / 100.0f, 120);

  // ── waves: n whole ones, and the next fading in, so turning the knob morphs smoothly ──
  int n = (int)qu_nf; float frac = qu_nf - n; if (n < 2) { n = 2; frac = 0; }
  if (n >= QU_MAXN) { n = QU_MAXN - 1; frac = 0; }
  int nw = frac > 0.02f ? n + 1 : n;
  qu_ang += sdt * qu_spin * 0.1f;
  qu_ph += sdt * qu_flow * 1.2f;
  if (v6_k12hit) { qu_scat = 1.0f; for (int i = 0; i < QU_MAXN; i++) qu_off[i] = v3_rndf() * 6.28f; }
  qu_scat -= dt * 0.35f; if (qu_scat < 0) qu_scat = 0;
  float sc = qu_scat * qu_scat * (3 - 2 * qu_scat);

  // ── the colour table: the sum of the waves → bands of colour ──
  int maxS = 127 * nw, sz = 2 * maxS + 1;
  if (sz > 4096) sz = 4096;
  // (whole numbers only: this runs for up to 3500 entries a frame)
  int bandsK = (int)((0.7f + qu_bands * 2.6f) * 256);              // colour bands across the range, ×256
  int cK = (int)((0.6f + qu_contr * 1.6f) * 256);                  // contrast, ×256
  bool sharp = qu_bands > 0.9f; int fK = qu_bands > 0.5f ? 1024 : 512;
  int div = maxS > 0 ? maxS : 1;
  for (int i = 0; i < sz; i++) {
    int v = ((i - maxS) * 4096) / div;                              // −4096 … 4096
    v = (v * cK) >> 8;
    int b = (((v + 4096) * bandsK) >> 8) & 8191;                    // position within a band pair, 8192 = 2 bands
    int lev;
    if (sharp) lev = (b & 4095) < 2048 ? 15 : 1;
    else { int c = v3_cos(((b & 4095) * fK) >> 12); lev = 8 + ((c * 7) >> 14); }
    if (v > 3700) lev = 15;
    int hue = ((v + 4096) >> 9) & 15;
    qu_lut[i] = (uint8_t)((v3_clamp(lev, 0, 15) << 4) | hue);
  }

  // ── each wave's direction, spacing and phase (half-res pixels) ──
  float z = v6_zoom(1.0f);
  float wlpx = (9.0f + qu_wl * 30.0f) * z;                         // wavelength in half-res px
  float k = 256.0f * 256.0f / wlpx;                                 // phase steps (1/256 of a table step) per px
  int32_t kx[QU_MAXN], ky[QU_MAXN], p0[QU_MAXN]; int amp[QU_MAXN];
  for (int i = 0; i < nw; i++) {
    float th = qu_ang + i * 3.14159265f / (nw > n ? n + frac : n); // even angles over half a turn
    kx[i] = (int32_t)(cosf(th) * k); ky[i] = (int32_t)(sinf(th) * k);
    float ph = qu_ph * (1.0f + (L.two && (i & 1) ? 0.618f : 0.0f)) + qu_off[i] * sc;
    if (qu_warp > 0.01f) ph += qu_warp * 2.0f * sinf(v6_t * 0.31f + i * 2.4f);
    p0[i] = (int32_t)(ph * 256.0f * 256.0f / 6.2832f);
    amp[i] = (i == n && frac > 0.02f) ? (int)(frac * 256) : 256;
  }
  // Moiré Lotus: odd waves at a longer wavelength
  if (L.two) for (int i = 1; i < nw; i += 2) { kx[i] = kx[i] * 5 / 8; ky[i] = ky[i] * 5 / 8; }
  int off = maxS;
  for (int y = 0; y < H / 2; y++) {
    for (int x = 0; x < W / 2; x++) qu_sum[x] = 0;
    int yy = y - 60;
    for (int i = 0; i < nw; i++) {
      int32_t wy = 0;
      if (qu_warp > 0.01f) wy = (int32_t)(qu_warp * 900.0f) * qu_sin[(y * 3 + i * 40 + (int)(v6_t * 30)) & 255];   // a slow wobble along each wave
      qu_wave(qu_sum, p0[i] + yy * ky[i] - 80 * kx[i] + wy, kx[i], W / 2, amp[i]);
    }
    qu_out(v3_buf + 2 * y * W, qu_sum, qu_lut, off);
  }
  v6_end();
}

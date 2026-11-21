// =====================================================================
// PROGRAM: WHITNEY — John Whitney's differential motion (by Dewey) — Set 7, slot 9
//
// John Whitney built analog computers out of anti-aircraft gun directors
// to make animation ("Catalog", 1961), then made "Permutations" and
// "Arabesque" with the simplest idea of all: let a crowd of points move at
// rates 1, 2, 3, 4 … — each one a little faster than the one before. Most
// of the time they scatter; but again and again they fall into step, into
// rings, stars, spirals and rosettes — visual harmony, as in music, where
// the notes are whole-number multiples of each other.
//
// How it works: point i moves at i times the base rate, so its angle is
// i·τ (one integer multiply each, wrapping round on its own). Each preset
// places the points differently — on a circle, a spiral, a rose, a grid,
// in columns — and draws them as glowing dots, joins them with lines, or
// both, with a phosphor afterglow.
//
// Presets: k0 Permutations · k1 Arabesque · k2 Matrix · k3 Spiral · k4 Rose ·
//          k5 Lattice · k6 Galaxy · k7 Harmony · k8 Columns · k9 Bloom ·
//          k10 Catalog · k11 Tour
//   k12 — UNISON: slide to the nearest moment of harmony (a simple fraction)
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Points · p6 Rate (◄ back | forward ►) · p7 Shape · p8 Trails
//   p9 Links (◄ dots | lines ►) · p10 Dot Size · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { WN_CIRCLE, WN_ARABESQUE, WN_MATRIX, WN_SPIRAL, WN_ROSE, WN_LATTICE, WN_GALAXY, WN_HARMONY, WN_COLUMNS, WN_BLOOM, WN_CATALOG };
struct WnLook { uint16_t n; int8_t rate; uint8_t shape, trail, links, dot, pal, flow, spread, scan; };
static const WnLook wn_looks[11] = {
  //  n   rate shape trail links dot pal              flow spr scan
  { 120,  25,  50,  60,   0,  40, V7P_CHICAGO,      4, 100, 1 },   // Permutations
  { 180,  20,  40,  70,  60,  20, V7P_SCANIMATE,    5, 100, 1 },   // Arabesque
  { 192,  30,  50,  40,   0,  30, V7P_PAIK,         4, 100, 1 },   // Matrix
  { 200,  15,  50,  65,   0,  30, V7P_LUMIA,        4, 100, 1 },   // Spiral
  { 150,  20,  50,  60, 100,  10, V7P_BLACKLIGHT,   5, 100, 1 },   // Rose
  { 120,  25,  40,  70,  60,  20, V7P_VASULKA,      4, 100, 1 },   // Lattice
  { 256,  10,  50,  70,   0,  20, V7P_SUNSET,       4, 100, 1 },   // Galaxy
  { 140,  25,  50,  55,  30,  30, V7P_STUDIO,       5, 100, 1 },   // Harmony
  { 100,  30,  50,  60,  60,  30, V7P_PHOSPHOR,     3,  70, 1 },   // Columns
  { 160,  20,  50,  60,   0,  40, V7P_KODACHROME,   5, 100, 1 },   // Bloom
  { 120,  15,  30,  88,   0,  20, V7P_AMBER,        3,  60, 1 },   // Catalog
};
static const char* const wn_names[12] = { "Permutations", "Arabesque", "Matrix", "Spiral", "Rose", "Lattice", "Galaxy", "Harmony", "Columns", "Bloom", "Catalog", "Tour" };
static const char* const wn_lbl[6] = { "Points", "Rate", "Shape", "Trails", "Links: Dots|Lines", "Dot Size" };
const char* prog_whitney_name() { return "WHITNEY"; }
const char* prog_whitney_character() { return "John Whitney's differential motion: points at rates 1, 2, 3 … falling into harmony"; }
const char* prog_whitney_presetName(int p) { return (p >= 0 && p < 12) ? wn_names[p] : NULL; }
const char* prog_whitney_potLabel(int p, int pot) { (void)p; return v7_label(pot, wn_lbl); }
uint8_t prog_whitney_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static float wn_tau = 0.137f, wn_n, wn_rate, wn_shape, wn_trail, wn_links, wn_dot, wn_unison = -1, wn_from, wn_to;
static int16_t wn_px[257], wn_py[257];
void prog_whitney_init() { v6_init(); wn_tau = 0.137f; wn_unison = -1; }

// the nearest simple fraction p/q (q ≤ 12) to x, by trying them all
static float wn_nearest(float x) {
  float best = 0, bd = 2;
  for (int q = 1; q <= 12; q++) { float p = floorf(x * q + 0.5f), d = fabsf(x - p / q); if (d < bd - 0.0005f) { bd = d; best = p / q; } }
  return best;
}

void prog_whitney_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  const WnLook& L = wn_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&wn_n, v3_centre(5) ? (float)L.n : 12.0f + v3_u(5) * 244.0f, rate);
  v6_ease(&wn_rate, v7_kb(6, L.rate / 100.0f), rate);
  v6_ease(&wn_shape, v7_kn(7, L.shape / 100.0f), rate);
  v6_ease(&wn_trail, v7_kn(8, L.trail / 100.0f), rate);
  v6_ease(&wn_links, v7_kn(9, L.links / 100.0f), rate);
  v6_ease(&wn_dot, v7_kn(10, L.dot / 100.0f), rate);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 130);

  float sdt = v6_f.sdt;
  if (v6_k12hit) { wn_unison = 0; wn_from = wn_tau; wn_to = wn_nearest(wn_tau); if (fabsf(wn_to - wn_from) < 0.0003f) wn_to = wn_nearest(wn_tau + 0.04f); }
  if (wn_unison >= 0) {
    wn_unison += v6_f.dt * 0.6f;
    float e = wn_unison >= 1.0f ? 1.0f : wn_unison * wn_unison * (3 - 2 * wn_unison);
    wn_tau = wn_from + (wn_to - wn_from) * e;
    if (wn_unison >= 1.6f) wn_unison = -1;                                // hold the harmony a moment, then go on
  } else wn_tau += sdt * wn_rate * 0.010f;
  wn_tau -= floorf(wn_tau);

  int dec = 6 + (int)((1.0f - wn_trail) * (1.0f - wn_trail) * 120);
  v7_decay(dec);

  int n = v3_clamp((int)(wn_n + 0.5f), 4, 256);
  float z = v6_zoom(1.0f);
  int32_t R = (int32_t)(108 * 8 * z);                                     // 1/8 px
  uint32_t tq = (uint32_t)(uint64_t)((double)wn_tau * 4294967296.0);                       // τ as a 32-bit turn: i·τ wraps on its own
  float sh = wn_shape;
  uint32_t tqL = tq + (uint32_t)(uint64_t)((double)tq * sh * 2.0);           // (Lattice: the second, faster rate)
  int cx = HALFW * 8, cy = HALFH * 8;
  for (int i = 0; i < n; i++) {
    uint32_t ph = (uint32_t)(i + 1) * tq;
    int a = (int)(ph >> 22);                                              // i·τ, 1024 per turn
    int b = (i * 1024) / n;                                               // i / n round the circle
    int32_t x = 0, y = 0;
    switch (look) {
      case WN_CIRCLE: case WN_CATALOG: {
        int32_t r = look == WN_CIRCLE ? R : (R * (2 + (int)(sh * 6)) * (i + 8)) / ((n + 8) * 4);
        x = (r * v3_cos(a + b)) >> 14; y = (r * v3_sin(a + b)) >> 14; } break;
      case WN_ARABESQUE: {                                                // a ring breathing in and out, each point at its own rate
        int32_t r = (R * (8192 + (v3_sin(a) >> 1))) >> 14;
        int k = 1 + (int)(sh * 6);
        x = (r * v3_cos(b * k / 2)) >> 14; y = (r * v3_sin(b)) >> 14; } break;
      case WN_MATRIX: {                                                   // a grid of points, each on its own little circle
        int cols = 16, gx = i % cols, gy = i / cols, rows = (n + cols - 1) / cols;
        int32_t sp = (int32_t)(19 * 8 * z), rr = (int32_t)((4 + sh * 14) * 8 * z);
        int aa = (int)((uint32_t)(gx + gy + 1) * tq >> 22);
        x = ((2 * gx - (cols - 1)) * sp) / 2 + ((rr * v3_cos(aa)) >> 14);
        y = ((2 * gy - (rows - 1)) * sp) / 2 + ((rr * v3_sin(aa)) >> 14); } break;
      case WN_SPIRAL: {
        int32_t r = (R * (i + 1)) / n;
        x = (r * v3_cos(a)) >> 14; y = (r * v3_sin(a)) >> 14; } break;
      case WN_ROSE: {
        int k = 2 + (int)(sh * 7);
        int32_t r = (R * v3_cos(b * k / 2 + (a >> 2))) >> 14;
        x = (r * v3_cos(b)) >> 14; y = (r * v3_sin(b)) >> 14; } break;
      case WN_LATTICE: {                                                  // Lissajous: across at rate i, down at rate 2i (+ shape)
        int a2 = (int)((uint32_t)(i + 1) * tqL >> 22);
        x = (R * 13 / 10 * v3_sin(a + b / 4)) >> 14; y = (R * v3_sin(a2 * 2 + b)) >> 14; } break;
      case WN_GALAXY: {                                                   // radius grows as √i, like a sunflower
        int32_t r = (int32_t)(R * sqrtf((i + 1) / (float)n));
        int g = (int)(i * 391.0f * (0.5f + sh));
        x = (r * v3_cos(a + g)) >> 14; y = (r * v3_sin(a + g)) >> 14; } break;
      case WN_HARMONY: {                                                  // two rings turning opposite ways
        bool in = i & 1; int32_t r = in ? R / 2 : R;
        int aa = in ? -a : a;
        x = (r * v3_cos(aa + b)) >> 14; y = (r * v3_sin(aa + b)) >> 14; } break;
      case WN_COLUMNS: {                                                  // a row of points, each bobbing at its own rate
        x = (int32_t)((2 * i - n + 1) * (int32_t)(150 * 8 * z) / n);
        y = (R * v3_sin(a + (int)(b * sh))) >> 14; } break;
      default: {                                                          // bloom: petals opening and closing at rate i
        int32_t r = (R * v3_abs(v3_sin(a))) >> 14;
        int g = (int)(i * 391.0f * (0.5f + sh));
        x = (r * v3_cos(g)) >> 14; y = (r * v3_sin(g)) >> 14; }
    }
    wn_px[i] = (int16_t)v3_clamp(cx + x, -8000, 8 * W + 8000);
    wn_py[i] = (int16_t)v3_clamp(cy + y, -8000, 8 * H + 8000);
  }
  // ── draw: lines between neighbours, then dots ──
  float lk = wn_links;
  int lineLev = (int)(lk * 2.0f * 9), dotLev = (int)((1.0f - lk) * 2.0f * 15);
  lineLev = v3_clamp(lineLev, 0, 9); dotLev = v3_clamp(dotLev, 0, 15);
  if (lineLev > 0) {
    bool ring = look != WN_COLUMNS && look != WN_MATRIX;
    for (int i = 0; i + 1 < n + (ring ? 1 : 0); i++) {
      int j = (i + 1) % n;
      if (look == WN_MATRIX && (j % 16) == 0) continue;
      v6_lineAdd(wn_px[i], wn_py[i], wn_px[j], wn_py[j], lineLev, (i * 16 / n) & 15);
    }
  }
  if (dotLev > 0) {
    int rd = (int)(0.6f + wn_dot * 4 * z);
    for (int i = 0; i < n; i++) {
      int hue = (i * 16 / n) & 15;
      if (rd <= 0) v6_add(wn_px[i] >> 3, wn_py[i] >> 3, dotLev, hue);
      else v6_dot(wn_px[i], wn_py[i], rd, dotLev, hue);
    }
  }
  v7_end(L.scan);
}

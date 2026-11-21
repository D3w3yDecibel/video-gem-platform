// =====================================================================
// PROGRAM: WHITNEY — differential motion & drawing machines (by Dewey) — Set 8 slot 6
//
// Greatest hit from Set 7 (WHITNEY) and Set 1 (Hypnotic: Harmonograph and
// Spirograph): John Whitney's "differential motion" — dot 1 turns once, dot
// 2 twice, dot 3 three times … so the dots fall into rings, petals and
// spirals and back into chaos — plus the slow drawing machines: a
// harmonograph pen, a spirograph and a pendulum wave. Glowing dots and
// lines on black, with phosphor trails.
//
// How it works: dot i's angle is i × T, where T is one slowly turning
// 32-bit phase; the pattern (circle, rose, Lissajous, sunflower) places it.
// The pens draw many short anti-aliased steps per frame. The picture fades
// a little each frame, so everything leaves a trail.
//
// Presets: k0 Permutations · k1 Arabesque · k2 Lissajous · k3 Sunflower ·
//          k4 Harmonograph · k5 Spirograph · k6 Matrix · k7 Galaxy ·
//          k8 Pendulum Wave · k9 Ember Pen · k10 Bloom · k11 Tour
//   k12 — UNISON: everything glides into line, holds, and lets go
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Points · p6 Pattern · p7 Shape · p8 Trails · p9 Dot Size · p10 Lines
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8WH_PERM, H8WH_ROSE, H8WH_LISS, H8WH_SUN, H8WH_HARMO, H8WH_SPIRO, H8WH_PEND, H8WH_NPAT };
struct H8WhLook { uint8_t pat, n, shape, trails, dot, lines, rate, pal, flow, spread; int8_t key; };
static const H8WhLook h8wh_looks[11] = {
  // pattern     n shape trail dot lines rate pal          flow spr key
  { H8WH_PERM,  72,  40,  60,  45,   0,  50, V8P_LOUNGE,   3,  90,  0 },   // Permutations
  { H8WH_ROSE,  96,  50,  65,  40,  20,  45, V8P_VELVET,   2,  80,  0 },   // Arabesque
  { H8WH_LISS,  64,  45,  60,  45,  25,  45, V8P_RAINBOW,   3,  80,  0 },   // Lissajous
  { H8WH_SUN,  120,  50,  55,  40,   0,  40, V8P_FOREST,   2,  80,  0 },   // Sunflower
  { H8WH_HARMO,  0,  50,  92,  40,   0,  50, V8P_DUSK,     2,  90,  0 },   // Harmonograph
  { H8WH_SPIRO,  0,  45,  90,  40,   0,  50, V8P_GALAXY,    3,  90,  0 },   // Spirograph
  { H8WH_PERM,  64,  60,  55,  35,  60,  45, V8P_AURORA,   3,  90,  0 },   // Matrix
  { H8WH_SUN,  120,  70,  65,  35,  30,  50, V8P_SUNSET,   2,  90,  0 },   // Galaxy
  { H8WH_PEND,  24,  50,  60,  60,  12,  50, V8P_NEON,  3, 100,  0 },   // Pendulum Wave
  { H8WH_HARMO,  0,  65,  90,  45,   0,  55, V8P_EMBER,    2,  60,  0 },   // Ember Pen
  { H8WH_ROSE, 120,  70,  60,  35,  50,  45, V8P_PEACH,    2,  80,  0 },   // Bloom
};
static const char* const h8wh_names[12] = { "Permutations", "Arabesque", "Lissajous", "Sunflower", "Harmonograph", "Spirograph", "Matrix", "Galaxy", "Pendulum Wave", "Ember Pen", "Bloom", "Tour" };
static const char* const h8wh_lbl[6] = { "Points", "Pattern", "Shape", "Trails", "Dot Size", "Lines" };
const char* prog_ghwhitney_name() { return "WHITNEY"; }
const char* prog_ghwhitney_character() { return "Differential motion and drawing machines: dots and pens tracing slow glowing figures"; }
const char* prog_ghwhitney_presetName(int p) { return (p >= 0 && p < 12) ? h8wh_names[p] : NULL; }
const char* prog_ghwhitney_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8wh_lbl); }
uint8_t prog_ghwhitney_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8WH_MAXN 128
static float h8wh_n = 64, h8wh_shape = 0.5f, h8wh_trails = 0.6f, h8wh_dot = 0.4f, h8wh_lines = 0;
static uint32_t h8wh_T = 0;                     // the master phase: 2^32 = one turn
static float h8wh_uni = -1; static uint32_t h8wh_uT0 = 0, h8wh_uT1 = 0;
// the pens
static float h8wh_pt = 0, h8wh_amp = 1, h8wh_fx1, h8wh_fx2, h8wh_fy1, h8wh_fy2, h8wh_px1, h8wh_px2, h8wh_py1, h8wh_py2, h8wh_rot = 0;
static int32_t h8wh_lx = 0, h8wh_ly = 0; static bool h8wh_have = false;
static float h8wh_sR, h8wh_sr, h8wh_sd; static int h8wh_hue = 0;

static void h8wh_newPen(int pat) {
  static const float ratios[6][2] = { { 2, 3 }, { 3, 4 }, { 1, 2 }, { 3, 5 }, { 2, 5 }, { 4, 5 } };
  int k = v3_rnd(6);
  float det = (v3_rndf() - 0.5f) * 0.02f;
  h8wh_fx1 = ratios[k][0]; h8wh_fy1 = ratios[k][1] + det;
  h8wh_fx2 = ratios[(k + 2) % 6][1] + det * 0.5f; h8wh_fy2 = ratios[(k + 3) % 6][0];
  h8wh_px1 = v3_rndf() * 6.28f; h8wh_px2 = v3_rndf() * 6.28f; h8wh_py1 = v3_rndf() * 6.28f; h8wh_py2 = v3_rndf() * 6.28f;
  h8wh_amp = 1; h8wh_pt = 0; h8wh_have = false;
  static const float sp[5][2] = { { 5, 3 }, { 7, 4 }, { 8, 5 }, { 9, 4 }, { 11, 7 } };
  int s = v3_rnd(5);
  h8wh_sR = sp[s][0]; h8wh_sr = sp[s][1]; h8wh_sd = 0.6f + v3_rndf() * 1.0f;
  h8wh_hue = (h8wh_hue + 3) & 15;
  (void)pat;
}
void prog_ghwhitney_init() { v8_init(); h8wh_uni = -1; h8wh_newPen(0); }

void prog_ghwhitney_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8WhLook& L = h8wh_looks[look];
  int pat = v3_pick(6, L.pat, H8WH_NPAT);
  float rate = v8_rate();
  v6_ease(&h8wh_n, v3_centre(5) ? (float)(L.n ? L.n : 64) : 8.0f + v3_u(5) * (H8WH_MAXN - 8), rate);
  v6_ease(&h8wh_shape, v8_kn(7, L.shape / 100.0f), rate);
  v6_ease(&h8wh_trails, v8_kn(8, L.trails / 100.0f), rate);
  v6_ease(&h8wh_dot, v8_kn(9, L.dot / 100.0f), rate);
  v6_ease(&h8wh_lines, v8_kn(10, L.lines / 100.0f), rate);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 120, L.key / 100.0f);
  bool pen = pat == H8WH_HARMO || pat == H8WH_SPIRO;
  if (v6_first || v6_f.fresh) { memset(v3_buf, 0, W * H); h8wh_newPen(pat); h8wh_T = v3_rand(); }
  else if (v8_fade < 0.5f) v8_decay(40);
  else if (pen) { if (v8_frameNo % (1 + (int)(h8wh_trails * 8)) == 0) v8_decay(1); }   // pens: the drawing lingers
  else v8_decay(1 + (int)((1.0f - h8wh_trails) * 30));

  // the master phase (Unison: glide to the nearest quarter turn, hold, let go)
  uint32_t dT = (uint32_t)(v6_f.sdt * (0.0012f + 0.003f * L.rate / 100.0f) * 4294967296.0f);
  if (v6_k12hit && h8wh_uni < 0) { h8wh_uni = 0; h8wh_uT0 = h8wh_T; h8wh_uT1 = (h8wh_T + 0x20000000u) & 0xC0000000u; }
  if (h8wh_uni >= 0) {
    h8wh_uni += v6_f.dt / 6.0f;
    if (h8wh_uni < 0.6f) {                                           // glide (0 … 0.4), then hold
      float e = h8wh_uni < 0.4f ? h8wh_uni / 0.4f : 1.0f; e = e * e * (3 - 2 * e);
      h8wh_T = h8wh_uT0 + (uint32_t)(int32_t)((float)(int32_t)(h8wh_uT1 - h8wh_uT0) * e);
    } else h8wh_T += dT;                                              // … and let go
    if (h8wh_uni >= 1.0f) h8wh_uni = -1;
  } else h8wh_T += dT;

  float z = v8_zoom(1.0f);
  int cx = W * 4, cy = H * 4;                                         // the middle, 1/8 px
  int R = (int)(105 * 8 * z);
  int n = v3_clamp((int)(h8wh_n + 0.5f), 4, H8WH_MAXN);
  int dr = 1 + (int)(h8wh_dot * 7.0f), dl = 5 + (int)(h8wh_dot * 9);
  int lineLev = (int)(h8wh_lines * 11);
  if (pat <= H8WH_SUN || pat == H8WH_PEND) {
    int32_t lx = 0, ly = 0;
    int k = 2 + (int)(h8wh_shape * 12);                                // petals / Lissajous ratio
    for (int i = 1; i <= n; i++) {
      uint32_t a32 = (uint32_t)i * h8wh_T;
      int a = (int)(a32 >> 22);                                       // 1024 per turn
      int32_t x, y;
      switch (pat) {
        case H8WH_ROSE: { int32_t rr = (R * v3_cos((a * k) >> 1)) >> 14; x = cx + ((rr * v3_cos(a)) >> 14); y = cy + ((rr * v3_sin(a)) >> 14); break; }
        case H8WH_LISS: x = cx + ((R * 3 / 2 * v3_sin(a)) >> 14); y = cy + ((R * v3_sin((a * k + 256 * (i & 3)) >> 1)) >> 14); break;
        case H8WH_SUN: {
          int ga = (int)(((uint32_t)i * 2654435769u) >> 22) + (int)((a32 >> 25) * (1 + (int)(h8wh_shape * 3)));
          int32_t rr = (R * (int32_t)v3_isqrt((uint32_t)(i * 65536 / n))) >> 8;
          x = cx + ((rr * v3_cos(ga)) >> 14); y = cy + ((rr * v3_sin(ga)) >> 14); break; }
        case H8WH_PEND: {                                             // a row of pendulums, each a little faster
          x = cx - R * 3 / 2 + (R * 3 * (i - 1)) / (n > 1 ? n - 1 : 1);
          uint32_t ph = (uint32_t)(i + 8 + (int)(h8wh_shape * 8)) * (h8wh_T * 20u);
          y = cy + ((R * v3_sin((int)(ph >> 22))) >> 14); break; }
        default: { int32_t rr = R * i / n; x = cx + ((rr * v3_cos(a)) >> 14); y = cy + ((rr * v3_sin(a)) >> 14); break; }
      }
      int hue = (i * 16 / n) & 15;
      if (lineLev && i > 1) v6_lineAdd(lx, ly, x, y, lineLev, hue);
      v6_dot(x, y, dr, dl, hue);
      lx = x; ly = y;
    }
  } else {
    // the pens: many short steps per frame
    int steps = 24;
    float sdt = v6_f.sdt * 0.9f / steps;
    h8wh_rot += v6_f.sdt * 0.03f;
    float cr = cosf(h8wh_rot), sr = sinf(h8wh_rot);
    for (int s = 0; s < steps; s++) {
      h8wh_pt += sdt;
      float fx, fy;
      if (pat == H8WH_HARMO) {
        h8wh_amp *= 1.0f - sdt * (0.02f + 0.03f * (1.0f - h8wh_shape));
        float t = h8wh_pt * 2.2f;
        fx = h8wh_amp * (0.6f * sinf(h8wh_fx1 * t + h8wh_px1) + 0.4f * sinf(h8wh_fx2 * t + h8wh_px2));
        fy = h8wh_amp * (0.6f * sinf(h8wh_fy1 * t + h8wh_py1) + 0.4f * sinf(h8wh_fy2 * t + h8wh_py2));
      } else {
        float t = h8wh_pt * 1.6f, k = (h8wh_sR - h8wh_sr) / h8wh_sr;
        float sc = 1.0f / (h8wh_sR - h8wh_sr + h8wh_sd * h8wh_sr);
        fx = sc * ((h8wh_sR - h8wh_sr) * cosf(t) + h8wh_sd * h8wh_sr * cosf(k * t));
        fy = sc * ((h8wh_sR - h8wh_sr) * sinf(t) - h8wh_sd * h8wh_sr * sinf(k * t));
        h8wh_amp = 1.0f - h8wh_pt / (40.0f + h8wh_shape * 60.0f);
      }
      int32_t x = cx + (int32_t)((fx * cr - fy * sr) * R * 1.15f), y = cy + (int32_t)((fx * sr + fy * cr) * R * 1.15f);
      int hue = (h8wh_hue + (int)(h8wh_pt * 0.6f)) & 15;
      if (h8wh_have) v6_lineAdd(h8wh_lx, h8wh_ly, x, y, 4 + (int)(h8wh_dot * 6), hue);
      h8wh_lx = x; h8wh_ly = y; h8wh_have = true;
    }
    if (h8wh_amp < 0.12f) h8wh_newPen(pat);                           // the drawing has run down: start a new one
  }
  v8_end();
}

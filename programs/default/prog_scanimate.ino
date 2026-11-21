// =====================================================================
// PROGRAM: SCANIMATE — 1970s analog title animation (by Dewey) — Set 7, slot 5
//
// Before computer graphics, the flying, bending, glowing logos on 1970s
// television — network idents, Sesame Street, disco-era titles — came from
// Scanimate: artwork filmed off a high-resolution screen whose beam was
// pushed about by ramps and oscillators. Words swung round in space,
// twisted like ribbons, rippled like flags and left long trails of colour,
// each exposure a different hue.
//
// How it works: a word is built from the 5 × 7 font, one quad per lit
// font pixel. Every corner of the grid is pushed through the "Scanimate
// patch" — stretch, bend, ripple, a twist that grows down the word, a
// swing round the vertical, a roll, a spin and a flight in depth — and
// drawn in perspective. Up to fourteen earlier exposures, each a moment
// older and a hue further round, are drawn behind it: the trail.
//
// Presets: k0 Video · k1 Analog · k2 Ghost · k3 Synth · k4 Disco · k5 1974 ·
//          k6 Signal · k7 Space · k8 TV · k9 Colour · k10 Scan · k11 Tour
//   k12 — FLY: the word rushes at you and through
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Trails · p6 Spacing (time between exposures) · p7 Bend · p8 Ripple
//   p9 Swing · p10 Word · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define SM_NW 11
static const char* const sm_words[SM_NW] = { "VIDEO", "ANALOG", "GHOST", "SYNTH", "DISCO", "1974", "SIGNAL", "SPACE", "TV", "COLOR", "SCAN" };
// swing (deg), its speed ×100, roll (deg), speed, twist (deg over the word height), bend %, ripple %, ripple waves ×10,
// depth flight (px), speed, spin speed ×100, stretch %, scroll, trails, spacing (×100 s), hue step per exposure
struct SmLook { uint8_t word; uint8_t swing, swF, roll, rollF, twist, bend, rip, ripK, zA, zF, spin, stretch, scroll, trails, gap, hstep, pal, flow, spread, scan; };
static const SmLook sm_looks[11] = {
  // word swing swF roll rlF twist bend rip ripK  zA  zF spin str scr trl gap hst pal              flow spr scan
  {  0,   60,  45,   0,   0,   0,   0,   0,  0,   0,  0,   0,  0,  0, 10,  6,  1, V7P_SCANIMATE,    3, 100, 2 },   // Video
  {  1,   15,  30,  10,  25,   0,   0,  45, 15,   0,  0,   0,  0,  0,  6,  5,  1, V7P_CHICAGO,      4,  90, 2 },   // Analog
  {  2,    0,  35,   0,   0, 180,   0,   0,  0,   0,  0,   0,  0,  0, 12,  4,  1, V7P_VASULKA,      3,  80, 2 },   // Ghost
  {  3,   20,  25,   0,   0,   0,   0,   0,  0, 250, 35,   0,  0,  0, 14,  7,  1, V7P_BLACKLIGHT,   5, 100, 2 },   // Synth
  {  4,   25,  40,  20,  55,   0,  45,   0,  0,   0,  0,   0,  0,  0, 10,  5,  2, V7P_SCANIMATE,    6, 100, 2 },   // Disco
  {  5,    0,   0,  80,  30,   0,   0,  15, 10,   0,  0,   0,  0,  0,  9,  6,  1, V7P_SUNSET,       4,  90, 2 },   // 1974
  {  6,   10,  20,   0,   0,   0,   0,   0,  0,   0,  0,   0, 70,  0,  8,  6,  1, V7P_PAIK,         5, 100, 2 },   // Signal
  {  7,   45,  30,  25,  20,   0,   0,   0,  0, 180, 25,   0,  0,  0, 12,  8,  1, V7P_LUMIA,        4,  90, 2 },   // Space
  {  8,    0,   0,   0,   0,  30,   0,   0,  0,  80, 40,  35,  0,  0, 14,  5,  1, V7P_CHICAGO,      5, 100, 2 },   // TV
  {  9,   20,  35,   0,   0,  90,  20,  35, 20,   0,  0,   0,  0,  0, 10,  5,  1, V7P_STUDIO,       6, 100, 2 },   // Colour
  { 10,    0,   0,   0,   0,   0,  25,   0,  0,   0,  0,   0,  0, 40, 10,  6,  1, V7P_PHOSPHOR,     3,  70, 2 },   // Scan
};
static const char* const sm_names[12] = { "Video", "Analog", "Ghost", "Synth", "Disco", "1974", "Signal", "Space", "TV", "Colour", "Scan", "Tour" };
static const char* const sm_lbl[6] = { "Trails", "Spacing", "Bend", "Ripple", "Swing", "Word" };
const char* prog_scanimate_name() { return "SCANIMATE"; }
const char* prog_scanimate_character() { return "1970s Scanimate titles: words swing, twist and ripple, trailing colour"; }
const char* prog_scanimate_presetName(int p) { return (p >= 0 && p < 12) ? sm_names[p] : NULL; }
const char* prog_scanimate_potLabel(int p, int pot) { (void)p; return v7_label(pot, sm_lbl); }
uint8_t prog_scanimate_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define SM_MAXC 37                                                        // grid columns: 6 letters × 6 + 1
static int16_t* sm_gx; static int16_t* sm_gy;                              // projected grid, 1/16 px
static void sm_mem() { sm_gx = (int16_t*)v6_mem; sm_gy = sm_gx + SM_MAXC * 8; }

static float sm_trails, sm_gap, sm_bend, sm_rip, sm_swing, sm_fly = -1;
void prog_scanimate_init() { sm_mem(); v6_init(); sm_fly = -1; }

// one exposure of the word at time t: project the grid, then fill the lit font pixels
static void sm_exposure(const char* word, int look, float t, float cell, uint8_t colour, float scrollY) {
  const SmLook& L = sm_looks[look];
  int n = (int)strlen(word), cols = n * 6 - 1;
  float ang = L.swing * 0.01745f * sm_swing * 2.0f * sinf(t * L.swF * 0.01f * 6.2832f);
  float roll = L.roll * 0.01745f * sinf(t * L.rollF * 0.01f * 6.2832f);
  float spin = t * L.spin * 0.01f * 6.2832f;
  float zf = L.zA ? L.zA * (0.5f - 0.5f * cosf(t * L.zF * 0.01f * 6.2832f)) - L.zA * 0.35f : 0.0f;
  if (sm_fly >= 0) zf -= sm_fly * sm_fly * 900.0f;
  float stretch = 1.0f + L.stretch * 0.01f * (0.5f + 0.5f * sinf(t * 1.7f));
  int32_t cr = (int32_t)(cosf(roll) * 16384), sr = (int32_t)(sinf(roll) * 16384);
  int32_t cs = (int32_t)(cosf(spin) * 16384), ss = (int32_t)(sinf(spin) * 16384);
  int32_t c16 = (int32_t)(cell * 16);
  int32_t bendK = (int32_t)(sm_bend * 4096), ripA = (int32_t)(sm_rip * cell * 2.4f * 16);
  int ripPh = (int)(t * 900), ripK = L.ripK * 1024 / 60;
  int twistTot = (int)(L.twist * 1024 / 360);
  int32_t halfW = (int32_t)(cols * c16 / 2);
  int32_t F = 600 * 16, Z0 = (int32_t)(zf * 16);
  int32_t sy0 = (int32_t)(scrollY * 16);
  for (int r = 0; r <= 7; r++) {
    int32_t v0 = (int32_t)((2 * r - 7) * c16 / 2);
    int twA = (int)(ang * 1024 / 6.2832f) + (twistTot * (2 * r - 7)) / 14 + (twistTot ? (int)(t * 120) : 0);
    int32_t ca = v3_cos(twA), sa = v3_sin(twA);
    for (int c = 0; c <= cols; c++) {
      int32_t u = (int32_t)((2 * c - cols) * c16 / 2);
      u = (int32_t)(u * stretch);
      int32_t v = v0;
      if (bendK) { int32_t un = (u * 256) / (halfW + 1); v -= (int32_t)(((un * un) >> 8) * (int64_t)bendK * c16 >> 20) * 3; }
      if (ripA) v += (ripA * v3_sin(c * ripK - ripPh)) >> 14;
      int32_t x = (u * ca) >> 14, z = (u * sa) >> 14;                   // swing + twist
      int32_t y = (v * cr - z * sr) >> 14; z = (v * sr + z * cr) >> 14;  // roll
      int32_t x2 = (x * cs - y * ss) >> 14, y2 = (x * ss + y * cs) >> 14; // spin
      int32_t den = F + z + Z0; if (den < F / 8) den = F / 8;
      int32_t s = (F << 8) / den;                                           // ×256
      int k = r * SM_MAXC + c;
      sm_gx[k] = (int16_t)v3_clamp(HALFW * 16 + ((x2 * s) >> 8), -8000, 13000);
      sm_gy[k] = (int16_t)v3_clamp(HALFH * 16 + ((y2 * s) >> 8) + sy0, -8000, 12000);
    }
  }
  for (int i = 0; i < n; i++) {
    const uint8_t* g = v3_font[v3_glyph(word[i])];
    for (int r = 0; r < 7; r++) for (int b = 0; b < 5; b++) if (g[r] & (16 >> b)) {
      int c = i * 6 + b, k = r * SM_MAXC + c;
      int32_t xs[4] = { sm_gx[k], sm_gx[k + 1], sm_gx[k + 1 + SM_MAXC], sm_gx[k + SM_MAXC] };
      int32_t ys[4] = { sm_gy[k], sm_gy[k + 1], sm_gy[k + 1 + SM_MAXC], sm_gy[k + SM_MAXC] };
      v6_fill(xs, ys, 4, colour);
    }
  }
}

void prog_scanimate_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  sm_mem();
  const SmLook& L = sm_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&sm_trails, v3_centre(5) ? (float)L.trails : v3_u(5) * 14.0f, rate);
  v6_ease(&sm_gap, v3_centre(6) ? L.gap / 100.0f : 0.01f + v3_u(6) * 0.2f, rate);
  v6_ease(&sm_bend, v3_centre(7) ? L.bend / 100.0f : v3_bi(7), rate);
  v6_ease(&sm_rip, v3_centre(8) ? L.rip / 100.0f : v3_u(8), rate);
  v6_ease(&sm_swing, v3_centre(9) ? 0.5f : v3_u(9), rate);
  int w = v3_centre(10) ? L.word : v3_clamp(v3_k(10) * SM_NW / 1024, 0, SM_NW - 1);
  const char* word = sm_words[w];
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 140);

  float t = v6_t;
  if (v6_k12hit) sm_fly = 0;
  if (sm_fly >= 0) { sm_fly += v6_f.dt * 0.9f; if (sm_fly > 1.0f) sm_fly = -1; }
  v7_decay(64);                                                          // a short glow behind the moving word

  int cols = (int)strlen(word) * 6 - 1;
  float cell = v3_clampf(270.0f / cols, 4.0f, 22.0f) * v6_zoom(1.0f);
  int nt = (int)(sm_trails + 0.5f);
  int baseHue = ((int)(t * 1.5f)) & 15;
  for (int e = nt; e >= 0; e--) {                                         // oldest first, the live word last
    float te = t - e * sm_gap;
    int lev = e == 0 ? 15 : v3_clamp(13 - (e * 9) / (nt + 1), 3, 13);
    int hue = (baseHue + e * L.hstep) & 15;
    float sc = L.scroll ? fmodf(-e * L.scroll * 0.6f, (float)H) : 0.0f;
    sm_exposure(word, look, te, cell, V6C(lev, hue), sc);
  }
  v7_end(L.scan);
}

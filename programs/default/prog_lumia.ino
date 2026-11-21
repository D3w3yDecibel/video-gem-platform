// =====================================================================
// PROGRAM: LUMIA — Thomas Wilfred's Clavilux (by Dewey) — Set 7, slot 8
//
// From the 1920s Thomas Wilfred built the Clavilux, an instrument for
// playing light: lamps, coloured glass, turning reflectors and folded
// fabric threw slow veils, rays and clouds of colour onto a screen in a
// dark room. He called the art Lumia. His "Lumia Suite, Op. 158" ran at
// the Museum of Modern Art for years — a few forms of light drifting,
// folding and dissolving, never repeating. It is the ancestor of the
// liquid light show.
//
// How it works: the light is worked out on a coarse grid (80 × 60): each
// form — a veil (a soft, waving band), a ray from a hidden lamp, a glowing
// cloud, a flame — adds its light and colour to the cells it touches. The
// grid is then spread smoothly over the screen, blending between cells,
// with a fine dither so the gradients stay soft.
//
// Presets: k0 Opus · k1 Aurora · k2 Nebula · k3 Rays · k4 Ribbons · k5 Clavilux ·
//          k6 Tide · k7 Flame · k8 Smoke · k9 Dawn · k10 Prism · k11 Tour
//   k12 — BLOOM: a new cloud of light swells up in the middle
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Forms · p6 Softness · p7 Drift · p8 Size · p9 Grain · p10 Glow
//   p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define LU_GW 80
#define LU_GH 60
enum { LU_VEIL, LU_RAY, LU_ORB, LU_FLAME };
#define LU_MAXF 8
struct LuLook { uint8_t types[LU_MAXF]; uint8_t n, soft, drift, size, grain, glow, pal, flow, spread, scan; };
static const LuLook lu_looks[11] = {
  // forms (V veil, R ray, O orb, F flame)                                         n  soft drift size grain glow pal              flow spr scan
  { {LU_VEIL, LU_VEIL, LU_ORB, LU_VEIL, LU_ORB, LU_VEIL, LU_RAY, LU_ORB},           4,  60,  30,  60,  30,  60, V7P_LUMIA,        3,  90, 0 },   // Opus
  { {LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME},5, 45,  35,  55,  30,  55, V7P_PHOSPHOR,     4,  80, 0 },   // Aurora
  { {LU_ORB, LU_ORB, LU_ORB, LU_ORB, LU_ORB, LU_ORB, LU_ORB, LU_ORB},               6,  70,  25,  60,  40,  55, V7P_BLACKLIGHT,   4, 100, 0 },   // Nebula
  { {LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY},               6,  40,  30,  70,  30,  60, V7P_SUNSET,       4,  90, 0 },   // Rays
  { {LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL},       6,  25,  35,  50,  25,  60, V7P_VASULKA,      5, 100, 0 },   // Ribbons
  { {LU_VEIL, LU_RAY, LU_ORB, LU_VEIL, LU_RAY, LU_ORB, LU_VEIL, LU_RAY},            6,  55,  30,  60,  30,  60, V7P_LUMIA,        4, 100, 0 },   // Clavilux
  { {LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL, LU_VEIL},       4,  70,  15,  80,  30,  55, V7P_VASULKA,      2,  60, 0 },   // Tide
  { {LU_FLAME, LU_FLAME, LU_ORB, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME, LU_FLAME}, 5,  45,  50,  55,  35,  65, V7P_AMBER,        5,  80, 0 },   // Flame
  { {LU_FLAME, LU_VEIL, LU_FLAME, LU_VEIL, LU_FLAME, LU_VEIL, LU_FLAME, LU_VEIL},   6,  75,  25,  60,  50,  45, V7P_GHOST,        2,  50, 0 },   // Smoke
  { {LU_VEIL, LU_ORB, LU_VEIL, LU_VEIL, LU_ORB, LU_VEIL, LU_VEIL, LU_VEIL},         3,  80,  15,  85,  25,  60, V7P_SUNSET,       2,  70, 0 },   // Dawn
  { {LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY, LU_RAY},               7,  30,  25,  75,  20,  65, V7P_CHICAGO,      4, 110, 0 },   // Prism
};
static const char* const lu_names[12] = { "Opus", "Aurora", "Nebula", "Rays", "Ribbons", "Clavilux", "Tide", "Flame", "Smoke", "Dawn", "Prism", "Tour" };
static const char* const lu_lbl[6] = { "Forms", "Softness", "Drift", "Size", "Grain", "Glow" };
const char* prog_lumia_name() { return "LUMIA"; }
const char* prog_lumia_character() { return "Thomas Wilfred's Clavilux: slow veils, rays and clouds of coloured light"; }
const char* prog_lumia_presetName(int p) { return (p >= 0 && p < 12) ? lu_names[p] : NULL; }
const char* prog_lumia_potLabel(int p, int pot) { (void)p; return v7_label(pot, lu_lbl); }
uint8_t prog_lumia_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: light (×16, up to 65535) and hue×light per cell; the interpolated row
static uint16_t *lu_L, *lu_HL; static uint8_t *lu_lev, *lu_hue;
// (level and hue are written over the light tables, byte k over word k: in order, nothing is lost)
static void lu_mem() { lu_L = (uint16_t*)v6_mem; lu_HL = lu_L + LU_GW * LU_GH; lu_lev = (uint8_t*)lu_L; lu_hue = (uint8_t*)lu_HL; }

static float lu_n, lu_soft, lu_drift, lu_size, lu_grain, lu_glow, lu_t = 0, lu_bloom = -1;
static uint32_t lu_seed = 1;
void prog_lumia_init() { lu_mem(); v6_init(); lu_bloom = -1; lu_seed = 1 + v3_rnd(1000); }

static const uint8_t lu_bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

// add one form's light to the grid. Cell coordinates in 1/16 cells; a slow clock `t` per form.
static void lu_form(int type, int idx, float t, float size, float soft, int hue, int peak) {
  uint32_t h = v3_hash(lu_seed * 977u + idx * 7919u);
  float f1 = 0.07f + (h & 255) / 2000.0f, f2 = 0.05f + ((h >> 8) & 255) / 2500.0f, f3 = 0.11f + ((h >> 16) & 255) / 1800.0f;
  float p1 = (h & 1023) / 160.0f, p2 = ((h >> 10) & 1023) / 160.0f;
  int32_t cx16 = (int32_t)((LU_GW * 0.5f + LU_GW * 0.32f * sinf(t * f1 + p1)) * 16);
  int32_t cy16 = (int32_t)((LU_GH * 0.5f + LU_GH * 0.30f * sinf(t * f2 + p2)) * 16);
  uint16_t* Lp = lu_L; uint16_t* Hp = lu_HL;
  if (type == LU_ORB) {
    int32_t R = (int32_t)((6.0f + size * 22.0f) * (0.75f + 0.25f * sinf(t * f3 + p2)) * 16);
    int32_t R2 = (R * R) >> 8;                                            // in (1/16 cell)² / 256
    int x0 = v3_clamp((cx16 - R) >> 4, 0, LU_GW - 1), x1 = v3_clamp((cx16 + R) >> 4, 0, LU_GW - 1);
    int y0 = v3_clamp((cy16 - R) >> 4, 0, LU_GH - 1), y1 = v3_clamp((cy16 + R) >> 4, 0, LU_GH - 1);
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
      int32_t dx = x * 16 + 8 - cx16, dy = y * 16 + 8 - cy16;
      int32_t d2 = (dx * dx + dy * dy) >> 8;
      if (d2 >= R2) continue;
      int32_t q = 256 - d2 * 256 / R2;                                    // 0 … 256
      int32_t I = (peak * ((q * q) >> 8)) >> 8;
      int k = y * LU_GW + x;
      Lp[k] = (uint16_t)v3_clamp(Lp[k] + I, 0, 65535); Hp[k] = (uint16_t)v3_clamp(Hp[k] + ((I * hue) >> 4), 0, 65535);
    }
    return;
  }
  // veils, rays and flames: a (waving) band across the grid at an angle
  float ang;
  if (type == LU_FLAME) ang = 1.5708f + 0.25f * sinf(t * f1 * 0.7f + p1);
  else if (type == LU_RAY) ang = 1.5708f + 1.1f * sinf(t * f2 * 0.8f + p1 + idx);
  else ang = 0.35f * sinf(t * f3 * 0.5f + p2) + (idx & 1 ? 0.3f : -0.2f);
  int32_t ca = (int32_t)(cosf(ang) * 16384), sa = (int32_t)(sinf(ang) * 16384);
  if (type == LU_RAY) { cx16 = (int32_t)(LU_GW * 0.5f * 16 + 40 * 16 * sinf(t * f1 * 0.3f + p2)); cy16 = (LU_GH + 6) * 16; }   // a lamp below the screen
  int32_t w16 = (int32_t)((2.0f + soft * 13.0f) * (0.6f + size) * 16);   // half-width of the band
  if (type == LU_RAY) w16 = w16 * 3 / 4 + 8;
  int32_t amp = type == LU_RAY ? 0 : (int32_t)((2.0f + size * 10.0f) * 16 * (type == LU_FLAME ? 0.6f : 1.0f));
  int kA = (int)(1024.0f * (type == LU_FLAME ? 2.4f : 1.0f) / LU_GW);      // waves along the band, per cell
  int ph = (int)(t * (type == LU_FLAME ? 900.0f : 260.0f) * (0.6f + f2 * 4)) + idx * 300;
  int32_t inv = 65536 / (w16 + 1);
  for (int y = 0; y < LU_GH; y++) {
    int32_t dy = y * 16 + 8 - cy16;
    for (int x = 0; x < LU_GW; x++) {
      int32_t dx = x * 16 + 8 - cx16;
      int32_t u = (dx * ca + dy * sa) >> 14, v = (-dx * sa + dy * ca) >> 14;   // along, across (1/16 cells)
      if (amp) v -= (amp * v3_sin((int)((u * kA) >> 4) - ph)) >> 14;
      int32_t av = v < 0 ? -v : v;
      if (av >= w16) continue;
      int32_t q = 256 - ((av * inv) >> 8);                                   // 0 … 256 across the band
      int32_t I = (peak * ((q * q) >> 8)) >> 8;
      if (type == LU_RAY) { if (u > 0) continue; I = I * v3_clamp(256 + (int)(u / 6), 0, 256) >> 8; }   // fades with distance from the lamp
      if (type == LU_FLAME) { int32_t up = (y * 256) / LU_GH; I = (I * (40 + up)) >> 8; }              // flames brighter at the bottom
      int k = y * LU_GW + x;
      Lp[k] = (uint16_t)v3_clamp(Lp[k] + I, 0, 65535); Hp[k] = (uint16_t)v3_clamp(Hp[k] + ((I * hue) >> 4), 0, 65535);
    }
  }
}

// the grid spread over the screen: bilinear between cell centres, dithered (levels ≤ 226, so no clamping is needed)
__attribute__((optimize("O2"))) static void lu_upscale(int grainAmt) {
  uint8_t rl[LU_GW + 1], rh[LU_GW + 1];
  uint32_t g = (uint32_t)v7_frameNo * 2654435761u + 1;
  for (int y = 0; y < H; y++) {
    int32_t gy = (y * 16 - 24);                                              // 1/64 cell; cell centres at 4j + 1.5 px
    int j = gy >> 6, fy = (gy & 63) << 2;                                    // fy 0 … 252
    if (gy < 0) { j = 0; fy = 0; }
    int j1 = j + 1 < LU_GH ? j + 1 : j;
    const uint8_t *a = lu_lev + j * LU_GW, *b = lu_lev + j1 * LU_GW, *ha = lu_hue + j * LU_GW, *hb = lu_hue + j1 * LU_GW;
    for (int i = 0; i < LU_GW; i++) { rl[i] = (uint8_t)(a[i] + (((b[i] - a[i]) * fy) >> 8)); rh[i] = (uint8_t)(ha[i] + (((hb[i] - ha[i]) * fy) >> 8)); }
    rl[LU_GW] = rl[LU_GW - 1]; rh[LU_GW] = rh[LU_GW - 1];
    uint32_t* out = (uint32_t*)(v3_buf + y * W);
    const uint8_t* bay = lu_bayer + ((y & 3) << 2);
    int d0 = bay[0] * 4, d1 = bay[1] * 4, d2 = bay[2] * 4, d3 = bay[3] * 4;   // the dither, ×4
    for (int i = 0; i < LU_GW; i++) {
      int l = rl[i] << 2, dl = rl[i + 1] - rl[i], h = rh[i] << 2, dh = rh[i + 1] - rh[i];
      int gr = 0;
      if (grainAmt) { g = g * 1664525u + 1013904223u; gr = (int)((g >> 24) * grainAmt) >> 8; }   // film grain: a little extra light, per 4 px
      l += gr;
      uint32_t p0 = (uint32_t)((((l + d0) >> 6) << 4) | (((h + d0) >> 6) & 15));
      uint32_t p1 = (uint32_t)((((l + dl + d1) >> 6) << 4) | (((h + dh + d1) >> 6) & 15));
      uint32_t p2 = (uint32_t)((((l + 2 * dl + d2) >> 6) << 4) | (((h + 2 * dh + d2) >> 6) & 15));
      uint32_t p3 = (uint32_t)((((l + 3 * dl + d3) >> 6) << 4) | (((h + 3 * dh + d3) >> 6) & 15));
      out[i] = p0 | (p1 << 8) | (p2 << 16) | (p3 << 24);
    }
  }
}

void prog_lumia_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  lu_mem();
  const LuLook& L = lu_looks[look];
  float rate = v6_fade < 1.0f ? 1.0f : 3.0f;
  v6_ease(&lu_n, v3_centre(5) ? (float)L.n : 1.0f + v3_u(5) * (LU_MAXF - 1), rate);
  v6_ease(&lu_soft, v7_kn(6, L.soft / 100.0f), rate);
  v6_ease(&lu_drift, v7_kn(7, L.drift / 100.0f), rate);
  v6_ease(&lu_size, v7_kn(8, L.size / 100.0f), rate);
  v6_ease(&lu_grain, v7_kn(9, L.grain / 100.0f), rate);
  v6_ease(&lu_glow, v7_kn(10, L.glow / 100.0f), rate);
  if (v6_f.fresh) lu_seed = 1 + v3_rnd(100000);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 60);

  lu_t += v6_f.sdt * (0.2f + lu_drift * 2.5f);
  if (lu_t > 20000.0f) lu_t -= 20000.0f;
  memset(lu_L, 0, LU_GW * LU_GH * 4);
  float z = v6_zoom(1.0f);
  int nf = (int)(lu_n + 0.5f); nf = v3_clamp(nf, 1, LU_MAXF);
  int peak = (int)(1200 + lu_glow * 2600);
  for (int f = 0; f < nf; f++) {
    int hue = (f * 3 + 2) & 15;
    lu_form(L.types[f], f, lu_t + f * 37.0f, lu_size * z, lu_soft, hue, peak);
  }
  if (v6_k12hit) lu_bloom = 0;
  if (lu_bloom >= 0) {
    lu_bloom += v6_f.dt * 0.5f;
    float b = lu_bloom < 0.3f ? lu_bloom / 0.3f : 1.0f - (lu_bloom - 0.3f) / 0.7f;
    if (lu_bloom >= 1.0f) lu_bloom = -1;
    else {                                                                  // a cloud at the middle, growing as it brightens
      uint32_t sv = lu_seed; lu_seed = 4242;
      lu_form(LU_ORB, 0, 0.0f, (0.3f + lu_bloom * 1.4f) * z, 0.5f, 9, (int)(b * 3500));
      lu_seed = sv;
    }
  }
  // light → level (soft knee, so crossings glow but don't flatten), hue = the light-weighted mix
  for (int k = 0; k < LU_GW * LU_GH; k++) {
    uint32_t l = lu_L[k];
    int lev = (int)((l * 226) / (l + 900));                                 // 0 … 226 (room for dither and grain)
    lu_lev[k] = (uint8_t)lev;
    lu_hue[k] = l ? (uint8_t)v3_clamp((int)(((uint32_t)lu_HL[k] << 8) / l), 0, 255) : 0;
  }
  lu_upscale((int)(lu_grain * 40));
  v7_end(L.scan);
}

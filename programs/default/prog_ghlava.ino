// =====================================================================
// PROGRAM: LAVA — lava lamp & liquid light (by Dewey) — Set 8 slot 0
//
// Greatest hit from Set 1 (Liquid Light) and Set 3 (LIQUID): warm blobs of
// wax rising and sinking in a lamp, or coloured oils pressed between glass
// on an overhead projector. Remixed slow and smooth for mixing.
//
// How it works: up to 14 blobs, each a soft round "metaball". Their fields
// add up on the 80 × 60 light grid, row by row; where the sum passes a level
// the blobs merge into one glossy shape with a bright rim. Oil mode skips
// the edge and lets the colours pool and blend instead; Contour mode draws
// the field as rings, like a topographic map of the goo.
//
// Presets: k0 Lava Lamp · k1 Fillmore · k2 Groovy · k3 Contours · k4 Deep Sea ·
//          k5 Peach Melba · k6 Blob Party · k7 Velvet · k8 Forest Floor ·
//          k9 Aurora Goo · k10 Ember · k11 Tour
//   k12 — PLOP: a big new blob rises from the bottom and joins in
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Blobs · p6 Size · p7 Gooey · p8 Rim · p9 Glow · p10 Rise
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8LA_LAMP, H8LA_OIL, H8LA_CONTOUR };
struct H8LaLook { uint8_t mode, n, size, gooey, rim, glow, rise, pal, flow, spread; int8_t key; };
static const H8LaLook h8la_looks[11] = {
  // mode          n  size gooey rim glow rise pal          flow spr key
  { H8LA_LAMP,     7,  55,  50,  60,  55,  50, V8P_LAVA,     2,  45,  0 },   // Lava Lamp
  { H8LA_OIL,     10,  55,  50,  30,  50,  30, V8P_LOUNGE,   3,  90,  0 },   // Fillmore
  { H8LA_LAMP,     9,  45,  60,  70,  45,  55, V8P_NEON,    3,  80,  0 },   // Groovy
  { H8LA_CONTOUR,  6,  60,  50,  50,  40,  40, V8P_SUNSET,   3,  70, 30 },   // Contours
  { H8LA_LAMP,     6,  60,  45,  50,  65,  30, V8P_LAGOON,   2,  50,  0 },   // Deep Sea
  { H8LA_OIL,      8,  60,  50,  30,  40,  35, V8P_TROPIC,    2,  60,  0 },   // Peach Melba
  { H8LA_LAMP,    13,  35,  65,  65,  45,  60, V8P_SHERBET,  4, 100,  0 },   // Blob Party
  { H8LA_LAMP,     5,  75,  45,  55,  60,  35, V8P_VELVET,   2,  40,  0 },   // Velvet
  { H8LA_OIL,      9,  50,  50,  30,  45,  30, V8P_FIREICE,   2,  70,  0 },   // Forest Floor
  { H8LA_CONTOUR,  8,  50,  50,  50,  45,  45, V8P_AURORA,   3,  90, 30 },   // Aurora Goo
  { H8LA_LAMP,     6,  55,  50,  60,  70,  40, V8P_EMBER,    2,  40,  0 },   // Ember
};
static const char* const h8la_names[12] = { "Lava Lamp", "Fillmore", "Groovy", "Contours", "Deep Sea", "Peach Melba", "Blob Party", "Velvet", "Forest Floor", "Aurora Goo", "Ember", "Tour" };
static const char* const h8la_lbl[6] = { "Blobs", "Size", "Gooey", "Rim", "Glow", "Rise" };
const char* prog_ghlava_name() { return "LAVA"; }
const char* prog_ghlava_character() { return "Lava lamp and liquid light: slow glossy blobs that merge, rise and sink"; }
const char* prog_ghlava_presetName(int p) { return (p >= 0 && p < 12) ? h8la_names[p] : NULL; }
const char* prog_ghlava_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8la_lbl); }
uint8_t prog_ghlava_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8LA_MAX 15
static float h8la_n = 7, h8la_size = 0.5f, h8la_gooey = 0.5f, h8la_rim = 0.5f, h8la_glow = 0.5f, h8la_rise = 0.5f;
static float h8la_t = 0, h8la_plop = -1;
static uint32_t h8la_seed = 7;
// per frame: blob centre and radius in 1/16 cells, its hue
static int32_t h8la_bx[H8LA_MAX], h8la_by[H8LA_MAX], h8la_br[H8LA_MAX], h8la_inv[H8LA_MAX];
static uint8_t h8la_bh[H8LA_MAX];
void prog_ghlava_init() { v8_init(); h8la_plop = -1; h8la_seed = 1 + v3_rnd(100000); }

void prog_ghlava_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8LaLook& L = h8la_looks[look];
  float rate = v8_rate();
  v6_ease(&h8la_n, v3_centre(5) ? (float)L.n : 2.0f + v3_u(5) * 12.0f, rate);
  v6_ease(&h8la_size, v8_kn(6, L.size / 100.0f), rate);
  v6_ease(&h8la_gooey, v8_kn(7, L.gooey / 100.0f), rate);
  v6_ease(&h8la_rim, v8_kn(8, L.rim / 100.0f), rate);
  v6_ease(&h8la_glow, v8_kn(9, L.glow / 100.0f), rate);
  v6_ease(&h8la_rise, v8_kn(10, L.rise / 100.0f), rate);
  if (v6_f.fresh) h8la_seed = 1 + v3_rnd(100000);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 70, L.key / 100.0f);

  h8la_t += v6_f.sdt;
  if (h8la_t > 30000.0f) h8la_t -= 30000.0f;
  float z = v8_zoom(1.0f);
  int n = v3_clamp((int)(h8la_n + 0.5f), 1, H8LA_MAX - 1);
  float t = h8la_t;
  // blobs: lamp mode rises and sinks, oil mode drifts all over
  for (int b = 0; b < n; b++) {
    uint32_t h = v3_hash(h8la_seed * 131u + b * 977u);
    float x0 = 0.12f + (h & 1023) / 1023.0f * 0.76f, p1 = ((h >> 10) & 1023) / 163.0f, p2 = ((h >> 20) & 1023) / 163.0f;
    float rt = 0.6f + ((h >> 5) & 255) / 400.0f;
    float sz = 0.55f + ((h >> 13) & 255) / 400.0f;
    float x, y;
    if (L.mode == H8LA_OIL) {
      x = 0.5f + 0.42f * sinf(t * 0.05f * rt + p1) * cosf(t * 0.021f + p2);
      y = 0.5f + 0.40f * sinf(t * 0.043f * rt + p2);
    } else {
      x = x0 + 0.07f * sinf(t * 0.07f * rt + p1);
      y = 0.52f + 0.46f * sinf(t * (0.015f + 0.3f * h8la_rise * h8la_rise) * rt + p2);
    }
    float r = (3.0f + h8la_size * 16.0f) * sz * z * (0.85f + 0.15f * sinf(t * 0.11f * rt + p1));
    h8la_bx[b] = (int32_t)(x * V8_GW * 16); h8la_by[b] = (int32_t)(y * V8_GH * 16);
    h8la_br[b] = (int32_t)(r * 16 / 0.67f);
    h8la_bh[b] = (uint8_t)(((b * 3 + 1) & 15) * 16 + 8);         // neighbouring hues, so mixed oils stay in the family
  }
  if (v6_k12hit) h8la_plop = 0;
  if (h8la_plop >= 0) {
    h8la_plop += v6_f.dt * 0.12f;
    if (h8la_plop > 1.0f) h8la_plop = -1;
    else {
      float y = 1.25f - h8la_plop * 1.0f, r = (6.0f + 10.0f * h8la_size) * z * (h8la_plop < 0.2f ? h8la_plop * 5 : 1.0f);
      h8la_bx[n] = V8_GW * 8; h8la_by[n] = (int32_t)(y * V8_GH * 16); h8la_br[n] = (int32_t)(r * 16 / 0.67f); h8la_bh[n] = 9 * 16 + 8;
      n++;
    }
  }
  for (int b = 0; b < n; b++) {
    if (h8la_br[b] < 16) h8la_br[b] = 16;
    int32_t R2 = h8la_br[b] * h8la_br[b];
    h8la_inv[b] = (int32_t)((256LL << 16) / R2);
  }

  int T = 60 + (int)(h8la_gooey * 110);                        // the edge: field level where blobs begin (Q8)
  int rimW = 10 + (int)(h8la_rim * 80), rimL = 10 + (int)(h8la_rim * 110);
  int halo = 10 + (int)(h8la_glow * 150), lamp = (int)(h8la_glow * 120);
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int32_t fld[V8_GW], HF[V8_GW];
  for (int j = 0; j < V8_GH; j++) {
    for (int i = 0; i < V8_GW; i++) { fld[i] = 0; HF[i] = 0; }
    int32_t cy = j * 16 + 8;
    for (int b = 0; b < n; b++) {
      int32_t dy = cy - h8la_by[b], R = h8la_br[b];
      if (dy <= -R || dy >= R) continue;
      int i0 = v3_clamp((h8la_bx[b] - R) >> 4, 0, V8_GW - 1), i1 = v3_clamp((h8la_bx[b] + R) >> 4, 0, V8_GW - 1);
      int32_t R2 = R * R, dy2 = dy * dy, inv = h8la_inv[b]; int hb = h8la_bh[b];
      for (int i = i0; i <= i1; i++) {
        int32_t dx = i * 16 + 8 - h8la_bx[b], d2 = dx * dx + dy2;
        if (d2 >= R2) continue;
        int32_t q = 256 - ((d2 * inv) >> 16);                   // 0 … 256 (d2·inv < 2^24, no overflow)
        q = (q * q) >> 8;
        fld[i] += q; HF[i] += q * hb;
      }
    }
    int bg = (lamp * j * j) / (V8_GH * V8_GH);                  // the lamp's light from below
    for (int i = 0; i < V8_GW; i++) {
      int k = j * V8_GW + i;
      int32_t f = fld[i];
      int hue = f > 0 ? (int)(HF[i] / f) : 2 * 16;
      int lev;
      if (L.mode == H8LA_OIL) {
        lev = (int)((f * 226) / (f + 150)) + (bg >> 1);
      } else if (L.mode == H8LA_CONTOUR) {
        int ring = (int)((f * 3) & 255);                         // rings of the field
        int tri = ring < 128 ? ring : 255 - ring;
        lev = f < T / 3 ? (int)(f * 60 / (T / 3 + 1)) : 40 + ((tri * (110 + rimL)) >> 7);
        if (f > T * 3) lev += 30;
        hue = (hue + (int)(f >> 3)) & 255;
      } else {
        if (f < T) lev = bg + (int)((f * halo) / T);
        else {
          int e = (int)(f - T);
          lev = e < rimW ? 150 + ((rimW - e) * rimL) / rimW : 150 + v6_min((e - rimW) / 3, 40);
        }
      }
      LEV[k] = (uint8_t)v3_clamp(lev, 0, 226);
      HUE[k] = (uint8_t)hue;
    }
  }
  v8_upscale(10);
  v8_end();
}

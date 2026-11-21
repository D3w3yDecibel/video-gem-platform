// =====================================================================
// PROGRAM: QUASI — quasicrystal interference (by Dewey) — Set 8 slot 2
//
// Greatest hit from Set 6 (QUASI) and Set 3 (MOIRE): a handful of plane
// waves crossing at even angles add up to a pattern that is ordered but
// never repeats — five waves give a Penrose-like ten-fold flower field,
// three give soft honeycombs. Here the waves are long and slow, so the
// pattern breathes and turns like a lava-lit mandala.
//
// How it works: on the 80 × 60 grid each cell adds up n cosine waves (one
// table lookup each, phases stepped by addition across the row). The sum
// becomes light three ways: SOFT (a smooth gradient of the sum), BANDS
// (gentle contour stripes) or GLOW (only the peaks shine, on black).
//
// Presets: k0 Quasicrystal · k1 Penrose Glow · k2 Moire Silk · k3 Mandala Field ·
//          k4 Ripple Tank · k5 Starlight · k6 Op Bands · k7 Honeycomb · k8 Octagon ·
//          k9 Dodeca · k10 Ember Grid · k11 Tour
//   k12 — SHIFT: the waves slide to new places
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (wavelength) · p3 Colour Flow · p4 Hue Spread
//   p5 Waves · p6 Style (soft/bands/glow) · p7 Bands · p8 Turn · p9 Morph · p10 Contrast
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8QU_SOFT, H8QU_BANDS, H8QU_GLOW };
struct H8QuLook { uint8_t n, style, wl, bands, turn, morph, contrast, pal, flow, spread; int8_t key; };
static const H8QuLook h8qu_looks[11] = {
  // n style       wl bands turn morph contr pal          flow spr key
  { 7, H8QU_SOFT,  14,  40,  30,  40,  60, V8P_LOUNGE,   3,  90,  0 },   // Quasicrystal
  { 5, H8QU_GLOW,  12,  40,  30,  40,  60, V8P_AURORA,   3,  80,  0 },   // Penrose Glow
  { 3, H8QU_BANDS, 18,  30,  20,  30,  50, V8P_ACID,    2,  60,  0 },   // Moire Silk
  { 9, H8QU_SOFT,  16,  40,  25,  35,  70, V8P_SUNSET,   3, 100,  0 },   // Mandala Field
  { 4, H8QU_BANDS, 20,  45,  15,  50,  50, V8P_LAGOON,   2,  60,  0 },   // Ripple Tank
  { 7, H8QU_GLOW,  10,  40,  35,  45,  70, V8P_RAINBOW,    3,  90,  0 },   // Starlight
  { 5, H8QU_BANDS, 14,  60,  25,  40,  60, V8P_DUSK,     2,  80,  0 },   // Op Bands
  { 3, H8QU_SOFT,  10,  40,  20,  35,  70, V8P_FOREST,   2,  70,  0 },   // Honeycomb
  { 4, H8QU_GLOW,  13,  40,  30,  40,  60, V8P_CANDY,   2,  60,  0 },   // Octagon
  { 6, H8QU_SOFT,  12,  40,  30,  40,  60, V8P_SHERBET,  3, 100,  0 },   // Dodeca
  { 4, H8QU_BANDS, 16,  50,  20,  35,  60, V8P_EMBER,    2,  50,  0 },   // Ember Grid
};
static const char* const h8qu_names[12] = { "Quasicrystal", "Penrose Glow", "Moire Silk", "Mandala Field", "Ripple Tank", "Starlight", "Op Bands", "Honeycomb", "Octagon", "Dodeca", "Ember Grid", "Tour" };
static const char* const h8qu_lbl[6] = { "Waves", "Style", "Bands", "Turn", "Morph", "Contrast" };
const char* prog_ghquasi_name() { return "QUASI"; }
const char* prog_ghquasi_character() { return "Quasicrystal waves: plane waves crossing into a slow, never-repeating flower field"; }
const char* prog_ghquasi_presetName(int p) { return (p >= 0 && p < 12) ? h8qu_names[p] : NULL; }
const char* prog_ghquasi_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8qu_lbl); }
uint8_t prog_ghquasi_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8QU_MAX 13
static float h8qu_n = 5, h8qu_wl = 14, h8qu_bands = 0.4f, h8qu_turn = 0.3f, h8qu_morph = 0.4f, h8qu_con = 0.6f;
static float h8qu_rot = 0, h8qu_ph = 0;
static float h8qu_off[H8QU_MAX], h8qu_offT[H8QU_MAX];
void prog_ghquasi_init() { v8_init(); for (int w = 0; w < H8QU_MAX; w++) { h8qu_off[w] = h8qu_offT[w] = 0; } }

void prog_ghquasi_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8QuLook& L = h8qu_looks[look];
  float rate = v8_rate();
  v6_ease(&h8qu_n, v3_centre(5) ? (float)L.n : 2.0f + v3_u(5) * 11.0f, rate);
  v6_ease(&h8qu_wl, (float)L.wl, rate);
  v6_ease(&h8qu_bands, v8_kn(7, L.bands / 100.0f), rate);
  v6_ease(&h8qu_turn, v8_kn(8, L.turn / 100.0f), rate);
  v6_ease(&h8qu_morph, v8_kn(9, L.morph / 100.0f), rate);
  v6_ease(&h8qu_con, v8_kn(10, L.contrast / 100.0f), rate);
  int style = v3_pick(6, L.style, 3);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 80, L.key / 100.0f);
  if (v6_k12hit || v6_f.fresh) for (int w = 0; w < H8QU_MAX; w++) h8qu_offT[w] = h8qu_off[w] + (v3_rndf() - 0.5f) * 2048.0f;
  for (int w = 0; w < H8QU_MAX; w++) v6_ease(&h8qu_off[w], h8qu_offT[w], 0.6f);

  h8qu_rot += v6_f.sdt * (h8qu_turn - 0.15f) * (h8qu_turn - 0.15f) * (h8qu_turn > 0.15f ? 0.7f : -0.7f);
  h8qu_ph += v6_f.sdt * (0.03f + h8qu_morph * h8qu_morph * 4.0f);
  if (h8qu_rot > 6.2832f) h8qu_rot -= 6.2832f; if (h8qu_rot < 0) h8qu_rot += 6.2832f;
  if (h8qu_ph > 1000.0f) h8qu_ph -= 1000.0f;
  int n = v3_clamp((int)(h8qu_n + 0.5f), 2, H8QU_MAX);
  float wl = h8qu_wl * v8_zoom(1.0f);
  if (wl < 4.0f) wl = 4.0f;
  // per wave: angle step per cell across and down (1024 per turn, ×256), and the phase at the top-left cell
  int32_t kx[H8QU_MAX], ky[H8QU_MAX], p0[H8QU_MAX];
  float k = 1024.0f / wl;
  for (int w = 0; w < n; w++) {
    float th = 3.14159f * w / n + h8qu_rot;
    kx[w] = (int32_t)(cosf(th) * k * 256); ky[w] = (int32_t)(sinf(th) * k * 256);
    float ph = h8qu_ph * 160.0f * ((w & 1) ? 1.0f : 0.8f) + h8qu_off[w];
    p0[w] = (int32_t)(ph * 256) - 40 * kx[w] - 30 * ky[w];
  }
  int bands = 1 + (int)(h8qu_bands * 12), con = 40 + (int)(h8qu_con * 260);
  int hueMul = 40 + (int)(h8qu_con * 80);
  static float hueT = 0; hueT += v6_f.sdt * 18.0f; if (hueT > 2560.0f) hueT -= 2560.0f;
  int hT = (int)hueT;
  int g8 = (int)(256.0f * 1.7f / sqrtf((float)n));                  // the sum of n waves rarely reaches n: rescale
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int32_t acc[H8QU_MAX];
  for (int j = 0; j < V8_GH; j++) {
    for (int w = 0; w < n; w++) acc[w] = p0[w] + j * ky[w];
    for (int i = 0; i < V8_GW; i++) {
      int32_t s = 0, s2 = 0;
      for (int w = 0; w < n; w++) { int32_t c = v3_cos((int)(acc[w] >> 8)); s += c; s2 += (w & 1) ? -c : c; acc[w] += kx[w]; }
      // colour from a SECOND mix of the same waves (alternate ones flipped): it changes where the light doesn't,
      // so neighbouring petals and cells take clearly different colours (v3: more colour differentials)
      int32_t u2 = v3_clamp((s2 * g8) >> 8, -16384, 16384);
      int cdiff = ((u2 + 16384) >> 7) + hT;                          // one sweep of the colours across the second mix
      int32_t u = v3_clamp((s * g8) >> 8, -16384, 16384);
      int pos = (i * 3 + j * 4) >> 2;                                 // a gentle hue drift across the screen
      int c = j * V8_GW + i, lev, hue;
      if (style == H8QU_GLOW) {
        int32_t q = u > 0 ? (u * con) >> 14 : 0; if (q > 256) q = 256;
        lev = (q * q * 226) >> 16;
        hue = (((u + 16384) >> 7) * hueMul >> 8) + pos + cdiff;
      } else if (style == H8QU_BANDS) {
        int ph = (int)(((u + 16384) * bands) >> 7) & 255;
        int tri = ph < 128 ? ph : 255 - ph;                          // 0 … 127
        lev = 20 + ((tri * tri) >> 6) * con / 256 + 10;
        hue = (((u + 16384) >> 7) * hueMul >> 8) + (ph >> 4) + pos + cdiff;
      } else {
        lev = 112 + ((u * con) >> 14) * 84 / 260;                   // gentle: no hard black holes
        hue = (((u + 16384) >> 7) * hueMul >> 8) + pos + cdiff;
      }
      LEV[c] = (uint8_t)v3_clamp(lev, 0, 226); HUE[c] = (uint8_t)(hue & 255);
    }
  }
  v8_upscale(6);
  v8_end();
}

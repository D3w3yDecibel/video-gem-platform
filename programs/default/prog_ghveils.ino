// =====================================================================
// PROGRAM: VEILS — aurora curtains & Lumia veils (by Dewey) — Set 8 slot 1
//
// Greatest hit from Set 7 (LUMIA, Thomas Wilfred's Clavilux) and the Aurora
// looks of Set 2 (SPECTRUM FLOW) and Set 4 (2030s FUTURE): soft curtains of
// light hanging in the dark and folding slowly in a solar wind, or wide
// veils of colour drifting across like light through silk.
//
// How it works: each curtain is a wavy line across the screen made of two
// slow sine waves. On the 80 × 60 light grid, row by row, every curtain adds
// light near its line — aurora curtains are brightest at the bottom edge and
// fade upwards, through fine shimmering rays; veils glow either side of the
// line. Where curtains cross the light adds up and the colours blend.
//
// Presets: k0 Northern Lights · k1 Lumia · k2 Silk · k3 Borealis · k4 Dawn ·
//          k5 Curtain Call · k6 Tide · k7 Firefall · k8 Nebula · k9 Polar ·
//          k10 Sunset Veil · k11 Tour
//   k12 — FLARE: a surge of light runs along the curtains
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (curtain size) · p3 Colour Flow · p4 Hue Spread
//   p5 Curtains · p6 Height · p7 Wave · p8 Rays · p9 Drift · p10 Glow
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8VE_AURORA, H8VE_VEIL };
struct H8VeLook { uint8_t mode, n, height, wave, rays, drift, glow, stars, pal, flow, spread; int8_t key; };
static const H8VeLook h8ve_looks[11] = {
  // mode        n  hgt wave rays drift glow stars pal          flow spr key
  { H8VE_AURORA, 3,  55,  50,  60,  40,  55, 60, V8P_AURORA,   2,  50,  0 },   // Northern Lights
  { H8VE_VEIL,   4,  45,  55,  15,  40,  55,  0, V8P_DUSK,     3,  80,  0 },   // Lumia
  { H8VE_VEIL,   7,  15,  60,  10,  50,  60,  0, V8P_GALAXY,    3, 100,  0 },   // Silk
  { H8VE_AURORA, 4,  60,  45,  85,  35,  60, 70, V8P_LAGOON,   2,  60,  0 },   // Borealis
  { H8VE_VEIL,   3,  70,  40,  10,  30,  50,  0, V8P_PEACH,    2,  60,  0 },   // Dawn
  { H8VE_AURORA, 2,  80,  55,  70,  35,  60, 50, V8P_ELECTRIC,   2,  50,  0 },   // Curtain Call
  { H8VE_VEIL,   4,  55,  30,  10,  20,  50,  0, V8P_LAGOON,   1,  60,  0 },   // Tide
  { H8VE_AURORA, 3,  55,  60,  65,  45,  65, 30, V8P_EMBER,    3,  50,  0 },   // Firefall
  { H8VE_VEIL,   5,  60,  45,  25,  35,  50, 60, V8P_MIAMI,   2,  90,  0 },   // Nebula
  { H8VE_AURORA, 5,  45,  50,  70,  40,  55, 50, V8P_SHERBET,  3,  90,  0 },   // Polar
  { H8VE_VEIL,   4,  50,  50,  15,  35,  55,  0, V8P_SUNSET,   2,  70,  0 },   // Sunset Veil
};
static const char* const h8ve_names[12] = { "Northern Lights", "Lumia", "Silk", "Borealis", "Dawn", "Curtain Call", "Tide", "Firefall", "Nebula", "Polar", "Sunset Veil", "Tour" };
static const char* const h8ve_lbl[6] = { "Curtains", "Height", "Wave", "Rays", "Drift", "Glow" };
const char* prog_ghveils_name() { return "VEILS"; }
const char* prog_ghveils_character() { return "Aurora curtains and Lumia veils: soft folds of light drifting in the dark"; }
const char* prog_ghveils_presetName(int p) { return (p >= 0 && p < 12) ? h8ve_names[p] : NULL; }
const char* prog_ghveils_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8ve_lbl); }
uint8_t prog_ghveils_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8VE_MAX 8
static float h8ve_n = 3, h8ve_h = 0.5f, h8ve_wave = 0.5f, h8ve_rays = 0.5f, h8ve_drift = 0.5f, h8ve_glow = 0.5f;
static float h8ve_t = 0, h8ve_flare = -1;
static uint32_t h8ve_seed = 3;
void prog_ghveils_init() { v8_init(); h8ve_flare = -1; h8ve_seed = 1 + v3_rnd(100000); }

void prog_ghveils_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8VeLook& L = h8ve_looks[look];
  float rate = v8_rate();
  v6_ease(&h8ve_n, v3_centre(5) ? (float)L.n : 1.0f + v3_u(5) * (H8VE_MAX - 1), rate);
  v6_ease(&h8ve_h, v8_kn(6, L.height / 100.0f), rate);
  v6_ease(&h8ve_wave, v8_kn(7, L.wave / 100.0f), rate);
  v6_ease(&h8ve_rays, v8_kn(8, L.rays / 100.0f), rate);
  v6_ease(&h8ve_drift, v8_kn(9, L.drift / 100.0f), rate);
  v6_ease(&h8ve_glow, v8_kn(10, L.glow / 100.0f), rate);
  if (v6_f.fresh) h8ve_seed = 1 + v3_rnd(100000);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 80, L.key / 100.0f);
  h8ve_t += v6_f.sdt * (0.1f + h8ve_drift * h8ve_drift * 4.0f);
  if (h8ve_t > 20000.0f) h8ve_t -= 20000.0f;
  if (v6_k12hit) h8ve_flare = 0;
  int flareX = -1000;
  if (h8ve_flare >= 0) { h8ve_flare += v6_f.dt * 0.35f; if (h8ve_flare > 1.0f) h8ve_flare = -1; else flareX = (int)(h8ve_flare * 1.4f * V8_GW * 16) - V8_GW * 3; }

  float z = v8_zoom(1.0f);
  int n = v3_clamp((int)(h8ve_n + 0.5f), 1, H8VE_MAX);
  bool aur = L.mode == H8VE_AURORA;
  // each curtain: its line (1/16 cells) and rays per column, its height, hue
  int16_t (*yc)[V8_GW] = (int16_t (*)[V8_GW])V8_EXTRA;                 // in the shared memory, not RAM of its own
  uint8_t (*ray)[V8_GW] = (uint8_t (*)[V8_GW])(V8_EXTRA + H8VE_MAX * V8_GW * 2);
  int32_t hgt[H8VE_MAX], inv[H8VE_MAX]; int hue0[H8VE_MAX];
  for (int k = 0; k < n; k++) {
    uint32_t h = v3_hash(h8ve_seed * 31u + k * 7717u);
    float base = aur ? 0.42f + 0.38f * ((h & 255) / 255.0f) : 0.12f + 0.76f * ((k + 0.5f) / n) + 0.06f * (((h & 255) / 255.0f) - 0.5f);
    int f1 = (int)((5 + (h >> 8) % 8) * (0.5f + h8ve_wave) / z), f2 = (int)((14 + (h >> 12) % 14) * (0.5f + h8ve_wave) / z);
    int ph1 = (int)(h8ve_t * (40 + (h >> 16) % 40)) + (int)(h % 1024), ph2 = (int)(h8ve_t * (25 + (h >> 20) % 30)) + (int)((h >> 3) % 1024);
    int amp1 = (int)((1.0f + 17.0f * h8ve_wave) * 16 * z), amp2 = amp1 / 3;
    int bb = (int)((base + 0.08f * sinf(h8ve_t * 0.05f + k)) * V8_GH * 16);
    int rk = 30 + (int)((h >> 24) % 40);
    for (int i = 0; i < V8_GW; i++) {
      yc[k][i] = (int16_t)(bb + ((amp1 * v3_sin(i * f1 + ph1) + amp2 * v3_sin(i * f2 - ph2)) >> 14));
      if (aur) {
        int nz = (int)(v3_noise1((uint32_t)(i * rk * 4 + (int)(h8ve_t * 90) + 100000), h) >> 6);   // −256 … 256
        int fine = (int)(v3_noise1((uint32_t)(i * 300 + (int)(h8ve_t * 160) + 50000), h ^ 77) >> 7);
        ray[k][i] = (uint8_t)v3_clamp(170 + (((nz + fine) * (int)(h8ve_rays * 230)) >> 8), 10, 255);
      } else ray[k][i] = (uint8_t)v3_clamp(200 + (int)(v3_noise1((uint32_t)(i * 60 + (int)(h8ve_t * 40)), h) >> 9), 120, 255);
    }
    hgt[k] = (int32_t)((aur ? (4.0f + h8ve_h * 48.0f) : (1.0f + h8ve_h * 16.0f)) * 16 * z);
    if (hgt[k] < 16) hgt[k] = 16;
    inv[k] = (256 << 12) / hgt[k];
    hue0[k] = (int)((k * 5 + 2) & 15) * 16;
  }
  int peak = 90 + (int)(h8ve_glow * 420);
  int knee = 260 - (int)(h8ve_glow * 120);
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int32_t Lr[V8_GW], HLr[V8_GW];
  for (int j = 0; j < V8_GH; j++) {
    for (int i = 0; i < V8_GW; i++) { Lr[i] = 0; HLr[i] = 0; }
    int32_t cy = j * 16 + 8;
    for (int k = 0; k < n; k++) {
      int32_t hg = hgt[k], iv = inv[k];
      for (int i = 0; i < V8_GW; i++) {
        int32_t u = yc[k][i] - cy;                             // > 0: above the curtain's line
        int32_t I, hu;
        if (aur) {
          if (u < -24 || u >= hg) continue;
          if (u < 0) { int32_t q = 24 + u; I = (q * q * 180) / 576; }   // soft below the edge
          else {
            int32_t q = 256 - ((u * iv) >> 12); I = (q * q) >> 8;
            if (u < 20) I += (20 - u) * 4;                        // the bright lower edge
          }
          hu = hue0[k] + ((u * 48) / hg);                         // colour shifts going up
        } else {
          int32_t au = u < 0 ? -u : u;
          if (au >= hg) continue;
          int32_t q = 256 - ((au * iv) >> 12); I = (q * q) >> 8;
          hu = hue0[k] + (u * 24) / hg;
        }
        int32_t fl = flareX > -999 ? v3_abs(i * 16 - flareX) : 9999;
        if (fl < 160) I += ((160 - fl) * 2 * I) >> 8;
        I = (I * ray[k][i] >> 8) * peak >> 8;
        Lr[i] += I; HLr[i] += I * (hu & 255);
      }
    }
    for (int i = 0; i < V8_GW; i++) {
      int k = j * V8_GW + i;
      int32_t l = Lr[i];
      LEV[k] = (uint8_t)((l * 226) / (l + knee));
      HUE[k] = l ? (uint8_t)(HLr[i] / l) : 0;
    }
  }
  v8_upscale(8);
  if (L.stars) v8_stars(L.stars, 5, h8ve_seed);
  v8_end();
}

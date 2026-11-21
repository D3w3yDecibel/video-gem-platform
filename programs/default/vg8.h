// =====================================================================
// vg8.h — the Set 8 "Greatest Hits: Chill Edition" engine (by Dewey)
//
// Set 8 is a compilation: the favourite looks from Sets 1-7 rebuilt as
// slow, smooth, laid-back versions, plus two meme programs. It is made to
// be MIXED — run it into one side of a 2-channel mixer with a Recursion
// Studio 4K (or any other source) on the other.
//
// Built on the Set 6 engine (vg6.h → vg5fx.h → vg3.h), like Sets 6 and 7 —
// same knob layout, same tour, same live FX. What Set 8 adds:
//
//   • CHILL CLOCK — every program runs at 0.6× its natural pace before p1
//     Speed is applied. Tour (k11) holds each look 40 s, looks fade over 4 s,
//     palettes cross-fade over 3 s.
//   • 24 PALETTES — twelve chill (Dusk, Vapor, Lava, Lagoon, Sunset, Forest,
//     Lounge, Peach, Aurora, Velvet, Sherbet, Ember) and twelve bold, high-
//     contrast ones (Neon Pop, Miami, Fire & Ice, Acid, Rainbow, Tropic, Candy,
//     Toxic, Infrared, Galaxy, Retro Pop, Electric).
//   • CONTRAST and SATURATION on p14/p15, applied to every program's palette.
//   • p11 KEY (for the mixer): centre = the preset's own. Turn RIGHT to
//     crush the dark levels to pure black (clean luma key / additive mix over
//     the other channel). Turn LEFT to lift the blacks into a soft tinted
//     glow (when the Gem is the background layer).
//   • SOFT FIELDS — many programs work out light on an 80 × 60 grid (one
//     cell = 4 × 4 px) and spread it over the screen with bilinear blending
//     and a fine dither (v8_upscale). Smooth gradients, cheap to draw.
//   • A FULL 5×7 FONT (A-Z, 0-9, punctuation) for the meme programs.
//
// Knobs on every Set 8 program:
//   p0 Palette (centre = preset's own) · p1 Speed (asleep until turned)
//   p2 Zoom · p3 Colour Flow · p4 Hue Spread · p5-p10 the program's own
//   p11 Key: Glow ◄ | ► Black · p12 Warp, p13 Mirror (live FX, asleep until turned)
//   p14 CONTRAST (◄ one smooth gradient | opposite colour families ►)
//   p15 SATURATION (◄ pastel | super vivid ►)
// =====================================================================
#ifndef VG8_H
#define VG8_H
#include "vg6.h"                       // (only vg6: works whichever Set 7 engine is in the folder)

#define V8_CHILL 0.6f

// ─── Palettes: twelve gradients of six colours (they wrap round) ───────
#define V8_NPAL 24
static const uint8_t v8_pals[V8_NPAL][6][3] = {
  {{ 30,  20,  70}, {110,  50, 120}, {220, 110, 130}, {255, 180, 140}, {240, 200, 120}, { 60, 110, 140}},   //  0 Dusk: lofi indigo, plum, rose, peach
  {{255,  80, 190}, {150,  60, 230}, { 60, 200, 255}, {120, 255, 210}, {200, 160, 255}, { 40,  30, 120}},   //  1 Vapor
  {{120,   0,  30}, {230,  50,  20}, {255, 160,  30}, {255,  80, 140}, {150,  30, 170}, {200,  20,  60}},   //  2 Lava
  {{ 10,  40,  90}, {  0, 130, 150}, { 60, 210, 200}, {170, 250, 220}, {240, 220, 160}, { 30,  80, 170}},   //  3 Lagoon
  {{ 60,  20, 110}, {210,  40, 140}, {255, 110,  60}, {255, 200,  80}, {240,  90, 120}, { 40,  30,  90}},   //  4 Sunset
  {{ 10,  60,  40}, { 60, 140,  60}, {170, 210,  80}, {240, 220, 120}, { 40, 160, 140}, { 30,  90,  50}},   //  5 Forest
  {{ 90,  20, 160}, { 40,  80, 255}, {230,  40, 200}, { 40, 230, 255}, {150,  60, 255}, {255,  90, 170}},   //  6 Lounge
  {{255, 170, 130}, {255, 210, 160}, {255, 240, 210}, {240, 140, 150}, {255, 120, 100}, {180, 110, 150}},   //  7 Peach
  {{ 20, 220, 120}, {  0, 170, 170}, {120,  80, 230}, {240, 100, 200}, { 80, 230, 255}, { 20,  60, 160}},   //  8 Aurora
  {{110,  10,  40}, {170,  30,  70}, {230, 170,  70}, {120,  40, 120}, {220,  90, 120}, { 60,  10,  80}},   //  9 Velvet
  {{255, 170, 200}, {170, 250, 200}, {255, 240, 150}, {200, 170, 255}, {150, 210, 255}, {255, 200, 160}},   // 10 Sherbet
  {{ 60,  20,   5}, {170,  70,  10}, {255, 150,  40}, {255, 230, 170}, {200, 100,  40}, {120,  40,  15}},   // 11 Ember
  // the bold ones: strong colours set against their opposites
  {{255,   0, 150}, {  0, 240, 255}, {255, 240,   0}, {140,   0, 255}, {  0, 255, 120}, {255,  90,   0}},   // 12 Neon Pop
  {{255,  40, 160}, {  0, 210, 200}, {255, 120, 200}, {  0, 140, 255}, {255, 200,  80}, { 20, 230, 170}},   // 13 Miami
  {{255,  40,   0}, {255, 180,   0}, {255, 255, 200}, {  0, 200, 255}, {  0,  60, 255}, {140,   0, 255}},   // 14 Fire & Ice
  {{170, 255,   0}, {120,   0, 255}, {  0, 255, 170}, {255,   0, 220}, {230, 255,  60}, { 60,   0, 180}},   // 15 Acid
  {{255,   0,   0}, {255, 200,   0}, {  0, 255,   0}, {  0, 220, 255}, { 40,   0, 255}, {255,   0, 220}},   // 16 Rainbow
  {{255,  90,  40}, {  0, 200, 150}, {255, 220,   0}, {  0, 120, 220}, {255,  60, 140}, {120, 240,  60}},   // 17 Tropic
  {{255,  80, 180}, { 80, 220, 255}, {255, 250, 120}, {180, 100, 255}, {120, 255, 180}, {255, 150,  90}},   // 18 Candy
  {{  0, 255,  60}, {255,   0,  90}, {  0,  60,  30}, {200, 255,   0}, {140,   0,  70}, {  0, 255, 200}},   // 19 Toxic
  {{ 40,   0, 120}, {200,   0, 120}, {255,  60,   0}, {255, 230,   0}, {255, 255, 255}, {  0, 180, 255}},   // 20 Infrared
  {{ 40,   0, 140}, {255,   0, 180}, {  0, 200, 255}, {255, 255, 255}, {140,  60, 255}, {255, 120,   0}},   // 21 Galaxy
  {{255,  60,  60}, {255, 200,  40}, { 40, 180, 255}, {255, 255, 255}, { 40, 200, 120}, {150,  60, 255}},   // 22 Retro Pop
  {{  0,  80, 255}, {  0, 255, 255}, {255,   0, 255}, {255, 255,   0}, {255,   0,  80}, {  0, 255, 100}},   // 23 Electric
};
static const char* const v8_palNames[V8_NPAL] = { "Dusk", "Vapor", "Lava", "Lagoon", "Sunset", "Forest", "Lounge", "Peach", "Aurora", "Velvet", "Sherbet", "Ember",
  "Neon Pop", "Miami", "Fire & Ice", "Acid", "Rainbow", "Tropic", "Candy", "Toxic", "Infrared", "Galaxy", "Retro Pop", "Electric" };
enum { V8P_DUSK, V8P_VAPOR, V8P_LAVA, V8P_LAGOON, V8P_SUNSET, V8P_FOREST, V8P_LOUNGE, V8P_PEACH, V8P_AURORA, V8P_VELVET, V8P_SHERBET, V8P_EMBER,
       V8P_NEON, V8P_MIAMI, V8P_FIREICE, V8P_ACID, V8P_RAINBOW, V8P_TROPIC, V8P_CANDY, V8P_TOXIC, V8P_INFRARED, V8P_GALAXY, V8P_RETROPOP, V8P_ELECTRIC };
// the colour knobs (p14 Contrast, p15 Saturation): the presets' own amounts in their centre bands
#define V8_OWN_CONTRAST 0.45f
#define V8_OWN_VIVID 0.35f

// ─── Frame state ──────────────────────────────────────────────────────
static float v8_fade = 1.0f;          // 0 → 1 over 4 s after each look change
static float v8_key = 0;              // −1 (glow) … 0 … 1 (black), eased
static float v8_con = V8_OWN_CONTRAST, v8_viv = V8_OWN_VIVID;   // contrast 0 … 1, saturation −1 … 1, eased
static float v8_cs[2] = { 512, 512 };                          // p14 and p15, smoothed here (they are not FX in Set 8)
static int v8_frameNo = 0;
static void v8_init() { v6_init(); v8_fade = 1.0f; v8_key = 0; }

// every frame, first. Returns the look to draw (preset 11 = tour).
static int v8_begin(int preset, int nLooks) {
  // vg6 parks a sleeping FX knob's smoothed value at 512; the smoothing then drifts it back toward the
  // real knob, which looks like a turn and wakes the FX (a knob left at an end switched FX on). Put
  // the real value back first, so only a real turn wakes them.
  if (v6_refOK) for (int i = 0; i < 4; i++) if (!v6_armed[1 + i]) v3_sm[12 + i] = (float)pots[12 + i];
  int look = v6_begin(preset, nLooks);
  v8_frameNo++;
  fx5_amt[2] = fx5_amt[3] = 0;                                  // p14, p15 are colour knobs in Set 8, not the colour/glitch FX
  for (int i = 0; i < 2; i++) { float k = (float)pots[14 + i]; if (v6_first) v8_cs[i] = k; else v8_cs[i] += (k - v8_cs[i]) * v3_clampf(v3_dt / 0.12f, 0.0f, 1.0f); }
  v6_t -= v6_f.sdt * (1.0f - V8_CHILL);
  v6_f.sdt *= V8_CHILL;
  if (preset == 11) v6_tourT -= v3_dt * 0.5f;             // the tour holds each look 40 s
  if (v6_first) v8_fade = 1.0f;
  else if (v6_f.fresh) v8_fade = 0.0f;
  else if (v8_fade < 1.0f) { v8_fade += v3_dt / 4.0f; if (v8_fade > 1.0f) v8_fade = 1.0f; }
  return look;
}
// zoom from p2: the look's own in the centre, ⅓× … 3× across the dial
static float v8_zoom(float own) { return v3_centre(2) ? own : own * powf(3.0f, v3_bi(2)); }
// rate for v6_ease: slow during a look change, quicker once settled (follows the knobs)
static inline float v8_rate() { return v8_fade < 1.0f ? 0.8f : 3.0f; }
static inline void v8_end() { fx5_apply(); }
// phosphor afterglow: fade by rate16 sixteenths of a level a frame (the fraction spread over the rows in turn)
__attribute__((optimize("O2"))) static void v8_decay(int rate16) {
  if (rate16 <= 0) return;
  int whole = rate16 >> 4, frac = rate16 & 15;
  for (int y = 0; y < H; y++) {
    int n = whole + ((((y * 7) + v8_frameNo * 5) & 15) < frac ? 1 : 0);
    if (n <= 0) continue;
    if (n > 15) n = 15;
    uint32_t sub = (uint32_t)n * 0x01010101u;
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int i = 0; i < W / 4; i++) {
      uint32_t v = p[i];
      if (!(v & 0xF0F0F0F0u)) continue;                              // all black already
      uint32_t lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
      uint32_t d = (lev | 0x10101010u) - sub;
      uint32_t keep = ((d >> 4) & 0x01010101u) * 0x0Fu;
      p[i] = ((d & keep) << 4) | hue;
    }
  }
}

// ─── Palette with the KEY knob ────────────────────────────────────────
static void v8_grad(int pal, int p, int* o) {
  p %= 1536; if (p < 0) p += 1536;
  int i = p >> 8, f = p & 255, j = i == 5 ? 0 : i + 1;
  const uint8_t* a = v8_pals[pal][i]; const uint8_t* b = v8_pals[pal][j];
  for (int c = 0; c < 3; c++) o[c] = a[c] + (((b[c] - a[c]) * f) >> 8);
}
// the two colour families: hues 0-5 sit on one side of the gradient, 8-13 on the opposite side (+3 of 6 stops),
// with short blends between (hues 6-7 and 14-15), so neighbouring bands, blobs and threads set opposite colours
// against each other. Contrast 0 = one smooth gradient, 1 = full opposites.
static const uint8_t v8_fam[16] = { 0, 0, 0, 0, 0, 0, 64, 192, 255, 255, 255, 255, 255, 255, 192, 64 };
static void v8_palette(int spread256, int sheen, float key) {
  if (!v6_gammaOK) { for (int l = 0; l < 16; l++) v6_gamma[l] = (int)(256.0f * powf(l / 15.0f, 1.25f)); v6_gammaOK = true; }
  int crush = key > 0 ? (int)(key * 100) : 0;                   // 1/16 levels pushed to black (up to ~6 levels)
  int lift = key < 0 ? (int)(-key * 46) : 0;                    // a tinted floor, 0 … 46 / 256
  int base = (int)(v6_flow * 1536.0f), xf = (int)(v6_palX * 256);
  int con = (int)(v8_con * 768), vv = (int)(v8_viv * 256);     // half the wheel; saturation −256 … 256
  int kk[16];
  for (int l = 0; l < 16; l++) {
    int e = l * 16 - crush;                                     // effective level, 1/16 steps
    if (e <= 0) { kk[l] = 0; continue; }
    e = e * 240 / (240 - crush);
    int i = e >> 4, f = e & 15;
    int g = i >= 15 ? v6_gamma[15] : v6_gamma[i] + (((v6_gamma[i + 1] - v6_gamma[i]) * f) >> 4);
    kk[l] = lift + (((256 - lift) * g) >> 8);
  }
  if (lift) kk[0] = lift / 2;
  for (int h = 0; h < 16; h++) {
    int tri = h <= 8 ? h : 16 - h;                                // ping-pong: hue 15 runs smoothly back into hue 0
    int p = base + ((tri * spread256 * 12) >> 4) + ((con * v8_fam[h]) >> 8);
    int ca[3], cb[3], c[3];
    v8_grad(v6_palA, p, ca); v8_grad(v6_palB, p, cb);
    for (int q = 0; q < 3; q++) c[q] = ca[q] + (((cb[q] - ca[q]) * xf) >> 8);
    int lum = (c[0] * 77 + c[1] * 150 + c[2] * 29) >> 8;         // saturation: push away from (or toward) grey
    int gain = 256 + (vv > 0 ? vv * 3 / 2 : vv * 3 / 4);
    for (int q = 0; q < 3; q++) c[q] = v3_clamp(lum + (((c[q] - lum) * gain) >> 8), 0, 255);
    for (int l = 0; l < 16; l++) {
      int k = kk[l], w = l > 11 ? ((l - 11) * sheen) >> 2 : 0;
      int r = (c[0] * k) >> 8, g = (c[1] * k) >> 8, b = (c[2] * k) >> 8;
      if (w) { r += ((255 - r) * w) >> 8; g += ((255 - g) * w) >> 8; b += ((255 - b) * w) >> 8; }
      display.setColor((uint8_t)((l << 4) | h), (uint8_t)v3_clamp(r, 0, 255), (uint8_t)v3_clamp(g, 0, 255), (uint8_t)v3_clamp(b, 0, 255));
    }
  }
}
// p14 / p15 as 0 … 1023 (smoothed); the centre band 440-600 = the preset's own
static inline bool v8_cCentre(int i) { return v8_cs[i] >= 440 && v8_cs[i] < 600; }
static inline float v8_cU(int i) { return v3_clampf(v8_cs[i] / 1023.0f, 0.0f, 1.0f); }
// palette, colour flow, hue spread and key from the common knobs (the look's own in the centre bands)
static void v8_colours(int ownPal, float ownFlow, float ownSpread, int sheen, float ownKey) {
  int pal = v3_centre(0) ? ownPal : v3_clamp(v3_k(0) * V8_NPAL / 1024, 0, V8_NPAL - 1);
  if (v6_first) { v6_palA = v6_palB = pal; v6_palX = 1.0f; }
  if (pal != v6_palB) { v6_palA = v6_palB; v6_palB = pal; v6_palX = 0; }
  if (v6_palX < 1.0f) { v6_palX += v3_dt / 3.0f; if (v6_palX >= 1.0f) { v6_palX = 1.0f; v6_palA = v6_palB; } }
  float fl = v3_centre(3) ? ownFlow * 1.6f : v3_bi(3) * fabsf(v3_bi(3)) * 0.5f;   // (Set 8 v2: flows a little livelier)
  v6_f.flowRate = fl;
  v6_flow += fl * v3_dt * (v6_f.speed > 0 ? 1.0f : 0.0f);
  v6_flow -= floorf(v6_flow);
  float sp = v3_centre(4) ? v3_clampf(ownSpread * 1.35f + 0.1f, 0.3f, 1.3f) : 0.08f + v3_u(4) * 1.6f;   // wider by default
  v6_f.spread = (int)(sp * 256);
  float kt = v3_centre(11) ? ownKey : v3_bi(11);
  if (v6_first) v8_key = kt; else v8_key += (kt - v8_key) * v3_clampf(v3_dt * 4.0f, 0.0f, 1.0f);
  float ct = v8_cCentre(0) ? V8_OWN_CONTRAST : v8_cU(0);
  float vt = v8_cCentre(1) ? V8_OWN_VIVID : v8_cU(1) * 2.0f - 1.0f;
  if (v6_first) { v8_con = ct; v8_viv = vt; }
  else { float a = v3_clampf(v3_dt * 4.0f, 0.0f, 1.0f); v8_con += (ct - v8_con) * a; v8_viv += (vt - v8_viv) * a; }
  v8_palette(v6_f.spread, sheen, v8_key);
}

// ─── Labels ───────────────────────────────────────────────────────────
static const char* v8_label(int pot, const char* const* own) {
  if (pot >= 0 && pot <= 4) return v6_commonLbl[pot];
  if (pot == 12 || pot == 13) return fx5_labels[pot - 12];
  if (pot == 14) return "Contrast";
  if (pot == 15) return "Saturation";
  if (pot >= 5 && pot <= 10) return own[pot - 5];
  if (pot == 11) return "Key: Glow|Black";
  return "";
}
// a knob value: the look's own in the centre band, else 0 … 1 across the dial
static inline float v8_kn(int pot, float own) { return v3_centre(pot) ? own : v3_u(pot); }

// ─── Soft fields: an 80 × 60 grid of light, spread over the screen ────
#define V8_GW 80
#define V8_GH 60
#define V8_GN (V8_GW * V8_GH)
#define V8_LEV (v6_mem)                    // level per cell, 0 … 226
#define V8_HUE (v6_mem + V8_GN)            // hue per cell, 0 … 255 (16 per palette hue)
#define V8_EXTRA (v6_mem + 2 * V8_GN)      // the rest of v6_mem (14 976 bytes) for the program
#define V8_EXTRA_N (V6_MEM - 2 * V8_GN)
static const uint8_t v8_bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
static inline int v8_hwrap(int d) { return d > 128 ? d - 256 : (d < -128 ? d + 256 : d); }
// grid → screen: bilinear between cells, Bayer-dithered, hue blended the short way round
__attribute__((optimize("O2"))) static void v8_upscale(int grainAmt) {
  const uint8_t* L = V8_LEV; const uint8_t* Hh = V8_HUE;
  uint8_t rl[V8_GW + 1]; int16_t rh[V8_GW + 1];
  uint32_t g = (uint32_t)v8_frameNo * 2654435761u + 1;
  for (int y = 0; y < H; y++) {
    int32_t gy = y * 16 - 24;
    int j = gy >> 6, fy = (gy & 63) << 2;
    if (gy < 0) { j = 0; fy = 0; }
    if (j >= V8_GH - 1) { j = V8_GH - 1; fy = 0; }
    int j1 = j + 1 < V8_GH ? j + 1 : j;
    const uint8_t *a = L + j * V8_GW, *b = L + j1 * V8_GW, *ha = Hh + j * V8_GW, *hb = Hh + j1 * V8_GW;
    for (int i = 0; i < V8_GW; i++) {
      rl[i] = (uint8_t)(a[i] + (((b[i] - a[i]) * fy) >> 8));
      rh[i] = (int16_t)(ha[i] + ((v8_hwrap(hb[i] - ha[i]) * fy) >> 8));
    }
    rl[V8_GW] = rl[V8_GW - 1]; rh[V8_GW] = rh[V8_GW - 1];
    uint32_t* out = (uint32_t*)(v3_buf + y * W);
    const uint8_t* bay = v8_bayer + ((y & 3) << 2);
    int d0 = bay[0] * 4, d1 = bay[1] * 4, d2 = bay[2] * 4, d3 = bay[3] * 4;
    for (int i = 0; i < V8_GW; i++) {
      int l = rl[i] << 2, dl = rl[i + 1] - rl[i];
      int h = rh[i] * 4 + 4096, dh = v8_hwrap(rh[i + 1] - rh[i]);         // (rh may dip below 0 after the wrap)
      if (grainAmt) { g = g * 1664525u + 1013904223u; l += (int)((g >> 24) * grainAmt) >> 8; }
      uint32_t p0 = (uint32_t)((((l + d0) >> 6) << 4) | (((h + d0) >> 6) & 15));
      uint32_t p1 = (uint32_t)((((l + dl + d1) >> 6) << 4) | (((h + dh + d1) >> 6) & 15));
      uint32_t p2 = (uint32_t)((((l + 2 * dl + d2) >> 6) << 4) | (((h + 2 * dh + d2) >> 6) & 15));
      uint32_t p3 = (uint32_t)((((l + 3 * dl + d3) >> 6) << 4) | (((h + 3 * dh + d3) >> 6) & 15));
      out[i] = p0 | (p1 << 8) | (p2 << 16) | (p3 << 24);
    }
  }
}
// polar tables for the grid (angle 0 … 1023, radius in px from the centre), in V8_EXTRA
#define V8_ANG ((uint16_t*)V8_EXTRA)
#define V8_RAD (V8_EXTRA + 2 * V8_GN)
static bool v8_polarOK = false;
static void v8_polar() {
  if (v8_polarOK) return;
  uint16_t* A = V8_ANG; uint8_t* R = V8_RAD;
  for (int j = 0; j < V8_GH; j++) for (int i = 0; i < V8_GW; i++) {
    int dx = 2 * i + 1 - V8_GW, dy = 2 * j + 1 - V8_GH;                  // half cells
    A[j * V8_GW + i] = (uint16_t)v3_atan(dy, dx);
    R[j * V8_GW + i] = (uint8_t)v3_clamp((int)v3_isqrt((uint32_t)(dx * dx + dy * dy)) * 2, 0, 255);
  }
  v8_polarOK = true;
}
// a few soft stars sprinkled over the dark parts (after v8_upscale)
static void v8_stars(int n, int lev, uint32_t seed) {
  for (int s = 0; s < n; s++) {
    uint32_t h = v3_hash(seed + s * 7919u);
    int x = (int)(h % W), y = (int)((h >> 9) % H);
    uint8_t* p = v3_buf + y * W + x;
    if ((*p >> 4) > 2) continue;
    int tw = (v3_sin((int)(h >> 20) + v8_frameNo * (2 + (int)(h & 3))) + 16384) >> 12;   // 0 … 8
    int l = v3_clamp(lev - 2 + (tw >> 1), 1, 15);
    *p = (uint8_t)((l << 4) | ((h >> 4) & 15));
  }
}

// ─── Shapes ───────────────────────────────────────────────────────────
static void v8_ellipse(int cx, int cy, int rx, int ry, uint8_t c) {
  if (rx <= 0 || ry <= 0) return;
  int32_t ry2 = ry * ry;
  for (int dy = -ry; dy <= ry; dy++) {
    int y = cy + dy; if ((unsigned)y >= H) continue;
    int32_t q = ((ry2 - dy * dy) << 8) / ry2;                       // (1 − dy²/ry²) × 256
    int w = (int)((rx * (int32_t)v3_isqrt((uint32_t)(q << 8))) >> 8);
    v3_hspan(cx - w, cx + w, y, c);
  }
}
static void v8_tri(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c) {
  int32_t xs[3] = { x0 * 16, x1 * 16, x2 * 16 }, ys[3] = { y0 * 16, y1 * 16, y2 * 16 };
  v6_fill(xs, ys, 3, c);
}
// a thick line: discs of radius r along it
static void v8_thick(int x0, int y0, int x1, int y1, int r, uint8_t c) {
  int dx = x1 - x0, dy = y1 - y0, n = v3_abs(dx) > v3_abs(dy) ? v3_abs(dx) : v3_abs(dy);
  if (n > 600) return;
  if (n == 0) { v3_disc(x0, y0, r, c); return; }
  int st = r > 1 ? r / 2 : 1;
  for (int i = 0; i <= n; i += st) v3_disc(x0 + dx * i / n, y0 + dy * i / n, r, c);
  v3_disc(x1, y1, r, c);
}

// ─── Full 5×7 font ────────────────────────────────────────────────────
static const char v8_fontChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.,:-'%/+=<>()#*_\"@";
static const uint8_t v8_font[][7] = {
  {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
  {31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},
  {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},
  {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
  {31,1,2,4,8,16,31},
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{31,2,4,2,1,17,14},{2,6,10,18,31,2,2},
  {31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
  {0,0,0,0,0,0,0},{4,4,4,4,4,0,4},{14,17,1,2,4,0,4},{0,0,0,0,0,12,12},{0,0,0,0,12,4,8},
  {0,12,12,0,12,12,0},{0,0,0,31,0,0,0},{4,4,8,0,0,0,0},{24,25,2,4,8,19,3},{1,1,2,4,8,16,16},
  {0,4,4,31,4,4,0},{0,0,31,0,31,0,0},{2,4,8,16,8,4,2},{8,4,2,1,2,4,8},{2,4,8,8,8,4,2},
  {8,4,2,2,2,4,8},{10,10,31,10,31,10,10},{0,4,21,14,21,4,0},{0,0,0,0,0,0,31},{10,10,0,0,0,0,0},
  {0,10,31,31,14,4,0},                                                                     // @ = a heart
};
static int v8_glyph(char c) {
  if (c >= 'a' && c <= 'z') c = (char)(c - 32);
  for (int i = 0; v8_fontChars[i]; i++) if (v8_fontChars[i] == c) return i;
  return 36;
}
static inline int v8_textW(const char* s, int sc) { return (int)strlen(s) * 6 * sc - sc; }
static void v8_text(int x, int y, const char* s, int sc, uint8_t c) {
  for (int i = 0; s[i]; i++) {
    const uint8_t* g = v8_font[v8_glyph(s[i])];
    int gx = x + i * 6 * sc;
    if (gx > W || gx + 5 * sc < 0) continue;
    for (int r = 0; r < 7; r++) { if (!g[r]) continue; for (int b = 0; b < 5; b++) if (g[r] & (16 >> b)) v3_rect(gx + b * sc, y + r * sc, gx + b * sc + sc - 1, y + r * sc + sc - 1, c); }
  }
}
// centred text with a dark drop shadow (readable over anything)
static void v8_textC(int cx, int y, const char* s, int sc, uint8_t c) {
  int x = cx - v8_textW(s, sc) / 2;
  v8_text(x + (sc + 1) / 2, y + (sc + 1) / 2, s, sc, (uint8_t)(c & 15));
  v8_text(x, y, s, sc, c);
}
#endif

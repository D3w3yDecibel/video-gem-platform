// =====================================================================
// PROGRAM: Y2K — the 2000s (by Dewey) — Set 4, slot 8
//
// The landscape flight in the millennium's glossy future: aqua and chrome,
// candy-coloured translucent hills (blueberry, tangerine, lime, grape),
// bubbles rising past lens flares, Frutiger-Aero meadows and water,
// bling, an ice age, and a green mobile-phone screen in big pixels.
//
// EXPLODE (p15, or k12 for one blast): the millennium bug bites. The
// picture glitches — lines tear sideways, blocks of the land jump and
// repeat, the colours slip out of register and error squares spray out of
// the horizon.
//
// Presets: k0 Aqua · k1 Bondi Blue · k2 Tangerine · k3 Lime · k4 Grape · k5 Chrome City
//          k6 Millennium · k7 Frutiger Aero · k8 Bling · k9 Ice Age · k10 Pocket LCD · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Bubbles · p8 Fog · p9 Sun · p10 Draw Distance · p11 Lens Flare · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 bubbles · 2 flare · 4 sun · 8 stars · 16 fireworks · 32 LCD
static const H4World yk_worlds[11] = {
  {{{0,80,200},{60,160,240},{180,230,255},{235,250,255}}, {{120,220,255},{255,255,255}}, {{0,40,120},{0,90,190},{0,150,230},{60,200,255},{170,235,255},{255,255,255}}, {0,120,220}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_SHARDS, 50, 55, 25, 0, 1, 0, 1 | 2, 0, 0},                                                 // 0 Aqua
  {{{0,60,90},{0,130,160},{100,210,220},{220,250,250}}, {{0,200,220},{255,255,255}}, {{0,50,80},{0,100,130},{0,150,170},{40,200,210},{150,235,240},{255,255,255}}, {0,150,170}, {255,255,255},
   T4_ROLLING, 0, X4_SHARDS, 90, 40, 25, 0, 1, 0, 1 | 2 | 4, 26, 90},                                                // 1 Bondi Blue
  {{{255,120,0},{255,170,60},{255,220,150},{255,245,220}}, {{255,140,0},{255,255,230}}, {{150,40,0},{210,80,0},{255,120,0},{255,170,40},{255,215,130},{255,250,230}}, {255,100,0}, {255,255,255},
   T4_ROLLING, S4_CONTOUR, X4_SHARDS, 0, 45, 25, 0, 1, 0, 1 | 4, 30, 80},                                            // 2 Tangerine
  {{{90,170,0},{160,220,40},{220,250,150},{245,255,220}}, {{180,255,0},{255,255,255}}, {{20,80,0},{50,130,0},{100,180,0},{160,220,20},{210,245,120},{255,255,230}}, {80,170,0}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_SHARDS, 30, 50, 25, 0, 1, 0, 1 | 2, 0, 0},                                                 // 3 Lime
  {{{60,0,100},{120,40,170},{200,140,230},{240,220,255}}, {{180,90,255},{255,255,255}}, {{50,0,90},{90,10,150},{140,50,200},{190,110,240},{230,180,255},{255,240,255}}, {100,20,150}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_SHARDS, 30, 55, 25, 0, 1, 0, 1 | 2, 0, 0},                                                 // 4 Grape
  {{{10,10,30},{40,50,80},{110,120,150},{200,210,230}}, {{140,160,190},{255,255,255}}, {{20,25,40},{50,60,80},{90,100,125},{140,150,175},{190,200,220},{240,245,255}}, {60,70,90}, {120,220,255},
   T4_CITY, S4_EDGES, X4_SHARDS, 0, 50, 30, 0, 1, 0, 2, 0, 0},                                                       // 5 Chrome City
  {{{0,0,20},{10,10,60},{40,30,110},{100,60,150}}, {{255,200,60},{255,255,255}}, {{20,20,40},{40,40,80},{70,60,120},{110,90,160},{160,140,200},{220,210,240}}, {10,10,40}, {255,220,100},
   T4_MAP, S4_GRID, X4_SHARDS, 30, 50, 30, 0, 1, W4_STARS, 8 | 16, 0, 0},                                            // 6 Millennium
  {{{30,120,230},{90,180,250},{180,235,255},{240,255,250}}, {{255,255,200},{255,255,255}}, {{20,100,40},{50,150,50},{90,190,60},{150,220,90},{60,170,230},{230,255,255}}, {20,160,230}, {255,255,255},
   T4_ROLLING, 0, X4_SHARDS, 70, 40, 20, 0, 1, 0, 1 | 2 | 4, 24, 110},                                               // 7 Frutiger Aero
  {{{40,0,40},{150,30,120},{255,150,210},{255,230,240}}, {{255,215,0},{255,255,255}}, {{110,70,0},{180,130,0},{240,190,30},{230,60,160},{255,140,210},{255,255,255}}, {200,40,140}, {255,255,255},
   T4_SPIKES, S4_EDGES, X4_SHARDS, 0, 90, 25, 0, 2, W4_STARS, 2 | 8, 0, 0},                                          // 8 Bling
  {{{150,200,240},{200,230,250},{235,245,255},{250,255,255}}, {{150,220,255},{255,255,255}}, {{40,90,160},{80,140,210},{130,190,240},{180,220,250},{220,240,255},{255,255,255}}, {80,150,210}, {0,120,200},
   T4_SPIKES, S4_EDGES, X4_SHARDS, 30, 80, 35, 0, 1, 0, 2 | 4, 20, 110},                                             // 9 Ice Age
  {{{110,140,40},{130,160,50},{150,180,60},{160,190,70}}, {{40,60,20},{170,200,80}}, {{40,60,20},{60,80,25},{80,100,30},{100,120,35},{120,140,45},{140,165,55}}, {50,70,20}, {30,40,10},
   T4_MAP, S4_EDGES, X4_SHARDS, 0, 50, 0, 0, 4, 0, 32, 0, 0},                                                        // 10 Pocket LCD
};
static const char* const yk_presetNames[12] = {
  "Aqua", "Bondi Blue", "Tangerine", "Lime", "Grape", "Chrome City", "Millennium", "Frutiger Aero", "Bling", "Ice Age", "Pocket LCD", "Tour"
};
const char* prog_y2k_name() { return "2000s Y2K"; }
const char* prog_y2k_character() { return "The 2000s: glossy aqua and candy colours, bubbles and lens flares; EXPLODE is the millennium bug glitching it all"; }
const char* prog_y2k_presetName(int preset) { return (preset >= 0 && preset < 12) ? yk_presetNames[preset] : NULL; }
const char* prog_y2k_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Bubbles";
  if (pot == 11) return "Lens Flare";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_y2k_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_y2k_init() { h4_init(yk_worlds, 11); }

static float yk_glT = 0;
void prog_y2k_draw(int preset) {
  if (!h4_prepare(preset, 8.0f, 1.1f, 0.9f)) return;               // glossy light
  const H4World& wp = yk_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float bubbles = v3_centre(7) ? ((deco & 1) ? 0.6f : 0.0f) : v3_u(7);
  float flare = v3_centre(11) ? ((deco & 2) ? 0.8f : 0.0f) : v3_u(11);
  h4_prop(0, 255, 255, 255); h4_prop(1, 200, 240, 255); h4_prop(2, 120, 200, 255);   // bubble rim, shine, flare
  h4_prop(3, 255, 0, 80); h4_prop(4, 0, 255, 120); h4_prop(5, 40, 80, 255); h4_prop(6, 255, 240, 0); h4_prop(7, 0, 0, 0);   // errors

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(60 * h4k.skyK), hzY, 91);
  int sx = 220, sy = hzY - (wp.sunY ? wp.sunY / 2 : 40);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_glow(sx, sy, R * 3, 8); h4_sun(sx, sy, R, false); }
  if (deco & 16) {                                                  // millennium fireworks
    for (int i = 0; i < 3; i++) {
      float u = fmodf(h4_t * 0.4f + i * 0.37f, 1.0f); int cx = 60 + i * 100, cy = 30 + (i * 13) % 25, r = (int)(u * 40);
      for (int a = 0; a < 1024; a += 64) { int x = cx + ((v3_cos(a + i * 20) * r) >> 14), y = cy + ((v3_sin(a + i * 20) * r) >> 14) + (int)(u * u * 10); v3_put(x, y, (uint8_t)(H4_ACC + 15 - (int)(u * 10))); v3_put(x + 1, y, (uint8_t)(H4_PROP + 3 + i)); }
    }
  }
  if (E > 0.02f) h4_glow(160, hzY, (int)(20 + E * 70), (int)(6 + E * 8));

  // ── ground ──
  h4_ground(hzY, h4_rollPx);

  // ── bubbles rising, with a shine ──
  if (bubbles > 0.02f) {
    int n = (int)(bubbles * 22);
    for (int i = 0; i < n; i++) {
      uint32_t h = v3_hash(i * 2654435761u + 5);
      float sp = 15 + (h & 31);
      int r = 3 + (int)((h >> 5) & 7);
      int x = (int)((h >> 8) % W) + (int)(6 * sinf(h4_t + i)), y = H + 10 - (int)fmodf(h4_t * sp + (h >> 16) % H, (float)(H + 30));
      for (int a = 0; a < 1024; a += 24) v3_put(x + ((v3_cos(a) * r) >> 14), y + ((v3_sin(a) * r) >> 14), (uint8_t)(H4_PROP + ((a > 600 && a < 900) ? 0 : 1)));
      v3_put(x - r / 2, y - r / 2, H4_PROP + 0); v3_put(x - r / 2 + 1, y - r / 2, H4_PROP + 0);
    }
  }
  if (flare > 0.02f && (deco & 4)) {                                // a lens flare: rings strung through the middle of the picture
    for (int k = 1; k <= 5; k++) {
      float u = k * 0.35f; int x = sx + (int)((160 - sx) * u * 2), y = sy + (int)((120 - sy) * u * 2), r = 3 + ((k * 7) % 11);
      for (int a = 0; a < 1024; a += 16) v3_put(x + ((v3_cos(a) * r) >> 14), y + ((v3_sin(a) * r) >> 14), H4_PROP + 2);
      if (k & 1) v3_disc(x, y, r / 3, H4_PROP + 1);
    }
  }
  // ── the millennium bug ──
  if (E > 0.02f) {
    yk_glT += dt;
    int bands = 2 + (int)(E * 14);
    for (int b = 0; b < bands; b++) {                               // torn lines
      uint32_t h = v3_hash((uint32_t)(yk_glT * 12) * 977u + b * 131u);
      int y0 = (int)(h % H), hh = 2 + (int)((h >> 8) % (uint32_t)(4 + E * 20)), dx = (int)((h >> 16) % 80) - 40;
      for (int y = y0; y < y0 + hh && y < H; y++) h4_shiftRow(y, (int)(dx * E));
    }
    int blocks = (int)(E * 6);
    for (int b = 0; b < blocks; b++) {                              // a block of the picture copied somewhere else
      uint32_t h = v3_hash((uint32_t)(yk_glT * 8) * 31u + b * 7919u);
      int sx0 = (int)(h % (W - 40)), sy0 = (int)((h >> 9) % (H - 20)), w = 16 + (int)((h >> 18) % 40), hgt = 6 + (int)((h >> 24) % 18);
      int tx = v3_clamp(sx0 + (int)((h >> 3) % 60) - 30, 0, W - w - 1), ty = v3_clamp(sy0 + (int)((h >> 5) % 30) - 15, 0, H - hgt - 1);
      for (int y = 0; y < hgt; y++) memmove(v3_buf + (ty + y) * W + tx, v3_buf + (sy0 + y < H ? sy0 + y : H - 1) * W + v3_clamp(sx0, 0, W - w - 1), w);
    }
  }
  if (h4_trigger) h4_burstAt(160, (float)hzY, 30, 240, H4_PROP + 3, P4_SQUARE, 5, 20);
  int emit = (v3_rndf() < E) ? 1 + (int)(E * 1.5f) : 0;
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 80 + v3_rndf() * 200; h4_emit(160, (float)hzY, cosf(a) * s, sinf(a) * s * 0.7f, 0.5f + v3_rndf(), (uint8_t)(H4_PROP + 3 + v3_rnd(5)), P4_SQUARE, (uint8_t)(3 + v3_rnd(6))); }
  h4_parts(dt, 0.0f, 0.0f);
  if (deco & 32) h4_scanlines();                                     // the phone screen's pixel grid
}

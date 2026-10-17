// =====================================================================
// PROGRAM: NOIR — the 1930s (by Dewey) — Set 4, slot 1
//
// The landscape flight as film noir and the Depression era: hard black and
// white with a single red accent, rain slanting past rooftops, searchlights
// and a full moon, venetian-blind shadows striping the scene, fog on the
// docks, and a Dust Bowl plain under a choking brown sky.
//
// EXPLODE (p15, or k12 for one blast): the flashbulb pops — the picture
// cracks into horizontal blinds that slide apart, white light floods out of
// the horizon and broken glass flies at the camera.
//
// Presets: k0 Rain City · k1 Docks · k2 Searchlight · k3 Full Moon · k4 Red Lipstick · k5 Venetian
//          k6 Dust Bowl · k7 Gotham · k8 Skyline · k9 Desert Noir · k10 Vertigo · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Rain · p8 Fog · p9 Moon · p10 Draw Distance · p11 Blinds · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 rain · 2 searchlights · 4 moon · 8 blinds · 16 dust storm · 32 red windows
static const H4World nr_worlds[11] = {
  {{{0,0,0},{10,10,12},{35,35,40},{80,80,85}}, {{60,60,65},{255,255,255}}, {{5,5,5},{20,20,22},{45,45,50},{90,90,95},{160,160,165},{235,235,240}}, {10,10,12}, {230,20,30},
   T4_CITY, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 50, 55, 0, 1, 0, 1 | 32, 0, 0},                                       // 0 Rain City
  {{{0,0,0},{20,20,22},{70,70,75},{140,140,145}}, {{80,80,85},{255,255,255}}, {{10,10,10},{30,30,32},{60,60,62},{100,100,104},{150,150,155},{210,210,215}}, {25,25,28}, {255,255,255},
   T4_MAP, S4_EDGES, X4_SLABS | X4_SHARDS, 130, 40, 90, 0, 1, W4_CALM, 4, 18, 120},                                  // 1 Docks
  {{{0,0,0},{5,5,8},{25,25,30},{60,60,65}}, {{120,120,125},{255,255,255}}, {{0,0,0},{15,15,15},{40,40,40},{80,80,80},{140,140,140},{220,220,220}}, {0,0,0}, {255,255,255},
   T4_CITY, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 60, 40, 0, 1, W4_STARS, 2, 0, 0},                                     // 2 Searchlight
  {{{0,0,0},{10,10,15},{40,40,50},{100,100,110}}, {{150,150,160},{255,255,255}}, {{0,0,0},{20,20,25},{50,50,60},{100,100,110},{170,170,180},{245,245,250}}, {10,10,15}, {255,255,255},
   T4_MAP, S4_EDGES, X4_SLABS | X4_SHARDS, 30, 70, 35, 0, 1, W4_STARS, 4, 26, 130},                                  // 3 Full Moon
  {{{0,0,0},{30,0,5},{90,10,20},{180,30,40}}, {{150,0,20},{255,220,220}}, {{0,0,0},{20,5,8},{50,10,15},{120,15,25},{200,30,40},{255,240,240}}, {10,0,0}, {255,30,40},
   T4_CITY, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 50, 50, 0, 1, 0, 1 | 32, 0, 0},                                       // 4 Red Lipstick
  {{{0,0,0},{20,20,20},{80,80,80},{180,180,180}}, {{100,100,100},{255,255,255}}, {{0,0,0},{25,25,25},{60,60,60},{110,110,110},{180,180,180},{250,250,250}}, {15,15,15}, {255,255,255},
   T4_MAP, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 60, 30, 0, 1, 0, 8, 0, 0},                                             // 5 Venetian
  {{{60,40,20},{120,85,50},{170,130,85},{200,165,120}}, {{120,80,40},{255,230,180}}, {{60,45,30},{100,75,50},{140,105,70},{170,135,95},{200,165,125},{230,200,165}}, {80,60,40}, {40,25,15},
   T4_DUNES, 0, X4_SLABS | X4_SHARDS, 0, 40, 85, 0, 1, 0, 16 | 4, 20, 150},                                         // 6 Dust Bowl
  {{{0,0,0},{15,15,20},{50,50,60},{110,110,120}}, {{90,90,100},{255,255,255}}, {{0,0,0},{15,15,18},{35,35,40},{70,70,80},{130,130,140},{220,220,230}}, {0,0,0}, {255,220,120},
   T4_CITY, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 85, 75, 0, 2, 0, 1 | 2 | 32, 0, 0},                                   // 7 Gotham
  {{{0,0,5},{5,5,20},{20,20,45},{50,50,80}}, {{90,90,120},{255,255,255}}, {{0,0,0},{10,10,15},{25,25,35},{50,50,65},{100,100,120},{200,200,220}}, {0,0,10}, {255,240,180},
   T4_CITY, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 60, 45, 0, 1, W4_STARS, 4 | 32, 14, 140},                             // 8 Skyline
  {{{0,0,0},{30,30,30},{110,110,110},{220,220,220}}, {{130,130,130},{255,255,255}}, {{20,20,20},{60,60,60},{110,110,110},{160,160,160},{210,210,210},{255,255,255}}, {0,0,0}, {200,0,0},
   T4_MESA, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 60, 30, 0, 1, 0, 4, 22, 110},                                         // 9 Desert Noir
  {{{0,0,0},{40,0,10},{140,10,30},{255,240,230}}, {{200,0,30},{255,255,255}}, {{0,0,0},{40,40,40},{100,100,100},{200,200,200},{255,255,255},{255,40,60}}, {0,0,0}, {255,20,40},
   T4_SPIKES, S4_EDGES | S4_CONTOUR, X4_SLABS | X4_SHARDS, 0, 90, 25, 0, 2, 0, 8, 0, 0},                             // 10 Vertigo
};
static const char* const nr_presetNames[12] = {
  "Rain City", "Docks", "Searchlight", "Full Moon", "Red Lipstick", "Venetian", "Dust Bowl", "Gotham", "Skyline", "Desert Noir", "Vertigo", "Tour"
};
const char* prog_noir_name() { return "1930s NOIR"; }
const char* prog_noir_character() { return "The 1930s: film noir in black, white and red, rain, searchlights and blinds; EXPLODE pops the flashbulb"; }
const char* prog_noir_presetName(int preset) { return (preset >= 0 && preset < 12) ? nr_presetNames[preset] : NULL; }
const char* prog_noir_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Rain";
  if (pot == 9) return "Moon";
  if (pot == 11) return "Blinds";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_noir_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_noir_init() { h4_init(nr_worlds, 11); }

void prog_noir_draw(int preset) {
  if (!h4_prepare(preset, 6.0f, 1.6f, 1.0f)) return;              // hard light
  const H4World& wp = nr_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float rain = v3_centre(7) ? ((deco & 1) ? 0.6f : 0.0f) : v3_u(7);
  float blinds = v3_centre(11) ? ((deco & 8) ? 0.8f : 0.0f) : v3_u(11);

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(40 * h4k.skyK), hzY, 31);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_glow(240, hzY - wp.sunY / 2, R * 2, 6); h4_sun(240, hzY - wp.sunY / 2, R, false); }
  if (deco & 2) {
    for (int i = 0; i < 4; i++) {                                  // searchlights crossing in the sky
      float a = -1.5708f + 0.6f * sinf(h4_t * (0.3f + i * 0.11f) + i * 1.7f);
      int bx = 40 + i * 80, by = hzY + 20;
      int32_t xs[3] = {bx * 16, (int32_t)((bx + cosf(a - 0.04f) * 300) * 16), (int32_t)((bx + cosf(a + 0.04f) * 300) * 16)};
      int32_t ys[3] = {by * 16, (int32_t)((by + sinf(a - 0.04f) * 300) * 16), (int32_t)((by + sinf(a + 0.04f) * 300) * 16)};
      v3_poly(xs, ys, 3, H4_ACC + 7);
    }
  }
  if (E > 0.02f) { h4_glow(160, hzY, (int)(30 + E * 140), (int)(9 + E * 7)); }   // the flashbulb

  // ── ground ──
  h4_ground(hzY, h4_rollPx);

  // red windows in the city at night (the one colour noir allows)
  if (deco & 32) for (int i = 0; i < 70; i++) { uint32_t h = v3_hash(i * 7717u + (uint32_t)(h4_head * 30) * 3u); int x = h % W, y = hzY + (int)((h >> 9) % (uint32_t)(H - hzY)); uint8_t* p = v3_buf + y * W + x; if (*p >= 64 && ((*p - 64) >> 4) > 2 && ((*p - 64) >> 4) < 11) { *p = h4_pix(11, 0, 3); if (x + 1 < W) p[1] = h4_pix(11, 0, 3); } }
  // the dust storm: brown grain blowing sideways
  if (deco & 16) for (int i = 0; i < 400; i++) { uint32_t r = v3_rand(); int x = (int)((r % W + (uint32_t)(h4_t * 200)) % W), y = (r >> 9) % H; v3_put(x, y, H4_ACC + 4 + ((r >> 20) & 3)); }

  // ── explosion: the blinds slide apart and glass flies ──
  if (E > 0.02f) {
    for (int y = 0; y < H; y++) {                                  // horizontal blinds sliding left and right
      int band = y / 12;
      int dx = (int)(E * 60 * ((band & 1) ? 1 : -1) * (0.3f + ((v3_hash(band * 31u) & 255) / 255.0f)));
      if ((y % 12) < 2) memset(v3_buf + y * W, H4_ACC + 15, W);
      else h4_shiftRow(y, dx);
    }
  }
  if (h4_trigger) { h4_burstAt(160, (float)hzY, 50, 260, H4_ACC + 15, P4_SHARD, 6, 30); h4_burstAt(160, (float)hzY, 4, 10, H4_ACC + 15, P4_RING, 30, 0); }
  int emit = (int)(E * 3 + (v3_rndf() < E * 3 - (int)(E * 3) ? 1 : 0));
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 100 + v3_rndf() * 200; h4_emit(160, (float)hzY, cosf(a) * s, sinf(a) * s * 0.8f, 0.6f + v3_rndf(), (uint8_t)(v3_rnd(5) ? H4_ACC + 15 : h4_pix(11, 0, 3)), P4_SHARD, (uint8_t)(3 + v3_rnd(7))); }
  h4_parts(dt, 60.0f, 0.2f);

  // ── rain and blinds ──
  if (rain > 0.02f) {
    int n = (int)(rain * 260);
    for (int i = 0; i < n; i++) { uint32_t r = v3_hash(i * 977u + (uint32_t)(h4_t * 30) * 7919u); int x = r % (W + 40) - 20, y = (r >> 9) % H; v3_line(x, y, x - 3, y + 9, H4_ACC + 9); }
  }
  if (blinds > 0.02f) {                                            // venetian-blind shadows: darker bands across everything
    int period = 14, dark = 2 + (int)(blinds * 7);
    for (int y = 0; y < H; y++) {
      if (((y + (int)(h4_t * 4)) % period) >= dark) continue;
      uint8_t* p = v3_buf + y * W;
      for (int x = 0; x < W; x++) { uint8_t v = p[x]; if (v >= 64) p[x] = (uint8_t)(v & ~3); else if (v < 40) p[x] = (uint8_t)(v > 8 ? v - 8 : 0); }   // in shadow: the darkest light
    }
  }
}

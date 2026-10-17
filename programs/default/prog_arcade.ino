// =====================================================================
// PROGRAM: ARCADE — the 1980s (by Dewey) — Set 4, slot 6
//
// The landscape flight from HORIZON, as the 1980s saw it: neon grids under
// a striped sun, Tron wireframes, green vector monitors, chunky 8-bit hills,
// Miami sunsets and rainy neon cities — with scanlines and big pixels.
//
// EXPLODE (p15, or k12 for one blast): the horizon disintegrates into
// pixels — the land breaks into square blocks that fly apart, layer by
// layer, and a shower of 8-bit sparks bursts out of the vanishing point.
//
// Presets: k0 Outrun · k1 Tron · k2 Vector · k3 8-Bit · k4 Miami · k5 Neon City
//          k6 Galaxian · k7 Rad Desert · k8 Blade Night · k9 Cabinet · k10 Chrome · k11 Tour
//
// Knobs: p0 Palette (centre = preset) · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains
//        p6 Water Level · p7 Scanlines · p8 Fog · p9 Sun · p10 Draw Distance · p11 Pixel Size
//        p12 Sky · p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

static const H4World ar_worlds[11] = {
  // sky top → horizon                                        accent dark → bright        ground low → high                                                                water          line
  {{{10,0,40},{70,0,110},{200,30,150},{255,110,140}}, {{255,0,140},{255,240,60}}, {{30,0,70},{40,5,90},{60,10,120},{90,20,150},{130,30,170},{170,60,200}}, {20,0,60}, {255,0,220},
   T4_MAP, S4_GRID, X4_BLOCKS | X4_SLABS, 0, 45, 35, 0, 2, W4_STARS | W4_STRIPE, 0, 46, 90},                       // 0 Outrun
  {{{0,0,0},{0,0,10},{0,10,30},{0,40,70}}, {{0,60,255},{0,255,255}}, {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}, {0,0,0}, {0,255,255},
   T4_MAP, S4_WIRE, X4_BLOCKS | X4_SLABS, 0, 30, 30, 0, 1, W4_STARS, 0, 0, 0},                                       // 1 Tron
  {{{0,0,0},{0,0,0},{0,8,0},{0,30,0}}, {{0,90,0},{120,255,120}}, {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}, {0,0,0}, {60,255,60},
   T4_SPIKES, S4_WIRE | S4_EDGES, X4_BLOCKS, 0, 90, 20, 0, 1, W4_STARS, 1, 12, 30},                                // 2 Vector
  {{{60,100,255},{90,140,255},{140,190,255},{190,220,255}}, {{255,255,255},{255,255,255}}, {{0,170,0},{60,200,40},{0,120,0},{150,90,40},{200,140,80},{255,255,255}}, {40,90,250}, {0,0,0},
   T4_TERRACE, S4_EDGES, X4_BLOCKS | X4_SLABS, 50, 70, 25, 28, 4, 0, 2, 0, 0},                                       // 3 8-Bit
  {{{30,20,90},{230,80,160},{255,150,120},{255,210,140}}, {{255,90,90},{255,250,170}}, {{0,160,170},{40,200,190},{250,120,170},{255,170,200},{255,220,230},{255,255,255}}, {0,120,170}, {255,90,200},
   T4_MAP, S4_GRID, X4_BLOCKS | X4_SLABS, 90, 45, 35, 0, 2, W4_STRIPE, 0, 40, 70},                                   // 4 Miami
  {{{5,0,20},{30,0,60},{80,0,110},{150,20,140}}, {{0,255,255},{255,255,255}}, {{20,0,40},{40,0,80},{70,0,120},{0,120,170},{0,200,220},{120,255,255}}, {10,0,30}, {255,0,200},
   T4_CITY, S4_GRID | S4_EDGES, X4_BLOCKS | X4_SLABS, 0, 70, 40, 0, 2, W4_STARS, 0, 0, 0},                           // 5 Neon City
  {{{0,0,0},{0,0,10},{0,0,25},{20,10,50}}, {{120,40,200},{255,220,120}}, {{30,30,40},{70,70,90},{110,110,140},{150,150,190},{200,200,230},{255,255,255}}, {0,0,0}, {255,60,60},
   T4_INVERT, S4_EDGES, X4_BLOCKS | X4_SLABS, 0, 40, 15, 0, 2, W4_STARS, 3, 30, 100},                                // 6 Galaxian
  {{{0,30,80},{0,120,160},{255,140,60},{255,210,120}}, {{255,60,0},{255,255,120}}, {{120,40,20},{200,90,40},{240,140,60},{255,180,90},{255,210,140},{255,240,200}}, {0,0,0}, {0,255,230},
   T4_DUNES, S4_GRID, X4_BLOCKS | X4_SLABS, 0, 50, 30, 0, 2, W4_STRIPE, 0, 44, 80},                                  // 7 Rad Desert
  {{{0,0,0},{20,10,20},{90,40,30},{220,120,40}}, {{255,90,0},{255,230,150}}, {{10,10,15},{20,20,30},{40,30,40},{60,40,40},{120,60,40},{255,140,40}}, {10,10,20}, {0,220,255},
   T4_CITY, S4_EDGES, X4_BLOCKS | X4_SLABS, 0, 90, 70, 0, 2, 0, 4, 0, 0},                                            // 8 Blade Night
  {{{0,0,0},{0,0,120},{0,0,200},{60,60,255}}, {{255,0,0},{255,255,0}}, {{255,0,0},{255,160,0},{255,255,0},{0,220,0},{0,180,255},{255,255,255}}, {0,0,180}, {255,255,255},
   T4_TERRACE, S4_EDGES, X4_BLOCKS | X4_SLABS, 40, 80, 20, 40, 4, W4_STARS, 0, 30, 70},                              // 9 Cabinet
  {{{20,0,50},{90,40,160},{200,140,230},{255,230,255}}, {{255,100,255},{255,255,255}}, {{40,20,80},{90,60,140},{160,120,200},{220,200,240},{255,255,255},{200,220,255}}, {60,30,100}, {255,255,255},
   T4_SPIKES, S4_EDGES | S4_CONTOUR, X4_BLOCKS | X4_SLABS, 0, 100, 25, 0, 2, W4_STRIPE, 0, 60, 70},                  // 10 Chrome
};
static const char* const ar_presetNames[12] = {
  "Outrun", "Tron", "Vector", "8-Bit", "Miami", "Neon City", "Galaxian", "Rad Desert", "Blade Night", "Cabinet", "Chrome", "Tour"
};
const char* prog_arcade_name() { return "1980s ARCADE"; }
const char* prog_arcade_character() { return "The 1980s: neon grids, striped suns, vector monitors and 8-bit hills; EXPLODE scatters the land into pixels"; }
const char* prog_arcade_presetName(int preset) { return (preset >= 0 && preset < 12) ? ar_presetNames[preset] : NULL; }
const char* prog_arcade_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Scanlines";
  if (pot == 11) return "Pixel Size";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_arcade_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_arcade_init() { h4_init(ar_worlds, 11); }

void prog_arcade_draw(int preset) {
  if (!h4_prepare(preset, 10.0f, 1.0f, 1.0f)) return;
  const H4World& w = ar_worlds[h4_world];
  const H4World& wp = ar_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  // era knobs
  bool scan = v3_centre(7) ? true : v3_k(7) > 512;
  if (!v3_centre(11)) { int k = v3_k(11); h4c.pixW = k < 340 ? 1 : (k < 680 ? 2 : 4); }

  // ── sky ──
  int hzY = h4_hzY;
  h4_sky(hzY, 110);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(70 * h4k.skyK), hzY, 5);
  if (wp.deco == 3 && h4k.skyK > 0.1f) {                            // Galaxian: a ringed planet
    int R = (int)(26 * h4k.sunK); int px = 80 + (int)(sinf(h4_head) * 20), py = hzY - 50;
    v3_disc(px, py, R, H4_ACC + 9); v3_disc(px - R / 4, py - R / 4, R * 2 / 3, H4_ACC + 12);
    for (int a = 0; a < 1024; a += 4) { int x = px + ((v3_cos(a) * R * 2) >> 14), y = py + ((v3_sin(a) * R / 3) >> 14); if (a > 512 || x < px - R || x > px + R) v3_put(x, y, H4_ACC + 15); }
  }
  if (wp.sunR) h4_sun(160, hzY - wp.sunY / 3, (int)(wp.sunR * h4k.sunK * (1.0f + E * 0.3f)), wp.flags & W4_STRIPE);
  if (wp.deco == 1 && h4k.skyK > 0.1f) {                            // Vector: a wireframe moon
    int R = (int)(14 * h4k.sunK), mx = 240, my = 40;
    for (int a = 0; a < 1024; a += 8) v3_put(mx + ((v3_cos(a) * R) >> 14), my + ((v3_sin(a) * R) >> 14), H4_ACC + 15);
    for (int k = -2; k <= 2; k++) for (int a = 0; a < 1024; a += 16) v3_put(mx + ((v3_cos(a) * R * (5 - (k < 0 ? -k : k)) / 5) >> 14) , my + k * R / 3, H4_ACC + 10);
  }
  if (wp.deco == 2 && h4k.skyK > 0.1f) {                            // 8-Bit: blocky clouds
    for (int i = 0; i < 5; i++) {
      int cx = (int)(i * 90 + 30 - fmodf(h4_head * 80 + h4_t * 6, 450.0f)); while (cx < -60) cx += 450;
      int cy = 20 + (i * 37) % 50;
      for (int k = 0; k < 3; k++) v3_rect(cx + k * 12 - 8, cy - (k == 1 ? 8 : 4), cx + k * 12 + 8, cy + 6, H4_ACC + 15);
    }
  }
  if (wp.deco == 4) {                                               // Blade Night: rain
    for (int i = 0; i < 140; i++) { uint32_t r = v3_hash(i * 977u + (uint32_t)(h4_t * 40) * 13u); int x = r % W, y = (r >> 9) % H; v3_line(x, y, x - 2, y + 7, H4_ACC + 6); }
  }
  // the core of the explosion: a white-hot glow at the vanishing point, seen through the cracks
  if (E > 0.02f) h4_glow(160, hzY, (int)(30 + E * 90), (int)(6 + E * 10));

  // ── ground ──
  h4_ground(hzY, h4_rollPx);

  // ── explosion: 8-bit sparks ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 70, 220, H4_ACC + 15, P4_SQUARE, 4, 40);
  int emit = (int)(E * E * 6 + (v3_rndf() < E * 3 - (int)(E * 3) ? 1 : 0));
  for (int i = 0; i < emit; i++) {
    float a = v3_rndf() * 6.2831853f, s = 80 + v3_rndf() * 200;
    uint8_t c = (uint8_t)(i & 1 ? 64 + 11 * 16 + 3 : H4_ACC + 10 + v3_rnd(6));
    h4_emit(160 + cosf(a) * 10, (float)hzY + sinf(a) * 6, cosf(a) * s, sinf(a) * s * 0.6f - 30, 0.8f + v3_rndf(), c, P4_SQUARE, (uint8_t)(h4c.pixW + 1 + v3_rnd(3)));
  }
  h4_parts(dt, 140.0f, 0.4f);
  if (scan) h4_scanlines();
  (void)w;
}

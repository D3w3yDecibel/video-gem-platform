// =====================================================================
// PROGRAM: TECHNICOLOR — the 1940s (by Dewey) — Set 4, slot 2
//
// The landscape flight in glorious three-strip Technicolor: saturated
// painted hills, a yellow brick road winding over them, poppy fields,
// matte-painted skies full of puffy clouds, a rainbow, wartime poster
// colours, jungles, tropics and an emerald city on the horizon.
//
// EXPLODE (p15, or k12 for one blast): a cyclone. The land twists round
// into a spiral, a funnel cloud reaches down to the horizon and the
// countryside — fences, roofs, a cow or two — whirls up into the sky.
//
// Presets: k0 Brick Road · k1 Poppy Field · k2 Kansas · k3 Victory · k4 Jungle · k5 Hollywood Hills
//          k6 Fantasia · k7 Tropics · k8 Painted Desert · k9 Rainbow · k10 Emerald City · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Clouds · p8 Fog · p9 Sun · p10 Draw Distance · p11 Rainbow · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 clouds · 2 rainbow · 4 sun · 8 stars · 16 sepia (Kansas)
static const H4World tc_worlds[11] = {
  {{{20,80,220},{60,140,240},{140,200,255},{210,235,255}}, {{255,200,0},{255,255,220}}, {{20,120,30},{40,170,40},{80,200,50},{120,220,70},{170,230,100},{220,250,160}}, {30,90,200}, {255,220,0},
   T4_ROLLING, S4_ROAD, X4_TWIST, 0, 45, 30, 0, 1, 0, 1 | 4, 26, 110},                                               // 0 Brick Road
  {{{40,90,220},{90,150,240},{170,210,255},{235,240,255}}, {{255,220,120},{255,255,240}}, {{30,110,30},{50,150,40},{70,170,50},{100,190,60},{130,200,80},{170,220,110}}, {30,80,190}, {235,20,30},
   T4_ROLLING, S4_FLOWERS, X4_TWIST, 0, 35, 30, 0, 1, 0, 1, 0, 0},                                                   // 1 Poppy Field
  {{{110,100,80},{160,145,115},{205,190,155},{235,220,190}}, {{150,120,70},{255,245,210}}, {{120,95,50},{160,130,70},{190,160,90},{210,180,110},{225,200,135},{240,220,170}}, {90,80,60}, {80,60,30},
   T4_ROLLING, 0, X4_TWIST, 0, 20, 45, 0, 1, 0, 1 | 16, 0, 0},                                                       // 2 Kansas
  {{{0,20,100},{0,50,160},{190,210,240},{255,255,255}}, {{220,0,20},{255,255,255}}, {{180,0,20},{220,20,30},{240,240,240},{255,255,255},{30,60,170},{10,30,120}}, {20,40,140}, {255,255,255},
   T4_MAP, S4_EDGES, X4_TWIST, 40, 55, 30, 0, 1, 0, 4, 30, 90},                                                      // 3 Victory
  {{{60,20,80},{200,60,60},{255,150,40},{255,220,120}}, {{255,90,0},{255,255,180}}, {{0,40,20},{0,80,30},{10,120,40},{40,150,40},{90,170,50},{160,200,90}}, {20,80,60}, {255,120,0},
   T4_MAP, 0, X4_TWIST, 70, 60, 40, 0, 1, 0, 4, 40, 80},                                                             // 4 Jungle
  {{{70,60,150},{220,130,110},{255,190,110},{255,230,170}}, {{255,140,40},{255,250,210}}, {{90,70,40},{140,110,50},{190,150,60},{220,180,80},{240,210,120},{255,235,170}}, {60,60,120}, {255,240,200},
   T4_ROLLING, 0, X4_TWIST, 0, 55, 35, 0, 1, 0, 1 | 4, 34, 50},                                                      // 5 Hollywood Hills
  {{{10,0,50},{50,10,120},{130,50,190},{220,140,240}}, {{255,120,255},{255,255,255}}, {{20,0,60},{50,10,110},{90,30,160},{40,90,200},{80,160,230},{200,230,255}}, {30,0,90}, {255,255,120},
   T4_MAP, S4_FLOWERS, X4_TWIST, 40, 60, 30, 0, 1, W4_GLOW, 8, 0, 0},                                                // 6 Fantasia
  {{{0,110,220},{40,170,240},{140,220,250},{220,245,255}}, {{255,230,120},{255,255,240}}, {{230,210,150},{60,170,60},{30,140,50},{20,110,40},{90,120,70},{150,150,110}}, {0,180,200}, {255,255,255},
   T4_MAP, 0, X4_TWIST, 110, 70, 30, 0, 1, 0, 1 | 4, 20, 100},                                                       // 7 Tropics
  {{{60,90,180},{160,170,210},{250,190,140},{255,220,170}}, {{230,90,30},{255,240,190}}, {{150,50,30},{190,80,40},{220,120,60},{240,160,90},{250,200,140},{255,230,190}}, {0,0,0}, {255,255,255},
   T4_MESA, S4_EDGES, X4_TWIST, 0, 60, 35, 0, 1, 0, 1 | 4, 24, 90},                                                  // 8 Painted Desert
  {{{30,60,200},{80,140,240},{170,210,255},{235,245,255}}, {{255,240,180},{255,255,255}}, {{230,30,40},{255,140,0},{255,230,0},{40,190,60},{40,100,230},{150,60,210}}, {40,80,200}, {255,255,255},
   T4_ROLLING, S4_CONTOUR, X4_TWIST, 0, 45, 30, 0, 1, 0, 1 | 2, 0, 0},                                               // 9 Rainbow
  {{{0,40,30},{0,100,70},{100,200,140},{200,255,210}}, {{0,200,90},{220,255,230}}, {{0,60,30},{0,110,50},{0,160,70},{40,200,100},{120,230,150},{220,255,230}}, {0,80,50}, {255,230,0},
   T4_CITY, S4_EDGES | S4_ROAD, X4_TWIST, 0, 45, 35, 0, 1, 0, 4 | 8, 30, 80},                                       // 10 Emerald City
};
static const char* const tc_presetNames[12] = {
  "Brick Road", "Poppy Field", "Kansas", "Victory", "Jungle", "Hollywood Hills", "Fantasia", "Tropics", "Painted Desert", "Rainbow", "Emerald City", "Tour"
};
const char* prog_techni_name() { return "1940s TECHNICOLOR"; }
const char* prog_techni_character() { return "The 1940s: painted Technicolor hills, brick roads, clouds and rainbows; EXPLODE whips up a cyclone"; }
const char* prog_techni_presetName(int preset) { return (preset >= 0 && preset < 12) ? tc_presetNames[preset] : NULL; }
const char* prog_techni_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Clouds";
  if (pot == 11) return "Rainbow";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_techni_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_techni_init() { h4_init(tc_worlds, 11); }

static float tc_spin = 0;
void prog_techni_draw(int preset) {
  if (!h4_prepare(preset, 7.0f, 0.9f, 0.8f)) return;
  const H4World& wp = tc_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float clouds = v3_centre(7) ? ((deco & 1) ? 0.6f : 0.0f) : v3_u(7);
  float rainbow = v3_centre(11) ? ((deco & 2) ? 1.0f : 0.0f) : v3_u(11);
  tc_spin += dt * (0.3f + E * 3.0f);

  // ── sky: a matte painting ──
  h4_sky(hzY, 130);
  if ((deco & 8) && h4k.skyK > 0.1f) h4_stars((int)(80 * h4k.skyK), hzY, 41);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_glow(200, hzY - wp.sunY / 2, R * 2, 7); h4_sun(200, hzY - wp.sunY / 2, R, false); }
  if (rainbow > 0.05f) {                                            // a rainbow arc over the land
    static const uint8_t rb[6][3] = {{255,30,30},{255,150,0},{255,240,0},{40,220,60},{40,110,255},{160,60,230}};
    for (int b = 0; b < 6; b++) h4_prop(b, rb[b][0], rb[b][1], rb[b][2]);
    int cx = 160 + (int)(sinf(h4_head) * 60), R0 = 150;
    for (int b = 0; b < 6; b++) {
      int R = R0 - b * 5;
      for (int a = 512 + 40; a < 1024 - 40; a += 2) {
        int x = cx + ((v3_cos(a) * R) >> 14), y = hzY + 30 + ((v3_sin(a) * R * 7 / 10) >> 14);
        if ((v3_hash((uint32_t)(a * 7 + b)) & 255) < (uint32_t)(rainbow * 255)) for (int k = 0; k < 5; k++) v3_put(x, y + k, (uint8_t)(H4_PROP + b));
      }
    }
  }
  if (clouds > 0.02f && h4k.skyK > 0.05f) {                        // puffy painted clouds drifting by
    int n = 2 + (int)(clouds * 6);
    for (int i = 0; i < n; i++) {
      float x = fmodf(i * 113.0f - h4_head * 90.0f - h4_t * (4 + i), 480.0f); if (x < 0) x += 480; x -= 80;
      int y = 18 + (i * 29) % (hzY > 50 ? hzY - 30 : 20);
      h4_cloud((int)x, y, 9 + (i % 3) * 3, H4_ACC + 15, H4_ACC + 9);
    }
  }
  // the cyclone: a dark funnel twisting down to the horizon
  if (E > 0.05f) {
    h4_prop(6, 70, 65, 75); h4_prop(7, 120, 115, 125);             // storm greys
    for (int y = 0; y < hzY + 4; y++) {
      float u = (float)y / (hzY + 4);
      int hw = (int)((8 + 70 * (1 - u) * (1 - u)) * (0.4f + E));
      int cx = 160 + (int)(sinf(tc_spin * 2 + u * 6) * 14 * u);
      for (int x = cx - hw; x <= cx + hw; x++) if ((unsigned)x < W) v3_buf[y * W + x] = (uint8_t)(H4_PROP + (((x + y * 2 + (int)(tc_spin * 60)) & 15) < 7 ? 6 : 7));   // banded, turning
    }
  }

  // ── ground (twisted by the cyclone; the whole view rocks in the wind) ──
  h4_ground(hzY, h4_rollPx + (int)(sinf(tc_spin * 1.3f) * E * 70));
  if (E > 0.25f) {                                                  // the funnel touches down: its foot whirls over the land
    int foot = (int)((E - 0.25f) * 110);
    for (int y = hzY; y < hzY + foot && y < H; y++) {
      float u = (float)(y - hzY) / (foot + 1);
      int hw = (int)((8 + 40 * u) * (0.4f + E)), cx = 160 + (int)(sinf(tc_spin * 2 + 6 + u * 3) * 14);
      for (int x = cx - hw; x <= cx + hw; x++) if ((unsigned)x < W && ((x * 3 + y * 5 + (int)(tc_spin * 90)) & 7) < 5) v3_buf[y * W + x] = (uint8_t)(H4_PROP + (((x + y * 2 + (int)(tc_spin * 60)) & 15) < 7 ? 6 : 7));
    }
  }

  // ── debris whirling round the funnel ──
  int emit = (int)(E * 6 + (v3_rndf() < E * 6 - (int)(E * 6) ? 1 : 0)) + (h4_trigger ? 30 : 0);
  for (int i = 0; i < emit; i++) {
    float a = v3_rndf() * 6.2831853f, r = 20 + v3_rndf() * 60;
    float x = 160 + cosf(a) * r, y = (float)hzY - v3_rndf() * 20;
    uint8_t c = (uint8_t)(64 + (1 + v3_rnd(10)) * 16 + 3);
    h4_emit(x, y, -sinf(a) * 220, -60 - v3_rndf() * 120, 1.5f + v3_rndf() * 1.5f, c, (uint8_t)(v3_rnd(3) ? P4_SQUARE : P4_SHARD), (uint8_t)(2 + v3_rnd(5)));
  }
  // particles circle the funnel: pull them sideways round its axis
  for (int i = 0; i < h4_np; i++) { H4Part* p = &h4_p[i]; float dx = p->x - 160; p->vx += (-dx * 3.0f - p->vy * 0.0f) * dt; }
  h4_parts(dt, -20.0f, 0.3f);
  if (deco & 16) h4_grain(250, H4_ACC + 15, 0, 1);
}

// =====================================================================
// PROGRAM: FUTURE — the 2030s (by Dewey) — Set 4, slot 11
//
// The landscape flight a decade from now: prism-crystal mountains, green
// solarpunk valleys, a ringworld arching overhead, a Dyson-swarm sun, ice
// moons, a hyperloop city, Mars domes, aurora-lit plains, bioluminescent
// seas, a quantum wireframe and an exoplanet under twin suns.
//
// EXPLODE (p15, or k12 for one blast): space-time tears. The far land curls
// up over your head and twists into a tunnel, stars streak past into
// hyperspace and a white rift opens on the horizon.
//
// Presets: k0 Crystal · k1 Solarpunk · k2 Ringworld · k3 Dyson Sun · k4 Ice Moon · k5 Hyperloop
//          k6 Mars Colony · k7 Aurora Prime · k8 Bioluminescent · k9 Quantum · k10 Exoplanet · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Aurora · p8 Fog · p9 Sun · p10 Draw Distance · p11 Warp Streaks · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 aurora · 2 ring arch · 4 big sun · 8 stars · 16 domes · 32 twin suns · 64 planet
static const H4World fu_worlds[11] = {
  {{{10,0,40},{60,20,120},{160,100,220},{240,200,255}}, {{140,255,255},{255,255,255}}, {{40,0,100},{0,80,180},{0,180,200},{120,60,220},{255,100,230},{200,255,255}}, {20,0,60}, {255,255,255},
   T4_FACETS, S4_EDGES, X4_FOLD | X4_TWIST, 0, 70, 25, 0, 1, W4_STARS, 8 | 64, 0, 0},                               // 0 Crystal
  {{{60,170,230},{140,210,240},{220,240,220},{250,250,230}}, {{255,240,140},{255,255,255}}, {{30,120,60},{70,170,70},{140,210,90},{240,240,220},{200,230,240},{255,255,255}}, {20,150,190}, {255,255,255},
   T4_ROLLING, S4_EDGES, X4_FOLD | X4_TWIST, 60, 45, 25, 0, 1, 0, 4, 24, 110},                                       // 1 Solarpunk
  {{{0,0,20},{20,30,80},{90,120,180},{190,210,240}}, {{200,220,255},{255,255,255}}, {{40,60,40},{70,110,60},{110,150,80},{150,170,120},{190,200,180},{235,240,240}}, {20,50,100}, {255,255,255},
   T4_MAP, 0, X4_FOLD | X4_TWIST, 40, 55, 35, 0, 1, W4_STARS, 2 | 8, 0, 0},                                          // 2 Ringworld
  {{{20,0,0},{120,30,0},{255,120,20},{255,220,120}}, {{255,90,0},{255,255,220}}, {{40,10,10},{90,30,20},{150,60,30},{200,100,40},{240,160,70},{255,220,140}}, {0,0,0}, {255,240,180},
   T4_MESA, S4_EDGES, X4_FOLD | X4_TWIST, 0, 35, 35, 0, 1, 0, 4, 70, 60},                                            // 3 Dyson Sun
  {{{0,0,10},{0,10,40},{30,60,110},{120,160,210}}, {{170,220,255},{255,255,255}}, {{20,40,90},{50,80,140},{90,130,190},{140,180,230},{200,225,250},{255,255,255}}, {20,40,90}, {120,220,255},
   T4_SPIKES, S4_EDGES, X4_FOLD | X4_TWIST, 0, 85, 20, 0, 1, W4_STARS, 8 | 64, 0, 0},                                 // 4 Ice Moon
  {{{0,0,15},{10,10,40},{40,30,90},{110,60,140}}, {{0,255,220},{255,255,255}}, {{10,10,30},{30,30,70},{60,60,120},{100,100,170},{0,200,220},{220,255,255}}, {0,0,20}, {0,255,200},
   T4_CITY, S4_GRID | S4_EDGES, X4_FOLD | X4_TWIST, 0, 55, 35, 0, 1, W4_STARS, 8, 0, 0},                              // 5 Hyperloop
  {{{80,30,20},{170,80,50},{220,140,100},{240,190,150}}, {{255,200,150},{255,255,240}}, {{110,40,20},{160,60,30},{200,90,50},{220,120,70},{235,160,110},{250,210,170}}, {0,0,0}, {220,240,255},
   T4_MESA, S4_EDGES, X4_FOLD | X4_TWIST, 0, 35, 40, 0, 1, 0, 16 | 4, 14, 120},                                      // 6 Mars Colony
  {{{0,0,10},{0,20,40},{10,60,80},{40,110,120}}, {{40,255,140},{220,100,255}}, {{20,40,60},{40,70,90},{70,100,120},{110,140,160},{170,190,210},{235,245,255}}, {10,30,60}, {150,255,200},
   T4_MAP, 0, X4_FOLD | X4_TWIST, 40, 50, 35, 0, 1, W4_STARS, 1 | 8, 0, 0},                                          // 7 Aurora Prime
  {{{0,0,5},{0,10,20},{0,30,40},{0,60,70}}, {{0,255,200},{200,255,255}}, {{0,15,20},{0,30,35},{10,50,50},{20,70,60},{40,90,70},{70,120,90}}, {0,255,200}, {0,255,255},
   T4_MAP, S4_CONTOUR, X4_FOLD | X4_TWIST, 60, 50, 35, 0, 1, W4_GLOW | W4_STARS, 8, 0, 0},                            // 8 Bioluminescent
  {{{5,0,20},{20,0,50},{60,10,100},{120,40,170}}, {{255,0,255},{255,255,255}}, {{15,0,40},{25,0,60},{35,5,80},{45,10,100},{60,15,120},{80,20,140}}, {20,0,50}, {200,140,255},
   T4_MAP, S4_WIRE, X4_FOLD | X4_TWIST, 0, 55, 15, 0, 1, W4_STARS, 8, 0, 0},                                // 9 Quantum
  {{{40,0,60},{120,20,90},{220,90,90},{255,180,120}}, {{255,120,40},{255,255,220}}, {{20,60,80},{30,110,110},{60,160,120},{180,60,140},{230,110,170},{255,200,220}}, {120,20,120}, {255,240,120},
   T4_MAP, 0, X4_FOLD | X4_TWIST, 40, 60, 30, 0, 1, 0, 32 | 64, 20, 70},                                             // 10 Exoplanet
};
static const char* const fu_presetNames[12] = {
  "Crystal", "Solarpunk", "Ringworld", "Dyson Sun", "Ice Moon", "Hyperloop", "Mars Colony", "Aurora Prime", "Bioluminescent", "Quantum", "Exoplanet", "Tour"
};
const char* prog_future_name() { return "2030s FUTURE"; }
const char* prog_future_character() { return "The 2030s: crystal mountains, solarpunk valleys, ringworlds and twin suns; EXPLODE tears space-time open"; }
const char* prog_future_presetName(int preset) { return (preset >= 0 && preset < 12) ? fu_presetNames[preset] : NULL; }
const char* prog_future_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Aurora";
  if (pot == 11) return "Warp Streaks";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_future_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_future_init() { h4_init(fu_worlds, 11); }

void prog_future_draw(int preset) {
  if (!h4_prepare(preset, 9.0f, 1.1f, 0.9f)) return;
  const H4World& wp = fu_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float aurora = v3_centre(7) ? ((deco & 1) ? 1.0f : 0.0f) : v3_u(7) * 1.5f;
  float streaks = v3_centre(11) ? 0.0f : v3_u(11);
  h4_prop(0, 255, 255, 255); h4_prop(1, 200, 230, 255); h4_prop(2, 120, 160, 220); h4_prop(3, 60, 80, 120);   // ring, domes
  h4_prop(4, 40, 255, 160); h4_prop(5, 120, 200, 255); h4_prop(6, 220, 100, 255); h4_prop(7, 255, 255, 200);  // aurora, streaks

  // ── sky ──
  h4_sky(hzY, 130);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(80 * h4k.skyK), hzY, 121);
  if (deco & 2) {                                                   // a ringworld arching over the sky
    for (int k = 0; k < 6; k++) for (int x = 0; x < W; x++) { int dx = x - 160; int y = hzY - 95 + (dx * dx) / 300 + k; if ((unsigned)y < (unsigned)hzY) v3_put(x, y, (uint8_t)(H4_PROP + (k < 2 ? 0 : (k < 4 ? 1 : 2)))); }
  }
  if (deco & 64) {                                                  // a big planet low in the sky
    int R = (int)(34 * h4k.sunK), px = 70 + (int)(sinf(h4_head) * 30), py = hzY - 30;
    v3_disc(px, py, R, H4_ACC + 6); v3_disc(px - R / 3, py - R / 3, R * 3 / 4, H4_ACC + 9);
    for (int k = -2; k <= 2; k++) v3_hspan(px - R + 4, px + R - 4, py + k * R / 3, H4_ACC + 4);
  }
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_glow(160, hzY - wp.sunY / 3, R * 2, 7); h4_sun(160, hzY - wp.sunY / 3, R, false); }
  if (deco & 32) { h4_sun(120, hzY - 40, (int)(14 * h4k.sunK), false); h4_sun(210, hzY - 26, (int)(9 * h4k.sunK), false); }
  if (aurora > 0.02f && h4k.skyK > 0.05f) {                         // aurora curtains
    int ta = (int)(h4_t * 163), ha = (int)(h4_head * 326);
    for (int x = 0; x < W; x++) {
      int u = x * 3 + ha;
      int top = 15 + ((14 * v3_sin(u * 13 / 10 + ta * 4 / 10) + 9 * v3_sin(u * 31 / 10 - ta * 7 / 10)) >> 14);
      int len = (int)(((26 << 14) + 20 * v3_sin(u * 23 / 10 + ta / 2)) * aurora) >> 14;
      for (int y = top; y < top + len && y < hzY - 2; y++) { if (y < 0) continue; int k = (y - top) * 3 / (len + 1); if (((x * 5 + (int)(h4_t * 9)) & 7) < 3 && k == 0) continue; v3_put(x, y, (uint8_t)(H4_PROP + 4 + (k == 0 ? 0 : (k == 1 ? 1 : 2)))); }
    }
  }
  if (E > 0.02f) {                                                  // the rift: a white tear on the horizon
    h4_glow(160, hzY, (int)(15 + E * 45), (int)(6 + E * 6));
    int hw = (int)(E * 150), th = (int)(1 + E * 5);
    for (int x = 160 - hw; x <= 160 + hw; x++) { int t = th - v3_abs(x - 160) * th / (hw + 1); for (int k = -t; k <= t; k++) v3_put(x, hzY + k, H4_PROP + 0); }
  }

  // ── ground: folding up and twisting into a tunnel ──
  h4_ground(hzY, h4_rollPx);

  if (deco & 16) {                                                  // Mars domes on the near ground
    for (int i = 0; i < 3; i++) {
      int x = (int)(fmodf(i * 110.0f - h4_head * 150 + 640, 360.0f)) - 20, y = H - 20 - i * 18, R = 18 - i * 4;
      for (int a = 512; a < 1024; a += 6) v3_put(x + ((v3_cos(a) * R) >> 14), y + ((v3_sin(a) * R) >> 14), H4_PROP + 1);
      v3_hspan(x - R, x + R, y, H4_PROP + 2);
      v3_disc(x - R / 3, y - R / 2, 2, H4_PROP + 0);
    }
  }
  // ── hyperspace streaks ──
  float st = streaks + E;
  if (h4_trigger) for (int i = 0; i < 60; i++) { float a = v3_rndf() * 6.2831853f, s = 300 + v3_rndf() * 300; h4_emit(160 + cosf(a) * 8, (float)hzY + sinf(a) * 8, cosf(a) * s, sinf(a) * s, 0.8f, H4_PROP + 7, P4_STREAK, 1); }
  int emit = (int)(st * 6);
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 200 + v3_rndf() * 400; h4_emit(160 + cosf(a) * 10, (float)hzY + sinf(a) * 10, cosf(a) * s, sinf(a) * s, 1.0f, (uint8_t)(H4_PROP + (v3_rnd(3) ? 7 : 5)), P4_STREAK, 1); }
  for (int i = 0; i < h4_np; i++) { H4Part* p = &h4_p[i]; if (p->shape == P4_STREAK) { p->vx *= 1.0f + dt * 2.5f; p->vy *= 1.0f + dt * 2.5f; } }   // streaks speed up as they fly out
  h4_parts(dt, 0.0f, 0.0f);
}

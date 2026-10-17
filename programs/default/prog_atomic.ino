// =====================================================================
// PROGRAM: ATOMIC — the 1950s (by Dewey) — Set 4, slot 3
//
// The landscape flight as the atomic age dreamed it: turquoise, coral and
// mustard, desert mesas under Googie starbursts, boomerangs and atoms in
// the sky, Sputnik beeping overhead, flying saucers, a ringed Saturn over a
// moonscape, pastel suburbia and glowing green fallout lakes.
//
// EXPLODE (p15, or k12 for one blast): an atomic flash on the horizon. A
// mushroom cloud boils up, the ground heaves into a mound beneath it and a
// shockwave ring rolls out across the land towards you.
//
// Presets: k0 Test Site · k1 Googie · k2 Sputnik · k3 Saturn · k4 Drive-In · k5 Mesa Town
//          k6 Saucers · k7 Rocket Range · k8 Suburbia · k9 Boomerang · k10 Fallout · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Starbursts · p8 Fog · p9 Sun · p10 Draw Distance · p11 Saucers · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 starbursts · 2 saucers · 4 sun · 8 sputnik · 16 saturn · 32 atoms / boomerangs
static const H4World at_worlds[11] = {
  {{{0,140,160},{90,200,200},{255,200,150},{255,230,190}}, {{255,90,70},{255,250,210}}, {{170,80,50},{210,110,70},{235,150,90},{245,190,120},{250,215,160},{255,240,210}}, {0,0,0}, {255,90,80},
   T4_MESA, S4_EDGES, X4_ERUPT | X4_RING, 0, 55, 30, 0, 1, 0, 1 | 4, 26, 70},                                        // 0 Test Site
  {{{0,170,180},{120,220,210},{250,240,200},{255,250,230}}, {{255,110,90},{255,255,255}}, {{0,150,150},{80,190,180},{250,140,120},{255,200,80},{250,230,180},{255,255,240}}, {0,140,160}, {40,40,40},
   T4_ROLLING, S4_EDGES, X4_ERUPT | X4_RING, 0, 30, 25, 0, 1, 0, 1 | 32, 0, 0},                                      // 1 Googie
  {{{0,0,20},{0,20,60},{20,60,110},{60,110,150}}, {{180,180,190},{255,255,255}}, {{20,30,50},{40,60,80},{70,90,110},{110,130,140},{160,170,180},{220,225,230}}, {10,20,40}, {255,120,90},
   T4_MAP, S4_CONTOUR, X4_ERUPT | X4_RING, 0, 50, 30, 0, 1, W4_STARS, 8, 0, 0},                                      // 2 Sputnik
  {{{0,0,0},{10,10,30},{40,30,70},{120,90,120}}, {{230,170,90},{255,240,200}}, {{60,60,70},{90,90,100},{120,120,130},{160,160,165},{200,200,205},{240,240,245}}, {0,0,0}, {255,200,120},
   T4_INVERT, S4_EDGES, X4_ERUPT | X4_RING, 0, 40, 15, 0, 1, W4_STARS, 16, 0, 0},                                    // 3 Saturn
  {{{5,0,30},{30,10,70},{110,40,110},{230,110,120}}, {{255,80,120},{255,255,220}}, {{10,10,30},{20,30,40},{30,50,50},{50,70,60},{80,90,70},{120,110,90}}, {10,10,30}, {255,240,140},
   T4_ROLLING, 0, X4_ERUPT | X4_RING, 0, 30, 40, 0, 1, W4_STARS, 2 | 32, 0, 0},                                      // 4 Drive-In
  {{{40,150,170},{150,210,200},{255,190,140},{255,225,180}}, {{230,170,40},{255,250,200}}, {{150,70,40},{190,100,60},{220,140,70},{240,180,90},{250,210,130},{255,235,180}}, {0,0,0}, {0,150,150},
   T4_MESA, S4_EDGES, X4_ERUPT | X4_RING, 0, 70, 30, 0, 1, 0, 1 | 4, 30, 90},                                        // 5 Mesa Town
  {{{0,20,40},{0,70,90},{80,160,160},{200,230,210}}, {{120,255,200},{255,255,255}}, {{30,40,30},{60,80,50},{100,110,60},{140,140,80},{180,170,110},{220,210,160}}, {20,50,60}, {120,255,180},
   T4_MAP, 0, X4_ERUPT | X4_RING, 30, 55, 35, 0, 1, W4_STARS, 2, 0, 0},                                             // 6 Saucers
  {{{80,170,220},{150,210,235},{240,230,200},{255,245,220}}, {{255,60,40},{255,255,255}}, {{200,170,120},{220,195,140},{235,215,165},{240,230,190},{250,240,210},{255,250,235}}, {0,0,0}, {255,60,50},
   T4_DUNES, S4_GRID, X4_ERUPT | X4_RING, 0, 40, 30, 0, 1, 0, 4 | 8, 24, 100},                                       // 7 Rocket Range
  {{{140,200,240},{190,225,245},{250,230,235},{255,245,240}}, {{255,170,190},{255,255,255}}, {{120,200,150},{160,220,170},{250,190,200},{255,220,170},{200,230,250},{255,255,255}}, {120,190,230}, {255,140,170},
   T4_ROLLING, S4_EDGES, X4_ERUPT | X4_RING, 60, 30, 25, 0, 1, 0, 1 | 4, 20, 110},                                   // 8 Suburbia
  {{{0,90,110},{0,170,170},{120,220,200},{230,250,230}}, {{240,80,60},{255,240,200}}, {{0,60,70},{0,110,110},{230,100,80},{250,170,60},{250,220,130},{255,250,220}}, {0,80,100}, {20,20,20},
   T4_MAP, S4_EDGES, X4_ERUPT | X4_RING, 40, 50, 30, 0, 1, 0, 32 | 1, 0, 0},                                         // 9 Boomerang
  {{{10,20,0},{30,60,10},{90,140,30},{170,210,80}}, {{60,255,40},{230,255,200}}, {{30,30,20},{50,50,30},{70,70,40},{100,90,50},{130,120,70},{170,160,100}}, {60,255,40}, {120,255,80},
   T4_MAP, S4_CONTOUR, X4_ERUPT | X4_RING, 70, 50, 50, 0, 1, W4_GLOW, 4, 18, 80},                                    // 10 Fallout
};
static const char* const at_presetNames[12] = {
  "Test Site", "Googie", "Sputnik", "Saturn", "Drive-In", "Mesa Town", "Saucers", "Rocket Range", "Suburbia", "Boomerang", "Fallout", "Tour"
};
const char* prog_atomic_name() { return "1950s ATOMIC"; }
const char* prog_atomic_character() { return "The 1950s: atomic-age turquoise and coral, Googie stars, saucers and Sputnik; EXPLODE is a mushroom cloud"; }
const char* prog_atomic_presetName(int preset) { return (preset >= 0 && preset < 12) ? at_presetNames[preset] : NULL; }
const char* prog_atomic_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Starbursts";
  if (pot == 11) return "Saucers";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_atomic_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_atomic_init() { h4_init(at_worlds, 11); }

// a Googie starburst: thin spikes round a point
static void at_star(int cx, int cy, int r, int rot, uint8_t c) {
  for (int k = 0; k < 8; k++) { int a = rot + k * 128, rr = (k & 1) ? r / 2 : r; v3_line(cx, cy, cx + ((v3_cos(a) * rr) >> 14), cy + ((v3_sin(a) * rr) >> 14), c); }
  v3_disc(cx, cy, 1, c);
}
static float at_cloudT = 0;
void prog_atomic_draw(int preset) {
  if (!h4_prepare(preset, 7.0f, 1.0f, 0.9f)) return;
  const H4World& wp = at_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float bursts = v3_centre(7) ? ((deco & 1) ? 0.6f : 0.0f) : v3_u(7);
  float saucers = v3_centre(11) ? ((deco & 2) ? 0.6f : 0.0f) : v3_u(11);
  h4_prop(0, 255, 255, 255); h4_prop(1, 255, 90, 70); h4_prop(2, 20, 20, 30); h4_prop(3, 200, 210, 220);   // white, coral, ink, chrome
  h4_prop(4, 255, 250, 220); h4_prop(5, 255, 190, 90); h4_prop(6, 200, 120, 80); h4_prop(7, 120, 90, 80);  // the cloud: hot core → dust

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(60 * h4k.skyK), hzY, 51);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_sun(70, hzY - wp.sunY / 2, R, false); }
  if (deco & 16) {                                                  // Saturn and its rings
    int px = 110 + (int)(sinf(h4_head) * 30), py = hzY - 55, R = (int)(24 * h4k.sunK);
    for (int a = 512; a < 1024; a += 3) v3_put(px + ((v3_cos(a) * R * 2) >> 14), py + ((v3_sin(a) * R / 3) >> 14), H4_ACC + 12);
    v3_disc(px, py, R, H4_ACC + 8); v3_disc(px - R / 4, py - R / 4, R * 2 / 3, H4_ACC + 11);
    for (int a = 0; a < 512; a += 3) v3_put(px + ((v3_cos(a) * R * 2) >> 14), py + ((v3_sin(a) * R / 3) >> 14), H4_ACC + 15);
  }
  if (bursts > 0.02f && h4k.skyK > 0.05f) {
    int n = 2 + (int)(bursts * 9);
    for (int i = 0; i < n; i++) {
      int x = (int)(fmodf(i * 97.0f - h4_head * 70.0f + 640, 340.0f)) - 10, y = 14 + (i * 37) % (hzY > 40 ? hzY - 20 : 20);
      at_star(x, y, 6 + (i % 3) * 3, (int)(h4_t * 40) + i * 50, (uint8_t)(H4_PROP + (i % 3 == 0 ? 1 : 0)));
    }
  }
  if (deco & 32) {                                                  // boomerangs and an atom
    for (int i = 0; i < 3; i++) {
      int x = 50 + i * 110, y = 22 + (i * 17) % 30;
      v3_line(x - 10, y + 4, x, y - 3, H4_PROP + 2); v3_line(x, y - 3, x + 12, y + 3, H4_PROP + 2); v3_line(x - 10, y + 5, x, y - 2, H4_PROP + 1);
    }
    int ax = 260, ay = 30;
    for (int o = 0; o < 3; o++) for (int a = 0; a < 1024; a += 8) {
      int ex = (v3_cos(a) * 14) >> 14, ey = (v3_sin(a) * 5) >> 14, r = o * 341 + (int)(h4_t * 30);
      v3_put(ax + ((ex * v3_cos(r) - ey * v3_sin(r)) >> 14), ay + ((ex * v3_sin(r) + ey * v3_cos(r)) >> 14), H4_PROP + 2);
    }
    v3_disc(ax, ay, 2, H4_PROP + 1);
  }
  if (deco & 8) {                                                   // Sputnik crossing the sky
    float u = fmodf(h4_t * 0.05f, 1.0f); int x = (int)(-20 + u * 360), y = 20 + (int)(10 * sinf(u * 3));
    v3_disc(x, y, 3, H4_PROP + 3);
    for (int k = 0; k < 4; k++) v3_line(x, y, x - 12 - k * 2, y + 6 - k * 4, H4_PROP + 3);
  }
  if (saucers > 0.02f) {                                            // flying saucers swooping
    int n = 1 + (int)(saucers * 4);
    for (int i = 0; i < n; i++) {
      int x = 160 + (int)(120 * sinf(h4_t * (0.4f + i * 0.17f) + i * 2)), y = 30 + (int)(18 * sinf(h4_t * (0.7f + i * 0.2f) + i)) + i * 12;
      for (int k = -12; k <= 12; k++) v3_put(x + k, y, H4_PROP + 3);
      for (int k = -9; k <= 9; k++) v3_put(x + k, y + 1, H4_PROP + 3);
      v3_disc(x, y - 3, 4, H4_ACC + 14);
      if (((int)(h4_t * 6) + i) & 1) { v3_put(x - 8, y + 2, H4_PROP + 1); v3_put(x + 8, y + 2, H4_PROP + 1); } else v3_put(x, y + 2, H4_PROP + 1);
    }
  }
  // ── the mushroom cloud ──
  if (E > 0.02f) {
    at_cloudT += dt;
    h4_glow(160, hzY, (int)(30 + E * 150), (int)(10 + E * 6));     // the flash
    int stemH = (int)(E * (hzY + 10) * 0.75f), capY = hzY - stemH;
    int capR = (int)(10 + E * 48);
    for (int y = capY; y < hzY; y++) {                             // the stem, boiling
      int w = 5 + (int)(E * 10) + ((v3_sin((y * 20) + (int)(at_cloudT * 300)) * 3) >> 14);
      v3_hspan(160 - w, 160 + w, y, (uint8_t)(H4_PROP + 4 + ((y + (int)(at_cloudT * 30)) / 6) % 3));
    }
    for (int k = 0; k < 9; k++) {                                  // the cap: rolling billows
      int a = k * 114 + (int)(at_cloudT * 60);
      int bx = 160 + ((v3_cos(a) * capR) >> 14), by = capY + ((v3_sin(a) * capR / 3) >> 14);
      v3_disc(bx, by, capR / 2, (uint8_t)(H4_PROP + 5 + (k & 1)));
    }
    v3_disc(160, capY - capR / 6, capR * 2 / 3, H4_PROP + 4);
    v3_disc(160, capY + capR / 5, capR / 2, H4_PROP + 5);
  } else at_cloudT = 0;

  // ── ground: the ground heaves and a shockwave rolls out ──
  h4_ground(hzY, h4_rollPx);

  if (h4_trigger) { h4_burstAt(160, (float)hzY, 50, 180, H4_PROP + 7, P4_DOT, 2, 10); h4_burstAt(160, (float)hzY, 3, 5, H4_PROP + 4, P4_RING, 40, 0); }
  int emit = (int)(E * 4);
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 3.14159f, s = 120 + v3_rndf() * 140; h4_emit(160 + cosf(a) * 30, (float)hzY + 4, cosf(a) * s, -sinf(a) * s * 0.3f, 0.8f + v3_rndf(), (uint8_t)(H4_PROP + 6 + v3_rnd(2)), P4_DOT, 2); }
  h4_parts(dt, 30.0f, 0.6f);
}

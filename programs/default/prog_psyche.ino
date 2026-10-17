// =====================================================================
// PROGRAM: PSYCHEDELIC — the 1960s (by Dewey) — Set 4, slot 4
//
// The landscape flight on a trip: rainbow contour bands flowing over the
// hills, colours that keep cycling, op-art black and white, flower-power
// meadows, lava-lamp lakes, a sitar sunset, Lucy's diamond sky, poster
// colours that clash on purpose, and a kaleidoscope mirror.
//
// EXPLODE (p15, or k12 for one blast): the horizon melts into a
// kaleidoscope. Ripples roll out through the land, the picture mirrors into
// two, then four, the colours spin faster and flower petals blow out of the
// vanishing point.
//
// Presets: k0 Acid Rainbow · k1 Op Art · k2 Flower Power · k3 Lava Lamp · k4 Paisley · k5 Sitar Sunset
//          k6 Lucy's Sky · k7 Peace Valley · k8 Kaleidoscope · k9 Fillmore · k10 Moonwalk · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Mirror · p8 Fog · p9 Sun · p10 Draw Distance · p11 Colour Cycle · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 swirl sky · 2 sun · 4 diamonds (stars) · 8 mirror · 16 earth
static const H4World py_worlds[11] = {
  {{{60,0,120},{200,0,160},{255,120,40},{255,230,60}}, {{255,0,200},{255,255,120}}, {{255,0,120},{255,140,0},{255,240,0},{0,220,120},{0,140,255},{170,0,255}}, {80,0,160}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_RING, 0, 55, 25, 0, 1, 0, 1 | 2, 34, 90},                                                  // 0 Acid Rainbow
  {{{0,0,0},{255,255,255},{0,0,0},{255,255,255}}, {{0,0,0},{255,255,255}}, {{0,0,0},{255,255,255},{0,0,0},{255,255,255},{0,0,0},{255,255,255}}, {255,255,255}, {255,0,0},
   T4_MAP, S4_CONTOUR, X4_RING, 0, 50, 0, 8, 1, 0, 1, 0, 0},                                                         // 1 Op Art
  {{{60,170,255},{140,210,255},{255,200,230},{255,240,200}}, {{255,120,0},{255,250,120}}, {{90,180,40},{130,210,50},{255,120,180},{255,200,0},{255,90,40},{200,90,255}}, {80,160,240}, {255,40,140},
   T4_ROLLING, S4_FLOWERS, X4_RING, 0, 40, 25, 0, 1, 0, 2, 30, 80},                                                  // 2 Flower Power
  {{{40,0,60},{120,0,120},{255,60,100},{255,170,60}}, {{255,60,0},{255,240,100}}, {{50,0,70},{90,0,110},{150,20,140},{220,60,120},{255,120,90},{255,200,90}}, {255,100,0}, {255,230,0},
   T4_MAP, 0, X4_RING, 80, 55, 30, 0, 1, W4_GLOW, 1, 0, 0},                                                          // 3 Lava Lamp
  {{{80,0,40},{180,20,80},{255,120,60},{255,200,120}}, {{255,60,120},{255,255,200}}, {{120,0,60},{200,30,100},{255,100,60},{255,170,0},{100,200,170},{0,130,140}}, {60,0,80}, {255,240,180},
   T4_MAP, S4_CONTOUR, X4_RING, 0, 50, 30, 0, 1, 0, 1, 0, 0},                                                        // 4 Paisley
  {{{50,0,90},{170,40,120},{255,130,40},{255,210,90}}, {{255,80,0},{255,240,150}}, {{120,20,90},{190,50,90},{240,110,60},{255,160,40},{255,210,90},{255,240,170}}, {0,0,0}, {255,255,200},
   T4_DUNES, S4_CONTOUR, X4_RING, 0, 50, 30, 0, 1, 0, 2, 48, 70},                                                    // 5 Sitar Sunset
  {{{0,0,30},{40,0,90},{130,40,180},{230,140,240}}, {{120,200,255},{255,255,255}}, {{30,0,80},{80,20,150},{150,60,220},{230,140,255},{120,220,255},{255,255,255}}, {20,0,60}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_RING, 30, 55, 30, 0, 1, W4_STARS, 4, 0, 0},                                                // 6 Lucy's Sky
  {{{90,180,255},{160,215,255},{240,240,200},{255,250,220}}, {{255,200,0},{255,255,220}}, {{60,140,40},{120,180,40},{200,210,60},{250,220,80},{250,170,60},{255,120,60}}, {60,140,220}, {255,255,255},
   T4_ROLLING, S4_FLOWERS, X4_RING, 50, 40, 25, 0, 1, 0, 2, 28, 90},                                                 // 7 Peace Valley
  {{{0,0,60},{120,0,140},{255,40,120},{255,200,0}}, {{0,255,200},{255,255,255}}, {{255,0,100},{255,200,0},{0,255,150},{0,150,255},{150,0,255},{255,0,200}}, {0,0,0}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_RING, 0, 55, 20, 0, 1, 0, 1 | 8, 0, 0},                                                    // 8 Kaleidoscope
  {{{0,160,60},{0,220,120},{255,40,40},{255,120,0}}, {{255,0,0},{255,255,0}}, {{255,0,0},{0,200,60},{255,0,0},{0,200,60},{255,220,0},{0,100,255}}, {0,120,255}, {255,255,0},
   T4_MAP, S4_EDGES, X4_RING, 30, 50, 15, 0, 1, 0, 1, 0, 0},                                                         // 9 Fillmore
  {{{0,0,0},{0,0,10},{10,10,30},{30,30,60}}, {{40,90,200},{200,230,255}}, {{70,70,75},{100,100,105},{130,130,135},{160,160,165},{200,200,205},{240,240,245}}, {0,0,0}, {255,255,255},
   T4_INVERT, S4_EDGES, X4_RING, 0, 40, 10, 0, 1, W4_STARS, 16, 0, 0},                                               // 10 Moonwalk
};
static const char* const py_presetNames[12] = {
  "Acid Rainbow", "Op Art", "Flower Power", "Lava Lamp", "Paisley", "Sitar Sunset", "Lucy's Sky", "Peace Valley", "Kaleidoscope", "Fillmore", "Moonwalk", "Tour"
};
const char* prog_psyche_name() { return "1960s PSYCHEDELIC"; }
const char* prog_psyche_character() { return "The 1960s: rainbow bands, op art, flower power and kaleidoscopes; EXPLODE melts the horizon into a kaleidoscope"; }
const char* prog_psyche_presetName(int preset) { return (preset >= 0 && preset < 12) ? py_presetNames[preset] : NULL; }
const char* prog_psyche_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Mirror";
  if (pot == 11) return "Colour Cycle";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_psyche_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_psyche_init() { h4_init(py_worlds, 11); }

static float py_cyc = 0;
void prog_psyche_draw(int preset) {
  const H4World& w0 = py_worlds[h4_pal];
  float cycK = v3_centre(11) ? (w0.style & S4_CONTOUR ? 0.5f : 0.2f) : v3_u(11) * 2.0f;
  py_cyc += v3_dt * cycK * (1.0f + h4_ex * 3.0f);
  if (py_cyc > 600.0f) py_cyc -= 600.0f;
  h4_bandShift = py_cyc;                                            // the colours flow over the land
  if (!h4_prepare(preset, 6.0f, 0.8f, 0.7f)) return;
  const H4World& wp = py_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  int mirror = v3_centre(7) ? ((deco & 8) ? 1 : 0) : (v3_k(7) < 340 ? 0 : (v3_k(7) < 680 ? 1 : 2));
  if (E > 0.35f && mirror < 1) mirror = 1;                          // exploding: the kaleidoscope opens
  if (E > 0.7f) mirror = 2;
  // contour bands that keep moving: rebuild them with a drifting offset
  if (h4c.style & S4_CONTOUR) { int off = (int)(py_cyc * 12); for (int h = 0; h < 256; h++) if (h4_band[h] != 0) h4_band[h] = (uint8_t)(((h + off) & 31) < 3 ? 11 : 1 + ((((h + off) >> 5) + 10) % 10)); }

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) {
    h4_stars((int)(50 * h4k.skyK), hzY, 61);
    if (deco & 4) for (int i = 0; i < 12; i++) {                    // diamonds
      uint32_t h = v3_hash(i * 31u + 7); int x = (int)(h % W), y = 10 + (int)((h >> 9) % 60), r = 2 + (int)((h4_t * 2 + i) * 1) % 3;
      v3_line(x - r, y, x, y - r, H4_ACC + 15); v3_line(x, y - r, x + r, y, H4_ACC + 15); v3_line(x + r, y, x, y + r, H4_ACC + 15); v3_line(x, y + r, x - r, y, H4_ACC + 15);
    }
  }
  if (deco & 1) {                                                   // a turning op-art spiral behind everything
    int rot = (int)(py_cyc * 200);
    h4_rays(160, hzY - 10, 16, 280, rot / 163.0f, 6.2831853f, H4_ACC + 3, H4_ACC + 8, 0.5f);
  }
  if (deco & 2) { int R = (int)(wp.sunR * h4k.sunK); h4_sun(160, hzY - wp.sunY / 3, R, true); }
  if (deco & 16) {                                                  // the Earth rising
    int R = (int)(18 * h4k.sunK); v3_disc(230, 40, R, H4_ACC + 7); v3_disc(226, 36, R * 2 / 3, H4_ACC + 12);
  }
  if (E > 0.02f) h4_glow(160, hzY, (int)(20 + E * 100), (int)(6 + E * 8));

  // ── ground: ripples roll out through the land ──
  h4_ground(hzY, h4_rollPx);

  // ── petals blowing out ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 60, 200, 64 + 11 * 16 + 3, P4_STAR, 4, 40);
  int emit = (int)(E * 4);
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 60 + v3_rndf() * 160; h4_emit(160, (float)hzY, cosf(a) * s, sinf(a) * s * 0.7f, 1.0f + v3_rndf(), (uint8_t)(64 + (1 + v3_rnd(10)) * 16 + 3), (uint8_t)(v3_rnd(2) ? P4_STAR : P4_DOT), (uint8_t)(2 + v3_rnd(4))); }
  h4_parts(dt, -10.0f, 0.5f);
  if (mirror) h4_mirror(mirror);
}

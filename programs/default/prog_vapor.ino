// =====================================================================
// PROGRAM: VAPOR — the 2010s (by Dewey) — Set 4, slot 9
//
// The landscape flight as vaporwave and the 2010s internet: pastel pink and
// teal, an endless grid floor, a striped sunset, palm-tree silhouettes,
// marble-white hills, an aesthetic ocean, lo-fi night skies, pastel
// pyramids, cloud nine, a blocky game world and a faded photo filter.
//
// EXPLODE (p15, or k12 for one blast): the dream folds. The far land curls
// up into the sky like a city folding over itself, pastel shards drift
// upwards in slow motion and the picture slips like a worn tape.
//
// Presets: k0 Vapor Sunset · k1 Palm Mall · k2 Marble · k3 Aesthetic Ocean · k4 Lo-Fi Night · k5 Synth Dusk
//          k6 Seapunk · k7 Pastel Pyramids · k8 Cloud Nine · k9 Blockworld · k10 Faded Photo · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Palms · p8 Fog · p9 Sun · p10 Draw Distance · p11 VHS · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 palms · 2 striped sun · 4 moon · 8 stars · 16 clouds · 32 VHS
static const H4World vp_worlds[11] = {
  {{{40,10,90},{160,60,180},{255,140,190},{255,200,200}}, {{255,90,170},{255,240,170}}, {{20,10,60},{60,30,110},{120,60,160},{80,200,220},{160,240,240},{255,255,255}}, {40,20,90}, {0,255,240},
   T4_FLAT, S4_GRID, X4_FOLD, 0, 30, 30, 0, 1, 0, 1 | 2, 50, 70},                                                    // 0 Vapor Sunset
  {{{120,200,230},{200,170,230},{255,190,220},{255,230,230}}, {{255,140,190},{255,250,230}}, {{140,230,220},{180,240,230},{255,200,230},{255,170,210},{230,200,255},{255,255,255}}, {90,210,220}, {255,120,200},
   T4_MAP, 0, X4_FOLD, 50, 35, 30, 0, 1, 0, 1 | 16, 0, 0},                                                           // 1 Palm Mall
  {{{230,200,230},{240,220,235},{250,235,240},{255,248,250}}, {{255,180,210},{255,255,255}}, {{110,100,130},{160,150,175},{205,195,215},{235,228,240},{250,245,252},{255,255,255}}, {120,220,230}, {255,60,160},
   T4_INVERT, S4_EDGES, X4_FOLD, 40, 50, 20, 0, 1, 0, 2, 40, 90},                                                    // 2 Marble
  {{{30,0,70},{120,40,160},{255,120,200},{255,190,190}}, {{255,80,160},{255,240,180}}, {{40,10,80},{80,30,130},{140,70,170},{200,120,200},{240,180,230},{255,230,250}}, {0,170,200}, {255,0,220},
   T4_MAP, S4_GRID, X4_FOLD, 120, 40, 30, 0, 1, W4_CALM, 2 | 1, 46, 80},                                             // 3 Aesthetic Ocean
  {{{5,0,25},{25,10,60},{70,40,110},{130,80,150}}, {{200,180,255},{255,255,255}}, {{20,10,40},{40,25,70},{70,45,100},{100,70,130},{140,110,170},{190,170,220}}, {20,10,50}, {255,180,230},
   T4_ROLLING, 0, X4_FOLD, 30, 40, 40, 0, 1, W4_STARS, 4 | 8 | 32, 18, 120},                                         // 4 Lo-Fi Night
  {{{30,0,50},{120,20,110},{255,90,140},{255,180,150}}, {{255,60,120},{255,230,160}}, {{40,0,60},{90,10,100},{160,30,130},{230,70,140},{255,140,160},{255,210,200}}, {30,0,60}, {255,220,120},
   T4_MAP, S4_CONTOUR, X4_FOLD, 0, 55, 35, 0, 1, 0, 2, 44, 80},                                                      // 5 Synth Dusk
  {{{0,80,100},{0,170,170},{120,240,220},{220,255,240}}, {{0,255,200},{255,255,255}}, {{0,60,90},{0,120,140},{0,190,190},{120,240,220},{255,150,220},{255,255,255}}, {0,200,210}, {255,100,220},
   T4_MAP, 0, X4_FOLD, 110, 45, 25, 0, 1, 0, 1 | 16, 0, 0},                                                          // 6 Seapunk
  {{{80,40,150},{200,120,210},{255,200,220},{255,235,215}}, {{255,170,120},{255,250,230}}, {{150,110,200},{190,150,230},{240,190,230},{255,210,200},{255,230,190},{255,250,240}}, {0,0,0}, {0,230,240},
   T4_PYRAMID, S4_GRID, X4_FOLD, 0, 50, 25, 0, 1, 0, 2, 40, 70},                                                     // 7 Pastel Pyramids
  {{{50,100,230},{140,150,250},{240,180,235},{255,225,240}}, {{255,255,255},{255,255,255}}, {{120,130,220},{165,160,240},{210,185,245},{245,205,240},{255,228,245},{255,255,255}}, {150,170,255}, {255,120,200},
   T4_ROLLING, 0, X4_FOLD, 0, 40, 40, 0, 1, 0, 16, 0, 0},                                                            // 8 Cloud Nine
  {{{60,130,230},{110,170,250},{170,210,255},{220,235,255}}, {{255,255,200},{255,255,255}}, {{70,130,50},{90,170,60},{110,200,70},{130,110,80},{150,150,150},{255,255,255}}, {40,90,230}, {40,40,40},
   T4_FACETS, 0, X4_FOLD, 60, 60, 25, 0, 2, 0, 16, 0, 0},                                                            // 9 Blockworld
  {{{150,140,130},{200,180,160},{235,215,190},{245,230,210}}, {{230,160,120},{255,245,225}}, {{120,120,90},{150,145,110},{180,165,130},{205,185,150},{225,205,175},{240,225,200}}, {130,150,160}, {250,200,170},
   T4_MAP, 0, X4_FOLD, 50, 50, 45, 0, 1, 0, 32, 0, 0},                                                               // 10 Faded Photo
};
static const char* const vp_presetNames[12] = {
  "Vapor Sunset", "Palm Mall", "Marble", "Aesthetic Ocean", "Lo-Fi Night", "Synth Dusk", "Seapunk", "Pastel Pyramids", "Cloud Nine", "Blockworld", "Faded Photo", "Tour"
};
const char* prog_vapor_name() { return "2010s VAPOR"; }
const char* prog_vapor_character() { return "The 2010s: vaporwave pastels, grid floors, palms and sunsets; EXPLODE folds the dream world up into the sky"; }
const char* prog_vapor_presetName(int preset) { return (preset >= 0 && preset < 12) ? vp_presetNames[preset] : NULL; }
const char* prog_vapor_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Palms";
  if (pot == 11) return "VHS";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_vapor_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_vapor_init() { h4_init(vp_worlds, 11); }

static void vp_palm(int x, int y, int h, uint8_t c) {             // a palm silhouette
  for (int k = 0; k < h; k++) { int xx = x + (k * k) / (h * 3); v3_put(xx, y - k, c); v3_put(xx + 1, y - k, c); }
  int tx = x + h / 3, ty = y - h;
  for (int f = 0; f < 6; f++) {
    int a = 600 + f * 100 + (f > 2 ? 60 : 0);
    for (int k = 0; k < h / 2; k++) { int r = k; int px = tx + ((v3_cos(a) * r) >> 14), py = ty + ((v3_sin(a) * r) >> 14) + (k * k) / (h * 2); v3_put(px, py, c); v3_put(px, py + 1, c); }
  }
}
void prog_vapor_draw(int preset) {
  if (!h4_prepare(preset, 5.0f, 0.8f, 0.8f)) return;               // soft and slow
  const H4World& wp = vp_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float palms = v3_centre(7) ? ((deco & 1) ? 1.0f : 0.0f) : v3_u(7);
  float vhs = v3_centre(11) ? ((deco & 32) ? 0.7f : 0.15f) : v3_u(11);
  h4_prop(0, 20, 10, 40); h4_prop(1, 255, 255, 255); h4_prop(2, 255, 200, 230); h4_prop(3, 160, 240, 240);   // palms, cloud, pastels
  h4_prop(4, 255, 170, 220); h4_prop(5, 200, 180, 255); h4_prop(6, 170, 255, 230); h4_prop(7, 255, 240, 180);

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(60 * h4k.skyK), hzY, 101);
  if (deco & 2) h4_sun(160, hzY - wp.sunY / 3, (int)(wp.sunR * h4k.sunK), true);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_glow(230, hzY - 60, R * 2, 5); v3_disc(230, hzY - 60, R, H4_ACC + 14); v3_disc(236, hzY - 64, R, (uint8_t)(hzY - 64 > 0 ? 6 : 0)); }
  if ((deco & 16) && h4k.skyK > 0.05f) for (int i = 0; i < 6; i++) {
    float x = fmodf(i * 97.0f - h4_head * 70 - h4_t * 3 + 800, 420.0f) - 50; int y = 14 + (i * 31) % (hzY > 50 ? hzY - 30 : 20);
    h4_cloud((int)x, y, 10 + (i % 3) * 3, H4_PROP + 1, H4_PROP + 2);
  }
  if (E > 0.02f) h4_glow(160, hzY, (int)(20 + E * 60), (int)(5 + E * 5));

  // ── ground: the far land folds up into the sky ──
  h4_ground(hzY, h4_rollPx);

  // ── palms in silhouette along the shore ──
  if (palms > 0.02f) {
    int n = (int)(palms * 5) + 1;
    for (int i = 0; i < n; i++) {
      int x = (int)(fmodf(i * 73.0f + 30 - h4_head * 120 + 640, 400.0f)) - 40;
      vp_palm(x, H - 2, 60 + (i % 3) * 20, H4_PROP + 0);
    }
  }
  // ── pastel shards drifting up in slow motion ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 30, 60, H4_PROP + 4, P4_SQUARE, 6, 30);
  if (v3_rndf() < E * 0.5f) h4_emit((float)v3_rnd(W), (float)(hzY + v3_rnd(H - hzY)), (v3_rndf() - 0.5f) * 20, -15 - v3_rndf() * 30, 3.0f + v3_rndf() * 3, (uint8_t)(H4_PROP + 2 + v3_rnd(6)), (uint8_t)(v3_rnd(2) ? P4_SQUARE : P4_SHARD), (uint8_t)(3 + v3_rnd(8)));
  h4_parts(dt, 0.0f, 0.0f);
  // ── VHS: tracking wobble, a noise band, chroma slip ──
  float v = vhs + E * 0.6f;
  if (v > 0.02f) {
    int band = (int)fmodf(h4_t * 40, (float)(H + 40)) - 20;
    for (int y = 0; y < H; y++) {
      int dx = (int)(v * 3 * sinf(y * 0.07f + h4_t * 2));
      if (y >= band && y < band + 6) dx += (int)(v * (v3_rnd(20) - 10));
      if (dx) h4_shiftRow(y, dx);
    }
    if (v > 0.3f) h4_scanlines();
  }
}

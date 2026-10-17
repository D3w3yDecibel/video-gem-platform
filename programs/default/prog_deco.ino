// =====================================================================
// PROGRAM: DECO — the 1920s (by Dewey) — Set 4, slot 0
//
// The landscape flight as Art Deco: black and gold, stepped ziggurats and
// skyscrapers inked in gold, sunbursts fanning up from the horizon,
// searchlights sweeping a jazz-age night, Egyptian-revival pyramids, the
// green light across Gatsby's bay, and silent-film sepia with grain.
//
// EXPLODE (p15, or k12 for one blast): the horizon shatters like gilded
// glass — the land cracks into shards that lift apart, the sunburst blazes
// out of the vanishing point and gold fragments spin away.
//
// Presets: k0 Metropolis · k1 Gatsby · k2 Chrysler · k3 Nile Deco · k4 Jazz Night · k5 Silent Film
//          k6 Ziggurat · k7 Ocean Liner · k8 Radio City · k9 Speakeasy · k10 Expo · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Film Grain · p8 Fog · p9 Sun · p10 Draw Distance · p11 Sunburst · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 sunburst · 2 searchlights · 4 sun disc · 8 green light · 16 heavy sepia grain · 32 stars
static const H4World dc_worlds[11] = {
  {{{0,0,0},{15,10,5},{60,40,10},{150,110,40}}, {{120,80,10},{255,230,140}}, {{8,8,10},{20,18,15},{35,30,22},{60,50,30},{90,75,40},{130,110,60}}, {10,10,12}, {255,200,60},
   T4_CITY, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 50, 45, 0, 1, 0, 1 | 2, 0, 0},                                         // 0 Metropolis
  {{{0,5,20},{10,25,60},{30,60,100},{90,110,130}}, {{0,120,40},{120,255,140}}, {{20,20,30},{60,55,40},{120,100,60},{190,160,90},{230,210,150},{250,240,210}}, {10,30,60}, {255,210,90},
   T4_TERRACE, S4_EDGES, X4_SHARDS | X4_SLABS, 120, 50, 40, 20, 1, W4_STARS | W4_CALM, 8 | 32, 0, 0},              // 1 Gatsby
  {{{10,10,20},{60,60,80},{150,150,170},{220,220,230}}, {{120,120,140},{255,255,255}}, {{20,20,25},{50,50,60},{90,90,105},{140,140,155},{190,190,205},{240,240,250}}, {30,30,40}, {255,255,255},
   T4_CITY, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 55, 35, 0, 1, 0, 1, 0, 0},                                            // 2 Chrysler
  {{{10,30,110},{30,70,170},{120,140,200},{240,200,120}}, {{200,120,20},{255,240,160}}, {{120,80,30},{170,120,50},{210,160,70},{235,195,100},{250,220,140},{255,240,190}}, {20,60,150}, {20,60,180},
   T4_PYRAMID, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 60, 35, 0, 1, 0, 1 | 4, 40, 80},                                   // 3 Nile Deco
  {{{0,0,10},{5,10,40},{20,30,80},{60,50,90}}, {{150,110,30},{255,240,180}}, {{5,5,20},{10,15,40},{20,25,60},{30,35,80},{45,50,100},{70,70,120}}, {5,5,25}, {255,200,60},
   T4_MAP, S4_CONTOUR, X4_SHARDS | X4_SLABS, 0, 55, 40, 0, 1, W4_STARS, 2 | 32, 0, 0},                               // 4 Jazz Night
  {{{30,25,20},{90,75,55},{170,150,120},{220,205,175}}, {{100,85,60},{255,245,220}}, {{25,20,15},{60,50,40},{100,85,65},{140,120,95},{185,165,135},{230,215,190}}, {70,60,45}, {245,235,210},
   T4_MAP, 0, X4_SHARDS | X4_SLABS, 40, 60, 50, 0, 1, 0, 16 | 4, 30, 60},                                            // 5 Silent Film
  {{{0,20,15},{0,60,45},{40,120,90},{200,190,120}}, {{160,120,20},{255,235,150}}, {{0,40,30},{0,80,60},{20,120,85},{60,160,110},{180,160,80},{255,220,120}}, {0,30,25}, {255,210,80},
   T4_TERRACE, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 90, 35, 32, 1, 0, 1, 0, 0},                                        // 6 Ziggurat
  {{{10,20,50},{40,70,120},{140,170,200},{240,220,190}}, {{200,40,30},{255,240,200}}, {{40,40,50},{120,30,30},{200,50,40},{230,220,200},{245,240,225},{255,255,250}}, {15,40,90}, {255,220,120},
   T4_MAP, S4_EDGES, X4_SHARDS | X4_SLABS, 150, 60, 35, 0, 1, W4_CALM, 1 | 4, 34, 70},                               // 7 Ocean Liner
  {{{20,0,10},{90,30,50},{220,120,110},{255,200,170}}, {{200,90,80},{255,240,200}}, {{40,10,20},{90,30,40},{170,70,70},{230,140,120},{250,200,170},{255,235,215}}, {40,10,20}, {255,215,110},
   T4_CITY, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 50, 35, 0, 1, 0, 1 | 2, 0, 0},                                        // 8 Radio City
  {{{5,0,0},{30,5,5},{70,15,15},{120,40,30}}, {{140,20,20},{255,200,140}}, {{10,0,0},{30,5,5},{60,10,10},{100,20,20},{150,40,30},{200,90,60}}, {20,0,0}, {255,190,70},
   T4_MAP, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 35, 85, 0, 1, 0, 2, 0, 0},                                             // 9 Speakeasy
  {{{0,0,0},{0,30,25},{0,90,75},{150,200,170}}, {{0,120,100},{255,250,220}}, {{0,0,0},{10,20,18},{0,70,60},{0,130,110},{230,210,150},{255,250,230}}, {0,0,0}, {230,200,90},
   T4_PYRAMID, S4_EDGES | S4_CONTOUR, X4_SHARDS | X4_SLABS, 0, 70, 30, 0, 1, W4_STARS, 2 | 32, 0, 0},                // 10 Expo
};
static const char* const dc_presetNames[12] = {
  "Metropolis", "Gatsby", "Chrysler", "Nile Deco", "Jazz Night", "Silent Film", "Ziggurat", "Ocean Liner", "Radio City", "Speakeasy", "Expo", "Tour"
};
const char* prog_deco_name() { return "1920s DECO"; }
const char* prog_deco_character() { return "The 1920s: Art Deco gold on black, sunbursts and searchlights; EXPLODE shatters the horizon like gilded glass"; }
const char* prog_deco_presetName(int preset) { return (preset >= 0 && preset < 12) ? dc_presetNames[preset] : NULL; }
const char* prog_deco_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Film Grain";
  if (pot == 11) return "Sunburst";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_deco_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_deco_init() { h4_init(dc_worlds, 11); }

void prog_deco_draw(int preset) {
  if (!h4_prepare(preset, 8.0f, 1.0f, 1.0f)) return;
  const H4World& wp = dc_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float grain = v3_centre(7) ? ((deco & 16) ? 0.8f : 0.15f) : v3_u(7);
  float burst = v3_centre(11) ? ((deco & 1) ? 1.0f : 0.0f) : v3_u(11) * 1.5f;

  // ── sky ──
  h4_sky(hzY, 120);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(60 * h4k.skyK), hzY, 11);
  if (burst > 0.02f || E > 0.02f) {                                 // the sunburst: alternating gold rays fanning up from the horizon
    float k = burst + E * 1.5f;
    int n = 18 + (int)(E * 14);
    h4_rays(160, hzY + 6, n, (int)(80 + 200 * (k > 1.2f ? 1.2f : k)), sinf(h4_t * 0.3f) * 0.05f + h4_head * 0.0f, 3.14159f * (0.9f + E * 0.6f), H4_ACC + 4 + (int)(E * 6), H4_ACC + 1 + (int)(E * 4), 0.45f);
  }
  if (deco & 2) {                                                   // searchlights sweeping from below the horizon
    for (int i = 0; i < 3; i++) {
      float a = -1.5708f + 0.55f * sinf(h4_t * (0.35f + i * 0.13f) + i * 2.0f);
      int bx = 60 + i * 100, by = hzY + 10;
      int32_t xs[3] = {bx * 16, (int32_t)((bx + cosf(a - 0.05f) * 260) * 16), (int32_t)((bx + cosf(a + 0.05f) * 260) * 16)};
      int32_t ys[3] = {by * 16, (int32_t)((by + sinf(a - 0.05f) * 260) * 16), (int32_t)((by + sinf(a + 0.05f) * 260) * 16)};
      v3_poly(xs, ys, 3, H4_ACC + 5);
    }
  }
  if (deco & 4) h4_sun(160, hzY - wp.sunY / 3, (int)(wp.sunR * h4k.sunK), false);
  if (deco & 8) { v3_disc(250, hzY + 4, 3, H4_ACC + 15); h4_glow(250, hzY + 4, 12, 9); }   // the green light at the end of the dock
  if (E > 0.02f) h4_glow(160, hzY, (int)(25 + E * 80), (int)(8 + E * 8));

  // ── ground ──
  h4_ground(hzY, h4_rollPx);

  // ── explosion: gilded shards spinning away ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 40, 200, 64 + 11 * 16 + 3, P4_SHARD, 7, 50);
  int emit = (int)(E * 4 + (v3_rndf() < E * 4 - (int)(E * 4) ? 1 : 0));
  for (int i = 0; i < emit; i++) {
    float a = -3.14159f * v3_rndf(), s = 60 + v3_rndf() * 180;
    h4_emit(160 + cosf(a) * 20, (float)hzY, cosf(a) * s, sinf(a) * s, 1.0f + v3_rndf() * 1.5f, (uint8_t)(i & 1 ? 64 + 11 * 16 + 3 : H4_ACC + 12 + v3_rnd(4)), P4_SHARD, (uint8_t)(3 + v3_rnd(6)));
  }
  h4_parts(dt, 120.0f, 0.3f);
  if (grain > 0.02f) h4_grain((int)(grain * 500), H4_ACC + 15, 0, grain > 0.5f ? 2 : 0);
}

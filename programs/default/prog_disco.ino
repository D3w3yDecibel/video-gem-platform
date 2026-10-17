// =====================================================================
// PROGRAM: DISCO — the 1970s (by Dewey) — Set 4, slot 5
//
// The landscape flight as a Saturday night: a lit dance floor stretching
// to the horizon with tiles flashing on the beat, a mirror ball turning in
// the sky throwing beams, 70s sunset stripes in orange, brown and gold,
// funk canyons, roller rinks, shag-carpet hills and glam-rock glitter peaks.
//
// EXPLODE (p15, or k12 for one blast): the floor goes up. The dance floor
// breaks into tiles that lift and scatter, the mirror ball's beams fan out
// everywhere and a storm of sparkles bursts from the horizon.
//
// Presets: k0 Dance Floor · k1 Studio 54 · k2 Funk Canyon · k3 Sunset Stripes · k4 Roller Rink · k5 Saturday Night
//          k6 Soul Train · k7 Groovy Hills · k8 Shag Carpet · k9 Glam Rock · k10 Boogie Wonderland · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Mirror Ball · p8 Fog · p9 Sun · p10 Draw Distance · p11 Stripes · p12 Sky
//        p13 Autopilot · p14 Pulse (the beat) · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 mirror ball · 2 striped sky · 4 sun · 8 stars · 16 light beams
static const H4World ds_worlds[11] = {
  {{{10,0,20},{40,0,60},{120,20,100},{230,80,120}}, {{255,120,200},{255,255,255}}, {{20,0,30},{255,0,120},{255,200,0},{0,200,255},{180,0,255},{255,255,255}}, {10,0,15}, {255,255,255},
   T4_FLAT, S4_TILES, X4_SHARDS | X4_SLABS, 0, 30, 30, 0, 1, W4_STARS, 1 | 8 | 16, 0, 0},                            // 0 Dance Floor
  {{{0,0,0},{30,0,40},{100,40,90},{200,150,120}}, {{230,180,60},{255,250,220}}, {{10,0,10},{120,60,160},{230,180,60},{255,230,140},{200,40,120},{255,255,255}}, {0,0,0}, {255,215,90},
   T4_FLAT, S4_TILES, X4_SHARDS | X4_SLABS, 0, 30, 25, 0, 1, W4_STARS, 1 | 16, 0, 0},                                // 1 Studio 54
  {{{120,40,10},{220,100,20},{255,170,40},{255,220,120}}, {{230,90,0},{255,240,160}}, {{110,50,20},{170,80,30},{220,120,40},{240,160,60},{250,200,90},{255,230,150}}, {0,0,0}, {90,40,10},
   T4_MESA, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 70, 30, 0, 1, 0, 2 | 4, 34, 70},                                      // 2 Funk Canyon
  {{{80,20,10},{200,80,20},{250,150,30},{255,210,80}}, {{255,110,0},{255,240,150}}, {{90,50,20},{150,90,30},{200,140,40},{230,180,60},{180,110,40},{120,70,30}}, {60,40,20}, {255,230,120},
   T4_ROLLING, 0, X4_SHARDS | X4_SLABS, 0, 45, 30, 0, 1, 0, 2 | 4, 44, 60},                                          // 3 Sunset Stripes
  {{{0,0,30},{40,0,80},{200,50,150},{255,150,200}}, {{0,200,255},{255,255,255}}, {{30,0,50},{60,0,90},{255,90,200},{90,200,255},{255,255,255},{255,150,220}}, {20,0,40}, {0,255,255},
   T4_FLAT, S4_GRID, X4_SHARDS | X4_SLABS, 0, 30, 25, 0, 1, W4_STARS, 1 | 16, 0, 0},                                 // 4 Roller Rink
  {{{0,0,10},{20,0,40},{70,10,80},{150,40,110}}, {{255,60,140},{255,255,255}}, {{10,0,20},{30,10,50},{60,20,90},{255,40,140},{255,200,60},{0,220,255}}, {0,0,0}, {255,220,60},
   T4_CITY, S4_EDGES | S4_GRID, X4_SHARDS | X4_SLABS, 0, 45, 40, 0, 1, W4_STARS, 1 | 16, 0, 0},                       // 5 Saturday Night
  {{{30,0,60},{100,0,120},{230,60,120},{255,180,60}}, {{255,200,0},{255,255,255}}, {{255,0,0},{255,140,0},{255,255,0},{0,220,0},{0,120,255},{200,0,255}}, {0,0,0}, {255,255,255},
   T4_FLAT, S4_TILES, X4_SHARDS | X4_SLABS, 0, 30, 30, 0, 1, 0, 1 | 2 | 16, 0, 0},                                   // 6 Soul Train
  {{{90,60,20},{200,130,40},{240,190,90},{250,230,170}}, {{230,120,0},{255,245,180}}, {{110,120,30},{150,150,40},{200,170,50},{230,130,40},{200,90,30},{150,60,20}}, {80,110,90}, {255,230,140},
   T4_ROLLING, 0, X4_SHARDS | X4_SLABS, 40, 45, 30, 0, 1, 0, 4 | 2, 38, 80},                                         // 7 Groovy Hills
  {{{50,25,10},{120,60,20},{200,120,40},{240,190,110}}, {{200,90,20},{255,230,170}}, {{90,40,20},{140,70,30},{190,110,40},{230,150,50},{250,190,80},{255,220,130}}, {60,30,10}, {255,200,100},
   T4_TERRACE, S4_EDGES, X4_SHARDS | X4_SLABS, 0, 50, 35, 12, 1, 0, 2, 0, 0},                                        // 8 Shag Carpet
  {{{20,0,40},{80,20,120},{200,120,220},{255,230,255}}, {{200,200,255},{255,255,255}}, {{60,60,80},{120,120,150},{190,190,220},{255,255,255},{255,120,240},{120,240,255}}, {30,0,50}, {255,255,255},
   T4_SPIKES, S4_EDGES | S4_CONTOUR, X4_SHARDS | X4_SLABS, 0, 90, 25, 0, 2, W4_STARS, 1 | 16 | 8, 0, 0},              // 9 Glam Rock
  {{{40,0,40},{140,20,90},{255,110,60},{255,210,120}}, {{255,170,0},{255,255,240}}, {{60,10,40},{255,80,0},{255,200,0},{255,60,140},{140,40,200},{255,255,255}}, {40,0,30}, {255,255,255},
   T4_ROLLING, S4_TILES, X4_SHARDS | X4_SLABS, 0, 35, 30, 0, 1, 0, 1 | 2 | 16, 0, 0},                                // 10 Boogie Wonderland
};
static const char* const ds_presetNames[12] = {
  "Dance Floor", "Studio 54", "Funk Canyon", "Sunset Stripes", "Roller Rink", "Saturday Night", "Soul Train", "Groovy Hills", "Shag Carpet", "Glam Rock", "Boogie Wonderland", "Tour"
};
const char* prog_disco_name() { return "1970s DISCO"; }
const char* prog_disco_character() { return "The 1970s: lit dance floors, a mirror ball and orange-brown sunset stripes; EXPLODE sends the floor flying"; }
const char* prog_disco_presetName(int preset) { return (preset >= 0 && preset < 12) ? ds_presetNames[preset] : NULL; }
const char* prog_disco_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Mirror Ball";
  if (pot == 11) return "Stripes";
  if (pot == 14) return "Beat";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_disco_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_disco_init() { h4_init(ds_worlds, 11); }

static float ds_spin = 0;
void prog_disco_draw(int preset) {
  if (!h4_prepare(preset, 6.0f, 1.0f, 0.9f)) return;
  const H4World& wp = ds_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float ball = v3_centre(7) ? ((deco & 1) ? 1.0f : 0.0f) : v3_u(7) * 1.4f;
  float stripes = v3_centre(11) ? ((deco & 2) ? 1.0f : 0.0f) : v3_u(11);
  ds_spin += dt * (0.6f + E * 2.0f);
  h4_prop(0, 255, 255, 255); h4_prop(1, 200, 200, 220); h4_prop(2, 120, 120, 150); h4_prop(3, 60, 60, 80);   // mirror ball facets
  h4_prop(4, 255, 255, 255); h4_prop(5, 255, 120, 220); h4_prop(6, 120, 220, 255); h4_prop(7, 255, 230, 120); // beams

  // ── sky ──
  h4_sky(hzY, 120);
  if (stripes > 0.02f) {                                            // 70s stripes across the sunset
    static const int8_t sy[5] = {-10, -22, -34, -46, -58};
    for (int i = 0; i < 5; i++) { int y = hzY + sy[i] * (hzY + 20) / 80, th = (int)((3 + i) * stripes); for (int k = 0; k < th; k++) v3_hspan(0, W - 1, y - k, (uint8_t)(H4_ACC + 3 + i * 3)); }
  }
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(50 * h4k.skyK), hzY, 71);
  if (deco & 4) { int R = (int)(wp.sunR * h4k.sunK); h4_sun(160, hzY - wp.sunY / 3, R, false); }
  int bx = 160, by = hzY > 60 ? hzY / 2 - 6 : 24, br = (int)(18 * (ball > 0.05f ? ball : 0));
  if ((deco & 16) || E > 0.05f) {                                   // light beams sweeping down from the ball
    int nb = 6 + (int)(E * 10);
    for (int i = 0; i < nb; i++) {
      float a = 1.5708f + 1.2f * sinf(ds_spin * (0.7f + i * 0.13f) + i * 1.1f) * (1.0f + E);
      int32_t xs[3] = {bx * 16, (int32_t)((bx + cosf(a - 0.03f) * 320) * 16), (int32_t)((bx + cosf(a + 0.03f) * 320) * 16)};
      int32_t ys[3] = {by * 16, (int32_t)((by + sinf(a - 0.03f) * 320) * 16), (int32_t)((by + sinf(a + 0.03f) * 320) * 16)};
      v3_poly(xs, ys, 3, (uint8_t)(H4_PROP + 4 + (i & 3)));
    }
  }
  if (E > 0.02f) h4_glow(160, hzY, (int)(25 + E * 90), (int)(7 + E * 8));

  // ── ground: tiles flash on the beat ──
  h4_ground(hzY, h4_rollPx);

  // ── the mirror ball (over the ground, hanging from above) ──
  if (br > 2) {
    v3_line(bx, 0, bx, by - br, H4_PROP + 2);
    for (int y = -br; y <= br; y++) {
      int hw = (int)v3_isqrt((uint32_t)(br * br - y * y));
      for (int x = -hw; x <= hw; x++) {
        int fx = ((x + (int)(ds_spin * 20)) >> 2), fy = (y + br) >> 2;
        uint32_t h = v3_hash((uint32_t)(fx * 31 + fy * 7));
        int c = ((x & 3) == 0 || (y & 3) == 0) ? 3 : (h & 7) == 0 ? 0 : (x + y < 0 ? 1 : 2);
        v3_put(bx + x, by + y, (uint8_t)(H4_PROP + c));
      }
    }
  }
  // ── sparkles ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 80, 260, H4_PROP + 0, P4_STAR, 4, 40);
  int emit = (int)(E * 5) + ((v3_rndf() < 0.3f * ball) ? 1 : 0);
  for (int i = 0; i < emit; i++) {
    bool fromBall = i == emit - 1 && ball > 0.05f && E < 0.05f;
    float a = v3_rndf() * 6.2831853f, s = fromBall ? 40 + v3_rndf() * 60 : 80 + v3_rndf() * 200;
    h4_emit(fromBall ? bx : 160, fromBall ? by : (float)hzY, cosf(a) * s, sinf(a) * s * 0.8f, 0.6f + v3_rndf(), (uint8_t)(H4_PROP + (v3_rnd(2) ? 0 : 5 + v3_rnd(3))), P4_STAR, (uint8_t)(2 + v3_rnd(3)));
  }
  h4_parts(dt, 40.0f, 0.4f);
}

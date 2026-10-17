// =====================================================================
// PROGRAM: CGI — the 1990s (by Dewey) — Set 4, slot 7
//
// The landscape flight as the 90s rendered it: chunky low-poly worlds in
// flat-shaded blocks, rave lasers fanning over a black field, a Myst-like
// island, screensaver colours, a VR wireframe, acid-house yellow, Matrix
// green, primary-coloured console hills and Windows-95 clouds.
//
// EXPLODE (p15, or k12 for one blast): the polygons come apart. The land
// splits into layered facets that lift and shear, a fan of lasers sweeps
// out of the horizon and triangles tumble towards the camera.
//
// Presets: k0 Low Poly · k1 Rave · k2 Myst Island · k3 Screensaver · k4 VR Grid · k5 Acid House
//          k6 Matrix · k7 Console · k8 Polygon Canyon · k9 Jungle · k10 Win 95 · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Lasers · p8 Fog · p9 Sun · p10 Draw Distance · p11 Polygon Size · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 lasers · 2 sun · 4 stars · 8 code rain · 16 clouds · 32 smiley
static const H4World cg_worlds[11] = {
  {{{20,0,60},{80,20,140},{200,90,200},{255,190,170}}, {{255,120,0},{255,250,200}}, {{0,90,110},{0,140,150},{60,180,170},{170,90,200},{230,130,200},{255,220,230}}, {0,50,110}, {255,255,255},
   T4_FACETS, 0, X4_SLABS | X4_SHARDS, 40, 55, 25, 0, 1, 0, 2, 30, 80},                                              // 0 Low Poly
  {{{0,0,0},{0,0,0},{10,0,20},{40,0,60}}, {{0,255,0},{255,255,255}}, {{0,0,0},{10,10,20},{20,20,40},{30,30,60},{40,40,80},{60,60,110}}, {0,0,0}, {0,255,120},
   T4_FACETS, S4_GRID, X4_SLABS | X4_SHARDS, 0, 40, 30, 0, 1, W4_STARS, 1, 0, 0},                                    // 1 Rave
  {{{40,60,90},{90,120,150},{170,180,170},{230,220,190}}, {{240,200,120},{255,250,230}}, {{150,140,110},{70,110,50},{40,90,40},{90,90,80},{130,130,120},{200,200,190}}, {20,60,90}, {255,240,200},
   T4_FACETS, 0, X4_SLABS | X4_SHARDS, 110, 70, 45, 0, 1, W4_CALM, 2 | 16, 22, 100},                                 // 2 Myst Island
  {{{0,0,0},{0,0,20},{0,0,50},{0,0,90}}, {{255,0,255},{255,255,255}}, {{255,0,0},{255,255,0},{0,255,0},{0,255,255},{0,0,255},{255,0,255}}, {0,0,60}, {255,255,255},
   T4_FACETS, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 60, 20, 0, 1, W4_STARS, 4, 0, 0},                                   // 3 Screensaver
  {{{0,0,0},{0,0,0},{20,0,30},{60,0,80}}, {{255,0,255},{255,255,255}}, {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}, {0,0,0}, {255,0,255},
   T4_FACETS, S4_WIRE, X4_SLABS | X4_SHARDS, 0, 55, 20, 0, 1, W4_STARS, 0, 0, 0},                                    // 4 VR Grid
  {{{0,0,0},{20,20,0},{90,90,0},{200,200,0}}, {{255,240,0},{255,255,200}}, {{0,0,0},{40,40,0},{120,120,0},{220,220,0},{255,255,0},{255,255,255}}, {0,0,0}, {0,0,0},
   T4_FACETS, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 50, 20, 0, 1, 0, 1 | 32, 0, 0},                                     // 5 Acid House
  {{{0,0,0},{0,10,0},{0,30,0},{0,60,10}}, {{0,120,30},{180,255,180}}, {{0,10,0},{0,30,5},{0,60,10},{0,100,20},{20,160,40},{120,255,140}}, {0,0,0}, {150,255,150},
   T4_FACETS, S4_GRID, X4_SLABS | X4_SHARDS, 0, 50, 40, 0, 1, 0, 8, 0, 0},                                           // 6 Matrix
  {{{0,90,255},{60,150,255},{150,210,255},{220,240,255}}, {{255,220,0},{255,255,255}}, {{0,170,0},{60,210,0},{255,220,0},{255,120,0},{230,30,30},{255,255,255}}, {0,90,240}, {0,0,0},
   T4_FACETS, S4_EDGES, X4_SLABS | X4_SHARDS, 50, 60, 20, 0, 1, 0, 16 | 2, 26, 90},                                  // 7 Console
  {{{60,20,60},{180,70,60},{255,140,60},{255,210,120}}, {{255,90,0},{255,240,180}}, {{120,40,30},{180,70,40},{220,110,50},{240,150,70},{250,190,100},{255,225,160}}, {0,0,0}, {60,20,20},
   T4_FACETS, S4_EDGES, X4_SLABS | X4_SHARDS, 0, 80, 30, 0, 1, 0, 2, 34, 70},                                        // 8 Polygon Canyon
  {{{0,30,20},{0,70,40},{60,130,70},{160,190,120}}, {{120,255,60},{240,255,220}}, {{0,40,20},{0,70,30},{20,100,40},{60,130,50},{120,150,60},{180,190,100}}, {0,60,60}, {255,200,0},
   T4_FACETS, 0, X4_SLABS | X4_SHARDS, 60, 65, 60, 0, 1, 0, 1, 0, 0},                                                // 9 Jungle
  {{{0,100,200},{40,140,230},{120,190,250},{200,230,255}}, {{255,255,255},{255,255,255}}, {{0,128,128},{0,160,160},{60,190,190},{192,192,192},{230,230,230},{255,255,255}}, {0,0,128}, {0,0,128},
   T4_FACETS, S4_EDGES, X4_SLABS | X4_SHARDS, 40, 50, 20, 0, 1, 0, 16, 0, 0},                                        // 10 Win 95
};
static const char* const cg_presetNames[12] = {
  "Low Poly", "Rave", "Myst Island", "Screensaver", "VR Grid", "Acid House", "Matrix", "Console", "Polygon Canyon", "Jungle", "Win 95", "Tour"
};
const char* prog_cgi_name() { return "1990s CGI"; }
const char* prog_cgi_character() { return "The 1990s: low-poly worlds, rave lasers, Matrix green and console colours; EXPLODE breaks the polygons apart"; }
const char* prog_cgi_presetName(int preset) { return (preset >= 0 && preset < 12) ? cg_presetNames[preset] : NULL; }
const char* prog_cgi_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Lasers";
  if (pot == 11) return "Polygon Size";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_cgi_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_cgi_init() { h4_init(cg_worlds, 11); }

void prog_cgi_draw(int preset) {
  if (!h4_prepare(preset, 8.0f, 1.3f, 0.8f)) return;               // flat, hard 90s shading
  const H4World& wp = cg_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float lasers = v3_centre(7) ? ((deco & 1) ? 0.8f : 0.0f) : v3_u(7);
  h4c.facet = v3_centre(11) ? 4 : (v3_k(11) < 340 ? 2 : (v3_k(11) < 680 ? 4 : 8));
  h4_prop(0, 255, 0, 80); h4_prop(1, 0, 255, 120); h4_prop(2, 0, 160, 255); h4_prop(3, 255, 230, 0);   // lasers
  h4_prop(4, 255, 255, 255); h4_prop(5, 200, 200, 210); h4_prop(6, 255, 230, 0); h4_prop(7, 0, 0, 0);  // clouds, smiley

  // ── sky ──
  h4_sky(hzY, 110);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(50 * h4k.skyK), hzY, 81);
  if (deco & 2) { int R = (int)(wp.sunR * h4k.sunK); h4_sun(220, hzY - wp.sunY / 2, R, false); }
  if ((deco & 16) && h4k.skyK > 0.05f) for (int i = 0; i < 5; i++) {   // flat 90s clouds
    int x = (int)(fmodf(i * 87.0f - h4_head * 70 - h4_t * 5 + 800, 400.0f)) - 40, y = 16 + (i * 23) % 40;
    v3_rect(x - 14, y - 3, x + 14, y + 4, H4_PROP + 4); v3_rect(x - 6, y - 8, x + 8, y, H4_PROP + 4); v3_rect(x - 14, y + 3, x + 14, y + 5, H4_PROP + 5);
  }
  if (deco & 32) {                                                  // the acid smiley
    int cx = 250, cy = hzY > 60 ? hzY - 34 : 30, R = (int)(16 * h4k.sunK) + 2;
    v3_disc(cx, cy, R, H4_PROP + 6); v3_disc(cx - R / 3, cy - R / 4, R / 7 + 1, H4_PROP + 7); v3_disc(cx + R / 3, cy - R / 4, R / 7 + 1, H4_PROP + 7);
    for (int a = 80; a < 432; a += 8) v3_put(cx + ((v3_cos(a) * R * 6 / 10) >> 14), cy + ((v3_sin(a) * R * 6 / 10) >> 14), H4_PROP + 7);
  }
  float lz = lasers + E * 1.2f;
  if (lz > 0.02f) {                                                 // rave lasers fanning from the horizon
    int n = 4 + (int)(lz * 10);
    for (int i = 0; i < n; i++) {
      float a = -1.5708f + 1.3f * sinf(h4_t * (0.8f + i * 0.07f) + i * 0.9f) * (0.6f + E * 0.6f);
      int x1 = 160 + (int)(cosf(a) * 400), y1 = hzY + (int)(sinf(a) * 400);
      v3_line(160, hzY, x1, y1, (uint8_t)(H4_PROP + (i & 3)));
    }
  }
  if (E > 0.02f) h4_glow(160, hzY, (int)(20 + E * 80), (int)(8 + E * 7));

  // ── ground ──
  h4_ground(hzY, h4_rollPx);

  if (deco & 8) {                                                   // Matrix code rain over everything
    for (int c = 0; c < 40; c++) {
      int x = c * 8 + 2, head = (int)(fmodf(h4_t * (40 + (v3_hash(c * 7u) & 63)) + (v3_hash(c * 13u) & 255), 300.0f)) - 30;
      for (int k = 0; k < 10; k++) { int y = head - k * 6; if (y < 0 || y >= H) continue; uint8_t col = k == 0 ? (uint8_t)(64 + 11 * 16 + 3) : (uint8_t)(64 + 9 * 16 + (k > 5 ? 8 : 0) + 3); v3_rect(x, y, x + 3, y + 3, col); }
    }
  }
  // ── triangles tumbling at the camera ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 40, 240, H4_PROP + 4, P4_SHARD, 8, 20);
  int emit = (int)(E * 4);
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 80 + v3_rndf() * 200; h4_emit(160, (float)hzY, cosf(a) * s, sinf(a) * s * 0.7f, 0.8f + v3_rndf(), (uint8_t)(i & 1 ? H4_PROP + v3_rnd(4) : 64 + (1 + v3_rnd(10)) * 16 + 3), P4_SHARD, (uint8_t)(4 + v3_rnd(8))); }
  h4_parts(dt, 60.0f, 0.3f);
}

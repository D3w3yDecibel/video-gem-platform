// =====================================================================
// PROGRAM: NEURAL — the 2020s (by Dewey) — Set 4, slot 10
//
// The landscape flight as the machines see it: the land as a point cloud,
// a LIDAR scan sweeping over it, heat-map colours, contour lines on black,
// holograms, latent-space pastels, a city made of points, thermal cameras,
// satellite views, night vision and dreamlike AI colours — with a HUD.
//
// EXPLODE (p15, or k12 for one blast): the model loses its grip. The land
// dissolves into floating points that drift apart, data scatters out of the
// horizon and the scan lines break up.
//
// Presets: k0 Point Cloud · k1 Lidar · k2 Heatmap · k3 Topo · k4 Hologram · k5 Latent Space
//          k6 Data City · k7 Thermal · k8 Satellite · k9 Night Vision · k10 Deep Dream · k11 Tour
//
// Knobs: p0 Palette · p1 Speed · p2 Altitude · p3 Turn · p4 Pitch · p5 Mountains · p6 Water Level
//        p7 Scan · p8 Fog · p9 Sun · p10 Draw Distance · p11 HUD · p12 Sky
//        p13 Autopilot · p14 Pulse · p15 EXPLODE
// =====================================================================

#include "vg4.h"

// deco bits: 1 scan sweep · 2 HUD · 4 stars · 8 grid sky
static const H4World nu_worlds[11] = {
  {{{0,0,0},{0,0,5},{5,0,20},{20,5,40}}, {{80,0,160},{255,255,255}}, {{40,0,120},{120,0,200},{220,0,180},{255,80,80},{255,190,0},{255,255,160}}, {0,40,120}, {0,255,255},
   T4_MAP, S4_DOTS, X4_SCATTER, 0, 55, 10, 0, 1, W4_STARS, 1 | 2, 0, 0},                                             // 0 Point Cloud
  {{{0,0,0},{0,5,0},{0,15,5},{0,30,10}}, {{0,120,40},{150,255,180}}, {{0,40,10},{0,80,20},{0,130,40},{40,190,80},{120,240,140},{220,255,230}}, {0,30,30}, {200,255,220},
   T4_MAP, S4_DOTS, X4_SCATTER, 0, 55, 10, 0, 1, 0, 1 | 2, 0, 0},                                                    // 1 Lidar
  {{{0,0,30},{0,20,60},{20,40,90},{60,70,110}}, {{255,60,0},{255,255,200}}, {{0,0,120},{0,120,220},{0,220,120},{240,240,0},{255,120,0},{255,0,0}}, {0,0,90}, {255,255,255},
   T4_MAP, S4_CONTOUR, X4_SCATTER, 0, 60, 20, 0, 1, 0, 2, 0, 0},                                                     // 2 Heatmap
  {{{0,0,0},{0,0,0},{5,5,10},{15,15,25}}, {{120,120,140},{255,255,255}}, {{0,0,0},{5,5,8},{10,10,15},{15,15,22},{20,20,30},{28,28,40}}, {0,0,0}, {255,170,60},
   T4_MAP, S4_CONTOUR, X4_SCATTER, 0, 60, 10, 0, 1, W4_STARS, 2, 0, 0},                                              // 3 Topo
  {{{0,0,10},{0,10,30},{0,40,70},{0,90,120}}, {{0,160,255},{200,255,255}}, {{0,20,40},{0,50,80},{0,90,130},{0,140,180},{60,200,230},{180,250,255}}, {0,10,30}, {0,255,255},
   T4_MAP, S4_WIRE | S4_DOTS, X4_SCATTER, 0, 50, 15, 0, 1, W4_STARS, 1 | 8, 0, 0},                                    // 4 Hologram
  {{{20,10,40},{70,40,110},{180,120,200},{250,200,230}}, {{255,150,220},{255,255,255}}, {{60,30,110},{120,70,170},{200,120,210},{255,170,200},{255,210,180},{255,245,230}}, {40,20,80}, {80,0,160},
   T4_ROLLING, S4_CONTOUR, X4_SCATTER, 0, 45, 25, 0, 1, 0, 0, 0, 0},                                                    // 5 Latent Space
  {{{0,0,0},{0,0,10},{0,10,30},{10,20,50}}, {{0,200,255},{255,255,255}}, {{0,30,60},{0,70,120},{0,120,180},{60,170,230},{150,220,255},{255,255,255}}, {0,0,0}, {255,0,140},
   T4_CITY, S4_DOTS | S4_GRID, X4_SCATTER, 0, 50, 15, 0, 1, W4_STARS, 2 | 8, 0, 0},                                   // 6 Data City
  {{{10,0,20},{40,0,60},{90,0,90},{160,20,90}}, {{255,100,0},{255,255,200}}, {{20,0,60},{80,0,120},{180,0,120},{255,60,0},{255,170,0},{255,255,180}}, {10,0,40}, {255,255,255},
   T4_MAP, 0, X4_SCATTER, 0, 55, 20, 0, 1, 0, 2, 0, 0},                                                              // 7 Thermal
  {{{20,30,40},{50,70,90},{110,130,140},{170,180,180}}, {{255,255,255},{255,255,255}}, {{180,170,130},{110,140,70},{70,110,50},{120,110,90},{170,160,150},{240,240,240}}, {30,70,110}, {255,220,0},
   T4_MAP, S4_GRID, X4_SCATTER, 50, 55, 30, 0, 1, 0, 2, 0, 0},                                                       // 8 Satellite
  {{{0,10,0},{0,30,0},{10,70,10},{40,120,40}}, {{40,180,40},{220,255,220}}, {{0,30,0},{10,60,10},{20,100,20},{40,140,40},{90,190,90},{180,255,180}}, {0,40,0}, {200,255,200},
   T4_MAP, S4_EDGES, X4_SCATTER, 30, 55, 30, 0, 1, W4_STARS, 2, 0, 0},                                               // 9 Night Vision
  {{{40,0,60},{140,20,120},{255,120,60},{255,220,120}}, {{255,0,200},{255,255,200}}, {{40,80,200},{240,40,140},{255,180,0},{0,200,160},{180,60,255},{255,255,255}}, {80,0,120}, {0,255,200},
   T4_MAP, S4_CONTOUR, X4_SCATTER, 30, 60, 20, 0, 1, 0, 1, 0, 0},                                          // 10 Deep Dream
};
static const char* const nu_presetNames[12] = {
  "Point Cloud", "Lidar", "Heatmap", "Topo", "Hologram", "Latent Space", "Data City", "Thermal", "Satellite", "Night Vision", "Deep Dream", "Tour"
};
const char* prog_neural_name() { return "2020s NEURAL"; }
const char* prog_neural_character() { return "The 2020s: the land as point clouds, scans, heat maps and holograms; EXPLODE dissolves it into drifting data"; }
const char* prog_neural_presetName(int preset) { return (preset >= 0 && preset < 12) ? nu_presetNames[preset] : NULL; }
const char* prog_neural_potLabel(int preset, int pot) {
  (void)preset;
  if (pot == 7) return "Scan";
  if (pot == 11) return "HUD";
  return (pot >= 0 && pot < 16) ? h4_labelsBase[pot] : "";
}
uint8_t prog_neural_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }
void prog_neural_init() { h4_init(nu_worlds, 11); }

void prog_neural_draw(int preset) {
  if (!h4_prepare(preset, 7.0f, 1.0f, 1.0f)) return;
  const H4World& wp = nu_worlds[h4_pal];
  float dt = h4_dt, E = h4_ex;
  int hzY = h4_hzY, deco = wp.deco;
  float scan = v3_centre(7) ? ((deco & 1) ? 1.0f : 0.0f) : v3_u(7);
  float hud = v3_centre(11) ? ((deco & 2) ? 1.0f : 0.0f) : v3_u(11);
  h4_prop(0, 0, 255, 200); h4_prop(1, 255, 255, 255); h4_prop(2, 255, 60, 140); h4_prop(3, 0, 120, 100);   // HUD
  h4_prop(4, 255, 255, 255); h4_prop(5, 0, 255, 255); h4_prop(6, 255, 0, 200); h4_prop(7, 255, 220, 0);    // data

  // ── sky ──
  h4_sky(hzY, 110);
  if ((wp.flags & W4_STARS) && h4k.skyK > 0.1f) h4_stars((int)(50 * h4k.skyK), hzY, 111);
  if ((deco & 8) && h4k.skyK > 0.05f) {                             // a wireframe dome over the sky
    for (int k = 1; k < 6; k++) { int y = hzY - k * k * 4; if (y < 0) break; for (int x = 0; x < W; x += 3) v3_put(x, y, H4_PROP + 3); }
    for (int k = -8; k <= 8; k++) v3_line(160 + k * 10, hzY, 160 + k * 40, 0, H4_PROP + 3);
  }
  if (E > 0.1f) h4_glow(160, hzY, (int)(10 + E * 30), (int)(4 + E * 5));

  // ── ground: dissolving into points ──
  h4_ground(hzY, h4_rollPx);

  // ── the scan sweep: a bright line rolling over the land, leaving a lit band behind it ──
  if (scan > 0.02f) {
    float u = fmodf(h4_t * 0.35f, 1.0f);
    int y0 = hzY + 2 + (int)(u * u * (H - hzY));
    for (int k = 0; k < 2 + (int)(scan * 3); k++) {
      int y = y0 - k; if ((unsigned)y >= H) continue;
      uint8_t* p = v3_buf + y * W;
      for (int x = 0; x < W; x++) if (p[x] >= 64 || k == 0) p[x] = k == 0 ? (uint8_t)(H4_PROP + 0) : (uint8_t)(64 + 11 * 16 + 3);
    }
  }
  // ── data scattering out of the horizon ──
  if (h4_trigger) h4_burstAt(160, (float)hzY, 80, 220, H4_PROP + 5, P4_DOT, 1, 10);
  int emit = (int)(E * 8);
  for (int i = 0; i < emit; i++) { float a = v3_rndf() * 6.2831853f, s = 30 + v3_rndf() * 150; h4_emit(160 + (v3_rndf() - 0.5f) * 200 * E, (float)hzY + (v3_rndf() - 0.3f) * 60 * E, cosf(a) * s, sinf(a) * s * 0.5f - 20, 1.0f + v3_rndf() * 2, (uint8_t)(H4_PROP + 4 + v3_rnd(4)), P4_DOT, (uint8_t)(1 + v3_rnd(2))); }
  h4_parts(dt, -10.0f, 0.2f);
  // broken scan lines
  if (E > 0.3f) for (int i = 0; i < (int)(E * 12); i++) { int y = v3_rnd(H), x = v3_rnd(W), l = 20 + v3_rnd(80); v3_hspan(x, x + l, y, (uint8_t)(H4_PROP + 5 + v3_rnd(2))); }
  // ── the HUD ──
  if (hud > 0.02f) {
    uint8_t c = H4_PROP + 0;
    int m = 12;
    v3_line(m, m, m + 20, m, c); v3_line(m, m, m, m + 20, c); v3_line(W - m, m, W - m - 20, m, c); v3_line(W - m, m, W - m, m + 20, c);
    v3_line(m, H - m, m + 20, H - m, c); v3_line(m, H - m, m, H - m - 20, c); v3_line(W - m, H - m, W - m - 20, H - m, c); v3_line(W - m, H - m, W - m, H - m - 20, c);
    v3_line(150, 120, 156, 120, c); v3_line(164, 120, 170, 120, c); v3_line(160, 110, 160, 116, c); v3_line(160, 124, 160, 130, c);
    int head = ((int)(h4_head * 163) % 1024 + 1024) % 1024;
    for (int k = -6; k <= 6; k++) { int x = 160 + k * 18 - (head % 18); v3_line(x, 22, x, (((head / 18 + k) % 5) == 0) ? 30 : 26, c); }   // a heading tape
    char buf[8]; int alt = (int)h4_alt; buf[0] = 'A'; buf[1] = (char)('0' + (alt / 100) % 10); buf[2] = (char)('0' + (alt / 10) % 10); buf[3] = (char)('0' + alt % 10); buf[4] = 0;
    v3_text(W - 50, H - 30, buf, 1, H4_PROP + 1);
    if (hud > 0.5f) { int n = (int)(E * 99); buf[0] = 'E'; buf[1] = (char)('0' + n / 10); buf[2] = (char)('0' + n % 10); buf[3] = 0; v3_text(24, H - 30, buf, 1, E > 0.5f ? H4_PROP + 2 : H4_PROP + 1); }
  }
}

// =====================================================================
// PROGRAM: GIRIH — Islamic star patterns that breathe (by Dewey) — Set 6, slot 3
//
// The geometric strapwork of the Alhambra, Fez and Isfahan: bands of light
// weaving six-, eight- and twelve-pointed stars, with the spaces between
// them filled like glazed tiles. The patterns are made the way Hankin
// discovered the old masters made them: from the middle of every edge of a
// grid of hexagons (or squares), two straps set off at a "contact angle".
// Change that angle and the whole pattern melts from one star into another —
// so here it slowly sweeps, and the stars open and close like flowers.
//
// How it works: like SIREN, every pixel looks up where it sits in its
// hexagon or square (one folded wedge of it), and a table worked out once a
// frame gives the colour for every spot in that wedge.
//
// Presets: k0 Alhambra · k1 Fez · k2 Isfahan · k3 Samarkand · k4 Zellige ·
//          k5 Breathing Stars · k6 Rose Window · k7 Night Mosque · k8 Cairo ·
//          k9 Weave · k10 Gilded · k11 Tour
//   k12 — TURN: the contact angle swings through a whole cycle of stars
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Star (the contact angle: ◄ open | sharp ►) · p6 Strap Width · p7 Morph
//   p8 Second Strap · p9 Spin · p10 Drift · p11 Tile Glaze (fill brightness)
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

// sq: 0 hexagons, 1 squares · ang: contact angle (degrees) · morph: how far it sweeps (degrees)
// f2: second strap family 0-100 (its angle a2) · w: strap width ×100 · glaze: fill brightness 0-15
struct GrLook { uint8_t sq, pal, ang, morph, f2, a2, w, glaze, spin, drift, size, flow, spread; };
static const GrLook gr_looks[11] = {
  // sq pal ang morph f2  a2  w  glaze spin drift size flow spread
  {  0,  2, 60, 15,  0,  0, 12, 7,  4, 10, 60, 4, 50 },   // Alhambra
  {  1,  6, 67, 12,  0,  0, 11, 7,  0, 12, 70, 4, 50 },   // Fez
  {  0,  4, 70, 10, 60, 40, 10, 8,  5,  8, 64, 5, 60 },   // Isfahan
  {  0,  0, 55, 20, 80, 20, 9,  6,  6, 10, 56, 6, 70 },   // Samarkand
  {  1,  2, 45, 10, 70, 30, 10, 8,  0, 10, 76, 5, 70 },   // Zellige
  {  0,  3, 55, 30,  0,  0, 11, 7,  3,  6, 60, 4, 60 },   // Breathing Stars
  {  0,  8, 75, 10,100, 15, 8,  8, 10,  0, 90, 6, 90 },   // Rose Window
  {  0, 10, 62, 18, 50, 35, 7,  2,  4,  8, 60, 6, 80 },   // Night Mosque
  {  1,  9, 60, 12,  0,  0, 13, 7,  0, 10, 74, 4, 50 },   // Cairo
  {  1, 11, 50, 35, 90, 45, 9,  6,  6, 14, 64, 8,100 },   // Weave
  {  0,  1, 66, 14, 40, 25, 12, 5,  3,  6, 66, 3, 50 },   // Gilded
};
static const char* const gr_names[12] = { "Alhambra", "Fez", "Isfahan", "Samarkand", "Zellige", "Breathing Stars", "Rose Window", "Night Mosque", "Cairo", "Weave", "Gilded", "Tour" };
static const char* const gr_lbl[7] = { "Star (Contact Angle)", "Strap Width", "Morph", "Second Strap", "Spin", "Drift", "Tile Glaze" };
const char* prog_girih_name() { return "GIRIH"; }
const char* prog_girih_character() { return "Islamic star patterns that breathe: Hankin strapwork over glazed tiles, morphing from star to star"; }
const char* prog_girih_presetName(int p) { return (p >= 0 && p < 12) ? gr_names[p] : NULL; }
const char* prog_girih_potLabel(int p, int pot) { (void)p; return v6_label(pot, gr_lbl); }
uint8_t prog_girih_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static uint8_t* gr_rt;                 // colour of every spot in the folded cell, this frame
static uint8_t* gr_rtB;                // … for the corner (B) hexagons, tinted differently
static void gr_mem() { gr_rt = v6_mem; gr_rtB = v6_mem + V6_QN + 3; }

static float gr_ang, gr_morph, gr_f2, gr_a2, gr_w, gr_glaze, gr_spin, gr_drift, gr_size;
static float gr_rot = 0, gr_panU = 0, gr_panV = 0, gr_ph = 0, gr_kick = 0;
static int gr_sqNow = 0;

void prog_girih_init() { gr_mem(); v6_init(); gr_ph = 0; gr_rot = 0; gr_kick = 0; }

// the colour of each spot in the folded wedge. X, Y in units where the edge's midpoint is (255, 0)
// (hexagon: the wedge runs 0° … 30°; square: 0° … 45°)
static void gr_table(int sq, float th1, float th2, float f2, float w, int glaze, int hueB) {
  // strap 1: from the edge's midpoint M = (255, 0), at th1 to the edge
  int c1 = (int)(cosf(th1) * 256), s1 = (int)(sinf(th1) * 256);
  // strap 2: from the corner V, at th2 off the line to the centre
  float vy = sq ? 255.0f : 147.2f, va = atan2f(vy, 255.0f) + th2;
  int c2 = (int)(cosf(va) * 256), s2 = (int)(sinf(va) * 256), vyi = (int)vy;
  int wi = (int)(w * 255), w2 = (int)(w * 255 * 0.8f), edge = 5 + wi / 5;
  int two = (int)(f2 * 100.0f) > 5;                                   // (an int: a float test here would be redone for every entry)
  for (int j = 0; j < V6_Q; j++) for (int i = 0; i < V6_Q; i++) {
    int idx = j * V6_Q + i, X, Y;
    if (sq) { int a = i * 4, b = j * 4; if (a > 255) a = 255; if (b > 255) b = 255; X = a > b ? a : b; Y = a > b ? b : a; }
    else { int e = v6_geoWedge[idx]; X = e & 255; Y = (((e >> 8) & 127) * 297) >> 8; }
    // signed distances to the two strap lines
    int d1 = ((X - 255) * c1 + Y * s1) >> 8;                            // (P − M) × (−sin, cos): <0 on the star's side
    int d2 = two ? (((X - 255) * s2 - (Y - vyi) * c2) >> 8) : 9999;
    int a1 = v3_abs(d1), a2 = v3_abs(d2);
    int lev, hue;
    if (a1 < wi && (a1 <= a2 || a2 >= w2)) {                            // on strap 1 (it goes over strap 2)
      lev = 15 - (a1 * 6) / (wi + 1); hue = 0;
      if (a1 > wi - edge) lev = 3;                                      // its carved edge
    } else if (two && a2 < w2) {                                        // on strap 2
      lev = 13 - (a2 * 6) / (w2 + 1); hue = 8;
      if (a2 > w2 - edge) lev = 2;
      if (a1 < wi + edge) lev = 1;                                      // tucked under strap 1
    } else {                                                             // a tile between the straps
      int r = X * X + Y * Y;
      int region = (d1 < 0 ? 0 : 1) + (two && d2 < 0 ? 2 : 0);
      static const uint8_t rh[4] = { 4, 11, 6, 13 };
      hue = rh[region] + (r < 120 * 120 ? 2 : 0);
      lev = glaze + (region == 0 ? 1 : 0) - (r > 200 * 200 ? 1 : 0);
      int dd = v6_min(a1 - wi, two ? a2 - w2 : 9999);                    // a dark grout line round each tile
      if (dd < edge + 3) lev = v6_min(lev, 1);
    }
    lev = v3_clamp(lev, 0, 15);
    int hB = (hue + (hue >= 4 ? hueB : 0)) & 15;                        // the second tint
    // hexagons: whether a spot belongs to a corner (B) hexagon is fixed by where it sits, so bake the tint in
    bool B = !sq && (v6_geoWedge[idx] & 0x8000);
    gr_rt[idx] = (uint8_t)((lev << 4) | (B ? hB : (hue & 15)));
    gr_rtB[idx] = (uint8_t)((lev << 4) | hB);
  }
}

__attribute__((optimize("O2"))) static void gr_row(uint8_t* row, uint32_t U, uint32_t V, int32_t du, int32_t dv, int hb, int sq) {
  const uint8_t* rt = gr_rt; const uint8_t* rb = gr_rtB;
  (void)rb;
  if (sq) {                                                             // squares: alternate cells get the second tint
    for (int x = 0; x < W; x++, U += du, V += dv) {
      int c = (((U ^ V) >> 16) & 1) ? rb[V6_QIDX(U, V)] : rt[V6_QIDX(U, V)];
      row[x] = (uint8_t)((c & 0xF0) | ((c + hb) & 15));
    }
  } else {                                                              // hexagons: the corner (B) cells get the second tint
    for (int x = 0; x < W; x++, U += du, V += dv) {
      int c = rt[V6_QIDX(U, V)];
      row[x] = (uint8_t)((c & 0xF0) | ((c + hb) & 15));
    }
  }
}

void prog_girih_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  gr_mem();
  const GrLook& L = gr_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  float tAng = v3_centre(5) ? L.ang : 35.0f + v3_u(5) * 50.0f;
  float tW = v3_centre(6) ? L.w / 100.0f : 0.03f + v3_u(6) * 0.22f;
  float tMorph = v3_centre(7) ? L.morph : v3_u(7) * 40.0f;
  float tF2 = v3_centre(8) ? L.f2 / 100.0f : v3_u(8);
  float tSpin = v3_centre(9) ? L.spin / 100.0f : v3_bi(9) * 0.5f;
  float tDrift = v3_centre(10) ? L.drift / 100.0f : v3_u(10) * 0.6f;
  float tGlaze = v3_centre(11) ? L.glaze : v3_u(11) * 11.0f;
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&gr_ang, tAng, rate); v6_ease(&gr_w, tW, rate); v6_ease(&gr_morph, tMorph, rate); v6_ease(&gr_f2, tF2, rate);
  v6_ease(&gr_a2, (float)L.a2, rate); v6_ease(&gr_spin, tSpin, rate); v6_ease(&gr_drift, tDrift, rate);
  v6_ease(&gr_glaze, tGlaze, rate); v6_ease(&gr_size, (float)L.size, rate);
  if (v6_first || v6_fade > 0.5f) gr_sqNow = L.sq;                       // squares ↔ hexagons halfway through a fade
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 140);

  // ── the breathing: the contact angle sweeps; k12 swings it through a whole turn of stars ──
  gr_ph += sdt * 0.22f;
  if (v6_k12hit) gr_kick = 1.0f;
  gr_kick -= dt * 0.25f; if (gr_kick < 0) gr_kick = 0;
  float kick = gr_kick > 0 ? sinf(gr_kick * 3.14159f) * 40.0f : 0.0f;
  float ang = gr_ang + gr_morph * sinf(gr_ph) + kick;
  ang = v3_clampf(ang, 18.0f, 88.0f);
  float th1 = ang * 0.0174533f, th2 = (gr_a2 + 10.0f * sinf(gr_ph * 0.7f + 1.0f)) * 0.0174533f;
  // squares fade out while the shape changes over, so the switch is hidden
  float fadeK = v6_fade < 1.0f ? fabsf(v6_fade - 0.5f) * 2.0f : 1.0f;
  gr_table(gr_sqNow, th1, th2, gr_f2, gr_w * (0.85f + 0.15f * sinf(gr_ph * 1.3f)), (int)(gr_glaze * fadeK), 5);

  // ── the view ──
  gr_rot += sdt * gr_spin * 0.2f;
  gr_panU += sdt * gr_drift * 0.25f; gr_panV += sdt * gr_drift * 0.13f;
  float sp = gr_size * v6_zoom(1.0f);
  V6View a;
  v6_view(&a, sp, gr_rot, gr_panU, gr_panV, 0, 0);
  if (gr_sqNow) { a.dv_x = (int32_t)(a.dv_x * 1.7320508f); a.dv_y = (int32_t)(a.dv_y * 1.7320508f); }   // squares: the same scale both ways
  int hbK = (int)(v6_f.spread > 0 ? 3 : 0);
  for (int y = 0; y < H; y++) {
    int32_t sy = y - 120;
    int hb = ((y * hbK) >> 7) & 15;                                     // a gentle colour band down the screen
    gr_row(v3_buf + y * W, a.u0 + (uint32_t)(sy * a.du_y) - (uint32_t)(160 * a.du_x), a.v0 + (uint32_t)(sy * a.dv_y) - (uint32_t)(160 * a.dv_x), a.du_x, a.dv_x, hb, gr_sqNow);
  }
  if (fadeK < 0.999f) {                                                 // dim during the swap
    int k = (int)((1.0f - fadeK) * 12);
    if (k > 0) v3_fadeLev(k);
  }
  v6_end();
}

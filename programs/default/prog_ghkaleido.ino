// =====================================================================
// PROGRAM: KALEIDO — liquid kaleidoscope (by Dewey) — Set 8 slot 3
//
// Greatest hit from Set 6 (KALEIDO), Set 4 (1960s PSYCHEDELIC) and RMNA's
// Symmetry: a slow liquid pattern seen through a ring of mirrors. Melting
// plasma, glass beads, ripples or shards, folded into 4 to 16 mirrored
// wedges that turn like a mandala breathing.
//
// How it works: every cell of the 80 × 60 grid knows its angle and distance
// from the centre (tables made once). The angle is folded into one mirrored
// wedge, turned back into x/y inside that wedge, and the pattern is looked
// up there — so it is drawn once and the mirrors make it whole.
//
// Presets: k0 Liquid · k1 Beads · k2 Ripples · k3 Shards · k4 Lotus · k5 Temple ·
//          k6 Jelly · k7 Prism · k8 Mandala · k9 Garden · k10 Ember · k11 Tour
//   k12 — DROP: a ring of light spreads out from the middle
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Mirrors · p6 Pattern · p7 Detail · p8 Turn · p9 Flow · p10 Glow
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8KA_PLASMA, H8KA_BEADS, H8KA_RIPPLES, H8KA_SHARDS, H8KA_NPAT };
struct H8KaLook { uint8_t mirrors, pat, detail, turn, flow, glow, pal, fl, spread; int8_t key; };
static const H8KaLook h8ka_looks[11] = {
  // mir pattern        detail turn flow glow pal          fl spr key
  {  6, H8KA_PLASMA,     45,   55,  45,  55, V8P_LOUNGE,   3,  90,  0 },   // Liquid
  {  8, H8KA_BEADS,      50,   60,  40,  60, V8P_VAPOR,    3,  80,  0 },   // Beads
  {  6, H8KA_RIPPLES,    50,   55,  45,  55, V8P_FIREICE,   2,  70,  0 },   // Ripples
  { 12, H8KA_SHARDS,     45,   60,  40,  55, V8P_SHERBET,  3, 100,  0 },   // Shards
  {  8, H8KA_PLASMA,     40,   55,  40,  60, V8P_SUNSET,   2,  80,  0 },   // Lotus
  { 12, H8KA_BEADS,      60,   55,  35,  60, V8P_NEON,   2,  70,  0 },   // Temple
  {  4, H8KA_RIPPLES,    40,   50,  40,  55, V8P_LAGOON,   2,  60,  0 },   // Jelly
  {  6, H8KA_SHARDS,     50,   60,  45,  55, V8P_AURORA,   3,  90,  0 },   // Prism
  { 12, H8KA_PLASMA,     55,   60,  40,  55, V8P_RAINBOW,     3,  90,  0 },   // Mandala
  {  6, H8KA_BEADS,      45,   55,  40,  55, V8P_FOREST,   2,  70,  0 },   // Garden
  {  8, H8KA_RIPPLES,    50,   55,  45,  60, V8P_EMBER,    2,  50,  0 },   // Ember
};
static const char* const h8ka_names[12] = { "Liquid", "Beads", "Ripples", "Shards", "Lotus", "Temple", "Jelly", "Prism", "Mandala", "Garden", "Ember", "Tour" };
static const char* const h8ka_lbl[6] = { "Mirrors", "Pattern", "Detail", "Turn", "Flow", "Glow" };
const char* prog_ghkaleido_name() { return "KALEIDO"; }
const char* prog_ghkaleido_character() { return "Liquid kaleidoscope: melting colour folded through a slowly turning ring of mirrors"; }
const char* prog_ghkaleido_presetName(int p) { return (p >= 0 && p < 12) ? h8ka_names[p] : NULL; }
const char* prog_ghkaleido_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8ka_lbl); }
uint8_t prog_ghkaleido_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static const uint8_t h8ka_mirrorChoice[7] = { 3, 4, 6, 8, 12, 16, 24 };
static float h8ka_detail = 0.5f, h8ka_turn = 0.5f, h8ka_flow = 0.5f, h8ka_glow = 0.5f;
static float h8ka_rot = 0, h8ka_t = 0, h8ka_drop = -1;
void prog_ghkaleido_init() { v8_init(); v8_polarOK = false; h8ka_drop = -1; }

void prog_ghkaleido_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  v8_polar();
  const H8KaLook& L = h8ka_looks[look];
  float rate = v8_rate();
  v6_ease(&h8ka_detail, v8_kn(7, L.detail / 100.0f), rate);
  v6_ease(&h8ka_turn, v3_centre(8) ? L.turn / 100.0f : v3_u(8), rate);
  v6_ease(&h8ka_flow, v8_kn(9, L.flow / 100.0f), rate);
  v6_ease(&h8ka_glow, v8_kn(10, L.glow / 100.0f), rate);
  int nm = v3_centre(5) ? L.mirrors : h8ka_mirrorChoice[v3_clamp(v3_k(5) * 7 / 1024, 0, 6)];
  int pat = v3_pick(6, L.pat, H8KA_NPAT);
  v8_colours(L.pal, L.fl / 100.0f, L.spread / 100.0f, 90, L.key / 100.0f);

  h8ka_rot += v6_f.sdt * (h8ka_turn - 0.5f) * fabsf(h8ka_turn - 0.5f) * 800.0f;              // centre of p8 = still … either way it turns
  if (h8ka_rot > 1024.0f) h8ka_rot -= 1024.0f; if (h8ka_rot < 0) h8ka_rot += 1024.0f;
  h8ka_t += v6_f.sdt * (0.05f + h8ka_flow * h8ka_flow * 4.0f);
  if (h8ka_t > 5000.0f) h8ka_t -= 5000.0f;
  if (v6_k12hit) h8ka_drop = 0;
  int dropR = -1000;
  if (h8ka_drop >= 0) { h8ka_drop += v6_f.dt * 0.25f; if (h8ka_drop > 1.0f) h8ka_drop = -1; else dropR = (int)(h8ka_drop * 230); }

  int rot = (int)h8ka_rot;
  int zk = (int)(256.0f / v8_zoom(1.0f));                          // radius scale, Q8
  int dk = 3 + (int)(h8ka_detail * 45);                              // pattern frequency
  float t = h8ka_t;
  int ph1 = (int)(t * 90), ph2 = (int)(t * 70), ph3 = (int)(t * 55), ph4 = (int)(t * 120);
  int s1u = (int)(40 * sinf(t * 0.13f)), s1v = (int)(30 * sinf(t * 0.17f + 1)), s2u = (int)(60 + 30 * sinf(t * 0.11f + 2)), s2v = (int)(20 * cosf(t * 0.09f));
  int con = 60 + (int)(h8ka_glow * 230), breathe = (int)(t * 40);
  const uint16_t* A = V8_ANG; const uint8_t* R = V8_RAD;
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  for (int k = 0; k < V8_GN; k++) {
    int a = (A[k] + rot) & 1023;
    int m = (a * nm) & 2047; if (m > 1024) m = 2048 - m;
    int al = m / nm;                                                // angle inside the wedge
    int r = (R[k] * zk) >> 8;
    int u = (r * v3_cos(al)) >> 14, v = (r * v3_sin(al)) >> 14;     // px, inside the wedge
    int32_t f; int lev, hue;
    switch (pat) {
      case H8KA_BEADS: {
        int cu = ((u * dk) >> 2) + (ph1 >> 2), cv = ((v * dk) >> 2) - (ph2 >> 3);
        int bu = 128 - v3_abs((cu & 255) - 128), bv = 128 - v3_abs((cv & 255) - 128);   // 0 … 128
        int b = (bu * bv) >> 6;                                     // 0 … 256
        b = b > 60 ? (b - 60) * 256 / 196 : 0;
        lev = (b * con) >> 8; if (lev > 200) lev = 200;
        lev += (b > 200 ? 26 : 0);
        hue = ((cu >> 8) * 3 + (cv >> 8) * 5) * 16 + (r >> 1);
        break; }
      case H8KA_RIPPLES: {
        int d1 = v3_abs(u - s1u) + v3_abs(v - s1v), d2 = v3_abs(u - s2u) + v3_abs(v - s2v);
        f = v3_sin(d1 * dk - ph4) + v3_sin(d2 * dk - ph1);           // ±32768
        lev = 113 + (((f >> 7) * con) >> 8) * 113 / 200;
        hue = (f >> 9) + (r >> 1);
        break; }
      case H8KA_SHARDS: {
        int p = ((u * dk) >> 3) + ((v * dk) >> 2) + ph2;
        int q = ((v * dk) >> 2) - ((u * dk) >> 4) - ph3;
        int tri = v3_abs((p & 511) - 256), tq = v3_abs((q & 511) - 256);   // 0 … 256
        int e = tri < tq ? tri : tq;
        lev = 40 + ((e * con) >> 8) * 186 / 270;
        hue = ((p >> 9) * 4 + (q >> 9) * 7) * 16 + (r >> 2);
        break; }
      default: {
        f = v3_sin(u * dk + ph1) + v3_sin(v * dk - ph2) + v3_sin(((u + v) * dk >> 1) + ph3) + v3_sin(r * dk - ph4);
        lev = 113 + (((f >> 8) * con) >> 8) * 113 / 200;            // f ±65536
        hue = (f >> 9) + (r >> 1) + breathe;
        break; }
    }
    if (dropR > -999) { int dd = v3_abs(r - dropR); if (dd < 24) lev += (24 - dd) * 6; }
    LEV[k] = (uint8_t)v3_clamp(lev, 0, 226); HUE[k] = (uint8_t)(hue & 255);
  }
  v8_upscale(6);
  v8_end();
}

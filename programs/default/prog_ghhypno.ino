// =====================================================================
// PROGRAM: HYPNO — hypnotic spirals & Julia dreams (by Dewey) — Set 8 slot 4
//
// Greatest hit from Set 1 (Hypnotic: Julia Dream, Hypno Spiral, Op Waves),
// Set 3 (WORMHOLE) and Set 7 (ROTO): spirals winding in, rings rippling out,
// a soft tunnel, petals, and the Julia set drifting through its shapes.
// All with soft cosine bands instead of hard black-and-white, and slow.
//
// How it works: the 80 × 60 grid's angle/distance tables give each cell a
// phase (distance × rings + angle × arms − time); its cosine is the light.
// The Julia set is iterated in fixed point on the same grid (22 steps), its
// constant c gliding round the classic "Julia dream" circle.
//
// Presets: k0 Hypno Spiral · k1 Julia Dream · k2 Ripple Rings · k3 Tunnel ·
//          k4 Petals · k5 Op Waves · k6 Twin Spiral · k7 Julia Lagoon ·
//          k8 Deep Tunnel · k9 Lotus · k10 Galaxy · k11 Tour
//   k12 — REVERSE: the motion glides to a stop and flows back the other way
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Arms · p6 Pattern · p7 Rings · p8 Twist · p9 Pull · p10 Contrast
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8HY_SPIRAL, H8HY_JULIA, H8HY_RINGS, H8HY_TUNNEL, H8HY_PETALS, H8HY_OPWAVE, H8HY_NPAT };
struct H8HyLook { uint8_t pat, arms, rings, twist, pull, contrast, pal, flow, spread; int8_t key; };
static const H8HyLook h8hy_looks[11] = {
  // pattern        arms rings twist pull contr pal          flow spr key
  { H8HY_SPIRAL,     3,  45,   40,  50,  70, V8P_VAPOR,    3,  80,  0 },   // Hypno Spiral
  { H8HY_JULIA,      1,  50,   50,  40,  60, V8P_LOUNGE,   3,  90,  0 },   // Julia Dream
  { H8HY_RINGS,      0,  45,   30,  45,  65, V8P_MIAMI,   2,  70,  0 },   // Ripple Rings
  { H8HY_TUNNEL,     6,  40,   40,  50,  70, V8P_DUSK,     3,  80,  0 },   // Tunnel
  { H8HY_PETALS,     6,  45,   30,  40,  65, V8P_SUNSET,   2,  80,  0 },   // Petals
  { H8HY_OPWAVE,     5,  55,   40,  45,  70, V8P_INFRARED,    2,  60,  0 },   // Op Waves
  { H8HY_SPIRAL,     2,  35,   60,  45,  70, V8P_SHERBET,  3, 100,  0 },   // Twin Spiral
  { H8HY_JULIA,      1,  60,   50,  35,  60, V8P_LAGOON,   2,  80,  0 },   // Julia Lagoon
  { H8HY_TUNNEL,     8,  55,   60,  60,  70, V8P_ELECTRIC,   3,  90,  0 },   // Deep Tunnel
  { H8HY_PETALS,     8,  40,   40,  40,  65, V8P_VELVET,   2,  70,  0 },   // Lotus
  { H8HY_SPIRAL,     4,  50,   80,  45,  70, V8P_LOUNGE,   3,  90,  0 },   // Galaxy
};
static const char* const h8hy_names[12] = { "Hypno Spiral", "Julia Dream", "Ripple Rings", "Tunnel", "Petals", "Op Waves", "Twin Spiral", "Julia Lagoon", "Deep Tunnel", "Lotus", "Galaxy", "Tour" };
static const char* const h8hy_lbl[6] = { "Arms", "Pattern", "Rings", "Twist", "Pull", "Contrast" };
const char* prog_ghhypno_name() { return "HYPNO"; }
const char* prog_ghhypno_character() { return "Hypnotic spirals, rings, tunnels and a drifting Julia set, soft and slow"; }
const char* prog_ghhypno_presetName(int p) { return (p >= 0 && p < 12) ? h8hy_names[p] : NULL; }
const char* prog_ghhypno_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8hy_lbl); }
uint8_t prog_ghhypno_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static float h8hy_rings = 0.5f, h8hy_twist = 0.5f, h8hy_pull = 0.5f, h8hy_con = 0.6f, h8hy_arms = 3;
static float h8hy_ph = 0, h8hy_dir = 1, h8hy_dirT = 1, h8hy_ang = 0, h8hy_jt = 0;
void prog_ghhypno_init() { v8_init(); v8_polarOK = false; h8hy_dir = h8hy_dirT = 1; }

#define H8HY_JMAX 24
static uint16_t h8hy_ll[64];                                          // 256·log2(½·log2 m) for the smooth escape count
static bool h8hy_llOK = false;
// the Julia set on the grid: c = 0.7885·e^(iθ), smooth ("continuous escape") colouring
static void h8hy_julia(float zoom, int con) {
  if (!h8hy_llOK) { for (int m = 0; m < 64; m++) h8hy_ll[m] = m < 4 ? 0 : (uint16_t)(256.0f * log2f(0.5f * log2f((float)m))); h8hy_llOK = true; }
  int32_t cx = (int32_t)(0.7885f * cosf(h8hy_jt) * 4096), cy = (int32_t)(0.7885f * sinf(h8hy_jt) * 4096);
  int32_t dz = (int32_t)(2.7f / V8_GW * 4096 / zoom);
  float rr = h8hy_ang * 0.0061f;                                     // a slow turn of the whole picture
  int32_t rc = (int32_t)(cosf(rr) * 4096), rs = (int32_t)(sinf(rr) * 4096);
  float gm = 0.8f + (260 - con) / 600.0f;                              // dark far out, glowing near the set
  static uint8_t lut[H8HY_JMAX + 2];
  for (int n = 0; n <= H8HY_JMAX + 1; n++) lut[n] = (uint8_t)(226.0f * powf(v3_clampf(n / 16.0f, 0.0f, 1.0f), gm));
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int hueT = (int)(h8hy_ph * 0.25f);
  for (int j = 0; j < V8_GH; j++) {
    int32_t y0 = (2 * j + 1 - V8_GH) * dz / 2;
    for (int i = 0; i < V8_GW; i++) {
      int32_t x0 = (2 * i + 1 - V8_GW) * dz / 2;
      int32_t zx = (x0 * rc - y0 * rs) >> 12, zy = (x0 * rs + y0 * rc) >> 12;
      int n = 0; int32_t m = 0;
      for (; n < H8HY_JMAX; n++) {
        if ((uint32_t)(zx + 46000) > 92000u || (uint32_t)(zy + 46000) > 92000u) { m = 63 << 12; break; }   // (keeps zx² inside 32 bits)
        int32_t x2 = (zx * zx) >> 12, y2 = (zy * zy) >> 12;
        m = x2 + y2;
        if (m > (16 << 12)) break;                                   // escape radius 4: smoother bands
        zy = ((zx * zy) >> 11) + cy; zx = x2 - y2 + cx;
      }
      int k = j * V8_GW + i;
      if (n >= H8HY_JMAX) { LEV[k] = (uint8_t)(12 + v3_clamp((int)(m >> 9), 0, 80)); HUE[k] = (uint8_t)(hueT + 128 + (m >> 10)); continue; }   // inside: dim, with a little structure
      int nu = (n + 1) * 256 - h8hy_ll[v3_clamp((int)(m >> 12), 4, 63)];   // smooth count × 256
      if (nu < 0) nu = 0;
      int ni = v3_clamp(nu >> 8, 0, H8HY_JMAX), f = nu & 255;
      LEV[k] = (uint8_t)(lut[ni] + (((lut[ni + 1] - lut[ni]) * f) >> 8));
      HUE[k] = (uint8_t)((nu * 11 >> 8) + hueT);
    }
  }
}

void prog_ghhypno_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8HyLook& L = h8hy_looks[look];
  int pat = v3_pick(6, L.pat, H8HY_NPAT);
  if (pat != H8HY_JULIA) v8_polar();
  float rate = v8_rate();
  v6_ease(&h8hy_arms, v3_centre(5) ? (float)L.arms : (float)(int)(v3_u(5) * 12.99f), rate);
  v6_ease(&h8hy_rings, v8_kn(7, L.rings / 100.0f), rate);
  v6_ease(&h8hy_twist, v8_kn(8, L.twist / 100.0f), rate);
  v6_ease(&h8hy_pull, v8_kn(9, L.pull / 100.0f), rate);
  v6_ease(&h8hy_con, v8_kn(10, L.contrast / 100.0f), rate);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 90, L.key / 100.0f);
  if (v6_k12hit) h8hy_dirT = -h8hy_dirT;
  v6_ease(&h8hy_dir, h8hy_dirT, 0.5f);
  float sp = v6_f.sdt * h8hy_dir;
  h8hy_ph += sp * (20.0f + h8hy_pull * h8hy_pull * 1100.0f);
  h8hy_ang += sp * 12.0f;
  h8hy_jt += sp * 0.07f * (0.4f + h8hy_pull);
  if (h8hy_ph > 1.0e6f || h8hy_ph < -1.0e6f) h8hy_ph = 0;
  if (h8hy_ang > 1024.0f) h8hy_ang -= 1024.0f; if (h8hy_ang < 0) h8hy_ang += 1024.0f;
  float zoom = v8_zoom(1.0f);
  int con = 60 + (int)(h8hy_con * 200);
  if (pat == H8HY_JULIA) { h8hy_julia(zoom, con); v8_upscale(6); v8_end(); return; }

  const uint16_t* A = V8_ANG; const uint8_t* R = V8_RAD;
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int zk = (int)(256.0f / zoom);
  int K = 6 + (int)(h8hy_rings * 130);                               // phase per px of radius (1024 per band)
  int arms = (int)(h8hy_arms + 0.5f);
  int T = (int)h8hy_ph, rot = (int)h8hy_ang;
  int tw = (int)(h8hy_twist * 512);
  for (int k = 0; k < V8_GN; k++) {
    int a = A[k], r = (R[k] * zk) >> 8, p, lev, hue;
    switch (pat) {
      case H8HY_RINGS:
        p = r * K - T + ((v3_sin(a * 3 + rot * 2) * tw) >> 12);
        hue = (v3_sin(p) >> 9) + (r >> 1);
        break;
      case H8HY_TUNNEL: {
        int depth = 6000 / (r + 6);                                  // far away in the middle
        p = depth * K / 2 - T;
        int q = a * (arms ? arms : 1) + ((depth * tw) >> 6) + rot * 3;
        int c2 = (v3_cos(p) + v3_cos(q)) >> 1;
        lev = 113 + ((c2 >> 6) * con >> 8) * 113 / 256;
        lev = lev * v3_clamp(r, 0, 48) / 48;                         // fog in the distance
        LEV[k] = (uint8_t)v3_clamp(lev, 0, 226); HUE[k] = (uint8_t)((v3_sin(p) >> 9) + (v3_sin(q) >> 9) + (r >> 1));
        continue; }
      case H8HY_PETALS: {
        int pe = v3_abs(v3_cos((a * (arms ? arms : 1) >> 1) + rot));  // 0 … 16384
        p = r * K - ((pe * (40 + tw / 4)) >> 8) - T;
        hue = (v3_sin(p) >> 9) + (pe >> 9) + (r >> 1);
        break; }
      case H8HY_OPWAVE:
        p = r * K + ((v3_sin(a * (arms ? arms : 1) + rot * 4) * (60 + tw)) >> 11) - T;
        hue = (v3_sin(p) >> 9) + (r >> 1);
        break;
      default:                                                       // spiral
        p = r * K + a * arms + ((r * r * tw) >> 12) - T + rot;
        hue = (v3_sin(p) >> 9) + (r >> 1);                           // smooth and periodic: no seam where the angle wraps
        break;
    }
    lev = 113 + ((v3_cos(p) >> 6) * con >> 8) * 113 / 256;
    LEV[k] = (uint8_t)v3_clamp(lev, 0, 226); HUE[k] = (uint8_t)(hue & 255);
  }
  v8_upscale(6);
  v8_end();
}

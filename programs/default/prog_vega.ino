// =====================================================================
// PROGRAM: VEGA — Victor Vasarely's bulging grids (by Dewey) — Set 7, slot 1
//
// In "Vega" (1957 onward) Vasarely drew a chequerboard and swelled part of
// it outward, as if a ball were pushing up under a sheet of graph paper: a
// flat grid of squares that seems to bulge out of the canvas. He did it
// with circles, diamonds, stripes, in black and white and in glowing
// violets and reds. Here the bulge moves, breathes, sinks into a dimple,
// twists into a whirlpool and ripples like water.
//
// How it works: for every pixel the lens is one table lookup — its squared
// distance from the lens centre gives a scale (and, for twisting, a turn) —
// and the pattern is one more: the position inside a cell (to 1/16 of a
// cell) gives the shade, with soft edges, and the cell gives the hue.
//
// Presets: k0 Vega · k1 Pinch · k2 Dots · k3 Whirlpool · k4 Diamonds · k5 Ripple ·
//          k6 Stripes · k7 Breathe · k8 Cells · k9 Crosses · k10 Planet · k11 Tour
//   k12 — POP: the bulge springs up and wobbles
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Bulge (◄ pinch | bulge ►) · p6 Lens Size · p7 Cells · p8 Pattern · p9 Wander · p10 Twist (◄ | ►)
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { VV_CHECK, VV_DOTS, VV_DIAMOND, VV_STRIPE, VV_RINGS, VV_CROSS, VV_NPAT };
enum { VV_BULGE, VV_RIPPLE };
// bulge ×100 (− = pinch) · lens: radius, % of the screen height · cell px · wander % · twist: turns ×100 at the middle ·
// breathe: bulge swings (%) · hues: cells coloured by position (0 = two-tone)
struct VvLook { uint8_t pat, lensType; int8_t bulge; uint8_t lens, cell, wander; int8_t twist; uint8_t breathe, hues, size, pal, flow, spread, sat; };
static const VvLook vv_looks[11] = {
  // pattern     lens        bulge lens cell wand twist brth hues size pal            flow spr sat
  { VV_CHECK  , VV_BULGE ,  80,  55, 20, 35,  0,  0, 0, 50, O7P_BW      , 0,  50,   0 },   // Vega
  { VV_CHECK  , VV_BULGE , -70,  60, 18, 30,  0,  0, 1, 50, O7P_KITAOKA , 3,  90, 100 },   // Pinch
  { VV_DOTS   , VV_BULGE ,  75,  60, 16, 35,  0,  0, 1, 62, O7P_VEGA    , 3,  80, 100 },   // Dots
  { VV_CHECK  , VV_BULGE ,  30, 100, 22, 15, 55,  0, 1, 50, O7P_SPECTRUM, 3,  90, 100 },   // Whirlpool
  { VV_DIAMOND, VV_BULGE ,  70,  55, 20, 35,  0,  0, 1, 60, O7P_VEGA    , 4,  70, 100 },   // Diamonds
  { VV_CHECK  , VV_RIPPLE,  40,  90, 18, 20,  0,  0, 1, 50, O7P_ANUSZ   , 3,  90, 100 },   // Ripple
  { VV_STRIPE , VV_BULGE ,  85,  60, 12, 35,  0,  0, 1, 50, O7P_AGAM    , 3,  90, 100 },   // Stripes
  { VV_CHECK  , VV_BULGE ,   0,  60, 20, 20,  0, 85, 1, 50, O7P_SPECTRUM, 3, 100,  90 },   // Breathe
  { VV_RINGS  , VV_BULGE ,  70,  60, 24, 35,  0,  0, 1, 70, O7P_AGAM    , 4, 100, 100 },   // Cells
  { VV_CROSS  , VV_BULGE , -55,  65, 20, 30,  0,  0, 1, 45, O7P_STIJL   , 3,  90, 100 },   // Crosses
  { VV_CHECK  , VV_BULGE ,  92, 120, 10,  0,  0, 20, 1, 50, O7P_KITAOKA , 2,  60, 100 },   // Planet
};
static const char* const vv_names[12] = { "Vega", "Pinch", "Dots", "Whirlpool", "Diamonds", "Ripple", "Stripes", "Breathe", "Cells", "Crosses", "Planet", "Tour" };
static const char* const vv_lbl[6] = { "Bulge: Pinch|Bulge", "Lens Size", "Cells", "Pattern", "Wander", "Twist" };
const char* prog_vega_name() { return "VEGA"; }
const char* prog_vega_character() { return "Vasarely's Vega: a chequerboard swelling, sinking and twisting under a lens"; }
const char* prog_vega_presetName(int p) { return (p >= 0 && p < 12) ? vv_names[p] : NULL; }
const char* prog_vega_potLabel(int p, int pot) { (void)p; return o7_label(pot, vv_lbl); }
uint8_t prog_vega_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the lens (scale Q8 and the turn, per r²/64), the pattern (32 × 32), columns
#define VV_LN 1600
static int16_t *vv_sc, *vv_cs, *vv_sn; static uint8_t* vv_pat; static int32_t *vv_dx2, *vv_dxk; static uint8_t* vv_hue;
static void vv_mem() {
  vv_sc = (int16_t*)v6_mem; vv_cs = vv_sc + VV_LN; vv_sn = vv_cs + VV_LN;                     // 9600 bytes
  vv_pat = v6_mem + 9600; vv_dx2 = (int32_t*)(v6_mem + 10624); vv_dxk = vv_dx2 + W;            // 1024 + 2560
  vv_hue = v6_mem + 13184;                                                                      // 64
}
static float vv_bulge, vv_lensR, vv_cell, vv_wand, vv_twist, vv_size, vv_pop = 0, vv_popV = 0, vv_wph = 0, vv_pu = 0, vv_pv = 0, vv_rph = 0;
static int vv_patMade = -1, vv_sizeMade = -1, vv_hueMade = -1, vv_patHue = O7_GREY;   // (B&W: the hue goes in the pattern)
void prog_vega_init() { vv_mem(); v6_init(); vv_patMade = -1; vv_pop = vv_popV = 0; }

// the pattern over two cells each way (so it knows odd and even cells): index (u5 << 5) | v5,
// u5 = parity bit + 4 bits of position in the cell → shade << 4 (the hue is added per pixel)
static void vv_makePattern(int pat, int size) {
  for (int iu = 0; iu < 32; iu++) for (int iv = 0; iv < 32; iv++) {
    int pu = iu >> 4, pv = iv >> 4;
    int fu = (iu & 15) * 2 - 15, fv = (iv & 15) * 2 - 15;               // −15 … 15 (half-steps from the cell middle)
    int au = fu < 0 ? -fu : fu, av = fv < 0 ? -fv : fv, s;
    int R = 3 + size * 13 / 100;                                         // 3 … 16 half-steps (16 = touching)
    switch (pat) {
      case VV_CHECK: s = (pu ^ pv) ? 0 : 15; break;
      case VV_DOTS: { int d = (int)v3_isqrt((uint32_t)(fu * fu + fv * fv) * 16); s = v3_clamp((R * 4 - d) * 2 + 8, 0, 15); } break;
      case VV_DIAMOND: s = v3_clamp(((R * 3 / 2) - (au + av)) * 4 + 8, 0, 15); break;
      case VV_STRIPE: s = pu ? 0 : 15; (void)au; break;
      case VV_RINGS: { int d = (int)v3_isqrt((uint32_t)(fu * fu + fv * fv) * 16) / 4; s = ((d / 4) & 1) ? 2 : 14; if (d > 15) s = (pu ^ pv) ? 0 : 6; } break;
      default: { int m = au < av ? au : av; s = v3_clamp((R / 2 - m) * 5 + 8, 0, 15); }
    }
    if (pat != VV_CHECK && pat != VV_STRIPE && pat != VV_RINGS && ((pu ^ pv) & 1)) s = s * 3 / 4;   // alternate cells a little darker
    vv_pat[(iu << 5) | iv] = (uint8_t)((s << 4) | vv_patHue);
  }
}

// The lens is smooth, so u and v are worked out exactly every 8th pixel and stepped evenly between.
// u, v at a pixel: no twist: dx·scale, dy·scale; twisting: turned by the lens's angle as well
static inline __attribute__((always_inline)) void vv_uv(int x, int32_t dy2, int32_t dyk, bool tw, int32_t* u, int32_t* v) {
  int r = (vv_dx2[x] + dy2) >> 6; if (r >= VV_LN) r = VV_LN - 1;
  if (!tw) { int32_t s = vv_sc[r]; *u = (vv_dxk[x] * s) >> 8; *v = (dyk * s) >> 8; }
  else { int32_t c = vv_cs[r], s = vv_sn[r], a = vv_dxk[x]; *u = (a * c - dyk * s) >> 8; *v = (a * s + dyk * c) >> 8; }
}
__attribute__((optimize("O2"))) static void vv_row(uint8_t* out, int32_t dy2, int32_t dyk, int32_t pu, int32_t pv, bool tw, bool hues) {
  const uint8_t* pat = vv_pat; const uint8_t* hu = vv_hue;
  int32_t u0, v0; vv_uv(0, dy2, dyk, tw, &u0, &v0);
  for (int x = 0; x < W; x += 8) {
    int32_t u1, v1; vv_uv(x + 8 < W ? x + 8 : W - 1, dy2, dyk, tw, &u1, &v1);
    if (x + 8 >= W) { u1 = u0 + (u1 - u0) * 8 / 7; v1 = v0 + (v1 - v0) * 8 / 7; }
    int32_t du = (u1 - u0) >> 3, dv = (v1 - v0) >> 3;
    uint32_t u = (uint32_t)(u0 + pu), v = (uint32_t)(v0 + pv);
    uint8_t* o = out + x;
    if (hues) for (int k = 0; k < 8; k++, u += du, v += dv) o[k] = (uint8_t)(pat[((u >> 3) & 0x3E0) | ((v >> 8) & 31)] | hu[((u >> 12) + (v >> 12) * 3) & 63]);
    else for (int k = 0; k < 8; k++, u += du, v += dv) o[k] = pat[((u >> 3) & 0x3E0) | ((v >> 8) & 31)];
    u0 = u1; v0 = v1;
  }
}

void prog_vega_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  vv_mem();
  const VvLook& L = vv_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&vv_bulge, o7_kb(5, L.bulge / 100.0f), rate);
  v6_ease(&vv_lensR, v3_centre(6) ? o7_vary(6, L.lens / 100.0f) : 0.2f + v3_u(6) * 1.2f, rate);
  v6_ease(&vv_cell, v3_centre(7) ? o7_vary(7, (float)L.cell) : 6.0f + v3_u(7) * 34.0f, rate);
  v6_ease(&vv_wand, o7_kn(9, L.wander / 100.0f), rate);
  v6_ease(&vv_twist, v3_centre(10) ? o7_vary(10, L.twist / 100.0f) : v3_bi(10) * 2.0f, rate);
  v6_ease(&vv_size, (float)L.size, rate);
  int pat = o7_pick(8, L.pat, VV_NPAT);
  int sz = (int)vv_size;
  int phue = (L.hues || L.pal != O7P_BW) ? 0 : O7_GREY;
  if (pat != vv_patMade || sz != vv_sizeMade || phue != vv_patHue) { vv_patHue = phue; vv_makePattern(pat, sz); vv_patMade = pat; vv_sizeMade = sz; }
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);
  // the hue of each cell: two-tone, or stepping through the hues
  int hk = L.hues ? 1 : 0;
  if (hk != vv_hueMade) { for (int i = 0; i < 64; i++) vv_hue[i] = hk ? o7_mod15[i % 60] : (uint8_t)((i & 1) ? 4 : 0); vv_hueMade = hk; }

  float sdt = v6_f.sdt, t = v6_t;
  // POP: a damped spring kicked upward
  if (v6_k12hit) vv_popV += 4.0f;
  vv_popV += (-vv_pop * 30.0f - vv_popV * 3.0f) * v6_f.dt; vv_pop += vv_popV * v6_f.dt;
  vv_pop = v3_clampf(vv_pop, -1.0f, 1.0f);
  float b = vv_bulge + vv_pop * 0.6f;
  if (L.breathe) b += L.breathe * 0.01f * o7_nsin(t * 0.8f);
  b = v3_clampf(b, -0.95f, 0.97f);
  // ── the lens: scale (and turn) for each r²/64 ──
  float z = v6_zoom(1.0f);
  float R = vv_lensR * H, R2 = R * R;
  vv_rph += sdt * 1.6f;
  bool twist = fabsf(vv_twist) > 0.01f;
  // (whole numbers only: q = (r / R)² × 65536, stepping along the table)
  int32_t qstep = (int32_t)(64.0f * 65536.0f / (R2 > 1.0f ? R2 : 1.0f)), q = qstep / 2;
  int32_t bq = (int32_t)(b * (b > 0 ? 0.8f : 1.2f) * 65536.0f);          // the bulge's depth, ×65536
  int32_t ta = (int32_t)(vv_twist * 1024.0f), t0 = (int32_t)(0.3f * vv_twist * 163.0f * o7_nsin(t * 0.5f));
  int32_t rb = (int32_t)(b * 0.35f * 16384.0f), rph = (int32_t)(vv_rph * 163.0f);
  int n = VV_LN;
  for (int i = 0; i < VV_LN; i++, q += qstep) {
    int32_t sc = 256;
    if (L.lensType == VV_RIPPLE) {
      int32_t r12 = (int32_t)v3_isqrt((uint32_t)q << 8);                  // r / R × 4096
      if (r12 >= 10240) { n = i; break; }                                 // ripples die away by 2.5 R
      int32_t fade = 16384 - r12 * 16384 / 10240;
      sc = 256 - (int32_t)(((int64_t)rb * v3_sin((r12 * 9 * 163 >> 12) - rph) >> 14) * fade >> 20);
    } else if (q < 65536) {
      int32_t w = (int32_t)(((int64_t)(65536 - q) * (65536 - q)) >> 16);
      sc = 256 - (int32_t)(((int64_t)bq * w) >> 24);
    } else if (!twist) { n = i; break; }                                  // outside the lens: all the same
    vv_sc[i] = (int16_t)sc;
    if (twist) {
      int32_t w = q < 65536 ? (int32_t)(((int64_t)(65536 - q) * (65536 - q)) >> 16) : 0;
      int a = (int)((ta * w) >> 16) + t0;
      vv_cs[i] = (int16_t)((v3_cos(a) * sc) >> 14); vv_sn[i] = (int16_t)((v3_sin(a) * sc) >> 14);
    }
  }
  for (int i = n; i < VV_LN; i++) { vv_sc[i] = 256; if (twist) { vv_cs[i] = 256; vv_sn[i] = 0; } }
  // ── where the lens is, and the grid's drift ──
  vv_wph += sdt * 0.35f;
  float cx = HALFW + vv_wand * 110.0f * o7_nsin(vv_wph * 1.1f), cy = HALFH + vv_wand * 70.0f * o7_nsin(vv_wph * 0.83f + 1.0f);
  float cellPx = vv_cell * z;
  float k = 4096.0f / cellPx;                                             // 4096 = one cell
  vv_pu += sdt * 0.13f; vv_pv += sdt * 0.07f;
  if (vv_pu > 1000) vv_pu -= 1000; if (vv_pv > 1000) vv_pv -= 1000;
  // keep the grid lined up with the lens middle, drifting slowly
  int32_t pu = (int32_t)(vv_pu * 4096) + 4096 * 64 + 2048, pv = (int32_t)(vv_pv * 4096) + 4096 * 64 + 2048;
  int icx = (int)cx, icy = (int)cy;
  for (int x = 0; x < W; x++) { int d = x - icx; vv_dx2[x] = d * d; vv_dxk[x] = (int32_t)(d * k); }
  for (int y = 0; y < H; y++) {
    int d = y - icy;
    int32_t dyk = (int32_t)(d * k);
    vv_row(v3_buf + y * W, d * d, dyk, pu, pv, twist, L.hues || L.pal != O7P_BW);
  }
  o7_end();
}

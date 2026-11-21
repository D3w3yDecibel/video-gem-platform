// =====================================================================
// PROGRAM: GRATING — moiré (by Dewey) — Set 7, slot 2
//
// Lay one fine grating over another — two sheets of lines, two sets of
// rings — and a third pattern appears that is in neither: huge slow bands
// and rosettes, the moiré, which race across the screen when one sheet
// moves a hair. Op artists from Gerald Oster to Jesús Rafael Soto made
// whole works from it. Here the beat fringes also carry the colour: the hue
// follows the difference between the two gratings, so the moiré glows.
//
// How it works: a grating is a phase that grows by a fixed step along each
// row (lines), or the distance from a moving centre (rings: one table for
// the distance, interpolated, no square roots). Each phase becomes a shade
// through a stripe table; a third table combines the two shades (XOR,
// multiply, screen or difference), and the hue is the phase difference.
//
// Presets: k0 Twin Lines · k1 Rings · k2 Target · k3 Fringes · k4 Shimmer ·
//          k5 Ripple Tank · k6 Plaid · k7 Zoom · k8 Eye · k9 Nebula · k10 Glass · k11 Tour
//   k12 — JUMP: the second grating jumps to a new angle and scale
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Lines (fineness) · p6 Angle (between the two) · p7 Drift · p8 Thickness
//   p9 Mix (XOR | multiply | screen | difference) · p10 Layout (lines | rings | rings × lines)
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { GT_LL, GT_RR, GT_RL, GT_NLAY };
// freq: lines across ·  angle: between the gratings (deg ×10) · ratio: second's spacing (% of first) ·
// drift: how fast things move · sep: ring centres apart (px) · orbit: centres circle (1) · hue: colour the fringes
struct GtLook { uint8_t lay, op, freq; int16_t angle; uint8_t ratio, drift, sep, orbit, duty, soft, hue, pal, flow, spread, sat; };
static const GtLook gt_looks[11] = {
  // lay    op       freq angle ratio drift sep orb duty soft hue pal           flow spr sat
  { GT_LL, O7_XOR , 52,  40, 100, 30,   0, 0, 50,  0, 0, O7P_BW      , 0,  50,   0 },   // Twin Lines
  { GT_RR, O7_AND , 38,   0, 100, 30,  70, 0, 50,  0, 1, O7P_AGAM    , 3,  90, 100 },   // Rings
  { GT_RL, O7_XOR , 34,   0, 100, 25,   0, 1, 50,  0, 1, O7P_STIJL   , 3,  90, 100 },   // Target
  { GT_LL, O7_AND , 60,  25, 100, 25,   0, 0, 50, 10, 1, O7P_SPECTRUM, 3, 100, 100 },   // Fringes
  { GT_LL, O7_OR  , 62,  12, 103, 30,   0, 0, 40,  0, 1, O7P_RA      , 3,  90, 100 },   // Shimmer
  { GT_RR, O7_AND , 44,   0, 100, 30,  90, 1, 50, 10, 1, O7P_NEON    , 4, 100, 100 },   // Ripple Tank
  { GT_LL, O7_XOR , 30, 900, 100, 20,   0, 0, 50,  0, 1, O7P_STIJL   , 2,  80,  90 },   // Plaid
  { GT_LL, O7_XOR , 48,   0, 108, 25,   0, 0, 50,  0, 1, O7P_VEGA    , 3,  80, 100 },   // Zoom
  { GT_RR, O7_XOR , 46,   0, 106, 20,   0, 0, 50,  0, 1, O7P_RA      , 3,  90, 100 },   // Eye
  { GT_RR, O7_OR  , 40,   0, 100, 25, 120, 1, 50, 35, 1, O7P_AGAM    , 4, 100,  90 },   // Nebula
  { GT_RL, O7_DIFF, 80,  30, 100, 35,   0, 1, 50,  5, 1, O7P_KITAOKA , 3,  70,  90 },   // Glass
};
static const char* const gt_names[12] = { "Twin Lines", "Rings", "Target", "Fringes", "Shimmer", "Ripple Tank", "Plaid", "Zoom", "Eye", "Nebula", "Glass", "Tour" };
static const char* const gt_lbl[6] = { "Lines", "Angle", "Drift", "Thickness", "Mix: XOR|Mul|Screen|Diff", "Layout: Lines|Rings|Both" };
const char* prog_grating_name() { return "GRATING"; }
const char* prog_grating_character() { return "Moire: two gratings of lines or rings beating into huge glowing fringes"; }
const char* prog_grating_presetName(int p) { return (p >= 0 && p < 12) ? gt_names[p] : NULL; }
const char* prog_grating_potLabel(int p, int pot) { (void)p; return o7_label(pot, gt_lbl); }
uint8_t prog_grating_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: √ table (r² / 64 → r × 16), each grating's band (shade, and shade << 4), the combine
// table (shade pair → shade << 4), the hue table, and per-centre column tables
// (two √ tables: near the middle r² / 4, further out r² / 64 — each step well under a pixel's worth of ring)
#define GT_SQ 4800
#define GT_SN 2048
static uint16_t *gt_sq, *gt_sn; static uint8_t *gt_T, *gt_hue; static int32_t *gt_dxA, *gt_dxB; static uint32_t *gt_sA, *gt_sB;
static void gt_mem() {
  gt_sq = (uint16_t*)v6_mem;                                                       // 9600
  gt_sn = (uint16_t*)(v6_mem + 9600);                                              // 4096
  gt_T = v6_mem + 13696;                                                           // 4096: the two bands combined
  gt_hue = v6_mem + 17792;                                                         // 16
  gt_dxA = (int32_t*)(v6_mem + 17808); gt_dxB = gt_dxA + W;                           // 2560
  gt_sA = (uint32_t*)(v6_mem + 20368); gt_sB = gt_sA + W;                            // 2560 (4 words a block)
}
// one pixel from the two phases (2^32 a stripe each): the combined bands (64 steps × 64 steps) and the fringe hue
#define GT_PX(a, b) ((uint8_t)(T[(((a) >> 26) << 6) | ((b) >> 26)] | hu[((a) - (b)) >> 28]))
static float gt_freq, gt_ang, gt_drift, gt_duty, gt_phA = 0, gt_phB = 0, gt_rot = 0, gt_jumpA = 0, gt_jumpS = 0, gt_jA = 0, gt_jS = 0, gt_orb = 0;
static bool gt_sqOK = false;

void prog_grating_init() {
  gt_mem(); v6_init(); gt_jumpA = gt_jumpS = gt_jA = gt_jS = 0;
  for (int i = 0; i < GT_SQ; i++) gt_sq[i] = (uint16_t)v3_isqrt(((uint32_t)i * 64 + 32) * 256);   // √(r²) × 16
  for (int i = 0; i < GT_SN; i++) gt_sn[i] = (uint16_t)v3_isqrt(((uint32_t)i * 4 + 2) * 256);
  gt_sqOK = true;
}
// r × 16 from r²
static inline __attribute__((always_inline)) int32_t gt_r16(uint32_t r2) {
  if (r2 < GT_SN * 4) return gt_sn[r2 >> 2];
  uint32_t i = r2 >> 6; return gt_sq[i < GT_SQ ? i : GT_SQ - 1];
}

// lines × lines: two phases stepping along the row (2^32 a line)
__attribute__((optimize("O2"))) static void gt_rowLL(uint8_t* out, uint32_t a, int32_t da, uint32_t b, int32_t db) {
  const uint8_t *T = gt_T, *hu = gt_hue;
  for (int x = 0; x < W; x++, a += da, b += db)
    out[x] = GT_PX(a, b);
}
// rings × rings: two distances (r × 16) times a spacing. Rings change smoothly, so they are worked out
// exactly every other pixel and the pixel between takes the halfway phase.
// Two passes, so neither loop has too many values to juggle.
__attribute__((optimize("O2"))) static void gt_rowRR(uint8_t* out, int32_t dyA2, int32_t dyB2, uint32_t kA, uint32_t kB, uint32_t pa, uint32_t pb) {
  const int32_t *dA = gt_dxA, *dB = gt_dxB;
  uint32_t* q = gt_sA;
  // (1) the distances, exactly every 4th pixel (every other pixel on the rows near a centre, where the
  //     rings curve tightly), as a start phase and a step per pixel for each block
  int sh = (dyA2 >= 256 && dyB2 >= 256) ? 2 : 1, S = 1 << sh, nb = W >> sh;
  uint32_t qA = kA >> sh, qB = kB >> sh;
  int32_t rA = gt_r16((uint32_t)(dA[0] + dyA2)), rB = gt_r16((uint32_t)(dB[0] + dyB2));
  for (int i = 0, x = S; i < nb; i++, x += S, q += 4) {
    int xx = x < W ? x : W - 1;
    int32_t nA = gt_r16((uint32_t)(dA[xx] + dyA2)), nB = gt_r16((uint32_t)(dB[xx] + dyB2));
    q[0] = (uint32_t)rA * kA + pa; q[1] = (uint32_t)(nA - rA) * qA;
    q[2] = (uint32_t)rB * kB + pb; q[3] = (uint32_t)(nB - rB) * qB;
    rA = nA; rB = nB;
  }
  // (2) the pixels
  const uint8_t *T = gt_T, *hu = gt_hue;
  q = gt_sA;
  if (sh == 2) {
    for (uint8_t* const oe = out + W; out < oe; out += 4, q += 4) {
      uint32_t a = q[0], da = q[1], b = q[2], db = q[3];
      out[0] = GT_PX(a, b); a += da; b += db;
      out[1] = GT_PX(a, b); a += da; b += db;
      out[2] = GT_PX(a, b); a += da; b += db;
      out[3] = GT_PX(a, b);
    }
  } else {
    for (uint8_t* const oe = out + W; out < oe; out += 2, q += 4) {
      uint32_t a = q[0], b = q[2];
      out[0] = GT_PX(a, b); a += q[1]; b += q[3];
      out[1] = GT_PX(a, b);
    }
  }
}
// rings × lines
__attribute__((optimize("O2"))) static void gt_rowRL(uint8_t* out, int32_t dyA2, uint32_t kA, uint32_t pa, uint32_t b, int32_t db) {
  const uint8_t *T = gt_T, *hu = gt_hue; const int32_t* dA = gt_dxA;
  uint32_t a0 = (uint32_t)gt_r16((uint32_t)(dA[0] + dyA2)) * kA + pa;
  for (int x = 0; x < W; x += 2) {
    int xn = x + 2 < W ? x + 2 : W - 1;
    uint32_t a2 = (uint32_t)gt_r16((uint32_t)(dA[xn] + dyA2)) * kA + pa, a1 = a0 + (uint32_t)((int32_t)(a2 - a0) >> 1);
    out[x] = GT_PX(a0, b); b += db;
    out[x + 1] = GT_PX(a1, b); b += db;
    a0 = a2;
  }
}

void prog_grating_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  gt_mem();
  if (!gt_sqOK) prog_grating_init();
  const GtLook& L = gt_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&gt_freq, v3_centre(5) ? o7_vary(5, (float)L.freq) : 8.0f + v3_u(5) * 110.0f, rate);
  v6_ease(&gt_ang, v3_centre(6) ? o7_vary(6, L.angle * 0.001745f) : v3_bi(6) * 0.5f, rate);
  v6_ease(&gt_drift, o7_kn(7, L.drift / 100.0f), rate);
  v6_ease(&gt_duty, o7_kn(8, L.duty / 100.0f), rate);
  int op = o7_pick(9, L.op, 4), lay = o7_pick(10, L.lay, GT_NLAY);
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt, t = v6_t;
  float sp = sdt * (0.1f + gt_drift * 2.0f);
  gt_phA += sp * 0.6f; gt_phB -= sp * 0.45f; gt_rot += sp * 0.05f; gt_orb += sp * 0.4f;
  gt_phA -= floorf(gt_phA); gt_phB -= floorf(gt_phB);
  if (v6_k12hit) { gt_jumpA = (v3_rndf() - 0.5f) * 0.5f; gt_jumpS = (v3_rndf() - 0.5f) * 0.12f; }
  v6_ease(&gt_jA, gt_jumpA, 3.0f); v6_ease(&gt_jS, gt_jumpS, 3.0f);
  float z = v6_zoom(1.0f);
  float periodPx = 320.0f / gt_freq * z;
  if (periodPx < 2.5f) periodPx = 2.5f;                                    // (finer than this is all aliasing)
  float ratio = L.ratio / 100.0f * (1.0f + gt_jS) * (1.0f + 0.015f * o7_nsin(t * 0.4f) * (L.ratio != 100 ? 1.0f : 0.0f));
  // ── tables: the two bands, the combine table, the hues of the fringes ──
  int edge = o7_edge(periodPx, 64, L.soft / 100.0f);
  uint8_t tmp[64];
  o7_band(tmp, 64, (int)(gt_duty * 256), edge);
  for (int i = 0; i < 64; i++) for (int j = 0; j < 64; j++) gt_T[(i << 6) | j] = (uint8_t)(o7_op[op][(tmp[i] << 4) | tmp[j]] << 4);
  for (int i = 0; i < 16; i++) gt_hue[i] = L.hue ? o7_mod15[i] : O7_GREY;

  uint32_t pa = (uint32_t)(uint64_t)(gt_phA * 4294967296.0f), pb = (uint32_t)(uint64_t)(gt_phB * 4294967296.0f);
  float kpx = 4294967296.0f / periodPx;                                   // phase per pixel (2^32 a line)
  if (lay == GT_LL) {
    float a1 = gt_rot, a2 = gt_rot + gt_ang + gt_jA + (L.angle == 900 ? 0.0f : 0.04f * o7_nsin(t * 0.3f));
    float kb = kpx / ratio;
    int32_t dax = (int32_t)(cosf(a1) * kpx), day = (int32_t)(sinf(a1) * kpx);
    int32_t dbx = (int32_t)(cosf(a2) * kb), dby = (int32_t)(sinf(a2) * kb);
    uint32_t a = pa - (uint32_t)dax * HALFW - (uint32_t)day * HALFH, b = pb - (uint32_t)dbx * HALFW - (uint32_t)dby * HALFH;
    for (int y = 0; y < H; y++, a += (uint32_t)day, b += (uint32_t)dby) gt_rowLL(v3_buf + y * W, a, dax, b, dbx);
  } else {
    // ring centres: apart by `sep`, or circling round each other
    float sep = L.sep * z, ang = L.orbit ? gt_orb : 0.0f;
    float ax = HALFW - 0.5f * sep * cosf(ang), ay = HALFH - 0.5f * sep * sinf(ang) * 0.7f;
    float bx = HALFW + 0.5f * sep * cosf(ang), by = HALFH + 0.5f * sep * sinf(ang) * 0.7f;
    if (L.orbit && L.sep == 0) { ax = HALFW + 40 * o7_nsin(gt_orb * 0.7f); ay = HALFH + 25 * o7_nsin(gt_orb * 0.5f); }
    if (lay == GT_RR && L.sep == 0 && !L.orbit) { ax = bx = HALFW; ay = by = HALFH; }
    int iax = (int)ax, iay = (int)ay, ibx = (int)bx, iby = (int)by;
    for (int x = 0; x < W; x++) { int d = x - iax, e = x - ibx; gt_dxA[x] = d * d; gt_dxB[x] = e * e; }
    uint32_t kA = (uint32_t)(kpx / 16.0f), kB = (uint32_t)(kpx / 16.0f / ratio);  // per r × 16
    if (lay == GT_RR) {
      for (int y = 0; y < H; y++) { int d = y - iay, e = y - iby; gt_rowRR(v3_buf + y * W, d * d, e * e, kA, kB, pa, pb); }
    } else {
      float a2 = gt_rot * 3.0f + gt_ang + gt_jA, kb = kpx / ratio;
      int32_t dbx = (int32_t)(cosf(a2) * kb), dby = (int32_t)(sinf(a2) * kb);
      uint32_t b = pb - (uint32_t)dbx * HALFW - (uint32_t)dby * HALFH;
      for (int y = 0; y < H; y++, b += (uint32_t)dby) { int d = y - iay; gt_rowRL(v3_buf + y * W, d * d, kA, pa, b, dbx); }
    }
  }
  o7_end();
}

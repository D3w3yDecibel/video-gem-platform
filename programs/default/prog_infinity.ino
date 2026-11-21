// =====================================================================
// PROGRAM: INFINITY — nested shapes falling forever (by Dewey) — Set 7, slot 8
//
// Squares inside squares inside squares, each a little smaller, black and
// white, until the eye falls down the middle: Vasarely's "Vonal" squares,
// the endless corridor, the twisting square spiral every op-art poster
// has. Here they really do go on for ever — the rings stream toward the
// middle (or out of it), the shape morphs from square to circle to star,
// and each ring can turn a little more than the last so the whole tunnel
// twists.
//
// How it works: the middle wanders slowly, so every pixel is worked out. Per pixel: a distance from the middle in the shape's own measure (the
// bigger of |x| and |y| for a square, their sum for a diamond, a table for
// a circle) — after turning by an angle that depends on the distance, for
// twisting — then a depth table (1 / distance, or its logarithm) gives the
// ring, and a stripe table its shade and hue.
//
// Presets: k0 Squares · k1 Twist · k2 Diamonds · k3 Target · k4 Octagons · k5 Morph ·
//          k6 Star · k7 Corridor · k8 Cross · k9 Rise · k10 Vortex · k11 Tour
//   k12 — DIVE: hold to rush forward
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Rings · p6 Twist (◄ | ►) · p7 Shape (square → diamond → circle → octagon → star → cross)
//   p8 Thickness · p9 Depth: Even | Perspective · p10 Edge (sharp | soft)
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { NF_SQUARE, NF_DIAMOND, NF_CIRCLE, NF_OCTAGON, NF_STAR, NF_CROSS, NF_NSHAPE };
// rings: rings across the screen · twist: turns × 100 from edge to middle · persp: 0 even (log) … 100 perspective ·
// speed: rings per second (− = outward) · morph: shapes ×100 (e.g. 0 = square, 250 = between circle and octagon) ·
// aspect: corridor (wider than tall) · breathe: the shape morphs back and forth
struct NfLook { uint16_t morph; uint8_t rings; int8_t twist; uint8_t persp; int8_t speed; uint8_t aspect, breathe, duty, soft, hstep, pal, flow, spread, sat; };
static const NfLook nf_looks[11] = {
  // morph rings twist persp speed asp brth duty soft hst pal            flow spr sat
  {   0, 12,   0,  65,  60, 100,   0, 50, 0, 0, O7P_BW      , 0,  50,   0 },   // Squares
  {   0,  9,  60,  60,  40, 100,   0, 50, 0, 1, O7P_RA      , 3,  90, 100 },   // Twist
  { 100, 10,   0,  20,  45, 100,   0, 50, 5, 1, O7P_VEGA    , 3,  80, 100 },   // Diamonds
  { 200, 12,   0,   0,  40, 100,   0, 50, 0, 1, O7P_VEGA    , 3,  90, 100 },   // Target
  { 300,  9,   0, 100,  50, 100,   0, 50, 5, 1, O7P_NEON    , 4, 100, 100 },   // Octagons
  {   0, 10,   0,  60,  40, 100, 100, 50, 5, 1, O7P_RA      , 3,  80, 100 },   // Morph
  { 400, 10,  20,  40,  45, 100,   0, 50, 0, 1, O7P_NEON    , 3,  90, 100 },   // Star
  {   0, 12,   0, 100,  70,  75,   0, 50, 0, 2, O7P_STIJL   , 2,  60,  90 },   // Corridor
  { 500,  9,   0,  60,  40, 100,   0, 50, 0, 1, O7P_KITAOKA , 3,  90, 100 },   // Cross
  {   0, 10,   0,  40, -50, 100,   0, 50, 5, 1, O7P_SPECTRUM, 3, 100,  90 },   // Rise
  { 100,  8, -90,  50,  50, 100,   0, 50, 5, 1, O7P_AGAM    , 4, 100,  90 },   // Vortex
};
static const char* const nf_names[12] = { "Squares", "Twist", "Diamonds", "Target", "Octagons", "Morph", "Star", "Corridor", "Cross", "Rise", "Vortex", "Tour" };
static const char* const nf_lbl[6] = { "Rings", "Twist", "Shape", "Thickness", "Depth: Even|Perspective", "Edge: Sharp|Soft" };
const char* prog_infinity_name() { return "INFINITY"; }
const char* prog_infinity_character() { return "Nested squares, circles and stars falling forever down a twisting tunnel"; }
const char* prog_infinity_presetName(int p) { return (p >= 0 && p < 12) ? nf_names[p] : NULL; }
const char* prog_infinity_potLabel(int p, int pot) { (void)p; return o7_label(pot, nf_lbl); }
uint8_t prog_infinity_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the stripe field (4096), the depth table (distance in ½ px → phase, 1024 × 2), √ table for circles
// (r² / 16 → r × 2, 2560 × 2), the twist table (r² / 64 → cos, sin, 640 × 4), and |x| for each column
#define NF_DN 1024
static uint8_t* nf_comb; static uint16_t *nf_depth, *nf_sq; static int16_t *nf_tw, *nf_ax;
static void nf_mem() {
  nf_comb = v6_mem;                                     // 4096
  nf_depth = (uint16_t*)(v6_mem + 4096);                // 2048
  nf_sq = (uint16_t*)(v6_mem + 6144);                   // 5120
  nf_tw = (int16_t*)(v6_mem + 11264);                   // 2560
  nf_ax = (int16_t*)(v6_mem + 13824);                   // 640
}
static bool nf_sqOK = false;
static float nf_morph, nf_rings, nf_twist, nf_persp, nf_duty, nf_soft, nf_z = 0, nf_dive = 0;
void prog_infinity_init() {
  nf_mem(); v6_init(); nf_dive = 0;   // (the depth and twist tables are remade on the first frame: v6_first)
  for (int i = 0; i < 2560; i++) nf_sq[i] = (uint16_t)v3_isqrt((uint32_t)(i * 16 + 8) * 4);   // r × 2 (r² in ½-px units /4 … see below)
  nf_sqOK = true;
}

// the shape's distance (in ½ px) from |x|, |y| (in ½ px)
static int16_t nf_mA, nf_mB, nf_mW;                      // shape A, shape B, weight of B (0 … 256)
static inline __attribute__((always_inline)) int nf_one(int s, int ax, int ay) {
  switch (s) {
    case NF_SQUARE: return ax > ay ? ax : ay;
    case NF_DIAMOND: return ((ax + ay) * 181) >> 8;                         // (a diamond as wide as the square)
    case NF_CIRCLE: { uint32_t q = (uint32_t)(ax * ax + ay * ay) >> 6; return nf_sq[q < 2560 ? q : 2559]; }
    case NF_OCTAGON: { int m = ax > ay ? ax : ay, d = ((ax + ay) * 190) >> 8; return m > d ? m : d; }
    case NF_STAR: { int m = ax > ay ? ax : ay, d = ((ax + ay) * 150) >> 8; return m < d ? m : d; }
    default: { int m = ax < ay ? ax : ay, M = ax > ay ? ax : ay; return m * 2 + (M >> 3); }
  }
}
static inline int nf_dist(int ax, int ay) {
  int a = nf_one(nf_mA, ax, ay);
  if (!nf_mW) return a;
  return a + (((nf_one(nf_mB, ax, ay) - a) * nf_mW) >> 8);
}
// one row (the middle wanders, so every row is worked out — no mirror images). The shapes are
// fixed when this is inlined below, so each gets its own loop with no decisions inside.
static inline __attribute__((always_inline)) void nf_rowS(int sa, int sb, int w, int y, int ay, uint32_t ph) {
  uint8_t* top = v3_buf + y * W;
  const int16_t* axt = nf_ax; const uint16_t* dp = nf_depth; const uint8_t* comb = nf_comb;
  for (int x = 0; x < W; x++) {
    int ax = axt[x], d = nf_one(sa, ax, ay);
    if (w) d += ((nf_one(sb, ax, ay) - d) * w) >> 8;
    if (d >= NF_DN) d = NF_DN - 1;
    top[x] = comb[((dp[d] + ph) >> 4) & 4095];
  }
}
__attribute__((optimize("O2"))) static void nf_row(int y, int ay, uint32_t ph) {
  int w = nf_mW;
  if (!w) switch (nf_mA) {
    case NF_SQUARE: nf_rowS(NF_SQUARE, 0, 0, y, ay, ph); return;
    case NF_DIAMOND: nf_rowS(NF_DIAMOND, 0, 0, y, ay, ph); return;
    case NF_CIRCLE: nf_rowS(NF_CIRCLE, 0, 0, y, ay, ph); return;
    case NF_OCTAGON: nf_rowS(NF_OCTAGON, 0, 0, y, ay, ph); return;
    case NF_STAR: nf_rowS(NF_STAR, 0, 0, y, ay, ph); return;
    default: nf_rowS(NF_CROSS, 0, 0, y, ay, ph); return;
  }
  switch (nf_mA) {                                                         // morphing between neighbours
    case NF_SQUARE: nf_rowS(NF_SQUARE, NF_DIAMOND, w, y, ay, ph); return;
    case NF_DIAMOND: nf_rowS(NF_DIAMOND, NF_CIRCLE, w, y, ay, ph); return;
    case NF_CIRCLE: nf_rowS(NF_CIRCLE, NF_OCTAGON, w, y, ay, ph); return;
    case NF_OCTAGON: nf_rowS(NF_OCTAGON, NF_STAR, w, y, ay, ph); return;
    default: nf_rowS(NF_STAR, NF_CROSS, w, y, ay, ph); return;
  }
}
// … turning each point by an angle that depends on its distance from the middle (again one loop per shape)
static int nf_c2x = W;                                                    // the middle across, in ½ px
// (the turned distance changes smoothly, so it is worked out exactly every other pixel and the pixel
// between takes the halfway phase)
static inline __attribute__((always_inline)) uint32_t nf_twP(int sa, int sb, int w, int dx, int dy, int32_t dy2, int32_t xs) {
  uint32_t q = (uint32_t)(dx * dx + dy2) >> 8; if (q > 639) q = 639;
  int32_t c = nf_tw[2 * q], s = nf_tw[2 * q + 1];
  int rx = (dx * c - dy * s) >> 14, ry = (dx * s + dy * c) >> 14;
  rx = (rx * xs) >> 8;
  int ax = rx < 0 ? -rx : rx, ay = ry < 0 ? -ry : ry, d = nf_one(sa, ax, ay);
  if (w) d += ((nf_one(sb, ax, ay) - d) * w) >> 8;
  if (d >= NF_DN) d = NF_DN - 1;
  return nf_depth[d];
}
static inline __attribute__((always_inline)) void nf_rowTwistS(int sa, int sb, int w, int y, int dy, uint32_t ph, int32_t xs) {
  uint8_t* top = v3_buf + y * W;
  const uint8_t* comb = nf_comb;
  int32_t dy2 = dy * dy;
  int dx = 1 - nf_c2x;                                                    // ½ px from the middle
  uint32_t a = nf_twP(sa, sb, w, dx, dy, dy2, xs) + ph;
  for (int x = 0; x < W; x += 2) {
    dx += 4;
    uint32_t b = nf_twP(sa, sb, w, dx, dy, dy2, xs) + ph;
    uint32_t m = a + (uint32_t)((int32_t)(int16_t)(uint16_t)(b - a) >> 1);   // (phase wraps every 65536)
    top[x] = comb[(a >> 4) & 4095]; top[x + 1] = comb[(m >> 4) & 4095];
    a = b;
  }
}
__attribute__((optimize("O2"))) static void nf_rowTwist(int y, int dy, uint32_t ph, int32_t xs) {
  int w = nf_mW;
  if (!w) switch (nf_mA) {
    case NF_SQUARE: nf_rowTwistS(NF_SQUARE, 0, 0, y, dy, ph, xs); return;
    case NF_DIAMOND: nf_rowTwistS(NF_DIAMOND, 0, 0, y, dy, ph, xs); return;
    case NF_CIRCLE: nf_rowTwistS(NF_CIRCLE, 0, 0, y, dy, ph, xs); return;
    case NF_OCTAGON: nf_rowTwistS(NF_OCTAGON, 0, 0, y, dy, ph, xs); return;
    case NF_STAR: nf_rowTwistS(NF_STAR, 0, 0, y, dy, ph, xs); return;
    default: nf_rowTwistS(NF_CROSS, 0, 0, y, dy, ph, xs); return;
  }
  // (twisting while morphing)
  nf_rowTwistS(nf_mA, nf_mB, w, y, dy, ph, xs);
}

void prog_infinity_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  nf_mem();
  if (!nf_sqOK) prog_infinity_init();
  const NfLook& L = nf_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  float t = v6_t;
  float mo = v3_centre(7) ? L.morph / 100.0f : v3_u(7) * (float)(NF_NSHAPE - 1);
  if (L.breathe && v3_centre(7)) mo = 1.0f + 1.0f * o7_nsin(t * 0.5f);         // square ↔ diamond ↔ circle
  v6_ease(&nf_morph, mo, rate);
  v6_ease(&nf_rings, v3_centre(5) ? o7_vary(5, (float)L.rings) : 3.0f + v3_u(5) * 25.0f, rate);
  v6_ease(&nf_twist, v3_centre(6) ? o7_vary(6, L.twist / 100.0f) : v3_bi(6) * 1.5f, rate);
  v6_ease(&nf_persp, o7_kn(9, L.persp / 100.0f), rate);
  v6_ease(&nf_duty, o7_kn(8, L.duty / 100.0f), rate);
  v6_ease(&nf_soft, o7_kn(10, L.soft / 100.0f), rate);
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt;
  v6_ease(&nf_dive, v6_k12 ? 1.0f : 0.0f, 2.0f);
  nf_z += sdt * L.speed * 0.02f * (1.0f + nf_dive * 5.0f);
  nf_z -= floorf(nf_z / 16.0f) * 16.0f;                                    // (16 stripes, then the hues repeat)
  float m = v3_clampf(nf_morph, 0.0f, (float)NF_NSHAPE - 1.001f);
  nf_mA = (int16_t)m; nf_mB = (int16_t)(nf_mA + 1); nf_mW = (int16_t)((m - nf_mA) * 256);
  // ── depth: distance (½ px) → phase (4096 a ring): perspective 1 / d, or even (log) ──
  float z = v6_zoom(1.0f);
  float rings = nf_rings, p = nf_persp;
  float Kl = rings * 4096.0f / logf(320.0f / 6.0f);                        // even: so many rings from the edge to 3 px
  float Kp = rings * 4096.0f * 16.0f * z;                                  // perspective: 1 / d
  static float madeR = -1, madeP = -1, madeZ = -1;                        // (only when they change)
  if (rings != madeR || p != madeP || z != madeZ || v6_first) for (int d = 0; d < NF_DN; d++) {
    if (d == 0) { madeR = rings; madeP = p; madeZ = z; }
    float dd = (d < 6 ? 6 : d) / z;
    float ph = (1.0f - p) * (-Kl * logf(dd)) + p * (Kp / (dd + 8.0f));
    nf_depth[d] = (uint16_t)(int32_t)ph;
  }
  // ── the stripes ──
  uint8_t band[256];
  float ringPx = 120.0f / (rings > 1 ? rings : 1);                         // (roughly, for the edge softness)
  o7_band(band, 256, (int)(nf_duty * 256), o7_edge(ringPx * (0.7f + 6.0f * p), 256, nf_soft));   // (perspective rings are much wider outside)
  o7_comb(nf_comb, band, 0, L.hstep);
  uint32_t ph = (uint32_t)(int32_t)(nf_z * 4096.0f);
  // the middle wanders slowly (so the tunnel bends and nothing is a mirror image)
  float tw0 = v6_t * 0.12f;
  nf_c2x = (int)(2.0f * (HALFW + 45.0f * z * o7_nsin(tw0 * 1.1f)));
  int c2y = (int)(2.0f * (HALFH + 32.0f * z * o7_ncos(tw0 * 0.9f)));
  // ── the twist: an angle for each distance (r² / 256 in ½ px), turning more toward the middle ──
  bool twist = fabsf(nf_twist) > 0.005f;
  int32_t xs = L.aspect == 100 ? 256 : (int32_t)(256 * L.aspect / 100);   // corridor: squeeze across
  if (twist) {
    static float madeT = 1e9f;
    if (nf_twist != madeT || v6_first) {
      madeT = nf_twist;
      for (int q = 0; q < 640; q++) {
        float r = sqrtf(q * 256.0f + 128.0f) * 0.5f;                        // px
        float a = r < 220.0f ? nf_twist * 6.2832f * (1.0f - r / 220.0f) : 0.0f;
        nf_tw[2 * q] = (int16_t)(cosf(a) * 16384.0f); nf_tw[2 * q + 1] = (int16_t)(sinf(a) * 16384.0f);
      }
    }
    for (int y = 0; y < H; y++) nf_rowTwist(y, 2 * y + 1 - c2y, ph, xs);
  } else {
    for (int x = 0; x < W; x++) { int dx = 2 * x + 1 - nf_c2x; nf_ax[x] = (int16_t)(((dx < 0 ? -dx : dx) * xs) >> 8); }
    for (int y = 0; y < H; y++) { int dy = 2 * y + 1 - c2y; nf_row(y, dy < 0 ? -dy : dy, ph); }
  }
  o7_end();
}

// =====================================================================
// PROGRAM: ROTO — spirals and Rotoreliefs (by Dewey) — Set 7, slot 3
//
// In 1935 Marcel Duchamp printed "Rotoreliefs": discs of off-centre circles
// and spirals meant to be spun on a record player, where they swell into
// cones, eyes and goldfish bowls. Hypnotic spirals, sunbursts and Bridget
// Riley's "Blaze" (zig-zag rings that seem to spin) are the same family:
// patterns made of angle and distance, set turning.
//
// How it works: the angle of every pixel round the middle is a table
// (worked out once, for a quarter of the screen at half size, smoothed back
// up to full size row by row); the distance term is a second table, made
// each frame. Because the pattern is the same in all four quarters (with
// the angle mirrored), each table lookup paints four pixels at once.
// Rotoreliefs, whose middle wanders, use the distance from a moving centre.
//
// Presets: k0 Hypno · k1 Rotorelief · k2 Vortex · k3 Sunburst · k4 Target · k5 Blaze ·
//          k6 Galaxy · k7 Sunflower · k8 Pinwheel · k9 Fan · k10 Drain · k11 Tour
//   k12 — REVERSE: the spin slows, stops and turns the other way
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Arms · p6 Twist · p7 Spin (◄ | ►) · p8 Thickness · p9 Edge (sharp | soft)
//   p10 Rings: Even | Deepening · p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { RL_SPIRAL, RL_BLAZE, RL_ECCENTRIC, RL_XOR };
// arms: spiral arms (0 = rings only) · rings: rings across the radius · logr: rings deepen toward the middle ·
// spin: turns a second ×100 · flow: rings flowing out (×100) · zig: Blaze zig-zags · arms2/rings2: XOR's second pattern
struct RlLook { uint8_t mode, arms, rings, logr; int8_t spin, flow; uint8_t zig, zigAmp; int8_t arms2, rings2; uint8_t duty, soft, hstep, pal, cflow, spread, sat; };
static const RlLook rl_looks[11] = {
  // mode        arms rings log spin flow zig zA arms2 rings2 duty soft hst pal            cflow spr sat
  { RL_SPIRAL   ,  2, 14, 0,  30,  0,  0,  0,   0,  0, 50,  0, 0, O7P_BW      , 0,  50,   0 },   // Hypno
  { RL_ECCENTRIC,  0, 16, 0,  40,  0,  0,  0,   0,  0, 50,  0, 1, O7P_VEGA    , 3,  90, 100 },   // Rotorelief
  { RL_SPIRAL   ,  8, 10, 1,  20,  0,  0,  0,   0,  0, 50, 10, 1, O7P_VEGA    , 4,  90, 100 },   // Vortex
  { RL_SPIRAL   , 24,  0, 0,  10,  0,  0,  0,   0,  0, 50,  0, 1, O7P_NEON    , 3,  90, 100 },   // Sunburst
  { RL_SPIRAL   ,  0, 16, 0,   0, 40,  0,  0,   0,  0, 50,  5, 1, O7P_SPECTRUM, 3, 100,  90 },   // Target
  { RL_BLAZE    ,  0, 20, 0,   0, 15, 24, 30,   0,  0, 50,  0, 1, O7P_KITAOKA , 3,  90, 100 },   // Blaze
  { RL_SPIRAL   ,  4,  8, 1,  15,  0,  0,  0,   0,  0, 45, 45, 1, O7P_NEON    , 4, 100, 100 },   // Galaxy
  { RL_XOR      , 13, 12, 1,   6,  0,  0,  0, -21, 12, 50,  0, 1, O7P_SPECTRUM, 3,  90, 100 },   // Sunflower
  { RL_XOR      , 16,  0, 0,  20,  0,  0,  0,   0, 12, 50,  0, 1, O7P_ANUSZ   , 3,  90, 100 },   // Pinwheel
  { RL_SPIRAL   , 12,  3, 0,  15,  0,  0,  0,   0,  0, 50,  5, 2, O7P_RA      , 3,  80,  90 },   // Fan
  { RL_SPIRAL   ,  1, 12, 1, -40, 20,  0,  0,   0,  0, 50, 10, 1, O7P_KITAOKA , 2,  70,  90 },   // Drain
};
static const char* const rl_names[12] = { "Hypno", "Rotorelief", "Vortex", "Sunburst", "Target", "Blaze", "Galaxy", "Sunflower", "Pinwheel", "Fan", "Drain", "Tour" };
static const char* const rl_lbl[6] = { "Arms", "Twist", "Spin", "Thickness", "Edge: Sharp|Soft", "Rings: Even|Deepening" };
const char* prog_roto_name() { return "ROTO"; }
const char* prog_roto_character() { return "Duchamp's Rotoreliefs, hypnotic spirals and Riley's Blaze, all turning"; }
const char* prog_roto_presetName(int p) { return (p >= 0 && p < 12) ? rl_names[p] : NULL; }
const char* prog_roto_potLabel(int p, int pot) { (void)p; return o7_label(pot, rl_lbl); }
uint8_t prog_roto_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the quarter angle table (80 × 61, 0 … 1024 = a quarter turn), the distance table
// (r²/16 → phase), the stripe field, one row of angles, and the squared distances across
#define RL_QW 80
#define RL_QH 61
#define RL_RN 4096
static uint16_t *rl_aq, *rl_rt, *rl_row; static uint8_t* rl_comb; static int32_t* rl_dx2;
static void rl_mem() {
  rl_aq = (uint16_t*)v6_mem;                                  // 9760
  rl_rt = (uint16_t*)(v6_mem + 9760);                         // 8192
  rl_comb = v6_mem + 17952;                                   // 4096
  rl_row = (uint16_t*)(v6_mem + 22048);                       // 320
  rl_dx2 = (int32_t*)(v6_mem + 22368);                        // 160 … 320 × 4 → up to 1280 (24576 − 22368 = 2208)
}
static bool rl_angOK = false;
static float rl_arms, rl_twist, rl_spinK, rl_duty, rl_soft, rl_log, rl_ang = 0, rl_flowP = 0, rl_rev = 1.0f, rl_revT = 1.0f, rl_ecc = 0;

void prog_roto_init() {
  rl_mem(); v6_init(); rl_rev = rl_revT = 1.0f;
  // the angle of each half-size point in the lower-right quarter, measured from +x toward +y
  for (int j = 0; j < RL_QH; j++) for (int i = 0; i < RL_QW; i++)
    rl_aq[j * RL_QW + i] = (uint16_t)(v3_atan(4 * j + 1, 4 * i + 1) * 4 + 2);   // 1024 a quarter (4096 a turn)
  rl_angOK = true;
}
// the quarter row of angles for screen row offset j (0 … 119 below the middle), smoothed to full size
static void rl_angRow(int j) {
  int tj = j >> 1, w = j & 1;
  const uint16_t* a = rl_aq + tj * RL_QW; const uint16_t* b = rl_aq + (tj + 1 < RL_QH ? tj + 1 : tj) * RL_QW;
  for (int i = 0; i < RL_QW; i++) {
    int v = w ? (a[i] + b[i] + 1) >> 1 : a[i];
    int vn = i + 1 < RL_QW ? (w ? (a[i + 1] + b[i + 1] + 1) >> 1 : a[i + 1]) : v;
    rl_row[2 * i] = (uint16_t)v; rl_row[2 * i + 1] = (uint16_t)((v + vn + 1) >> 1);
  }
}

// slow random lobes round the circle (a few harmonics with drifting phases), added by angle, so the
// four quarters are never mirror images: rings bulge, spirals breathe unevenly
static int16_t rl_lobe[256];
// four pixels per lookup: P = arms·angle + distance term + lobes (16-bit phase, 4096 a stripe, 16 stripes)
__attribute__((optimize("O2"))) static void rl_quad(int j, int32_t dy2, uint32_t armsK, uint32_t tphase, uint32_t aHalf, uint32_t aFull) {
  uint8_t* rb = v3_buf + (HALFH + j) * W + HALFW;            // lower right, going right
  uint8_t* lb = v3_buf + (HALFH + j) * W + HALFW - 1;        // lower left, going left
  uint8_t* rt = v3_buf + (HALFH - 1 - j) * W + HALFW;
  uint8_t* lt = v3_buf + (HALFH - 1 - j) * W + HALFW - 1;
  const uint16_t *row = rl_row, *rtab = rl_rt; const int32_t* dx2 = rl_dx2; const uint8_t* comb = rl_comb; const int16_t* lo = rl_lobe;
  for (int i = 0; i < HALFW; i++) {
    int a = row[i];                                                         // 0 … 1024 (a quarter turn)
    uint32_t c = rtab[(dx2[i] + dy2) >> 4] + tphase, aa = a * armsK;
    rb[i]  = comb[((c + aa + lo[a >> 4]) >> 4) & 4095];
    lb[-i] = comb[((c + aHalf - aa + lo[(2048 - a) >> 4]) >> 4) & 4095];
    lt[-i] = comb[((c + aHalf + aa + lo[((2048 + a) >> 4) & 255]) >> 4) & 4095];
    rt[i]  = comb[((c + aFull - aa + lo[((4096 - a) >> 4) & 255]) >> 4) & 4095];
  }
}
// Blaze: the zig-zag depends on angle × an even count, the same in all four quarters
__attribute__((optimize("O2"))) static void rl_quadBlaze(int j, int32_t dy2, uint32_t tphase, const uint16_t* zig, uint32_t zk) {
  uint8_t* rb = v3_buf + (HALFH + j) * W + HALFW; uint8_t* lb = rb - 1;
  uint8_t* rt = v3_buf + (HALFH - 1 - j) * W + HALFW; uint8_t* lt = rt - 1;
  const uint16_t *row = rl_row, *rtab = rl_rt; const int32_t* dx2 = rl_dx2; const uint8_t* comb = rl_comb; const int16_t* lo = rl_lobe;
  for (int i = 0; i < HALFW; i++) {
    int a = row[i];
    uint32_t c = rtab[(dx2[i] + dy2) >> 4] + tphase + zig[(a * zk >> 4) & 255];
    rb[i]  = comb[((c + lo[a >> 4]) >> 4) & 4095];
    lb[-i] = comb[((c + lo[(2048 - a) >> 4]) >> 4) & 4095];
    lt[-i] = comb[((c + lo[((2048 + a) >> 4) & 255]) >> 4) & 4095];
    rt[i]  = comb[((c + lo[((4096 - a) >> 4) & 255]) >> 4) & 4095];
  }
}
// two patterns XORed (shade tables in rl_comb: first 256 = A as shade << 4, next 256 = B as shade, then the XOR)
__attribute__((optimize("O2"))) static void rl_quadXor(int j, int32_t dy2, uint32_t armsK, uint32_t armsK2, const uint16_t* rt2, uint32_t tA, uint32_t tB,
                                                      uint32_t hA, uint32_t fA, uint32_t hB, uint32_t fB) {
  uint8_t* rb = v3_buf + (HALFH + j) * W + HALFW; uint8_t* lb = rb - 1;
  uint8_t* rt = v3_buf + (HALFH - 1 - j) * W + HALFW; uint8_t* lt = rt - 1;
  const uint16_t *row = rl_row, *rtab = rl_rt; const int32_t* dx2 = rl_dx2;
  const uint8_t *bA = rl_comb, *bB = rl_comb + 256, *xo = rl_comb + 512;
  const int16_t* lo = rl_lobe;
  for (int i = 0; i < HALFW; i++) {
    int k = (dx2[i] + dy2) >> 4, a = row[i];
    uint32_t ca = rtab[k] + tA, cb = rt2[k >> 1] + tB, aa = a * armsK, ab = a * armsK2;
    rb[i]  = xo[bA[((ca + aa + lo[a >> 4]) >> 4) & 255]                    | bB[((cb + ab) >> 4) & 255]];
    lb[-i] = xo[bA[((ca + hA - aa + lo[(2048 - a) >> 4]) >> 4) & 255]      | bB[((cb + hB - ab) >> 4) & 255]];
    lt[-i] = xo[bA[((ca + hA + aa + lo[((2048 + a) >> 4) & 255]) >> 4) & 255] | bB[((cb + hB + ab) >> 4) & 255]];
    rt[i]  = xo[bA[((ca + fA - aa + lo[((4096 - a) >> 4) & 255]) >> 4) & 255] | bB[((cb + fB - ab) >> 4) & 255]];
  }
}
// off-centre rings (Rotorelief): one pixel at a time
__attribute__((optimize("O2"))) static void rl_rowEcc(uint8_t* out, int32_t dy2, uint32_t tphase) {
  const uint16_t* rtab = rl_rt; const int32_t* dx2 = rl_dx2; const uint8_t* comb = rl_comb;
  for (int x = 0; x < W; x++) { int k = (dx2[x] + dy2) >> 4; if (k >= RL_RN) k = RL_RN - 1; out[x] = comb[((rtab[k] + tphase) >> 4) & 4095]; }
}

// a distance table: entry i covers r² = i << sh (its middle); phase = −(k1·r + k2·ln r).
// Rebuilt only when it changes, worked out exactly every 8th entry and in straight steps between.
static float rl_madeK1 = -1, rl_madeK2 = -1; static int rl_madeN = -1; static uint16_t* rl_madeT = 0;
static inline float rl_ph(int i, int sh, float k1, float k2) {
  float r = sqrtf((float)(i << sh) + (1 << sh) * 0.5f);
  return k1 * r + (k2 != 0.0f ? k2 * logf(r > 3.0f ? r : 3.0f) : 0.0f);
}
static void rl_distB(uint16_t* tab, int n, int sh, float k1, float k2);
static void rl_dist(uint16_t* tab, int n, int sh, float k1, float k2) {
  if (tab == rl_madeT && n == rl_madeN && k1 == rl_madeK1 && k2 == rl_madeK2) return;
  rl_madeT = tab; rl_madeN = n; rl_madeK1 = k1; rl_madeK2 = k2;
  for (int i = 0; i < 16 && i < n; i++) tab[i] = (uint16_t)(int32_t)(-rl_ph(i, sh, k1, k2));
  for (int i = 16; i < n; i += 8) {
    int32_t a = (int32_t)(-rl_ph(i, sh, k1, k2) * 256.0f), b = (int32_t)(-rl_ph(i + 8, sh, k1, k2) * 256.0f), d = (b - a) / 8;
    for (int k = 0; k < 8 && i + k < n; k++, a += d) tab[i + k] = (uint16_t)(a >> 8);
  }
}
static float rl_madeB1 = -1, rl_madeB2 = -1;
static void rl_distB(uint16_t* tab, int n, int sh, float k1, float k2) {
  if (k1 == rl_madeB1 && k2 == rl_madeB2) return;
  rl_madeB1 = k1; rl_madeB2 = k2;
  for (int i = 0; i < n; i++) tab[i] = (uint16_t)(int32_t)(-rl_ph(i, sh, k1, k2));
}
void prog_roto_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  rl_mem();
  if (!rl_angOK) prog_roto_init();
  const RlLook& L = rl_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&rl_arms, v3_centre(5) ? (float)L.arms : (float)(int)(v3_u(5) * 24.99f), rate);
  v6_ease(&rl_twist, v3_centre(6) ? o7_vary(6, (float)L.rings) : v3_bi(6) * 30.0f, rate);
  v6_ease(&rl_spinK, v3_centre(7) ? o7_vary(7, L.spin / 100.0f) : v3_bi(7) * fabsf(v3_bi(7)) * 1.5f, rate);
  v6_ease(&rl_duty, o7_kn(8, L.duty / 100.0f), rate);
  v6_ease(&rl_soft, o7_kn(9, L.soft / 100.0f), rate);
  v6_ease(&rl_log, v3_centre(10) ? (float)L.logr : v3_u(10), rate);
  o7_colours(L.pal, L.cflow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt, t = v6_t;
  if (v6_k12hit) rl_revT = -rl_revT;
  v6_ease(&rl_rev, rl_revT, 1.2f);
  rl_ang += sdt * rl_spinK * rl_rev; rl_ang -= floorf(rl_ang);
  rl_flowP += sdt * L.flow * 0.02f * rl_rev; rl_flowP -= floorf(rl_flowP);
  float z = v6_zoom(1.0f);
  int arms = (int)(rl_arms + 0.5f);
  // ── the distance table: r²/16 → phase (4096 a stripe), even rings or deepening (log) ──
  float rings = rl_twist;                                                  // rings across 120 px
  float lg = rl_log;
  float k1 = rings * 4096.0f / 120.0f / z * (1.0f - lg), k2 = rings * 4096.0f / z * 0.55f * lg;   // per px · per ln(r)
  int n = L.mode == RL_ECCENTRIC ? RL_RN : 2516;                           // the quarter needs r² ≤ 160² + 120²
  rl_dist(rl_rt, n, 4, k1, k2);
  // ── the stripe field (4096 a stripe, 16 stripes) ──
  float periodPx = 120.0f / (rings > 1.0f ? rings : 1.0f) * z;
  if (arms > 0) { float pa = 6.2832f * 60.0f / arms; if (pa < periodPx) periodPx = pa; }
  uint32_t tphase = (uint32_t)(int32_t)(rl_flowP * 65536.0f) + (uint32_t)(int32_t)(rl_ang * 4096.0f * (arms ? arms : 1));
  if (L.mode == RL_XOR) {
    uint8_t tmp[256];
    o7_band(tmp, 256, (int)(rl_duty * 256), o7_edge(periodPx, 256, rl_soft));
    for (int i = 0; i < 256; i++) { rl_comb[i] = (uint8_t)(tmp[i] << 4); rl_comb[256 + i] = tmp[i]; }
    for (int i = 0; i < 256; i++) rl_comb[512 + i] = (uint8_t)((o7_op[O7_XOR][i] << 4) | (L.hstep ? 0 : O7_GREY));
  } else {
    uint8_t tmp[256];
    o7_band(tmp, 256, (int)(rl_duty * 256), o7_edge(periodPx, 256, rl_soft));
    // hues repeat every g stripes, g dividing both 16 and the arms, so there is no seam where the turn closes
    int g = 16; while (arms % g) g >>= 1;
    for (int j = 0; j < 16; j++) {
      int h = L.hstep ? o7_mod15[((j % g) * L.hstep) % 60] : 0;
      for (int i = 0; i < 256; i++) rl_comb[(j << 8) | i] = (uint8_t)((tmp[i] << 4) | h);
    }
  }
  // the lobes: three harmonics round the circle, their phases and strength wandering
  {
    float tl = v6_t * 0.15f, amp = 1300.0f * o7_rnd * (0.6f + 0.4f * o7_nsin(tl * 0.7f));
    int p1 = (int)(o7_noise(tl, 11u) * 1024), p2 = (int)(o7_noise(tl * 0.8f, 22u) * 1024), p3 = (int)(o7_noise(tl * 1.3f, 33u) * 1024);
    int32_t A = (int32_t)amp;
    for (int i = 0; i < 256; i++) {
      int th = i * 4;                                                     // 1024 a turn
      int32_t v = v3_sin(th * 2 + p1) + ((v3_sin(th * 3 + p2) * 3) >> 2) + (v3_sin(th * 5 + p3) >> 1);
      rl_lobe[i] = (int16_t)((v * A) >> 15);
    }
  }
  int32_t dyS = 0;
  if (L.mode == RL_ECCENTRIC) {
    // ── Rotorelief: the rings' centre circles the middle, so the disc seems to swell and tilt ──
    rl_ecc += sdt * (0.3f + fabsf(rl_spinK) * 4.0f) * rl_rev;
    int ex = (int)(HALFW + 26 * z * cosf(rl_ecc)), ey = (int)(HALFH + 26 * z * sinf(rl_ecc));
    for (int x = 0; x < W; x++) { int d = x - ex; rl_dx2[x] = d * d; }
    for (int y = 0; y < H; y++) { int d = y - ey; rl_rowEcc(v3_buf + y * W, d * d, tphase); }
    (void)dyS; o7_end(); return;
  }
  for (int i = 0; i < HALFW; i++) { int d = 2 * i + 1; rl_dx2[i] = (d * d) >> 2; }        // (i + ½)²
  // the angle is 4096 a turn and a stripe is 4096 of phase, so each arm adds one stripe per turn
  uint32_t armsK = (uint32_t)arms, aHalf = (uint32_t)arms * 2048u, aFull = (uint32_t)arms * 4096u;
  if (L.mode == RL_BLAZE) {
    uint16_t zig[256];
    int zn = 2 * ((L.zig + 1) / 2), za = (int)(L.zigAmp * 0.01f * 4096.0f * (1.0f + 0.3f * o7_nsin(t * 0.7f)));
    for (int i = 0; i < 256; i++) zig[i] = (uint16_t)(i < 128 ? (i * za) >> 7 : ((255 - i) * za) >> 7);
    uint32_t zk = (uint32_t)zn;                                            // angle (4096 a turn) × zigs → 256-step table: ×zn ÷ 16
    for (int j = 0; j < HALFH; j++) { rl_angRow(j); rl_quadBlaze(j, ((2 * j + 1) * (2 * j + 1)) >> 2, tphase, zig, zk); }
  } else if (L.mode == RL_XOR) {
    int arms2 = L.arms2;
    // pattern B's distance table sits after pattern A's (A needs 2516 entries; B is at half the detail: r² / 32)
    uint16_t* rB = rl_rt + 2560;
    float k3 = L.rings2 * 4096.0f / 120.0f / z;
    rl_distB(rB, 1260, 5, lg > 0.5f ? 0.0f : k3, lg > 0.5f ? k3 * 66.0f : 0.0f);
    uint32_t k2a = (uint32_t)(arms2 < 0 ? -arms2 : arms2);
    uint32_t tB = (uint32_t)(int32_t)(-rl_ang * 4096.0f * (arms2 ? arms2 : 1));
    for (int j = 0; j < HALFH; j++) {
      rl_angRow(j);
      int32_t dy2 = ((2 * j + 1) * (2 * j + 1)) >> 2;
      if (arms2 >= 0) rl_quadXor(j, dy2, armsK, k2a, rB, tphase, tB, aHalf, aFull, (uint32_t)arms2 * 2048u, (uint32_t)arms2 * 4096u);
      else rl_quadXor(j, dy2, armsK, (uint32_t)0 - k2a, rB, tphase, tB, aHalf, aFull, (uint32_t)(int32_t)arms2 * 2048u, (uint32_t)(int32_t)arms2 * 4096u);
    }
  } else {
    for (int j = 0; j < HALFH; j++) { rl_angRow(j); rl_quad(j, ((2 * j + 1) * (2 * j + 1)) >> 2, armsK, tphase, aHalf, aFull); }
  }
  o7_end();
}

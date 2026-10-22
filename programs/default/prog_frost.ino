// =====================================================================
// PROGRAM: FROST — snowflakes growing (by Dewey) — Set 6, slot 5
//
// A real snowflake, grown live: water vapour drifts across a hexagonal
// grid of cells and freezes onto the crystal, and the shapes that appear —
// fern-like dendrites, broad plates, stars with sectors — come out of the
// physics, not a drawing (Clifford Reiter's model, 2005). The ice is
// coloured by when it froze, so the flake shows its growth rings, and the
// vapour it's feeding on glows faintly around it. When a flake is fully
// grown it holds, melts away, and a new one — never quite the same — begins.
//
// How it works: snowflakes are six-fold symmetric, so only one sixth of
// the flake is simulated (4656 cells); the other five are copies.
//
// Presets: k0 Stellar Dendrite · k1 Fern · k2 Sectored Plate · k3 Hex Plate ·
//          k4 Stellar Plate · k5 Needle Star · k6 Broad Branch · k7 Ice Rose ·
//          k8 Feather · k9 Diamond · k10 Surprise (a new recipe every flake) · k11 Tour
//   k12 — NEW FLAKE: melt this one and start another
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Vapour (how humid: thin dendrites ◄ | plates ►) · p6 Growth (extra frost)
//   p7 Diffusion · p8 Vapour Glow · p9 Spin · p10 Growth Rings · p11 Sparkle
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

// beta: vapour ×1000 · gamma: added frost ×100000 · alpha: diffusion ×100
struct FrLook { uint16_t beta, gamma; uint8_t alpha, pal, glow, rings, spin, flow, spread; };
static const FrLook fr_looks[11] = {
  // beta gamma alpha pal glow rings spin flow spread
  {  400, 100, 100,  4, 40, 50,  5,  3, 60 },   // Stellar Dendrite
  {  350,  50, 100,  6, 50, 60,  4,  3, 60 },   // Fern
  {  550, 200, 100,  0, 40, 70,  4,  4, 80 },   // Sectored Plate
  {  950,  10, 100,  7, 30, 80,  3,  3, 70 },   // Hex Plate
  {  650,  10, 100,  2, 40, 60,  4,  4, 70 },   // Stellar Plate
  {  300,  10, 100, 10, 50, 50,  6,  5, 80 },   // Needle Star
  {  500, 300, 100,  9, 40, 70,  4,  3, 50 },   // Broad Branch
  {  800, 200, 100,  3, 35, 80,  4,  4, 70 },   // Ice Rose
  {  380, 30, 160,  8, 50, 60,  5,  5, 80 },    // Feather
  {  450,  60, 200, 11, 40, 90,  5,  6,100 },   // Diamond
  {  500, 100, 100,  0, 40, 60,  5,  4, 80 },   // Surprise
};
static const char* const fr_names[12] = { "Stellar Dendrite", "Fern", "Sectored Plate", "Hex Plate", "Stellar Plate", "Needle Star", "Broad Branch", "Ice Rose", "Feather", "Diamond", "Surprise", "Tour" };
static const char* const fr_lbl[7] = { "Vapour", "Growth", "Diffusion", "Vapour Glow", "Spin", "Growth Rings", "Sparkle" };
const char* prog_frost_name() { return "FROST"; }
const char* prog_frost_character() { return "Snowflakes growing live: vapour freezing onto a hexagonal crystal, coloured by its growth rings"; }
const char* prog_frost_presetName(int p) { return (p >= 0 && p < 12) ? fr_names[p] : NULL; }
const char* prog_frost_potLabel(int p, int pot) { (void)p; return v6_label(pot, fr_lbl); }
uint8_t prog_frost_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── The sixth of a flake ─────────────────────────────────────────────
// Hexagonal cells (q, r) with q ≥ 1, r ≥ 0, q + r ≤ R. Row r holds q = 1 … R − r.
// The other five sixths are this one turned by 60° steps; the centre cell is kept apart.
#define FR_R 96
#define FR_N (FR_R * (FR_R + 1) / 2)
static uint16_t* fr_s;           // water, 4096 = 1.0 (frozen at ≥ 1)
static uint16_t* fr_n;           // next step
static uint8_t* fr_a;            // when it froze (0 = not yet), for the growth rings
static uint16_t fr_off[FR_R + 1];
static int fr_c = 0;             // the centre cell's water
static void fr_mem() {                                            // (fr_s and fr_n swap between the two halves every step)
  uint16_t* b = (uint16_t*)v6_mem;
  if (fr_s != b && fr_s != b + FR_N) { fr_s = b; fr_n = b + FR_N; }
  fr_a = (uint8_t*)(b + 2 * FR_N);
}
static inline int fr_idx(int q, int r) { return fr_off[r] + q - 1; }
// any cell → its twin in the stored sixth (−1 = the centre, −2 = outside)
static int fr_canon(int q, int r) {
  if (q == 0 && r == 0) return -1;
  for (int k = 0; k < 6; k++) {
    if (q >= 1 && r >= 0) return q + r > FR_R ? -2 : fr_idx(q, r);
    int t = q; q = -r; r = t + r;                                   // turn 60°
  }
  return -2;
}

static int fr_beta = 1638, fr_gamma = 4, fr_alpha = 4096;   // Q12
static int fr_time = 0, fr_steps = 0, fr_maxR = 0, fr_stage = 0;   // stage 0 growing, 1 holding, 2 melting
static float fr_hold = 0, fr_melt = 0;
static int fr_look = -1;

static void fr_start(int beta1000, int gamma1e5, int alpha100) {
  fr_beta = beta1000 * 4096 / 1000; fr_gamma = v6_max(0, gamma1e5 * 4096 / 100000); if (gamma1e5 > 0 && fr_gamma == 0) fr_gamma = 1;
  fr_alpha = alpha100 * 4096 / 100;
  for (int i = 0; i < FR_N; i++) { fr_s[i] = (uint16_t)fr_beta; fr_n[i] = (uint16_t)fr_beta; fr_a[i] = 0; }
  fr_c = 4096;                                                     // a seed of ice in the middle
  fr_s[fr_idx(1, 0)] |= 0x8000;                                    // its neighbours (all twins of (1, 0)) are receptive
  fr_time = 0; fr_steps = 0; fr_maxR = 0; fr_stage = 0; fr_hold = 0; fr_melt = 0;
}

// one step of Reiter's model. Water is kept with bit 15 = "receptive" (ice, or touching ice):
// a receptive cell keeps its water out of the diffusion (u = 0), so u = v & ((v >> 15) − 1).
#define FR_RB 0x8000
#define FR_W(v) ((v) & 0x7FFF)
#define FR_UV(v) ((int)(v) & (((int)(v) >> 15) - 1))
// the six neighbours of a cell on the edge of the stored sixth (turned into the sixth)
static int fr_edgeSum(int q, int r) {
  static const int8_t dq[6] = { 1, -1, 0, -1, 0, 1 }, dr[6] = { 0, 0, 1, 1, -1, -1 };
  int sum = 0;
  for (int k = 0; k < 6; k++) {
    int j = fr_canon(q + dq[k], r + dr[k]);
    if (j == -1) continue;                                          // the centre is ice: u = 0
    if (j == -2) sum += fr_beta; else sum += FR_UV(fr_s[j]);
  }
  return sum;
}
__attribute__((optimize("O2"))) static void fr_step() {
  const uint16_t* s = fr_s; uint16_t* n = fr_n;
  const int beta = fr_beta, gam = fr_gamma, al12 = fr_alpha / 12, al2 = fr_alpha / 2;
  fr_c += gam;                                                      // the centre: frozen from the start, collecting frost
  // only the cells near the ice need working out; further out the vapour stays at beta
  int act = fr_maxR + 28; if (act > FR_R) act = FR_R;
  for (int r = 0; r < act; r++) {
    int base = fr_off[r], rowN = FR_R - r, up = rowN, dn = rowN + 1, qEnd = act - r;
    for (int q = 1; q <= qEnd; q++) {
      int i = base + q - 1, sum;
      if (r > 0 && q > 1 && q + r < FR_R) {                         // inside: all six neighbours are in the stored sixth
        sum = FR_UV(s[i + 1]) + FR_UV(s[i - 1]) + FR_UV(s[i + up]) + FR_UV(s[i + up - 1]) + FR_UV(s[i - dn]) + FR_UV(s[i - dn + 1]);
      } else sum = fr_edgeSum(q, r);
      int v = s[i], w = FR_W(v), nv;
      if (v & FR_RB) nv = w + gam + ((sum * al12) >> 12);          // receptive: keeps its water, gains frost and half the diffusion
      else nv = w + ((sum * al12) >> 12) - ((w * al2) >> 12);         // not: its water diffuses
      if (q + r >= act) nv = beta;                                   // the edge of the working area: a steady supply of vapour
      n[i] = (uint16_t)((nv > 32767 ? 32767 : (nv < 0 ? 0 : nv)) | (v & FR_RB));
    }
  }
  uint16_t* t = fr_s; fr_s = fr_n; fr_n = t;                        // swap
  fr_time++;
  // freeze: cells that reached 1.0 become ice; their neighbours become receptive
  int stamp = 1 + ((fr_time >> 5) % 127);
  for (int r = 0; r < act; r++) {
    int base = fr_off[r], qEnd = act - r;
    for (int q = 1; q <= qEnd; q++) {
      int i = base + q - 1;
      if (FR_W(fr_s[i]) >= 4096 && !fr_a[i]) {
        fr_a[i] = (uint8_t)stamp; fr_s[i] |= FR_RB;
        if (q + r > fr_maxR) fr_maxR = q + r;
        static const int8_t dq[6] = { 1, -1, 0, -1, 0, 1 }, dr[6] = { 0, 0, 1, 1, -1, -1 };
        for (int k = 0; k < 6; k++) { int j = fr_canon(q + dq[k], r + dr[k]); if (j >= 0) fr_s[j] |= FR_RB; }
      }
    }
  }
}

static float fr_rot = 0, fr_glow, fr_rings, fr_spinK, fr_spark;
static int fr_seq = 0;

void prog_frost_init() {
  fr_s = NULL; fr_mem(); v6_init();
  for (int r = 0; r <= FR_R; r++) fr_off[r] = (uint16_t)(r * FR_R - (r * (r - 1)) / 2);
  fr_look = -1; fr_seq = 0;
  fr_start(400, 100, 100);
}

void prog_frost_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  fr_mem();
  const FrLook& L = fr_looks[look];
  float dt = v6_f.dt;
  float tGlow = v3_centre(8) ? L.glow / 100.0f : v3_u(8);
  float tRings = v3_centre(10) ? L.rings / 100.0f : v3_u(10);
  float tSpin = v3_centre(9) ? L.spin / 100.0f : v3_bi(9);
  float tSpark = v3_centre(11) ? 0.3f : v3_u(11);
  v6_ease(&fr_glow, tGlow, 2.0f); v6_ease(&fr_rings, tRings, 2.0f); v6_ease(&fr_spinK, tSpin, 2.0f); v6_ease(&fr_spark, tSpark, 2.0f);
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 180);

  // a new flake on a new look, on k12, or when the last one has melted
  bool newFlake = look != fr_look || v6_k12hit || (fr_stage == 2 && fr_melt >= 1.0f);
  if (newFlake) {
    if (look != fr_look) fr_seq = 0; else fr_seq++;
    fr_look = look;
    int beta = L.beta, gamma = L.gamma, alpha = L.alpha;
    if (look == 10 || fr_seq > 0) {                                   // each flake a little different (Surprise: anything)
      if (look == 10) { beta = 300 + v3_rnd(650); gamma = v3_rnd(300); alpha = 100 + v3_rnd(100); }
      else { beta += v3_rndr(-40, 40); gamma = v6_max(0, gamma + v3_rndr(-gamma / 3, gamma / 3)); }
    }
    if (!v3_centre(5)) beta = 250 + (int)(v3_u(5) * 720);
    if (!v3_centre(6)) gamma = (int)(v3_u(6) * v3_u(6) * 600);
    if (!v3_centre(7)) alpha = 60 + (int)(v3_u(7) * 200);
    fr_start(beta, gamma, alpha);
  }

  // ── grow: a few steps a frame (Speed) ──
  float sp = v6_f.speed;
  if (fr_stage == 0) {
    int n = v3_clamp((int)(4 * sp + 0.5f), 0, 12);
    for (int k = 0; k < n; k++) fr_step();
    fr_steps += n;
    if (fr_maxR >= FR_R - 3 || fr_steps > 9000) fr_stage = 1;
  } else if (fr_stage == 1) { fr_hold += dt * (sp > 0 ? 1 : 0); if (fr_hold > 6.0f) fr_stage = 2; }
  else { fr_melt += dt / 3.0f; }

  // ── draw: every cell of the stored sixth, six times ──
  memset(v3_buf, 0, W * H);
  fr_rot += dt * sp * fr_spinK * 0.15f;
  float zoom = v6_zoom(1.0f);
  float cs = 1.22f * zoom;                                        // cell spacing, px
  float ca = cosf(fr_rot), sa = sinf(fr_rot);
  int ex[6], ey[6], fx[6], fy[6];                                 // the two cell steps for each of the six turns (1/16 px)
  for (int k = 0; k < 6; k++) {
    float b = k * 1.0471976f, cb = cosf(b), sb = sinf(b);
    // e1 = (1, 0), e2 = (½, √3/2), turned by k·60° + rot
    float e1x = cb, e1y = sb, e2x = 0.5f * cb - 0.8660254f * sb, e2y = 0.5f * sb + 0.8660254f * cb;
    ex[k] = (int)((e1x * ca - e1y * sa) * cs * 16); ey[k] = (int)((e1x * sa + e1y * ca) * cs * 16);
    fx[k] = (int)((e2x * ca - e2y * sa) * cs * 16); fy[k] = (int)((e2x * sa + e2y * ca) * cs * 16);
  }
  int melt = fr_stage == 2 ? (int)(fr_melt * 16) : 0;
  int glowK = (int)(fr_glow * 256), ringK = (int)(fr_rings * 16), betaD = v6_max(fr_beta, 400);
  int spark = (int)(fr_spark * 255);
  uint32_t sh = v3_hash((uint32_t)fr_time * 7u);
  int big = cs > 1.6f ? 1 : 0;
  for (int r = 0; r < FR_R; r++) {
    int base = fr_off[r], rowN = FR_R - r;
    for (int q = 1; q <= rowN; q++) {
      int i = base + q - 1, ag = fr_a[i], lev, hue;
      if (ag) {                                                     // ice: brighter where it's thicker; hue by when it froze
        int thick = v3_clamp(((fr_s[i] & 0x7FFF) - 4096) >> 7, 0, 5);
        lev = 10 + thick;
        hue = (ag * ringK) >> 4;
        if (spark && ((v3_hash(i * 2654435761u ^ sh) & 255) < (unsigned)(spark >> 4))) lev = 15;   // glints
      } else {                                                      // vapour: a faint glow where it's been drawn away
        if (!glowK) continue;
        int d = betaD - (fr_s[i] & 0x7FFF);
        lev = (d * glowK) >> 16; lev = v3_clamp(lev, 0, 6);
        if (lev <= 0) continue;
        hue = 8;
      }
      lev -= melt; if (lev <= 0) continue;
      uint8_t c = (uint8_t)((lev << 4) | (hue & 15));
      for (int k = 0; k < 6; k++) {
        int x = (160 * 16 + q * ex[k] + r * fx[k] + 8) >> 4, y = (120 * 16 + q * ey[k] + r * fy[k] + 8) >> 4;
        if ((unsigned)x < W - 1 && (unsigned)y < H - 1) {
          uint8_t* p = v3_buf + y * W + x;
          if ((*p >> 4) < lev) *p = c;
          if (p[1] >> 4 < lev) p[1] = c;                            // two pixels wide and tall, so neighbouring cells meet
          if (p[W] >> 4 < lev) p[W] = c;
          if (big && (p[W + 1] >> 4 < lev)) p[W + 1] = c;
        }
      }
    }
  }
  { int lev = v3_clamp(12 - melt, 0, 15); if (lev > 0) v3_rect(159, 119, 160, 120, (uint8_t)((lev << 4) | 0)); }   // the centre
  v6_end();
}

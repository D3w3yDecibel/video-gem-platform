// =====================================================================
// PROGRAM: CYMATICS — sand singing into shapes (by Dewey) — Set 6, slot 10
//
// The siren sings. Sprinkle sand on a metal plate, play a note through it,
// and the sand dances away from the parts that shake and settles along the
// still lines between them — Chladni figures, the patterns of sound. Here
// thousands of grains do just that on square, round and hexagonal plates:
// each new note sends them leaping, and a new figure gathers out of the
// chaos. The plate itself glows faintly where it's shaking hardest.
//
// How it works: each grain takes a random hop every step, a big one where
// the plate is moving and almost none on the still (nodal) lines — so the
// grains pile up on those lines.
//
// Presets: k0 Chladni Square · k1 Hex Plate · k2 Round Plate · k3 Star Plate ·
//          k4 Penrose Sand · k5 Mandala · k6 Singing Water · k7 Two Tones ·
//          k8 Spiral · k9 Sand Storm · k10 Siren's Song · k11 Tour
//   k12 — STRIKE: hit the plate — the sand leaps and the next note sounds
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Note (◄ pick one by hand | centre = they change by themselves)
//   p6 Pitch · p7 Shake · p8 Sand (how many grains) · p9 Plate Glow
//   p10 Plate Shape (square · round · hexagon) · p11 Trails · p12-p15 live FX
// =====================================================================
#include "vg6.h"

enum { CY_SQUARE, CY_HEX, CY_ROUND, CY_STAR, CY_PENROSE, CY_MANDALA, CY_TWO, CY_SPIRAL };
struct CyLook { uint8_t fam, shape, pal, pitch, shake, sand, glow, trail, hold, flow, spread; };
static const CyLook cy_looks[11] = {
  // fam        shape pal pitch shake sand glow trail hold flow spread
  { CY_SQUARE,  0,  9, 50, 50, 80, 30, 20, 8, 3, 40 },   // Chladni Square
  { CY_HEX,     2,  0, 50, 50, 80, 30, 20, 8, 4, 60 },   // Hex Plate
  { CY_ROUND,   1,  4, 50, 50, 80, 30, 20, 8, 4, 60 },   // Round Plate
  { CY_STAR,    2,  3, 55, 50, 80, 30, 25, 8, 5, 70 },   // Star Plate
  { CY_PENROSE, 1,  8, 45, 50, 85, 30, 25, 9, 5, 80 },   // Penrose Sand
  { CY_MANDALA, 1,  2, 50, 50, 85, 35, 25, 8, 5, 80 },   // Mandala
  { CY_ROUND,   1,  6, 45, 40, 40, 90, 50, 9, 4, 70 },   // Singing Water
  { CY_TWO,     0, 10, 50, 50, 80, 40, 30, 12, 6, 80 },  // Two Tones
  { CY_SPIRAL,  1,  5, 50, 55, 80, 35, 35, 8, 6, 60 },   // Spiral
  { CY_SQUARE,  0, 11, 80, 90, 90, 40, 40, 3, 8,100 },   // Sand Storm
  { 255,        2,  0, 50, 50, 85, 35, 25, 7, 5, 70 },   // Siren's Song (every family in turn)
};
static const char* const cy_names[12] = { "Chladni Square", "Hex Plate", "Round Plate", "Star Plate", "Penrose Sand", "Mandala", "Singing Water", "Two Tones", "Spiral", "Sand Storm", "Siren's Song", "Tour" };
static const char* const cy_lbl[7] = { "Note", "Pitch", "Shake", "Sand", "Plate Glow", "Plate Shape", "Trails" };
const char* prog_cymatics_name() { return "CYMATICS"; }
const char* prog_cymatics_character() { return "Sand singing into shapes: grains dance into Chladni figures on vibrating square, round and hex plates"; }
const char* prog_cymatics_presetName(int p) { return (p >= 0 && p < 12) ? cy_names[p] : NULL; }
const char* prog_cymatics_potLabel(int p, int pot) { (void)p; return v6_label(pot, cy_lbl); }
uint8_t prog_cymatics_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── Grains (in 1/16 px from the plate's centre) ───────────────────────
#define CY_MAXG 3000
static int16_t* cy_gx; static int16_t* cy_gy;
#define CY_FN 76                            // the plate's shaking, worked out on a 4-px grid (up to 76 × 76)
static uint8_t* cy_f;
static void cy_mem() { cy_gx = (int16_t*)v6_mem; cy_gy = cy_gx + CY_MAXG; cy_f = (uint8_t*)(cy_gy + CY_MAXG); }

// ─── The note: how the plate shakes at (x, y) (px from the centre) ─────
// family, two whole numbers n, m, and a wave number k (1024ths of a turn per px, ×256)
static int cy_fam = 0, cy_n = 1, cy_m = 2, cy_famB = 0, cy_nB = 1, cy_mB = 2;
static int32_t cy_k = 256;
static float cy_mix = 0;                    // Two Tones: how much of the second note
static int32_t cy_cosT[6], cy_sinT[6];      // the wave directions for hex / star / penrose
// |amplitude| at a point, 0 … 16384
static int32_t cy_amp(int fam, int n, int m, int32_t x, int32_t y, int32_t L) {
  int32_t a;
  switch (fam) {
    case CY_SQUARE: {                                     // Chladni: cos(nπx/L)cos(mπy/L) − cos(mπx/L)cos(nπy/L)
      int32_t u = (x + L) * 512 / (2 * L), v = (y + L) * 512 / (2 * L);   // 0 … 512 = 0 … π across the plate
      int32_t p1 = (v3_cos(n * u) * v3_cos(m * v)) >> 14, p2 = (v3_cos(m * u) * v3_cos(n * v)) >> 14;
      a = (n + m) & 1 ? p1 - p2 : p1 + p2;                         // the two kinds of Chladni figure
      a >>= 1; break;
    }
    case CY_HEX: case CY_STAR: case CY_PENROSE: {         // standing waves from 3, 6 or 5 directions
      int dirs = fam == CY_HEX ? 3 : (fam == CY_STAR ? 6 : 5), sum = 0;
      for (int i = 0; i < dirs; i++) sum += v3_cos((int)(((x * cy_cosT[i] + y * cy_sinT[i]) >> 14) * cy_k >> 8));
      a = sum / dirs; break;
    }
    case CY_ROUND: case CY_MANDALA: case CY_SPIRAL: {     // round plates: rings × petals
      int32_t r = (int32_t)v3_isqrt((uint32_t)(x * x + y * y));
      int th = v3_atan(y, x);
      int pet = fam == CY_MANDALA ? 6 * m : n;
      if (fam == CY_SPIRAL) a = v3_cos((int)((r * cy_k) >> 8) + pet * th);
      else a = (v3_cos((int)((r * cy_k) >> 8)) * v3_cos(pet * th)) >> 14;
      break;
    }
    default: a = 0;
  }
  return a < 0 ? -a : a;
}
static int32_t cy_ampAll(int32_t x, int32_t y, int32_t L) {
  int32_t a = cy_amp(cy_fam, cy_n, cy_m, x, y, L);
  if (cy_mix > 0.01f) { int32_t b = cy_amp(cy_famB, cy_nB, cy_mB, x, y, L); int mx = (int)(cy_mix * 256); a = (a * (256 - mx) + b * mx) >> 8; }
  return a;
}
// is (x, y) on the plate? shape 0 square, 1 round, 2 hexagon (L = half-size, px)
static inline bool cy_on(int32_t x, int32_t y, int shape, int32_t L) {
  if (shape == 0) return x > -L && x < L && y > -L && y < L;
  if (shape == 1) return x * x + y * y < L * L;
  int32_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
  return ay < L * 222 / 256 && (ax * 222 + ay * 128) < L * 222;
}

// ─── Notes ────────────────────────────────────────────────────────────
static int cy_noteI = 0;
static void cy_pickNote(int fam, int idx, int* n, int* m) {
  static const uint8_t sq[10][2] = { {1, 2}, {1, 3}, {2, 3}, {1, 4}, {2, 4}, {3, 4}, {1, 5}, {3, 5}, {2, 6}, {4, 6} };
  int i = ((idx % 10) + 10) % 10;
  *n = sq[i][0]; *m = sq[i][1];
  (void)fam;
}
static void cy_setNote(int fam, int idx, float pitch) {
  int famUse = fam;
  if (fam == 255) famUse = (idx / 2) % 8;                       // Siren's Song: every family in turn
  if (famUse == CY_TWO) famUse = CY_SQUARE;
  cy_famB = cy_fam; cy_nB = cy_n; cy_mB = cy_m;
  cy_fam = famUse;
  cy_pickNote(famUse, idx, &cy_n, &cy_m);
  int dirs = famUse == CY_HEX ? 3 : (famUse == CY_STAR ? 6 : 5);
  for (int i = 0; i < dirs; i++) { int a = (i * 512) / dirs; cy_cosT[i] = v3_cos(a); cy_sinT[i] = v3_sin(a); }
  float kk = (0.035f + pitch * 0.07f) * (1.0f + (idx % 5) * 0.22f);   // radians per px
  cy_k = (int32_t)(kk * 163.0f * 256.0f);
}

static float cy_noteT = 0, cy_strike = 0, cy_pitch, cy_shake, cy_sand, cy_glow, cy_trail;
static int cy_ng = 0;
static void cy_scatter(int n, int shape, int32_t L) {
  for (int i = 0; i < n; i++) {
    int32_t x, y;
    do { x = v3_rndr(-L, L); y = v3_rndr(-L, L); } while (!cy_on(x, y, shape, L));
    cy_gx[i] = (int16_t)(x * 16); cy_gy[i] = (int16_t)(y * 16);
  }
}

void prog_cymatics_init() {
  cy_mem(); v6_init();
  cy_noteI = 0; cy_noteT = 0; cy_ng = 0;
  cy_setNote(CY_SQUARE, 0, 0.5f);
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

// grains hop: big hops where the plate shakes, tiny ones on the still lines
__attribute__((optimize("O2"))) static void cy_hop(int n, int shape, int32_t L, int kick) {
  const uint8_t* f = cy_f;
  for (int i = 0; i < n; i++) {
    int32_t x = cy_gx[i], y = cy_gy[i];
    // the shaking here, blended from the four nearest grid points (so the still lines stay thin)
    int32_t gx = x + ((L - 2) << 4), gy = y + ((L - 2) << 4);   // 1/16 px from the first grid point
    if (gx < 0) gx = 0; if (gy < 0) gy = 0;
    int fx = gx >> 6, fy = gy >> 6, ux = gx & 63, uy = gy & 63;
    if (fx > CY_FN - 2) { fx = CY_FN - 2; ux = 63; } if (fy > CY_FN - 2) { fy = CY_FN - 2; uy = 63; }
    const uint8_t* q = f + fy * CY_FN + fx;
    int32_t top = q[0] * (64 - ux) + q[1] * ux, bot = q[CY_FN] * (64 - ux) + q[CY_FN + 1] * ux;
    int32_t a = (top * (64 - uy) + bot * uy) >> 12;             // 0 … 255
    int32_t s = (((a >> 2) * kick) >> 8) + 1;                   // hop size, 1/16 px (up to about 4 px where it shakes most)
    uint32_t h = v3_rand();
    int32_t nx = x + ((((int32_t)(h & 255) - 128) * s) >> 7), ny = y + ((((int32_t)((h >> 8) & 255) - 128) * s) >> 7);
    if (!cy_on(nx >> 4, ny >> 4, shape, L)) { nx = x - (nx - x); ny = y - (ny - y); if (!cy_on(nx >> 4, ny >> 4, shape, L)) { nx = x; ny = y; } }
    cy_gx[i] = (int16_t)nx; cy_gy[i] = (int16_t)ny;
  }
}

void prog_cymatics_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  cy_mem();
  const CyLook& L = cy_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  float tPitch = v3_centre(6) ? L.pitch / 100.0f : v3_u(6);
  float tShake = v3_centre(7) ? L.shake / 100.0f : v3_u(7);
  float tSand = v3_centre(8) ? L.sand / 100.0f : v3_u(8);
  float tGlow = v3_centre(9) ? L.glow / 100.0f : v3_u(9);
  int shape = v3_centre(10) ? L.shape : v3_clamp(v3_k(10) * 3 / 1024, 0, 2);
  float tTrail = v3_centre(11) ? L.trail / 100.0f : v3_u(11);
  v6_ease(&cy_pitch, tPitch, 3.0f); v6_ease(&cy_shake, tShake, 3.0f); v6_ease(&cy_sand, tSand, 2.0f); v6_ease(&cy_glow, tGlow, 3.0f); v6_ease(&cy_trail, tTrail, 3.0f);
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 160);

  float zoom = v6_zoom(1.0f);
  int32_t PL = (int32_t)(108 * zoom);                            // the plate's half-size, px
  if (PL > 150) PL = 150; if (PL < 30) PL = 30;
  int ng = v3_clamp((int)(400 + cy_sand * 2600), 100, CY_MAXG);
  static int lastShape = -1;
  if (v6_first || shape != lastShape) { cy_scatter(CY_MAXG, shape, PL); lastShape = shape; }
  cy_ng = ng;

  // ── the note: it changes every few seconds (or by hand on p5), k12 strikes ──
  bool manual = !v3_centre(5);
  int manualI = manual ? v3_k(5) * 20 / 1024 : 0;
  cy_noteT += sdt;
  bool change = v6_f.fresh || v6_k12hit || (!manual && cy_noteT > L.hold) || (manual && manualI != cy_noteI);
  if (change) {
    cy_noteI = manual ? manualI : cy_noteI + 1;
    cy_setNote(L.fam, cy_noteI, cy_pitch);
    cy_noteT = 0; cy_strike = 1.0f;
    cy_mix = L.fam == CY_TWO ? 1.0f : 0.0f;                      // Two Tones: glide from the old note to the new
  }
  if (L.fam == CY_TWO) cy_mix = v3_clampf(1.0f - cy_noteT / (L.hold * 0.8f), 0.0f, 1.0f);
  cy_strike -= dt * 0.9f; if (cy_strike < 0) cy_strike = 0;

  // ── the plate's shaking on a 4-px grid (again only when the note, plate or size changes, or two notes blend) ──
  static int fNote = -1, fShape = -1; static int32_t fPL = -1;
  if (change || fNote != cy_noteI || fShape != shape || fPL != PL || cy_mix > 0.01f || v6_first) {
    fNote = cy_noteI; fShape = shape; fPL = PL;
    int nc = v6_min(CY_FN, (2 * PL) / 4 + 1);
    for (int j = 0; j < nc; j++) for (int i = 0; i < nc; i++) cy_f[j * CY_FN + i] = (uint8_t)(cy_ampAll(i * 4 + 2 - PL, j * 4 + 2 - PL, PL) >> 6);
  }
  // ── the sand hops ──
  int kick = (int)((0.5f + cy_shake * 2.0f + cy_strike * 3.0f) * 256);
  int steps = v3_clamp((int)(4 * v6_f.speed + 0.5f), 0, 12);
  for (int k = 0; k < steps; k++) cy_hop(ng, shape, PL, kick);

  // ── draw: fade (trails), the plate's glow, then the sand ──
  int fade = 15 - (int)(cy_trail * 13.0f);
  if (fade >= 15) memset(v3_buf, 0, W * H); else v3_fadeLev(v6_max(fade, 2));
  if (cy_glow > 0.03f) {                                         // the plate, shaking: worked out on a coarse grid
    int gl = (int)(cy_glow * 9);
    for (int gy = -PL; gy < PL; gy += 4) for (int gx = -PL; gx < PL; gx += 4) {
      if (!cy_on(gx + 2, gy + 2, shape, PL)) continue;
      int fi = v3_clamp((gx + PL) >> 2, 0, CY_FN - 1), fj = v3_clamp((gy + PL) >> 2, 0, CY_FN - 1);
      int lev = (int)((cy_f[fj * CY_FN + fi] * gl) >> 8);
      if (lev <= 0) continue;
      uint8_t c = (uint8_t)((lev << 4) | 8);
      for (int yy = 0; yy < 4; yy++) {
        int Y = 120 + gy + yy; if ((unsigned)Y >= H) continue;
        uint8_t* p = v3_buf + Y * W + 160 + gx;
        for (int xx = 0; xx < 4; xx++) { int X = 160 + gx + xx; if ((unsigned)X < W && (p[xx] >> 4) < lev) p[xx] = c; }
      }
    }
  }
  // the plate's rim
  {
    uint8_t rc = (uint8_t)((4 << 4) | 12);
    if (shape == 0) { v3_line(160 - PL, 120 - PL, 160 + PL, 120 - PL, rc); v3_line(160 - PL, 120 + PL, 160 + PL, 120 + PL, rc); v3_line(160 - PL, 120 - PL, 160 - PL, 120 + PL, rc); v3_line(160 + PL, 120 - PL, 160 + PL, 120 + PL, rc); }
    else if (shape == 1) v6_circle(160 * 8, 120 * 8, PL * 8, 4, 12);
    else for (int k = 0; k < 6; k++) { int a = k * 171, b = a + 171; v3_line(160 + ((v3_cos(a) * PL) >> 14), 120 + ((v3_sin(a) * PL) >> 14), 160 + ((v3_cos(b) * PL) >> 14), 120 + ((v3_sin(b) * PL) >> 14), rc); }
  }
  // the sand: brighter where grains pile up
  for (int i = 0; i < ng; i++) {
    int x = 160 + (cy_gx[i] >> 4), y = 120 + (cy_gy[i] >> 4);
    if ((unsigned)x >= W || (unsigned)y >= H) continue;
    uint8_t* p = v3_buf + y * W + x;
    int lev = (*p >> 4) + 7; if (lev > 15) lev = 15;
    int hue = ((x + y) >> 5) & 15;
    *p = (uint8_t)((lev << 4) | hue);
    if (x + 1 < W) { int l2 = (p[1] >> 4) + 3; if (l2 > 15) l2 = 15; p[1] = (uint8_t)((l2 << 4) | hue); }
  }
  v6_end();
}

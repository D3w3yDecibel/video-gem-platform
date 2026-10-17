// =====================================================================
// PROGRAM: MOIRE — op-art interference (by Dewey) — Set 3, slot 10
//
// Two simple patterns — rings, zone plates, line gratings, waves, spokes,
// checkers — laid over each other. Where they overlap, huge ghostly shapes
// appear that are in neither pattern: moiré. As the patterns drift, turn
// and breathe, the ghost shapes swirl and flow across the screen. In the
// spirit of Bridget Riley and Victor Vasarely: sharp black and white, or
// bold contrasting colours.
//
// How it works: each pattern gives every pixel a "phase" (worked out a line
// at a time with additions only — even the ring distances are tracked
// step by step, no square roots), and the two phases decide the colour.
//
// Presets:
//   k0 Zone Plates — two Fresnel zone plates drifting apart and together
//   k1 Ripples     — two sets of rings, like two stones dropped in a pond
//   k2 Current     — Riley-style waves rolling across the screen
//   k3 Vasarely    — a bulging checkerboard
//   k4 Gratings    — two line gratings turning against each other
//   k5 Sunburst    — spokes against rings
//   k6 Interference — two ring sources, the waves shaded in colour
//   k7 Hypnotic    — a turning spiral over rings
//   k8 Tartan      — crossing coloured lines
//   k9 Pulse       — breathing zone plates in neon
//   k10 Op Grid    — a grid bent by waves
//   k11 Mix        — a new pairing of patterns every ~15 s
//
//   k12 — FLIP: swaps black and white (or the colours), smoothly
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)
//   p2 Frequency A   p3 Frequency B   p4 Separation (how far apart the two centres are)
//   p5 Turn (how fast gratings and spirals turn)   p6 Warp (waves and bulges)   p7 Line Width
//   p8 Colour Mode (centre = preset: black & white, two colours, four colours, soft bands, rainbow)
//   p9 Drift (how much the centres wander)   p10 Pattern A (centre = preset)   p11 Pattern B (centre = preset)
//   p12 Breathing (frequencies swelling and shrinking)   p13 Third Layer (a slow third pattern over the top)
//   p14 Mirror (centre = preset; left none, right 2-way / 4-way)   p15 Colour Flow
// =====================================================================

#include "vg3.h"
#include "wh_lut.h"                                 // (borrowed from WORMHOLE: angles, for spokes and spirals)

// ─── Shared memory: one line of phases per pattern ────────────────────
static int16_t* mo_bulge;                          // bulge scale by distance² (for the checkerboard)
#define MO_SQN 1400                                // square roots of 0, 64, 128 … 89,536 (in 1/16 px)
static int16_t* mo_sqrt;
static int16_t* mo_ra; static int16_t* mo_rb; static int16_t* mo_rc; static int16_t* mo_wave;
static void mo_mem() { mo_ra = (int16_t*)v3_scratch; mo_rb = mo_ra + 320; mo_rc = mo_rb + 320; mo_wave = mo_rc + 320; mo_bulge = mo_wave + 320; mo_sqrt = mo_bulge + 1024; }

// ─── Patterns ─────────────────────────────────────────────────────────
enum { MP_ZONE, MP_RINGS, MP_LINES, MP_WAVES, MP_SPOKES, MP_SPIRAL, MP_CHECK, MP_NPAT };
struct MoPat { int type; int32_t cx, cy; int32_t k; int32_t ca, sa; int32_t off; int m; int warp; };
static MoPat mo_p[3];                              // patterns A, B and the third layer
// one line of pattern i's phase (256 = one stripe period) for x0 … x1
// (patterns are passed by number, not by reference: Arduino's prototype generator can't see struct types)
static void mo_row(int pi, int y, int x0, int x1, int16_t* out) {
  const MoPat& P = mo_p[pi];
  int dy = y - P.cy;
  switch (P.type) {
    case MP_ZONE: {                                                 // phase ∝ r²: a Fresnel zone plate
      int32_t dx = x0 - P.cx, d2 = dx * dx + dy * dy;
      for (int x = x0; x <= x1; x++) { out[x] = (int16_t)(((d2 >> 2) * P.k >> 10) + P.off); d2 += 2 * dx + 1; dx++; }
      break;
    }
    case MP_RINGS: case MP_SPIRAL: {                                // phase ∝ r: rings (r from a square-root table, interpolated)
      int32_t dx = x0 - P.cx, d2 = dx * dx + dy * dy;               // (centres are kept where d2 stays inside the table)
      const int32_t k = P.k, off = P.off;
      const int16_t* T = mo_sqrt;
      if (P.type == MP_RINGS) {
        for (int x = x0; x <= x1; x++) {
          uint32_t i = (uint32_t)d2 >> 6;
          int32_t t0 = T[i], r16 = t0 + (((T[i + 1] - t0) * (d2 & 63)) >> 6);   // r in 1/16 px
          out[x] = (int16_t)(((r16 * k) >> 6) + off);
          d2 += 2 * dx + 1; dx++;
        }
      } else {
        const int m = P.m, ady = v3_abs(dy) >> 1;
        const uint32_t* lrow = wh_lut + (ady < WH_QH ? ady : WH_QH - 1) * WH_QW;
        int spa = 0;
        for (int x = x0; x <= x1; x++) {
          uint32_t i = (uint32_t)d2 >> 6;
          int32_t t0 = T[i], r16 = t0 + (((T[i + 1] - t0) * (d2 & 63)) >> 6);
          if (!(x & 1) || x == x0) {                                // the angle table is in 2-px steps: look it up every other pixel
            int qx = v3_abs(dx) >> 1; if (qx >= WH_QW) qx = WH_QW - 1;
            int q = (int)(lrow[qx] & 255);
            int a = dx >= 0 ? (dy >= 0 ? q : 1024 - q) : (dy >= 0 ? 512 - q : 512 + q);
            spa = (a * m) >> 2;
          }
          out[x] = (int16_t)(((r16 * k) >> 6) + off + spa);
          d2 += 2 * dx + 1; dx++;
        }
      }
      break;
    }
    case MP_LINES: {                                                // a straight grating at an angle
      int32_t pos = ((x0 - 160) * P.ca + (y - 120) * P.sa) >> 4, st = P.ca >> 4;   // position across the lines, 1/256 px
      for (int x = x0; x <= x1; x++) { out[x] = (int16_t)(((pos * P.k) >> 12) + P.off); pos += st; }
      break;
    }
    case MP_WAVES: {                                                // horizontal bands bent by a wave along x
      int base = (y * P.k) >> 4;
      for (int x = x0; x <= x1; x++) out[x] = (int16_t)(base + mo_wave[x] + P.off);
      break;
    }
    case MP_SPOKES: {                                               // spokes round a centre
      for (int x = x0; x <= x1; x++) {
        int dx = x - P.cx;
        int qx = v3_abs(dx) >> 1, qy = v3_abs(dy) >> 1;
        if (qx >= WH_QW) qx = WH_QW - 1; if (qy >= WH_QH) qy = WH_QH - 1;
        int q = (int)(wh_lut[qy * WH_QW + qx] & 255);
        int a = dx >= 0 ? (dy >= 0 ? q : 1024 - q) : (dy >= 0 ? 512 - q : 512 + q);
        out[x] = (int16_t)(((a * P.m) >> 2) + P.off);
      }
      break;
    }
    default: {                                                      // a checkerboard, bulging out round a centre
      int32_t dx = x0 - P.cx, d2 = dx * dx + dy * dy;
      for (int x = x0; x <= x1; x++) {
        int32_t gi = d2 >> 6; if (gi > 1023) gi = 1023;
        int32_t g = mo_bulge[gi];                                     // bigger squares near the middle
        int32_t u = (dx * g) >> 8, v = (dy * g) >> 8;
        int32_t cu = ((u * P.k) >> 4) + P.off, cv = ((v * P.k) >> 4) + (P.off >> 1);
        out[x] = (int16_t)((((cu ^ cv) >> 8) & 1) ? 191 : 0);     // squares: always one side of the line-width threshold
        d2 += 2 * dx + 1; dx++;
      }
    }
  }
}

// ─── Presets ──────────────────────────────────────────────────────────
enum { MC_BW, MC_DUO, MC_QUAD, MC_SOFT, MC_RAINBOW };
struct MoLook { uint8_t a, b, pal, col, fa, fb, sep, turn, warp, duty, drift, breath, third, mirror; };
static const MoLook mo_looks[11] = {
  //  A          B          pal col      fa  fb sep turn warp duty drift breath third mir
  {MP_ZONE,   MP_ZONE,    0, MC_BW,    45, 45, 45, 10,  0, 50, 50, 20,  0, 0},   // Zone Plates
  {MP_RINGS,  MP_RINGS,   1, MC_DUO,   50, 50, 50, 10,  0, 50, 50, 15,  0, 0},   // Ripples
  {MP_WAVES,  MP_WAVES,   0, MC_BW,    55, 60, 30, 20, 60, 50, 30, 10,  0, 0},   // Current
  {MP_CHECK,  MP_ZONE,    2, MC_QUAD,  45, 15, 20, 10, 70, 50, 40, 20,  0, 0},   // Vasarely
  {MP_LINES,  MP_LINES,   3, MC_DUO,   45, 45, 30, 40,  0, 50, 30, 10,  0, 0},   // Gratings
  {MP_SPOKES, MP_RINGS,   4, MC_BW,    45, 50, 20, 30,  0, 50, 30, 10,  0, 0},   // Sunburst
  {MP_RINGS,  MP_RINGS,   5, MC_SOFT,  40, 40, 60, 10,  0, 50, 60, 20,  0, 0},   // Interference
  {MP_SPIRAL, MP_ZONE,    6, MC_QUAD,  45, 30, 10, 50,  0, 50, 20, 15,  0, 0},   // Hypnotic
  {MP_LINES,  MP_LINES,   7, MC_RAINBOW, 35, 40, 0, 15, 0, 40, 20, 10,  0, 0},   // Tartan
  {MP_ZONE,   MP_ZONE,    4, MC_QUAD,  55, 50, 30, 10,  0, 50, 40, 70,  0, 2},   // Pulse
  {MP_WAVES,  MP_LINES,   0, MC_BW,    50, 50, 20, 10, 50, 30, 20, 10,  0, 0},   // Op Grid
};
static const char* const mo_presetNames[12] = {
  "Zone Plates", "Ripples", "Current", "Vasarely", "Gratings", "Sunburst",
  "Interference", "Hypnotic", "Tartan", "Pulse", "Op Grid", "Mix"
};
static const char* const mo_labels[16] = {
  "Palette", "Speed", "Frequency A", "Frequency B", "Separation", "Turn", "Warp", "Line Width",
  "Colour Mode", "Drift", "Pattern A", "Pattern B", "Breathing", "Third Layer", "Mirror", "Colour Flow"
};
const char* prog_moire_name() { return "MOIRE"; }
const char* prog_moire_character() { return "Op-art moire: rings, zone plates, gratings and waves interfering into flowing ghost shapes"; }
const char* prog_moire_presetName(int preset) { return (preset >= 0 && preset < 12) ? mo_presetNames[preset] : NULL; }
const char* prog_moire_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? mo_labels[pot] : ""; }
uint8_t prog_moire_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// four colours per palette: [00] [01] [10] [11] of the two patterns' stripes
#define MO_NPAL 8
static const uint8_t mo_pals[MO_NPAL][4][3] = {
  {{255,255,255},{0,0,0},{0,0,0},{255,255,255}},            // 0 Black & White
  {{0,10,40},{0,220,255},{255,0,150},{255,255,255}},        // 1 Neon Pond
  {{90,0,140},{255,140,0},{0,170,160},{255,230,40}},        // 2 Vasarely
  {{255,255,255},{220,0,30},{0,40,200},{10,10,10}},         // 3 Riley
  {{0,0,0},{255,220,0},{255,40,0},{255,255,255}},           // 4 Sun
  {{20,0,60},{0,160,255},{255,40,160},{240,240,255}},       // 5 Interference
  {{0,0,0},{170,0,255},{0,255,140},{255,255,255}},          // 6 Hypno
  {{30,60,30},{220,40,40},{40,80,220},{240,220,120}},       // 7 Tartan
};

// ─── State ────────────────────────────────────────────────────────────
static int mo_preset = -1, mo_look = 0, mo_palNow = 0, mo_palPrev = 0;
static float mo_t = 0, mo_palX = 1, mo_flip = 0, mo_flipT = 0, mo_mixT = 0, mo_flowT = 0;
static int mo_mixA = 0, mo_mixB = 0;
static bool mo_k12was = false;
static uint8_t mo_combo[4], mo_soft[128];
static int mo_gen = 0;                              // bumped on init: the shared memory may have been overwritten

void prog_moire_init() {
  mo_mem(); v3_reset(); mo_preset = -1; mo_gen++;
  for (int i = 0; i < MO_SQN; i++) mo_sqrt[i] = (int16_t)v3_isqrt((uint32_t)i * 64 * 256);
}

static void mo_palette(int colMode, float flow) {
  int xf = (int)(mo_palX * 256);
  uint8_t c[4][3];
  for (int i = 0; i < 4; i++) for (int q = 0; q < 3; q++) c[i][q] = v3_mix8(mo_pals[mo_palPrev][i][q], mo_pals[mo_palNow][i][q], xf);
  // flip: swap the colours smoothly
  int fl = (int)(mo_flip * 256);
  uint8_t d[4][3];
  for (int i = 0; i < 4; i++) for (int q = 0; q < 3; q++) d[i][q] = v3_mix8(c[i][q], c[3 - i][q], fl);
  // hues 0-3: the four stripe combinations; hues 4-11: a wheel through them (soft and rainbow modes)
  for (int i = 0; i < 4; i++) v3_setLH(i, d[i][0], d[i][1], d[i][2], 1.0f, 0);
  for (int h = 0; h < 8; h++) {
    float p = h * 0.5f + flow; p -= floorf(p / 4.0f) * 4.0f;
    int i0 = (int)p, i1 = (i0 + 1) & 3, f = (int)((p - i0) * 256);
    int r = v3_mix8(d[i0][0], d[i1][0], f), g = v3_mix8(d[i0][1], d[i1][1], f), b = v3_mix8(d[i0][2], d[i1][2], f);
    if (colMode == MC_RAINBOW) { uint8_t o[3]; v3_hsv((int)(h * 192 + flow * 384), 230, 255, o); r = o[0]; g = o[1]; b = o[2]; }
    v3_setLH(4 + h, r, g, b, 1.0f, 0);
  }
  // how stripe combinations become pixels
  switch (colMode) {
    case MC_BW:   mo_combo[0] = 0xF0 | 0; mo_combo[1] = 0xF0 | 1; mo_combo[2] = 0xF0 | 1; mo_combo[3] = 0xF0 | 0; break;   // XOR
    case MC_DUO:  mo_combo[0] = 0xF0 | 0; mo_combo[1] = 0xF0 | 1; mo_combo[2] = 0xF0 | 2; mo_combo[3] = 0xF0 | 0; break;
    default:      for (int i = 0; i < 4; i++) mo_combo[i] = (uint8_t)(0xF0 | i); break;
  }
  for (int i = 0; i < 128; i++) {                                    // soft: the summed stripes as bands of colour
    int lev = 3 + (i * 12) / 127;
    mo_soft[i] = (uint8_t)((lev << 4) | (4 + ((i >> 4) & 7)));
  }
}

static void mo_setPat(int pi, int type, float freq, float cx, float cy, float ang, float off, int m, int warp) {
  MoPat& P = mo_p[pi];
  P.type = type; P.cx = (int32_t)cx; P.cy = (int32_t)cy; P.off = (int32_t)off; P.m = m; P.warp = warp;
  P.ca = (int32_t)(cosf(ang) * 4096); P.sa = (int32_t)(sinf(ang) * 4096);
  switch (type) {
    case MP_ZONE: P.k = (int32_t)(120 + freq * 1400); break;          // rings get tighter outwards
    case MP_RINGS: case MP_SPIRAL: P.k = (int32_t)(1024.0f / (3.0f + (1 - freq) * 22.0f)); break;   // period 3-25 px (r is in 1/2 px, phase >> 3)
    case MP_LINES: P.k = (int32_t)(256.0f / (3.0f + (1 - freq) * 20.0f) * 16); break;
    case MP_WAVES: P.k = (int32_t)(256.0f / (4.0f + (1 - freq) * 18.0f) * 16); break;
    case MP_CHECK: P.k = (int32_t)(256.0f / (8.0f + (1 - freq) * 30.0f) * 16); break;
    default: P.k = 16; break;
  }
}

void prog_moire_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  mo_mem();
  v3_frame();
  float dt = v3_dt;
  bool mix = preset == 11;
  if (preset != mo_preset) {
    mo_preset = preset; mo_look = mix ? v3_rnd(11) : preset; mo_mixT = 0;
    mo_palNow = mo_palPrev = mo_looks[mo_look].pal; mo_palX = 1;
    mo_mixA = mo_looks[mo_look].a; mo_mixB = mo_looks[mo_look].b;
  }
  if (mix) {
    mo_mixT += dt;
    if (mo_mixT > 15.0f) {
      mo_mixT = 0; mo_look = (mo_look + 1 + v3_rnd(10)) % 11;
      mo_mixA = v3_rnd(MP_NPAT); mo_mixB = v3_rnd(MP_NPAT);
      mo_palPrev = mo_palNow; mo_palNow = v3_rnd(MO_NPAL); mo_palX = 0;
    }
  }
  const MoLook& L = mo_looks[mo_look];

  // ── knobs ──
  float speed = v3_speed(1, 3.0f);
  float fa = v3_centre(2) ? L.fa / 100.0f : v3_u(2), fb = v3_centre(3) ? L.fb / 100.0f : v3_u(3);
  float sep = v3_centre(4) ? L.sep / 100.0f : v3_u(4);
  float turn = v3_centre(5) ? L.turn / 100.0f : v3_bi(5);
  float warp = v3_centre(6) ? L.warp / 100.0f : v3_u(6);
  float duty = v3_centre(7) ? L.duty / 100.0f : 0.15f + v3_u(7) * 0.58f;
  int colMode = v3_pick(8, L.col, 5);
  float drift = v3_centre(9) ? L.drift / 100.0f : v3_u(9);
  int pa = v3_pick(10, mix ? mo_mixA : L.a, MP_NPAT), pb = v3_pick(11, mix ? mo_mixB : L.b, MP_NPAT);
  float breath = v3_centre(12) ? L.breath / 100.0f : v3_u(12);
  float third = v3_u(13) < 0.1f || v3_centre(13) ? (v3_centre(13) ? L.third / 100.0f : 0.0f) : v3_u(13);
  int mirror = v3_pick(14, L.mirror, 3);
  float flowK = v3_k(15) < 30 ? 0.0f : (v3_centre(15) ? 0.15f : v3_u(15));
  int pal = v3_centre(0) ? (mix ? mo_palNow : L.pal) : v3_clamp(v3_k(0) * MO_NPAL / 1024, 0, MO_NPAL - 1);
  if (pal != mo_palNow && mo_palX >= 1) { mo_palPrev = mo_palNow; mo_palNow = pal; mo_palX = 0; }
  if (mo_palX < 1) { mo_palX += dt / 2.0f; if (mo_palX > 1) mo_palX = 1; }

  // ── time ──
  float sdt = dt * speed;
  mo_t += sdt;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !mo_k12was) mo_flipT = mo_flipT > 0.5f ? 0.0f : 1.0f;
  mo_k12was = k12;
  mo_flip += (mo_flipT - mo_flip) * v3_clampf(dt * 3.0f, 0, 1);
  mo_flowT += dt * flowK;
  mo_palette(colMode, mo_flowT * 4.0f);

  // ── the two patterns ──
  float t = mo_t;
  float br = 1.0f + breath * 0.25f * sinf(t * 0.7f);
  float d = 20 + sep * 110;                                         // half the distance between the centres
  float wx = drift * 40, wy = drift * 30;
  float ax = 160 - d * cosf(t * 0.13f) + wx * sinf(t * 0.37f), ay = 120 - d * 0.5f * sinf(t * 0.11f) + wy * sinf(t * 0.29f + 1);
  float bx = 160 + d * cosf(t * 0.13f) + wx * sinf(t * 0.31f + 2), by = 120 + d * 0.5f * sinf(t * 0.11f) + wy * cosf(t * 0.23f);
  if (pa == MP_SPOKES || pa == MP_SPIRAL || pa == MP_CHECK) { ax = 160 + (ax - 160) * 0.5f; ay = 120 + (ay - 120) * 0.4f; }
  if (pb == MP_SPOKES || pb == MP_SPIRAL || pb == MP_CHECK) { bx = 160 + (bx - 160) * 0.5f; by = 120 + (by - 120) * 0.4f; }
  ax = v3_clampf(ax, 82, 238); ay = v3_clampf(ay, 62, 178); bx = v3_clampf(bx, 82, 238); by = v3_clampf(by, 62, 178);
  float angA = t * turn * 0.25f, angB = -t * turn * 0.21f + 0.04f + sep * 0.15f;
  if (pa == MP_LINES && pb == MP_LINES && colMode == MC_RAINBOW) angB = angA + 1.5708f;   // tartan: crossing lines
  mo_setPat(0, pa, v3_clampf(fa * br, 0, 1), ax, ay, angA, -t * 40, 6 + (int)(fa * 18), (int)(warp * 900));
  mo_setPat(1, pb, v3_clampf(fb / br, 0, 1), bx, by, angB, t * 33, 8 + (int)(fb * 16), (int)(warp * 900));
  bool useC = third > 0.05f;
  if (useC) mo_setPat(2, MP_RINGS, 0.05f, 160 + 60 * sinf(t * 0.07f), 120 + 40 * cosf(t * 0.09f), 0, t * 20, 0, 0);
  // the wave table (for wave patterns): a sine along x
  {
    int amp = (int)(warp * 260), f = 6 + (int)(fa * 12);
    for (int x = 0; x < W; x++) mo_wave[x] = (int16_t)((v3_sin(x * f / 4 + (int)(t * 90)) * amp) >> 14);
  }
  {
    static int bk = -1, bgen = -1;
    int nk = (int)(warp * 900);
    if (nk != bk || bgen != mo_gen) { bk = nk; bgen = mo_gen; for (int i = 0; i < 1024; i++) mo_bulge[i] = (int16_t)(256 + (nk * 256) / (256 + i)); }
  }
  int dutyI = (int)(duty * 256);
  int dutyC = (int)(third * 0.5f * 256);

  // ── pixels ──
  int xMax = mirror >= 1 ? 159 : 319, yMax = mirror >= 2 ? 119 : 239;
  const uint8_t* combo = mo_combo;
  for (int y = 0; y <= yMax; y++) {
    mo_row(0, y, 0, xMax, mo_ra);
    mo_row(1, y, 0, xMax, mo_rb);
    if (useC) mo_row(2, y, 0, xMax, mo_rc);
    uint8_t* out = v3_buf + y * W;
    if (colMode == MC_SOFT) {
      for (int x = 0; x <= xMax; x++) {
        int a = mo_ra[x] & 255, b = mo_rb[x] & 255;
        int ta = a < 128 ? a : 255 - a, tb = b < 128 ? b : 255 - b;  // triangle waves 0 … 127
        out[x] = mo_soft[(ta + tb) >> 1];
      }
    } else if (colMode == MC_RAINBOW) {
      for (int x = 0; x <= xMax; x++) {
        int a = mo_ra[x] & 255, b = mo_rb[x] & 255;
        int on = (a < dutyI) | ((b < dutyI) << 1);
        out[x] = on == 0 ? (uint8_t)0x30 | 4 : (uint8_t)((on == 3 ? 0xF0 : 0xB0) | (4 + ((((mo_ra[x] >> 8) & 3) + ((mo_rb[x] >> 8) & 3) * 2 + on) & 7)));
      }
    } else {
      for (int x = 0; x <= xMax; x++) {
        int on = ((mo_ra[x] & 255) < dutyI) | (((mo_rb[x] & 255) < dutyI) << 1);
        out[x] = combo[on];
      }
    }
    if (useC) for (int x = 0; x <= xMax; x++) if ((mo_rc[x] & 255) < dutyC) out[x] ^= 0x01;   // the third layer flips colours where it passes
  }
  if (mirror >= 1) for (int y = 0; y <= yMax; y++) { uint8_t* r = v3_buf + y * W; for (int x = 0; x < 160; x++) r[319 - x] = r[x]; }
  if (mirror >= 2) for (int y = 0; y < 120; y++) memcpy(v3_buf + (239 - y) * W, v3_buf + y * W, W);
}

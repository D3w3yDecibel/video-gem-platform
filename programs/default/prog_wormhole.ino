// =====================================================================
// PROGRAM: WORMHOLE — flight down an endless tunnel (by Dewey) — Set 3, slot 4
//
// You fly down a tunnel that never ends. The walls carry a pattern —
// spiral bands, checkers, hyperspace streaks, data, neon grids, scales —
// that rushes past in perspective, fades into the distance and ends in a
// glowing light. The tunnel winds and twists, rings of light race towards
// you, and every so often a new section with a new pattern appears far
// ahead and sweeps over you. k12 kicks in the warp drive.
//
// How it works (the classic demoscene tunnel): a table in flash gives the
// angle and depth of every 2×2-pixel block as seen from the vanishing point
// (one quarter of it, mirrored), so each block is just a few table lookups
// into a 64×64 wall texture. Moving the vanishing point bends the tunnel.
//
// Presets:
//   k0 Wormhole      — twisting spiral bands
//   k1 Checker       — the classic checkerboard tunnel
//   k2 Hyperspace    — star streaks rushing past in the dark
//   k3 Data Stream   — a square shaft of glowing data bits
//   k4 Neon Corridor — a square corridor of neon grid lines
//   k5 Vortex        — fiery rings in a hard twist
//   k6 Rings         — rainbow rings with light pulses
//   k7 Snake         — a winding tube of scales
//   k8 Flower        — a petal-shaped tunnel of liquid colour
//   k9 Catacomb      — an old brick tunnel
//   k10 Warp Drive   — rainbow streaks and speed surges
//   k11 Journey      — a new section, pattern and colour every few seconds
//
//   k12 — BOOST: warp speed for a moment
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = hover)   p2 Spin (centre = preset, left reverses)
//   p3 Twist (centre = preset, left twists the other way)   p4 Wander (how much the tunnel winds)
//   p5 Tiles Around   p6 Tiles Along   p7 Shape (centre = preset; left round → square, right 3-8 petals)
//   p8 Fog   p9 Light Pulses   p10 Pattern (centre = preset)   p11 Colour Flow (far left = still)
//   p12 Wander Speed   p13 Core Glow   p14 New Sections (how often; far left = never)
//   p15 Surges (automatic warp boosts)
// =====================================================================

#include "vg3.h"
#include "wh_lut.h"

// ─── Shared-memory layout (v3_scratch) ────────────────────────────────
static uint8_t* wh_texA;                           // 64×64 wall texture, (level << 4) | hue
static uint8_t* wh_texB;                           // the next section's texture
static uint8_t* wh_fog;                            // [64 depths][16 levels] → level << 4
static void wh_mem() { wh_texA = v3_scratch; wh_texB = v3_scratch + 4096; wh_fog = v3_scratch + 8192; }

// ─── Palettes: 8 wall colours (hues 1-8) and the light at the end (hue 15) ──
#define WH_NPAL 8
static const uint8_t wh_pals[WH_NPAL][9][3] = {
  {{255,0,170},{0,230,255},{255,220,0},{60,40,255},{255,90,0},{0,255,140},{200,0,255},{255,255,255},{255,240,255}},   // 0 Neon
  {{255,40,0},{255,140,0},{255,230,40},{180,0,20},{255,255,200},{255,90,30},{120,0,0},{255,200,120},{255,250,220}},   // 1 Inferno
  {{0,120,255},{120,220,255},{255,255,255},{0,40,160},{0,255,230},{160,140,255},{20,80,200},{200,255,255},{230,250,255}},// 2 Ice
  {{0,255,70},{0,150,40},{180,255,120},{0,90,30},{120,255,0},{0,255,200},{40,200,40},{230,255,230},{220,255,220}},      // 3 Matrix
  {{255,40,150},{120,30,220},{255,140,30},{0,200,220},{255,220,90},{60,0,140},{255,100,200},{0,255,255},{255,230,240}}, // 4 Synth
  {{255,0,0},{255,140,0},{255,255,0},{0,220,0},{0,140,255},{140,0,255},{255,0,200},{255,255,255},{255,255,255}},        // 5 Rainbow
  {{255,190,0},{120,80,20},{255,240,170},{60,30,0},{220,120,0},{255,220,90},{90,60,30},{255,255,230},{255,240,200}},    // 6 Gold
  {{170,255,0},{150,0,255},{0,255,170},{80,0,140},{255,255,0},{255,0,120},{40,160,0},{230,190,255},{240,255,220}},      // 7 Toxic
};

// ─── Wall patterns (64×64, tiling) ────────────────────────────────────
enum { WP_SPIRAL, WP_CHECK, WP_STREAK, WP_DATA, WP_GRID, WP_RINGS, WP_SCALES, WP_PLASMA, WP_BRICK, WP_RAINBOW, WP_N };
static inline void wh_tp(uint8_t* t, int u, int v, int lev, int hue) { t[((v & 63) << 6) | (u & 63)] = (uint8_t)((v3_clamp(lev, 0, 15) << 4) | hue); }
static void wh_makeTex(uint8_t* t, int pat) {
  memset(t, 0, 4096);
  switch (pat) {
    case WP_SPIRAL:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int s = (u + v) & 63, band = s >> 4, f = s & 15;
        wh_tp(t, u, v, f < 2 ? 2 : 15 - f / 2, 1 + band);
      }
      break;
    case WP_CHECK:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int c = ((u >> 3) ^ (v >> 3)) & 1;
        bool edge = (u & 7) == 0 || (v & 7) == 0;
        wh_tp(t, u, v, edge ? 15 : (c ? 13 : 9), edge ? 3 : (c ? 1 : 2));
      }
      break;
    case WP_STREAK:
      for (int k = 0; k < 110; k++) {
        int u = v3_rnd(64), v0 = v3_rnd(64), len = 5 + v3_rnd(22), hue = 1 + v3_rnd(4);
        for (int i = 0; i < len; i++) wh_tp(t, u, v0 + i, 15 - (len - 1 - i) * 12 / len, hue == 4 ? 8 : hue);
      }
      break;
    case WP_DATA:
      for (int cv = 0; cv < 16; cv++) for (int cu = 0; cu < 16; cu++) {
        int r = v3_rnd(100);
        if (r < 55) continue;
        int hue = r < 85 ? 1 : (r < 96 ? 3 : 8), lev = 8 + v3_rnd(8);
        for (int y = 0; y < 3; y++) for (int x = 0; x < 3; x++) wh_tp(t, cu * 4 + x, cv * 4 + y, lev - (x == 2 || y == 2 ? 4 : 0), hue);
      }
      for (int u = 0; u < 64; u += 16) for (int v = 0; v < 64; v++) wh_tp(t, u, v, 5, 2);
      break;
    case WP_GRID:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int du = u & 15, dv = v & 7;
        if (du == 0) wh_tp(t, u, v, 15, 1);
        else if (dv == 0) wh_tp(t, u, v, 15, 2);
        else if (du == 1 || du == 15) wh_tp(t, u, v, 6, 1);
        else if (dv == 1 || dv == 7) wh_tp(t, u, v, 5, 2);
        else wh_tp(t, u, v, ((u >> 4) + (v >> 3)) % 5 == 0 ? 3 : 1, 4);
      }
      break;
    case WP_RINGS:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int band = (v >> 4) & 3, f = v & 15;
        wh_tp(t, u, v, f == 0 ? 15 : 14 - f * 3 / 4, f == 0 ? 8 : 1 + band);
      }
      break;
    case WP_SCALES:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int row = v >> 3, uu = (u + (row & 1) * 4) & 7, vv = v & 7;
        int d2 = (uu - 4) * (uu - 4) + (vv - 2) * (vv - 2) * 2;     // a scale: round top, pointed bottom
        int hue = ((row >> 1) + ((u + (row & 1) * 4) >> 3)) % 3 == 0 ? 2 : 1;
        if (d2 > 26) wh_tp(t, u, v, 3, 3);
        else if (d2 > 19) wh_tp(t, u, v, 15, 3);
        else wh_tp(t, u, v, 14 - d2 / 3, hue);
      }
      break;
    case WP_PLASMA:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int s = v3_sin(u * 32) + v3_sin(v * 48 + 100) + v3_sin((u + v) * 16 + 300) + v3_sin((u - v) * 16);   // −4 … 4 (Q14)
        int k = (s + 65536) * 6 / 131073;                         // 0 … 5
        int f = ((s + 65536) * 6 % 131073) * 16 / 131073;
        wh_tp(t, u, v, 6 + (f < 8 ? f : 15 - f) * 9 / 7, 1 + k);
      }
      break;
    case WP_BRICK:
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int row = v >> 3, uu = (u + (row & 1) * 8) & 15, vv = v & 7;
        uint32_t h = v3_hash((uint32_t)(((u + (row & 1) * 8) >> 4) * 131 + row * 7));
        if (uu == 0 || vv == 0) wh_tp(t, u, v, 3, 4);
        else wh_tp(t, u, v, 9 + (int)(h % 5) - (vv == 7 || uu == 15 ? 3 : 0), (h >> 8) % 3 == 0 ? 2 : 1);
      }
      break;
    default:  // WP_RAINBOW
      for (int v = 0; v < 64; v++) for (int u = 0; u < 64; u++) {
        int band = (u * 6) >> 6, f = v & 15;
        wh_tp(t, u, v, (u * 6 & 63) < 6 ? 2 : 15 - f * 2 / 3, 1 + band);
      }
      for (int k = 0; k < 40; k++) { int u = v3_rnd(64), v0 = v3_rnd(64); for (int i = 0; i < 10; i++) wh_tp(t, u, v0 + i, 15, 8); }
      break;
  }
}

// ─── Presets ──────────────────────────────────────────────────────────
struct WhLook { uint8_t pat, pal, shape, petals; int8_t spin, twist; uint8_t wander, repU, repV, fog, pulse, flow, glow; uint8_t speed; };
//                     pat         pal shape petals spin twist wander repU repV fog pulse flow glow speed
static const WhLook wh_looks[11] = {
  {WP_SPIRAL,   4, 0, 0,  20,  30, 60, 2, 2, 50, 20, 20, 40, 50},   // Wormhole
  {WP_CHECK,    0, 0, 0,   8,   0, 40, 2, 2, 50,  0,  0, 30, 55},   // Checker
  {WP_STREAK,   2, 0, 0,   3,   0, 30, 3, 4, 30,  0, 10, 70, 90},   // Hyperspace
  {WP_DATA,     3, 1, 0,   0,   0, 35, 2, 2, 60, 40,  0, 30, 45},   // Data Stream
  {WP_GRID,     4, 1, 0,   0,   0, 50, 2, 3, 55, 50, 15, 35, 55},   // Neon Corridor
  {WP_RINGS,    1, 0, 0,  45,  70, 55, 2, 2, 45, 30, 30, 50, 50},   // Vortex
  {WP_RINGS,    5, 0, 0,  10,   0, 45, 1, 2, 40, 70, 40, 40, 45},   // Rings
  {WP_SCALES,   7, 0, 0,   6,  10, 90, 3, 3, 55,  0, 10, 30, 40},   // Snake
  {WP_PLASMA,   5, 2, 5,  15,  20, 50, 1, 1, 45, 20, 60, 40, 35},   // Flower
  {WP_BRICK,    6, 1, 0,   0,   0, 60, 2, 2, 75, 10,  0, 25, 35},   // Catacomb
  {WP_RAINBOW,  5, 0, 0,  25,  15, 50, 1, 4, 35, 60, 50, 60, 85},   // Warp Drive
};
static const char* const wh_presetNames[12] = {
  "Wormhole", "Checker", "Hyperspace", "Data Stream", "Neon Corridor", "Vortex",
  "Rings", "Snake", "Flower", "Catacomb", "Warp Drive", "Journey"
};
static const char* const wh_labels[16] = {
  "Palette", "Speed", "Spin", "Twist", "Wander", "Tiles Around", "Tiles Along", "Shape",
  "Fog", "Light Pulses", "Pattern", "Colour Flow", "Wander Speed", "Core Glow", "New Sections", "Surges"
};
const char* prog_wormhole_name() { return "WORMHOLE"; }
const char* prog_wormhole_character() { return "Flight down an endless twisting tunnel of patterns, towards the light at the end"; }
const char* prog_wormhole_presetName(int preset) { return (preset >= 0 && preset < 12) ? wh_presetNames[preset] : NULL; }
const char* prog_wormhole_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? wh_labels[pot] : ""; }
uint8_t prog_wormhole_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int wh_preset = -1, wh_look = 0, wh_patA = 0, wh_patB = -1, wh_palNow = 0, wh_palPrev = 0;
static int32_t wh_trav8 = 0;                      // distance flown, 1/256 texels
static float wh_palX = 1.0f, wh_spin = 0, wh_wT = 0, wh_flowT = 0, wh_pulseT = 0, wh_segT = 0;
static float wh_boost = 0, wh_surgeT = 4;
static int32_t wh_bound = 0x7fffffff;              // world position where the next section starts
static bool wh_k12was = false;
static uint8_t wh_lightPix[16];
static int16_t wh_petal[1024 / 8];                 // petal shape: depth scale by angle (every 8th step)

static int wh_fogKeyReset = 0;
void prog_wormhole_init() {
  wh_mem();
  wh_fogKeyReset++;
  v3_reset();
  wh_preset = -1;
}

static void wh_palette(float flow) {
  int xf = (int)(wh_palX * 256);
  // Colour Flow: the wall colours drift round the palette, smoothly
  float f = flow - floorf(flow / 8.0f) * 8.0f;
  int sh = (int)f, fr = (int)((f - sh) * 256);
  for (int h = 1; h <= 8; h++) {
    int c0 = (h - 1 + sh) & 7, c1 = (h + sh) & 7;
    int col[3];
    for (int c = 0; c < 3; c++) {
      int a = v3_mix8(wh_pals[wh_palPrev][c0][c], wh_pals[wh_palNow][c0][c], xf);
      int b = v3_mix8(wh_pals[wh_palPrev][c1][c], wh_pals[wh_palNow][c1][c], xf);
      col[c] = v3_mix8(a, b, fr);
    }
    v3_setLH(h, col[0], col[1], col[2], 1.2f, 0);
  }
  int lc[3];
  for (int c = 0; c < 3; c++) lc[c] = v3_mix8(wh_pals[wh_palPrev][8][c], wh_pals[wh_palNow][8][c], xf);
  v3_setLH(15, lc[0], lc[1], lc[2], 1.0f, 0);
}

void prog_wormhole_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  wh_mem();
  v3_frame();
  float dt = v3_dt;
  bool journey = preset == 11;
  if (preset != wh_preset) {
    wh_preset = preset;
    wh_look = journey ? v3_rnd(11) : preset;
    wh_palNow = wh_palPrev = wh_looks[wh_look].pal; wh_palX = 1;
    wh_patA = wh_looks[wh_look].pat; wh_makeTex(wh_texA, wh_patA);
    wh_patB = -1; wh_bound = 0x7fffffff; wh_segT = 0;
  }
  const WhLook& L = wh_looks[wh_look];

  // ── knobs ──
  float speed = v3_k(1) < 30 ? 0.0f : (L.speed / 50.0f) * powf(3.5f, v3_bi(1));
  float spin = v3_centre(2) ? L.spin / 100.0f : v3_bi(2) * 0.8f;
  float twist = v3_centre(3) ? L.twist / 100.0f : v3_bi(3) * 1.4f;
  float wander = v3_centre(4) ? L.wander / 100.0f : v3_u(4);
  int repU = v3_pick(5, L.repU, 4) + (v3_centre(5) ? 0 : 1); if (repU < 1) repU = 1;
  int repV = v3_pick(6, L.repV, 4) + (v3_centre(6) ? 0 : 1); if (repV < 1) repV = 1;
  int shape = L.shape, petals = L.petals;                          // 0 round, 1 square, 2 petals
  if (!v3_centre(7)) {
    int k = v3_k(7);
    if (k < 440) { shape = k < 220 ? 0 : 1; petals = 0; }
    else { shape = 2; petals = 3 + (k - 600) * 6 / 424; if (petals > 8) petals = 8; }
  }
  float fog = v3_centre(8) ? L.fog / 100.0f : v3_u(8);
  float pulse = v3_centre(9) ? L.pulse / 100.0f : v3_u(9);
  int pat = v3_pick(10, -1, WP_N);
  float flowK = v3_k(11) < 30 ? 0.0f : (v3_centre(11) ? L.flow / 100.0f : v3_u(11));
  float wSpeed = v3_rel(12, 1.0f, 3.0f);
  float glow = v3_centre(13) ? L.glow / 100.0f : v3_u(13);
  float segEvery = v3_k(14) < 30 ? 0.0f : (v3_centre(14) ? (journey ? 9.0f : 0.0f) : 40.0f - v3_u(14) * 34.0f);
  float surge = v3_u(15);
  if (!v3_centre(0)) { int p = v3_clamp(v3_k(0) * WH_NPAL / 1024, 0, WH_NPAL - 1); if (p != wh_palNow) { wh_palPrev = wh_palNow; wh_palNow = p; wh_palX = 0; } }
  if (wh_palX < 1) { wh_palX += dt / 2.0f; if (wh_palX > 1) wh_palX = 1; }

  // ── warp boost ──
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !wh_k12was) wh_boost = 1.0f;
  wh_k12was = k12;
  if (surge > 0.05f) { wh_surgeT -= dt; if (wh_surgeT <= 0) { wh_boost = 0.6f + 0.4f * v3_rndf(); wh_surgeT = 3.0f + (1.0f - surge) * 14.0f + v3_rndf() * 3; } }
  wh_boost *= expf(-dt * 1.1f);
  float vel = speed * (1.0f + 4.0f * wh_boost);

  // ── move ──
  wh_trav8 += (int32_t)(vel * dt * 26.0f * 256.0f);               // texels per second along the tunnel
  if (wh_trav8 > (1 << 30)) { wh_trav8 -= 1 << 29; if (wh_bound != 0x7fffffff) wh_bound -= 1 << 21; }   // (keeps the patterns lined up)
  wh_spin += spin * dt * (1.0f + wh_boost) * 160.0f;
  wh_wT += dt * wSpeed * (0.4f + 0.6f * speed) ;
  wh_flowT += dt * flowK * 0.8f;
  wh_pulseT += dt * (30.0f + vel * 26.0f);
  int32_t travel = wh_trav8 >> 8;

  // ── new sections: the next pattern appears far ahead and sweeps over you ──
  if (pat >= 0 && pat != wh_patA && wh_patB < 0) { wh_patB = pat; wh_makeTex(wh_texB, pat); wh_bound = travel + 140; }
  if (segEvery > 0 && pat < 0) {
    wh_segT += dt;
    if (wh_segT > segEvery && wh_patB < 0) {
      wh_segT = 0;
      int np = (wh_patA + 1 + v3_rnd(WP_N - 1)) % WP_N;            // a different pattern
      wh_patB = np; wh_makeTex(wh_texB, np); wh_bound = travel + 140;
      if (journey) { int nl = v3_rnd(11); if (v3_centre(0)) { wh_palPrev = wh_palNow; wh_palNow = wh_looks[nl].pal; wh_palX = 0; } }
    }
  }
  if (wh_patB >= 0 && travel > wh_bound + 2) { memcpy(wh_texA, wh_texB, 4096); wh_patA = wh_patB; wh_patB = -1; wh_bound = 0x7fffffff; }
  if (pat < 0 && !journey && segEvery == 0 && wh_patA != L.pat && wh_patB < 0) { wh_patB = L.pat; wh_makeTex(wh_texB, L.pat); wh_bound = travel + 140; }

  // ── colours, fog and the light ──
  wh_palette(wh_flowT);
  {
    static int fogKey = -1, fogGen = -1;
    int fk = (int)(fog * 200);
    if (fogGen != wh_fogKeyReset) { fogGen = wh_fogKeyReset; fogKey = -1; }
    if (fk != fogKey) {                                            // rebuilt only when Fog changes
      fogKey = fk;
      for (int zf = 0; zf < 64; zf++) {
        float d = zf / 63.0f;                                      // 0 near … 1 far
        float k = 1.0f - fog * powf(d, 0.55f) * 0.95f;
        for (int l = 0; l < 16; l++) wh_fog[(zf << 4) | l] = (uint8_t)(v3_clamp((int)(l * k + 0.5f), 0, 15) << 4);
      }
    }
  }
  int zLight = (int)(6400.0f / (1.5f + glow * 16.0f));            // the light at the end: core radius 1.5 … 17 blocks
  int lightSh = 0; while (lightSh < 10 && ((4095 - zLight) >> lightSh) > 15) lightSh++;
  for (int i = 0; i < 16; i++) wh_lightPix[i] = (uint8_t)(((1 + i * 14 / 15) << 4) | 15);
  // light pulses: bright rings travelling towards you, every 128 texels
  uint8_t pulseTab[128];
  int pAmp = (int)(pulse * 9);
  for (int i = 0; i < 128; i++) { int d = i < 64 ? i : 128 - i; pulseTab[i] = (uint8_t)(d < 6 ? pAmp * (6 - d) / 6 : 0); }
  int pulsePh = (int)wh_pulseT;
  {
    float amp = shape == 2 ? 0.28f : 0.0f;
    for (int i = 0; i < 128; i++) wh_petal[i] = (int16_t)(256 * (1.0f + amp * sinf(i * 8 * 6.2831853f / 1024.0f * petals + wh_spin * 0.004f)));
  }

  // ── the vanishing point wanders: the tunnel bends ──
  int cx = 80 + (int)(wander * (28 * sinf(wh_wT * 0.53f) + 12 * sinf(wh_wT * 1.31f + 1.0f)));
  int cy = 60 + (int)(wander * (20 * sinf(wh_wT * 0.71f + 2.0f) + 9 * sinf(wh_wT * 1.13f)));
  cx = v3_clamp(cx, 40, 120); cy = v3_clamp(cy, 30, 90);
  int spinA = (int)wh_spin & 1023;
  int tw = (int)(twist * 40);
  const uint8_t* fogT = wh_fog;
  const uint8_t* tA = wh_texA; const uint8_t* tB = wh_patB >= 0 ? wh_texB : wh_texA;
  int32_t bound = wh_patB >= 0 ? wh_bound : 0x7fffffff;
  int zShift = shape == 1 ? 20 : 8;

  for (int by = 0; by < 120; by++) {
    int dy = by - cy, qy = dy < 0 ? -dy : dy;
    const uint32_t* lrow = wh_lut + qy * WH_QW;
    uint16_t* r0 = (uint16_t*)(v3_buf + (by * 2) * W);
    uint16_t* r1 = (uint16_t*)(v3_buf + (by * 2 + 1) * W);
    for (int side = 0; side < 2; side++) {
      int bx0 = side ? cx : 0, bx1 = side ? 160 : cx;               // left of the vanishing point, then right
      int abase = side ? (dy >= 0 ? 0 : 1024) : (dy >= 0 ? 512 : 512);
      int asgn = side ? (dy >= 0 ? 1 : -1) : (dy >= 0 ? -1 : 1);
      for (int bx = bx0; bx < bx1; bx++) {
        int qx = side ? bx - cx : cx - bx;
        uint32_t e = lrow[qx];
        int z = (int)((e >> zShift) & 4095);
        int a = (abase + asgn * (int)(e & 255)) & 1023;
        if (shape == 2) z = (z * wh_petal[a >> 3]) >> 8;
        uint8_t pix;
        if (z >= zLight) {
          int li = (z - zLight) >> lightSh; pix = wh_lightPix[li > 15 ? 15 : li];
        } else {
          int u = ((((a + spinA) & 1023) * repU) >> 4) + ((z * tw) >> 10);
          int32_t vw = ((z * repV) >> 4) + travel;
          const uint8_t* tex = vw >= bound ? tB : tA;
          int t = tex[((vw & 63) << 6) | (u & 63)];
          int zf = z >> 6;
          int lev = fogT[(zf << 4) | (t >> 4)] + (pulseTab[(vw + pulsePh) & 127] << 4);
          if (lev > 0xF0) lev = 0xF0;
          pix = (uint8_t)(lev | (t & 15));
        }
        uint16_t p2 = (uint16_t)(pix | (pix << 8));
        r0[bx] = p2; r1[bx] = p2;
      }
    }
  }
}

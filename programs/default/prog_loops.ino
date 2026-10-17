// =====================================================================
// PROGRAM: LOOPS — self-replicating machines (by Dewey) — Set 3 — v2
// Square loops on a grid, each carrying a little signal train that circles
// inside it. A loop grows an arm and builds a copy of itself next door,
// turns, and builds another — colonies spread like crystals. When a build
// crashes into wreckage or another loop, the gene it passes on is damaged:
// usually into a SMALLER mutant. Smaller loops copy faster, so they spread
// and take over. Each size of loop has its own colour, so you can watch new
// species rise and old ones die out.
//
// v2: the world lives in its own grid and the screen is a CAMERA flying over
// it — drifting, panning, following one machine, touring with fly-out
// zooms, breathing in and out, or starting close on one loop and pulling
// back as its colony fills the world (the world wraps, so pulled right back
// it tiles like wallpaper). Cells can be drawn as blocks, dots, outlines,
// soft glows or a blueprint.
//
// Inspired by ekspertodniczego's "Eighty identical machines went in. None
// of the originals survived." (https://www.instagram.com/reels/DeXoMf1ukEE/),
// after Sayama's evolving self-replicating loops. The code here is an
// original loop-level simulation, not the original cell rules.
//
// Presets (camera · cell style):
//   k0 Evolution    — scattered loops; mutants take over   (drift · blocks)
//   k1 Colony       — one perfect loop's crystal colony    (pull back · glow)
//   k2 Eighty       — eighty identical machines in a grid  (zoom breathing · blocks)
//   k3 Two Tribes   — big loops vs small loops             (side pan · outlines)
//   k4 Rainbow      — wild mutation, lineage colours       (follow · dots)
//   k5 Wreckage     — wreckage lasts and becomes a maze    (wide, tiled · blocks)
//   k6 Giants       — a few huge machines                  (close follow · glow)
//   k7 Micro        — tiny loops in their thousands        (tour · dots)
//   k8 Signals      — small world, signal trains bright    (close follow · glow)
//   k9 Arena        — walls, no wrapping round             (fixed · blueprint)
//   k10 Bloom       — one colony slowly mutating           (pull back · blocks)
//   k11 Endless     — fresh machines dropped in now & then (tour · blocks)
//
//   k12 — INJECT: drop a handful of fresh machines in at random
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette          p1 Speed (far left = freeze)
//   p2 Count            p3 Loop Size         p4 Mutation       p5 Wreckage fade
//   p6 Zoom (×0.45 … ×2.2 on the camera's own zoom)
//   p7 Camera Motion (far left = still … centre = preset … right = 4× faster)
//   p8 Pan X   p9 Pan Y (offset the camera; centre = none)
//   p10 Style (blocks / dots / outlines / glow / blueprint; centre = preset)
//   p11 Glow            p12 Colour By (size / lineage / age; centre = preset)
//   p13 Lifespan        p14 Inject Rate (left = off, right = often)
//   p15 Epoch (far left = never restart; otherwise start again once the
//       original machines have been extinct for 60 … 20 (centre) … 5 s)
//
// If the camera ever finds itself looking at empty space for a second, it
// glides to the nearest machine — whatever the knobs say, there's always
// something to watch.
// =====================================================================

#include "vg_sin.h"
static inline int sr_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float sr_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static uint32_t sr_rng = 2024u;
static inline uint32_t sr_rand() { sr_rng = sr_rng * 1664525u + 1013904223u; return sr_rng >> 8; }
static inline float sr_randf() { return sr_rand() * (1.0f / 16777216.0f); }

// ─── Colours ──────────────────────────────────────────────────────────
// hues 0-7 species colours (big … small), 8 signal, 9 arm / building, 10 grid / walls
#define SR_NSCHEME 8
static const uint8_t sr_cols[SR_NSCHEME][11][3] = {
  {{40,120,255},{0,200,255},{0,255,200},{120,255,80},{255,230,40},{255,140,30},{255,50,80},{255,60,200},{255,255,255},{255,255,255},{40,70,120}},    // 0 Evo (cyan → red)
  {{80,80,255},{140,60,255},{220,40,255},{255,40,170},{255,40,90},{255,90,40},{255,170,30},{255,240,60},{255,255,255},{200,255,255},{80,50,130}},   // 1 Ultraviolet
  {{0,255,140},{40,255,60},{160,255,0},{230,255,40},{255,200,0},{255,120,0},{255,60,40},{255,30,120},{255,255,255},{255,255,200},{30,110,50}},     // 2 Toxic
  {{180,220,255},{120,180,255},{100,140,255},{140,120,255},{200,120,255},{255,120,220},{255,140,160},{255,180,140},{255,255,255},{255,255,255},{70,70,120}},  // 3 Pastel
  {{0,180,255},{0,230,255},{40,255,230},{60,255,140},{255,240,60},{255,150,40},{255,60,60},{255,40,180},{255,255,255},{255,255,255},{30,70,130}},  // 4 Evoloop (centre)
  {{255,255,255},{220,220,230},{190,190,205},{160,160,180},{130,130,155},{100,100,130},{80,80,110},{60,60,90},{255,60,60},{255,200,60},{70,70,90}},  // 5 Mono (signals red)
  {{255,120,0},{255,170,30},{255,220,80},{255,255,160},{200,255,120},{120,255,160},{80,220,255},{120,140,255},{255,255,255},{255,255,255},{110,60,20}}, // 6 Sunset
  {{0,255,0},{0,220,40},{0,190,70},{0,160,100},{60,200,120},{120,255,160},{180,255,200},{220,255,230},{255,255,255},{200,255,200},{0,90,30}},      // 7 Phosphor
};
static int16_t sr_levK[16];
static void sr_buildPalette(int scheme) {
  const uint8_t (*c)[3] = sr_cols[scheme];
  for (int h = 0; h < 16; h++) {
    int r = 0, g = 0, b = 0;
    if (h < 11) { r = c[h][0]; g = c[h][1]; b = c[h][2]; }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = sr_levK[l];
      display.setColor((l << 4) | h, r * k >> 8, g * k >> 8, b * k >> 8);
    }
  }
}
#define SR_WALL ((7 << 4) | 10)
#define SR_GRID ((3 << 4) | 10)

// ─── The world: its own grid of cells (level << 4 | hue) ──────────────
#define SR_WMAX 128
#define SR_HMAX 96
static uint8_t sr_world[SR_WMAX * SR_HMAX] __attribute__((aligned(4)));   // aligned: the fade reads it 4 bytes at a time (the RP2040 faults on misaligned words)
static uint8_t* sr_buf;                           // the screen
static int sr_GW = 128, sr_GH = 96, sr_wrap = 1;
#define SR_OCC 4                                  // a cell is taken if its brightness is at least this
static inline bool sr_inside(int cx, int cy) { return cx >= 0 && cy >= 0 && cx < sr_GW && cy < sr_GH; }
static inline void sr_norm(int* cx, int* cy) {
  if (!sr_wrap) return;
  if (*cx < 0) *cx += sr_GW; else if (*cx >= sr_GW) *cx -= sr_GW;
  if (*cy < 0) *cy += sr_GH; else if (*cy >= sr_GH) *cy -= sr_GH;
}
static inline bool sr_taken(int cx, int cy) {
  sr_norm(&cx, &cy);
  if (!sr_inside(cx, cy)) return true;                           // walls
  return (sr_world[cy * sr_GW + cx] >> 4) >= SR_OCC;
}
static inline void sr_cell(int cx, int cy, int hue, int lev) {
  sr_norm(&cx, &cy);
  if (!sr_inside(cx, cy) || lev <= 0) return;
  if (lev > 15) lev = 15;
  sr_world[cy * sr_GW + cx] = (uint8_t)((lev << 4) | hue);
}
static inline uint32_t sr_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void sr_fade(int dec) {
  uint32_t* b = (uint32_t*)sr_world;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  int n = sr_GW * sr_GH / 4;
  for (int i = 0; i < n; i++) b[i] = sr_fade4(b[i], d4);
}

// ─── The loops ────────────────────────────────────────────────────────
#define SR_MAX 160
#define SR_IDLE 0
#define SR_ARM 1
#define SR_BUILD 2
static int16_t sr_x[SR_MAX], sr_y[SR_MAX];        // top-left cell
static uint8_t sr_n[SR_MAX];                      // size (cells per side) — this loop's body
static uint8_t sr_gene[SR_MAX];                   // the size it will build its children at
static uint8_t sr_dir[SR_MAX], sr_tries[SR_MAX], sr_state[SR_MAX], sr_prog[SR_MAX], sr_lin[SR_MAX];
static uint16_t sr_age[SR_MAX], sr_wait[SR_MAX];
static uint8_t sr_orig[SR_MAX];                   // 1 = still carries the original, undamaged gene
static int sr_count = 0;
static int sr_gap = 2;                            // arm length (cells between parent and child)
static float sr_mut = 0.6f, sr_kill = 0.25f, sr_grow = 0.1f;
static int sr_life = 400;                         // ticks a loop may live
static int sr_colorBy = 0, sr_glow = 13, sr_sig = 15;
static int sr_n0 = 10;                            // the size the experiment started with

// the i-th cell (clockwise from the top-left) of a ring of side n at (x,y)
static void sr_ringCell(int x, int y, int n, int i, int* cx, int* cy) {
  int e = n - 1;
  if (i < e) { *cx = x + i; *cy = y; }
  else if (i < 2 * e) { *cx = x + e; *cy = y + (i - e); }
  else if (i < 3 * e) { *cx = x + e - (i - 2 * e); *cy = y + e; }
  else { *cx = x; *cy = y + e - (i - 3 * e); }
}
// where the child of a loop sits, and its arm cells, for a direction (0 right, 1 up, 2 left, 3 down)
static void sr_childAt(int x, int y, int n, int cn, int d, int* chx, int* chy) {
  int mid = n / 2 - cn / 2;                                       // line the child up on the arm
  switch (d) {
    case 0: *chx = x + n + sr_gap; *chy = y + mid; break;
    case 1: *chx = x + mid; *chy = y - cn - sr_gap; break;
    case 2: *chx = x - cn - sr_gap; *chy = y + mid; break;
    default: *chx = x + mid; *chy = y + n + sr_gap; break;
  }
}
static void sr_armCell(int x, int y, int n, int d, int i, int* cx, int* cy) {
  int m = n / 2;
  switch (d) {
    case 0: *cx = x + n + i; *cy = y + m; break;
    case 1: *cx = x + m; *cy = y - 1 - i; break;
    case 2: *cx = x - 1 - i; *cy = y + m; break;
    default: *cx = x + m; *cy = y + n + i; break;
  }
}

static int sr_hueFor(int k) {
  if (sr_colorBy == 1) return sr_lin[k] & 7;                      // lineage
  if (sr_colorBy == 2) return sr_clamp(sr_age[k] * 8 / (sr_life + 1), 0, 7);   // age
  return sr_clamp(sr_n0 - sr_n[k] + 1, 0, 7);                     // size: bigger = 0, originals = 1, smaller mutants 2 … 7
}

static int sr_addO(int x, int y, int n, int gene, int dir, int lin, int orig) {
  if (sr_count >= SR_MAX) return -1;
  int k = sr_count++;
  sr_norm(&x, &y);
  sr_x[k] = (int16_t)x; sr_y[k] = (int16_t)y; sr_n[k] = (uint8_t)n; sr_gene[k] = (uint8_t)gene;
  sr_dir[k] = (uint8_t)(dir & 3); sr_tries[k] = 0; sr_state[k] = SR_IDLE; sr_prog[k] = 0; sr_lin[k] = (uint8_t)lin;
  sr_age[k] = 0; sr_wait[k] = (uint16_t)(n + (sr_rand() % (n + 1)));
  sr_orig[k] = (uint8_t)orig;
  return k;
}
static void sr_remove(int k) {                                    // a loop dies: it stops being redrawn → wreckage
  int j = --sr_count;
  if (k != j) {
    sr_x[k] = sr_x[j]; sr_y[k] = sr_y[j]; sr_n[k] = sr_n[j]; sr_gene[k] = sr_gene[j];
    sr_dir[k] = sr_dir[j]; sr_tries[k] = sr_tries[j]; sr_state[k] = sr_state[j]; sr_prog[k] = sr_prog[j];
    sr_lin[k] = sr_lin[j]; sr_age[k] = sr_age[j]; sr_wait[k] = sr_wait[j]; sr_orig[k] = sr_orig[j];
  }
}
static bool sr_ringFree(int x, int y, int n) {
  for (int i = 0; i < 4 * (n - 1); i++) { int cx, cy; sr_ringCell(x, y, n, i, &cx, &cy); if (sr_taken(cx, cy)) return false; }
  return true;
}
// which live loop covers a cell (for crashes that kill)
static int sr_owner(int cx, int cy) {
  for (int k = 0; k < sr_count; k++) {
    int dx = cx - sr_x[k], dy = cy - sr_y[k];
    if (sr_wrap) { if (dx < 0) dx += sr_GW; if (dy < 0) dy += sr_GH; }
    if (dx >= 0 && dy >= 0 && dx < sr_n[k] && dy < sr_n[k]) return k;
  }
  return -1;
}
// a crash: the gene this loop passes on is damaged (usually smaller), and the loop hit may die
static void sr_crash(int k, int hx, int hy) {
  if (sr_randf() < sr_mut) {
    int g = sr_gene[k];
    float r = sr_randf();
    if (r < sr_grow) g += 1;
    else if (r < sr_grow + 0.15f) g -= 2;
    else g -= 1;
    sr_gene[k] = (uint8_t)sr_clamp(g, 3, 19);                    // a size-3 child is born sterile and soon dies
    sr_lin[k] = (uint8_t)(sr_lin[k] * 5 + 3 + (sr_rand() & 3));
    sr_orig[k] = 0;                                              // its signal train is damaged: no longer an original
  }
  if (sr_randf() < sr_kill) { int o = sr_owner(hx, hy); if (o >= 0 && o != k) sr_remove(o); }
}

// one step of one loop; returns false if it died (the caller must not advance k)
static bool sr_step(int k) {
  sr_age[k]++;
  int maxAge = sr_life * sr_n[k] / (sr_n0 > 0 ? sr_n0 : 1) + sr_n[k] * 6;   // small loops copy fast but die young
  if (sr_age[k] > maxAge || sr_n[k] < 4) { sr_remove(k); return false; }
  int x = sr_x[k], y = sr_y[k], n = sr_n[k], d = sr_dir[k], cn = sr_gene[k];
  switch (sr_state[k]) {
    case SR_IDLE:
      if (sr_wait[k] > 0) { sr_wait[k]--; break; }
      if (sr_tries[k] >= 4) { sr_remove(k); return false; }      // nowhere left to build: the loop dies
      { int ax, ay; sr_armCell(x, y, n, d, 0, &ax, &ay);
        if (sr_taken(ax, ay)) { sr_tries[k]++; sr_dir[k] = (uint8_t)((d + 1) & 3); sr_wait[k] = 2; }
        else { sr_state[k] = SR_ARM; sr_prog[k] = 0; } }
      break;
    case SR_ARM: {
      if (sr_prog[k] >= sr_gap) { sr_state[k] = SR_BUILD; sr_prog[k] = 0; break; }
      int ax, ay; sr_armCell(x, y, n, d, sr_prog[k], &ax, &ay);
      if (sr_taken(ax, ay) && sr_prog[k] > 0) { sr_crash(k, ax, ay); sr_tries[k]++; sr_dir[k] = (uint8_t)((d + 1) & 3); sr_state[k] = SR_IDLE; sr_wait[k] = (uint16_t)n; break; }
      sr_prog[k]++;
      break;
    }
    case SR_BUILD: {
      int chx, chy; sr_childAt(x, y, n, cn, d, &chx, &chy);
      int total = 4 * (cn - 1);
      if (sr_prog[k] >= total) {                                  // finished: the child comes alive
        sr_addO(chx, chy, cn, cn, (d + 1) & 3, sr_lin[k], sr_orig[k]);
        sr_tries[k]++; sr_dir[k] = (uint8_t)((d + 1) & 3); sr_state[k] = SR_IDLE; sr_wait[k] = (uint16_t)n;
        break;
      }
      int cx, cy; sr_ringCell(chx, chy, cn, sr_prog[k], &cx, &cy);
      if (!sr_wrap && !sr_inside(cx, cy)) { sr_tries[k]++; sr_dir[k] = (uint8_t)((d + 1) & 3); sr_state[k] = SR_IDLE; sr_wait[k] = (uint16_t)n; break; }
      if (sr_taken(cx, cy)) {                                     // crash mid-build: the half-built child is left as wreckage
        sr_crash(k, cx, cy);
        sr_tries[k]++; sr_dir[k] = (uint8_t)((d + 1) & 3); sr_state[k] = SR_IDLE; sr_wait[k] = (uint16_t)n;
        break;
      }
      sr_cell(cx, cy, 9, sr_glow);                                // lay the new cell now, so others see it at once
      sr_prog[k]++;
      break;
    }
  }
  return true;
}

// draw one live loop into the world: its ring, the signal train circling it, its arm and the child being built
static int sr_tick = 0;
static void sr_draw(int k) {
  int x = sr_x[k], y = sr_y[k], n = sr_n[k], hue = sr_hueFor(k);
  int per = 4 * (n - 1);
  int lev = sr_glow;
  for (int i = 0; i < per; i++) { int cx, cy; sr_ringCell(x, y, n, i, &cx, &cy); sr_cell(cx, cy, hue, lev); }
  if (sr_sig > 0) {                                               // the gene, as a few bright cells running round the ring
    int marks = 2 + sr_gene[k] / 4;
    int ph = (sr_tick + k * 7) % per;
    for (int m = 0; m < marks; m++) {
      int i = (ph + m * 2) % per;
      int cx, cy; sr_ringCell(x, y, n, i, &cx, &cy);
      sr_cell(cx, cy, 8, sr_sig - (m == 0 ? 0 : 2));
    }
  }
  if (n >= 5) sr_cell(x + n / 2, y + n / 2, hue, lev - 6);        // a faint core
  int d = sr_dir[k];
  if (sr_state[k] == SR_ARM || sr_state[k] == SR_BUILD) {
    int len = sr_state[k] == SR_ARM ? sr_prog[k] : sr_gap;
    for (int i = 0; i < len; i++) { int ax, ay; sr_armCell(x, y, n, d, i, &ax, &ay); sr_cell(ax, ay, 9, lev); }
  }
  if (sr_state[k] == SR_BUILD) {
    int chx, chy, cn = sr_gene[k]; sr_childAt(x, y, n, cn, d, &chx, &chy);
    for (int i = 0; i < sr_prog[k]; i++) { int cx, cy; sr_ringCell(chx, chy, cn, i, &cx, &cy); sr_cell(cx, cy, 9, lev - 1); }
  }
}

// ─── Seeding ──────────────────────────────────────────────────────────
#define SM_SCATTER 0
#define SM_SINGLE 1
#define SM_GRID 2
#define SM_TRIBES 3
static void sr_seed(int mode, int count, int n0, int lin0) {
  int tries = 0;
  if (mode == SM_SINGLE) { sr_addO(sr_GW / 2 - n0 / 2, sr_GH / 2 - n0 / 2, n0, n0, 0, lin0, 1); return; }
  if (mode == SM_GRID) {
    int pitch = n0 + sr_gap + 2;
    int cols = sr_GW / pitch, rows = sr_GH / pitch;
    int placed = 0;
    int ox = (sr_GW - cols * pitch) / 2, oy = (sr_GH - rows * pitch) / 2;
    for (int r = 0; r < rows && placed < count; r++) for (int c = 0; c < cols && placed < count; c++) {
      if (sr_addO(ox + c * pitch, oy + r * pitch, n0, n0, (int)(sr_rand() & 3), lin0, 1) >= 0) placed++;
    }
    return;
  }
  for (int i = 0; i < count && tries < count * 20; tries++) {
    int n = n0, lin = lin0 + (int)(sr_rand() % 3);
    int xmin = 0, xmax = sr_GW - n;
    if (mode == SM_TRIBES) {
      bool big = i & 1;
      n = big ? sr_clamp(n0 + 3, 4, sr_GH / 5) : sr_clamp(n0 - 3, 4, 19);
      lin = big ? 1 : 5;
      if (big) xmax = sr_GW / 2 - n; else xmin = sr_GW / 2;
      if (xmax < xmin) xmax = xmin;
    }
    int x = xmin + (int)(sr_rand() % (uint32_t)(xmax - xmin + 1)), y = (int)(sr_rand() % (uint32_t)(sr_GH - n + 1));
    if (!sr_ringFree(x - 1, y - 1, n + 2)) continue;              // keep a little space round each one
    if (sr_addO(x, y, n, n, (int)(sr_rand() & 3), lin, n == n0 ? 1 : 0) < 0) return;
    for (int j = 0; j < 4 * (n - 1); j++) { int cx, cy; sr_ringCell(x, y, n, j, &cx, &cy); sr_cell(cx, cy, 0, SR_OCC); }   // mark it taken now
    i++;
  }
}

// ─── Camera ───────────────────────────────────────────────────────────
#define CAM_FIXED 0      // the whole world, still
#define CAM_DRIFT 1      // a slow wandering glide
#define CAM_PAN 2        // a steady sideways pan
#define CAM_FOLLOW 3     // ride along with one machine and its descendants
#define CAM_TOUR 4       // fly from machine to machine, pulling out mid-flight
#define CAM_BREATHE 5    // the zoom slowly breathes in and out
#define CAM_GROW 6       // start close on the first loop and pull back as the colony spreads
#define CAM_WIDE 7       // pulled right back: the wrapped world tiles like wallpaper
static float sr_camX, sr_camY, sr_camZ;          // view centre (cells) and zoom (px per cell)
static float sr_tgX, sr_tgY;                     // follow / tour target
static float sr_camT, sr_legT, sr_reT;           // camera clock, time in this leg, time since re-aim
static float sr_l0X, sr_l0Y, sr_l0Z, sr_lDX, sr_lDY, sr_l1Z;   // a tour leg: start, travel, end zoom
static float sr_offX, sr_offY, sr_seekT;         // auto-seek offset when the view is empty
static bool sr_snap = false;                     // after a (re)start: jump straight to a machine if none is in view
static float sr_wdf(float d, float G) {          // shortest way round the wrapped world
  if (!sr_wrap) return d;
  while (d >= G * 0.5f) d -= G;
  while (d < -G * 0.5f) d += G;
  return d;
}
static inline float sr_lcx(int k) { return sr_x[k] + sr_n[k] * 0.5f; }
static inline float sr_lcy(int k) { return sr_y[k] + sr_n[k] * 0.5f; }
static int sr_nearest(float px, float py) {
  int best = -1; float bd = 1e30f;
  for (int k = 0; k < sr_count; k++) {
    float dx = sr_wdf(sr_lcx(k) - px, (float)sr_GW), dy = sr_wdf(sr_lcy(k) - py, (float)sr_GH);
    float d = dx * dx + dy * dy;
    if (d < bd) { bd = d; best = k; }
  }
  return best;
}
static void sr_pickTarget() {                    // a random machine, preferring one that is building
  if (sr_count == 0) { sr_tgX = sr_GW * 0.5f; sr_tgY = sr_GH * 0.5f; return; }
  int k = (int)(sr_rand() % (uint32_t)sr_count);
  for (int t = 0; t < 6 && sr_state[k] != SR_BUILD; t++) k = (int)(sr_rand() % (uint32_t)sr_count);
  sr_tgX = sr_lcx(k); sr_tgY = sr_lcy(k);
}
static inline float sr_smooth(float u) { u = sr_clampf(u, 0, 1); return u * u * (3 - 2 * u); }
static void sr_camReset(int mode, float zA) {
  sr_camX = sr_GW * 0.5f; sr_camY = sr_GH * 0.5f; sr_camZ = zA;
  sr_camT = 0; sr_legT = 1e9f; sr_reT = 0; sr_offX = sr_offY = 0; sr_seekT = 0;
  sr_tgX = sr_camX; sr_tgY = sr_camY;
  if (mode == CAM_FOLLOW || mode == CAM_TOUR) { sr_pickTarget(); sr_camX = sr_tgX; sr_camY = sr_tgY; sr_legT = 0; }
  sr_l0X = sr_camX; sr_l0Y = sr_camY; sr_l0Z = zA; sr_l1Z = zA; sr_lDX = sr_lDY = 0;
}
static void sr_camUpdate(int mode, float dt, float m, float zA, float zB) {
  float tdt = dt * m;                            // camera time runs at the Camera Motion knob's rate
  sr_camT += tdt;
  float tz = zA, zr = 3.0f;                      // target zoom and how fast we ease to it
  switch (mode) {
    case CAM_FIXED:
      sr_camX += sr_wdf(sr_GW * 0.5f - sr_camX, (float)sr_GW) * (1 - expf(-dt * 2));
      sr_camY += sr_wdf(sr_GH * 0.5f - sr_camY, (float)sr_GH) * (1 - expf(-dt * 2));
      break;
    case CAM_DRIFT: {
      float h = 0.6f + 1.4f * sinf(sr_camT * 0.045f) + 0.5f * sinf(sr_camT * 0.11f);
      sr_camX += cosf(h) * 3.0f * tdt; sr_camY += sinf(h) * 2.2f * tdt;
      tz = zA * (1.0f + 0.22f * sinf(sr_camT * 0.19f));
      break; }
    case CAM_PAN:
      sr_camX += 4.0f * tdt;
      sr_camY = sr_GH * 0.5f + sr_GH * 0.18f * sinf(sr_camT * 0.05f);
      tz = zA * (1.0f + 0.1f * sinf(sr_camT * 0.13f));
      break;
    case CAM_WIDE:
      sr_camX += 1.6f * tdt; sr_camY += 0.9f * tdt;
      tz = zA * (1.0f + 0.25f * (0.5f - 0.5f * cosf(sr_camT * 0.09f)));
      break;
    case CAM_FOLLOW: {
      sr_legT += tdt; sr_reT += dt;
      if (sr_legT > 9.0f || sr_count == 0) { sr_pickTarget(); sr_legT = 0; }
      else if (sr_reT > 0.5f) {                  // stay with whoever is nearest where we were looking (children carry on)
        sr_reT = 0;
        int k = sr_nearest(sr_tgX, sr_tgY);
        if (k >= 0) { sr_tgX = sr_lcx(k); sr_tgY = sr_lcy(k); }
      }
      float a = 1 - expf(-tdt * 1.1f);
      sr_camX += sr_wdf(sr_tgX - sr_camX, (float)sr_GW) * a;
      sr_camY += sr_wdf(sr_tgY - sr_camY, (float)sr_GH) * a;
      tz = zA * (1.0f + 0.15f * sinf(sr_camT * 0.27f));
      break; }
    case CAM_TOUR: {
      const float travel = 3.5f, dwell = 4.5f;
      sr_legT += tdt;
      if (sr_legT > travel + dwell) {            // new leg: from here to another machine, at a new zoom
        sr_legT = 0;
        sr_l0X = sr_camX; sr_l0Y = sr_camY; sr_l0Z = sr_camZ;
        sr_pickTarget();
        sr_lDX = sr_wdf(sr_tgX - sr_l0X, (float)sr_GW); sr_lDY = sr_wdf(sr_tgY - sr_l0Y, (float)sr_GH);
        sr_l1Z = zA * powf(zB / zA, sr_randf());
      }
      if (sr_legT < travel) {
        float u = sr_legT / travel, s = sr_smooth(u);
        sr_camX = sr_l0X + sr_lDX * s; sr_camY = sr_l0Y + sr_lDY * s;
        tz = sr_l0Z * powf(sr_l1Z / sr_l0Z, s) * (1.0f - 0.55f * sinf(3.14159f * u));   // pull out mid-flight
        zr = 8.0f;
      } else {
        tz = sr_l1Z * (1.0f + 0.05f * (sr_legT - travel));       // a slow push in while we watch
        sr_camX += 0.4f * tdt;
      }
      break; }
    case CAM_BREATHE: {
      sr_camX += 0.9f * tdt; sr_camY += 0.35f * tdt;
      float u = 0.5f - 0.5f * cosf(sr_camT * 6.2832f / 18.0f);
      tz = zA * powf(zB / zA, u);
      zr = 8.0f;
      break; }
    case CAM_GROW: {
      float sx = sr_GW * 0.5f, sy = sr_GH * 0.5f, ext = 4;
      for (int k = 0; k < sr_count; k++) {
        float dx = fabsf(sr_wdf(sr_lcx(k) - sx, (float)sr_GW)) + sr_n[k] * 0.5f;
        float dy = fabsf(sr_wdf(sr_lcy(k) - sy, (float)sr_GH)) + sr_n[k] * 0.5f;
        if (dx * 0.75f > ext) ext = dx * 0.75f;   // the screen is 4:3
        if (dy > ext) ext = dy;
      }
      tz = sr_clampf((float)H / (2 * ext + 6), 1.25f, zB);
      zr = tz < sr_camZ ? 1.0f : 0.25f;          // pull back briskly, creep back in slowly
      if (sr_camZ < 1.6f) sr_camX += 0.8f * tdt; // fully out: let the wallpaper drift
      else {
        sr_camX += sr_wdf(sx - sr_camX, (float)sr_GW) * (1 - expf(-dt));
        sr_camY += sr_wdf(sy - sr_camY, (float)sr_GH) * (1 - expf(-dt));
      }
      break; }
  }
  if (m > 0 || mode == CAM_FIXED || mode == CAM_GROW) sr_camZ += (tz - sr_camZ) * (1 - expf(-dt * zr));
  if (!(sr_camZ >= 0.5f && sr_camZ <= 20.0f)) sr_camZ = zA;     // never let a bad number through (NaN fails both tests)
  if (!(sr_camX > -1e4f && sr_camX < 1e4f)) sr_camX = sr_GW * 0.5f;
  if (!(sr_camY > -1e4f && sr_camY < 1e4f)) sr_camY = sr_GH * 0.5f;
  if (sr_wrap) {                                 // keep the numbers small
    while (sr_camX >= sr_GW) { sr_camX -= sr_GW; sr_l0X -= sr_GW; }
    while (sr_camX < 0) { sr_camX += sr_GW; sr_l0X += sr_GW; }
    while (sr_camY >= sr_GH) { sr_camY -= sr_GH; sr_l0Y -= sr_GH; }
    while (sr_camY < 0) { sr_camY += sr_GH; sr_l0Y += sr_GH; }
  }
}
// is any machine in view? if not for a second, glide the view to the nearest one
static void sr_seek(float vx, float vy, float z, float dt) {
  if (sr_count == 0) { sr_seekT = 0; return; }
  float hw = W * 0.5f / z, hh = H * 0.5f / z;
  bool seen = false;
  float inner = sr_seekT >= 0.4f ? 0.25f : 0.75f;                // once gliding, carry on until a machine is near the middle
  for (int k = 0; k < sr_count && !seen; k++) {
    if (fabsf(sr_wdf(sr_lcx(k) - vx, (float)sr_GW)) < hw * inner && fabsf(sr_wdf(sr_lcy(k) - vy, (float)sr_GH)) < hh * inner) seen = true;
  }
  if (seen) { sr_seekT = 0; sr_snap = false; return; }
  if (sr_snap) {
    sr_snap = false;
    int k = sr_nearest(vx, vy);
    sr_offX += sr_wdf(sr_lcx(k) - vx, (float)sr_GW);
    sr_offY += sr_wdf(sr_lcy(k) - vy, (float)sr_GH);
    return;
  }
  sr_seekT += dt;
  if (sr_seekT < 0.4f) return;
  int k = sr_nearest(vx, vy);
  float a = 1 - expf(-dt * 4.0f);
  sr_offX += sr_wdf(sr_lcx(k) - vx, (float)sr_GW) * a;
  sr_offY += sr_wdf(sr_lcy(k) - vy, (float)sr_GH) * a;
}

// ─── Render: the world through the camera onto the screen ─────────────
#define ST_BLOCKS 0
#define ST_DOTS 1
#define ST_OUTLINE 2
#define ST_GLOW 3
#define ST_BLUEPRINT 4
#define SR_FIRST 0x40
#define SR_LAST 0x80
static uint8_t sr_ci[W], sr_cf[W], sr_cd[W];      // per screen column: world cell, first/last-pixel flags, distance² from the cell centre
static uint8_t sr_rb[SR_WMAX + 2];                // one world row, plus [GW] = void and [GW+1] = wall
// where a screen pixel lands in its cell: whole cell index, flags and distance² (0 … 255) from the cell's centre
static inline int sr_axis(int32_t f, int32_t half8, uint8_t* flags, uint8_t* d2) {
  int c = f >> 16;
  int fr = (f >> 8) & 255;
  uint8_t fl = 0;
  if (fr < half8) fl |= SR_FIRST;
  if (fr + half8 >= 256) fl |= SR_LAST;
  int d = fr - 128; if (d < 0) d = -d;
  int q = d >> 3; q *= q; if (q > 255) q = 255;
  *flags = fl; *d2 = (uint8_t)q;
  return c;
}
static void sr_render(float cx, float cy, float z, int style) {
  uint8_t* fb = sr_buf;
  int32_t inv = (int32_t)(65536.0f / z);
  int32_t half8 = inv >> 9;                       // half a pixel, in 1/256ths of a cell
  bool shapes = z >= 3.0f;
  int GW = sr_GW, GH = sr_GH;
  int32_t f0 = (int32_t)floorf((cx - W * 0.5f / z) * 65536.0f) + inv / 2;
  for (int sx = 0; sx < W; sx++) {
    int c = sr_axis(f0 + sx * inv, half8, &sr_cf[sx], &sr_cd[sx]);
    if (sr_wrap) { c %= GW; if (c < 0) c += GW; }
    else if (c < 0 || c >= GW) c = (c == -1 || c == GW) ? GW + 1 : GW;
    sr_ci[sx] = (uint8_t)c;
  }
  int32_t g0 = (int32_t)floorf((cy - H * 0.5f / z) * 65536.0f) + inv / 2;
  int lastRow = -99999;
  for (int sy = 0; sy < H; sy++) {
    uint8_t rf, rd;
    int r = sr_axis(g0 + sy * inv, half8, &rf, &rd);
    if (sr_wrap) { r %= GH; if (r < 0) r += GH; }
    if (r != lastRow) {
      lastRow = r;
      if (r >= 0 && r < GH) { memcpy(sr_rb, sr_world + r * GW, GW); sr_rb[GW] = 0; sr_rb[GW + 1] = SR_WALL; }
      else { uint8_t v = (r == -1 || r == GH) ? SR_WALL : 0; memset(sr_rb, v, GW); sr_rb[GW] = 0; sr_rb[GW + 1] = v; }
    }
    uint8_t* d = fb + sy * W;
    const uint8_t* rb = sr_rb;
    if (!shapes) { for (int sx = 0; sx < W; sx++) d[sx] = rb[sr_ci[sx]]; continue; }
    switch (style) {
      default:
      case ST_BLOCKS:                             // solid squares with a 1-px gap, like a grid
        if (rf & SR_LAST) { memset(d, 0, W); break; }
        for (int sx = 0; sx < W; sx++) d[sx] = (sr_cf[sx] & SR_LAST) ? 0 : rb[sr_ci[sx]];
        break;
      case ST_DOTS:                               // round dots
        for (int sx = 0; sx < W; sx++) d[sx] = (sr_cd[sx] + rd <= 90) ? rb[sr_ci[sx]] : 0;
        break;
      case ST_OUTLINE:                            // hollow squares
        if (rf & (SR_FIRST | SR_LAST)) { for (int sx = 0; sx < W; sx++) d[sx] = rb[sr_ci[sx]]; break; }
        for (int sx = 0; sx < W; sx++) d[sx] = (sr_cf[sx] & (SR_FIRST | SR_LAST)) ? rb[sr_ci[sx]] : 0;
        break;
      case ST_GLOW:                               // soft balls of light, brightest in the middle
        for (int sx = 0; sx < W; sx++) {
          uint8_t v = rb[sr_ci[sx]];
          int lev = (v >> 4) - ((sr_cd[sx] + rd) >> 5);
          d[sx] = lev > 0 ? (uint8_t)((lev << 4) | (v & 15)) : 0;
        }
        break;
      case ST_BLUEPRINT:                          // a faint grid; lit cells drawn bright at the edges, half-filled inside
        for (int sx = 0; sx < W; sx++) {
          uint8_t v = rb[sr_ci[sx]];
          if ((rf | sr_cf[sx]) & SR_LAST) d[sx] = v ? v : SR_GRID;
          else d[sx] = v ? (uint8_t)(((((v >> 4) + 1) >> 1) << 4) | (v & 15)) : 0;
        }
        break;
    }
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float sr_sm[16];
static bool sr_smReady = false;
static inline int sr_k(int i) { int v = (int)sr_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
// a knob that is the preset's own value at its centre, scaled ×(1/4 … 4) either way
static inline float sr_rel(int i, float own, float lo, float hi) {
  float m = powf(4.0f, (sr_k(i) - 512) / 512.0f);
  float v = own * m;
  return v < lo ? lo : (v > hi ? hi : v);
}
// a knob with a centre band for the preset's own choice, and n choices spread either side of it
static inline int sr_choice(int i, int own, int n) {
  int k = sr_k(i);
  if (k >= 440 && k < 600) return own;
  int left = (n + 1) / 2;
  if (k < 440) return k * left / 440;
  return sr_clamp(left + (k - 600) * (n - left) / 424, left, n - 1);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_loops_name() { return "LOOPS"; }
const char* prog_loops_character() { return "Self-replicating machines: loops copy themselves, collide, mutate smaller and evolve — seen through a flying camera"; }

static const char* const sr_presetNames[] = {
  "Evolution", "Colony", "Eighty", "Two Tribes", "Rainbow", "Wreckage",
  "Giants", "Micro", "Signals", "Arena", "Bloom", "Endless"
};
#define SR_NUM_PRESETS 12
//                                                 Evo  Col  80   Tri  Rbw  Wrk  Gnt  Mic  Sig  Arn  Blm  End
static const uint8_t sr_pSeed[SR_NUM_PRESETS]  = {SM_SCATTER, SM_SINGLE, SM_GRID, SM_TRIBES, SM_SCATTER, SM_SCATTER, SM_SCATTER, SM_SCATTER, SM_SCATTER, SM_SCATTER, SM_SINGLE, SM_SCATTER};
static const uint8_t sr_pCount[SR_NUM_PRESETS] = { 14,   1,  80,  14,  18,   9,   5,  30,   6,  14,   1,   8};
static const uint8_t sr_pSize[SR_NUM_PRESETS]  = { 10,   8,   6,   9,  10,   9,  15,   5,   8,   8,  10,  10};
static const uint8_t sr_pMut[SR_NUM_PRESETS]   = { 60,   0,  70,  40, 100,  50,  50,  50,  40,  60,  25,  60};   // %
static const uint8_t sr_pFade[SR_NUM_PRESETS]  = { 30,  20,  30,  30,  30,   6,  25,  40,  30,  30,  20,  35};   // ×0.1 levels/s
static const uint8_t sr_pGap[SR_NUM_PRESETS]   = {  2,   2,   2,   2,   2,   2,   3,   1,   2,   2,   2,   2};   // arm length
static const uint8_t sr_pKill[SR_NUM_PRESETS]  = { 25,  10,  30,  40,  25,  20,  30,  25,  20,  35,  15,  25};   // %
static const uint8_t sr_pGrow[SR_NUM_PRESETS]  = { 10,   0,  10,  10,  25,  10,   5,  10,  10,  10,  15,  10};   // %
static const uint8_t sr_pWW[SR_NUM_PRESETS]    = {128, 128, 128, 128, 128, 128, 128, 128,  64,  80, 128, 128};   // world size
static const uint8_t sr_pWH[SR_NUM_PRESETS]    = { 96,  96,  96,  96,  96,  96,  96,  96,  48,  60,  96,  96};
static const uint8_t sr_pWrap[SR_NUM_PRESETS]  = {  1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,   1};
static const uint8_t sr_pColor[SR_NUM_PRESETS] = {  0,   0,   0,   1,   1,   0,   0,   0,   2,   0,   0,   0};
static const uint8_t sr_pPal[SR_NUM_PRESETS]   = {  4,   4,   4,   0,   1,   5,   6,   2,   7,   4,   3,   4};
static const uint8_t sr_pInject[SR_NUM_PRESETS]= {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  20};   // s
static const uint8_t sr_pCam[SR_NUM_PRESETS]   = {CAM_DRIFT, CAM_GROW, CAM_BREATHE, CAM_PAN, CAM_FOLLOW, CAM_WIDE, CAM_FOLLOW, CAM_TOUR, CAM_FOLLOW, CAM_FIXED, CAM_GROW, CAM_TOUR};
static const uint8_t sr_pZA[SR_NUM_PRESETS]    = { 40, 100,  25,  35,  55,  13,  60,  30,  80,  40, 100,  25};   // zoom ×10 (px per cell)
static const uint8_t sr_pZB[SR_NUM_PRESETS]    = { 40, 100,  80,  35,  55,  13,  60, 100,  80,  40,  90,  80};   // the other end, for tour / breathe / pull-back
static const uint8_t sr_pStyle[SR_NUM_PRESETS] = {ST_BLOCKS, ST_GLOW, ST_BLOCKS, ST_OUTLINE, ST_DOTS, ST_BLOCKS, ST_GLOW, ST_DOTS, ST_GLOW, ST_BLUEPRINT, ST_BLOCKS, ST_BLOCKS};

const char* prog_loops_presetName(int preset) {
  if (preset >= 0 && preset < SR_NUM_PRESETS) return sr_presetNames[preset];
  return NULL;
}
static const char* const sr_labels[16] = {
  "Palette", "Speed", "Count", "Loop Size", "Mutation", "Wreckage", "Zoom", "Camera Motion",
  "Pan X", "Pan Y", "Style", "Glow", "Colour By", "Lifespan", "Inject Rate", "Epoch"
};
const char* prog_loops_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return sr_labels[pot];
  return "";
}
uint8_t prog_loops_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we draw every pixel ourselves each frame
}

static int sr_lastPreset = -1;
void prog_loops_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    sr_levK[0] = 0;
    for (int l = 1; l < 16; l++) sr_levK[l] = (int16_t)(256.0f * (0.10f + 0.90f * powf(l / 15.0f, 1.1f)));
  }
  sr_lastPreset = -1;                                            // reseed on the next frame
}

void prog_loops_draw(int preset) {
  sr_buf = display.getBuffer();
  if (!sr_buf) return;
  if (preset < 0 || preset >= SR_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!sr_smReady) sr_sm[i] = pots[i];
      else sr_sm[i] += (pots[i] - sr_sm[i]) * a;
    }
    sr_smReady = true;
  }

  // ── knobs ──
  float sk = sr_sm[1] / 1023.0f;
  float tickRate = (sk < 0.03f) ? 0.0f : 6.0f * powf(30.0f, (sk - 0.03f) / 0.97f);   // steps per second, centre ≈ 33
  int pal = (sr_pPal[preset] + sr_k(0) * 8 / 1024 - 4 + 8) % 8;
  int count = sr_clamp((int)(sr_rel(2, sr_pCount[preset], 1, SR_MAX / 2) + 0.5f), 1, SR_MAX / 2);
  int n0 = sr_clamp((int)(sr_pSize[preset] + (sr_k(3) - 512) * 6 / 512), 4, 19);
  sr_mut = sr_rel(4, sr_pMut[preset] / 100.0f + 0.001f, 0.0f, 1.0f);
  if (sr_pMut[preset] == 0) sr_mut = sr_k(4) > 600 ? (sr_k(4) - 600) / 423.0f : 0.0f;   // Colony: pure unless you turn it up
  float fadeRate = sr_rel(5, sr_pFade[preset] / 10.0f, 0.3f, 12.0f);   // wreckage brightness levels lost per second
  float zoomMul = powf(2.2f, (sr_k(6) - 512) / 512.0f);          // ×0.45 … ×2.2
  float camMul = sr_k(7) < 50 ? 0.0f : powf(4.0f, (sr_k(7) - 512) / 512.0f);   // still … ×1 … ×4
  int style = sr_choice(10, sr_pStyle[preset], 5);
  sr_glow = 10 + sr_k(11) * 5 / 1024;
  sr_sig = sr_clamp(sr_glow + 2, 0, 15);
  sr_colorBy = sr_choice(12, sr_pColor[preset], 3);
  sr_life = 120 + sr_k(13) * 900 / 1023;
  sr_gap = sr_pGap[preset];
  sr_kill = sr_pKill[preset] / 100.0f;
  sr_grow = sr_pGrow[preset] / 100.0f;
  float injectEvery = sr_pInject[preset];
  int ik = sr_k(14);
  if (ik < 300) injectEvery = 0;                                 // left: never
  else if (ik > 724) injectEvery = 45.0f - (ik - 724) * 40.0f / 299.0f;   // right: every 45 … 5 s (centre = the preset's own)
  int epk = sr_k(15);
  float epochT = epk < 60 ? 0.0f : (epk < 512 ? 60.0f - (epk - 60) * 40.0f / 452.0f : 20.0f - (epk - 512) * 15.0f / 511.0f);   // 60 … 20 (centre) … 5 s
  sr_buildPalette(pal);
  int camMode = sr_pCam[preset];
  float zA = sr_pZA[preset] * 0.1f, zB = sr_pZB[preset] * 0.1f;

  // ── (re)start the experiment ──
  static float extinctT = 0, emptyT = 0;
  bool restart = preset != sr_lastPreset;
  if (epochT > 0 && extinctT > epochT) restart = true;            // the originals are long gone: a new experiment
  if (restart) {
    sr_GW = sr_pWW[preset]; sr_GH = sr_pWH[preset]; sr_wrap = sr_pWrap[preset];
  }
  if (n0 > sr_GH / 5) n0 = sr_GH / 5;                            // big loops must still fit several to a world
  if (restart) {
    sr_n0 = n0;
    memset(sr_world, 0, sizeof(sr_world));
    sr_count = 0;
    sr_rng = 2024u + (uint32_t)preset * 977u + (uint32_t)now;
    sr_seed(sr_pSeed[preset], count, n0, 1);
    if (sr_count == 0) sr_seed(SM_SINGLE, 1, n0, 1);
    bool firstTime = preset != sr_lastPreset;
    sr_lastPreset = preset;
    emptyT = 0; extinctT = 0; sr_snap = true;
    if (firstTime || camMode == CAM_GROW) sr_camReset(camMode, camMode == CAM_GROW ? zB : zA);
  }
  if (sr_count == 0) {                                             // everyone died: start again after a breath
    emptyT += dt;
    if (emptyT > 0.25f) {
      sr_seed(sr_pSeed[preset] == SM_GRID ? SM_SCATTER : sr_pSeed[preset], count, n0, (int)(sr_rand() & 7));
      if (sr_count == 0) sr_seed(SM_SCATTER, 1, 4, 1);
      emptyT = 0; sr_snap = true;
    }
  }

  // ── INJECT: k12, or every so often ──
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  static float injT = 0;
  bool inject = k12 && !k12Was;
  k12Was = k12;
  if (injectEvery > 0 && tickRate > 0) { injT += dt; if (injT > injectEvery) { injT = 0; inject = true; } }
  if (inject) sr_seed(SM_SCATTER, 4, sr_clamp(n0 + 2, 4, sr_GH / 5), (int)(sr_rand() & 7));

  // ── wreckage fades (and live loops are redrawn over it below) ──
  {
    static float fadeAcc = 0;
    fadeAcc += dt * fadeRate;
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) sr_fade(dec);
  }
  for (int k = 0; k < sr_count; k++) sr_draw(k);

  // ── simulate ──
  static float tickAcc = 0;
  tickAcc += dt * tickRate;
  int steps = (int)tickAcc;
  tickAcc -= steps;
  if (steps > 6) steps = 6;
  for (int s = 0; s < steps; s++) {
    sr_tick++;
    int start = sr_count > 0 ? (int)(sr_rand() % (uint32_t)sr_count) : 0;   // shuffled order: no one always goes first
    int nAtStart = sr_count;
    for (int v = 0, k = start; v < nAtStart && sr_count > 0; v++) {
      if (k >= sr_count) k = 0;
      if (sr_step(k)) k++;
    }
  }
  if (steps > 0) for (int k = 0; k < sr_count; k++) sr_draw(k);
  {                                                               // are any of the original machines left?
    int orig = 0;
    for (int k = 0; k < sr_count; k++) if (sr_orig[k]) orig++;
    if (orig == 0 && tickRate > 0) extinctT += dt; else extinctT = 0;
  }

  // ── camera ──
  sr_camUpdate(camMode, dt, camMul, zA, zB);
  float z = sr_clampf(sr_camZ * zoomMul, 1.0f, 12.0f);
  float vx = sr_camX + (sr_k(8) - 512) * (sr_GW * 0.5f / 512.0f) + sr_offX;
  float vy = sr_camY + (sr_k(9) - 512) * (sr_GH * 0.5f / 512.0f) + sr_offY;
  sr_seek(vx, vy, z, dt);
  if (sr_wrap) {
    vx = fmodf(vx, (float)sr_GW); if (vx < 0) vx += sr_GW;
    vy = fmodf(vy, (float)sr_GH); if (vy < 0) vy += sr_GH;
    sr_offX = fmodf(sr_offX, (float)sr_GW); sr_offY = fmodf(sr_offY, (float)sr_GH);
  }
  sr_render(vx, vy, z, style);
}

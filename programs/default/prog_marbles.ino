// =====================================================================
// PROGRAM: MARBLES — chrome balls on a clay marble run (by Dewey) — Set 5, slot 9
//
// The clay field from BLOCKS turned into a marble run, as in ducky3d's
// "Cycles": chrome balls roll along the tops of the shapes, bounce back off
// taller ones, drop off edges with a bounce, and sink into the tubes — to
// shoot out of another tube somewhere else. Balls that roll out of the
// play area fall back in from the sky.
//
// Presets:
//   k0 Marble Run — red and white terraces
//   k1 Staircase  — steps going down; balls hop from step to step
//   k2 Pinball    — a low flat field full of tubes, fast
//   k3 Pyramid    — balls roll off a stepped pyramid and drop back on top
//   k4 Rolling Sea — the field heaves in waves and the balls surf it
//   k5 Maze       — tall walls, balls bouncing along the corridors
//   k6 Gold Rush  — gold balls on navy and cream
//   k7 Bubblegum  — pink and white, a few big slow balls
//   k8 Night      — dark blue blocks, cyan light
//   k9 Avalanche  — many balls on a steep staircase
//   k10 Lone Ball — one big ball wandering
//   k11 Tour      — a new preset every 20 seconds
//
//   k12 — SHOWER: a handful of balls dropped from the sky
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Balls   p3 Ball Size   p4 Gravity
//   p5 Bounce   p6 Terrain (centre = preset: terraces, stairs, flat, pyramid, sea, maze)   p7 Terrain Height
//   p8 Holes   p9 Colour Mix   p10 Cell Size   p11 Speckles
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5c.h"

enum { MB5_TERRACE, MB5_STAIRS, MB5_FLAT, MB5_PYRAMID, MB5_SEA, MB5_MAZE, MB5_NTERR };
struct Mb5Preset { C5Look look; uint8_t terrain, balls, size, height, holes, mixB, cell, speed, bounce, tint, speck; };
static const Mb5Preset mb5_pre[11] = {
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MB5_TERRACE, 8, 100, 50, 80, 110, 26, 100, 110, 0, 40},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MB5_STAIRS, 9, 100, 50, 30, 110, 24, 100, 110, 0, 40},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MB5_FLAT, 12, 90, 20, 160, 110, 24, 170, 140, 0, 40},
  {{{150,140,120},{255,250,235},{120,60,10},{240,140,40},{230,120,30},{24,16,8}}, MB5_PYRAMID, 9, 100, 60, 40, 110, 24, 100, 110, 0, 30},
  {{{120,150,170},{235,248,255},{10,60,110},{50,150,230},{30,110,200},{8,14,24}}, MB5_SEA, 8, 100, 40, 40, 110, 26, 100, 90, 0, 30},
  {{{150,150,160},{250,248,246},{40,40,46},{120,120,130},{230,40,50},{14,14,16}}, MB5_MAZE, 10, 100, 36, 20, 60, 24, 120, 130, 0, 40},
  {{{170,160,130},{255,250,235},{10,20,60},{40,70,150},{40,70,150},{6,8,20}}, MB5_TERRACE, 8, 100, 50, 80, 110, 26, 100, 110, 1, 30},
  {{{170,150,160},{255,248,252},{140,40,90},{255,140,190},{255,90,150},{30,10,20}}, MB5_TERRACE, 4, 150, 40, 60, 110, 28, 70, 120, 0, 30},
  {{{10,20,50},{40,60,120},{0,60,80},{40,220,240},{120,240,255},{2,4,10}}, MB5_TERRACE, 8, 100, 50, 80, 110, 26, 100, 110, 0, 10},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MB5_STAIRS, 16, 80, 70, 30, 110, 22, 140, 110, 2, 40},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MB5_TERRACE, 1, 180, 40, 0, 110, 28, 80, 110, 0, 40},
};
static const char* const mb5_names[12] = {
  "Marble Run", "Staircase", "Pinball", "Pyramid", "Rolling Sea", "Maze", "Gold Rush", "Bubblegum", "Night", "Avalanche", "Lone Ball", "Tour"
};
static const char* const mb5_labels[16] = {
  "Palette", "Speed", "Balls", "Ball Size", "Gravity", "Bounce", "Terrain", "Terrain Height",
  "Holes", "Colour Mix", "Cell Size", "Speckles", "Light Side", "Wave", "Pop", "Chrome Tint"
};
const char* prog_marbles_name() { return "MARBLES"; }
const char* prog_marbles_character() { return "Chrome balls rolling over a clay marble run, bouncing, dropping into tubes and popping out again"; }
const char* prog_marbles_presetName(int preset) { return (preset >= 0 && preset < 12) ? mb5_names[preset] : NULL; }
const char* prog_marbles_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? mb5_labels[pot] : ""; }
uint8_t prog_marbles_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define MB5_MAX 16
#define MB5_AREA 5                                     // the play area: |i|, |j| ≤ 5 cells around the middle
struct Mb5Ball { int32_t fi, fj, z; int32_t vi, vj, vz; uint8_t alive, sunk; uint16_t wait; };   // fi, fj: 1/256 cell; z: 1/16 px
static Mb5Ball mb5_b[MB5_MAX];
static int mb5_preset = -1, mb5_look = 0, mb5_terr = 0, mb5_height = 50, mb5_holes = 60, mb5_mixB = 110, mb5_waveT = 0, mb5_r = 10;
static float mb5_tour = 0, mb5_wave = 0, mb5_t = 0;
static bool mb5_k12 = false;

static int mb5_cellH(int i, int j) {
  uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u) ^ 0x5Au);
  int H0 = mb5_height, z = 0;
  switch (mb5_terr) {
    case MB5_TERRACE: z = 10 + (int)((h >> 8) & 3) * H0 / 5; break;
    case MB5_STAIRS: z = 10 + (MB5_AREA * 2 - (i + j)) * H0 / 10; if (z < 6) z = 6; break;
    case MB5_FLAT: z = 12 + (int)((h >> 8) & 1) * 4; break;
    case MB5_PYRAMID: { int a = i < 0 ? -i : i, b = j < 0 ? -j : j, m = a > b ? a : b; z = 10 + (MB5_AREA + 1 - m) * H0 / 5; if (z < 6) z = 6; break; }
    case MB5_SEA: z = 20 + ((v3_sin(i * 160 + mb5_waveT) + v3_sin(j * 130 - mb5_waveT * 3 / 4)) * H0 >> 15); break;
    case MB5_MAZE: z = (((h >> 8) & 7) < 2 || (i % 3 == 0 && j % 4 != 1)) ? 14 + H0 : 14; break;
  }
  if (mb5_wave > 0.01f && mb5_terr != MB5_SEA) z += (int)((v3_sin((i + j) * 140 - mb5_waveT) * mb5_wave * 16) / 16384);
  return z < 4 ? 4 : z;
}
static bool mb5_isTube(int i, int j) { uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u) ^ 0x5Au); return (int)((h >> 16) & 255) < mb5_holes && mb5_terr != MB5_MAZE; }
static void mb5_cell(int i, int j, void* cv) {
  C5Cell* c = (C5Cell*)cv;
  uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u) ^ 0x5Au);
  c->seed = h; c->h = mb5_cellH(i, j);
  c->L = (h & 1) ? 0 : 7;
  c->hole = mb5_isTube(i, j) ? 150 : 0;
  c->clay = (int)((h >> 24) & 255) < mb5_mixB ? 1 : 0;
  c->scale = 230;
}
static void mb5_spawn(int idx, bool fromSky) {
  Mb5Ball& b = mb5_b[idx];
  b.fi = v3_rndr(-MB5_AREA + 1, MB5_AREA - 1) * 256; b.fj = v3_rndr(-MB5_AREA + 1, MB5_AREA - 1) * 256;
  if (mb5_terr == MB5_STAIRS) { b.fi = v3_rndr(-MB5_AREA, -MB5_AREA + 2) * 256; b.fj = v3_rndr(-MB5_AREA, MB5_AREA) * 256; }
  if (mb5_terr == MB5_PYRAMID) { b.fi = v3_rndr(-1, 1) * 256; b.fj = v3_rndr(-1, 1) * 256; }
  int d = v3_rnd(4);
  b.vi = d == 0 ? 1 : (d == 1 ? -1 : 0); b.vj = d == 2 ? 1 : (d == 3 ? -1 : 0);
  if (mb5_terr == MB5_STAIRS) { if (v3_rnd(2)) { b.vi = 1; b.vj = 0; } else { b.vi = 0; b.vj = 1; } }
  b.z = (mb5_cellH(b.fi >> 8, b.fj >> 8) + (fromSky ? 70 + v3_rnd(60) : 0)) * 16; b.vz = 0;
  b.alive = 1; b.sunk = 0; b.wait = 0;
}
static int mb5_nb = 8;
static void mb5_between(int d) {
  for (int k = 0; k < mb5_nb; k++) {
    Mb5Ball& b = mb5_b[k];
    if (!b.alive || b.sunk) continue;
    int ci = (b.fi + 128) >> 8, cj = (b.fj + 128) >> 8;
    if (ci + cj != d) continue;
    int g = mb5_cellH(ci, cj);
    int sx, sy; c5_toScreen(b.fi, b.fj, g, &sx, &sy);
    if (b.z / 16 - g < 120) c5_ballShadow(sx + 2, sy + 1, mb5_r * (140 - (b.z / 16 - g)) / 140 + 1);
    c5_toScreen(b.fi, b.fj, b.z / 16, &sx, &sy);
    c5_ball(sx, sy - mb5_r + 1, mb5_r);
  }
}
void prog_marbles_init() { v3_reset(); mb5_preset = -1; c5_S = 0; }

static void marbles5_draw(int preset) {
  v3_buf = c5_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != mb5_preset) { mb5_preset = preset; mb5_look = preset == 11 ? 0 : preset; mb5_tour = 0; for (int k = 0; k < MB5_MAX; k++) mb5_b[k].alive = 0; }
  if (preset == 11) { mb5_tour += v3_dt; if (mb5_tour > 20.0f) { mb5_tour = 0; mb5_look = (mb5_look + 1) % 11; } }
  const Mb5Preset& P = mb5_pre[mb5_look];
  const Mb5Preset& Q = mb5_pre[v3_centre(0) ? mb5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  mb5_nb = v3_centre(2) ? P.balls : 1 + v3_clamp(v3_k(2) * MB5_MAX / 1024, 0, MB5_MAX - 1);
  int S = v3_centre(10) ? P.cell : 14 + (int)(v3_u(10) * 22); S &= ~1;
  c5_profiles(S);
  mb5_r = (int)(v3_rel(3, P.size / 100.0f, 1.8f) * c5_S * 0.6f); if (mb5_r < 3) mb5_r = 3;
  int grav = (int)v3_rel(4, 9, 2.5f);
  int bounce = v3_centre(5) ? P.bounce : (int)(v3_u(5) * 230);
  mb5_terr = v3_centre(6) ? P.terrain : v3_clamp(v3_k(6) * MB5_NTERR / 1024, 0, MB5_NTERR - 1);
  mb5_height = (int)v3_rel(7, P.height, 2.0f);
  mb5_holes = v3_centre(8) ? P.holes : (int)(v3_u(8) * 255);
  mb5_mixB = v3_centre(9) ? P.mixB : (int)(v3_u(9) * 255);
  c5_speckles = v3_centre(11) ? Q.speck : (int)(v3_u(11) * 200);
  c5_light = v3_centre(12) ? 1 : (v3_k(12) < 512 ? -1 : 1);
  mb5_wave = v3_centre(13) ? 0.0f : v3_u(13) * 40;
  int pop = (int)v3_rel(14, 70, 2.0f);
  int tint = v3_centre(15) ? Q.tint : v3_clamp(v3_k(15) * 3 / 1024, 0, 2);
  c5_palette(Q.look);
  if (tint) {                                                       // gold or copper balls
    static const uint8_t tc[3][3] = {{0,0,0},{255,200,80},{230,120,70}};
    for (int i = 0; i < 64; i++) { int c = i * 4; display.setColor((uint8_t)(128 + i), v3_clamp(c * tc[tint][0] / 220, 0, 255), v3_clamp(c * tc[tint][1] / 220, 0, 255), v3_clamp(c * tc[tint][2] / 220, 0, 255)); }
  }

  float dt = v3_dt * speed;
  mb5_t += dt;
  if (mb5_terr == MB5_SEA || mb5_wave > 0.01f) mb5_waveT = (int)(mb5_t * 260);
  bool k12 = keysPressed[KEY_MOD_A], shower = k12 && !mb5_k12; mb5_k12 = k12;
  if (shower) { int n = 0; for (int k = 0; k < MB5_MAX && n < 5; k++) if (!mb5_b[k].alive || k >= mb5_nb) { mb5_spawn(k, true); n++; if (k >= mb5_nb) mb5_nb = k + 1; } }

  // ── the balls ──
  int steps = speed > 0 ? 2 : 0;
  int rollSp = (int)(P.speed * speed / 2);                         // 1/256 cell per half-step
  for (int k = 0; k < MB5_MAX; k++) {
    Mb5Ball& b = mb5_b[k];
    if (k >= mb5_nb) { if (!shower) b.alive = 0; continue; }
    if (!b.alive) { mb5_spawn(k, true); continue; }
    for (int st = 0; st < steps; st++) {
      if (b.sunk) {                                                // under a tube: wait, then shoot out of another one
        if (--b.wait == 0) {
          for (int tries = 0; tries < 30; tries++) {
            int i = v3_rndr(-MB5_AREA + 1, MB5_AREA - 1), j = v3_rndr(-MB5_AREA + 1, MB5_AREA - 1);
            if (mb5_isTube(i, j)) { b.fi = i * 256; b.fj = j * 256; b.z = (mb5_cellH(i, j) - 10) * 16; b.vz = pop * 8 / 5; b.sunk = 0; break; }
          }
          if (b.sunk) mb5_spawn(k, true);
        }
        continue;
      }
      int ci = (b.fi + 128) >> 8, cj = (b.fj + 128) >> 8;
      int g = mb5_cellH(ci, cj) * 16;
      // roll: look one step ahead — a taller block bounces it back
      int ni = b.fi + b.vi * rollSp, nj = b.fj + b.vj * rollSp;
      int nci = (ni + 128 + b.vi * 70) >> 8, ncj = (nj + 128 + b.vj * 70) >> 8;
      int ng = mb5_cellH(nci, ncj) * 16;
      if ((nci != ci || ncj != cj) && ng > b.z + 64) {
        b.vi = -b.vi; b.vj = -b.vj;
        if (mb5_terr == MB5_MAZE && v3_rnd(2)) { int t = b.vi; b.vi = b.vj; b.vj = t; }   // in the maze, sometimes turn instead
      } else { b.fi = ni; b.fj = nj; }
      // fall and bounce
      b.vz -= grav; b.z += b.vz;
      if (b.z < g) {
        // over the middle of a tube: in it goes
        int oi = b.fi - ci * 256, oj = b.fj - cj * 256;
        if (mb5_isTube(ci, cj) && v3_abs(oi) < 40 && v3_abs(oj) < 40 && (b.vz < -40 || v3_rnd(6) == 0)) { b.sunk = 1; b.wait = (uint16_t)(40 + v3_rnd(80)); continue; }
        b.z = g; b.vz = b.vz < -24 ? -b.vz * bounce / 256 : 0;
      }
      // keep them rolling along the middle of their lane
      if (b.vi) b.fj += (cj * 256 - b.fj) / 8; else b.fi += (ci * 256 - b.fi) / 8;
      // out of the play area (or down off the staircase): back in from the sky
      if (v3_abs(b.fi) > (MB5_AREA + 1) * 256 || v3_abs(b.fj) > (MB5_AREA + 1) * 256) mb5_spawn(k, true);
      if (mb5_terr == MB5_PYRAMID && b.z < 14 * 16 && b.vz == 0 && v3_rnd(60) == 0) mb5_spawn(k, true);
      else if (b.vz == 0 && v3_rnd(400) == 0 && mb5_terr != MB5_STAIRS) { int d = v3_rnd(4); b.vi = d == 0 ? 1 : (d == 1 ? -1 : 0); b.vj = d == 2 ? 1 : (d == 3 ? -1 : 0); }
    }
  }

  // ── draw ──
  c5_cx = 160; c5_top = 120 - 10;
  memset(v3_buf, 196, W * H);
  c5_field(mb5_height + 60, mb5_cell, mb5_between);
}
void prog_marbles_draw(int preset) { marbles5_draw(preset); fx5_apply(); }

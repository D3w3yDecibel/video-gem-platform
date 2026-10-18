// =====================================================================
// PROGRAM: PLOTTER — a pen plotter at work (by Dewey) — Set 5, slot 6
//
// The making of a di_diff_world piece: a pen on a gantry plots an isometric
// scene of blocks, towers and pipes one stroke at a time — outlines first,
// then the mesh on every face, denser on the faces in shadow. When the
// plot is finished an airbrush sweeps across and sprays the shading in,
// the finished sheet is held for a moment, then the paper feeds out and a
// new drawing begins.
//
// Nothing is stored but the scene's layout: every frame the plot is
// replayed from the start up to the stroke the pen has reached.
//
// Presets:
//   k0 Pen & Airbrush — black ink, grey airbrush, white paper
//   k1 Blueprint      — white on blue
//   k2 Sepia          — brown ink, warm spray
//   k3 Red Ink        — red pen, grey spray
//   k4 Gold on Black  — gold lines on black card
//   k5 Green Plotter  — an old green-screen plotter
//   k6 Blue Spray     — black ink with a blue airbrush
//   k7 Towers         — tall towers, deep shadows
//   k8 Pipe Rack      — mostly pipes
//   k9 Ziggurat       — stepped terraces
//   k10 Monoliths     — a few huge blocks
//   k11 Tour          — a new preset for every drawing
//
//   k12 — NEXT DRAWING
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Plot Speed (far left = freeze)   p2 Density   p3 Height
//   p4 Mesh Size   p5 Pipes   p6 Scene (centre = preset: mixed, towers, pipe rack, ziggurat, monoliths)
//   p7 Airbrush   p8 Light Side   p9 Hold   p10 Gantry (show the pen's arm)   p11 Cast Shadows
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5p.h"

enum { PL5_MIXED, PL5_TOWERS, PL5_RACK, PL5_ZIGGURAT, PL5_MONO, PL5_NSCENE };
struct Pl5Preset { P5Look look; uint8_t scene, density, height, mesh, pipes, air, sprayB; int8_t ink; uint8_t paper; };
static const Pl5Preset pl5_pre[11] = {
  {{{10,10,12},{110,110,112},{246,245,240},{10,10,12},{110,110,112},{246,245,240}}, PL5_MIXED, 60, 60, 2, 60, 60, 0, 90, 127},
  {{{240,245,255},{90,130,200},{20,50,120},{240,245,255},{90,130,200},{20,50,120}}, PL5_MIXED, 60, 60, 2, 60, 50, 0, 90, 127},
  {{{40,20,8},{150,105,60},{242,226,190},{40,20,8},{150,105,60},{242,226,190}}, PL5_MIXED, 60, 60, 2, 60, 60, 0, 90, 127},
  {{{120,10,10},{180,140,140},{248,244,238},{60,60,64},{150,150,152},{248,244,238}}, PL5_MIXED, 60, 60, 2, 60, 55, 128, 80, 127},
  {{{255,220,120},{120,90,30},{8,6,4},{255,220,120},{120,90,30},{8,6,4}}, PL5_MIXED, 60, 60, 2, 60, 50, 0, 90, 127},
  {{{120,255,140},{20,90,40},{0,12,4},{120,255,140},{20,90,40},{0,12,4}}, PL5_MIXED, 60, 60, 2, 60, 40, 0, 90, 127},
  {{{10,10,14},{110,110,115},{246,245,240},{0,20,70},{60,110,200},{246,245,240}}, PL5_MIXED, 60, 60, 2, 60, 60, 255, 90, 127},
  {{{10,10,12},{110,110,112},{246,245,240},{10,10,12},{110,110,112},{246,245,240}}, PL5_TOWERS, 60, 110, 3, 30, 70, 0, 90, 127},
  {{{10,12,16},{110,115,125},{240,242,245},{10,12,16},{110,115,125},{240,242,245}}, PL5_RACK, 60, 50, 3, 220, 55, 0, 90, 127},
  {{{20,14,8},{130,110,80},{240,232,215},{20,14,8},{130,110,80},{240,232,215}}, PL5_ZIGGURAT, 60, 60, 3, 20, 60, 0, 90, 127},
  {{{10,10,12},{110,110,112},{246,245,240},{10,10,12},{110,110,112},{246,245,240}}, PL5_MONO, 60, 80, 4, 20, 70, 0, 90, 127},
};
static const char* const pl5_names[12] = {
  "Pen & Airbrush", "Blueprint", "Sepia", "Red Ink", "Gold on Black", "Green Plotter", "Blue Spray", "Towers", "Pipe Rack", "Ziggurat", "Monoliths", "Tour"
};
static const char* const pl5_labels[16] = {
  "Palette", "Plot Speed", "Density", "Height", "Mesh Size", "Pipes", "Scene", "Airbrush",
  "Light Side", "Hold", "Gantry", "Cast Shadows", "Line Darkness", "Scale", "Spray Colour", "Mirror"
};
const char* prog_plotter_name() { return "PLOTTER"; }
const char* prog_plotter_character() { return "A pen plotter drawing an isometric scene stroke by stroke, then airbrushing the shadows in"; }
const char* prog_plotter_presetName(int preset) { return (preset >= 0 && preset < 12) ? pl5_names[preset] : NULL; }
const char* prog_plotter_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? pl5_labels[pot] : ""; }
uint8_t prog_plotter_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// a box in the scene: grid cells x0 … x1, y0 … y1 (in quarter cells), heights z0 … z1 (px)
struct Pl5Obj { int8_t x0, y0, x1, y1; int16_t z0, z1; int16_t first; uint8_t pipe, pad; };
#define PL5_MAX 90
static Pl5Obj* pl5_o;
static int pl5_n = 0, pl5_total = 0;
static int pl5_preset = -1, pl5_look = 0, pl5_phase = 0, pl5_scene = 0, pl5_meshQ = 3, pl5_cw = 14, pl5_side = 1, pl5_mirror = 0;
static float pl5_drawn = 0, pl5_t = 0, pl5_spray = 0;
static uint32_t pl5_seed = 1;
static int32_t pl5_ox = 160 * 16, pl5_oy = 60 * 16;      // where the scene's origin lands on screen (1/16 px), fitted to each scene
static bool pl5_k12 = false, pl5_new = true;
enum { PL5_PLOT, PL5_SPRAY, PL5_HOLD, PL5_FEED };

// iso projection: x, y in quarter cells, z in px → screen (1/16 px)
static inline void pl5_iso(int x, int y, int z, int32_t* sx, int32_t* sy) {
  int gx = pl5_mirror ? y : x, gy = pl5_mirror ? x : y;
  *sx = pl5_ox + (gx - gy) * pl5_cw * 4;                      // a quarter cell = cw/4 px across …
  *sy = pl5_oy + (gx + gy) * pl5_cw * 2 - z * 16;             // … and cw/8 px down
}
static void pl5_add(int x0, int y0, int x1, int y1, int z0, int z1, int pipe) {
  if (pl5_n >= PL5_MAX || x1 <= x0 || y1 <= y0 || z1 <= z0) return;
  Pl5Obj& o = pl5_o[pl5_n++];
  o.x0 = (int8_t)x0; o.y0 = (int8_t)y0; o.x1 = (int8_t)x1; o.y1 = (int8_t)y1; o.z0 = (int16_t)z0; o.z1 = (int16_t)z1; o.pipe = (uint8_t)pipe;
}
static void pl5_build(int density, int height, int pipes, int scene) {
  pl5_o = (Pl5Obj*)v3_scratch; pl5_n = 0;
  uint32_t save = v3_seed; v3_seed = pl5_seed;
  int G = 32;                                                      // the ground: 8 × 8 cells = 32 × 32 quarter cells
  if (scene == PL5_ZIGGURAT) {
    int steps = 4 + v3_rnd(3);
    for (int k = 0; k < steps; k++) { int m = k * 3; pl5_add(2 + m, 2 + m, G - 2 - m, G - 2 - m, k * 14 * height / 60, (k + 1) * 14 * height / 60, 0); }
    for (int k = 0; k < density / 12; k++) { int x = v3_rndr(0, G - 4), y = v3_rndr(0, G - 4); if (x > 4 && x < G - 6 && y > 4 && y < G - 6) continue; pl5_add(x, y, x + 3, y + 3, 0, v3_rndr(10, 40), 0); }
  } else {
    int nObj = scene == PL5_MONO ? 3 + v3_rnd(3) : density / 2 + 6;
    for (int k = 0; k < nObj; k++) {
      int w, d, h;
      if (scene == PL5_MONO) { w = v3_rndr(5, 9); d = v3_rndr(5, 9); h = v3_rndr(60, 140); }
      else if (scene == PL5_TOWERS) { w = v3_rndr(2, 5); d = v3_rndr(2, 5); h = v3_rndr(40, 150); }
      else { w = v3_rndr(2, 7); d = v3_rndr(2, 7); h = v3_rndr(8, 90); }
      h = h * height / 60;
      int x = v3_rndr(0, G - w), y = v3_rndr(0, G - d);
      if (scene == PL5_RACK && v3_rnd(3)) continue;
      pl5_add(x, y, x + w, y + d, 0, h, 0);
    }
    int np = (scene == PL5_RACK ? 26 : pipes / 10) ;
    for (int k = 0; k < np; k++) {                                // pipes: long thin boxes at a height
      int len = v3_rndr(8, G), z = v3_rndr(6, 70) * height / 60 + 4, th = scene == PL5_RACK ? 2 : v3_rndr(1, 2);
      if (v3_rnd(2)) { int x = v3_rndr(0, G - len), y = v3_rndr(0, G - th); pl5_add(x, y, x + len, y + th, z, z + th * pl5_cw / 2, 1); }
      else { int y = v3_rndr(0, G - len), x = v3_rndr(0, G - th); pl5_add(x, y, x + th, y + len, z, z + th * pl5_cw / 2, 1); }
    }
  }
  // painter's order: back (small x + y) to front; then the low ones first where they tie
  for (int i = 1; i < pl5_n; i++) {
    Pl5Obj v = pl5_o[i]; int j = i - 1;
    int kv = (v.x1 + v.y1) * 4 + v.z0 / 16;
    while (j >= 0 && (pl5_o[j].x1 + pl5_o[j].y1) * 4 + pl5_o[j].z0 / 16 > kv) { pl5_o[j + 1] = pl5_o[j]; j--; }
    pl5_o[j + 1] = v;
  }
  // fit the scene to the screen: centre its bounding box
  pl5_ox = 0; pl5_oy = 0;
  int32_t mnx = 1 << 30, mxx = -(1 << 30), mny = 1 << 30, mxy = -(1 << 30);
  for (int i = 0; i < pl5_n; i++) for (int c = 0; c < 8; c++) {
    int32_t sx, sy; pl5_iso((c & 1) ? pl5_o[i].x1 : pl5_o[i].x0, (c & 2) ? pl5_o[i].y1 : pl5_o[i].y0, (c & 4) ? pl5_o[i].z1 : pl5_o[i].z0, &sx, &sy);
    if (sx < mnx) mnx = sx; if (sx > mxx) mxx = sx; if (sy < mny) mny = sy; if (sy > mxy) mxy = sy;
  }
  if (pl5_n) { pl5_ox = 160 * 16 - (mnx + mxx) / 2; pl5_oy = 122 * 16 - (mny + mxy) / 2; }
  v3_seed = save;
}
// how many strokes an object takes, and stroke k of it (outline first, then the mesh on each face).
// All positions are in quarter cells; the mesh spacing pl5_meshQ is in quarter cells too.
static int pl5_counts(int oi, int* a, int* b, int* e) {
  const Pl5Obj& o = pl5_o[oi];
  int q = pl5_meshQ;
  *a = (o.x1 - o.x0) / q; *b = (o.y1 - o.y0) / q;
  int zs = q * pl5_cw / 4; if (zs < 3) zs = 3;
  *e = (o.z1 - o.z0) / zs;
  if (o.pipe) { if (o.x1 - o.x0 > o.y1 - o.y0) *b = 1; else *a = 1; *e = 0; }
  if (*a < 1) *a = 1; if (*b < 1) *b = 1;
  return 9 + (*a - 1) * 2 + (*b - 1) * 2 + (*e > 1 ? (*e - 1) * 2 : 0);
}
static void pl5_stroke(int oi, int k, int32_t* s) {
  const Pl5Obj& o = pl5_o[oi];
  int a, b, e; pl5_counts(oi, &a, &b, &e);
  int X0 = o.x0, X1 = o.x1, Y0 = o.y0, Y1 = o.y1, Z0 = o.z0, Z1 = o.z1;
  static const int8_t E[9][6] = {{0,0,1, 1,0,1},{1,0,1, 1,1,1},{1,1,1, 0,1,1},{0,1,1, 0,0,1},{0,1,1, 0,1,0},{1,1,1, 1,1,0},{1,0,1, 1,0,0},{0,1,0, 1,1,0},{1,1,0, 1,0,0}};
  if (k < 9) {                                                     // the outline: the 9 visible edges
    const int8_t* g = E[k];
    pl5_iso(g[0] ? X1 : X0, g[1] ? Y1 : Y0, g[2] ? Z1 : Z0, &s[0], &s[1]);
    pl5_iso(g[3] ? X1 : X0, g[4] ? Y1 : Y0, g[5] ? Z1 : Z0, &s[2], &s[3]);
    return;
  }
  k -= 9;
  if (k < a - 1) { int x = X0 + (k + 1) * (X1 - X0) / a; pl5_iso(x, Y0, Z1, &s[0], &s[1]); pl5_iso(x, Y1, Z1, &s[2], &s[3]); return; }   // top, one way
  k -= a - 1;
  if (k < b - 1) { int y = Y0 + (k + 1) * (Y1 - Y0) / b; pl5_iso(X0, y, Z1, &s[0], &s[1]); pl5_iso(X1, y, Z1, &s[2], &s[3]); return; }   // top, the other way
  k -= b - 1;
  if (k < a - 1) { int x = X0 + (k + 1) * (X1 - X0) / a; pl5_iso(x, Y1, Z1, &s[0], &s[1]); pl5_iso(x, Y1, Z0, &s[2], &s[3]); return; }   // left face, down
  k -= a - 1;
  if (k < b - 1) { int y = Y0 + (k + 1) * (Y1 - Y0) / b; pl5_iso(X1, y, Z1, &s[0], &s[1]); pl5_iso(X1, y, Z0, &s[2], &s[3]); return; }   // right face, down
  k -= b - 1;
  if (e > 1) {                                                     // across both side faces
    int z = Z1 - ((k >> 1) + 1) * (Z1 - Z0) / e;
    if ((k & 1) == 0) { pl5_iso(X0, Y1, z, &s[0], &s[1]); pl5_iso(X1, Y1, z, &s[2], &s[3]); }
    else { pl5_iso(X1, Y1, z, &s[0], &s[1]); pl5_iso(X1, Y0, z, &s[2], &s[3]); }
    return;
  }
  s[0] = s[1] = s[2] = s[3] = 0;
}
void prog_plotter_init() { v3_reset(); p5_paletteDirty(); pl5_preset = -1; pl5_new = true; }

static void plotter5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  pl5_o = (Pl5Obj*)v3_scratch;
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != pl5_preset) { pl5_preset = preset; pl5_look = preset == 11 ? 0 : preset; pl5_new = true; }
  const Pl5Preset& P = pl5_pre[pl5_look];
  const Pl5Preset& Q = pl5_pre[v3_centre(0) ? pl5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 4.0f) * 70.0f;                         // strokes per second
  int density = v3_centre(2) ? P.density : (int)(v3_u(2) * 150);
  int height = v3_centre(3) ? P.height : 10 + (int)(v3_u(3) * 140);
  int meshQ = v3_centre(4) ? P.mesh : 2 + (int)(v3_u(4) * 6);
  int pipes = v3_centre(5) ? P.pipes : (int)(v3_u(5) * 255);
  int scene = v3_centre(6) ? P.scene : v3_clamp(v3_k(6) * PL5_NSCENE / 1024, 0, PL5_NSCENE - 1);
  int air = (int)v3_rel(7, P.air, 2.0f);
  pl5_side = v3_centre(8) ? 1 : (v3_k(8) < 512 ? 0 : 1);
  float hold = v3_rel(9, 4.0f, 3.0f);
  bool gantry = v3_centre(10) ? true : v3_k(10) > 512;
  bool shadows = v3_centre(11) ? true : v3_k(11) > 512;
  p5_ink = v3_centre(12) ? Q.ink : (int)(v3_u(12) * 127);
  int cw = v3_centre(13) ? 17 : 8 + (int)(v3_u(13) * 16);
  int sprayB = v3_centre(14) ? Q.sprayB : (int)(v3_u(14) * 255);
  int mirror = v3_centre(15) ? 0 : (v3_k(15) < 512);
  if (meshQ != pl5_meshQ || cw != pl5_cw || scene != pl5_scene || mirror != pl5_mirror) { pl5_meshQ = meshQ; pl5_cw = cw; pl5_scene = scene; pl5_mirror = mirror; pl5_new = true; }
  p5_palette(Q.look, 1.0f);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !pl5_k12 && pl5_phase != PL5_FEED) { pl5_phase = PL5_FEED; pl5_t = 0; } pl5_k12 = k12;
  if (pl5_new) {
    pl5_new = false; pl5_seed = v3_rand() | 1;
    pl5_build(density, height, pipes, scene);
    pl5_total = 0;
    for (int i = 0; i < pl5_n; i++) { int a, b, e; pl5_o[i].first = (int16_t)pl5_total; pl5_total += pl5_counts(i, &a, &b, &e); }
    pl5_drawn = 0; pl5_phase = PL5_PLOT; pl5_t = 0; pl5_spray = 0;
  }
  float dt = v3_dt;
  pl5_t += dt;
  int scroll = 0;
  switch (pl5_phase) {
    case PL5_PLOT: pl5_drawn += speed * dt * (1.0f + pl5_drawn / 400.0f); if (pl5_drawn >= pl5_total) { pl5_drawn = (float)pl5_total; pl5_phase = PL5_SPRAY; pl5_t = 0; } break;
    case PL5_SPRAY: pl5_spray = pl5_t / 3.0f; if (speed <= 0) pl5_t -= dt; if (pl5_spray >= 1.0f) { pl5_spray = 1.0f; pl5_phase = PL5_HOLD; pl5_t = 0; } break;
    case PL5_HOLD: if (pl5_t > hold && speed > 0) { pl5_phase = PL5_FEED; pl5_t = 0; } break;
    case PL5_FEED: scroll = (int)(pl5_t * 160); if (scroll > H + 20) { if (pl5_preset == 11) pl5_look = (pl5_look + 1) % 11; pl5_new = true; } break;
  }

  // ── replay the plot ──
  memset(v3_buf, p5_tone(127, 0), W * H);
  int drawn = (int)pl5_drawn, sweep = (int)(pl5_spray * 420) - 50;   // the airbrush's left-to-right sweep (px)
  int32_t sOff = -scroll * 16;
  int penX = -1, penY = -1;
  for (int i = 0; i < pl5_n; i++) {
    const Pl5Obj& o = pl5_o[i];
    if (o.first >= drawn) break;
    // faces: paper-white until the airbrush has passed, then shaded
    int32_t T[8], L[8], R[8];
    pl5_iso(o.x0, o.y0, o.z1, &T[0], &T[1]); pl5_iso(o.x1, o.y0, o.z1, &T[2], &T[3]); pl5_iso(o.x1, o.y1, o.z1, &T[4], &T[5]); pl5_iso(o.x0, o.y1, o.z1, &T[6], &T[7]);
    pl5_iso(o.x0, o.y1, o.z1, &L[0], &L[1]); pl5_iso(o.x1, o.y1, o.z1, &L[2], &L[3]); pl5_iso(o.x1, o.y1, o.z0, &L[4], &L[5]); pl5_iso(o.x0, o.y1, o.z0, &L[6], &L[7]);
    pl5_iso(o.x1, o.y0, o.z1, &R[0], &R[1]); pl5_iso(o.x1, o.y1, o.z1, &R[2], &R[3]); pl5_iso(o.x1, o.y1, o.z0, &R[4], &R[5]); pl5_iso(o.x1, o.y0, o.z0, &R[6], &R[7]);
    int cx = (int)(T[4] >> 4);
    int sp = v3_clamp((sweep - cx) * 256 / 60, 0, 256);             // how much of the spray this object has had
    int lit = pl5_side ? 0 : 1;
    if (shadows && sp > 0) {                                       // cast shadow on the ground, to the lower right
      int32_t g[8]; int sh = (o.z1 - o.z0) / 3 * 16, shx = pl5_side ? sh : -sh;
      pl5_iso(o.x0, o.y1, o.z0, &g[0], &g[1]); pl5_iso(o.x1, o.y1, o.z0, &g[2], &g[3]); g[4] = g[2] + shx; g[5] = g[3] + sh / 3; g[6] = g[0] + shx; g[7] = g[1] + sh / 3;
      int32_t gx[4] = {g[0], g[2], g[4], g[6]}, gy[4] = {g[1] + sOff, g[3] + sOff, g[5] + sOff, g[7] + sOff};
      p5_quad(gx, gy, p5_tone(127 - (air * sp >> 8) * 3 / 4, sprayB > 128 ? 1 : 0));
    }
    int32_t tx[4] = {T[0], T[2], T[4], T[6]}, ty[4] = {T[1] + sOff, T[3] + sOff, T[5] + sOff, T[7] + sOff};
    int32_t lx[4] = {L[0], L[2], L[4], L[6]}, ly[4] = {L[1] + sOff, L[3] + sOff, L[5] + sOff, L[7] + sOff};
    int32_t rx[4] = {R[0], R[2], R[4], R[6]}, ry[4] = {R[1] + sOff, R[3] + sOff, R[5] + sOff, R[7] + sOff};
    int rampS = sprayB > 128 ? 1 : 0;
    p5_quad(tx, ty, p5_tone(127 - (air * sp >> 8) / 6, 0));
    p5_quad(lx, ly, p5_tone(127 - (air * sp >> 8) * (lit == 0 ? 1 : 2) / 2, rampS));
    p5_quad(rx, ry, p5_tone(127 - (air * sp >> 8) * (lit == 1 ? 1 : 2) / 2, rampS));
    if (sp > 0 && !o.pipe) {                                       // the airbrush gathers darker near the ground
      int y0 = (int)((R[3] + sOff) >> 4), y1 = (int)((R[5] + sOff) >> 4);
      for (int y = (y0 + y1) / 2; y < y1; y++) p5_sprayRow(y, (int)(L[0] >> 4), (int)(R[0] >> 4), ((y - (y0 + y1) / 2) * (air * sp >> 8)) / (y1 - y0 + 1), ((y - (y0 + y1) / 2) * (air * sp >> 8)) / (y1 - y0 + 1), (uint32_t)i);
    }
    // the strokes plotted so far
    int cnt = pl5_total - o.first; if (i + 1 < pl5_n) cnt = pl5_o[i + 1].first - o.first;
    int upto = drawn - o.first; if (upto > cnt) upto = cnt;
    uint8_t ink = p5_line(127, 0), inkS = p5_line(127 - 40, 0);
    for (int k = 0; k < upto; k++) {
      int32_t s[4]; pl5_stroke(i, k, s);
      p5_seg((int)(s[0] >> 4), (int)((s[1] + sOff) >> 4), (int)(s[2] >> 4), (int)((s[3] + sOff) >> 4), k < 9 ? ink : inkS);
      if (k == upto - 1 && upto < cnt) { penX = (int)(s[2] >> 4); penY = (int)((s[3] + sOff) >> 4); }
    }
    if (upto < cnt && upto > 0 && drawn - o.first < cnt) {          // the stroke in progress: drawn part of the way
      int32_t s[4]; pl5_stroke(i, upto, s);
      float f = pl5_drawn - (float)drawn;
      int ex = (int)((s[0] + (int32_t)((s[2] - s[0]) * f)) >> 4), ey = (int)((s[1] + (int32_t)((s[3] - s[1]) * f) + sOff) >> 4);
      p5_seg((int)(s[0] >> 4), (int)((s[1] + sOff) >> 4), ex, ey, upto < 9 ? ink : inkS);
      penX = ex; penY = ey;
    }
  }
  // ── the airbrush and the pen ──
  if (pl5_phase == PL5_SPRAY) {
    int x = sweep; uint8_t nz = p5_tone(30, sprayB > 128 ? 1 : 0);
    for (int k = 0; k < 60; k++) { int yy = v3_rnd(H), xx = x + v3_rndr(-6, 6); v3_put(xx, yy, nz); }   // spray drifting off the nozzle
    for (int y = 0; y < H; y += 2) v3_put(x, y, p5_tone(60, 0));
  }
  if (gantry && pl5_phase == PL5_PLOT && penX >= 0) {
    for (int x = 0; x < W; x += 2) { v3_put(x, penY - 7, p5_tone(70, 0)); v3_put(x, penY + 7, p5_tone(70, 0)); }   // the arm
    v3_disc(penX + 2, penY + 3, 4, p5_tone(85, 0));                 // the pen's shadow
    v3_disc(penX, penY, 4, p5_tone(20, 0)); v3_disc(penX, penY, 2, p5_tone(100, 0)); v3_put(penX, penY, p5_tone(0, 0));
  }
  if (pl5_phase == PL5_FEED) {                                       // the next sheet arriving below
    int top = H - scroll + 10; if (top < 0) top = 0;
    for (int y = top; y < H; y++) memset(v3_buf + y * W, p5_tone(127, 0), W);
    for (int y = top; y < top + 5 && y < H; y++) p5_sprayRow(y, 0, W - 1, 20 - (y - top) * 4, 20 - (y - top) * 4, 3);
  }
}
void prog_plotter_draw(int preset) { plotter5_draw(preset); fx5_apply(); }

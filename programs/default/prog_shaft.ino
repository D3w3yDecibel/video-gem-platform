// =====================================================================
// PROGRAM: SHAFT — falling down a pen-plotted shaft (by Dewey) — Set 5, slot 4
//
// After di_diff_world's pen plot + airbrush: you look straight down a deep
// industrial shaft full of towers, slabs and pipes. Every surface is drawn
// as a fine square mesh of pen lines, the tops lit and the walls shaded,
// and an airbrushed darkness swallows everything deeper down. The camera
// keeps sinking, so towers rush up past you and new ones appear below.
//
// How it works: each object is a box (a footprint, a top and a bottom).
// Boxes are drawn deepest-top first; for each one, the walls that face you
// and then its top. Walls are cut into bands that get darker with depth,
// and the mesh lines are spaced evenly in the world, so they bunch up in
// the distance the way they do in the plot.
//
// Presets:
//   k0 Pen Plot    — black ink on white paper, as in the original
//   k1 Blueprint   — white lines on blue
//   k2 Sepia       — brown ink on old paper
//   k3 Night Shaft — dark faces, glowing cyan lines
//   k4 Red Light   — one side lit by a red lamp
//   k5 Four Lights — every wall tinted by the light it faces, cyan and magenta
//   k6 Pipeworks   — mostly pipes, steel blue
//   k7 Towers      — tall towers, slow
//   k8 Freefall    — falling fast and spinning
//   k9 Gilded      — gold on black
//   k10 X-Ray      — only the lines, white on black
//   k11 Tour       — a new preset every 20 seconds
//
//   k12 — DROP: a burst of speed
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Spin (left/right = either way)
//   p3 Density   p4 Pipes ↔ Towers   p5 Mesh Size   p6 Depth (how far down you can see)   p7 Lens (wide … narrow)
//   p8 Light Angle   p9 Airbrush (how fast the dark comes up)   p10 Line Darkness   p11 Shaft Walls
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5p.h"

struct Sh5Box { int16_t x0, y0, x1, y1; int32_t zt, zb; uint8_t kind; uint8_t pad[3]; };
enum { SH5_TOWER, SH5_SLAB, SH5_PIPEX, SH5_PIPEY, SH5_RISER };
#define SH5_MAX 120
#define SH5_R 260                                     // half-width of the shaft (world units)
static Sh5Box* sh5_b;
static int sh5_n = 0;

struct Sh5Preset { P5Look look; uint8_t speed, pipes, density, spin, mesh, colour; int8_t ink; uint8_t fill; };   // fill: 0 = solid faces, 1 = lines only
static const Sh5Preset sh5_pre[11] = {
  {{{10,10,12},{120,120,118},{245,244,238},{10,10,12},{120,120,118},{245,244,238}}, 40, 90, 95, 20, 9, 0, 46, 0},
  {{{5,20,70},{30,80,160},{120,170,230},{5,20,70},{30,80,160},{120,170,230}}, 40, 110, 95, 30, 9, 0, -60, 0},
  {{{30,18,8},{140,100,60},{240,225,190},{30,18,8},{140,100,60},{240,225,190}}, 35, 90, 95, 20, 9, 0, 46, 0},
  {{{0,4,8},{10,40,50},{30,90,100},{0,4,8},{10,40,50},{30,90,100}}, 45, 90, 95, 30, 8, 0, -90, 0},
  {{{8,8,10},{100,100,104},{235,235,235},{20,0,0},{160,30,20},{255,170,140}}, 40, 90, 95, 20, 9, 1, 44, 0},
  {{{0,10,20},{40,140,160},{210,250,255},{20,0,20},{170,40,150},{255,200,250}}, 40, 90, 95, 25, 9, 2, 44, 0},
  {{{5,10,20},{70,100,130},{210,225,240},{5,10,20},{70,100,130},{210,225,240}}, 40, 200, 110, 20, 7, 0, 40, 0},
  {{{10,14,10},{100,115,100},{230,238,225},{10,14,10},{100,115,100},{230,238,225}}, 20, 30, 80, 10, 10, 0, 46, 0},
  {{{10,10,12},{120,120,118},{245,244,238},{10,10,12},{120,120,118},{245,244,238}}, 120, 90, 95, 160, 9, 0, 46, 0},
  {{{10,6,0},{140,95,20},{255,225,140},{10,6,0},{140,95,20},{255,225,140}}, 40, 90, 95, 20, 9, 0, 50, 0},
  {{{0,0,0},{60,70,90},{235,245,255},{0,0,0},{60,70,90},{235,245,255}}, 45, 90, 90, 30, 9, 0, 0, 1},
};
static const char* const sh5_names[12] = {
  "Pen Plot", "Blueprint", "Sepia", "Night Shaft", "Red Light", "Four Lights", "Pipeworks", "Towers", "Freefall", "Gilded", "X-Ray", "Tour"
};
static const char* const sh5_labels[16] = {
  "Palette", "Speed", "Spin", "Density", "Pipes / Towers", "Mesh Size", "Depth", "Lens",
  "Light Angle", "Airbrush", "Line Darkness", "Shaft Walls", "Tower Size", "Drift", "Coloured Light", "Direction"
};
const char* prog_shaft_name() { return "SHAFT"; }
const char* prog_shaft_character() { return "Falling down a deep shaft of towers and pipes drawn as pen-plotted mesh with airbrushed shadow"; }
const char* prog_shaft_presetName(int preset) { return (preset >= 0 && preset < 12) ? sh5_names[preset] : NULL; }
const char* prog_shaft_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? sh5_labels[pot] : ""; }
uint8_t prog_shaft_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int sh5_preset = -1, sh5_look = 0;
static float sh5_camZ = 0, sh5_rot = 0, sh5_tour = 0, sh5_boost = 0, sh5_t = 0;
static bool sh5_k12 = false;
// per frame: camera and lens
static int32_t sh5_cz, sh5_cx, sh5_cy, sh5_F, sh5_far;
static int32_t sh5_c, sh5_s;                           // rotation, Q14

static void sh5_spawn(int idx, int32_t zt, int pipes, float tower) {
  Sh5Box* b = &sh5_b[idx];
  int r = v3_rnd(256);
  int R = SH5_R - 20;
  if (r < pipes) {                                   // a pipe: long and thin, or a riser going straight down
    int w = v3_rndr(9, 18), len = v3_rndr(120, 420), k = v3_rnd(5);
    int cx = v3_rndr(-R, R), cy = v3_rndr(-R, R);
    if (k < 2) { b->kind = SH5_PIPEX; b->x0 = (int16_t)v3_clamp(cx - len / 2, -SH5_R, SH5_R); b->x1 = (int16_t)v3_clamp(cx + len / 2, -SH5_R, SH5_R); b->y0 = (int16_t)cy; b->y1 = (int16_t)(cy + w); b->zb = zt - w; }
    else if (k < 4) { b->kind = SH5_PIPEY; b->y0 = (int16_t)v3_clamp(cy - len / 2, -SH5_R, SH5_R); b->y1 = (int16_t)v3_clamp(cy + len / 2, -SH5_R, SH5_R); b->x0 = (int16_t)cx; b->x1 = (int16_t)(cx + w); b->zb = zt - w; }
    else { b->kind = SH5_RISER; b->x0 = (int16_t)cx; b->x1 = (int16_t)(cx + w); b->y0 = (int16_t)cy; b->y1 = (int16_t)(cy + w); b->zb = zt - v3_rndr(300, 1600); }
  } else if (r < pipes + (256 - pipes) / 3) {         // a slab
    int w = (int)(v3_rndr(60, 180) * tower), h = (int)(v3_rndr(40, 120) * tower);
    int cx = v3_rndr(-R, R), cy = v3_rndr(-R, R);
    b->kind = SH5_SLAB; b->x0 = (int16_t)(cx - w / 2); b->x1 = (int16_t)(cx + w / 2); b->y0 = (int16_t)(cy - h / 2); b->y1 = (int16_t)(cy + h / 2);
    b->zb = zt - v3_rndr(16, 50);
  } else {                                           // a tower
    int w = (int)(v3_rndr(40, 140) * tower), h = (int)(v3_rndr(40, 140) * tower);
    int cx = v3_rndr(-R, R), cy = v3_rndr(-R, R);
    b->kind = SH5_TOWER; b->x0 = (int16_t)(cx - w / 2); b->x1 = (int16_t)(cx + w / 2); b->y0 = (int16_t)(cy - h / 2); b->y1 = (int16_t)(cy + h / 2);
    b->zb = zt - v3_rndr(400, 2400);
  }
  b->zt = zt;
}
void prog_shaft_init() {
  v3_reset(); p5_paletteDirty(); sh5_preset = -1; sh5_b = (Sh5Box*)v3_scratch; sh5_n = 0; sh5_camZ = 0; sh5_rot = 0;
}
// world (x, y, z) → screen, 1/16 px; false if too close or behind
static inline bool sh5_proj(int32_t x, int32_t y, int32_t z, int32_t* sx, int32_t* sy) {
  int32_t d = sh5_cz - z; if (d < 8) return false;
  x -= sh5_cx; y -= sh5_cy;
  int32_t xr = (x * sh5_c - y * sh5_s) >> 14, yr = (x * sh5_s + y * sh5_c) >> 14;
  int32_t inv = (sh5_F << 16) / d;
  *sx = 160 * 16 + ((xr * inv) >> 12); *sy = 120 * 16 + ((yr * inv) >> 12);
  return true;
}
// two world points at the same depth → screen (one divide for both)
static inline bool sh5_proj2(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t z, int32_t* s) {
  int32_t d = sh5_cz - z; if (d < 8) return false;
  int32_t inv = (sh5_F << 16) / d;
  ax -= sh5_cx; ay -= sh5_cy; bx -= sh5_cx; by -= sh5_cy;
  int32_t xr = (ax * sh5_c - ay * sh5_s) >> 14, yr = (ax * sh5_s + ay * sh5_c) >> 14;
  s[0] = 160 * 16 + ((xr * inv) >> 12); s[1] = 120 * 16 + ((yr * inv) >> 12);
  xr = (bx * sh5_c - by * sh5_s) >> 14; yr = (bx * sh5_s + by * sh5_c) >> 14;
  s[2] = 160 * 16 + ((xr * inv) >> 12); s[3] = 120 * 16 + ((yr * inv) >> 12);
  return true;
}
static float sh5_air = 1.0f, sh5_colour = 0, sh5_airWas = -1;
static int32_t sh5_lx = 150, sh5_ly = -200;           // light direction, Q8
static uint8_t sh5_fog[129];                          // airbrush darkness by depth (/255), worked out when the knob moves
static int sh5_mesh = 9, sh5_fillMode = 0;
static inline uint8_t sh5_lc(int t, int ramp) { return sh5_fillMode ? p5_tone(t + 20, ramp) : p5_line(t, ramp); }   // pen line colour
static int sh5_tone(int light, int32_t z) {             // light: 0 … 256
  int32_t d = sh5_cz - z; if (d < 0) d = 0;
  int u = d >= sh5_far ? 128 : (int)((d << 7) / sh5_far);
  return (127 * light * sh5_fog[u]) >> 16;
}
// a wall: the edge (ax, ay)-(bx, by) running from z0 down to z1; ramp = which tone ramp
static void sh5_wall(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t z0, int32_t z1, int light, int ramp) {
  int32_t zEnd = z1 > sh5_cz - sh5_far ? z1 : sh5_cz - sh5_far;
  if (z0 >= sh5_cz - 8) z0 = sh5_cz - 9;
  if (zEnd >= z0) return;
  // tone bands: 6 bands, closer together near the top
  const int NB = 6;
  int32_t zs[NB + 1];
  for (int k = 0; k <= NB; k++) { int32_t d0 = sh5_cz - z0, d1 = sh5_cz - zEnd; zs[k] = sh5_cz - (d0 + (d1 - d0) * k * k / (NB * NB)); }
  int32_t qa[4], qb[4];
  bool okA = sh5_proj2(ax, ay, bx, by, zs[0], qa);
  for (int k = 0; k < NB; k++, okA = true) {
    bool okB = sh5_proj2(ax, ay, bx, by, zs[k + 1], qb);
    int32_t xs[4] = {qa[0], qa[2], qb[2], qb[0]}, ys[4] = {qa[1], qa[3], qb[3], qb[1]};
    for (int j = 0; j < 4; j++) qa[j] = qb[j];
    if (!okB || !okA) continue;
    int t = sh5_tone(light, (zs[k] + zs[k + 1]) / 2);
    if (!sh5_fillMode) p5_quad(xs, ys, p5_tone(t, ramp));
  }
  // mesh: lines down the wall, and lines across it at even world steps
  int32_t len = v3_abs(bx - ax) + v3_abs(by - ay);
  int nu = len / sh5_mesh; if (nu > 40) nu = 40;
  int32_t tx0, ty0, tx1, ty1, ex0, ey0, ex1, ey1;
  if (!sh5_proj(ax, ay, z0, &tx0, &ty0) || !sh5_proj(bx, by, z0, &tx1, &ty1) || !sh5_proj(ax, ay, zEnd, &ex0, &ey0) || !sh5_proj(bx, by, zEnd, &ex1, &ey1)) return;
  int tTop = sh5_tone(light, z0), tMid = sh5_tone(light, (z0 + zEnd) / 2);
  uint8_t lc = sh5_lc(tMid, ramp);
  { int32_t xs4[4] = {tx0, tx1, ex1, ex0}, ys4[4] = {ty0, ty1, ey1, ey0}; p5_mesh(xs4, ys4, nu, 0, lc); }
  int32_t z = z0, lastY = -9999, lastX = -9999;
  for (int k = 0; k < 64 && z > zEnd; k++, z -= sh5_mesh) {
    int32_t q[4];
    if (!sh5_proj2(ax, ay, bx, by, z, q)) continue;
    if (v3_abs(q[0] - lastX) + v3_abs(q[1] - lastY) < 40) break;   // closer than 2.5 px: stop, it would be solid
    lastX = q[0]; lastY = q[1];
    int t = sh5_tone(light, z);
    p5_seg(q[0] >> 4, q[1] >> 4, q[2] >> 4, q[3] >> 4, sh5_lc(t, ramp));
  }
  (void)tTop;
}
static void sh5_drawBox(int idx) {
  const Sh5Box& b = sh5_b[idx];
  int32_t zt = b.zt, zb = b.zb;
  if (zt >= sh5_cz - 8) return;
  int32_t X0 = b.x0, X1 = b.x1, Y0 = b.y0, Y1 = b.y1;
  int32_t cx = sh5_cx, cy = sh5_cy;
  // walls that face the camera; each gets light from the lamp's direction
  struct { int32_t ax, ay, bx, by; int nx, ny; bool vis; } wl[4] = {
    {X1, Y0, X1, Y1, 1, 0, cx > X1}, {X0, Y1, X0, Y0, -1, 0, cx < X0}, {X1, Y1, X0, Y1, 0, 1, cy > Y1}, {X0, Y0, X1, Y0, 0, -1, cy < Y0} };
  for (int i = 0; i < 4; i++) {
    if (!wl[i].vis) continue;
    int32_t nx = (wl[i].nx * sh5_c - wl[i].ny * sh5_s) >> 6, ny = (wl[i].nx * sh5_s + wl[i].ny * sh5_c) >> 6;   // Q8
    int32_t d = (nx * sh5_lx + ny * sh5_ly) >> 8;                                                             // Q8
    int light = 108 + ((84 * (d > 0 ? d : 0)) >> 8);
    int ramp = 0;
    if (sh5_colour > 0.5f) ramp = (sh5_colour > 1.5f) ? (wl[i].nx != 0 ? 1 : 0) : (d > 77 ? 1 : 0);
    sh5_wall(wl[i].ax, wl[i].ay, wl[i].bx, wl[i].by, zt, zb, light, ramp);
  }
  // the top
  int32_t xs[4], ys[4];
  if (!sh5_proj(X0, Y0, zt, &xs[0], &ys[0]) || !sh5_proj(X1, Y0, zt, &xs[1], &ys[1]) || !sh5_proj(X1, Y1, zt, &xs[2], &ys[2]) || !sh5_proj(X0, Y1, zt, &xs[3], &ys[3])) return;
  int t = sh5_tone(256, zt);
  int pw = (v3_abs(xs[1] - xs[0]) + v3_abs(ys[1] - ys[0])) >> 4, ph = (v3_abs(xs[3] - xs[0]) + v3_abs(ys[3] - ys[0])) >> 4;
  if (b.kind == SH5_PIPEX || b.kind == SH5_PIPEY) {                   // a pipe: three strips across (dark, light, dark) and rings along it
    bool alongX = b.kind == SH5_PIPEX;
    static const uint8_t sh5_band[3] = {140, 255, 154};
    for (int k = 0; k < 3; k++) {
      int32_t qx[4], qy[4];
      if (alongX) {                                   // strips run along x: split edges 0→3 and 1→2
        qx[0] = xs[0] + (xs[3] - xs[0]) * k / 3;       qy[0] = ys[0] + (ys[3] - ys[0]) * k / 3;
        qx[1] = xs[1] + (xs[2] - xs[1]) * k / 3;       qy[1] = ys[1] + (ys[2] - ys[1]) * k / 3;
        qx[2] = xs[1] + (xs[2] - xs[1]) * (k + 1) / 3; qy[2] = ys[1] + (ys[2] - ys[1]) * (k + 1) / 3;
        qx[3] = xs[0] + (xs[3] - xs[0]) * (k + 1) / 3; qy[3] = ys[0] + (ys[3] - ys[0]) * (k + 1) / 3;
      } else {                                        // strips run along y: split edges 0→1 and 3→2
        qx[0] = xs[0] + (xs[1] - xs[0]) * k / 3;       qy[0] = ys[0] + (ys[1] - ys[0]) * k / 3;
        qx[1] = xs[0] + (xs[1] - xs[0]) * (k + 1) / 3; qy[1] = ys[0] + (ys[1] - ys[0]) * (k + 1) / 3;
        qx[2] = xs[3] + (xs[2] - xs[3]) * (k + 1) / 3; qy[2] = ys[3] + (ys[2] - ys[3]) * (k + 1) / 3;
        qx[3] = xs[3] + (xs[2] - xs[3]) * k / 3;       qy[3] = ys[3] + (ys[2] - ys[3]) * k / 3;
      }
      if (!sh5_fillMode) p5_quad(qx, qy, p5_tone((t * sh5_band[k]) >> 8, 0));
    }
    int n = (alongX ? pw : ph) / 3; if (n > 50) n = 50;
    if (alongX) p5_mesh(xs, ys, n, 3, sh5_lc(t, 0));
    else p5_mesh(xs, ys, 3, n, sh5_lc(t, 0));
    return;
  }
  if (!sh5_fillMode) p5_quad(xs, ys, p5_tone(t, 0));
  int nu = (X1 - X0) / sh5_mesh, nv = (Y1 - Y0) / sh5_mesh;
  if (nu > pw / 2) nu = pw / 2; if (nv > ph / 2) nv = ph / 2;
  p5_mesh(xs, ys, nu, nv, sh5_lc(t, 0));
  if (b.kind == SH5_TOWER && pw > 20) p5_outline(xs, ys, p5_line(t - 10, 0));
}

static void shaft5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != sh5_preset) { sh5_preset = preset; sh5_look = preset == 11 ? 0 : preset; sh5_tour = 0; sh5_n = 0; }
  if (preset == 11) { sh5_tour += v3_dt; if (sh5_tour > 20.0f) { sh5_tour = 0; sh5_look = (sh5_look + 1) % 11; } }
  const Sh5Preset& P = sh5_pre[sh5_look];
  const Sh5Preset& Q = sh5_pre[v3_centre(0) ? sh5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];
  sh5_b = (Sh5Box*)v3_scratch;

  float speed = v3_speed(1, 3.0f) * P.speed * 3.0f;
  float spin = v3_centre(2) ? P.spin / 600.0f : v3_bi(2) * 0.8f;
  int n = v3_centre(3) ? P.density : 12 + (int)(v3_u(3) * (SH5_MAX - 12));
  int pipes = v3_centre(4) ? P.pipes : (int)((1.0f - v3_u(4)) * 255);
  sh5_mesh = v3_centre(5) ? P.mesh : 4 + (int)(v3_u(5) * 20);
  sh5_far = (int32_t)v3_rel(6, 2600, 2.0f);
  sh5_F = (int32_t)v3_rel(7, 150, 1.8f);
  float la = v3_centre(8) ? -0.9f + sh5_t * 0.05f : v3_u(8) * 6.2832f;
  sh5_lx = (int32_t)(cosf(la) * 256); sh5_ly = (int32_t)(sinf(la) * 256);
  sh5_air = v3_rel(9, 2.0f, 3.0f);
  p5_ink = v3_centre(10) ? Q.ink : (int)(v3_bi(10) * 110);
  float walls = v3_centre(11) ? 1.0f : v3_u(11) * 2.0f;
  float tower = v3_rel(12, 1.0f, 2.0f);
  float drift = v3_centre(13) ? 0.3f : v3_u(13);
  sh5_colour = v3_centre(14) ? Q.colour : v3_u(14) * 2.99f;
  float dirn = v3_centre(15) ? 1.0f : (v3_k(15) < 512 ? -1.0f : 1.0f);
  sh5_fillMode = Q.fill;

  p5_palette(Q.look, 0.9f);
  if (sh5_air != sh5_airWas) { sh5_airWas = sh5_air; for (int u = 0; u <= 128; u++) sh5_fog[u] = (uint8_t)(255 * powf(1.0f - u / 128.0f, sh5_air)); }

  // ── motion ──
  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !sh5_k12) sh5_boost = 1.0f; sh5_k12 = k12;
  sh5_boost -= v3_dt * 0.5f; if (sh5_boost < 0) sh5_boost = 0;
  float dt = v3_dt;
  sh5_t += dt;
  sh5_camZ -= dt * speed * (1.0f + 5.0f * sh5_boost) * dirn;
  sh5_rot += dt * spin;
  if (sh5_rot > 6.2832f) sh5_rot -= 6.2832f; else if (sh5_rot < 0) sh5_rot += 6.2832f;
  sh5_cz = (int32_t)sh5_camZ;
  sh5_cx = (int32_t)(drift * 90 * sinf(sh5_t * 0.21f)); sh5_cy = (int32_t)(drift * 90 * sinf(sh5_t * 0.17f + 1.3f));
  sh5_c = (int32_t)(cosf(sh5_rot) * 16384); sh5_s = (int32_t)(sinf(sh5_rot) * 16384);

  // ── the boxes: recycle the ones we've passed (or that fell out of sight when rising) ──
  const int32_t span = sh5_far + 200;
  if (sh5_n != n) {
    if (n > sh5_n) for (int i = sh5_n; i < n; i++) sh5_spawn(i, sh5_cz - 60 - (int32_t)v3_rnd(span), pipes, tower);
    sh5_n = n;
  }
  for (int i = 0; i < sh5_n; i++) {
    Sh5Box& b = sh5_b[i];
    if (b.zt > sh5_cz - 30) sh5_spawn(i, b.zt - span, pipes, tower);
    else if (b.zt < sh5_cz - span - 60) sh5_spawn(i, b.zt + span, pipes, tower);
  }
  for (int i = 1; i < sh5_n; i++) {                                  // deepest top first (insertion sort: nearly sorted already)
    Sh5Box v = sh5_b[i]; int j = i - 1;
    while (j >= 0 && sh5_b[j].zt > v.zt) { sh5_b[j + 1] = sh5_b[j]; j--; }
    sh5_b[j + 1] = v;
  }

  // ── draw: darkness at the bottom, the shaft's own walls, then every box ──
  memset(v3_buf, p5_tone(sh5_fillMode ? 0 : 0, 0), W * H);
  if (walls > 0.05f) {
    int32_t R = (int32_t)(SH5_R * (0.9f + walls * 0.6f)) + 40;
    sh5_wall(-R, -R, R, -R, sh5_cz - 10, sh5_cz - sh5_far, 128, 0);   // seen from inside: all four
    sh5_wall(R, -R, R, R, sh5_cz - 10, sh5_cz - sh5_far, 160, sh5_colour > 1.5f ? 1 : 0);
    sh5_wall(R, R, -R, R, sh5_cz - 10, sh5_cz - sh5_far, 115, 0);
    sh5_wall(-R, R, -R, -R, sh5_cz - 10, sh5_cz - sh5_far, 140, sh5_colour > 1.5f ? 1 : 0);
  }
  for (int i = 0; i < sh5_n; i++) sh5_drawBox(i);
}
void prog_shaft_draw(int preset) { shaft5_draw(preset); fx5_apply(); }

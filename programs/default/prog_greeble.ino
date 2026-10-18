// =====================================================================
// PROGRAM: GREEBLE — flying over a plotted machine city (by Dewey) — Set 5, slot 7
//
// An endless plan of blocks, plant and pipe runs scrolling under you, seen
// from above at a slant: every block is extruded and covered in pen mesh,
// and up to four lamps each throw their own airbrushed shadow across the
// ground — as in di_diff_world's piece lit by four light sources, one
// stencil per light. With coloured lamps the shadows come out in colour.
//
// The plan is worked out from the position alone (a hash of each grid
// cell), so it never repeats and never needs storing.
//
// Presets:
//   k0 Plan View     — black and white, one lamp
//   k1 Two Lamps     — a warm lamp and a blue one
//   k2 Four Lights   — four shadows round every block
//   k3 Night Works   — dark ground, glowing roofs
//   k4 Blueprint     — white on blue
//   k5 Rooftops      — terracotta and warm light
//   k6 Circuit Board — chips and traces on green
//   k7 Desert Blocks — sand and long low-sun shadows
//   k8 Brutalist     — tall concrete, the sun going round
//   k9 Ice Station   — white blocks, blue shadows
//   k10 Neon Grid    — only lines, bright on black
//   k11 Tour         — a new preset every 20 seconds
//
//   k12 — SUN TRAVEL: the lamps sweep round once, fast
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Heading   p3 Density
//   p4 Height   p5 Block Size   p6 Lamps (1-4)   p7 Lamp Turn (how fast the lamps circle)
//   p8 Shadow Length   p9 Shadow Darkness   p10 Mesh Size   p11 Pipes
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5p.h"

struct Gr5Preset { P5Look look; uint8_t lamps, density, height, cell, mesh, pipes, shLen, shDark, colour, turn, ground; int8_t ink; uint8_t fill; };
static const Gr5Preset gr5_pre[11] = {
  {{{10,10,12},{120,120,122},{246,245,240},{10,10,12},{120,120,122},{246,245,240}}, 1, 140, 60, 26, 5, 40, 60, 40, 0, 10, 120, 46, 0},
  {{{14,10,6},{150,110,70},{250,235,210},{0,10,40},{40,80,170},{200,220,255}}, 2, 140, 60, 26, 5, 40, 60, 40, 1, 20, 120, 46, 0},
  {{{10,10,12},{120,120,122},{246,245,240},{20,0,30},{150,40,140},{255,200,240}}, 4, 120, 60, 28, 5, 30, 50, 28, 1, 10, 122, 46, 0},
  {{{0,0,6},{20,30,60},{255,230,150},{0,0,6},{20,30,60},{255,230,150}}, 2, 140, 60, 26, 5, 40, 60, 30, 0, 10, 10, -60, 0},
  {{{6,24,80},{60,110,190},{220,235,255},{6,24,80},{60,110,190},{220,235,255}}, 1, 140, 60, 26, 5, 40, 60, 30, 0, 10, 70, -50, 0},
  {{{40,14,6},{190,90,50},{255,230,200},{40,14,6},{190,90,50},{255,230,200}}, 1, 170, 40, 22, 4, 20, 70, 40, 0, 10, 110, 46, 0},
  {{{0,20,8},{20,110,50},{190,255,190},{0,10,4},{60,60,60},{230,230,230}}, 1, 110, 25, 24, 4, 120, 40, 30, 1, 0, 30, -40, 0},
  {{{40,24,8},{190,140,80},{255,235,190},{40,24,8},{190,140,80},{255,235,190}}, 1, 90, 40, 30, 6, 10, 150, 40, 0, 6, 118, 40, 0},
  {{{14,14,14},{120,118,112},{230,228,220},{14,14,14},{120,118,112},{230,228,220}}, 1, 130, 120, 34, 6, 20, 90, 46, 0, 40, 112, 46, 0},
  {{{10,30,60},{120,170,220},{250,252,255},{0,20,60},{60,120,210},{220,235,255}}, 2, 120, 60, 26, 5, 30, 70, 30, 1, 10, 124, 40, 0},
  {{{0,0,0},{40,0,60},{255,120,255},{0,0,0},{0,60,80},{120,255,255}}, 2, 140, 60, 26, 5, 40, 60, 0, 1, 15, 0, 0, 1},
};
static const char* const gr5_names[12] = {
  "Plan View", "Two Lamps", "Four Lights", "Night Works", "Blueprint", "Rooftops", "Circuit Board", "Desert Blocks", "Brutalist", "Ice Station", "Neon Grid", "Tour"
};
static const char* const gr5_labels[16] = {
  "Palette", "Speed", "Heading", "Density", "Height", "Block Size", "Lamps", "Lamp Turn",
  "Shadow Length", "Shadow Darkness", "Mesh Size", "Pipes", "Tilt", "Line Darkness", "Coloured Light", "Breathing"
};
const char* prog_greeble_name() { return "GREEBLE"; }
const char* prog_greeble_character() { return "Flying over an endless pen-plotted machine city, each lamp throwing its own airbrushed shadow"; }
const char* prog_greeble_presetName(int preset) { return (preset >= 0 && preset < 12) ? gr5_names[preset] : NULL; }
const char* prog_greeble_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? gr5_labels[pot] : ""; }
uint8_t prog_greeble_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int gr5_preset = -1, gr5_look = 0;
static float gr5_x = 0, gr5_y = 0, gr5_t = 0, gr5_tour = 0, gr5_lampA = 0, gr5_sweep = 0;
static bool gr5_k12 = false;

// darken a convex polygon (shadow), and paint it into a ramp (the lamp's colour)
static void gr5_shade(const int32_t* xs, const int32_t* ys, int n, int amt, int ramp) {
  // reuse p5_fill's edge walk by filling a marker, then darken: done row by row with the span tables
  if (n < 3) return;
  int32_t ymin = ys[0], ymax = ys[0];
  for (int i = 1; i < n; i++) { if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i]; }
  int r0 = (int)((ymin + 7) >> 4), r1 = (int)((ymax - 8) >> 4);
  if (r0 < 0) r0 = 0; if (r1 > H - 1) r1 = H - 1;
  if (r0 > r1) return;
  for (int y = r0; y <= r1; y++) { p5_xl[y] = 32767; p5_xr[y] = -32768; }
  for (int i = 0; i < n; i++) {
    int j = i + 1 == n ? 0 : i + 1;
    int32_t xa = xs[i], ya = ys[i], xb = xs[j], yb = ys[j];
    if (ya == yb) continue;
    if (ya > yb) { int32_t t = xa; xa = xb; xb = t; t = ya; ya = yb; yb = t; }
    int e0 = (int)((ya + 7) >> 4), e1 = (int)((yb - 8) >> 4);
    if (e0 < r0) e0 = r0; if (e1 > r1) e1 = r1;
    if (e0 > e1) continue;
    int32_t step = (xb - xa) * 4096 / (yb - ya);
    int32_t x8 = xa * 256 + (((int32_t)e0 * 16 + 8 - ya) * step >> 4);
    for (int y = e0; y <= e1; y++, x8 += step) {
      int32_t x16 = x8 >> 8; int xl = (int)((x16 + 8) >> 4), xr = (int)((x16 - 8) >> 4);
      if (xl < p5_xl[y]) p5_xl[y] = (int16_t)xl; if (xr > p5_xr[y]) p5_xr[y] = (int16_t)xr;
    }
  }
  for (int y = r0; y <= r1; y++) {
    int a = p5_xl[y], b = p5_xr[y]; if (a < 0) a = 0; if (b > W - 1) b = W - 1;
    uint8_t* p = v3_buf + y * W;
    for (int x = a; x <= b; x++) { int t = (p[x] & 127) - amt; p[x] = (uint8_t)(ramp * 128 | (t < 0 ? 0 : t)); }
  }
}
// the convex outline of a rectangle and its copy moved by (dx, dy): a hexagon, in order
static int gr5_hull(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t dx, int32_t dy, int32_t* xs, int32_t* ys) {
  int32_t cx[8] = {x0, x1, x1, x0, x0 + dx, x1 + dx, x1 + dx, x0 + dx}, cy[8] = {y0, y0, y1, y1, y0 + dy, y0 + dy, y1 + dy, y1 + dy};
  // a gift-wrap over 8 points is overkill: pick the six by the shadow's direction
  int n = 0;
  if (dx >= 0 && dy >= 0) { int o[6] = {0, 1, 5, 6, 7, 3}; for (int i = 0; i < 6; i++) { xs[n] = cx[o[i]]; ys[n] = cy[o[i]]; n++; } }
  else if (dx < 0 && dy >= 0) { int o[6] = {0, 1, 2, 6, 7, 4}; for (int i = 0; i < 6; i++) { xs[n] = cx[o[i]]; ys[n] = cy[o[i]]; n++; } }
  else if (dx < 0 && dy < 0) { int o[6] = {4, 5, 1, 2, 3, 7}; for (int i = 0; i < 6; i++) { xs[n] = cx[o[i]]; ys[n] = cy[o[i]]; n++; } }
  else { int o[6] = {4, 5, 6, 2, 3, 0}; for (int i = 0; i < 6; i++) { xs[n] = cx[o[i]]; ys[n] = cy[o[i]]; n++; } }
  return n;
}
void prog_greeble_init() { v3_reset(); p5_paletteDirty(); gr5_preset = -1; }

static void greeble5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != gr5_preset) { gr5_preset = preset; gr5_look = preset == 11 ? 0 : preset; gr5_tour = 0; }
  if (preset == 11) { gr5_tour += v3_dt; if (gr5_tour > 20.0f) { gr5_tour = 0; gr5_look = (gr5_look + 1) % 11; } }
  const Gr5Preset& P = gr5_pre[gr5_look];
  const Gr5Preset& Q = gr5_pre[v3_centre(0) ? gr5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 4.0f) * 22.0f;
  float head = v3_centre(2) ? 0.6f + 0.25f * sinf(gr5_t * 0.05f) : v3_u(2) * 6.2832f;
  int density = v3_centre(3) ? P.density : (int)(v3_u(3) * 255);
  int height = v3_centre(4) ? P.height : 8 + (int)(v3_u(4) * 150);
  int cell = v3_centre(5) ? P.cell : 14 + (int)(v3_u(5) * 34);
  int lamps = v3_centre(6) ? P.lamps : 1 + v3_clamp(v3_k(6) * 4 / 1024, 0, 3);
  float turn = v3_centre(7) ? P.turn / 400.0f : v3_bi(7) * 0.6f;
  float shLen = v3_rel(8, P.shLen / 100.0f, 2.5f);
  int shDark = v3_centre(9) ? P.shDark : (int)(v3_u(9) * 90);
  int mesh = v3_centre(10) ? P.mesh : 3 + (int)(v3_u(10) * 12);
  int pipes = v3_centre(11) ? P.pipes : (int)(v3_u(11) * 255);
  float tilt = v3_rel(12, 1.0f, 2.0f);
  p5_ink = v3_centre(13) ? Q.ink : (int)(v3_bi(13) * 110);
  int colour = v3_centre(14) ? Q.colour : v3_k(14) > 512;
  float breathe = v3_centre(15) ? 0.0f : v3_u(15);
  int fillMode = Q.fill;
  p5_palette(Q.look, 0.95f);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !gr5_k12) gr5_sweep = 6.2832f; gr5_k12 = k12;
  float dt = v3_dt;
  gr5_t += dt;
  gr5_x += cosf(head) * speed * dt; gr5_y += sinf(head) * speed * dt;
  if (gr5_x > 1e6f || gr5_x < -1e6f) gr5_x = 0; if (gr5_y > 1e6f || gr5_y < -1e6f) gr5_y = 0;
  float sw = gr5_sweep > 0 ? dt * 3.0f : 0; gr5_sweep -= sw;
  gr5_lampA += dt * turn + sw;

  // lamps: directions of their shadows (Q8 px per px of height)
  int32_t ldx[4], ldy[4];
  for (int k = 0; k < lamps; k++) {
    float a = gr5_lampA + k * 6.2832f / lamps + 0.5f;
    ldx[k] = (int32_t)(cosf(a) * shLen * 256); ldy[k] = (int32_t)(sinf(a) * shLen * 256);
  }
  int32_t tx = (int32_t)(-0.28f * tilt * 256), ty = (int32_t)(-0.42f * tilt * 256);   // how a height of 1 shifts on screen (Q8)

  // ── ground: paper with a mesh that travels with the city ──
  int gTone = Q.ground;
  memset(v3_buf, p5_tone(gTone, 0), W * H);
  int ox = (int)gr5_x, oy = (int)gr5_y;
  {
    uint8_t gl = p5_tone(fillMode ? 40 : gTone - 14, 0);
    int gs = cell / 2 > 6 ? cell / 2 : 6;
    for (int y = ((-oy) % gs + gs) % gs; y < H; y += gs) memset(v3_buf + y * W, gl, W);
    for (int x = ((-ox) % gs + gs) % gs; x < W; x += gs) for (int y = 0; y < H; y++) v3_buf[y * W + x] = gl;
  }
  // ── the blocks in view (a margin all round for tall ones and their shadows) ──
  int c0x = (int)floorf((float)ox / cell) - 2, c1x = (int)floorf((float)(ox + W) / cell) + 6;
  int c0y = (int)floorf((float)oy / cell) - 2, c1y = (int)floorf((float)(oy + H) / cell) + 6;
  // shadows first, all of them, so they lie on the ground under everything
  for (int pass = 0; pass < 2; pass++) {
    for (int cy = c0y; cy <= c1y; cy++) for (int cx = c0x; cx <= c1x; cx++) {
      uint32_t h = v3_hash(((uint32_t)(cx) * 73856093u) ^ ((uint32_t)(cy) * 19349663u));
      bool block = (int)(h & 255) < density;
      int ph = 0;
      // pipes: some rows carry a pipe the whole way along
      uint32_t hr = v3_hash((uint32_t)cy * 2654435761u + 99);
      bool pipeRow = (int)(hr & 255) < pipes / 3;
      if (!block && !pipeRow) continue;
      int x0, y0, x1, y1, z;
      if (block) {
        int m = 2 + (int)((h >> 8) & 3), w = cell - m - (int)((h >> 10) & 7), d = cell - m - (int)((h >> 13) & 7);
        x0 = cx * cell + m - ox; y0 = cy * cell + m - oy; x1 = x0 + (w > 4 ? w : 4); y1 = y0 + (d > 4 ? d : 4);
        z = 4 + (int)(((h >> 16) & 255) * height >> 9);
        if (((h >> 24) & 7) == 0) z = z * 3 / 2 + 12;                    // now and then a tall one
        if (breathe > 0) z = (int)(z * (1.0f + breathe * 0.6f * sinf(gr5_t * 1.3f + (cx * 0.7f + cy * 0.4f))));
        if (z < 2) z = 2;
      } else {
        x0 = cx * cell - ox; x1 = x0 + cell; int th = 3 + (int)(hr >> 8 & 3); y0 = cy * cell + cell / 2 - th - oy; y1 = y0 + th * 2;
        z = th * 2 + 4 + (int)((hr >> 12) & 15); ph = 1;
      }
      if (x1 + 60 < 0 || x0 - 80 > W || y1 + 60 < 0 || y0 - 120 > H) continue;
      if (pass == 0) {
        if (fillMode) continue;
        for (int k = 0; k < lamps; k++) {
          int32_t xs[6], ys[6];
          int n = gr5_hull(x0 * 16, y0 * 16, x1 * 16, y1 * 16, (ldx[k] * z) >> 4, (ldy[k] * z) >> 4, xs, ys);
          gr5_shade(xs, ys, n, shDark, colour ? (k & 1) : 0);
        }
        continue;
      }
      // the block: two side faces (south and east, which face us), then the top
      int32_t sx = (tx * z) >> 8, sy = (ty * z) >> 8;
      int topT = fillMode ? 6 : (ph ? 112 : 124), sTone = fillMode ? 4 : 92, eTone = fillMode ? 4 : 70;
      int32_t S[4] = {x0 * 16, x1 * 16, (x1 + sx) * 16, (x0 + sx) * 16}, Sy[4] = {y1 * 16, y1 * 16, (y1 + sy) * 16, (y1 + sy) * 16};
      int32_t E[4] = {x1 * 16, x1 * 16, (x1 + sx) * 16, (x1 + sx) * 16}, Ey[4] = {y0 * 16, y1 * 16, (y1 + sy) * 16, (y0 + sy) * 16};
      int32_t T[4] = {(x0 + sx) * 16, (x1 + sx) * 16, (x1 + sx) * 16, (x0 + sx) * 16}, Ty[4] = {(y0 + sy) * 16, (y0 + sy) * 16, (y1 + sy) * 16, (y1 + sy) * 16};
      int ramp = (colour && !ph && ((h >> 5) & 3) == 0) ? 1 : 0;     // the odd block lit in the lamp's colour
      uint8_t lineC = fillMode ? p5_tone(110 - ((h >> 4) & 31), (h >> 7) & 1) : 0;
      p5_quad(S, Sy, p5_tone(sTone, ramp)); p5_quad(E, Ey, p5_tone(eTone, ramp));
      int nz = z / (mesh + 1); if (nz > 20) nz = 20;
      p5_mesh(S, Sy, (x1 - x0) / mesh, nz, fillMode ? lineC : p5_line(sTone, ramp));
      p5_mesh(E, Ey, (y1 - y0) / mesh, nz, fillMode ? lineC : p5_line(eTone, ramp));
      p5_quad(T, Ty, p5_tone(topT, ramp));
      p5_mesh(T, Ty, (x1 - x0) / mesh, (y1 - y0) / mesh, fillMode ? lineC : p5_line(topT, ramp));
      p5_outline(T, Ty, fillMode ? lineC : p5_line(topT - 20, ramp));
    }
  }
}
void prog_greeble_draw(int preset) { greeble5_draw(preset); fx5_apply(); }

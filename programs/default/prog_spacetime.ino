// =====================================================================
// PROGRAM: SPACETIME — slices of curved space, stacked through time (by Dewey) — Set 3
// A little solar system runs live: a sun and planets pulling on each other,
// plus a small extra pull close in that makes the orbits slowly swing round
// (the way relativity makes Mercury's orbit precess). Space is drawn as a
// wireframe sheet that every body dents into a gravity well. Several sheets
// are stacked one above the other: the top one is NOW, each one below is
// the same space a moment earlier — so the stack is spacetime, cut into
// slices, and the bodies' paths thread down through it as worldlines.
// The camera swings round the stack, tilts and dives in.
//
// Inspired by The Reason Room's "ADM 3+1: A Different Way to Visualize
// Spacetime" (https://www.instagram.com/reels/DdthoZUzaeO/) — an N-body
// solar system with a relativistic correction, shown as stacked spatial
// slices. The code here is an original, much simpler take on that look:
// Newtonian gravity plus a precession term, not the real ADM equations.
//
// Presets:
//   k0 Foliation   — the solar system, eight slices of time, round grids
//   k1 Rubber Sheet— one big sheet, square grid, the classic picture
//   k2 Binary      — two suns dancing, a planet circling both
//   k3 Precession  — one eccentric planet; its orbit swings into a rosette
//   k4 Tower       — twelve slices seen from the side, worldlines bright
//   k5 Flyby       — a rogue star crashes through every so often
//   k6 Figure Eight— three equal stars chasing round a figure of eight
//   k7 Ripples     — two stars spiral together, sending out gravitational waves
//   k8 Lattice     — square grids from above, neon colours
//   k9 Black Hole  — one deep well; a disc of matter spirals in
//   k10 Orrery     — the camera flies in to each planet in turn
//   k11 Cosmos     — tours all of the above
//
//   k12 — FLYBY: send a rogue star through the system now
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette      p1 Speed (far left = freeze time; the camera still moves)
//   p2 Slices (1 … 12)   p3 Slice Gap   p4 Grid Density   p5 Well Depth
//   p6 Spin (camera; left = backwards)   p7 Tilt (left = side on, right = from above)
//   p8 Zoom         p9 Time Between Slices
//   p10 Grid Style (square / round / contours / starburst / triangles; centre = preset)
//   p11 Trails (and worldlines)
//   p12 Colour Mode (by depth / by time / in bands; centre = preset)  p13 Relativity (how fast orbits precess)
//   p14 Sun Mass (heavier or lighter than the orbits expect: they go eccentric)
//   p15 Brightness
// Every knob at either end still shows a picture: the sheets are always there.
// =====================================================================

#include "vg_sin.h"
static inline float sp_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int sp_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static uint32_t sp_rng = 4242u;
static inline uint32_t sp_rand() { sp_rng = sp_rng * 1664525u + 1013904223u; return sp_rng >> 8; }
static inline float sp_randf() { return sp_rand() * (1.0f / 16777216.0f); }
static inline float sp_smooth(float u) { u = sp_clampf(u, 0, 1); return u * u * (3 - 2 * u); }
static inline float sp_sinT(int a) { return vg_sin1024[a & 1023] * (1.0f / 16384.0f); }   // a in 1/1024 turns
static inline float sp_cosT(int a) { return vg_sin1024[(a + 256) & 1023] * (1.0f / 16384.0f); }
static inline int sp_atan(float dy, float dx) {                    // 0 … 1023 = one turn
  float ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax < 1e-6f && ay < 1e-6f) return 0;
  float z = ax > ay ? ay / ax : ax / ay;
  float a = z * (128.0f + 44.5f * (1.0f - z));
  if (ay > ax) a = 256.0f - a;
  if (dx < 0) a = 512.0f - a;
  if (dy < 0) a = 1024.0f - a;
  return ((int)a) & 1023;
}

// ─── Colours ──────────────────────────────────────────────────────────
// hues 1-4: the sheet's four contrasting colours (flat … deep, or by time, or in bands),
// 5 stars, 6 disc matter, 7 … 13 bodies
#define SP_NPAL 8
#define HP_G0 1
#define HP_STAR 5
#define HP_PART 6
#define HP_BODY 7
static const uint8_t sp_pal[SP_NPAL][14][3] = {
  {{0,0,0},{215,225,250},{0,200,255},{255,120,0},{255,30,80},{140,140,170},{255,200,80},
   {255,90,20},{0,230,255},{140,255,60},{255,60,220},{255,230,40},{160,110,255},{255,255,255}},    // 0 Classic: white, cyan, orange, red
  {{0,0,0},{0,255,170},{0,130,255},{255,0,200},{255,240,0},{90,120,160},{255,80,200},
   {255,240,0},{255,50,50},{0,200,255},{255,255,255},{255,120,0},{140,255,60},{220,90,255}},     // 1 Neon: mint, blue, magenta, yellow
  {{0,0,0},{60,110,255},{150,210,255},{255,255,255},{255,190,0},{90,110,170},{255,220,120},
   {255,200,0},{255,70,60},{0,255,200},{255,110,255},{255,255,255},{120,200,255},{255,150,40}},  // 2 Blueprint: blue, sky, white, gold
  {{0,0,0},{255,50,20},{255,150,0},{255,240,80},{255,255,255},{160,90,70},{255,230,150},
   {255,255,220},{0,220,255},{60,120,255},{170,255,255},{120,255,180},{200,160,255},{0,255,140}}, // 3 Infrared: red, orange, yellow, white
  {{0,0,0},{40,255,110},{0,200,220},{120,80,255},{255,60,200},{80,140,120},{150,255,220},
   {255,255,140},{255,80,160},{0,200,255},{255,180,40},{255,255,255},{255,60,60},{180,120,255}}, // 4 Aurora: green, teal, violet, pink
  {{0,0,0},{200,200,200},{255,255,255},{255,80,80},{255,0,0},{120,120,120},{255,120,120},
   {255,40,40},{255,255,255},{180,180,180},{255,150,150},{220,220,220},{255,90,90},{160,160,160}}, // 5 Mono + red
  {{0,0,0},{255,40,200},{140,40,255},{0,220,255},{255,220,60},{110,80,160},{255,160,220},
   {255,220,60},{0,255,200},{255,90,40},{255,255,255},{80,140,255},{255,40,120},{170,255,80}},   // 6 Synthwave: pink, purple, cyan, gold
  {{0,0,0},{170,255,0},{0,255,120},{255,0,90},{255,255,255},{80,130,60},{220,255,120},
   {255,0,90},{255,255,255},{0,200,255},{255,200,0},{190,80,255},{0,255,160},{255,120,40}},     // 7 Toxic: lime, green, crimson, white
};
static int16_t sp_levK[16];
static void sp_buildPalette(int pal, float bright) {
  int b = (int)(sp_clampf(bright, 0, 1.4f) * 256);
  for (int h = 0; h < 16; h++) {
    int r = 0, g = 0, bl = 0;
    if (h < 14) { r = sp_pal[pal][h][0]; g = sp_pal[pal][h][1]; bl = sp_pal[pal][h][2]; }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = sp_levK[l] * b >> 8;
      display.setColor((l << 4) | h, sp_clamp(r * k >> 8, 0, 255), sp_clamp(g * k >> 8, 0, 255), sp_clamp(bl * k >> 8, 0, 255));
    }
  }
}

// ─── Drawing primitives ───────────────────────────────────────────────
static uint8_t* sp_buf;
static inline void sp_put(int x, int y, int lev, int hue) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || lev <= 0) return;
  if (lev > 15) lev = 15;
  uint8_t* p = sp_buf + y * W + x;
  if (lev > (*p >> 4)) *p = (uint8_t)((lev << 4) | hue);       // brightest wins
}
static void sp_line(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (lev <= 0) return;
  if (lev > 15) lev = 15;
  if ((unsigned)x0 < (unsigned)W && (unsigned)x1 < (unsigned)W && (unsigned)y0 < (unsigned)H && (unsigned)y1 < (unsigned)H) {
    // fast path, the whole line on screen: walk a pointer, no bounds checks
    uint8_t c = (uint8_t)((lev << 4) | hue);
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? W : -W;
    uint8_t* p = sp_buf + y0 * W + x0;
    if (dx >= dy) {
      int err = dx >> 1;
      for (int n = 0; n <= dx; n++) { if (c > *p) *p = c; p += sx; err -= dy; if (err < 0) { err += dx; p += sy; } }
    } else {
      int err = dy >> 1;
      for (int n = 0; n <= dy; n++) { if (c > *p) *p = c; p += sy; err -= dx; if (err < 0) { err += dy; p += sx; } }
    }
    return;
  }
  if ((x0 < 0 && x1 < 0) || (y0 < 0 && y1 < 0) || (x0 >= W && x1 >= W) || (y0 >= H && y1 >= H)) return;
  if (x0 < -2000 || x0 > 2000 || x1 < -2000 || x1 > 2000 || y0 < -2000 || y0 > 2000 || y1 < -2000 || y1 > 2000) return;
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
  int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int n = 0; n < 4000; n++) {
    sp_put(x0, y0, lev, hue);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
// an anti-aliased line between points given in 1/8 pixels (Wu's method): the line's
// brightness is shared between the two pixels it passes between, so it reads finer than a pixel.
// Tight inner loop: the position steps by addition, and pixels are written in place.
static inline void sp_put2(uint8_t* p, int lev, int hue) {       // brightest-wins, no bounds check
  if (lev > (*p >> 4)) *p = (uint8_t)((lev << 4) | hue);
}
static void sp_lineAA(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (lev <= 0) return;
  if (lev > 15) lev = 15;
  if ((x0 < 0 && x1 < 0) || (y0 < 0 && y1 < 0) || (x0 >= W * 8 && x1 >= W * 8) || (y0 >= H * 8 && y1 >= H * 8)) return;
  int dx = x1 - x0, dy = y1 - y0;
  int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  if (adx > 8 * 1200 || ady > 8 * 1200) return;
  if (adx < 8 && ady < 8) { sp_put((x0 + 4) >> 3, (y0 + 4) >> 3, lev, hue); return; }
  if (adx >= ady) {                                               // mostly horizontal: step x
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dy = -dy; }
    int32_t grad = (int32_t)dy * 65536 / (x1 - x0);              // y change per 1/8 px of x, ×65536
    int xs = (x0 + 7) >> 3, xe = x1 >> 3;
    if (xs < 0) xs = 0; if (xe > W - 1) xe = W - 1;
    if (xs > xe) return;
    int32_t y = y0 * 65536 + (int32_t)(xs * 8 - x0) * grad;      // 1/8 px × 65536
    int32_t step = grad * 8;
    for (int x = xs; x <= xe; x++, y += step) {
      int yy = (int)(y >> 16);
      int yi = yy >> 3, f = yy & 7;
      if ((unsigned)yi < (unsigned)(H - 1)) {                      // both pixels on screen (the common case)
        uint8_t* p = sp_buf + yi * W + x;
        sp_put2(p, (lev * (8 - f) + 4) >> 3, hue);
        if (f) sp_put2(p + W, (lev * f + 4) >> 3, hue);
      } else if (yi == H - 1) sp_put(x, yi, (lev * (8 - f) + 4) >> 3, hue);
      else if (yi == -1 && f) sp_put(x, 0, (lev * f + 4) >> 3, hue);
    }
  } else {                                                        // mostly vertical: step y
    if (y1 < y0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dx = -dx; }
    int32_t grad = (int32_t)dx * 65536 / (y1 - y0);
    int ys = (y0 + 7) >> 3, ye = y1 >> 3;
    if (ys < 0) ys = 0; if (ye > H - 1) ye = H - 1;
    if (ys > ye) return;
    int32_t x = x0 * 65536 + (int32_t)(ys * 8 - y0) * grad;
    int32_t step = grad * 8;
    uint8_t* row = sp_buf + ys * W;
    for (int y = ys; y <= ye; y++, x += step, row += W) {
      int xx = (int)(x >> 16);
      int xi = xx >> 3, f = xx & 7;
      if ((unsigned)xi < (unsigned)(W - 1)) {
        uint8_t* p = row + xi;
        sp_put2(p, (lev * (8 - f) + 4) >> 3, hue);
        if (f) sp_put2(p + 1, (lev * f + 4) >> 3, hue);
      } else if (xi == W - 1) sp_put2(row + xi, (lev * (8 - f) + 4) >> 3, hue);
      else if (xi == -1 && f) sp_put2(row, (lev * f + 4) >> 3, hue);
    }
  }
}
static void sp_disc(int cx, int cy, float rad, int lev, int hue) {
  if (rad < 0.8f) { sp_put(cx, cy, lev, hue); return; }
  if (rad > 40) rad = 40;
  int R = (int)(rad + 0.999f);
  int r2 = (int)(rad * rad * 16.0f), in = (int)((rad - 1.0f) * (rad - 1.0f) * 16.0f);
  for (int dy = -R; dy <= R; dy++) for (int dx = -R; dx <= R; dx++) {
    int d2 = (dx * dx + dy * dy) * 16;
    if (d2 > r2) continue;
    // a bright centre fading to the rim, so each body reads as a ball
    int l = d2 <= in ? lev - (d2 * 3 / (r2 + 1)) : lev - 5;
    sp_put(cx + dx, cy + dy, l, hue);
  }
}

// ─── The bodies ───────────────────────────────────────────────────────
#define SP_NB 7                                    // up to 6 bodies + the rogue star (last)
#define SP_ROGUE (SP_NB - 1)
static float sp_bx[SP_NB], sp_by[SP_NB], sp_vx[SP_NB], sp_vy[SP_NB], sp_m[SP_NB];
static float sp_wm[SP_NB];                         // how deep each body's well is drawn (planets exaggerated)
static float sp_size[SP_NB];                       // drawn radius (world units)
static uint8_t sp_on[SP_NB];
static float sp_r0[SP_NB];                         // a planet's home orbit radius (0 = a star, never re-placed)
static int sp_nb = 0;
static float sp_eps = 0.03f;                       // softening
static float sp_alpha = 0.0f;                      // the precession term
static float sp_alphaBase = 0.0f;                  // the scene's own amount (Relativity knob scales it)
static float sp_massMul = 1.0f;
// history: every SP_HS sim-seconds, where everyone was (Q13 fixed point; 3.99 = not there)
#define SP_HN 240
#define SP_HS 0.05f
#define SP_WARM 100                        // samples run ahead when a scene starts (5 s)
static int16_t sp_hx[SP_HN][SP_NB], sp_hy[SP_HN][SP_NB];
static int sp_hHead = 0, sp_hFill = 0;
static float sp_simT = 0, sp_nextSample = 0;
// disc matter for the black hole
#define SP_NP 48
static float sp_px[SP_NP], sp_py[SP_NP], sp_pvx[SP_NP], sp_pvy[SP_NP];
static int sp_np = 0;
static float sp_drag = 0;
// gravitational-wave binary (driven, not integrated)
static bool sp_gw = false;
static float sp_gwD = 0.35f, sp_gwPhi = 0, sp_gwOmega = 0, sp_gwT = 0, sp_ring = 0;
static int sp_gwStage = 0;                         // 0 inspiral, 1 ringdown
// the rogue star
static float sp_flybyEvery = 0, sp_flybyT = 0;
static int sp_scn = 0;

static void sp_setCircular(int k, float r, float ang, float M, float dir) {
  sp_r0[k] = r;
  sp_bx[k] = r * cosf(ang); sp_by[k] = r * sinf(ang);
  float v = sqrtf(M / r) * dir;
  sp_vx[k] = -sinf(ang) * v; sp_vy[k] = cosf(ang) * v;
}
static void sp_launchRogue() {
  int k = SP_ROGUE;
  float a = sp_randf() * 6.2831853f;
  sp_bx[k] = 1.7f * cosf(a); sp_by[k] = 1.7f * sinf(a);
  float b = (sp_randf() - 0.5f) * 0.9f;                           // aim to one side of the centre
  float tx = -sinf(a) * b, ty = cosf(a) * b;
  float dx = tx - sp_bx[k], dy = ty - sp_by[k], d = sqrtf(dx * dx + dy * dy);
  float sp = 1.4f + sp_randf() * 0.5f;
  sp_vx[k] = dx / d * sp; sp_vy[k] = dy / d * sp;
  sp_m[k] = 0.18f; sp_wm[k] = 0.7f; sp_size[k] = 0.075f; sp_on[k] = 1;
}

static void sp_accel(int k, float x, float y, float* ax, float* ay) {
  float sx = 0, sy = 0;
  for (int j = 0; j < SP_NB; j++) {
    if (j == k || !sp_on[j] || sp_m[j] <= 0) continue;
    float dx = sp_bx[j] - x, dy = sp_by[j] - y;
    float r2 = dx * dx + dy * dy + sp_eps * sp_eps;
    float r = sqrtf(r2);
    float mj = sp_m[j] * (j == 0 ? sp_massMul : 1.0f);
    float f = mj / (r2 * r);
    if (j == 0 && sp_alpha > 0 && k >= 0) {                       // the precession term, round the main body only
      float hx = (x - sp_bx[0]), hy = (y - sp_by[0]);
      float vx = sp_vx[k] - sp_vx[0], vy = sp_vy[k] - sp_vy[0];
      float h = hx * vy - hy * vx;
      float g = sp_alpha * h * h / r2;
      f *= 1.0f + (g > 0.6f ? 0.6f : g);                          // capped, so a close pass can't fling anything out
    }
    sx += dx * f; sy += dy * f;
  }
  *ax = sx; *ay = sy;
}
static void sp_sample() {
  sp_hHead = (sp_hHead + 1) % SP_HN;
  if (sp_hFill < SP_HN) sp_hFill++;
  for (int k = 0; k < SP_NB; k++) {
    float x = sp_on[k] ? sp_clampf(sp_bx[k], -3.9f, 3.9f) : 3.99f, y = sp_on[k] ? sp_clampf(sp_by[k], -3.9f, 3.9f) : 3.99f;
    sp_hx[sp_hHead][k] = (int16_t)(x * 8192.0f); sp_hy[sp_hHead][k] = (int16_t)(y * 8192.0f);
  }
}
static void sp_gwPlace() {
  float d = sp_gwD * 0.5f;
  sp_bx[0] = d * cosf(sp_gwPhi); sp_by[0] = d * sinf(sp_gwPhi);
  sp_bx[1] = -sp_bx[0]; sp_by[1] = -sp_by[0];
}
static void sp_step(float h) {
  if (sp_gw) {                                                    // two stars spiralling together, then merged and ringing
    sp_gwT += h;
    if (sp_gwStage == 0) {
      sp_gwD = 0.38f * powf(sp_clampf(1.0f - sp_gwT / 16.0f, 0.0f, 1.0f), 0.25f);
      if (sp_gwD < 0.05f) { sp_gwStage = 1; sp_gwT = 0; sp_on[1] = 0; sp_m[0] = 1.0f; sp_wm[0] = 1.4f; sp_size[0] = 0.09f; sp_bx[0] = sp_by[0] = 0; sp_ring = 1.0f; }
      else { sp_gwOmega = sqrtf(1.0f / (sp_gwD * sp_gwD * sp_gwD)); sp_gwPhi += sp_gwOmega * h; sp_gwPlace(); }
    } else {
      sp_ring *= expf(-h * 0.5f);
      sp_gwPhi += sp_gwOmega * h;
      if (sp_gwT > 6.0f) {                                         // start again
        sp_gwStage = 0; sp_gwT = 0; sp_on[1] = 1; sp_m[0] = sp_m[1] = 0.5f; sp_wm[0] = sp_wm[1] = 0.8f; sp_size[0] = sp_size[1] = 0.065f; sp_ring = 0;
      }
    }
  }
  // leapfrog (kick-drift-kick) for everything that moves freely
  float ax[SP_NB], ay[SP_NB];
  int first = sp_gw ? 2 : 0;
  for (int k = first; k < SP_NB; k++) if (sp_on[k]) sp_accel(k, sp_bx[k], sp_by[k], &ax[k], &ay[k]);
  for (int k = first; k < SP_NB; k++) if (sp_on[k]) { sp_vx[k] += ax[k] * h * 0.5f; sp_vy[k] += ay[k] * h * 0.5f; sp_bx[k] += sp_vx[k] * h; sp_by[k] += sp_vy[k] * h; }
  for (int k = first; k < SP_NB; k++) if (sp_on[k]) { sp_accel(k, sp_bx[k], sp_by[k], &ax[k], &ay[k]); sp_vx[k] += ax[k] * h * 0.5f; sp_vy[k] += ay[k] * h * 0.5f; }
  // a planet thrown off the sheet comes back on a fresh orbit round the centre
  for (int k = 1; k < SP_ROGUE; k++) {
    if (!sp_on[k] || sp_r0[k] <= 0) continue;
    if (sp_bx[k] * sp_bx[k] + sp_by[k] * sp_by[k] > 2.6f) sp_setCircular(k, sp_r0[k], sp_randf() * 6.2831853f, 1.0f, 1.0f);
  }
  // after a flyby the system keeps a little drift: glide its centre of mass gently back home
  if (!sp_on[SP_ROGUE] && !sp_gw) {
    float M = 0, px = 0, py = 0, cx = 0, cy = 0;
    for (int k = 0; k < SP_ROGUE; k++) if (sp_on[k]) { M += sp_m[k]; px += sp_m[k] * sp_vx[k]; py += sp_m[k] * sp_vy[k]; cx += sp_m[k] * sp_bx[k]; cy += sp_m[k] * sp_by[k]; }
    if (M > 0) {
      float c = 0.6f * h;
      for (int k = 0; k < SP_ROGUE; k++) if (sp_on[k]) { sp_vx[k] -= px / M; sp_vy[k] -= py / M; sp_bx[k] -= cx / M * c; sp_by[k] -= cy / M * c; }
    }
  }
  // the rogue leaves once it is far away
  if (sp_on[SP_ROGUE] && (sp_bx[SP_ROGUE] * sp_bx[SP_ROGUE] + sp_by[SP_ROGUE] * sp_by[SP_ROGUE]) > 6.0f) sp_on[SP_ROGUE] = 0;
}
static void sp_stepParticles(float h) {
  for (int i = 0; i < sp_np; i++) {
    float ax, ay;
    sp_accel(-1, sp_px[i], sp_py[i], &ax, &ay);
    sp_pvx[i] += ax * h - sp_pvx[i] * sp_drag * h; sp_pvy[i] += ay * h - sp_pvy[i] * sp_drag * h;
    sp_px[i] += sp_pvx[i] * h; sp_py[i] += sp_pvy[i] * h;
    float r2 = sp_px[i] * sp_px[i] + sp_py[i] * sp_py[i];
    if (r2 < 0.006f || r2 > 2.5f) {                               // swallowed (or flung out): a new grain at the rim
      float a = sp_randf() * 6.2831853f, r = 0.75f + sp_randf() * 0.3f;
      sp_px[i] = r * cosf(a); sp_py[i] = r * sinf(a);
      float v = sqrtf(sp_m[0] * sp_massMul / r);
      sp_pvx[i] = -sinf(a) * v; sp_pvy[i] = cosf(a) * v;
    }
  }
}
// healthy? (anything gone wild resets the scene)
static bool sp_sane() {
  for (int k = 0; k < SP_NB; k++) {
    if (!sp_on[k]) continue;
    float x = sp_bx[k], y = sp_by[k];
    if (!(x > -50 && x < 50 && y > -50 && y < 50)) return false;
    if (k != SP_ROGUE && x * x + y * y > 9.0f) return false;      // a planet thrown out of the system
  }
  return true;
}
static float sp_maxH = 0.004f;                     // largest physics step (coarser while warming up)
static void sp_advance(float dt, bool particles) {
  if (dt <= 0) return;
  int n = (int)(dt / sp_maxH) + 1;
  if (n > 40) n = 40;
  float h = dt / n;
  for (int s = 0; s < n; s++) {
    sp_step(h);
    if (particles) sp_stepParticles(h);
    sp_simT += h;
    if (sp_simT >= sp_nextSample) { sp_sample(); sp_nextSample += SP_HS; }
  }
  if (sp_flybyEvery > 0) { sp_flybyT += dt; if (sp_flybyT > sp_flybyEvery && !sp_on[SP_ROGUE]) { sp_flybyT = 0; sp_launchRogue(); } }
}

// ─── Scenes ───────────────────────────────────────────────────────────
#define SC_SOLAR 0
#define SC_BINARY 1
#define SC_PRECESS 2
#define SC_EIGHT 3
#define SC_RIPPLES 4
#define SC_HOLE 5
static void sp_setup(int scene, bool flyby) {
  for (int k = 0; k < SP_NB; k++) { sp_r0[k] = 0; sp_on[k] = 0; sp_m[k] = 0; sp_wm[k] = 0; sp_size[k] = 0.03f; sp_bx[k] = sp_by[k] = sp_vx[k] = sp_vy[k] = 0; }
  sp_gw = false; sp_np = 0; sp_drag = 0; sp_eps = 0.03f; sp_alpha = 0.03f;
  sp_flybyEvery = flyby ? 18.0f : 0.0f; sp_flybyT = flyby ? 12.0f : 0.0f;
  float a0 = sp_randf() * 6.2831853f;
  if (scene == SC_SOLAR) {
    sp_on[0] = 1; sp_m[0] = 1.0f; sp_wm[0] = 1.0f; sp_size[0] = 0.085f;
    static const float rr[5] = {0.24f, 0.38f, 0.54f, 0.71f, 0.9f};
    static const float mm[5] = {0.0003f, 0.0008f, 0.001f, 0.002f, 0.0015f};
    static const float ww[5] = {0.10f, 0.14f, 0.15f, 0.24f, 0.2f};
    static const float ss[5] = {0.025f, 0.033f, 0.035f, 0.05f, 0.045f};
    for (int i = 0; i < 5; i++) { int k = i + 1; sp_on[k] = 1; sp_m[k] = mm[i]; sp_wm[k] = ww[i]; sp_size[k] = ss[i]; sp_setCircular(k, rr[i], a0 + i * 2.4f, 1.0f, 1.0f); }
  } else if (scene == SC_BINARY) {
    for (int k = 0; k < 2; k++) { sp_on[k] = 1; sp_m[k] = 0.5f; sp_wm[k] = 0.7f; sp_size[k] = 0.065f; }
    float d = 0.3f, v = 0.5f * sqrtf(1.0f / d);
    sp_bx[0] = d * 0.5f * cosf(a0); sp_by[0] = d * 0.5f * sinf(a0);
    sp_bx[1] = -sp_bx[0]; sp_by[1] = -sp_by[0];
    sp_vx[0] = -sinf(a0) * v; sp_vy[0] = cosf(a0) * v; sp_vx[1] = -sp_vx[0]; sp_vy[1] = -sp_vy[0];
    sp_on[2] = 1; sp_m[2] = 0.0005f; sp_wm[2] = 0.18f; sp_size[2] = 0.04f; sp_setCircular(2, 0.9f, a0 + 1.0f, 1.0f, 1.0f);
    sp_on[3] = 1; sp_m[3] = 0.0003f; sp_wm[3] = 0.12f; sp_size[3] = 0.03f; sp_setCircular(3, 1.06f, a0 + 3.0f, 1.0f, 1.0f);
    sp_alpha = 0;
  } else if (scene == SC_PRECESS) {
    sp_on[0] = 1; sp_m[0] = 1.0f; sp_wm[0] = 1.0f; sp_size[0] = 0.08f;
    sp_on[1] = 1; sp_m[1] = 0.0003f; sp_wm[1] = 0.16f; sp_size[1] = 0.04f;
    float e = 0.5f, ra = 0.85f, a = ra / (1 + e), v = sqrtf((1 - e) / (a * (1 + e)));
    sp_bx[1] = ra * cosf(a0); sp_by[1] = ra * sinf(a0); sp_vx[1] = -sinf(a0) * v; sp_vy[1] = cosf(a0) * v;
    sp_on[2] = 1; sp_m[2] = 0.0002f; sp_wm[2] = 0.1f; sp_size[2] = 0.028f;
    e = 0.35f; ra = 0.5f; a = ra / (1 + e); v = sqrtf((1 - e) / (a * (1 + e)));
    sp_bx[2] = ra * cosf(a0 + 2.5f); sp_by[2] = ra * sinf(a0 + 2.5f); sp_vx[2] = -sinf(a0 + 2.5f) * v; sp_vy[2] = cosf(a0 + 2.5f) * v;
    sp_alpha = 0.04f; sp_eps = 0.02f;
  } else if (scene == SC_EIGHT) {                                 // Chenciner–Montgomery figure-eight, scaled
    const float s = 0.62f, vs = 1.0f / sqrtf(s);
    for (int k = 0; k < 3; k++) { sp_on[k] = 1; sp_m[k] = 1.0f; sp_wm[k] = 0.55f; sp_size[k] = 0.06f; }
    sp_bx[0] = 0.97000436f * s; sp_by[0] = -0.24308753f * s; sp_bx[1] = -sp_bx[0]; sp_by[1] = -sp_by[0];
    sp_vx[2] = -0.93240737f * vs; sp_vy[2] = -0.86473146f * vs;
    sp_vx[0] = sp_vx[1] = -sp_vx[2] * 0.5f; sp_vy[0] = sp_vy[1] = -sp_vy[2] * 0.5f;
    sp_eps = 0.005f; sp_alpha = 0;
  } else if (scene == SC_RIPPLES) {
    sp_gw = true; sp_gwStage = 0; sp_gwT = 0; sp_gwPhi = a0; sp_ring = 0;
    for (int k = 0; k < 2; k++) { sp_on[k] = 1; sp_m[k] = 0.5f; sp_wm[k] = 0.8f; sp_size[k] = 0.065f; }
    sp_gwD = 0.38f; sp_gwOmega = sqrtf(1.0f / (sp_gwD * sp_gwD * sp_gwD)); sp_gwPlace();
    sp_on[2] = 1; sp_m[2] = 0.0003f; sp_wm[2] = 0.14f; sp_size[2] = 0.035f; sp_setCircular(2, 0.9f, a0 + 2.0f, 1.0f, 1.0f);
    sp_alpha = 0;
  } else {                                                        // SC_HOLE
    sp_on[0] = 1; sp_m[0] = 1.0f; sp_wm[0] = 2.6f; sp_size[0] = 0.05f;
    sp_eps = 0.04f; sp_alpha = 0.1f; sp_drag = 0.05f;
    sp_on[1] = 1; sp_m[1] = 0.0003f; sp_wm[1] = 0.15f; sp_size[1] = 0.035f; sp_setCircular(1, 0.88f, a0, 1.0f, 1.0f);
    sp_np = SP_NP;
    for (int i = 0; i < sp_np; i++) {
      float a = sp_randf() * 6.2831853f, r = 0.2f + sp_randf() * 0.85f, v = sqrtf(1.0f / r);
      sp_px[i] = r * cosf(a); sp_py[i] = r * sinf(a); sp_pvx[i] = -sinf(a) * v; sp_pvy[i] = cosf(a) * v;
    }
  }
  if (!sp_gw) {                                                   // centre of mass at rest at the origin, so nothing drifts away
    float M = 0, px = 0, py = 0, cx = 0, cy = 0;
    for (int k = 0; k < SP_NB; k++) if (sp_on[k]) { M += sp_m[k]; px += sp_m[k] * sp_vx[k]; py += sp_m[k] * sp_vy[k]; cx += sp_m[k] * sp_bx[k]; cy += sp_m[k] * sp_by[k]; }
    if (M > 0) for (int k = 0; k < SP_NB; k++) if (sp_on[k]) { sp_vx[k] -= px / M; sp_vy[k] -= py / M; sp_bx[k] -= cx / M; sp_by[k] -= cy / M; }
  }
  sp_scn = scene;
  sp_alphaBase = sp_alpha;
  // fill the history by running the scene forward, so every slice has a past from the first frame
  sp_hHead = 0; sp_hFill = 0; sp_simT = 0; sp_nextSample = 0;
  // (coarse steps, and only as far back as the slices need: on the RP2040 a full-length warm-up takes most of a
  //  second, and the platform's 500 ms watchdog would reset the board. The rest of the history fills in as it runs.)
  float savedFly = sp_flybyEvery; sp_flybyEvery = 0;
  sp_maxH = 0.0125f;
  for (int i = 0; i < SP_WARM; i++) { sp_advance(SP_HS, false); if ((i & 7) == 7) watchdog_update(); }
  sp_maxH = 0.004f;
  sp_flybyEvery = savedFly;
}
// where body k was `back` sim-seconds ago (interpolated); false if it wasn't there
static bool sp_past(int k, float back, float* x, float* y) {
  if (back <= 0.0001f) { if (!sp_on[k]) return false; *x = sp_bx[k]; *y = sp_by[k]; return true; }
  float since = sp_simT - (sp_nextSample - SP_HS);                // time since the newest sample
  float s = (back - since) / SP_HS;
  if (s < 0) {                                                    // between the newest sample and now
    float u = back / (since > 1e-4f ? since : 1e-4f);
    int16_t hx = sp_hx[sp_hHead][k], hy = sp_hy[sp_hHead][k];
    if (hx >= 32600 || !sp_on[k]) return false;
    float sx = hx / 8192.0f, sy = hy / 8192.0f;
    *x = sp_bx[k] + (sx - sp_bx[k]) * u; *y = sp_by[k] + (sy - sp_by[k]) * u;
    return true;
  }
  int i0 = (int)s;
  float f = s - i0;
  if (i0 >= sp_hFill - 1) { i0 = sp_hFill - 2; f = 1.0f; }
  if (i0 < 0) return false;
  int a = (sp_hHead - i0 + SP_HN * 4) % SP_HN, b = (sp_hHead - i0 - 1 + SP_HN * 4) % SP_HN;
  if (sp_hx[a][k] >= 32600 || sp_hx[b][k] >= 32600) return false;
  float ax = sp_hx[a][k] / 8192.0f, ay = sp_hy[a][k] / 8192.0f, bx = sp_hx[b][k] / 8192.0f, by = sp_hy[b][k] / 8192.0f;
  *x = ax + (bx - ax) * f; *y = ay + (by - ay) * f;
  return true;
}

// ─── Camera ───────────────────────────────────────────────────────────
static float sp_yaw = 0, sp_tilt = 0.5f, sp_zoom = 1.0f, sp_camX = 0, sp_camZ = 0;
static float sp_cy, sp_sy, sp_ct, sp_st;
static inline void sp_project(float x, float y, float z, int* sx, int* sy, float* depth) {
  x -= sp_camX; z -= sp_camZ;
  float x1 = x * sp_cy + z * sp_sy, z1 = -x * sp_sy + z * sp_cy;
  float yv = y * sp_ct + z1 * sp_st;                              // screen up
  float zc = -y * sp_st + z1 * sp_ct;                             // depth (bigger = further)
  const float dist = 3.2f;
  float s = dist / (dist + zc);
  if (s < 0.05f) s = 0.05f;
  float k = 96.0f * sp_zoom * s;
  *sx = (int)(W * 0.5f + x1 * k);
  *sy = (int)(H * 0.5f - yv * k);
  *depth = zc;
}
static inline int sp_levFor(float depth, int base) { return sp_clamp(base - (int)((depth + 0.6f) * 2.6f), 4, 15); }

// ─── Knobs ────────────────────────────────────────────────────────────
static float sp_sm[16];
static bool sp_smReady = false;
static inline int sp_k(int i) { int v = (int)sp_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
static inline bool sp_centre(int i) { int k = sp_k(i); return k >= 440 && k < 600; }
static inline float sp_bi(int i) { return (sp_k(i) - 512) / 512.0f; }
static inline float sp_rel(int i, float own, float range) { return own * powf(range, sp_bi(i)); }
static inline int sp_choice(int i, int own, int n) {             // centre band = own; n choices spread either side
  int k = sp_k(i);
  if (k >= 440 && k < 600) return own;
  int left = (n + 1) / 2;
  if (k < 440) return sp_clamp(k * left / 440, 0, left - 1);
  return sp_clamp(left + (k - 600) * (n - left) / 424, left, n - 1);
}

// ─── Presets ──────────────────────────────────────────────────────────
const char* prog_spacetime_name() { return "SPACETIME"; }
const char* prog_spacetime_character() { return "Spacetime slices: a live solar system dents stacked wireframe sheets, one per moment"; }
static const char* const sp_presetNames[] = {
  "Foliation", "Rubber Sheet", "Binary", "Precession", "Tower", "Flyby",
  "Figure Eight", "Ripples", "Lattice", "Black Hole", "Orrery", "Cosmos"
};
#define SP_NUM_PRESETS 12
// grid styles
#define GS_SQUARE 0
#define GS_ROUND 1
#define GS_CONTOUR 2                                  // rings only: like contour lines on a map
#define GS_STAR 3                                     // spokes only: a starburst
#define GS_TRI 4                                      // square grid with diagonals: a triangle mesh
#define SP_NGS 5
// colour modes
#define CM_DEPTH 0                                    // the four sheet colours by how deep the well is
#define CM_TIME 1                                     // each slice of time its own colour
#define CM_BANDS 2                                    // contour bands of alternating colours down each well
#define SP_NCM 3
//                                                  Fol  Rub  Bin  Pre  Tow  Fly  Ei8  Rip  Lat  BH   Orr  Cos
static const uint8_t sp_pScene[SP_NUM_PRESETS]  = {SC_SOLAR, SC_SOLAR, SC_BINARY, SC_PRECESS, SC_SOLAR, SC_SOLAR, SC_EIGHT, SC_RIPPLES, SC_SOLAR, SC_HOLE, SC_SOLAR, SC_SOLAR};
static const uint8_t sp_pSlices[SP_NUM_PRESETS] = {  8,   1,   6,   3,  12,   6,   6,   2,   5,   2,   4,   8};
static const uint8_t sp_pGap[SP_NUM_PRESETS]    = { 12,  20,  14,  20,   8,  14,  14,  30,  18,  26,  18,  12};   // ×0.01
static const uint8_t sp_pGrid[SP_NUM_PRESETS]   = {GS_ROUND, GS_TRI, GS_ROUND, GS_CONTOUR, GS_ROUND, GS_SQUARE, GS_STAR, GS_SQUARE, GS_TRI, GS_CONTOUR, GS_ROUND, GS_ROUND};
static const uint8_t sp_pColour[SP_NUM_PRESETS] = {CM_TIME, CM_BANDS, CM_DEPTH, CM_BANDS, CM_TIME, CM_DEPTH, CM_BANDS, CM_BANDS, CM_TIME, CM_DEPTH, CM_DEPTH, CM_TIME};
static const uint8_t sp_pDens[SP_NUM_PRESETS]   = { 55,  70,  55,  60,  40,  55,  60,  80,  60,  65,  55,  55};   // %
static const uint8_t sp_pTilt[SP_NUM_PRESETS]   = { 24,  48,  32,  70,  10,  30,  40,  45,  68,  42,  30,  26};   // degrees
static const int8_t  sp_pSpin[SP_NUM_PRESETS]   = {  6,   5,   8,   4,   7,   6,   5,   6,   3,   8,   4,   6};   // ×0.01 turns/s
static const uint8_t sp_pZoom[SP_NUM_PRESETS]   = {100, 120, 100, 110,  95, 100, 100, 115, 100, 115, 100, 100};   // %
static const uint8_t sp_pLag[SP_NUM_PRESETS]    = { 25,  25,  20,  60,  18,  25,  20,  30,  30,  25,  40,  25};   // ×0.01 s between slices
static const uint8_t sp_pTrail[SP_NUM_PRESETS]  = { 40,  60,  50, 150,  20,  50,  60,  40,  50,  30,  50,  40};   // samples
static const uint8_t sp_pPal[SP_NUM_PRESETS]    = {  0,   6,   2,   3,   4,   1,   7,   1,   6,   3,   4,   0};
static const uint8_t sp_pDepth[SP_NUM_PRESETS]  = { 55,  70,  60,  55,  45,  55,  55,  60,  50,  50,  60,  55};   // %

const char* prog_spacetime_presetName(int preset) {
  if (preset >= 0 && preset < SP_NUM_PRESETS) return sp_presetNames[preset];
  return NULL;
}
static const char* const sp_labels[16] = {
  "Palette", "Speed", "Slices", "Slice Gap", "Grid Density", "Well Depth", "Spin", "Tilt",
  "Zoom", "Time Lag", "Grid Style", "Trails", "Colour Mode", "Relativity", "Sun Mass", "Brightness"
};
const char* prog_spacetime_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return sp_labels[pot];
  return "";
}
uint8_t prog_spacetime_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we draw every pixel ourselves each frame
}

// ─── Per-frame working space ──────────────────────────────────────────
#define SP_MAXV 560
static int16_t sp_vxs[SP_MAXV], sp_vys[SP_MAXV];   // projected vertices, 1/8 px
static uint8_t sp_vlev[SP_MAXV], sp_vhue[SP_MAXV];
static float sp_slX[SP_NB], sp_slY[SP_NB];          // body positions for the slice being drawn
static uint8_t sp_slOn[SP_NB];
static int16_t sp_wlX[12][SP_NB], sp_wlY[12][SP_NB]; // worldline points (1/8 px) per slice
static uint8_t sp_wlOk[12][SP_NB];
static int16_t sp_starX[70], sp_starY[70];

// 1 / sqrt(r² + softening²) by table (the RP2040 has no FPU: a square root and a divide per
// grid point per body was the slowest thing in the program). Q11, linear between entries.
#define SP_LUTN 1024
#define SP_LUTR2 8.0f                                  // the table covers r² up to this
static uint16_t sp_inv[SP_LUTN + 1];
static float sp_lutE2 = -1;
static void sp_buildLut(float e2) {
  if (e2 == sp_lutE2) return;
  sp_lutE2 = e2;
  for (int i = 0; i <= SP_LUTN; i++) {
    float r2 = i * (SP_LUTR2 / SP_LUTN);
    float v = 2048.0f / sqrtf(r2 + e2);
    sp_inv[i] = (uint16_t)(v > 65535.0f ? 65535.0f : v);
  }
}
static inline float sp_invR(float r2) {
  float fi = r2 * (SP_LUTN / SP_LUTR2);
  if (fi >= SP_LUTN) return 1.0f / sqrtf(r2 + sp_lutE2);
  int i = (int)fi;
  float f = fi - i;
  return (sp_inv[i] + (sp_inv[i + 1] - sp_inv[i]) * f) * (1.0f / 2048.0f);
}

// the well depth at (x, z) for the current slice (negative = down)
static float sp_depthScale = 0.13f, sp_depthClamp = 0.9f;
static float sp_gwAmp = 0;
static float sp_wk[SP_NB];                         // this slice's well weights (0 = skip)
static inline float sp_well(float x, float z) {
  float phi = 0;
  for (int k = 0; k < SP_NB; k++) {
    if (sp_wk[k] <= 0) continue;
    float dx = x - sp_slX[k], dz = z - sp_slY[k];
    phi += sp_wk[k] * sp_invR(dx * dx + dz * dz);
  }
  float d = -sp_depthScale * phi;
  if (sp_gwAmp > 0.0005f) {                                      // gravitational waves: a two-armed spiral running outward
    float r2 = x * x + z * z;
    float r = r2 * sp_invR(r2);                                  // ≈ r (softened), no square root
    int th = sp_atan(z, x);
    float ph = 2.0f * sp_gwPhi * (1024.0f / 6.2831853f) - r * 2.0f * sp_gwOmega / 1.6f * (1024.0f / 6.2831853f);
    float env = r < 0.12f ? r / 0.12f : 0.25f / (r + 0.13f);
    d += sp_gwAmp * env * sp_cosT(2 * th - (int)ph);
  }
  return d < -sp_depthClamp ? -sp_depthClamp : d;
}
// which of the four sheet colours a point gets
static int sp_cmode = CM_DEPTH, sp_slHue = 0;
static float sp_bandK = 1.0f;
static inline int sp_hueAt(float d) {
  float q = -d / sp_depthScale;                                  // 0 flat … ~8 deep
  if (sp_cmode == CM_TIME) return HP_G0 + ((sp_slHue + (q > 3.5f ? 2 : 0)) & 3);
  if (sp_cmode == CM_BANDS) { int b = (int)(q * sp_bandK); return HP_G0 + (b & 3); }
  return HP_G0 + (q > 6.0f ? 3 : (q > 3.2f ? 2 : (q > 1.9f ? 1 : 0)));
}
// project to 1/8 px
static inline void sp_project8(float x, float y, float z, int* sx, int* sy, float* depth) {
  x -= sp_camX; z -= sp_camZ;
  float x1 = x * sp_cy + z * sp_sy, z1 = -x * sp_sy + z * sp_cy;
  float yv = y * sp_ct + z1 * sp_st;
  float zc = -y * sp_st + z1 * sp_ct;
  const float dist = 3.2f;
  float s = dist / (dist + zc);
  if (s < 0.05f) s = 0.05f;
  float k = 96.0f * 8.0f * sp_zoom * s;
  float fx = W * 4.0f + x1 * k, fy = H * 4.0f - yv * k;
  *sx = (int)sp_clampf(fx, -12000.0f, 12000.0f);
  *sy = (int)sp_clampf(fy, -12000.0f, 12000.0f);
  *depth = zc;
}

void prog_spacetime_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    sp_levK[0] = 0;
    for (int l = 1; l < 16; l++) sp_levK[l] = (int16_t)(256.0f * (0.04f + 0.96f * powf(l / 15.0f, 1.35f)));
    uint32_t r = 99u;
    for (int i = 0; i < 70; i++) { r = r * 1664525u + 1013904223u; sp_starX[i] = (int16_t)((r >> 8) % W); r = r * 1664525u + 1013904223u; sp_starY[i] = (int16_t)((r >> 8) % H); }
  }
}

static int sp_lastPreset = -1;
static int sp_cosmosScene = 0;
static float sp_cosmosT = 0;
static int sp_cosPal = 0, sp_cosGrid = 1, sp_cosCol = 0;

static inline void sp_vertex(int idx, float x, float z, float yk, int baseLev) {
  float d = sp_well(x, z);
  int sx, sy; float dep;
  sp_project8(x, yk + d, z, &sx, &sy, &dep);
  sp_vxs[idx] = (int16_t)sx; sp_vys[idx] = (int16_t)sy;
  sp_vlev[idx] = (uint8_t)sp_levFor(dep, baseLev);
  sp_vhue[idx] = (uint8_t)sp_hueAt(d);
}
static int sp_aaMin = 8;                            // edges at or below this level use plain lines
static inline void sp_edge(int a, int b, int dim) {
  int h = sp_vhue[a] > sp_vhue[b] ? sp_vhue[a] : sp_vhue[b];
  int lev = ((sp_vlev[a] + sp_vlev[b]) >> 1) - dim;
  if (lev <= sp_aaMin) sp_line((sp_vxs[a] + 4) >> 3, (sp_vys[a] + 4) >> 3, (sp_vxs[b] + 4) >> 3, (sp_vys[b] + 4) >> 3, lev, h);   // dim, far lines: a plain line is cheaper and looks the same
  else sp_lineAA(sp_vxs[a], sp_vys[a], sp_vxs[b], sp_vys[b], lev, h);
}

// ─── Fixed-point drawing pipeline ─────────────────────────────────────
// The RP2040 has no floating-point hardware: every float multiply or divide is a
// software routine of dozens of cycles. Everything done per grid point (the wells,
// the 3-D rotation, the perspective) runs here in integers instead — Q12 world
// units (4096 = 1.0), Q14 sines, Q11 well strength, 1/8-pixel screen positions.
#define SP_Q 4096
#define SP_DISTQ 13107                              // camera distance 3.2 in Q12
static int32_t sp_qbx[SP_NB], sp_qbz[SP_NB], sp_qw[SP_NB];   // this slice's bodies (Q12) and well weights (Q8)
static int sp_qn = 0;
static uint8_t sp_qk[SP_NB];                        // which bodies are in this slice
static int32_t sp_dsQ = 0, sp_clampQ = 0;           // depth scale (Q12) and deepest allowed dip (Q12)
static int32_t sp_ccy, sp_csy, sp_cct, sp_cst, sp_qcx, sp_qcz, sp_kd;
static int32_t sp_bandQ = 256;                      // contour band spacing (Q8)
static int32_t sp_gwAQ = 0, sp_gwKI = 0; static int sp_gwPhI = 0;   // gravitational waves: amplitude (Q12), wavenumber, phase
static inline int sp_atanI(int32_t dy, int32_t dx) {            // angle, 0 … 1023 = one turn, integers only
  int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax == 0 && ay == 0) return 0;
  int32_t zz = ax > ay ? (ay << 12) / ax : (ax << 12) / ay;
  int32_t t = 8192 + ((2848 * (4096 - zz)) >> 12);
  int a = (int)((zz * t) >> 18);
  if (ay > ax) a = 256 - a;
  if (dx < 0) a = 512 - a;
  if (dy < 0) a = 1024 - a;
  return a & 1023;
}
static inline int32_t sp_invQ(int32_t r2) {        // 1/sqrt(r² + e²) in Q11, r² in Q12
  if (r2 >= (int32_t)(SP_LUTR2 * SP_Q)) return (int32_t)(2048.0f / sqrtf(r2 * (1.0f / SP_Q) + sp_lutE2));
  int i = r2 >> 5, f = r2 & 31;                     // the table steps r² by 1/128 = 32 in Q12
  return sp_inv[i] + (((int32_t)(sp_inv[i + 1] - sp_inv[i]) * f) >> 5);
}
static int32_t sp_qcut[SP_NB], sp_qoff[SP_NB];      // small bodies: reach (Q12, 0 = everywhere) and the well value there (Q11)
#define SP_CUTR 1640                                // small bodies only dent the sheet within 0.4 of themselves
static inline int32_t sp_phiQ(int32_t x, int32_t z) {   // total well strength at (x, z), Q11
  int32_t phi = 0;
  for (int n = 0; n < sp_qn; n++) {
    int k = sp_qk[n];
    int32_t dx = x - sp_qbx[k], dz = z - sp_qbz[k];
    int32_t c = sp_qcut[k];
    if (c) {                                        // a planet: a local dent that fades smoothly to nothing at its reach
      if (dx > c || dx < -c || dz > c || dz < -c) continue;
      int32_t r2 = (dx * dx + dz * dz) >> 12;
      int32_t v = sp_invQ(r2) - sp_qoff[k];
      if (v > 0) phi += (sp_qw[k] * v) >> 8;
      continue;
    }
    if (dx > 14000 || dx < -14000 || dz > 14000 || dz < -14000) continue;   // far away: nothing, and keeps the squares in range
    int32_t r2 = (dx * dx + dz * dz) >> 12;
    phi += (sp_qw[k] * sp_invQ(r2)) >> 8;
  }
  return phi;
}
static inline int32_t sp_depthQ(int32_t x, int32_t z, int32_t phi) {   // the sheet's dip at (x, z), Q12 (negative = down)
  int32_t d = -((sp_dsQ * phi) >> 11);
  if (sp_gwAQ > 0) {                                // Ripples only: gravitational waves, a two-armed spiral running outward
    int32_t r2 = (x * x + z * z) >> 12;
    int32_t r = (r2 * sp_invQ(r2)) >> 11;                        // ≈ r (Q12), no square root
    int th = sp_atanI(z, x);
    int ph = sp_gwPhI - (int)((r * sp_gwKI) >> 12);
    int32_t env = r < 492 ? (r * 4096) / 492 : (1024 * 4096) / (r + 532);   // rises to the binary, then falls off as 1/r
    d += (((sp_gwAQ * env) >> 12) * vg_sin1024[(2 * th - ph + 256) & 1023]) >> 14;
  }
  return d < -sp_clampQ ? -sp_clampQ : d;
}
static inline int sp_hueQ(int32_t phi) {           // which of the four sheet colours (phi in Q11: 2048 = 1)
  if (sp_cmode == CM_TIME) return HP_G0 + ((sp_slHue + (phi > 7168 ? 2 : 0)) & 3);
  if (sp_cmode == CM_BANDS) return HP_G0 + ((int)((phi * sp_bandQ) >> 19) & 3);
  return HP_G0 + (phi > 12288 ? 3 : (phi > 6554 ? 2 : (phi > 3891 ? 1 : 0)));
}
static inline void sp_projQ(int32_t x, int32_t y, int32_t z, int* sx8, int* sy8, int32_t* zc) {
  x -= sp_qcx; z -= sp_qcz;
  int32_t x1 = (x * sp_ccy + z * sp_csy) >> 14, z1 = (z * sp_ccy - x * sp_csy) >> 14;
  int32_t yv = (y * sp_cct + z1 * sp_cst) >> 14, d = (z1 * sp_cct - y * sp_cst) >> 14;
  int32_t den = SP_DISTQ + d;
  if (den < 1400) den = 1400;
  int32_t inv = (sp_kd << 12) / den;                             // one divide per point (scale ÷ distance, Q12)
  int32_t px = W * 4 + ((x1 * inv) >> 12), py = H * 4 - ((yv * inv) >> 12);
  *sx8 = px < -12000 ? -12000 : (px > 12000 ? 12000 : px);
  *sy8 = py < -12000 ? -12000 : (py > 12000 ? 12000 : py);
  *zc = d;
}
static inline int sp_levQ(int32_t zc, int base) {  // nearer = brighter
  int l = base - (int)(((zc + 2458) * 42) >> 16);
  return l < 4 ? 4 : (l > 15 ? 15 : l);
}
static inline void sp_vertQ(int idx, int32_t x, int32_t z, int32_t yk, int baseLev) {
  int32_t phi = sp_phiQ(x, z);
  int32_t d = sp_depthQ(x, z, phi);
  int sx, sy; int32_t zc;
  sp_projQ(x, yk + d, z, &sx, &sy, &zc);
  sp_vxs[idx] = (int16_t)sx; sp_vys[idx] = (int16_t)sy;
  sp_vlev[idx] = (uint8_t)sp_levQ(zc, baseLev);
  sp_vhue[idx] = (uint8_t)sp_hueQ(phi);
}
// a history sample as Q12 (false if the body wasn't there)
static inline bool sp_histQ(int k, int idx, int32_t* x, int32_t* z) {
  if (idx >= sp_hFill) return false;
  int e = (sp_hHead - idx + SP_HN * 4) % SP_HN;
  int16_t hx = sp_hx[e][k];
  if (hx >= 32600) return false;
  *x = hx >> 1; *z = sp_hy[e][k] >> 1;               // Q13 → Q12
  return true;
}

float sp_lodRef = 1.0f;                                         // detail level 0.35 … 1, steered by the frame time
void prog_spacetime_draw(int preset) {
  unsigned long frameStartUs = micros();
  sp_buf = display.getBuffer();
  if (!sp_buf) return;
  if (preset < 0 || preset >= SP_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f; if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!sp_smReady) sp_sm[i] = pots[i];
      else sp_sm[i] += (pots[i] - sp_sm[i]) * a;
    }
    sp_smReady = true;
  }

  // Cosmos tours the other presets' scenes, each with a fresh palette, grid style and colour mode
  int look = preset;
  float fade = 1.0f;
  if (preset == 11) {
    sp_cosmosT += dt;
    if (sp_cosmosT > 24.0f) {
      sp_cosmosT = 0; sp_cosmosScene = (sp_cosmosScene + 1 + (int)(sp_rand() % 3)) % 11; sp_lastPreset = -2;
      sp_cosPal = (int)(sp_rand() % SP_NPAL); sp_cosGrid = (int)(sp_rand() % SP_NGS); sp_cosCol = (int)(sp_rand() % SP_NCM);
    }
    look = sp_cosmosScene;
    fade = sp_smooth(sp_cosmosT / 1.0f) * sp_smooth((24.0f - sp_cosmosT) / 1.0f);
  }

  if (preset != sp_lastPreset) {
    if (sp_lastPreset != -2) {
      sp_cosmosT = 0;
      if (preset == 11) { look = sp_cosmosScene; sp_cosPal = sp_pPal[look]; sp_cosGrid = sp_pGrid[look]; sp_cosCol = sp_pColour[look]; }
    }
    sp_lastPreset = preset;
    sp_rng = 4242u + (uint32_t)look * 313u + (uint32_t)now;
    sp_massMul = 1.0f;
    sp_setup(sp_pScene[look], look == 5);
    sp_yaw = sp_randf() * 6.2831853f;
    sp_tilt = sp_pTilt[look] * 0.01745f;
    sp_zoom = sp_pZoom[look] / 100.0f;
    sp_camX = sp_camZ = 0;
  }
  sp_buildLut(sp_eps * sp_eps + (sp_scn == SC_HOLE ? 0.004f : 0.012f));

  // ── knobs ──
  int ownPal = preset == 11 ? sp_cosPal : sp_pPal[look];
  int ownGrid = preset == 11 ? sp_cosGrid : sp_pGrid[look];
  int ownCol = preset == 11 ? sp_cosCol : sp_pColour[look];
  int pal = sp_centre(0) ? ownPal : sp_clamp(sp_k(0) * SP_NPAL / 1024, 0, SP_NPAL - 1);
  float sk = sp_sm[1] / 1023.0f;
  float speed = sk < 0.03f ? 0.0f : 0.15f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.55 … right 2.1
  int slices = sp_centre(2) ? sp_pSlices[look] : sp_clamp(1 + sp_k(2) * 12 / 1024, 1, 12);
  float gap = sp_rel(3, sp_pGap[look] * 0.01f, 2.2f);
  float dens = sp_clampf(sp_pDens[look] / 100.0f + sp_bi(4) * 0.5f, 0.0f, 1.0f);
  sp_depthScale = 0.14f * sp_rel(5, sp_pDepth[look] / 100.0f, 2.5f);
  sp_depthClamp = sp_scn == SC_HOLE ? 1.4f : 0.85f;
  float spb = sp_bi(6);
  float spin = sp_pSpin[look] * 0.01f * (spb >= 0 ? 1.0f + 3.0f * spb : 1.0f + 2.0f * spb);
  float tiltT = sp_centre(7) ? sp_pTilt[look] * 0.01745f : (4.0f + sp_k(7) * 84.0f / 1023.0f) * 0.01745f;
  float zoomK = sp_rel(8, 1.0f, 2.2f);
  float lag = sp_rel(9, sp_pLag[look] * 0.01f, 3.0f);
  int gstyle = sp_choice(10, ownGrid, SP_NGS);
  int trail = sp_clamp((int)(sp_pTrail[look] * powf(2.5f, sp_bi(11))), 0, 150);
  float world = sp_clampf(0.6f + sp_bi(11) * 0.6f, 0.0f, 1.0f);   // worldlines follow the Trails knob
  if (sp_k(11) < 30) { trail = 0; world = 0; }
  sp_cmode = sp_choice(12, ownCol, SP_NCM);
  float rel = sp_rel(13, 1.0f, 4.0f);
  if (sp_k(13) < 30) rel = 0;
  float massMul = sp_rel(14, 1.0f, 1.6f);
  float bright = 0.55f + sp_k(15) * 0.75f / 1023.0f;              // 0.55 … 1.3, centre ≈ 0.92
  sp_buildPalette(pal, bright * (0.15f + 0.85f * fade));

  // keep the history long enough for the slices
  float maxBack = SP_HS * (SP_HN - 2 - trail);
  if (slices > 1 && lag * (slices - 1) > maxBack) lag = maxBack / (slices - 1);
  if (lag < 0.02f) lag = 0.02f;

  // ── physics ──
  sp_alpha = sp_alphaBase * rel;
  sp_massMul = massMul;
  bool k12 = keysPressed[KEY_MOD_A];
  static bool k12Was = false;
  if (k12 && !k12Was && !sp_on[SP_ROGUE]) sp_launchRogue();
  k12Was = k12;
  sp_advance(dt * speed, sp_np > 0);
  if (!sp_sane()) { sp_massMul = 1.0f; sp_setup(sp_scn, look == 5); }
  sp_gwAmp = 0;
  if (sp_gw) sp_gwAmp = sp_gwStage == 0 ? 0.05f * sqrtf(0.38f / (sp_gwD + 0.02f)) : 0.16f * sp_ring;
  static float bandPh = 0; bandPh += dt * 0.35f;
  sp_bandK = 0.9f + 0.25f * sinf(bandPh);                         // the contour bands slowly breathe up and down the wells

  // ── camera ──
  sp_yaw += spin * 6.2831853f * dt;
  if (sp_yaw > 6.2831853f) sp_yaw -= 6.2831853f; if (sp_yaw < 0) sp_yaw += 6.2831853f;
  static float camT = 0; camT += dt;
  float tiltWave = 0.12f * sinf(camT * 0.11f);                    // a slow nod
  sp_tilt += (sp_clampf(tiltT + tiltWave, 0.05f, 1.55f) - sp_tilt) * (1 - expf(-dt * 1.5f));
  float zTarget = sp_pZoom[look] / 100.0f * zoomK;
  float tx = 0, tz = 0;
  if (look == 10) {                                               // Orrery: fly in to each planet in turn, then back out
    float cyc = fmodf(camT, 10.0f);
    int who = 1 + ((int)(camT / 10.0f)) % 5;
    float inOut = sp_smooth((cyc - 1.0f) / 2.5f) * sp_smooth((9.5f - cyc) / 2.0f);
    if (sp_on[who]) { tx = sp_bx[who] * inOut; tz = sp_by[who] * inOut; }
    zTarget *= 1.0f + 1.9f * inOut;
  } else if (look != 1 && look != 8) {
    zTarget *= 1.0f + 0.12f * sinf(camT * 0.07f);                 // a gentle breathing zoom
  }
  if (zTarget > 4.0f) zTarget = 4.0f;                              // never so close that the sheet leaves the screen
  sp_zoom += (zTarget - sp_zoom) * (1 - expf(-dt * 1.8f));
  sp_camX += (tx - sp_camX) * (1 - expf(-dt * 2.0f));
  sp_camZ += (tz - sp_camZ) * (1 - expf(-dt * 2.0f));
  sp_cy = cosf(sp_yaw); sp_sy = sinf(sp_yaw); sp_ct = cosf(sp_tilt); sp_st = sinf(sp_tilt);

  // ── draw ──
  memset(sp_buf, 0, W * H);
  for (int i = 0; i < 70; i++) sp_put(sp_starX[i], sp_starY[i], 3 + (i % 4), HP_STAR);

  // grid resolution: bounded per frame
  // adaptive detail: keep each frame inside one screen refresh (the platform waits for the next
  // refresh before showing a frame, so a frame that runs even slightly long is held twice as long —
  // that uneven rhythm is what reads as choppy). The grid gets coarser when frames run long, finer when there's time.
  float lod = sp_lodRef;
  int budget = (int)(2000.0f * lod / (1.0f + (slices - 1) * 0.42f));
  if (budget < 120) budget = 120;   // the top (now) slice gets this; the ones below use about half
  if (budget > SP_MAXV) budget = SP_MAXV;
  int rings = 0, spokes = 0, M = 0;
  bool roundish = gstyle == GS_ROUND || gstyle == GS_CONTOUR || gstyle == GS_STAR;
  if (roundish) {
    rings = 5 + (int)(dens * 13.0f); spokes = 20 + (int)(dens * 36.0f);
    if (gstyle == GS_CONTOUR) { rings += 4; }                      // more rings, they're the whole picture
    if (gstyle == GS_STAR) { spokes += 6; rings = rings * 2 / 3 + 2; }
    while (rings * spokes + 1 > budget && (rings > 5 || spokes > 20)) { if (rings > 5) rings--; if (spokes > 20) spokes -= 2; }
  } else {
    M = 11 + (int)(dens * 14.0f);
    while (M * M > budget && M > 7) M--;
  }
  const int32_t LQ = (int32_t)(1.1f * SP_Q);                     // sheet half-size
  float totalH = gap * (slices - 1);
  // per-frame fixed-point constants
  sp_ccy = (int32_t)(sp_cy * 16384); sp_csy = (int32_t)(sp_sy * 16384); sp_cct = (int32_t)(sp_ct * 16384); sp_cst = (int32_t)(sp_st * 16384);
  sp_qcx = (int32_t)(sp_camX * SP_Q); sp_qcz = (int32_t)(sp_camZ * SP_Q);
  sp_kd = (int32_t)(96.0f * 8.0f * sp_zoom * 3.2f);
  sp_dsQ = (int32_t)(sp_depthScale * SP_Q); sp_clampQ = (int32_t)(sp_depthClamp * SP_Q);
  sp_bandQ = (int32_t)(sp_bandK * 256);
  sp_gwAQ = sp_gwAmp > 0.0005f ? (int32_t)(sp_gwAmp * SP_Q) : 0;
  sp_gwPhI = (int)(2.0f * sp_gwPhi * (1024.0f / 6.2831853f));
  sp_gwKI = (int32_t)(2.0f * sp_gwOmega / 1.6f * (1024.0f / 6.2831853f) * 1.0f);
  // the grid's own coordinates, worked out once per frame
  static int32_t ringR[24];
  static int16_t spC[64], spS[64];
  if (roundish) {
    if (spokes > 64) spokes = 64;
    if (rings > 24) rings = 24;
    for (int j = 1; j <= rings; j++) ringR[j - 1] = LQ * j / rings;
    for (int i = 0; i < spokes; i++) { int a = i * 1024 / spokes; spC[i] = vg_sin1024[(a + 256) & 1023]; spS[i] = vg_sin1024[a & 1023]; }
  }
  int trailSteps = trail > 1 ? trail / 3 + 2 : 0;
  const int this_M = M;                  // trail points every 3 samples (they're joined by smooth lines)

  for (int sl = 0; sl < slices; sl++) {
    watchdog_update();                                            // heavy settings on the RP2040: never let the 500 ms watchdog fire mid-frame
    float back = lag * sl;                                        // the top slice is now; each one below is earlier
    int32_t ykQ = (int32_t)((totalH * 0.5f - gap * sl) * SP_Q);
    sp_qn = 0;
    // where each body was at this slice's moment: the top slice uses the live positions, the others
    // interpolate between two history samples in integers
    int hA = -1, hB = -1; int32_t hF = 0;
    {
      float sinceS = sp_simT - (sp_nextSample - SP_HS);
      float sS = (back - sinceS) / SP_HS;
      if (sl > 0 && sS >= 0) {
        int i0 = (int)sS; hF = (int32_t)((sS - i0) * 4096);
        if (i0 >= sp_hFill - 1) { i0 = sp_hFill - 2; hF = 4096; }
        if (i0 >= 0) { hA = (sp_hHead - i0 + SP_HN * 4) % SP_HN; hB = (sp_hHead - i0 - 1 + SP_HN * 4) % SP_HN; }
      }
    }
    for (int k = 0; k < SP_NB; k++) {
      float x = 0, y = 0;
      if (hA >= 0) {
        int16_t ax = sp_hx[hA][k], bx = sp_hx[hB][k];
        sp_slOn[k] = ax < 32600 && bx < 32600;
        if (sp_slOn[k]) {
          int32_t ay = sp_hy[hA][k], by = sp_hy[hB][k];
          sp_qbx[k] = (ax + (((bx - ax) * hF) >> 12)) >> 1; sp_qbz[k] = (ay + (((by - ay) * hF) >> 12)) >> 1;   // Q13 → Q12
        } else sp_qbx[k] = sp_qbz[k] = 0;
        x = sp_qbx[k] * (1.0f / SP_Q); y = sp_qbz[k] * (1.0f / SP_Q);
      } else {
        sp_slOn[k] = sp_past(k, back, &x, &y);
        if (!sp_slOn[k]) { x = y = 0; }
        sp_qbx[k] = (int32_t)(x * SP_Q); sp_qbz[k] = (int32_t)(y * SP_Q);
      }
      sp_slX[k] = x; sp_slY[k] = y;
      float w = sp_slOn[k] ? sp_wm[k] * (k == 0 ? sp_massMul : 1.0f) : 0.0f;
      sp_qw[k] = (int32_t)(w * 256);
      if (sp_qw[k] > 0) sp_qk[sp_qn++] = (uint8_t)k;
      sp_qcut[k] = (w > 0 && w < 0.3f) ? SP_CUTR : 0;
      if (sp_qcut[k]) sp_qoff[k] = sp_invQ((SP_CUTR * SP_CUTR) >> 12);
    }
    sp_slHue = sl;
    int baseLev = sp_clamp(13 - sl * 4 / (slices > 1 ? slices : 1), 8, 14);
    sp_aaMin = sl == 0 ? 8 : 15;                                  // smooth (anti-aliased) lines on the top slice; plain ones below
    // vertices + edges (the slices below the top are drawn at half detail: every other spoke / grid line)
    int st = (sl > 0 && ((roundish && spokes >= 24) || (!roundish && M >= 13))) ? 2 : 1;
    if (roundish) {
      sp_vertQ(0, 0, 0, ykQ, baseLev);
      int nv = 1;
      int sN = spokes / st;
      for (int j = 0; j < rings; j++) {
        int32_t r = ringR[j];
        for (int i = 0; i < sN; i++) sp_vertQ(nv++, (r * spC[i * st]) >> 14, (r * spS[i * st]) >> 14, ykQ, baseLev);
      }
      int spokes = sN;                                            // (this slice's own count)
      for (int j = 1; j <= rings; j++) for (int i = 0; i < spokes; i++) {
        int v0 = 1 + (j - 1) * spokes + i, v1 = 1 + (j - 1) * spokes + (i + 1) % spokes;
        int vin = j == 1 ? 0 : v0 - spokes;
        if (gstyle != GS_STAR) sp_edge(v0, v1, 0);
        if (gstyle == GS_ROUND && ((i & 1) == 0 || j > rings / 2)) sp_edge(vin, v0, 1);
        if (gstyle == GS_STAR) sp_edge(vin, v0, 0);
      }
      if (gstyle == GS_STAR) for (int i = 0; i < spokes; i += 2) { int v = 1 + (rings - 1) * spokes + i, w = 1 + (rings - 1) * spokes + (i + 2) % spokes; sp_edge(v, w, 2); }   // a faint rim
    } else {
      int nv = 0;
      int M = st == 2 ? (this_M + 1) / 2 : this_M;
      for (int j = 0; j < M; j++) {
        int32_t z = -LQ + 2 * LQ * j / (M - 1);
        for (int i = 0; i < M; i++) sp_vertQ(nv++, -LQ + 2 * LQ * i / (M - 1), z, ykQ, baseLev);
      }
      for (int j = 0; j < M; j++) for (int i = 0; i < M; i++) {
        int v = j * M + i;
        if (i + 1 < M) sp_edge(v, v + 1, 0);
        if (j + 1 < M) sp_edge(v, v + M, 0);
        if (gstyle == GS_TRI && i + 1 < M && j + 1 < M) sp_edge(v, v + M + 1, 2);
      }
    }
    // trails: each body's recent path on this slice, sitting in the sheet (read straight from the history)
    if (trailSteps > 0) {
      float since = sp_simT - (sp_nextSample - SP_HS);
      int i0 = (int)((back - since) / SP_HS) + 1;
      if (i0 < 0) i0 = 0;
      for (int k = 0; k < SP_NB; k++) {
        if (!sp_slOn[k]) continue;
        if (k == 0 && sp_scn != SC_EIGHT && sp_scn != SC_BINARY) continue;   // the sun doesn't need a trail
        int32_t x = sp_qbx[k], z = sp_qbz[k];
        int32_t d = sp_depthQ(x, z, sp_phiQ(x, z)) * 7 / 8;
        int px, py; int32_t zc;
        sp_projQ(x, ykQ + d, z, &px, &py, &zc);
        int myTrail = trailSteps * (slices + 1 - sl) / (slices + 1);   // older slices carry shorter trails
        for (int t = 0; t < myTrail; t++) {
          if (!sp_histQ(k, i0 + t * 3, &x, &z)) break;
          d = sp_depthQ(x, z, sp_phiQ(x, z)) * 7 / 8;
          int sx, sy;
          sp_projQ(x, ykQ + d, z, &sx, &sy, &zc);
          int tl = sp_levQ(zc, baseLev + 2) - t * 7 / trailSteps;
          if (sl == 0) sp_lineAA(px, py, sx, sy, tl, HP_BODY + (k % 7));
          else sp_line((px + 4) >> 3, (py + 4) >> 3, (sx + 4) >> 3, (sy + 4) >> 3, tl, HP_BODY + (k % 7));
          px = sx; py = sy;
        }
      }
    }
    // the bodies, each with a halo ring in a contrasting colour
    for (int k = 0; k < SP_NB; k++) {
      sp_wlOk[sl < 12 ? sl : 11][k] = 0;
      if (!sp_slOn[k]) continue;
      int32_t x = sp_qbx[k], z = sp_qbz[k];
      int32_t d = sp_depthQ(x, z, sp_phiQ(x, z));
      int sx8, sy8; int32_t zc;
      sp_projQ(x, ykQ + d + (int32_t)(sp_size[k] * 0.6f * SP_Q), z, &sx8, &sy8, &zc);
      int sx = sx8 >> 3, sy = sy8 >> 3;
      int32_t den = SP_DISTQ + zc; if (den < 1400) den = 1400;
      float rad = sp_size[k] * 96.0f * sp_zoom * (float)SP_DISTQ / (float)den;
      int lev = sp_levQ(zc, 16);
      if (rad > 2.5f) {                                           // a thin halo just outside the ball
        int hr = (int)((rad + 2.0f) * 8.0f);
        int hue = HP_BODY + ((k + 3) % 7);
        int prevX = sx8 + hr, prevY = sy8;
        for (int a = 64; a <= 1024; a += 64) {
          int qx = sx8 + ((hr * vg_sin1024[(a + 256) & 1023]) >> 14), qy = sy8 + ((hr * vg_sin1024[a & 1023]) >> 14);
          sp_lineAA(prevX, prevY, qx, qy, lev - 4, hue);
          prevX = qx; prevY = qy;
        }
      }
      sp_disc(sx, sy, rad, lev, HP_BODY + (k % 7));
      if (sl < 12) { sp_wlX[sl][k] = (int16_t)sx8; sp_wlY[sl][k] = (int16_t)sy8; sp_wlOk[sl][k] = 1; }
    }
    // disc matter (only the present slice has it)
    if (sl == 0 && sp_np > 0) {
      for (int i = 0; i < sp_np; i++) {
        int32_t x = (int32_t)(sp_px[i] * SP_Q), z = (int32_t)(sp_py[i] * SP_Q);
        int32_t d = sp_depthQ(x, z, sp_phiQ(x, z));
        int sx, sy; int32_t zc;
        sp_projQ(x, ykQ + d + 41, z, &sx, &sy, &zc);
        sx >>= 3; sy >>= 3;
        int lev = sp_levQ(zc, 16);
        sp_put(sx, sy, lev, HP_PART); sp_put(sx + 1, sy, lev, HP_PART); sp_put(sx, sy + 1, lev - 1, HP_PART); sp_put(sx + 1, sy + 1, lev - 2, HP_PART);
      }
    }
  }
  // worldlines: each body threaded down through the slices
  if (world > 0.02f && slices > 1) {
    int wl = 6 + (int)(world * 8.0f);
    for (int k = 0; k < SP_NB; k++) {
      for (int sl = 0; sl + 1 < slices && sl + 1 < 12; sl++) {
        if (!sp_wlOk[sl][k] || !sp_wlOk[sl + 1][k]) continue;
        sp_lineAA(sp_wlX[sl][k], sp_wlY[sl][k], sp_wlX[sl + 1][k], sp_wlY[sl + 1][k], wl, HP_BODY + (k % 7));
      }
    }
  }
  {                                                               // how long did this frame take? steer the detail level
    unsigned long us = micros() - frameStartUs;
    if (us > 13000) sp_lodRef *= 0.93f;                           // too slow: coarser
    else if (us < 10000) sp_lodRef *= 1.02f;                      // room to spare: finer
    if (sp_lodRef > 1.0f) sp_lodRef = 1.0f;
    if (sp_lodRef < 0.35f) sp_lodRef = 0.35f;
  }
}

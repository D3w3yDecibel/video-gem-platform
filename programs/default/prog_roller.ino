// =====================================================================
// PROGRAM: ROLLER — race a marble down a floating isometric course
// (by Dewey) — Set 5 "Arcade", slot 1
//
// In the spirit of the 1984 marble-racing cabinet: a glass marble rolls
// down a chequered course that floats in space — slopes, staircases, narrow
// bridges, funnels, bumpy fields and ski-jumps — against the clock. Black
// STEELIES hunt you down and shove you off the edge; little SPRINGS hop
// across the course and bat you sideways. Fall off and you lose time; reach
// the goal for bonus time and a new, harder course. Every course is
// generated, so no two races are the same.
//
// It plays itself; turn p0/p1 and you steer the marble (p0 = left/right,
// p1 = up/down the screen, like tilting a trackball).
//
// Presets:
//   k0 Practice (wide and gentle)   k1 Beginner   k2 Stairway   k3 Aerial
//   k4 Neon Night   k5 Candy   k6 Ice Rink (slippery)   k7 Lava
//   k8 Wireframe   k9 Steelie Swarm   k10 Moon (low gravity)
//   k11 Tour — a new look every course
//   k12 — BOOST (a burst of speed downhill). Joins the game.
//
// Knobs:
//   p0 Steer ◄ left | right ►   p1 Steer ◄ down | up ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Steelies (centre = preset)   p8 Course Width   p9 Steepness
//   p10 Course Style (centre = preset: mixed, stairs, bridges, funnels, bumps, chutes, jumps)
//   p11 Tiles (centre = preset: chequer + grid, solid, grid only, stripes)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 tile A, 2 tile B, 3 walls, 4 grid, 5 marble, 6 steelie, 7 goal, 8 HUD, 9 springs, 10 stars, 11 shadow
struct Rl6Pre {
  uint8_t tA[3], tB[3], wall[3], grid[3], ball[3];
  uint8_t look, width, steep, steelies, style, tiles, grav, slip, trails;
};
static const Rl6Pre rl6_pre[11] = {
  {{90,190,90},{70,160,70},{60,90,140},{20,60,20},{80,140,255}, A5_ARCADE, 6, 3, 0, 0, 0, 10, 0, 0},
  {{200,200,210},{150,150,170},{90,70,140},{60,60,80},{80,140,255}, A5_ARCADE, 5, 4, 1, 0, 0, 10, 0, 0},
  {{230,180,90},{200,140,60},{140,80,60},{90,50,20},{100,180,255}, A5_ARCADE, 5, 4, 1, 1, 0, 10, 0, 0},
  {{140,200,255},{90,150,230},{60,60,160},{30,50,120},{255,200,60}, A5_ARCADE, 3, 4, 1, 2, 0, 10, 0, 0},
  {{255,0,200},{0,220,255},{60,0,120},{255,255,255},{255,255,90}, A5_NEON, 5, 4, 2, 0, 2, 10, 0, 120},
  {{255,180,210},{180,230,255},{220,190,255},{255,255,255},{255,120,160}, A5_ARCADE, 5, 4, 1, 4, 0, 10, 0, 0},
  {{220,240,255},{170,210,250},{80,140,220},{120,170,230},{255,90,90}, A5_ARCADE, 5, 4, 1, 3, 0, 10, 1, 0},
  {{255,120,20},{200,40,10},{90,10,10},{255,220,60},{120,220,255}, A5_ARCADE, 4, 5, 2, 5, 3, 10, 0, 0},
  {{0,255,120},{0,200,255},{0,60,40},{0,255,160},{255,255,255}, A5_PHOSPHOR, 5, 4, 1, 0, 2, 10, 0, 160},
  {{180,180,180},{140,140,150},{70,70,90},{40,40,50},{255,210,40}, A5_ARCADE, 6, 4, 4, 0, 0, 10, 0, 0},
  {{170,170,170},{120,120,130},{60,60,70},{40,40,40},{120,200,255}, A5_ARCADE, 5, 4, 1, 6, 1, 4, 0, 60},
};
static const char* const rl6_names[12] = { "Practice", "Beginner", "Stairway", "Aerial", "Neon Night", "Candy", "Ice Rink", "Lava", "Wireframe", "Steelie Swarm", "Moon", "Tour" };
static const char* const rl6_lbl[16] = { "Steer: Left|Right", "Steer: Down|Up", "", "", "", "", "", "Steelies", "Course Width", "Steepness", "Course Style", "Tiles", "", "", "", "" };
const char* prog_roller_name() { return "ROLLER"; }
const char* prog_roller_character() { return "A marble races down a floating isometric course against the clock - plays itself until you touch p0/p1/k12"; }
const char* prog_roller_presetName(int preset) { return (preset >= 0 && preset < 12) ? rl6_names[preset] : NULL; }
const char* prog_roller_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, rl6_lbl); }
uint8_t prog_roller_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ── the course: 16 segments of 10 diagonals, all worked out from a seed ──
#define RL6_NS 16
#define RL6_SL 10
#define RL6_LEN (RL6_NS * RL6_SL)
#define RL6_GOAL (RL6_LEN - 6)
enum { RL6_SLOPE, RL6_STAIRS, RL6_BRIDGE, RL6_FUNNEL, RL6_BUMPS, RL6_CHUTE, RL6_JUMP, RL6_NTYPE };
static uint8_t rl6_segT[RL6_NS], rl6_segW[RL6_NS];
static int16_t rl6_segH[RL6_NS + 1];
static int rl6_amp = 64, rl6_ph1 = 0, rl6_ph2 = 0;
static int rl6_widthK = 16, rl6_steepK = 16;                  // /16
static inline int rl6_seg(int d) { return v3_clamp(d / RL6_SL, 0, RL6_NS - 1); }
static inline int rl6_c16(int d) {                            // course centre across (e), 1/16 units
  int c = (rl6_amp * v3_sin(d * 9 + rl6_ph1) >> 14) + ((rl6_amp / 2) * v3_sin(d * 23 + rl6_ph2) >> 14);
  if (d < 6) c = c * d / 6;
  return c;
}
static inline int rl6_w16(int d) {
  int s = rl6_seg(d), w = rl6_segW[s] * rl6_widthK;
  if (d < 6 || d > RL6_GOAL - 4) w = a5_max(w, 5 * 16);
  return w;
}
static bool rl6_exists(int i, int j) {
  int dc = i + j + 1, e = i - j;
  if (dc < 0 || dc > RL6_LEN) return false;
  return v3_abs(e * 16 - rl6_c16(dc)) <= rl6_w16(dc);
}
static int rl6_drop(int s) {                                  // how far a segment falls, px
  switch (rl6_segT[s]) { case RL6_STAIRS: return 50; case RL6_CHUTE: return 70; case RL6_JUMP: return 40; case RL6_BRIDGE: return 20; default: return 34; }
}
// the height of corner (ci, cj) of tile (i, j), px (tiles in a staircase are offset, which makes walls)
static int rl6_corner(int i, int j, int ci, int cj) {
  int dc = i + j + 1, s = rl6_seg(dc), dl = dc - s * RL6_SL;
  int x = i + ci, y = j + cj, d = x + y, e = x - y;
  int base = rl6_segH[s], drop = rl6_drop(s) * rl6_steepK / 16;
  int h;
  switch (rl6_segT[s]) {
    case RL6_STAIRS: h = base - drop * (dl / 2) / 5; break;
    case RL6_JUMP: {
      int dlc = d - s * RL6_SL;
      if (dl <= 6) h = base - drop * dlc / 20 + a5_max(0, a5_min(dlc - 3, 3)) * 4;
      else h = base - drop * 3 / 4 - drop * (dlc - 7) / 12;   // land well below the lip
      break;
    }
    case RL6_FUNNEL: h = base - drop * (d - s * RL6_SL) / RL6_SL + v3_abs(e * 16 - rl6_c16(d)) / 8; break;
    case RL6_BUMPS: h = base - drop * (d - s * RL6_SL) / RL6_SL + ((3 * v3_sin(e * 256)) >> 14); break;   // ridges running down the course
    default: h = base - drop * (d - s * RL6_SL) / RL6_SL; break;
  }
  if (dc < 3) h = rl6_segH[0];
  return h;
}
static void rl6_newCourse(uint32_t seed, int style, int level) {
  uint32_t r = seed;
  rl6_amp = 48 + (int)(v3_hash(r) % 48); rl6_ph1 = (int)(v3_hash(r + 1) & 1023); rl6_ph2 = (int)(v3_hash(r + 2) & 1023);
  int h = 0;
  for (int s = 0; s < RL6_NS; s++) {
    uint32_t q = v3_hash(r * 31 + s * 977);
    int t = (int)(q % RL6_NTYPE);
    if (style > 0 && (q >> 8) % 3 != 0) t = style;           // a style knob makes most segments that kind
    if (s == 0) t = RL6_SLOPE;
    if (t == RL6_BRIDGE && level < 2 && style == 0) t = RL6_FUNNEL;
    rl6_segT[s] = (uint8_t)t;
    int w = t == RL6_BRIDGE ? 2 : (t == RL6_FUNNEL ? 5 : 3 + (int)((q >> 12) % 3));
    rl6_segW[s] = (uint8_t)w;
    rl6_segH[s] = (int16_t)h;
    h -= rl6_drop(s) * rl6_steepK / 16;
  }
  rl6_segH[RL6_NS] = (int16_t)h;
}

// ── marbles: [0] is yours, the rest are steelies ──
#define RL6_NM 6
static float rl6_x[RL6_NM], rl6_y[RL6_NM], rl6_z[RL6_NM], rl6_vx[RL6_NM], rl6_vy[RL6_NM], rl6_vz[RL6_NM];
static bool rl6_air[RL6_NM], rl6_on[RL6_NM];
static float rl6_maxZ[RL6_NM];
static int rl6_nm = 1;
#define RL6_NJ 4
static float rl6_jx[RL6_NJ], rl6_jy[RL6_NJ], rl6_jdir[RL6_NJ], rl6_jt[RL6_NJ];
static int rl6_nj = 0;
static float rl6_time = 60, rl6_camX = 0, rl6_camY = 0, rl6_boost = 0, rl6_g = 1.0f, rl6_fric = 0.992f;
static float rl6_stuck = 0;
static float rl6_safeX = 2, rl6_safeY = 2, rl6_lost = 0, rl6_goalT = 0, rl6_bestD = 0;
static int rl6_course = 1, rl6_over = 0, rl6_preset = -1, rl6_look = 0, rl6_style = 0, rl6_tiles = 0, rl6_banner = 0;
static uint32_t rl6_score = 0, rl6_hi = 20000, rl6_seed = 1;

// the ground under (x, y): false if there is no course there
static bool rl6_ground(float x, float y, float* h) {
  int i = (int)floorf(x), j = (int)floorf(y);
  if (!rl6_exists(i, j)) return false;
  float fx = x - i, fy = y - j;
  float h00 = (float)rl6_corner(i, j, 0, 0), h10 = (float)rl6_corner(i, j, 1, 0), h01 = (float)rl6_corner(i, j, 0, 1), h11 = (float)rl6_corner(i, j, 1, 1);
  *h = h00 + (h10 - h00) * fx + (h01 - h00) * fy + (h00 - h10 - h01 + h11) * fx * fy;
  return true;
}
static void rl6_place(int k, float x, float y) {
  float h = 0; rl6_ground(x, y, &h);
  rl6_x[k] = x; rl6_y[k] = y; rl6_z[k] = h; rl6_vx[k] = rl6_vy[k] = rl6_vz[k] = 0; rl6_air[k] = false; rl6_on[k] = true; rl6_maxZ[k] = h;
}
// a point on the course centre line at diagonal d (world x, y)
static void rl6_centre(float d, float* x, float* y) {
  float e = rl6_c16((int)d) / 16.0f;
  *x = (d + e) / 2.0f; *y = (d - e) / 2.0f;
}
static void rl6_startCourse() {
  rl6_newCourse(rl6_seed, rl6_style, rl6_course);
  float x, y; rl6_centre(2.5f, &x, &y);
  rl6_place(0, x, y);
  rl6_safeX = x; rl6_safeY = y; rl6_bestD = 2;
  for (int k = 1; k < rl6_nm; k++) { rl6_centre(18.0f + k * 14 + v3_rnd(6), &x, &y); rl6_place(k, x + 0.5f, y - 0.5f); }
  rl6_nj = v3_clamp(rl6_course, 0, RL6_NJ);
  for (int k = 0; k < rl6_nj; k++) { rl6_centre(30.0f + k * 28, &x, &y); rl6_jx[k] = x; rl6_jy[k] = y; rl6_jdir[k] = (k & 1) ? 1.0f : -1.0f; rl6_jt[k] = v3_rndf(); }
  rl6_camY = (x + y) * 8 - 90; rl6_camX = (x - y) * 16;
  rl6_banner = 120; rl6_goalT = 0; rl6_lost = 0;
}
static void rl6_newGame() { rl6_score = 0; rl6_course = 1; rl6_time = 70; rl6_over = 0; rl6_seed = v3_rand(); rl6_startCourse(); }

// one marble, one step: control (ax, ay) in world units
static void rl6_stepMarble(int k, float ax, float ay) {
  if (!rl6_on[k]) return;
  float h0;
  bool on = rl6_ground(rl6_x[k], rl6_y[k], &h0);
  if (!rl6_air[k] && on) {
    // slope of this tile's own surface (never across a ledge)
    int ti = (int)floorf(rl6_x[k]), tj = (int)floorf(rl6_y[k]);
    float fx = rl6_x[k] - ti, fy = rl6_y[k] - tj;
    float h00 = (float)rl6_corner(ti, tj, 0, 0), h10 = (float)rl6_corner(ti, tj, 1, 0), h01 = (float)rl6_corner(ti, tj, 0, 1), h11 = (float)rl6_corner(ti, tj, 1, 1);
    float tw = h00 - h10 - h01 + h11;
    float gx = (h10 - h00) + tw * fy, gy = (h01 - h00) + tw * fx;   // px per tile
    rl6_vx[k] -= gx * 0.0012f * rl6_g; rl6_vy[k] -= gy * 0.0012f * rl6_g;
    rl6_vx[k] += ax; rl6_vy[k] += ay;
    rl6_vx[k] *= rl6_fric; rl6_vy[k] *= rl6_fric;
  } else {
    rl6_vx[k] += ax * 0.25f; rl6_vy[k] += ay * 0.25f;
  }
  float sp = sqrtf(rl6_vx[k] * rl6_vx[k] + rl6_vy[k] * rl6_vy[k]), mx = 0.2f;
  if (sp > mx) { rl6_vx[k] *= mx / sp; rl6_vy[k] *= mx / sp; }
  float nx = rl6_x[k] + rl6_vx[k], ny = rl6_y[k] + rl6_vy[k], hn;
  bool onN = rl6_ground(nx, ny, &hn);
  if (onN && hn > rl6_z[k] + 5.0f) {                          // a wall: bounce off the side we hit
    float ht;
    if ((int)floorf(nx) != (int)floorf(rl6_x[k]) && (!rl6_ground(nx, rl6_y[k], &ht) || ht > rl6_z[k] + 5.0f)) { rl6_vx[k] = -rl6_vx[k] * 0.5f; nx = rl6_x[k]; }
    if ((int)floorf(ny) != (int)floorf(rl6_y[k]) && (!rl6_ground(nx, ny, &ht) || ht > rl6_z[k] + 5.0f)) { rl6_vy[k] = -rl6_vy[k] * 0.5f; ny = rl6_y[k]; }
    onN = rl6_ground(nx, ny, &hn);
  }
  rl6_x[k] = nx; rl6_y[k] = ny;
  if (rl6_air[k]) {
    rl6_vz[k] -= 0.32f * rl6_g; rl6_z[k] += rl6_vz[k];
    if (onN && rl6_z[k] <= hn) {
      float fall = rl6_maxZ[k] - hn;
      rl6_z[k] = hn; rl6_air[k] = false; rl6_vz[k] = 0;
      if (fall > 70.0f && k == 0) { rl6_lost = 60; }   // too far: the marble shatters
      else if (fall > 12.0f) a5_shake(fall / 20.0f);
    }
  } else if (onN) {
    if (hn < rl6_z[k] - 3.0f) { rl6_air[k] = true; rl6_vz[k] = 0; rl6_maxZ[k] = rl6_z[k]; }   // off a ledge
    else rl6_z[k] = hn;
  } else { rl6_air[k] = true; rl6_vz[k] = 0; rl6_maxZ[k] = rl6_z[k]; }
  if (rl6_air[k] && rl6_z[k] > rl6_maxZ[k]) rl6_maxZ[k] = rl6_z[k];
  if (rl6_air[k] && !onN && rl6_z[k] < rl6_maxZ[k] - 110.0f) {   // fell into space
    if (k == 0) rl6_lost = 70; else rl6_on[k] = false;
  }
}

// the computer: steer along (d) and across (e) the course separately — keep a safe speed, hug the centre line
static void rl6_ai(float* ax, float* ay) {
  float d = rl6_x[0] + rl6_y[0], e = rl6_x[0] - rl6_y[0];
  int s = rl6_seg((int)d), t = rl6_segT[s], tn = rl6_segT[rl6_seg((int)d + 4)];
  float ce = rl6_c16((int)(d + 2.0f)) / 16.0f;
  for (int k = 1; k < rl6_nm; k++) if (rl6_on[k]) {             // sidestep a steelie that's close
    float dd = rl6_x[k] + rl6_y[k] - d, de = rl6_x[k] - rl6_y[k] - e;
    if (dd * dd + de * de < 3.0f) ce -= (de > 0 ? 1.0f : -1.0f) * 0.8f;
  }
  float vd = rl6_vx[0] + rl6_vy[0], ve = rl6_vx[0] - rl6_vy[0];
  float want = (t == RL6_BRIDGE || tn == RL6_BRIDGE || t == RL6_STAIRS) ? 0.09f : ((t == RL6_JUMP || tn == RL6_JUMP) ? 0.17f : 0.14f);
  float wantE = v3_clampf((ce - e) * 0.06f, -0.08f, 0.08f);
  float ad = v3_clampf((want - vd) * 0.15f, -0.012f, 0.012f), ae = v3_clampf((wantE - ve) * 0.25f, -0.014f, 0.014f);
  *ax = (ad + ae) / 2; *ay = (ad - ae) / 2;
}

void prog_roller_init() { v3_reset(); a5_reset(); rl6_preset = -1; rl6_over = 0; a5_partClear(); }

// world → screen
static inline int rl6_sx(float x, float y) { return 160 + (int)((x - y) * 16 - rl6_camX); }
static inline int rl6_sy(float x, float y, float z) { return (int)((x + y) * 8 - z - rl6_camY); }

static void rl6_drawMarble(int k, uint8_t lev) {
  if (!rl6_on[k]) return;
  float x = rl6_x[k], y = rl6_y[k];
  int sx = rl6_sx(x, y), sy = rl6_sy(x, y, rl6_z[k]) - 6;
  int hue = k == 0 ? 5 : 6;
  if (k == 0 && rl6_lost > 0) {                                // shattered
    int t = 70 - (int)rl6_lost;
    for (int a = 0; a < 8; a++) v3_disc(sx + ((v3_cos(a * 128) * t) >> 15), sy + ((v3_sin(a * 128) * t) >> 15) + t * t / 40, 1, A5C(15, hue));
    return;
  }
  v3_disc(sx, sy, 6, A5C(lev > 11 ? 10 : lev, hue));
  v3_disc(sx - 1, sy - 1, 4, A5C(13, hue));
  v3_disc(sx - 2, sy - 3, 1, A5C(15, 0));
  if (k == 0) { int a = (int)((x + y) * 160) & 1023; v3_put(sx + (v3_cos(a) * 4 >> 14), sy + (v3_sin(a) * 3 >> 14), A5C(15, 0)); }
}

static void roller5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != rl6_preset) { rl6_preset = preset; rl6_look = preset == 11 ? 0 : preset; rl6_course = 0; rl6_over = 0; }
  const Rl6Pre& P = rl6_pre[rl6_look];
  float diff = a5_diffK(0.4f);
  int nm = 1 + (v3_centre(7) ? P.steelies + (rl6_course > 2 ? 1 : 0) : (int)(v3_u(7) * 5.99f));
  rl6_widthK = v3_centre(8) ? 16 : 8 + (int)(v3_u(8) * 20);
  rl6_steepK = v3_centre(9) ? 16 * P.steep / 4 : 6 + (int)(v3_u(9) * 30);
  int style = v3_centre(10) ? P.style : v3_clamp(v3_k(10) * RL6_NTYPE / 1024, 0, RL6_NTYPE - 1);
  rl6_tiles = v3_pick(11, P.tiles, 4);
  rl6_g = P.grav / 10.0f;
  rl6_fric = P.slip ? 0.997f : 0.988f;
  if (rl6_course == 0 || style != rl6_style) { rl6_style = style; rl6_nm = nm; rl6_newGame(); }
  if (nm != rl6_nm) { for (int k = rl6_nm; k < nm; k++) { float x, y; rl6_centre(rl6_x[0] + rl6_y[0] + 12 + k * 5, &x, &y); rl6_place(k, x, y); } rl6_nm = nm; }

  uint8_t hues[12][3] = { {255,255,255},{0},{0},{0},{0},{0},{60,60,70},{255,255,255},{255,220,40},{80,230,80},{200,210,255},{0,0,0} };
  memcpy(hues[1], P.tA, 3); memcpy(hues[2], P.tB, 3); memcpy(hues[3], P.wall, 3); memcpy(hues[4], P.grid, 3); memcpy(hues[5], P.ball, 3);
  a5_palette(hues, 12, a5_lookK(P.look));

  // ── game steps ──
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (rl6_banner > 0) { rl6_banner--; if (rl6_banner > 50) continue; }
    if (rl6_over > 0) { if (--rl6_over == 0) { if (rl6_preset == 11) rl6_look = (rl6_look + 1) % 11; rl6_newGame(); } continue; }
    if (rl6_goalT > 0) {
      rl6_goalT -= 1;
      if (rl6_time > 0) { rl6_time -= 0.25f; rl6_score += 10; }
      if (rl6_goalT <= 0) { rl6_course++; rl6_seed = v3_rand(); rl6_time = v3_clampf(rl6_time, 0, 30) + 50; if (rl6_preset == 11) rl6_look = (rl6_look + 1) % 11; rl6_startCourse(); }
      continue;
    }
    if (rl6_lost > 0) {                                        // lost a marble: back to the last safe spot
      if (--rl6_lost <= 0) { rl6_place(0, rl6_safeX, rl6_safeY); rl6_time -= 2; }
      if (rl6_lost == 59) a5_burst(rl6_sx(rl6_x[0], rl6_y[0]), rl6_sy(rl6_x[0], rl6_y[0], rl6_z[0]), 30, 5, 40, 30);
      continue;
    }
    rl6_time -= 1.0f / 60.0f;
    if (rl6_time <= 0) { rl6_time = 0; rl6_over = 240; if (rl6_score > rl6_hi) rl6_hi = rl6_score; continue; }
    float ax, ay;
    if (a5_player) {
      float sx = a5_stick(0), sy = -a5_stick(1);
      ax = (sx + sy) * 0.004f; ay = (sy - sx) * 0.004f;
      if (a5_fire) rl6_boost = 30;
    } else {
      rl6_ai(&ax, &ay);
      if (v3_rnd(500) == 0 && rl6_segT[rl6_seg((int)(rl6_x[0] + rl6_y[0]))] == RL6_SLOPE) rl6_boost = 30;
    }
    if (rl6_boost > 0) { rl6_boost--; ax += 0.007f; ay += 0.007f; }
    rl6_stepMarble(0, ax, ay);
    // steelies: chase you (harder = more eager)
    for (int k = 1; k < rl6_nm; k++) {
      if (!rl6_on[k]) { if (v3_rnd(200) == 0) { float x, y; rl6_centre(rl6_x[0] + rl6_y[0] + 14, &x, &y); rl6_place(k, x, y); } continue; }
      float dx = rl6_x[0] - rl6_x[k], dy = rl6_y[0] - rl6_y[k], dl = sqrtf(dx * dx + dy * dy) + 0.01f;
      float pull = dl < 8.0f ? (0.0018f + diff * 0.003f) : 0.0f;
      // stay on: steer back to the centre line too
      float cx, cy; rl6_centre(rl6_x[k] + rl6_y[k] + 1.0f, &cx, &cy);
      rl6_stepMarble(k, dx / dl * pull + (cx - rl6_x[k]) * 0.002f, dy / dl * pull + (cy - rl6_y[k]) * 0.002f);
      if (dl > 26.0f) { float x, y; rl6_centre(rl6_x[0] + rl6_y[0] + 16, &x, &y); rl6_place(k, x, y); }
      // bump
      if (dl < 0.8f && v3_abs((int)(rl6_z[k] - rl6_z[0])) < 10) {
        float px = dx / dl, py = dy / dl, rv = (rl6_vx[0] - rl6_vx[k]) * px + (rl6_vy[0] - rl6_vy[k]) * py;
        if (rv < 0) { rl6_vx[0] -= rv * px * 1.2f; rl6_vy[0] -= rv * py * 1.2f; rl6_vx[k] += rv * px; rl6_vy[k] += rv * py; }
        rl6_x[0] += px * 0.05f; rl6_y[0] += py * 0.05f;
      }
    }
    // springs hop across the course
    for (int k = 0; k < rl6_nj; k++) {
      rl6_jt[k] += 1.0f / 50.0f; if (rl6_jt[k] > 1) rl6_jt[k] -= 1;
      float d = rl6_jx[k] + rl6_jy[k], e = rl6_jx[k] - rl6_jy[k] + rl6_jdir[k] * 0.03f;
      float c = rl6_c16((int)d) / 16.0f, w = rl6_w16((int)d) / 16.0f;
      if (e > c + w - 0.5f) rl6_jdir[k] = -1; if (e < c - w + 0.5f) rl6_jdir[k] = 1;
      rl6_jx[k] = (d + e) / 2; rl6_jy[k] = (d - e) / 2;
      float dx = rl6_x[0] - rl6_jx[k], dy = rl6_y[0] - rl6_jy[k];
      float hop = sinf(rl6_jt[k] * 3.14159f) * 16;
      if (dx * dx + dy * dy < 0.5f && hop < 8 && !rl6_air[0]) { rl6_vx[0] += rl6_jdir[k] * 0.12f; rl6_vy[0] -= rl6_jdir[k] * 0.12f; a5_shake(2); }
    }
    // progress, checkpoints and the goal
    float d0 = rl6_x[0] + rl6_y[0];
    rl6_stuck = (d0 > rl6_bestD + 0.05f) ? 0 : rl6_stuck + 1;
    if (rl6_stuck > 200 && !a5_player) { rl6_boost = 40; rl6_stuck = 0; }
    if (!rl6_air[0] && d0 > rl6_bestD) { rl6_score += (uint32_t)((d0 - rl6_bestD) * 10); rl6_bestD = d0; }
    if (!rl6_air[0] && ((int)d0 % RL6_SL) < 2 && d0 > rl6_x[0] - 1) { rl6_centre((float)((int)d0), &rl6_safeX, &rl6_safeY); }
    if (d0 > RL6_GOAL && !rl6_air[0]) { rl6_goalT = 120; rl6_score += 1000; a5_burst(rl6_sx(rl6_x[0], rl6_y[0]), rl6_sy(rl6_x[0], rl6_y[0], rl6_z[0]), 60, 7, 60, 50); }
  }

  // ── camera ──
  float tx = (rl6_x[0] - rl6_y[0]) * 16 * 0.75f, ty = (rl6_x[0] + rl6_y[0]) * 8 - rl6_z[0] - 100;
  rl6_camX += (tx - rl6_camX) * 0.08f; rl6_camY += (ty - rl6_camY) * 0.12f;

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  a5_stars(60, 0, -4, 10, 5 + rl6_course);
  int dB = (int)(rl6_x[0] + rl6_y[0]), dMin = dB - 18, dMax = dB + 28;
  dMin = v3_clamp(dMin, -2, RL6_LEN + 2); dMax = v3_clamp(dMax, -2, RL6_LEN + 2);
  // marbles in draw order
  int order[RL6_NM], no = 0;
  for (int k = 0; k < rl6_nm; k++) if (rl6_on[k]) order[no++] = k;
  for (int a = 1; a < no; a++) { int v = order[a], b = a - 1; while (b >= 0 && rl6_x[order[b]] + rl6_y[order[b]] > rl6_x[v] + rl6_y[v]) { order[b + 1] = order[b]; b--; } order[b + 1] = v; }
  int nextM = 0;
  for (int dg = dMin; dg <= dMax; dg++) {
    // marbles that sit behind this diagonal's tiles
    while (nextM < no && rl6_x[order[nextM]] + rl6_y[order[nextM]] < dg + 0.6f) { rl6_drawMarble(order[nextM], 9); nextM++; }
    int dc = dg + 1, c = rl6_c16(dc), w = rl6_w16(dc);
    int e0 = (c - w) / 16 - 2, e1 = (c + w) / 16 + 2;
    for (int e = e0; e <= e1; e++) {
      if (((e + dg) & 1) != 0) continue;
      int i = (dg + e) / 2, j = (dg - e) / 2;
      if (!rl6_exists(i, j)) continue;
      int h00 = rl6_corner(i, j, 0, 0), h10 = rl6_corner(i, j, 1, 0), h11 = rl6_corner(i, j, 1, 1), h01 = rl6_corner(i, j, 0, 1);
      int x0 = 160 + (i - j) * 16 - (int)rl6_camX, y0 = (i + j) * 8 - (int)rl6_camY;
      if (x0 < -40 || x0 > W + 40 || y0 - h00 > H + 40 || y0 - h00 < -80) continue;
      // walls: the +i side (lower right) and the +j side (lower left)
      int nb10, nb11, nb01, nb11b;
      if (rl6_exists(i + 1, j)) { nb10 = rl6_corner(i + 1, j, 0, 0); nb11 = rl6_corner(i + 1, j, 0, 1); } else { nb10 = h10 - 40; nb11 = h11 - 40; }
      if (nb10 < h10 || nb11 < h11) {
        int xa = x0 + 16, ya = y0 + 8, xb = x0, yb = y0 + 16;
        a5_quad(xa, ya - h10, xb, yb - h11, xb, yb - nb11, xa, ya - nb10, A5C(rl6_tiles == 2 ? 3 : 7, 3));
        if (rl6_tiles == 2) { a5_seg(xa, ya - h10, xa, ya - nb10, A5C(12, 4)); }
      }
      if (rl6_exists(i, j + 1)) { nb01 = rl6_corner(i, j + 1, 0, 0); nb11b = rl6_corner(i, j + 1, 1, 0); } else { nb01 = h01 - 40; nb11b = h11 - 40; }
      if (nb01 < h01 || nb11b < h11) {
        int xa = x0 - 16, ya = y0 + 8, xb = x0, yb = y0 + 16;
        a5_quad(xa, ya - h01, xb, yb - h11, xb, yb - nb11b, xa, ya - nb01, A5C(rl6_tiles == 2 ? 2 : 10, 3));
        if (rl6_tiles == 2) { a5_seg(xa, ya - h01, xa, ya - nb01, A5C(12, 4)); }
      }
      // the top
      int gx = h10 + h11 - h00 - h01, gy = h01 + h11 - h00 - h10;
      int lev = v3_clamp(11 - (gx + gy) / 6 + (gx - gy) / 10, 5, 15);
      int hue = (rl6_tiles == 3) ? (1 + (dg & 1)) : (1 + ((i ^ j) & 1));
      bool goal = dc >= RL6_GOAL && dc <= RL6_GOAL + 1;
      if (goal) hue = 7, lev = ((i ^ j) & 1) ? 15 : 3;
      if (rl6_tiles == 2 && !goal) lev = 1;
      a5_quad(x0, y0 - h00, x0 + 16, y0 + 8 - h10, x0, y0 + 16 - h11, x0 - 16, y0 + 8 - h01, A5C(lev, hue));
      if (rl6_tiles == 0 || rl6_tiles == 2) {
        uint8_t gc = rl6_tiles == 2 ? A5C(15, 4) : A5C(9, 4);
        a5_seg(x0, y0 - h00, x0 + 16, y0 + 8 - h10, gc); a5_seg(x0, y0 - h00, x0 - 16, y0 + 8 - h01, gc);
        if (rl6_tiles == 2) { a5_seg(x0 + 16, y0 + 8 - h10, x0, y0 + 16 - h11, gc); a5_seg(x0 - 16, y0 + 8 - h01, x0, y0 + 16 - h11, gc); }
      }
    }
    // springs on this diagonal
    for (int k = 0; k < rl6_nj; k++) if ((int)(rl6_jx[k] + rl6_jy[k]) == dg) {
      float h = 0; rl6_ground(rl6_jx[k], rl6_jy[k], &h);
      float hop = sinf(rl6_jt[k] * 3.14159f) * 16;
      int sx = rl6_sx(rl6_jx[k], rl6_jy[k]), sy = rl6_sy(rl6_jx[k], rl6_jy[k], h);
      v3_disc(sx, sy, 3, A5C(2, 11));
      int top = sy - 4 - (int)hop;
      for (int q = 0; q < 6; q++) a5_seg(sx - 3, top + q * 2 + 2, sx + 3, top + q * 2 + 3, A5C(q & 1 ? 9 : 14, 9));
      v3_disc(sx, top, 3, A5C(15, 9)); v3_put(sx - 1, top - 1, A5C(15, 0)); v3_put(sx + 1, top - 1, A5C(15, 0));
    }
  }
  while (nextM < no) { rl6_drawMarble(order[nextM], 9); nextM++; }
  // shadow under an airborne marble
  if (rl6_air[0] && rl6_lost <= 0) { float h; if (rl6_ground(rl6_x[0], rl6_y[0], &h)) { int sx = rl6_sx(rl6_x[0], rl6_y[0]), sy = rl6_sy(rl6_x[0], rl6_y[0], h); for (int q = -4; q <= 4; q++) v3_put(sx + q, sy, A5C(1, 11)); } }
  a5_partDraw();

  a5_post(rl6_sx(rl6_x[0], rl6_y[0]), rl6_sy(rl6_x[0], rl6_y[0], rl6_z[0]));
  a5_hud(rl6_score, rl6_hi, A5C(15, 8), A5C(15, 0));
  char s[24];
  snprintf(s, 24, "TIME %d", (int)ceilf(rl6_time)); a5_textS(W - 60, 2, s, 1, rl6_time < 10 && ((int)(globalTime * 4) & 1) ? A5C(15, 7) : A5C(15, 8));
  snprintf(s, 24, "RACE %d", rl6_course); a5_textS(W - 60, 12, s, 1, A5C(12, 0));
  if (rl6_banner > 0) { snprintf(s, 24, "RACE %d", rl6_course); a5_banner(H / 2 - 30, s, 2, A5C(15, 8)); if (rl6_banner < 80) a5_banner(H / 2 - 4, "GO!", 2, A5C(15, 7)); }
  if (rl6_goalT > 0) a5_banner(H / 2 - 30, "GOAL!", 3, A5C(15, 8));
  if (rl6_over > 0) a5_banner(H / 2 - 8, "TIME UP", 2, A5C(15, 7));
  a5_crt();
}
void prog_roller_draw(int preset) { roller5_draw(preset); fx5_apply(); }

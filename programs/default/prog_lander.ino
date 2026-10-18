// =====================================================================
// PROGRAM: LANDER — set a spidery craft down on the moon (by Dewey)
// Set 5 "Arcade", slot 7
//
// In the spirit of the 1979 vector moon-landing cabinet: a little lander
// falls towards jagged mountains. Tilt it, burn fuel to slow down, and set
// it down gently on one of the flat pads — the narrow ones score 2×, 3×,
// 4× or 5×. Come in too fast or crooked and it breaks apart. The view
// zooms in as you get close to the ground, like the original. Run out of
// fuel and it's over.
//
// It plays itself; touch p0/p1 or k12 and you fly it.
//
// Presets:
//   k0 Classic   k1 Green Phosphor   k2 Amber   k3 Mars   k4 Heavy Moon
//   k5 Asteroid (barely any gravity)   k6 Neon   k7 Ice Moon   k8 Starry Night
//   k9 Canyon (deep and jagged)   k10 Sunset   k11 Tour — a new preset every landing
//   k12 — ABORT (full burn, straight up). Joins the game.
//
// Knobs:
//   p0 Tilt (the knob's angle is the lander's angle; touch to play)
//   p1 Thrust (◄ off … full ►)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Gravity (centre = preset)   p8 Mountains (gentle … jagged)
//   p9 Landing Pads (few … many)   p10 Fuel (centre = preset)
//   p11 Line Glow (thin … thick halo)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 terrain, 2 lander, 3 flame, 4 pads, 5 HUD, 6 stars, 7 sky glow, 8 debris
struct Ld6Pre { uint8_t ground[3], craft[3], pad[3]; uint8_t look, grav, rough, pads, fuel, glow, trails, stars, fill; };
static const Ld6Pre ld6_pre[11] = {
  {{255,255,255},{255,255,255},{255,255,255}, A5_ARCADE, 10, 50, 50, 50, 1, 60, 0, 0},
  {{255,255,255},{255,255,255},{255,255,255}, A5_PHOSPHOR, 10, 50, 50, 50, 1, 160, 0, 0},
  {{255,255,255},{255,255,255},{255,255,255}, A5_AMBER, 10, 50, 50, 50, 1, 160, 0, 0},
  {{230,90,40},{230,230,240},{255,220,80}, A5_ARCADE, 14, 55, 50, 50, 1, 60, 1, 1},
  {{170,170,190},{255,220,120},{120,220,255}, A5_ARCADE, 20, 50, 60, 70, 1, 60, 1, 1},
  {{160,150,140},{120,220,255},{255,200,80}, A5_ARCADE, 4, 70, 50, 40, 1, 60, 1, 1},
  {{255,0,200},{0,255,255},{255,255,0}, A5_NEON, 10, 55, 50, 50, 2, 140, 1, 0},
  {{200,230,255},{255,120,120},{120,255,200}, A5_ARCADE, 9, 45, 50, 50, 1, 60, 1, 1},
  {{150,160,220},{255,255,255},{255,230,120}, A5_ARCADE, 10, 50, 50, 50, 1, 80, 2, 1},
  {{220,140,90},{220,240,255},{120,255,140}, A5_ARCADE, 11, 95, 40, 55, 1, 60, 1, 1},
  {{120,50,90},{255,230,200},{255,170,60}, A5_SUNSET, 10, 50, 50, 50, 1, 80, 1, 1},
};
static const char* const ld6_names[12] = { "Classic", "Green Phosphor", "Amber", "Mars", "Heavy Moon", "Asteroid", "Neon", "Ice Moon", "Starry Night", "Canyon", "Sunset", "Tour" };
static const char* const ld6_lbl[16] = { "Tilt", "Thrust", "", "", "", "", "", "Gravity", "Mountains", "Landing Pads", "Fuel", "Line Glow", "", "", "", "" };
const char* prog_lander_name() { return "LANDER"; }
const char* prog_lander_character() { return "Set a vector moon-lander down on a narrow pad - plays itself until you touch p0/p1/k12"; }
const char* prog_lander_presetName(int preset) { return (preset >= 0 && preset < 12) ? ld6_names[preset] : NULL; }
const char* prog_lander_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, ld6_lbl); }
uint8_t prog_lander_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define LD6_WW 960                      // the world wraps every 960 px
#define LD6_NP 121                      // terrain points, 8 px apart
#define LD6_NPAD 8
static int16_t ld6_th[LD6_NP];          // terrain height (world y, 0 = top, bigger = lower)
static int ld6_padA[LD6_NPAD], ld6_padB[LD6_NPAD], ld6_padM[LD6_NPAD], ld6_np = 0;   // pads: first and last point, multiplier
static float ld6_x = 100, ld6_y = 40, ld6_vx = 0.6f, ld6_vy = 0, ld6_a = 0, ld6_thrust = 0, ld6_fuel = 1000;
static float ld6_camX = 0, ld6_camY = 0, ld6_zoom = 1, ld6_state = 0, ld6_stateT = 0, ld6_time = 0, ld6_g = 0.01f, ld6_banner = 0;
static int ld6_lastPts = 0;
static int ld6_preset = -1, ld6_look = 0, ld6_target = -1, ld6_lastMult = 0, ld6_landings = 0;
static uint32_t ld6_score = 0, ld6_hi = 3000;
static float ld6_diff = 0.4f, ld6_aiErr = 0, ld6_aiA = 0;
static float ld6_dbx[6], ld6_dby[6], ld6_dbvx[6], ld6_dbvy[6], ld6_dba[6];   // debris

static inline int ld6_wrapi(int i) { return ((i % (LD6_NP - 1)) + (LD6_NP - 1)) % (LD6_NP - 1); }
static float ld6_groundAt(float x) {
  x = fmodf(x, (float)LD6_WW); if (x < 0) x += LD6_WW;
  int i = (int)(x / 8); float f = (x - i * 8) / 8.0f;
  return ld6_th[ld6_wrapi(i)] * (1 - f) + ld6_th[ld6_wrapi(i + 1)] * f;
}
static void ld6_makeTerrain(int rough, int pads) {
  uint32_t seed = v3_rand();
  for (int i = 0; i < LD6_NP - 1; i++) {
    float h = 170 + 30 * sinf(i * 0.11f + (seed & 255)) + 22 * sinf(i * 0.37f + ((seed >> 8) & 255));
    h += (int)(v3_hash(seed + i * 977) % 40 - 20) * rough / 50.0f;
    if (rough > 70) h += ((i * 7 + (seed & 15)) % 19 < 4) ? -40 * (rough - 70) / 30.0f : 0;
    ld6_th[i] = (int16_t)v3_clamp((int)h, 80, 228);
  }
  // pads: flat runs of 2 (5×) to 5 (2×) segments
  ld6_np = 0;
  int n = v3_clamp(2 + pads / 14, 2, LD6_NPAD);
  for (int k = 0; k < n; k++) {
    int len = 2 + v3_rnd(4), a = (k * (LD6_NP - 1)) / n + v3_rnd(6), b = a + len;
    if (b >= LD6_NP - 1) continue;
    int h = ld6_th[a];
    for (int i = a; i <= b; i++) ld6_th[i] = (int16_t)h;
    ld6_padA[ld6_np] = a; ld6_padB[ld6_np] = b; ld6_padM[ld6_np] = len >= 5 ? 2 : (len == 4 ? 3 : (len == 3 ? 4 : 5)); ld6_np++;
  }
  ld6_th[LD6_NP - 1] = ld6_th[0];
}
static void ld6_newDescent() {
  ld6_x = v3_rndf() * LD6_WW; ld6_y = 30; ld6_vx = (v3_rndf() - 0.5f) * 1.2f; ld6_vy = 0; ld6_a = 0;
  ld6_state = 0; ld6_stateT = 0; ld6_target = -1; ld6_aiErr = (v3_rndf() - 0.5f) * (1.0f - ld6_diff);
  ld6_banner = 1.2f;
}
static void ld6_newGame(int rough, int pads, float fuel) { ld6_score = 0; ld6_fuel = fuel; ld6_time = 0; ld6_landings = 0; ld6_makeTerrain(rough, pads); ld6_newDescent(); }
static void ld6_crash() {
  ld6_state = 2; ld6_stateT = 3.0f; ld6_fuel -= 150; a5_shake(5);
  for (int k = 0; k < 6; k++) { ld6_dbx[k] = ld6_x; ld6_dby[k] = ld6_y; float a = v3_rndf() * 3.1416f + 3.1416f; ld6_dbvx[k] = cosf(a) * (0.5f + v3_rndf()) + ld6_vx * 0.3f; ld6_dbvy[k] = sinf(a) * (1 + v3_rndf()); ld6_dba[k] = v3_rndf() * 6; }
}

// the computer pilot: pick a pad, drift over it, then a careful suicide burn
static void ld6_ai(float* angle, float* thrust) {
  if (ld6_target < 0) {
    float best = 1e9f;
    for (int k = 0; k < ld6_np; k++) {
      float px = (ld6_padA[k] + ld6_padB[k]) * 4.0f, dx = px - fmodf(ld6_x + LD6_WW, LD6_WW);
      if (dx > LD6_WW / 2) dx -= LD6_WW; if (dx < -LD6_WW / 2) dx += LD6_WW;
      float cost = fabsf(dx) - ld6_padM[k] * 40;
      if (cost < best) { best = cost; ld6_target = k; }
    }
  }
  int k = ld6_target;
  float px = (ld6_padA[k] + ld6_padB[k]) * 4.0f + ld6_aiErr * 4, dx = px - fmodf(ld6_x + LD6_WW, LD6_WW);
  if (dx > LD6_WW / 2) dx -= LD6_WW; if (dx < -LD6_WW / 2) dx += LD6_WW;
  float alt = ld6_th[ld6_padA[k]] - ld6_y - 6;
  float wantVx = v3_clampf(dx * 0.012f, -1.2f, 1.2f);
  if (fabsf(dx) < 6) wantVx = dx * 0.05f;
  float ax = (wantVx - ld6_vx) * 0.09f;                              // horizontal: tilt into it
  float wantVy = alt > 60 ? 1.0f : 0.15f + alt * 0.012f;              // vertical: slow down near the ground
  if (fabsf(dx) > 30 && alt < 50) wantVy = -0.2f;                     // not over the pad yet: hold height
  ax = v3_clampf(ax, -0.015f, 0.015f);
  float ay = v3_clampf((ld6_vy - wantVy) * 0.3f + ld6_g, 0.0f, 0.04f);   // we can only push upwards
  *angle = v3_clampf(atan2f(ax, ay), -1.2f, 1.2f);
  if (alt < 14) *angle = v3_clampf(*angle, -0.12f, 0.12f);
  *thrust = v3_clampf(sqrtf(ax * ax + ay * ay) / 0.032f, 0, 1);
}

void prog_lander_init() { v3_reset(); a5_reset(); ld6_preset = -1; a5_partClear(); }

static void lander5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  const Ld6Pre& P0 = ld6_pre[preset == 11 ? 0 : preset];
  int rough = v3_centre(8) ? P0.rough : (int)(v3_u(8) * 100);
  int pads = v3_centre(9) ? P0.pads : (int)(v3_u(9) * 100);
  float fuel0 = v3_centre(10) ? P0.fuel * 20.0f : 200 + v3_u(10) * 2800;
  if (preset != ld6_preset) { ld6_preset = preset; ld6_look = preset == 11 ? 0 : preset; ld6_newGame(rough, pads, fuel0); }
  const Ld6Pre& P = ld6_pre[ld6_look];
  ld6_diff = a5_diffK(0.4f);
  ld6_g = (v3_centre(7) ? P.grav : 1 + v3_u(7) * 25) * 0.001f;
  int glow = v3_centre(11) ? P.glow : (int)(v3_u(11) * 2.99f);
  bool halo = glow == 2;

  uint8_t hues[9][3] = { {255,255,255},{0},{0},{255,170,60},{0},{255,255,255},{200,210,255},{60,30,80},{255,200,120} };
  memcpy(hues[1], P.ground, 3); memcpy(hues[2], P.craft, 3); memcpy(hues[4], P.pad, 3);
  a5_palette(hues, 9, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (ld6_banner > 0) ld6_banner -= dt;
    if (ld6_state != 0) {                                            // landed or crashed: a pause, then go again
      ld6_stateT -= dt;
      for (int k = 0; k < 6; k++) { ld6_dbx[k] += ld6_dbvx[k]; ld6_dby[k] += ld6_dbvy[k]; ld6_dbvy[k] += ld6_g; ld6_dba[k] += 0.1f; float gy = ld6_groundAt(ld6_dbx[k]); if (ld6_dby[k] > gy) { ld6_dby[k] = gy; ld6_dbvy[k] *= -0.3f; ld6_dbvx[k] *= 0.6f; } }
      if (ld6_stateT <= 0) {
        if (ld6_state == 3 || ld6_fuel <= 0) { if (ld6_score > ld6_hi) ld6_hi = ld6_score; if (ld6_preset == 11) ld6_look = (ld6_look + 1) % 11; ld6_newGame(rough, pads, fuel0); }
        else { if (ld6_state == 1 && ld6_preset == 11) ld6_look = (ld6_look + 1) % 11; if (ld6_state == 1) ld6_makeTerrain(rough, pads); ld6_newDescent(); }
      }
      if (ld6_fuel <= 0 && ld6_state != 3 && ld6_stateT <= 0.5f) { ld6_state = 3; ld6_stateT = 3.0f; }
      continue;
    }
    ld6_time += dt;
    float wantA, thr;
    if (a5_player) {
      wantA = (v3_u(0) - 0.5f) * 3.1416f;
      thr = v3_clampf(a5_stick(1), 0, 1);
      if (a5_fireHeld) { wantA = 0; thr = 1; }
    } else { ld6_ai(&wantA, &thr); ld6_aiA += (wantA - ld6_aiA) * 0.15f; wantA = ld6_aiA; }
    ld6_a += v3_clampf(wantA - ld6_a, -0.05f, 0.05f);
    if (ld6_fuel <= 0) thr = 0;
    ld6_thrust += (thr - ld6_thrust) * 0.3f;
    float acc = ld6_thrust * 0.032f;
    ld6_vx += sinf(ld6_a) * acc; ld6_vy += -cosf(ld6_a) * acc + ld6_g;
    ld6_fuel -= ld6_thrust * 0.9f;
    ld6_x += ld6_vx; ld6_y += ld6_vy;
    if (ld6_y < 14) { ld6_y = 14; ld6_vy = 0; }
    if (ld6_x < 0) ld6_x += LD6_WW; if (ld6_x >= LD6_WW) ld6_x -= LD6_WW;
    // touching down? check both feet
    float fl = ld6_groundAt(ld6_x - 5), fr = ld6_groundAt(ld6_x + 5), fc = ld6_groundAt(ld6_x);
    if (ld6_y + 6 >= fl || ld6_y + 6 >= fr || ld6_y + 2 >= fc) {
      int pad = -1;
      for (int k = 0; k < ld6_np; k++) { float a = ld6_padA[k] * 8.0f, b = ld6_padB[k] * 8.0f; if (ld6_x - 5 >= a && ld6_x + 5 <= b) pad = k; }
      bool soft = ld6_vy < 0.55f && fabsf(ld6_vx) < 0.45f && fabsf(ld6_a) < 0.2f;
      if (pad >= 0 && soft) {
        int m = ld6_padM[pad]; ld6_lastMult = m;
        int pts = (ld6_vy < 0.25f ? 50 : 15) * m;
        ld6_score += pts; ld6_lastPts = pts; ld6_fuel += 50; ld6_landings++;
        ld6_state = 1; ld6_stateT = 3.0f; ld6_y = ld6_th[ld6_padA[pad]] - 6; ld6_vx = ld6_vy = 0; ld6_a = 0;
        a5_burst((int)((ld6_x - ld6_camX) * ld6_zoom + W / 2), (int)((ld6_y + 6 - ld6_camY) * ld6_zoom + H / 2), 20, 3, 25, 30);
      } else { ld6_crash(); ld6_lastMult = 0; }
    }
  }

  // ── camera: zoom in close to the ground ──
  float alt = ld6_groundAt(ld6_x) - ld6_y;
  float zt = alt < 70 ? 2.2f : (alt < 110 ? 1.5f : 1.0f);
  ld6_zoom += (zt - ld6_zoom) * 0.06f;
  ld6_camX = ld6_x;
  float cy = ld6_zoom > 1.2f ? ld6_y + 30 : 120;
  ld6_camY += (cy - ld6_camY) * 0.1f;
  float z = ld6_zoom;
  #define LD6_SX(wx) ((int)(((wx) - ld6_camX) * z * 8) + W / 2 * 8)
  #define LD6_SY(wy) ((int)(((wy) - ld6_camY) * z * 8) + H / 2 * 8)

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (P.stars) a5_stars(P.stars == 2 ? 120 : 50, 0, 0, 6, 31);
  // terrain: work out which points are in view (with wrap-around)
  float halfW = W / 2 / z + 10;
  int i0 = (int)floorf((ld6_camX - halfW) / 8), i1 = (int)ceilf((ld6_camX + halfW) / 8);
  int px = 0, py = 0;
  for (int i = i0; i <= i1; i++) {
    float wx = i * 8.0f; int h = ld6_th[ld6_wrapi(i)];
    int sx = LD6_SX(wx), sy = LD6_SY(h);
    if (i > i0) {
      bool pad = false; int m = 0;
      for (int k = 0; k < ld6_np; k++) { int a = ld6_padA[k], b = ld6_padB[k], ii = ld6_wrapi(i - 1); if (ii >= a && ii < b) { pad = true; m = ld6_padM[k]; } }
      if (P.fill) {                                                    // solid ground under the line
        int xa = px >> 3, xb = sx >> 3, ya = py >> 3, yb = sy >> 3;
        for (int x = a5_max(xa, 0); x <= a5_min(xb, W - 1); x++) { int y = xb == xa ? ya : ya + (yb - ya) * (x - xa) / (xb - xa); for (int yy = a5_max(y + 1, 0); yy < H; yy += 2) v3_put(x, yy, A5C(3 + ((yy - y) < 6 ? 2 : 0), 1)); }
      }
      if (pad) a5_glow(px, py, sx, sy, 15, 4, true); else a5_glow(px, py, sx, sy, 15, 1, halo);
      (void)m;
    }
    px = sx; py = sy;
  }
  // multipliers under each pad
  for (int k = 0; k < ld6_np; k++) {
    float wx = (ld6_padA[k] + ld6_padB[k]) * 4.0f;
    float dx = wx - ld6_camX; if (dx > LD6_WW / 2) wx -= LD6_WW; if (dx < -LD6_WW / 2) wx += LD6_WW;
    int sx = LD6_SX(wx) >> 3, sy = (LD6_SY(ld6_th[ld6_padA[k]]) >> 3) + 4;
    char s[4] = { (char)('0' + ld6_padM[k]), 'x', 0, 0 };
    if (sx > -10 && sx < W + 10) a5_text(sx - 5, sy, s, 1, A5C(15, 4));
  }
  // the lander (or its pieces)
  if (ld6_state == 2 || ld6_state == 3) {
    for (int k = 0; k < 6; k++) { float c = cosf(ld6_dba[k]) * 5, s = sinf(ld6_dba[k]) * 5; v3_lineAA(LD6_SX(ld6_dbx[k] - c), LD6_SY(ld6_dby[k] - s), LD6_SX(ld6_dbx[k] + c), LD6_SY(ld6_dby[k] + s), 15, 2); }
  } else {
    float c = cosf(ld6_a), s = sinf(ld6_a);
    #define LD6_P(fx, fy, X, Y) int X = LD6_SX(ld6_x + (fx) * c - (fy) * s), Y = LD6_SY(ld6_y + (fx) * s + (fy) * c);
    LD6_P(-3, -6, a1x, a1y) LD6_P(3, -6, a2x, a2y) LD6_P(4, -3, a3x, a3y) LD6_P(3, 0, a4x, a4y) LD6_P(-3, 0, a5x, a5y) LD6_P(-4, -3, a6x, a6y)
    LD6_P(-4, 0, b1x, b1y) LD6_P(4, 0, b2x, b2y) LD6_P(-4, 2, b3x, b3y) LD6_P(4, 2, b4x, b4y)
    LD6_P(-3, 2, l1x, l1y) LD6_P(-6, 6, l2x, l2y) LD6_P(3, 2, r1x, r1y) LD6_P(6, 6, r2x, r2y)
    LD6_P(-7, 6, f1x, f1y) LD6_P(-5, 6, f2x, f2y) LD6_P(5, 6, f3x, f3y) LD6_P(7, 6, f4x, f4y)
    a5_glow(a1x, a1y, a2x, a2y, 15, 2, halo); a5_glow(a2x, a2y, a3x, a3y, 15, 2, halo); a5_glow(a3x, a3y, a4x, a4y, 15, 2, halo);
    a5_glow(a4x, a4y, a5x, a5y, 15, 2, halo); a5_glow(a5x, a5y, a6x, a6y, 15, 2, halo); a5_glow(a6x, a6y, a1x, a1y, 15, 2, halo);
    a5_glow(b1x, b1y, b2x, b2y, 15, 2, halo); a5_glow(b2x, b2y, b4x, b4y, 15, 2, halo); a5_glow(b4x, b4y, b3x, b3y, 15, 2, halo); a5_glow(b3x, b3y, b1x, b1y, 15, 2, halo);
    a5_glow(l1x, l1y, l2x, l2y, 15, 2, halo); a5_glow(r1x, r1y, r2x, r2y, 15, 2, halo);
    v3_lineAA(f1x, f1y, f2x, f2y, 15, 2); v3_lineAA(f3x, f3y, f4x, f4y, 15, 2);
    if (ld6_thrust > 0.05f && ld6_state == 0) {
      float len = 4 + ld6_thrust * 10 + v3_rndf() * 4 * ld6_thrust;
      LD6_P(-2, 3, n1x, n1y) LD6_P(2, 3, n2x, n2y) LD6_P(0, 3 + len, tx, ty)
      v3_lineAA(n1x, n1y, tx, ty, 15, 3); v3_lineAA(n2x, n2y, tx, ty, 15, 3);
    }
  }
  a5_partDraw();

  a5_post(W / 2, H / 2);
  // readouts
  char s[32];
  uint8_t c1 = A5C(15, 5), c2 = A5C(11, 5);
  snprintf(s, 32, "SCORE %04u", (unsigned)(ld6_score % 10000)); a5_textS(6, 4, s, 1, c1);
  snprintf(s, 32, "TIME  %d:%02d", (int)ld6_time / 60, (int)ld6_time % 60); a5_textS(6, 14, s, 1, c2);
  snprintf(s, 32, "FUEL  %04d", (int)a5_max(0, (int)ld6_fuel)); a5_textS(6, 24, s, 1, ld6_fuel < 150 && ((int)(globalTime * 4) & 1) ? A5C(15, 3) : c2);
  snprintf(s, 32, "ALTITUDE %4d", (int)a5_max(0, (int)(alt - 6) * 2)); a5_textS(W - 112, 4, s, 1, c1);
  snprintf(s, 32, "HORIZ SPEED %3d %c", (int)(fabsf(ld6_vx) * 60), ld6_vx >= 0 ? '>' : '<'); a5_textS(W - 112, 14, s, 1, c2);
  snprintf(s, 32, "VERT SPEED  %3d %c", (int)(fabsf(ld6_vy) * 60), ld6_vy >= 0 ? '*' : '+'); a5_textS(W - 112, 24, s, 1, c2);
  if (a5_player) a5_textS(W / 2 - 9, 4, "1UP", 1, c1); else if (((int)(globalTime * 2.5f)) & 1) a5_textS(W / 2 - 12, 4, "DEMO", 1, c1);
  if (a5_join > 0) a5_textC(H / 2 - 30, "PLAYER 1", 2, c1);
  if (ld6_state == 1) { snprintf(s, 32, "LANDED  %d POINTS", ld6_lastPts); a5_banner(H / 2 - 50, "THE EAGLE IS DOWN", 1, c1); a5_banner(H / 2 - 36, s, 1, c2); }
  if (ld6_state == 2) a5_banner(H / 2 - 50, "CRASHED - LOST 150 FUEL", 1, A5C(15, 3));
  if (ld6_state == 3 || (ld6_fuel <= 0 && ld6_state == 2)) a5_banner(H / 2 - 30, "OUT OF FUEL", 2, A5C(15, 3));
  if (ld6_banner > 0 && ld6_state == 0) a5_banner(H / 2 - 50, "LAND ON A PAD", 1, c2);
  a5_crt();
}
void prog_lander_draw(int preset) { lander5_draw(preset); fx5_apply(); }

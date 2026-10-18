// =====================================================================
// PROGRAM: TANKS — a wireframe tank battle, seen from the gunner's seat
// (by Dewey) — Set 5 "Arcade", slot 8
//
// In the spirit of the 1980 first-person vector tank cabinet: you look out
// of a tank's periscope across a flat plain scattered with blocks and
// pyramids, mountains and a smoking volcano on the horizon, a moon in the
// sky. Enemy tanks prowl and fire; the radar at the top shows where they
// are. Get one in your sights and fire. Take a hit and the glass cracks.
// Turn up the ground grid for a neon synth-wave plain.
//
// It plays itself; touch p0/p1 or k12 and you drive.
//
// Presets:
//   k0 Classic Green   k1 Red Alert   k2 Amber   k3 Cyan Night   k4 Desert Dusk
//   k5 Arctic   k6 Neon Grid   k7 Tank Swarm   k8 Phosphor (long trails)
//   k9 Moonbase (lots of cover)   k10 Thermal   k11 Tour — a new preset every life
//   k12 — FIRE. Joins the game.
//
// Knobs:
//   p0 Steer ◄ left | right ►   p1 Drive ◄ reverse | forward ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Enemy Tanks (centre = preset)   p8 Obstacles (few … many)
//   p9 Field of View (narrow … wide)   p10 Ground (centre = preset: bare, dots, grid, rolling grid)
//   p11 Line Glow (thin … thick halo)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 world lines, 2 enemies, 3 mountains, 4 HUD, 5 radar, 6 shells, 7 volcano, 8 moon, 9 ground grid, 10 sky glow, 11 bang
struct Tk6Pre { uint8_t world[3], enemy[3], mount[3], grid[3]; uint8_t look, tanks, obst, ground, glow, trails, sun; };
static const Tk6Pre tk6_pre[11] = {
  {{60,255,60},{255,60,60},{60,255,60},{30,140,30}, A5_ARCADE, 1, 40, 0, 1, 90, 0},
  {{255,60,40},{255,255,255},{255,80,40},{140,30,20}, A5_ARCADE, 1, 40, 1, 1, 90, 0},
  {{255,180,40},{255,255,200},{255,160,40},{140,90,20}, A5_ARCADE, 1, 40, 1, 1, 120, 0},
  {{0,220,255},{255,80,200},{0,160,255},{0,80,140}, A5_ARCADE, 2, 40, 1, 1, 110, 0},
  {{255,200,120},{255,80,60},{200,90,60},{160,80,40}, A5_ARCADE, 2, 35, 2, 1, 90, 1},
  {{220,240,255},{255,90,90},{160,200,255},{100,140,200}, A5_ARCADE, 2, 40, 1, 1, 90, 0},
  {{0,255,255},{255,255,0},{255,0,200},{255,0,200}, A5_NEON, 2, 30, 3, 2, 140, 1},
  {{60,255,60},{255,60,60},{60,255,60},{30,140,30}, A5_ARCADE, 5, 40, 0, 1, 90, 0},
  {{60,255,60},{255,255,255},{60,255,60},{30,140,30}, A5_PHOSPHOR, 2, 40, 1, 1, 200, 0},
  {{200,200,220},{255,120,60},{150,150,170},{90,90,110}, A5_ARCADE, 2, 90, 1, 1, 90, 0},
  {{220,220,220},{255,255,255},{160,160,160},{100,100,100}, A5_THERMAL, 2, 40, 2, 1, 110, 0},
};
static const char* const tk6_names[12] = { "Classic Green", "Red Alert", "Amber", "Cyan Night", "Desert Dusk", "Arctic", "Neon Grid", "Tank Swarm", "Phosphor", "Moonbase", "Thermal", "Tour" };
static const char* const tk6_lbl[16] = { "Steer: Left|Right", "Drive: Reverse|Forward", "", "", "", "", "", "Enemy Tanks", "Obstacles", "Field of View", "Ground", "Line Glow", "", "", "", "" };
const char* prog_tanks_name() { return "TANKS"; }
const char* prog_tanks_character() { return "First-person wireframe tank battle with radar, mountains and a volcano - plays itself until you touch p0/p1/k12"; }
const char* prog_tanks_presetName(int preset) { return (preset >= 0 && preset < 12) ? tk6_names[preset] : NULL; }
const char* prog_tanks_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, tk6_lbl); }
uint8_t prog_tanks_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define TK6_WS 4000.0f                 // the world wraps every 4000 units
#define TK6_NO 40
#define TK6_NE 6
#define TK6_HOR 112
static float tk6_ox[TK6_NO], tk6_oz[TK6_NO], tk6_oa[TK6_NO]; static uint8_t tk6_ot[TK6_NO];   // obstacles: 0 cube 1 pyramid 2 tall block
static int tk6_no = 0;
static float tk6_ex[TK6_NE], tk6_ez[TK6_NE], tk6_ea[TK6_NE], tk6_eFire[TK6_NE], tk6_eTurn[TK6_NE]; static bool tk6_eon[TK6_NE];
static float tk6_sx[TK6_NE + 1], tk6_sz[TK6_NE + 1], tk6_svx[TK6_NE + 1], tk6_svz[TK6_NE + 1], tk6_st[TK6_NE + 1];   // shells: [0] is yours
static float tk6_px = 0, tk6_pz = 0, tk6_pa = 0, tk6_dead = 0, tk6_crackX = 160, tk6_crackY = 120, tk6_f = 200, tk6_ufoX = 0, tk6_ufoZ = 0, tk6_ufoT = 20;
static bool tk6_ufo = false;
static int tk6_lives = 3, tk6_preset = -1, tk6_look = 0, tk6_kills = 0, tk6_over = 0;
static uint32_t tk6_score = 0, tk6_hi = 100000;
static float tk6_diff = 0.4f, tk6_aiTurn = 0, tk6_aiMove = 0, tk6_aiT = 0, tk6_overT = 0, tk6_shake = 0;
static bool tk6_halo = false;

static inline float tk6_wrapd(float d) { while (d > TK6_WS / 2) d -= TK6_WS; while (d < -TK6_WS / 2) d += TK6_WS; return d; }
static inline float tk6_angd(float a) { while (a > 3.14159f) a -= 6.28318f; while (a < -3.14159f) a += 6.28318f; return a; }
// a world point → camera space (right, up, forward)
static inline void tk6_cam(float wx, float wy, float wz, float* cx, float* cy, float* cz) {
  float dx = tk6_wrapd(wx - tk6_px), dz = tk6_wrapd(wz - tk6_pz), s = sinf(tk6_pa), c = cosf(tk6_pa);
  *cx = dx * c - dz * s; *cz = dx * s + dz * c; *cy = wy - 10;
}
// a 3-D line, clipped at the near plane, drawn smooth
static void tk6_line3(float ax, float ay, float az, float bx, float by, float bz, int lev, int hue) {
  const float nearZ = 6;
  if (az < nearZ && bz < nearZ) return;
  if (az < nearZ) { float t = (nearZ - az) / (bz - az); ax += (bx - ax) * t; ay += (by - ay) * t; az = nearZ; }
  if (bz < nearZ) { float t = (nearZ - bz) / (az - bz); bx += (ax - bx) * t; by += (ay - by) * t; bz = nearZ; }
  float x0 = 160 + tk6_f * ax / az, y0 = TK6_HOR - tk6_f * ay / az, x1 = 160 + tk6_f * bx / bz, y1 = TK6_HOR - tk6_f * by / bz;
  if ((x0 < -400 && x1 < -400) || (x0 > 720 && x1 > 720)) return;
  x0 = v3_clampf(x0, -900, 1200); x1 = v3_clampf(x1, -900, 1200); y0 = v3_clampf(y0, -900, 1100); y1 = v3_clampf(y1, -900, 1100);
  a5_glow((int)(x0 * 8), (int)(y0 * 8), (int)(x1 * 8), (int)(y1 * 8), lev, hue, tk6_halo);
}
// a model: vertices (x, y, z) in its own space, edges as pairs; placed at (wx, wz), turned by a
static void tk6_model(const int8_t (*v)[3], int nv, const uint8_t (*e)[2], int ne, float wx, float wz, float a, float scale, int lev, int hue) {
  float cx[24], cy[24], cz[24], s = sinf(a), c = cosf(a);
  float dx = tk6_wrapd(wx - tk6_px), dz = tk6_wrapd(wz - tk6_pz);
  if (dx * dx + dz * dz > 1600.0f * 1600.0f) return;
  for (int i = 0; i < nv && i < 24; i++) {
    float mx = v[i][0] * scale, my = v[i][1] * scale, mz = v[i][2] * scale;
    float rx = mx * c + mz * s, rz = -mx * s + mz * c;
    tk6_cam(wx + rx, my, wz + rz, &cx[i], &cy[i], &cz[i]);
  }
  for (int k = 0; k < ne; k++) tk6_line3(cx[e[k][0]], cy[e[k][0]], cz[e[k][0]], cx[e[k][1]], cy[e[k][1]], cz[e[k][1]], lev, hue);
}
// ── models ──
static const int8_t tk6_vCube[8][3] = { {-15,0,-15},{15,0,-15},{15,0,15},{-15,0,15},{-15,30,-15},{15,30,-15},{15,30,15},{-15,30,15} };
static const uint8_t tk6_eCube[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
static const int8_t tk6_vPyr[5][3] = { {-18,0,-18},{18,0,-18},{18,0,18},{-18,0,18},{0,40,0} };
static const uint8_t tk6_ePyr[8][2] = { {0,1},{1,2},{2,3},{3,0},{0,4},{1,4},{2,4},{3,4} };
static const int8_t tk6_vTall[8][3] = { {-10,0,-10},{10,0,-10},{10,0,10},{-10,0,10},{-10,64,-10},{10,64,-10},{10,64,10},{-10,64,10} };
static const int8_t tk6_vTank[18][3] = {
  {-14,0,-20},{14,0,-20},{14,0,20},{-14,0,20},            // track base
  {-16,6,-24},{16,6,-24},{16,6,24},{-16,6,24},            // hull edge
  {-10,12,-14},{10,12,-14},{10,12,12},{-10,12,12},        // deck
  {-6,18,-6},{6,18,-6},{0,18,6},                          // turret
  {-1,15,6},{1,15,6},{0,15,34} };                         // gun
static const uint8_t tk6_eTank[25][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7},
  {8,9},{9,10},{10,11},{11,8},{4,8},{5,9},{6,10},{7,11},{12,13},{13,14},{14,12},{15,17},{16,17} };
static const int8_t tk6_vUfo[9][3] = { {-20,30,0},{-10,30,17},{10,30,17},{20,30,0},{10,30,-17},{-10,30,-17},{0,40,0},{0,22,0},{0,30,0} };
static const uint8_t tk6_eUfo[18][2] = { {0,1},{1,2},{2,3},{3,4},{4,5},{5,0},{0,6},{1,6},{2,6},{3,6},{4,6},{5,6},{0,7},{1,7},{2,7},{3,7},{4,7},{5,7} };

static bool tk6_blocked(float x, float z, float r) {
  for (int i = 0; i < tk6_no; i++) { float dx = tk6_wrapd(tk6_ox[i] - x), dz = tk6_wrapd(tk6_oz[i] - z); if (dx * dx + dz * dz < r * r) return true; }
  return false;
}
static void tk6_spawnEnemy(int i) {
  for (int tries = 0; tries < 20; tries++) {
    float a = v3_rndf() * 6.28f, d = 700 + v3_rndf() * 600;
    float x = tk6_px + sinf(a) * d, z = tk6_pz + cosf(a) * d;
    if (!tk6_blocked(x, z, 50)) { tk6_ex[i] = x; tk6_ez[i] = z; break; }
  }
  tk6_ea[i] = v3_rndf() * 6.28f; tk6_eon[i] = true; tk6_eFire[i] = 3 + v3_rndf() * 3; tk6_eTurn[i] = 0;
}
static void tk6_makeField(int obst) {
  tk6_no = v3_clamp(obst * TK6_NO / 100, 4, TK6_NO);
  for (int i = 0; i < tk6_no; i++) { tk6_ox[i] = v3_rndf() * TK6_WS; tk6_oz[i] = v3_rndf() * TK6_WS; tk6_oa[i] = v3_rndf() * 6.28f; tk6_ot[i] = (uint8_t)v3_rnd(3); }
}
static void tk6_newGame(int obst) {
  tk6_score = 0; tk6_lives = 3; tk6_kills = 0; tk6_over = 0; tk6_dead = 0;
  tk6_px = 0; tk6_pz = 0; tk6_pa = 0;
  tk6_makeField(obst);
  for (int i = 0; i < TK6_NE; i++) tk6_eon[i] = false;
  for (int i = 0; i <= TK6_NE; i++) tk6_st[i] = 0;
}
static void tk6_hitPlayer(float fromX, float fromZ) {
  if (tk6_dead > 0) return;
  tk6_dead = 3.0f; a5_shake(8);
  float ang = tk6_angd(atan2f(tk6_wrapd(fromX - tk6_px), tk6_wrapd(fromZ - tk6_pz)) - tk6_pa);
  tk6_crackX = 160 + v3_clampf(ang * 120, -110, 110); tk6_crackY = 110 + v3_rndf() * 40 - 20;
}

// the computer driver: turn to the nearest enemy (leading it), keep a good range, fire when lined up
static void tk6_ai() {
  int best = -1; float bd = 1e9f;
  for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) { float dx = tk6_wrapd(tk6_ex[i] - tk6_px), dz = tk6_wrapd(tk6_ez[i] - tk6_pz), d = dx * dx + dz * dz; if (d < bd) { bd = d; best = i; } }
  tk6_aiMove = 0.4f; tk6_aiTurn = 0.15f;
  if (best < 0) return;
  float dx = tk6_wrapd(tk6_ex[best] - tk6_px), dz = tk6_wrapd(tk6_ez[best] - tk6_pz), d = sqrtf(dx * dx + dz * dz);
  float lead = d / 12.0f * 1.1f;
  dx += sinf(tk6_ea[best]) * lead * 0.8f; dz += cosf(tk6_ea[best]) * lead * 0.8f;
  float want = atan2f(dx, dz), da = tk6_angd(want - tk6_pa) + (v3_rndf() - 0.5f) * 0.05f * (1.0f - tk6_diff);
  tk6_aiTurn = v3_clampf(da * 3.0f, -1, 1);
  tk6_aiMove = d > 450 ? 1.0f : (d < 220 ? -0.5f : 0.1f);
  // an enemy shell coming straight at us: swerve
  for (int s = 1; s <= TK6_NE; s++) if (tk6_st[s] > 0) {
    float sx = tk6_wrapd(tk6_sx[s] - tk6_px), sz = tk6_wrapd(tk6_sz[s] - tk6_pz);
    float along = -(sx * tk6_svx[s] + sz * tk6_svz[s]) / 12.0f;
    float miss = fabsf(sx * tk6_svz[s] - sz * tk6_svx[s]) / 12.0f;
    if (along > 0 && along < 260 && miss < 28) { tk6_aiTurn = 1.0f; tk6_aiMove = 1.0f; }
  }
}

void prog_tanks_init() { v3_reset(); a5_reset(); tk6_preset = -1; a5_partClear(); }

static void tanks5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  const Tk6Pre& P0 = tk6_pre[preset == 11 ? 0 : preset];
  int obst = v3_centre(8) ? P0.obst : (int)(v3_u(8) * 100);
  if (preset != tk6_preset) { tk6_preset = preset; tk6_look = preset == 11 ? 0 : preset; tk6_newGame(obst); }
  const Tk6Pre& P = tk6_pre[tk6_look];
  tk6_diff = a5_diffK(0.4f);
  int nTanks = v3_centre(7) ? P.tanks : 1 + (int)(v3_u(7) * 5.99f);
  tk6_f = v3_centre(9) ? 200 : 110 + v3_u(9) * 260;
  int ground = v3_pick(10, P.ground, 4);
  int glow = v3_centre(11) ? P.glow : (int)(v3_u(11) * 2.99f);
  tk6_halo = glow == 2;

  uint8_t hues[12][3] = { {255,255,255},{0},{0},{0},{255,255,255},{0},{255,255,255},{255,90,30},{255,250,220},{0},{0},{255,200,80} };
  memcpy(hues[1], P.world, 3); memcpy(hues[2], P.enemy, 3); memcpy(hues[3], P.mount, 3); memcpy(hues[5], P.world, 3); memcpy(hues[9], P.grid, 3);
  hues[4][0] = P.world[0]; hues[4][1] = P.world[1]; hues[4][2] = P.world[2];
  hues[10][0] = P.sun ? 255 : P.grid[0] / 3; hues[10][1] = P.sun ? 60 : P.grid[1] / 3; hues[10][2] = P.sun ? 120 : P.grid[2] / 3;
  a5_palette(hues, 12, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (tk6_over) { tk6_overT -= dt; if (tk6_overT <= 0) { if (tk6_preset == 11) tk6_look = (tk6_look + 1) % 11; tk6_newGame(obst); } continue; }
    // keep the right number of enemies around
    int alive = 0; for (int i = 0; i < TK6_NE; i++) alive += tk6_eon[i];
    for (int i = 0; i < nTanks && alive < nTanks; i++) if (!tk6_eon[i]) { tk6_spawnEnemy(i); alive++; }
    for (int i = nTanks; i < TK6_NE; i++) tk6_eon[i] = false;
    if (tk6_dead > 0) {
      tk6_dead -= dt;
      if (tk6_dead <= 0) {
        if (--tk6_lives < 0) { tk6_over = 1; tk6_overT = 4; if (tk6_score > tk6_hi) tk6_hi = tk6_score; }
        else { for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) tk6_spawnEnemy(i); for (int s = 0; s <= TK6_NE; s++) tk6_st[s] = 0; if (tk6_preset == 11) tk6_look = (tk6_look + 1) % 11; }
      }
      continue;
    }
    // driving
    float turn, move; bool fire;
    if (a5_player) { turn = a5_stick(0); move = a5_stick(1); fire = a5_fire; }
    else { tk6_aiT -= dt; if (tk6_aiT <= 0) { tk6_ai(); tk6_aiT = 0.1f; } turn = tk6_aiTurn; move = tk6_aiMove; fire = false;
      for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) { float dx = tk6_wrapd(tk6_ex[i] - tk6_px), dz = tk6_wrapd(tk6_ez[i] - tk6_pz); if (fabsf(tk6_angd(atan2f(dx, dz) - tk6_pa)) < 0.05f && dx * dx + dz * dz < 900 * 900) fire = true; } }
    tk6_pa = tk6_angd(tk6_pa + turn * 0.025f);
    float nx = tk6_px + sinf(tk6_pa) * move * 2.2f, nz = tk6_pz + cosf(tk6_pa) * move * 2.2f;
    if (!tk6_blocked(nx, nz, 34)) { tk6_px = nx; tk6_pz = nz; } else if (move != 0) a5_shake(1);
    tk6_px = fmodf(tk6_px + TK6_WS, TK6_WS); tk6_pz = fmodf(tk6_pz + TK6_WS, TK6_WS);
    if (fire && tk6_st[0] <= 0) { tk6_sx[0] = tk6_px; tk6_sz[0] = tk6_pz; tk6_svx[0] = sinf(tk6_pa) * 12; tk6_svz[0] = cosf(tk6_pa) * 12; tk6_st[0] = 1.6f; }
    // enemies
    for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) {
      float dx = tk6_wrapd(tk6_px - tk6_ex[i]), dz = tk6_wrapd(tk6_pz - tk6_ez[i]), d = sqrtf(dx * dx + dz * dz);
      float want = atan2f(dx, dz), da = tk6_angd(want - tk6_ea[i]);
      if (tk6_eTurn[i] > 0) { tk6_eTurn[i] -= dt; da = 1.0f; }
      tk6_ea[i] = tk6_angd(tk6_ea[i] + v3_clampf(da, -0.012f - tk6_diff * 0.01f, 0.012f + tk6_diff * 0.01f));
      float sp = d > 300 ? 1.1f + tk6_diff : 0.3f;
      float ex = tk6_ex[i] + sinf(tk6_ea[i]) * sp, ez = tk6_ez[i] + cosf(tk6_ea[i]) * sp;
      if (!tk6_blocked(ex, ez, 34)) { tk6_ex[i] = ex; tk6_ez[i] = ez; } else tk6_eTurn[i] = 0.8f;
      tk6_eFire[i] -= dt;
      if (tk6_eFire[i] <= 0 && fabsf(da) < 0.08f && d < 800 && tk6_st[1 + i] <= 0) {
        tk6_eFire[i] = 3.5f - tk6_diff * 2 + v3_rndf() * 2;
        tk6_sx[1 + i] = tk6_ex[i]; tk6_sz[1 + i] = tk6_ez[i]; tk6_svx[1 + i] = sinf(tk6_ea[i]) * 9; tk6_svz[1 + i] = cosf(tk6_ea[i]) * 9; tk6_st[1 + i] = 2.0f;
      }
    }
    // shells
    for (int s = 0; s <= TK6_NE; s++) if (tk6_st[s] > 0) {
      tk6_st[s] -= dt; tk6_sx[s] += tk6_svx[s]; tk6_sz[s] += tk6_svz[s];
      if (tk6_blocked(tk6_sx[s], tk6_sz[s], 18)) { tk6_st[s] = 0; continue; }
      if (s == 0) {
        for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) { float dx = tk6_wrapd(tk6_sx[0] - tk6_ex[i]), dz = tk6_wrapd(tk6_sz[0] - tk6_ez[i]);
          if (dx * dx + dz * dz < 26 * 26) { tk6_eon[i] = false; tk6_st[0] = 0; tk6_score += 1000; tk6_kills++;
            float cx, cy, cz; tk6_cam(tk6_ex[i], 10, tk6_ez[i], &cx, &cy, &cz); if (cz > 10) a5_burst((int)(160 + tk6_f * cx / cz), (int)(TK6_HOR - tk6_f * cy / cz), 40, 11, 40, 40); break; } }
        if (tk6_ufo && tk6_st[0] > 0) { float dx = tk6_wrapd(tk6_sx[0] - tk6_ufoX), dz = tk6_wrapd(tk6_sz[0] - tk6_ufoZ); if (dx * dx + dz * dz < 30 * 30) { tk6_ufo = false; tk6_score += 5000; tk6_st[0] = 0; tk6_ufoT = 20; } }
      } else {
        float dx = tk6_wrapd(tk6_sx[s] - tk6_px), dz = tk6_wrapd(tk6_sz[s] - tk6_pz);
        if (dx * dx + dz * dz < 22 * 22) { tk6_st[s] = 0; tk6_hitPlayer(tk6_sx[s] - tk6_svx[s] * 20, tk6_sz[s] - tk6_svz[s] * 20); }
      }
    }
    // a saucer wanders through now and then
    if (!tk6_ufo) { tk6_ufoT -= dt; if (tk6_ufoT <= 0) { tk6_ufo = true; tk6_ufoX = tk6_px + 500; tk6_ufoZ = tk6_pz + 500; } }
    else { tk6_ufoX += 1.2f; tk6_ufoZ += sinf(globalTime) * 1.5f; if (tk6_wrapd(tk6_ufoX - tk6_px) > 1200) { tk6_ufo = false; tk6_ufoT = 25; } }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  // sky: a sunset glow and the moon (they turn with you)
  if (P.sun) for (int y = TK6_HOR - 40; y < TK6_HOR; y += 2) { int lev = (y - (TK6_HOR - 40)) / 5; memset(v3_buf + y * W, A5C(lev, 10), W); }
  { float ma = tk6_angd(0.9f - tk6_pa) * tk6_f; int mx = 160 + (int)ma, my = 40;
    if (mx > -20 && mx < W + 20) { v3_disc(mx, my, 9, A5C(13, 8)); v3_disc(mx + 4, my - 2, 8, 0); } }
  // mountains: a ring of peaks round the horizon
  {
    int prevX = 0, prevY = 0;
    for (int k = 0; k <= 64; k++) {
      float a = k * 6.28318f / 64; float rel = tk6_angd(a - tk6_pa);
      uint32_t h = v3_hash(k % 64 * 131 + 7);
      int peak = 8 + (int)(h % 28) + ((k % 64) == 20 ? 18 : 0);
      int x = 160 + (int)(rel * tk6_f), y = TK6_HOR - peak;
      if (k > 0 && fabsf(tk6_angd(a - tk6_pa)) < 1.9f && v3_abs(x - prevX) < 200) a5_glow(prevX * 8, prevY * 8, x * 8, y * 8, 12, 3, false);
      prevX = x; prevY = y;
      if ((k % 64) == 20 && x > -30 && x < W + 30) {                   // the volcano, smoking
        for (int q = 0; q < 10; q++) { float t = fmodf(globalTime * 0.7f + q * 0.1f, 1.0f); int px = x + (int)((v3_hash(q * 77) % 21) - 10) * t * 2, py = y - (int)(t * 40) + (int)(t * t * 30); v3_putMax(px, py, 15 - (int)(t * 10), 7); }
      }
    }
  }
  a5_seg(0, TK6_HOR, W - 1, TK6_HOR, A5C(8, 1));
  // ground
  if (ground == 1) { for (int i = 0; i < 60; i++) { float wx = fmodf(v3_hash(i * 31) % 4000 + 0.0f, TK6_WS), wz = fmodf(v3_hash(i * 57 + 1) % 4000 + 0.0f, TK6_WS); float cx, cy, cz; tk6_cam(wx, 0, wz, &cx, &cy, &cz); if (cz > 10 && cz < 1200) v3_putMax((int)(160 + tk6_f * cx / cz), (int)(TK6_HOR - tk6_f * cy / cz), 12, 9); } }
  if (ground >= 2) {
    float gs = 100, ox = floorf(tk6_px / gs) * gs, oz = floorf(tk6_pz / gs) * gs, R = 900;
    if (ground == 3) { oz = floorf((tk6_pz + globalTime * 60) / gs) * gs - globalTime * 60; }
    for (int k = -9; k <= 9; k++) {
      float cx0, cy0, cz0, cx1, cy1, cz1;
      tk6_cam(ox + k * gs, 0, tk6_pz - R, &cx0, &cy0, &cz0); tk6_cam(ox + k * gs, 0, tk6_pz + R, &cx1, &cy1, &cz1); tk6_line3(cx0, cy0, cz0, cx1, cy1, cz1, 9, 9);
      tk6_cam(tk6_px - R, 0, oz + k * gs, &cx0, &cy0, &cz0); tk6_cam(tk6_px + R, 0, oz + k * gs, &cx1, &cy1, &cz1); tk6_line3(cx0, cy0, cz0, cx1, cy1, cz1, 9, 9);
    }
  }
  // obstacles, enemies, saucer, shells
  for (int i = 0; i < tk6_no; i++) {
    if (tk6_ot[i] == 0) tk6_model(tk6_vCube, 8, tk6_eCube, 12, tk6_ox[i], tk6_oz[i], tk6_oa[i], 1.0f, 15, 1);
    else if (tk6_ot[i] == 1) tk6_model(tk6_vPyr, 5, tk6_ePyr, 8, tk6_ox[i], tk6_oz[i], tk6_oa[i], 1.0f, 15, 1);
    else tk6_model(tk6_vTall, 8, tk6_eCube, 12, tk6_ox[i], tk6_oz[i], tk6_oa[i], 1.0f, 15, 1);
  }
  for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) tk6_model(tk6_vTank, 18, tk6_eTank, 25, tk6_ex[i], tk6_ez[i], tk6_ea[i], 1.0f, 15, 2);
  if (tk6_ufo) tk6_model(tk6_vUfo, 9, tk6_eUfo, 18, tk6_ufoX, tk6_ufoZ, globalTime, 1.0f, 15, 0);
  for (int s = 0; s <= TK6_NE; s++) if (tk6_st[s] > 0) {
    float cx, cy, cz; tk6_cam(tk6_sx[s], 10, tk6_sz[s], &cx, &cy, &cz);
    if (cz > 8) { int x = (int)(160 + tk6_f * cx / cz), y = (int)(TK6_HOR - tk6_f * cy / cz), r = v3_clamp((int)(300 / cz), 1, 5); v3_disc(x, y, r, A5C(15, s ? 2 : 6)); }
  }
  a5_partDraw();
  // the periscope: reticle, radar, messages
  uint8_t hc = A5C(15, 4), hd = A5C(9, 4);
  a5_seg(160, TK6_HOR - 30, 160, TK6_HOR - 14, hc); a5_seg(160, TK6_HOR + 14, 160, TK6_HOR + 30, hc);
  a5_seg(140, TK6_HOR - 30, 180, TK6_HOR - 30, hc); a5_seg(140, TK6_HOR + 30, 180, TK6_HOR + 30, hc);
  a5_seg(140, TK6_HOR - 30, 140, TK6_HOR - 24, hc); a5_seg(180, TK6_HOR - 30, 180, TK6_HOR - 24, hc); a5_seg(140, TK6_HOR + 30, 140, TK6_HOR + 24, hc); a5_seg(180, TK6_HOR + 30, 180, TK6_HOR + 24, hc);
  { int rx = 160, ry = 26, rr = 16; a5_circle(rx, ry, rr, hd); int sa = (int)(globalTime * 400) & 1023; a5_seg(rx, ry, rx + (v3_sin(sa) * rr >> 14), ry - (v3_cos(sa) * rr >> 14), A5C(13, 5)); v3_put(rx, ry, hc);
    for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) { float dx = tk6_wrapd(tk6_ex[i] - tk6_px), dz = tk6_wrapd(tk6_ez[i] - tk6_pz), d = sqrtf(dx * dx + dz * dz); if (d < 1500) { float a = atan2f(dx, dz) - tk6_pa, r = d / 1500 * rr; v3_rect(rx + (int)(sinf(a) * r), ry - (int)(cosf(a) * r), rx + (int)(sinf(a) * r) + 1, ry - (int)(cosf(a) * r) + 1, A5C(15, 2)); } } }
  int inRange = -1; const char* dirMsg = 0;
  for (int i = 0; i < TK6_NE; i++) if (tk6_eon[i]) {
    float dx = tk6_wrapd(tk6_ex[i] - tk6_px), dz = tk6_wrapd(tk6_ez[i] - tk6_pz), rel = tk6_angd(atan2f(dx, dz) - tk6_pa);
    if (fabsf(rel) < 0.12f && dx * dx + dz * dz < 900 * 900) inRange = i;
    else if (!dirMsg) dirMsg = fabsf(rel) > 2.3f ? "ENEMY TO REAR" : (rel < 0 ? "ENEMY TO LEFT" : "ENEMY TO RIGHT");
  }
  if (tk6_dead <= 0) {
    if (inRange >= 0) a5_textS(110, 46, "ENEMY IN RANGE", 1, A5C(15, 2));
    else if (dirMsg && ((int)(globalTime * 3) & 1)) a5_textS(160 - a5_textW(dirMsg, 1) / 2, 46, dirMsg, 1, A5C(15, 2));
  }
  // cracked glass
  if (tk6_dead > 0) {
    uint32_t h = 991;
    for (int k = 0; k < 14; k++) {
      int a = (int)(v3_hash(h + k) & 1023), len = 30 + (int)(v3_hash(h + k * 3) % 160);
      int x0 = (int)tk6_crackX, y0 = (int)tk6_crackY;
      for (int seg = 0; seg < 4; seg++) { int x1 = x0 + (v3_cos(a) * (len / 4) >> 14), y1 = y0 + (v3_sin(a) * (len / 4) >> 14); a5_seg(x0, y0, x1, y1, A5C(15, 0)); x0 = x1; y0 = y1; a += (int)(v3_hash(h + k * 7 + seg) % 120) - 60; }
    }
  }
  a5_post(160, TK6_HOR);
  // score and lives
  char s[24];
  snprintf(s, 24, "SCORE %u", (unsigned)tk6_score); a5_textS(W - 4 - a5_textW(s, 1), 4, s, 1, hc);
  snprintf(s, 24, "HIGH SCORE %u", (unsigned)tk6_hi); a5_textS(W - 4 - a5_textW(s, 1), 14, s, 1, hd);
  for (int i = 0; i < v3_clamp(tk6_lives, 0, 5); i++) { int x = W - 14 - i * 14, y = 30; a5_seg(x, y + 4, x + 10, y + 4, hc); a5_seg(x + 1, y + 4, x + 3, y + 1, hc); a5_seg(x + 3, y + 1, x + 8, y + 1, hc); a5_seg(x + 8, y + 1, x + 10, y + 4, hc); a5_seg(x + 5, y + 1, x + 5, y - 1, hc); a5_seg(x + 5, y - 1, x + 11, y - 1, hc); }
  if (a5_player) a5_textS(4, 4, "1UP", 1, hc); else if (((int)(globalTime * 2.5f)) & 1) a5_textS(4, 4, "DEMO", 1, hc);
  if (a5_join > 0) a5_textC(H / 2 + 40, "PLAYER 1", 2, hc);
  if (tk6_over) a5_banner(H / 2 + 40, "GAME OVER", 2, A5C(15, 2));
  a5_crt();
}
void prog_tanks_draw(int preset) { tanks5_draw(preset); fx5_apply(); }

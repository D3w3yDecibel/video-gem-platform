// =====================================================================
// PROGRAM: DEFENCE — protect six cities from falling missiles (by Dewey)
// Set 5 "Arcade", slot 4
//
// In the spirit of the 1980 missile-defence cabinet: missiles streak down
// from the top of the screen trailing lines, aimed at your six cities and
// three launch sites. Put the crosshair in their path and fire: your shot
// races up to the cross and blooms into a flashing ball of fire that eats
// anything flying through it — and every missile it catches blows up too,
// so one good shot can set off a chain. Later waves split in mid-air,
// send bombers across and drop smart bombs that dodge your fireballs.
// Every wave changes the colours. When the last city falls: THE END.
//
// It plays itself; touch p0/p1 or k12 and you aim and fire.
//
// Presets:
//   k0 Classic   k1 Night Raid   k2 Dusk   k3 Arctic   k4 Neon
//   k5 Blitz (lots of missiles)   k6 Big Bang (huge fireballs)
//   k7 Split Storm (missiles split)   k8 Green Screen   k9 Amber Screen
//   k10 Sunset   k11 Tour — a new preset every wave
//   k12 — FIRE (from the nearest site with missiles left). Joins the game.
//
// Knobs:
//   p0 Crosshair ◄ left | right ►   p1 Crosshair ◄ low | high ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Missiles per Wave (centre = preset)   p8 Splitting (centre = preset)
//   p9 Fireball Size   p10 Colour Changes (centre = every wave; left = never; right = all the time)
//   p11 Sky (centre = preset: black, stars, dusk glow, aurora)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 ground, 2 cities, 3 sites, 4 their trails, 5 their heads, 6 your trails, 7 crosshair,
//       8 fire A, 9 HUD, 10 sky top, 11 sky glow, 12 bombers, 13 fire B, 14 fire C, 15 stars
struct Df6Pre { uint8_t look, missiles, split, blast, sky, trails, scheme; };
static const Df6Pre df6_pre[11] = {
  { A5_ARCADE, 10, 15, 100, 0, 0, 0 }, { A5_ARCADE, 12, 20, 100, 1, 60, 1 }, { A5_ARCADE, 10, 15, 100, 2, 0, 2 },
  { A5_ARCADE, 10, 15, 100, 1, 0, 3 }, { A5_NEON, 12, 20, 100, 3, 120, 4 }, { A5_ARCADE, 22, 15, 90, 0, 0, 0 },
  { A5_ARCADE, 10, 15, 190, 1, 60, 1 }, { A5_ARCADE, 10, 60, 100, 2, 0, 2 }, { A5_PHOSPHOR, 12, 20, 100, 0, 150, 0 },
  { A5_AMBER, 12, 20, 100, 0, 150, 0 }, { A5_ARCADE, 12, 20, 110, 2, 40, 5 },
};
static const char* const df6_names[12] = { "Classic", "Night Raid", "Dusk", "Arctic", "Neon", "Blitz", "Big Bang", "Split Storm", "Green Screen", "Amber Screen", "Sunset", "Tour" };
static const char* const df6_lbl[16] = { "Crosshair: Left|Right", "Crosshair: Low|High", "", "", "", "", "", "Missiles per Wave", "Splitting", "Fireball Size", "Colour Changes", "Sky", "", "", "", "" };
const char* prog_defence_name() { return "DEFENCE"; }
const char* prog_defence_character() { return "Missiles rain on six cities; fireballs chain in the sky - plays itself until you touch p0/p1/k12"; }
const char* prog_defence_presetName(int preset) { return (preset >= 0 && preset < 12) ? df6_names[preset] : NULL; }
const char* prog_defence_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, df6_lbl); }
uint8_t prog_defence_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// colour schemes for each wave: ground, cities, their trails, your trails, sky top
static const uint8_t df6_schemes[6][8][5][3] = {
  { // classic: changes every wave
    {{230,200,0},{40,120,255},{255,40,40},{60,90,255},{0,0,0}}, {{230,200,0},{60,220,255},{40,255,40},{255,40,40},{0,0,0}},
    {{40,60,255},{255,220,0},{255,40,255},{40,255,40},{0,0,0}}, {{255,40,40},{255,255,255},{255,220,0},{60,220,255},{0,0,0}},
    {{40,200,60},{255,80,80},{255,255,255},{255,120,0},{0,0,0}}, {{200,0,200},{60,255,60},{0,200,255},{255,255,0},{0,0,0}},
    {{0,170,170},{255,160,40},{255,60,120},{200,255,60},{0,0,0}}, {{255,140,0},{120,120,255},{255,255,0},{255,0,160},{0,0,0}} },
  { // night raid: deep blue
    {{30,60,120},{255,220,120},{255,60,60},{120,200,255},{0,0,30}}, {{40,40,110},{255,240,160},{255,120,40},{160,255,200},{0,0,40}},
    {{20,70,100},{255,200,100},{255,40,120},{120,255,255},{0,10,40}}, {{40,30,90},{255,255,180},{255,80,40},{180,180,255},{5,0,35}},
    {{30,60,120},{255,220,120},{255,60,60},{120,200,255},{0,0,30}}, {{40,40,110},{255,240,160},{255,120,40},{160,255,200},{0,0,40}},
    {{20,70,100},{255,200,100},{255,40,120},{120,255,255},{0,10,40}}, {{40,30,90},{255,255,180},{255,80,40},{180,180,255},{5,0,35}} },
  { // dusk
    {{90,40,30},{255,200,120},{255,250,200},{120,220,255},{40,10,60}}, {{70,30,50},{255,180,100},{255,255,255},{100,255,200},{50,10,50}},
    {{80,50,20},{255,220,160},{255,240,180},{160,200,255},{30,10,70}}, {{60,30,30},{255,190,140},{255,255,220},{120,255,255},{60,20,60}},
    {{90,40,30},{255,200,120},{255,250,200},{120,220,255},{40,10,60}}, {{70,30,50},{255,180,100},{255,255,255},{100,255,200},{50,10,50}},
    {{80,50,20},{255,220,160},{255,240,180},{160,200,255},{30,10,70}}, {{60,30,30},{255,190,140},{255,255,220},{120,255,255},{60,20,60}} },
  { // arctic
    {{220,235,255},{80,160,255},{255,60,60},{40,90,200},{10,20,50}}, {{200,225,255},{60,200,220},{255,120,40},{60,80,220},{10,20,60}},
    {{230,240,255},{100,140,255},{220,40,120},{30,120,200},{10,25,55}}, {{210,230,250},{80,180,240},{255,80,0},{40,60,180},{15,20,50}},
    {{220,235,255},{80,160,255},{255,60,60},{40,90,200},{10,20,50}}, {{200,225,255},{60,200,220},{255,120,40},{60,80,220},{10,20,60}},
    {{230,240,255},{100,140,255},{220,40,120},{30,120,200},{10,25,55}}, {{210,230,250},{80,180,240},{255,80,0},{40,60,180},{15,20,50}} },
  { // neon
    {{255,0,200},{0,255,255},{255,255,0},{0,255,120},{10,0,30}}, {{0,255,200},{255,0,255},{255,120,0},{0,160,255},{20,0,30}},
    {{255,120,0},{0,255,120},{255,0,120},{255,255,0},{0,10,30}}, {{120,0,255},{255,255,0},{0,255,255},{255,0,160},{20,0,20}},
    {{255,0,200},{0,255,255},{255,255,0},{0,255,120},{10,0,30}}, {{0,255,200},{255,0,255},{255,120,0},{0,160,255},{20,0,30}},
    {{255,120,0},{0,255,120},{255,0,120},{255,255,0},{0,10,30}}, {{120,0,255},{255,255,0},{0,255,255},{255,0,160},{20,0,20}} },
  { // sunset
    {{60,20,40},{255,200,90},{255,250,220},{255,120,160},{60,20,80}}, {{70,25,40},{255,170,80},{255,255,255},{255,150,100},{80,30,90}},
    {{50,20,50},{255,210,120},{255,240,200},{255,110,180},{70,20,100}}, {{60,30,30},{255,190,110},{255,250,210},{255,140,120},{90,30,80}},
    {{60,20,40},{255,200,90},{255,250,220},{255,120,160},{60,20,80}}, {{70,25,40},{255,170,80},{255,255,255},{255,150,100},{80,30,90}},
    {{50,20,50},{255,210,120},{255,240,200},{255,110,180},{70,20,100}}, {{60,30,30},{255,190,110},{255,250,210},{255,140,120},{90,30,80}} },
};

#define DF6_GROUND 222
#define DF6_NM 30
#define DF6_NC 10
#define DF6_NX 24
static const int df6_siteX[3] = { 24, 160, 296 };
static const int df6_cityX[6] = { 62, 90, 118, 202, 230, 258 };
static float df6_mx[DF6_NM], df6_my[DF6_NM], df6_m0x[DF6_NM], df6_m0y[DF6_NM], df6_mvx[DF6_NM], df6_mvy[DF6_NM];
static int8_t df6_mt[DF6_NM], df6_mk[DF6_NM];                  // target (0-5 city, 6-8 site), kind 0 none 1 missile 2 smart bomb
static bool df6_mSplit[DF6_NM];
static float df6_cx[DF6_NC], df6_cy[DF6_NC], df6_c0x[DF6_NC], df6_c0y[DF6_NC], df6_ctx[DF6_NC], df6_cty[DF6_NC];
static bool df6_con[DF6_NC];
static float df6_xx[DF6_NX], df6_xy[DF6_NX], df6_xt[DF6_NX];   // fireballs: age (s), 0 = none
static bool df6_city[6]; static int df6_ammo[3];
static float df6_hx = 160, df6_hy = 120, df6_spawnT = 0, df6_banner = 0, df6_bonusT = 0, df6_over = 0, df6_bx = -40, df6_by = 80, df6_bvx = 0;
static int df6_wave = 1, df6_toSpawn = 0, df6_preset = -1, df6_look = 0, df6_cityBank = 0, df6_bomber = 0;
static uint32_t df6_score = 0, df6_hi = 7500, df6_nextCity = 10000;
static float df6_diff = 0.4f, df6_blastK = 1, df6_aiT = 0, df6_aiX = 160, df6_aiY = 120;
static bool df6_aiFire = false;

static inline float df6_blastR(float t) {                      // a fireball's radius at age t
  float R = 17 * df6_blastK;
  if (t < 0.45f) return R * t / 0.45f;
  if (t < 0.9f) return R;
  if (t < 1.4f) return R * (1.4f - t) / 0.5f;
  return 0;
}
static void df6_boom(float x, float y) {
  for (int i = 0; i < DF6_NX; i++) if (df6_xt[i] <= 0) { df6_xx[i] = x; df6_xy[i] = y; df6_xt[i] = 0.001f; return; }
}
static int df6_pickTarget() {
  int live[9], n = 0;
  for (int c = 0; c < 6; c++) if (df6_city[c]) live[n++] = c;
  for (int s = 0; s < 3; s++) if (df6_ammo[s] > 0 || v3_rnd(3) == 0) live[n++] = 6 + s;
  if (!n) return v3_rnd(9);
  return live[v3_rnd(n)];
}
static inline float df6_targetX(int t) { return t < 6 ? (float)df6_cityX[t] : (float)df6_siteX[t - 6]; }
static void df6_launch(float x, float y, int target, int kind) {
  for (int i = 0; i < DF6_NM; i++) if (!df6_mk[i]) {
    float tx = df6_targetX(target) + (v3_rndf() - 0.5f) * 6, ty = DF6_GROUND - 4;
    float sp = (0.22f + df6_diff * 0.25f + df6_wave * 0.04f) * (kind == 2 ? 0.8f : 1.0f);
    float dx = tx - x, dy = ty - y, dl = sqrtf(dx * dx + dy * dy) + 0.01f;
    df6_mx[i] = df6_m0x[i] = x; df6_my[i] = df6_m0y[i] = y; df6_mvx[i] = dx / dl * sp; df6_mvy[i] = dy / dl * sp;
    df6_mt[i] = (int8_t)target; df6_mk[i] = (int8_t)kind; df6_mSplit[i] = false;
    return;
  }
}
static void df6_startWave() {
  memset(df6_mk, 0, sizeof(df6_mk)); memset(df6_con, 0, sizeof(df6_con)); memset(df6_xt, 0, sizeof(df6_xt));
  for (int s = 0; s < 3; s++) df6_ammo[s] = 10;
  while (df6_cityBank > 0) { bool placed = false; for (int c = 0; c < 6 && !placed; c++) if (!df6_city[c]) { df6_city[c] = true; placed = true; } df6_cityBank--; if (!placed) break; }
  df6_toSpawn = v3_centre(7) ? df6_pre[df6_look].missiles + df6_wave * 2 : 4 + (int)(v3_u(7) * 40);
  df6_spawnT = 2.0f; df6_banner = 2.0f; df6_bomber = 0; df6_bx = -40;
}
static void df6_newGame() {
  df6_score = 0; df6_wave = 1; df6_over = 0; df6_cityBank = 0; df6_nextCity = 10000;
  for (int c = 0; c < 6; c++) df6_city[c] = true;
  df6_startWave();
}
static void df6_fire(float tx, float ty) {
  ty = v3_clampf(ty, 14, DF6_GROUND - 18);
  int best = -1; float bd = 1e9f;
  for (int s = 0; s < 3; s++) if (df6_ammo[s] > 0) { float d = fabsf(df6_siteX[s] - tx) + (s == 1 ? -20 : 0); if (d < bd) { bd = d; best = s; } }
  if (best < 0) return;
  for (int i = 0; i < DF6_NC; i++) if (!df6_con[i]) {
    df6_ammo[best]--; df6_con[i] = true;
    df6_cx[i] = df6_c0x[i] = (float)df6_siteX[best]; df6_cy[i] = df6_c0y[i] = DF6_GROUND - 10; df6_ctx[i] = tx; df6_cty[i] = ty;
    return;
  }
}
// the computer gunner: find the missile that lands soonest and isn't already covered, lead it, fire
static void df6_ai() {
  float best = 1e9f; int bi = -1; float bx = 0, by = 0;
  for (int i = 0; i < DF6_NM; i++) if (df6_mk[i]) {
    if (df6_mt[i] < 6 && !df6_city[df6_mt[i]]) continue;          // heading for rubble: let it go
    float tLand = (DF6_GROUND - df6_my[i]) / (df6_mvy[i] + 0.001f);
    // already inside a fireball's reach, or a shot is on its way there?
    bool covered = false;
    for (int c = 0; c < DF6_NC && !covered; c++) if (df6_con[c]) {
      float steps = sqrtf((df6_ctx[c] - df6_cx[c]) * (df6_ctx[c] - df6_cx[c]) + (df6_cty[c] - df6_cy[c]) * (df6_cty[c] - df6_cy[c])) / 4.0f + 20;
      float px = df6_mx[i] + df6_mvx[i] * steps, py = df6_my[i] + df6_mvy[i] * steps;
      if (fabsf(px - df6_ctx[c]) < 14 * df6_blastK && fabsf(py - df6_cty[c]) < 14 * df6_blastK) covered = true;
    }
    for (int x = 0; x < DF6_NX && !covered; x++) if (df6_xt[x] > 0 && df6_xt[x] < 1.0f) {
      float px = df6_mx[i] + df6_mvx[i] * 20, py = df6_my[i] + df6_mvy[i] * 20;
      float r = 17 * df6_blastK; if ((px - df6_xx[x]) * (px - df6_xx[x]) + (py - df6_xy[x]) * (py - df6_xy[x]) < r * r) covered = true;
    }
    if (covered) continue;
    if (tLand < best) {
      best = tLand; bi = i;
      // where will it be when a shot gets there and the ball has grown?
      float sx = df6_siteX[1], tt = 30;
      for (int it = 0; it < 3; it++) { float px = df6_mx[i] + df6_mvx[i] * tt, py = df6_my[i] + df6_mvy[i] * tt; tt = sqrtf((px - sx) * (px - sx) + (py - DF6_GROUND) * (py - DF6_GROUND)) / 4.0f + 14; }
      bx = df6_mx[i] + df6_mvx[i] * tt; by = df6_my[i] + df6_mvy[i] * tt;
    }
  }
  if (bi < 0) { df6_aiFire = false; return; }
  df6_aiX = bx + (v3_rndf() - 0.5f) * 8 * (1.0f - df6_diff); df6_aiY = by; df6_aiFire = true;
}

void prog_defence_init() { v3_reset(); a5_reset(); df6_preset = -1; a5_partClear(); }

static void defence5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != df6_preset) { df6_preset = preset; df6_look = preset == 11 ? 0 : preset; df6_newGame(); }
  const Df6Pre& P = df6_pre[df6_look];
  df6_diff = a5_diffK(0.35f);
  int splitPct = v3_centre(8) ? P.split : (int)(v3_u(8) * 90);
  df6_blastK = v3_centre(9) ? P.blast / 100.0f : 0.5f + v3_u(9) * 2.0f;
  int sky = v3_pick(11, P.sky, 4);
  // colours: this wave's scheme
  int cw = df6_wave - 1;
  if (!v3_centre(10)) cw = v3_k(10) < 440 ? 0 : (int)(globalTime * 2.0f);
  const uint8_t (*S)[3] = df6_schemes[P.scheme][cw & 7];
  int fl = (int)(globalTime * 20);
  uint8_t fire[3][3]; for (int k = 0; k < 3; k++) v3_hsv((fl * 256 + k * 512) % 1536, 255, 255, fire[k]);
  uint8_t hues[16][3] = { {255,255,255},{0},{0},{0},{0},{255,255,255},{0},{255,255,255},{0},{255,230,60},{0},{0},{200,200,200},{0},{0},{200,210,255} };
  memcpy(hues[1], S[0], 3); memcpy(hues[2], S[1], 3); memcpy(hues[3], S[0], 3); memcpy(hues[4], S[2], 3); memcpy(hues[6], S[3], 3);
  memcpy(hues[8], fire[0], 3); memcpy(hues[13], fire[1], 3); memcpy(hues[14], fire[2], 3); memcpy(hues[10], S[4], 3);
  uint8_t glow[3] = { (uint8_t)(S[2][0] / 3 + 40), (uint8_t)(S[2][1] / 4 + 10), (uint8_t)(S[2][2] / 3 + 40) }; memcpy(hues[11], glow, 3);
  hues[12][0] = S[1][0]; hues[12][1] = S[1][1]; hues[12][2] = S[1][2];
  a5_palette(hues, 16, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (df6_banner > 0) df6_banner -= dt;
    if (df6_over > 0) {
      df6_over -= dt;
      if (df6_over > 3.5f && v3_rnd(4) == 0) df6_boom(v3_rndf() * W, 20 + v3_rndf() * 190);
      if (df6_over <= 0) { if (df6_preset == 11) df6_look = (df6_look + 1) % 11; df6_newGame(); }
    }
    // crosshair
    if (a5_player) {
      float tx = 6 + v3_u(0) * (W - 12), ty = 14 + (1.0f - v3_u(1)) * (DF6_GROUND - 32);
      df6_hx += (tx - df6_hx) * 0.4f; df6_hy += (ty - df6_hy) * 0.4f;
      if (a5_fire && df6_over <= 0) df6_fire(df6_hx, df6_hy);
    } else if (df6_over <= 0) {
      df6_aiT -= dt; if (df6_aiT <= 0) { df6_ai(); df6_aiT = 0.08f; }
      float dx = df6_aiX - df6_hx, dy = df6_aiY - df6_hy, dl = sqrtf(dx * dx + dy * dy), mv = 5.0f + df6_diff * 3;
      if (dl > mv) { df6_hx += dx / dl * mv; df6_hy += dy / dl * mv; } else { df6_hx = df6_aiX; df6_hy = df6_aiY; if (df6_aiFire) { df6_fire(df6_hx, df6_hy); df6_aiFire = false; df6_aiT = 0.12f + (1.0f - df6_diff) * 0.15f; } }
    }
    df6_hx = v3_clampf(df6_hx, 4, W - 4); df6_hy = v3_clampf(df6_hy, 12, DF6_GROUND - 14);
    // your shots
    for (int i = 0; i < DF6_NC; i++) if (df6_con[i]) {
      float dx = df6_ctx[i] - df6_cx[i], dy = df6_cty[i] - df6_cy[i], dl = sqrtf(dx * dx + dy * dy), sp = 4.0f;
      if (dl <= sp) { df6_con[i] = false; df6_boom(df6_ctx[i], df6_cty[i]); }
      else { df6_cx[i] += dx / dl * sp; df6_cy[i] += dy / dl * sp; }
    }
    // fireballs grow, then shrink
    for (int x = 0; x < DF6_NX; x++) if (df6_xt[x] > 0) { df6_xt[x] += dt; if (df6_xt[x] > 1.4f) df6_xt[x] = 0; }
    if (df6_over > 0) continue;
    // incoming
    df6_spawnT -= dt;
    if (df6_spawnT <= 0 && df6_toSpawn > 0) {
      int burst = 1 + v3_rnd(2 + df6_wave / 3);
      for (int b = 0; b < burst && df6_toSpawn > 0; b++, df6_toSpawn--) df6_launch(10 + v3_rndf() * (W - 20), 12, df6_pickTarget(), (df6_wave >= 4 && v3_rnd(7) == 0) ? 2 : 1);
      df6_spawnT = 1.6f + v3_rndf() * 2.0f - df6_diff;
    }
    if (df6_wave >= 2 && !df6_bomber && v3_rnd(1500) == 0) { df6_bomber = 1; df6_bx = v3_rnd(2) ? -20 : W + 20; df6_by = 60 + v3_rndf() * 60; df6_bvx = df6_bx < 0 ? 0.6f : -0.6f; }
    if (df6_bomber) {
      df6_bx += df6_bvx;
      if (v3_rnd(120) == 0 && df6_bx > 20 && df6_bx < W - 20) df6_launch(df6_bx, df6_by + 4, df6_pickTarget(), 1);
      if (df6_bx < -30 || df6_bx > W + 30) df6_bomber = 0;
      for (int x = 0; x < DF6_NX; x++) if (df6_xt[x] > 0) { float r = df6_blastR(df6_xt[x]); if ((df6_bx - df6_xx[x]) * (df6_bx - df6_xx[x]) + (df6_by - df6_xy[x]) * (df6_by - df6_xy[x]) < r * r + 40) { df6_bomber = 0; df6_score += 100; df6_boom(df6_bx, df6_by); break; } }
    }
    for (int i = 0; i < DF6_NM; i++) if (df6_mk[i]) {
      if (df6_mk[i] == 2) {                                            // smart bombs sidestep fireballs
        for (int x = 0; x < DF6_NX; x++) if (df6_xt[x] > 0) { float dx = df6_mx[i] - df6_xx[x], dy = df6_my[i] - df6_xy[x]; float r = df6_blastR(df6_xt[x]) + 12; if (dx * dx + dy * dy < r * r) df6_mx[i] += dx > 0 ? 0.8f : -0.8f; }
      }
      df6_mx[i] += df6_mvx[i]; df6_my[i] += df6_mvy[i];
      // split in mid-air
      if (!df6_mSplit[i] && df6_mk[i] == 1 && df6_my[i] > 60 && df6_my[i] < 140 && v3_rnd(1000) < splitPct / 4) {
        df6_mSplit[i] = true; int n = 1 + v3_rnd(2);
        for (int k = 0; k < n; k++) df6_launch(df6_mx[i], df6_my[i], df6_pickTarget(), 1);
        for (int k = 0; k < DF6_NM; k++) if (df6_mk[k] && df6_m0y[k] == df6_my[i] && df6_m0x[k] == df6_mx[i]) { df6_m0x[k] = df6_m0x[i]; df6_m0y[k] = df6_m0y[i]; df6_mSplit[k] = true; }
      }
      // caught in a fireball?
      bool hit = false;
      for (int x = 0; x < DF6_NX && !hit; x++) if (df6_xt[x] > 0) { float r = df6_blastR(df6_xt[x]); float dx = df6_mx[i] - df6_xx[x], dy = df6_my[i] - df6_xy[x]; if (dx * dx + dy * dy < r * r) hit = true; }
      if (hit) { df6_score += df6_mk[i] == 2 ? 125 : 25; df6_mk[i] = 0; df6_boom(df6_mx[i], df6_my[i]); continue; }
      if (df6_my[i] >= DF6_GROUND - 5) {                               // impact
        int t = df6_mt[i]; df6_mk[i] = 0; df6_boom(df6_mx[i], df6_my[i]); a5_shake(3);
        if (t < 6 && df6_city[t]) { df6_city[t] = false; a5_burst(df6_cityX[t], DF6_GROUND - 4, 30, 2, 40, 40); }
        if (t >= 6) df6_ammo[t - 6] = 0;
      }
    }
    // wave over?
    bool busy = df6_toSpawn > 0 || df6_bomber;
    for (int i = 0; i < DF6_NM && !busy; i++) if (df6_mk[i]) busy = true;
    for (int x = 0; x < DF6_NX && !busy; x++) if (df6_xt[x] > 0) busy = true;
    for (int c = 0; c < DF6_NC && !busy; c++) if (df6_con[c]) busy = true;
    int cities = 0; for (int c = 0; c < 6; c++) cities += df6_city[c];
    if (cities == 0 && df6_cityBank == 0 && !busy) { df6_over = 6.0f; if (df6_score > df6_hi) df6_hi = df6_score; a5_shake(8); continue; }
    if (!busy) {
      if (df6_bonusT <= 0) df6_bonusT = 2.6f;
      df6_bonusT -= dt;
      if (df6_bonusT < 2.0f) {                                         // count the bonus
        bool counting = false;
        for (int s = 0; s < 3 && !counting; s++) if (df6_ammo[s] > 0 && v3_rnd(2) == 0) { df6_ammo[s]--; df6_score += 5 * (1 + df6_wave / 2); counting = true; }
        if (!counting && df6_bonusT < 0.2f) {
          df6_score += cities * 100 * (1 + df6_wave / 2);
          if (df6_score >= df6_nextCity) { df6_cityBank++; df6_nextCity += 10000; }
          df6_wave++; df6_bonusT = 0; if (df6_preset == 11) df6_look = (df6_look + 1) % 11;
          df6_startWave();
        }
      }
    }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (sky == 1) a5_stars(70, 0, 0, 15, 11);
  if (sky == 2) for (int y = 120; y < DF6_GROUND; y += 2) { int lev = (y - 120) * 7 / 100; if (lev > 0) { uint8_t c = A5C(lev, 11); memset(v3_buf + y * W, c, W); } }
  if (sky == 3) for (int x = 0; x < W; x += 2) { int top = 40 + (v3_sin(x * 6 + (int)(globalTime * 60)) >> 10) + (v3_sin(x * 17 - (int)(globalTime * 90)) >> 11); for (int y = top; y < top + 40; y += 2) v3_putMax(x, y, 6 - (y - top) / 7, 2); }
  // ground: hills, sites and cities
  for (int x = 0; x < W; x++) {
    int h = 0;
    for (int s = 0; s < 3; s++) { int d = v3_abs(x - df6_siteX[s]); if (d < 22) h = a5_max(h, 10 - d / 3); }
    int top = DF6_GROUND - h;
    for (int y = top; y < H; y++) v3_buf[y * W + x] = A5C(y == top ? 15 : 11, 1);
  }
  for (int s = 0; s < 3; s++) for (int k = 0; k < df6_ammo[s]; k++) {          // missiles left, stacked as a little pyramid
    static const int8_t ox[10] = { 0, -3, 3, -6, 0, 6, -9, -3, 3, 9 }, oy[10] = { 0, 3, 3, 6, 6, 6, 9, 9, 9, 9 };
    int x = df6_siteX[s] + ox[k], y = DF6_GROUND - 9 + oy[k];
    v3_rect(x, y - 1, x, y + 1, A5C(15, 6)); v3_put(x - 1, y + 1, A5C(12, 6)); v3_put(x + 1, y + 1, A5C(12, 6));
  }
  for (int c = 0; c < 6; c++) {
    int x = df6_cityX[c], y = DF6_GROUND;
    if (df6_city[c]) {
      static const int8_t bh[7] = { 5, 9, 7, 12, 6, 10, 4 };
      for (int b = 0; b < 7; b++) { v3_rect(x - 10 + b * 3, y - bh[b], x - 8 + b * 3, y - 1, A5C(b & 1 ? 13 : 10, 2)); if (bh[b] > 6) v3_put(x - 9 + b * 3, y - bh[b] + 2, A5C(15, 9)); }
    } else { for (int b = 0; b < 6; b++) v3_rect(x - 9 + b * 3, y - 2 - (b * 7 % 3), x - 8 + b * 3, y - 1, A5C(6, 2)); }
  }
  // incoming trails and heads
  for (int i = 0; i < DF6_NM; i++) if (df6_mk[i]) {
    if (df6_mk[i] == 2) { int x = (int)df6_mx[i], y = (int)df6_my[i]; uint8_t c = A5C(15, (fl & 2) ? 5 : 4); a5_seg(x, y - 3, x + 3, y, c); a5_seg(x + 3, y, x, y + 3, c); a5_seg(x, y + 3, x - 3, y, c); a5_seg(x - 3, y, x, y - 3, c); continue; }
    a5_seg((int)df6_m0x[i], (int)df6_m0y[i], (int)df6_mx[i], (int)df6_my[i], A5C(12, 4));
    v3_rect((int)df6_mx[i], (int)df6_my[i], (int)df6_mx[i] + 1, (int)df6_my[i] + 1, A5C(15, (fl & 1) ? 5 : 4));
  }
  if (df6_bomber) { int x = (int)df6_bx, y = (int)df6_by; v3_rect(x - 7, y - 1, x + 7, y + 1, A5C(14, 12)); v3_rect(x - 2, y - 4, x + 2, y + 4, A5C(12, 12)); v3_put(x + (df6_bvx > 0 ? 7 : -7), y, A5C(15, 0)); }
  // your shots
  for (int i = 0; i < DF6_NC; i++) if (df6_con[i]) {
    a5_seg((int)df6_c0x[i], (int)df6_c0y[i], (int)df6_cx[i], (int)df6_cy[i], A5C(13, 6));
    int x = (int)df6_ctx[i], y = (int)df6_cty[i];
    a5_seg(x - 2, y - 2, x + 2, y + 2, A5C(15, 7)); a5_seg(x - 2, y + 2, x + 2, y - 2, A5C(15, 7));
  }
  // fireballs, flashing
  for (int x = 0; x < DF6_NX; x++) if (df6_xt[x] > 0) {
    int r = (int)df6_blastR(df6_xt[x]); if (r < 1) continue;
    int hue = ((fl + x) % 4 == 0) ? 0 : ((fl + x) % 4 == 1 ? 8 : ((fl + x) % 4 == 2 ? 13 : 14));
    v3_disc((int)df6_xx[x], (int)df6_xy[x], r, A5C(15, hue));
    if (r > 4) v3_disc((int)df6_xx[x], (int)df6_xy[x], r / 2, A5C(15, ((fl + x + 1) & 3) == 0 ? 0 : 13));
  }
  a5_partDraw();
  // crosshair
  if (df6_over <= 0) { int x = (int)df6_hx, y = (int)df6_hy; uint8_t c = A5C(15, 7); a5_seg(x - 5, y, x - 2, y, c); a5_seg(x + 2, y, x + 5, y, c); a5_seg(x, y - 5, x, y - 2, c); a5_seg(x, y + 2, x, y + 5, c); }
  if (df6_over > 0 && df6_over < 4.5f) {                             // THE END: a growing burst
    float t = 4.5f - df6_over; int r = (int)(t * 60);
    for (int k = 0; k < 8; k++) { int a = k * 128, b = a + 128; a5_seg(160 + (v3_cos(a) * r >> 14), 120 + (v3_sin(a) * r >> 14), 160 + (v3_cos(b) * r >> 14), 120 + (v3_sin(b) * r >> 14), A5C(15, (fl + k) & 1 ? 8 : 13)); }
  }

  a5_post((int)df6_hx, (int)df6_hy);
  a5_hud(df6_score, df6_hi, A5C(15, 9), A5C(15, 4));
  char s[20]; snprintf(s, 20, "WAVE %d", df6_wave); a5_textS(W - 54, 2, s, 1, A5C(15, 9));
  if (df6_banner > 0) { a5_banner(H / 2 - 40, s, 2, A5C(15, 9)); a5_banner(H / 2 - 8, "DEFEND CITIES", 1, A5C(15, 4)); }
  if (df6_bonusT > 0 && df6_bonusT < 2.4f) a5_banner(H / 2 - 20, "BONUS POINTS", 1, A5C(15, 9));
  if (df6_over > 0 && df6_over < 4.5f) a5_banner(H / 2 - 12, "THE END", 3, A5C(15, (fl & 2) ? 8 : 0));
  a5_crt();
}
void prog_defence_draw(int preset) { defence5_draw(preset); fx5_apply(); }

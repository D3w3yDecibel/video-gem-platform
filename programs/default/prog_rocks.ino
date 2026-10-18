// =====================================================================
// PROGRAM: ROCKS — blast space rocks in glowing vectors (by Dewey)
// Set 5 "Arcade", slot 3
//
// In the spirit of the 1979 vector rock-blaster: a little arrowhead ship
// drifts in a wrap-around field of tumbling rocks. Shoot a big rock and it
// splits in two, then two again, then turns to dust. Flying saucers cross
// now and then — the big one sprays shots, the little one aims. Clear the
// field and a bigger wave drifts in. Turn up the gravity well for a star in
// the middle that pulls everything into orbit.
//
// It plays itself; touch p0/p1 or k12 and you fly the ship.
//
// Presets:
//   k0 Classic   k1 Colour Vector   k2 Green Phosphor   k3 Amber
//   k4 Gravity Star   k5 Blizzard (lots of small rocks)   k6 Giants
//   k7 Saucer Attack   k8 Neon   k9 Bounce (walls instead of wrap-around)
//   k10 Deep Space (long trails)   k11 Tour — a new preset every wave
//   k12 — FIRE. Joins the game.
//
// Knobs:
//   p0 Ship Heading (the knob's angle is the ship's angle; touch to play)
//   p1 ◄ hyperspace (flick) | thrust ►
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Rocks per Wave (centre = preset)   p8 Rock Shape (round … jagged … square)
//   p9 Gravity Well (centre = preset)   p10 Scale (everything smaller … bigger)
//   p11 Line Glow (thin … thick halo)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 ship, 2 rocks A, 3 rocks B, 4 rocks C, 5 saucer, 6 shots, 7 flame, 8 star, 9 HUD, 10 enemy shots, 11 stars
struct Rk6Pre { uint8_t ship[3], rock[3][3]; uint8_t look, rocks, grav, big, saucers, bounce, glow, trails, stars; };
static const Rk6Pre rk6_pre[11] = {
  {{255,255,255},{{255,255,255},{255,255,255},{255,255,255}}, A5_ARCADE, 4, 0, 0, 1, 0, 1, 90, 0},
  {{255,255,255},{{255,90,90},{90,200,255},{255,220,60}}, A5_ARCADE, 4, 0, 0, 1, 0, 1, 90, 1},
  {{255,255,255},{{255,255,255},{220,220,220},{200,200,200}}, A5_PHOSPHOR, 5, 0, 0, 1, 0, 1, 170, 0},
  {{255,255,255},{{255,255,255},{220,220,220},{200,200,200}}, A5_AMBER, 5, 0, 0, 1, 0, 1, 170, 0},
  {{255,255,255},{{170,200,255},{255,170,120},{200,255,170}}, A5_ARCADE, 4, 30, 0, 1, 0, 1, 130, 1},
  {{255,255,255},{{200,230,255},{170,210,255},{240,250,255}}, A5_ARCADE, 10, 0, 2, 1, 0, 1, 60, 1},
  {{255,255,255},{{255,150,60},{200,110,60},{255,200,120}}, A5_ARCADE, 3, 0, 1, 1, 0, 1, 90, 1},
  {{255,255,255},{{150,150,170},{120,120,140},{180,180,200}}, A5_ARCADE, 3, 0, 0, 4, 0, 1, 90, 1},
  {{255,255,255},{{255,0,200},{0,255,255},{180,255,0}}, A5_NEON, 5, 0, 0, 1, 0, 2, 140, 1},
  {{255,255,255},{{255,255,120},{120,255,200},{255,160,255}}, A5_ARCADE, 5, 0, 0, 1, 1, 1, 90, 1},
  {{255,255,255},{{150,170,255},{255,140,200},{140,255,220}}, A5_ARCADE, 5, 20, 0, 1, 0, 1, 215, 1},
};
static const char* const rk6_names[12] = { "Classic", "Colour Vector", "Green Phosphor", "Amber", "Gravity Star", "Blizzard", "Giants", "Saucer Attack", "Neon", "Bounce", "Deep Space", "Tour" };
static const char* const rk6_lbl[16] = { "Ship Heading", "Hyperspace|Thrust", "", "", "", "", "", "Rocks per Wave", "Rock Shape", "Gravity Well", "Scale", "Line Glow", "", "", "", "" };
const char* prog_rocks_name() { return "ROCKS"; }
const char* prog_rocks_character() { return "Vector rock-blaster: split the tumbling rocks, dodge the saucers - plays itself until you touch p0/p1/k12"; }
const char* prog_rocks_presetName(int preset) { return (preset >= 0 && preset < 12) ? rk6_names[preset] : NULL; }
const char* prog_rocks_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, rk6_lbl); }
uint8_t prog_rocks_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define RK6_NR 40
#define RK6_NB 6
#define RK6_NEB 4
static float rk6_rx[RK6_NR], rk6_ry[RK6_NR], rk6_rvx[RK6_NR], rk6_rvy[RK6_NR], rk6_ra[RK6_NR], rk6_rva[RK6_NR];
static uint8_t rk6_rs[RK6_NR], rk6_rh[RK6_NR]; static uint32_t rk6_rseed[RK6_NR];   // size 0 = none, 1-3
static float rk6_bx[RK6_NB], rk6_by[RK6_NB], rk6_bvx[RK6_NB], rk6_bvy[RK6_NB], rk6_bt[RK6_NB];
static float rk6_ex[RK6_NEB], rk6_ey[RK6_NEB], rk6_evx[RK6_NEB], rk6_evy[RK6_NEB], rk6_et[RK6_NEB];
static float rk6_sx = 160, rk6_sy = 120, rk6_svx = 0, rk6_svy = 0, rk6_sa = -1.5708f, rk6_thr = 0;
static float rk6_ux = -50, rk6_uy = 0, rk6_uvx = 0, rk6_uvy = 0, rk6_uT = 10, rk6_ufire = 0;   // the saucer
static int rk6_uon = 0;                                                                      // 0 none, 1 big, 2 small
static float rk6_dead = 0, rk6_hyper = 0, rk6_fireT = 0, rk6_banner = 0, rk6_over = 0, rk6_safe = 0, rk6_scale = 1, rk6_grav = 0;
static int rk6_wave = 1, rk6_lives = 3, rk6_preset = -1, rk6_look = 0, rk6_shapeK = 0;
static uint32_t rk6_score = 0, rk6_hi = 10000, rk6_nextLife = 10000;
static float rk6_diff = 0.4f, rk6_p1was = 512, rk6_aiT = 0, rk6_aiAim = 0;
static bool rk6_bounce = false, rk6_aiThrust = false, rk6_aiFire = false;
static const float rk6_rad[4] = { 0, 7, 14, 27 };

static inline void rk6_wrap(float* x, float* y, float* vx, float* vy, float m) {
  if (rk6_bounce) {
    if (*x < m) { *x = m; *vx = fabsf(*vx); } if (*x > W - m) { *x = W - m; *vx = -fabsf(*vx); }
    if (*y < 12 + m) { *y = 12 + m; *vy = fabsf(*vy); } if (*y > H - m) { *y = H - m; *vy = -fabsf(*vy); }
  } else {
    if (*x < -m) *x += W + 2 * m; if (*x > W + m) *x -= W + 2 * m;
    if (*y < -m) *y += H + 2 * m; if (*y > H + m) *y -= H + 2 * m;
  }
}
static void rk6_addRock(float x, float y, int size, float spd) {
  for (int i = 0; i < RK6_NR; i++) if (!rk6_rs[i]) {
    float a = v3_rndf() * 6.2832f, s = spd * (0.5f + v3_rndf());
    rk6_rx[i] = x; rk6_ry[i] = y; rk6_rvx[i] = cosf(a) * s; rk6_rvy[i] = sinf(a) * s;
    rk6_ra[i] = v3_rndf() * 6.28f; rk6_rva[i] = (v3_rndf() - 0.5f) * 0.06f;
    rk6_rs[i] = (uint8_t)size; rk6_rh[i] = (uint8_t)(2 + v3_rnd(3)); rk6_rseed[i] = v3_rand();
    return;
  }
}
static void rk6_startWave() {
  memset(rk6_rs, 0, sizeof(rk6_rs));
  const Rk6Pre& P = rk6_pre[rk6_look];
  int n = v3_centre(7) ? P.rocks + rk6_wave - 1 : 1 + (int)(v3_u(7) * 12);
  n = v3_clamp(n, 1, 12);
  int size = P.big == 2 ? 2 : 3;
  if (P.big == 2) n *= 2;
  for (int k = 0; k < n; k++) {
    float x, y;
    do { x = v3_rndf() * W; y = 12 + v3_rndf() * (H - 12); } while (fabsf(x - rk6_sx) < 80 && fabsf(y - rk6_sy) < 70);
    rk6_addRock(x, y, size, 0.35f + rk6_diff * 0.3f + rk6_wave * 0.03f);
  }
  rk6_banner = 1.5f; rk6_uon = 0; rk6_uT = 8 + v3_rndf() * 10;
}
static void rk6_resetShip() { rk6_sx = W / 2; rk6_sy = rk6_grav > 0.01f ? H / 2 + 60 : H / 2 + 6; rk6_svx = rk6_svy = 0; rk6_sa = -1.5708f; rk6_safe = 2.0f; }
static void rk6_newGame() { rk6_score = 0; rk6_lives = 3; rk6_wave = 1; rk6_over = 0; rk6_dead = 0; rk6_nextLife = 10000; rk6_resetShip(); memset(rk6_bt, 0, sizeof(rk6_bt)); memset(rk6_et, 0, sizeof(rk6_et)); rk6_startWave(); }
static void rk6_breakRock(int i, bool score) {
  int s = rk6_rs[i]; float x = rk6_rx[i], y = rk6_ry[i];
  if (score) rk6_score += s == 3 ? 20 : (s == 2 ? 50 : 100);
  a5_burst((int)x, (int)y, 6 + s * 6, rk6_rh[i], 20 + s * 10, 26);
  if (s == 3) a5_shake(1.5f);
  rk6_rs[i] = 0;
  if (s > 1) { float sp = 0.6f + rk6_diff * 0.4f + rk6_wave * 0.03f; rk6_addRock(x, y, s - 1, sp); rk6_addRock(x, y, s - 1, sp); }
}
static void rk6_die() {
  if (rk6_dead > 0 || rk6_safe > 0) return;
  rk6_dead = 2.5f; a5_shake(5);
  a5_burst((int)rk6_sx, (int)rk6_sy, 50, 1, 50, 60);
}
// a rock's outline: 10 corners, its own jagged pattern
static void rk6_rockPts(int i, int* X, int* Y) {
  float r = rk6_rad[rk6_rs[i]] * rk6_scale;
  for (int k = 0; k < 10; k++) {
    float a = rk6_ra[i] + k * 0.6283f;
    uint32_t h = v3_hash(rk6_rseed[i] + k * 7919);
    float j = 0.72f + (h & 255) / 255.0f * 0.4f;
    if (rk6_shapeK == 0) j = 0.92f + (h & 15) / 100.0f;                    // round
    if (rk6_shapeK == 2) j = 0.5f + (h & 255) / 255.0f * 0.7f;             // very jagged
    float rr = r * j, ca = cosf(a), sa = sinf(a);
    if (rk6_shapeK == 3) { float m = fabsf(ca) > fabsf(sa) ? fabsf(ca) : fabsf(sa); rr = r * 0.8f / m; }   // square-ish
    X[k] = (int)((rk6_rx[i] + ca * rr) * 8); Y[k] = (int)((rk6_ry[i] + sa * rr) * 8);
  }
}

// the computer pilot
static void rk6_ai(float* turnTo, bool* thrust, bool* fire) {
  // the most dangerous rock: soonest to come within reach
  int threat = -1, target = -1; float tBest = 1e9f, dBest = 1e9f;
  for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) {
    float dx = rk6_rx[i] - rk6_sx, dy = rk6_ry[i] - rk6_sy;
    if (!rk6_bounce) { if (dx > W / 2) dx -= W; if (dx < -W / 2) dx += W; if (dy > H / 2) dy -= H; if (dy < -H / 2) dy += H; }
    float d = sqrtf(dx * dx + dy * dy) - rk6_rad[rk6_rs[i]] * rk6_scale;
    float rvx = rk6_rvx[i] - rk6_svx, rvy = rk6_rvy[i] - rk6_svy;
    float closing = -(dx * rvx + dy * rvy) / (d + rk6_rad[rk6_rs[i]] * rk6_scale + 0.01f);
    float t = closing > 0.05f ? d / closing : 1e6f;
    if (d < 40 && t < tBest) { tBest = t; threat = i; }
    float score = d - (closing > 0 ? closing * 40 : 0);
    if (score < dBest) { dBest = score; target = i; }
  }
  if (rk6_uon && (target < 0 || dBest > 60)) target = -2;
  float tx, ty, tvx, tvy;
  if (target == -2) { tx = rk6_ux; ty = rk6_uy; tvx = rk6_uvx; tvy = rk6_uvy; }
  else if (target >= 0) { tx = rk6_rx[target]; ty = rk6_ry[target]; tvx = rk6_rvx[target]; tvy = rk6_rvy[target]; }
  else { *turnTo = rk6_sa + 0.02f; *thrust = false; *fire = false; return; }
  float dx = tx - rk6_sx, dy = ty - rk6_sy;
  if (!rk6_bounce) { if (dx > W / 2) dx -= W; if (dx < -W / 2) dx += W; if (dy > H / 2) dy -= H; if (dy < -H / 2) dy += H; }
  float tt = sqrtf(dx * dx + dy * dy) / 4.0f;                   // shots fly ~4 px a step
  float ax = dx + (tvx - rk6_svx) * tt, ay = dy + (tvy - rk6_svy) * tt;
  *turnTo = atan2f(ay, ax);
  float da = *turnTo - rk6_sa; while (da > 3.1416f) da -= 6.2832f; while (da < -3.1416f) da += 6.2832f;
  *fire = fabsf(da) < 0.12f;
  *thrust = false;
  if (threat >= 0 && tBest < 30) {                                // a rock on its way: sidestep at right angles
    float rx = rk6_rx[threat] - rk6_sx, ry = rk6_ry[threat] - rk6_sy;
    float side = atan2f(ry, rx) + 1.5708f;
    float ds = side - rk6_sa; while (ds > 3.1416f) ds -= 6.2832f; while (ds < -3.1416f) ds += 6.2832f;
    if (fabsf(ds) < 0.6f || fabsf(ds) > 2.5f) *thrust = true;
    if (tBest < 8 && v3_rnd(3) == 0) *thrust = true;
  }
  float sp = sqrtf(rk6_svx * rk6_svx + rk6_svy * rk6_svy);
  if (sp > 1.4f) *thrust = false;
  if (rk6_grav > 0.01f) {                                         // keep in orbit: never fall into the star
    float cx = rk6_sx - W / 2, cy = rk6_sy - H / 2, cd = sqrtf(cx * cx + cy * cy) + 0.01f;
    float inward = -(cx * rk6_svx + cy * rk6_svy) / cd;
    if (cd < 70 && inward > 0.2f) { *turnTo = atan2f(cy, cx) + 0.7f; *thrust = true; }
  }
  // drift back to the middle if we wander
  if (!*thrust && (fabsf(rk6_sx - W / 2) > 100 || fabsf(rk6_sy - H / 2) > 80) && sp < 0.4f && threat < 0) {
    float home = atan2f(H / 2 - rk6_sy, W / 2 - rk6_sx), dh = home - rk6_sa;
    while (dh > 3.1416f) dh -= 6.2832f; while (dh < -3.1416f) dh += 6.2832f;
    if (fabsf(dh) < 0.3f) *thrust = true;
  }
}

void prog_rocks_init() { v3_reset(); a5_reset(); rk6_preset = -1; a5_partClear(); }

static void rocks5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != rk6_preset) { rk6_preset = preset; rk6_look = preset == 11 ? 0 : preset; rk6_newGame(); }
  const Rk6Pre& P = rk6_pre[rk6_look];
  rk6_diff = a5_diffK(0.35f);
  rk6_shapeK = v3_centre(8) ? 1 : v3_clamp(v3_k(8) * 4 / 1024, 0, 3);
  rk6_grav = v3_centre(9) ? P.grav / 100.0f : v3_u(9);
  rk6_scale = v3_centre(10) ? (P.big == 1 ? 1.5f : 1.0f) : 0.5f + v3_u(10) * 1.3f;
  int glow = v3_centre(11) ? P.glow : (int)(v3_u(11) * 2.99f);
  rk6_bounce = P.bounce;
  bool halo = glow == 2;

  uint8_t hues[12][3] = { {255,255,255},{0},{0},{0},{0},{150,255,150},{255,255,255},{255,140,40},{255,230,150},{255,255,255},{255,120,120},{180,190,255} };
  memcpy(hues[1], P.ship, 3); memcpy(hues[2], P.rock[0], 3); memcpy(hues[3], P.rock[1], 3); memcpy(hues[4], P.rock[2], 3);
  a5_palette(hues, 12, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (rk6_banner > 0) rk6_banner -= dt;
    if (rk6_safe > 0) rk6_safe -= dt;
    if (rk6_over > 0) { rk6_over -= dt; if (rk6_over <= 0) { if (rk6_preset == 11) rk6_look = (rk6_look + 1) % 11; rk6_newGame(); } }
    // gravity well pulls at (x, y)
    #define RK6_PULL(px, py, pvx, pvy, k) if (rk6_grav > 0.01f) { float gx = W / 2 - (px), gy = H / 2 - (py), r2 = gx * gx + gy * gy + 300; float f = rk6_grav * (k) * 120.0f / (r2 * sqrtf(r2)); (pvx) += gx * f; (pvy) += gy * f; }
    // rocks
    for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) {
      RK6_PULL(rk6_rx[i], rk6_ry[i], rk6_rvx[i], rk6_rvy[i], 1.0f)
      float sp = sqrtf(rk6_rvx[i] * rk6_rvx[i] + rk6_rvy[i] * rk6_rvy[i]); if (sp > 1.8f) { rk6_rvx[i] *= 1.8f / sp; rk6_rvy[i] *= 1.8f / sp; }
      rk6_rx[i] += rk6_rvx[i]; rk6_ry[i] += rk6_rvy[i]; rk6_ra[i] += rk6_rva[i];
      rk6_wrap(&rk6_rx[i], &rk6_ry[i], &rk6_rvx[i], &rk6_rvy[i], rk6_rad[rk6_rs[i]] * rk6_scale);
      if (rk6_grav > 0.01f && fabsf(rk6_rx[i] - W / 2) < 8 && fabsf(rk6_ry[i] - H / 2) < 8) { rk6_rvx[i] = (rk6_rx[i] - W / 2) * 0.3f + (v3_rndf() - 0.5f); rk6_rvy[i] = (rk6_ry[i] - H / 2) * 0.3f + (v3_rndf() - 0.5f); }   // slingshot round the star
    }
    // the ship
    if (rk6_dead > 0) {
      rk6_dead -= dt;
      if (rk6_dead <= 0) {
        if (--rk6_lives < 0) { rk6_over = 4; rk6_dead = 0; if (rk6_score > rk6_hi) rk6_hi = rk6_score; rk6_lives = 0; rk6_dead = 4.1f; }
        else rk6_resetShip();
      }
    } else if (rk6_hyper > 0) {
      rk6_hyper -= dt; if (rk6_hyper <= 0) { rk6_sx = 30 + v3_rndf() * (W - 60); rk6_sy = 30 + v3_rndf() * (H - 60); rk6_svx = rk6_svy = 0; }
    } else {
      bool thrust, fire; float turnTo;
      if (a5_player) {
        turnTo = v3_u(0) * 6.2832f - 1.5708f;
        float t = a5_stick(1); thrust = t > 0.1f; fire = a5_fire || (a5_fireHeld && rk6_fireT < -0.25f);
        float p1 = v3_sm[1];
        if (p1 < 40 && rk6_p1was >= 40) { rk6_hyper = 0.6f; a5_burst((int)rk6_sx, (int)rk6_sy, 16, 1, 30, 20); }
        rk6_p1was = p1;
      } else {
        rk6_aiT -= dt;
        if (rk6_aiT <= 0) { rk6_ai(&rk6_aiAim, &rk6_aiThrust, &rk6_aiFire); rk6_aiT = 0.05f + v3_rndf() * 0.1f; }
        turnTo = rk6_aiAim; thrust = rk6_aiThrust; fire = rk6_aiFire && v3_rnd(4) == 0;
      }
      float da = turnTo - rk6_sa; while (da > 3.1416f) da -= 6.2832f; while (da < -3.1416f) da += 6.2832f;
      rk6_sa += v3_clampf(da, -0.09f, 0.09f);
      if (thrust) { rk6_svx += cosf(rk6_sa) * 0.06f; rk6_svy += sinf(rk6_sa) * 0.06f; }
      rk6_thr = thrust ? 1.0f : 0.0f;
      RK6_PULL(rk6_sx, rk6_sy, rk6_svx, rk6_svy, 1.0f)
      rk6_svx *= 0.992f; rk6_svy *= 0.992f;
      float sp = sqrtf(rk6_svx * rk6_svx + rk6_svy * rk6_svy); if (sp > 3.0f) { rk6_svx *= 3.0f / sp; rk6_svy *= 3.0f / sp; }
      rk6_sx += rk6_svx; rk6_sy += rk6_svy;
      rk6_wrap(&rk6_sx, &rk6_sy, &rk6_svx, &rk6_svy, 6);
      if (rk6_grav > 0.3f && fabsf(rk6_sx - W / 2) < 6 && fabsf(rk6_sy - H / 2) < 6) rk6_die();
      rk6_fireT -= dt;
      if (fire && rk6_fireT <= 0) {
        for (int b = 0; b < RK6_NB; b++) if (rk6_bt[b] <= 0) {
          rk6_bx[b] = rk6_sx + cosf(rk6_sa) * 8 * rk6_scale; rk6_by[b] = rk6_sy + sinf(rk6_sa) * 8 * rk6_scale;
          rk6_bvx[b] = rk6_svx + cosf(rk6_sa) * 4; rk6_bvy[b] = rk6_svy + sinf(rk6_sa) * 4; rk6_bt[b] = 1.1f; rk6_fireT = 0.12f; break;
        }
      }
      // crashing into a rock
      for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) {
        float dx = rk6_rx[i] - rk6_sx, dy = rk6_ry[i] - rk6_sy, r = rk6_rad[rk6_rs[i]] * rk6_scale * 0.85f + 4 * rk6_scale;
        if (dx * dx + dy * dy < r * r && rk6_safe <= 0) { rk6_breakRock(i, true); rk6_die(); break; }
      }
    }
    // shots
    for (int b = 0; b < RK6_NB; b++) if (rk6_bt[b] > 0) {
      rk6_bt[b] -= dt; RK6_PULL(rk6_bx[b], rk6_by[b], rk6_bvx[b], rk6_bvy[b], 0.5f)
      rk6_bx[b] += rk6_bvx[b]; rk6_by[b] += rk6_bvy[b];
      rk6_wrap(&rk6_bx[b], &rk6_by[b], &rk6_bvx[b], &rk6_bvy[b], 0);
      for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) {
        float dx = rk6_rx[i] - rk6_bx[b], dy = rk6_ry[i] - rk6_by[b], r = rk6_rad[rk6_rs[i]] * rk6_scale;
        if (dx * dx + dy * dy < r * r) { rk6_breakRock(i, true); rk6_bt[b] = 0; break; }
      }
      if (rk6_bt[b] > 0 && rk6_uon) {
        float dx = rk6_ux - rk6_bx[b], dy = rk6_uy - rk6_by[b], r = (rk6_uon == 1 ? 12 : 7) * rk6_scale;
        if (dx * dx + dy * dy < r * r) { rk6_score += rk6_uon == 1 ? 200 : 1000; a5_burst((int)rk6_ux, (int)rk6_uy, 40, 5, 50, 40); a5_shake(3); rk6_uon = 0; rk6_uT = 10 + v3_rndf() * 10; rk6_bt[b] = 0; }
      }
    }
    // the saucer
    if (!rk6_uon && rk6_over <= 0) {
      rk6_uT -= dt * (P.saucers > 1 ? 3.0f : 1.0f);
      if (rk6_uT <= 0) { rk6_uon = (rk6_score > 3000 && v3_rnd(2)) || v3_rnd(4) == 0 ? 2 : 1; rk6_ux = v3_rnd(2) ? -10 : W + 10; rk6_uy = 30 + v3_rndf() * (H - 60); rk6_uvx = (rk6_ux < 0 ? 1 : -1) * (0.8f + rk6_diff * 0.6f); rk6_uvy = 0; rk6_ufire = 1; }
    } else if (rk6_uon) {
      rk6_ux += rk6_uvx; rk6_uy += rk6_uvy;
      if (v3_rnd(90) == 0) rk6_uvy = (v3_rnd(3) - 1) * 0.7f;
      if (rk6_uy < 20) rk6_uvy = 0.7f; if (rk6_uy > H - 10) rk6_uvy = -0.7f;
      if (rk6_ux < -20 || rk6_ux > W + 20) { rk6_uon = 0; rk6_uT = 10 + v3_rndf() * 12; }
      rk6_ufire -= dt;
      if (rk6_ufire <= 0 && rk6_dead <= 0) {
        rk6_ufire = 1.2f - rk6_diff * 0.6f;
        float a = rk6_uon == 2 ? atan2f(rk6_sy - rk6_uy, rk6_sx - rk6_ux) + (v3_rndf() - 0.5f) * (0.6f - rk6_diff * 0.5f) : v3_rndf() * 6.2832f;
        for (int b = 0; b < RK6_NEB; b++) if (rk6_et[b] <= 0) { rk6_ex[b] = rk6_ux; rk6_ey[b] = rk6_uy; rk6_evx[b] = cosf(a) * 2.6f; rk6_evy[b] = sinf(a) * 2.6f; rk6_et[b] = 1.4f; break; }
      }
      for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) { float dx = rk6_rx[i] - rk6_ux, dy = rk6_ry[i] - rk6_uy, r = rk6_rad[rk6_rs[i]] * rk6_scale + 6; if (dx * dx + dy * dy < r * r) { rk6_breakRock(i, false); a5_burst((int)rk6_ux, (int)rk6_uy, 30, 5, 40, 30); rk6_uon = 0; rk6_uT = 10; break; } }
      if (rk6_uon && rk6_dead <= 0) { float dx = rk6_ux - rk6_sx, dy = rk6_uy - rk6_sy; if (dx * dx + dy * dy < 120) rk6_die(); }
    }
    for (int b = 0; b < RK6_NEB; b++) if (rk6_et[b] > 0) {
      rk6_et[b] -= dt; rk6_ex[b] += rk6_evx[b]; rk6_ey[b] += rk6_evy[b];
      rk6_wrap(&rk6_ex[b], &rk6_ey[b], &rk6_evx[b], &rk6_evy[b], 0);
      if (rk6_dead <= 0 && rk6_hyper <= 0) { float dx = rk6_ex[b] - rk6_sx, dy = rk6_ey[b] - rk6_sy; if (dx * dx + dy * dy < 30 * rk6_scale) { rk6_die(); rk6_et[b] = 0; } }
    }
    if (rk6_score >= rk6_nextLife) { rk6_lives++; rk6_nextLife += 10000; }
    // wave cleared
    bool any = false; for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) any = true;
    if (!any && rk6_over <= 0) { rk6_wave++; if (rk6_preset == 11) rk6_look = (rk6_look + 1) % 11; rk6_startWave(); }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (P.stars) a5_stars(70, 0, 0, 11, 3);
  if (rk6_grav > 0.01f) {                                          // the star in the middle
    int r = 3 + (int)(rk6_grav * 4);
    for (int k = 0; k < 8; k++) { int a = k * 128 + (int)(globalTime * 300); v3_lineAA(160 * 8, 120 * 8, 160 * 8 + ((v3_cos(a) * r * 3) >> 11), 120 * 8 + ((v3_sin(a) * r * 3) >> 11), 10, 8); }
    v3_disc(160, 120, r / 2, A5C(15, 8));
  }
  for (int i = 0; i < RK6_NR; i++) if (rk6_rs[i]) {
    int X[10], Y[10]; rk6_rockPts(i, X, Y);
    int hue = (P.rock[0][0] == P.rock[1][0] && P.rock[0][1] == P.rock[1][1]) ? 2 : rk6_rh[i];
    float r = rk6_rad[rk6_rs[i]] * rk6_scale * 1.2f;
    int ox[2] = { 0, rk6_rx[i] < r ? W * 8 : (rk6_rx[i] > W - r ? -W * 8 : 0) }, oy[2] = { 0, rk6_ry[i] < r ? H * 8 : (rk6_ry[i] > H - r ? -H * 8 : 0) };
    for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) {           // drawn again on the far side when it straddles an edge
      if ((a && !ox[1]) || (b && !oy[1]) || rk6_bounce && (a || b)) continue;
      for (int k = 0; k < 10; k++) a5_glow(X[k] + ox[a], Y[k] + oy[b], X[(k + 1) % 10] + ox[a], Y[(k + 1) % 10] + oy[b], 15, hue, halo);
    }
  }
  for (int b = 0; b < RK6_NB; b++) if (rk6_bt[b] > 0) { v3_rect((int)rk6_bx[b], (int)rk6_by[b], (int)rk6_bx[b] + 1, (int)rk6_by[b] + 1, A5C(15, 6)); }
  for (int b = 0; b < RK6_NEB; b++) if (rk6_et[b] > 0) { v3_rect((int)rk6_ex[b], (int)rk6_ey[b], (int)rk6_ex[b] + 1, (int)rk6_ey[b] + 1, A5C(15, 10)); }
  if (rk6_uon) {                                                     // a saucer: two hulls and a dome
    float s = (rk6_uon == 1 ? 1.0f : 0.6f) * rk6_scale; int x = (int)(rk6_ux * 8), y = (int)(rk6_uy * 8);
    int a = (int)(12 * s * 8), b = (int)(5 * s * 8), c = (int)(4 * s * 8), d = (int)(9 * s * 8);
    int X[8] = { x - a, x - b, x + b, x + a, x + b, x - b, x - c, x + c }, Y[8] = { y, y - (int)(3 * s * 8), y - (int)(3 * s * 8), y, y + (int)(4 * s * 8), y + (int)(4 * s * 8), y - (int)(7 * s * 8), y - (int)(7 * s * 8) };
    a5_glow(X[0], Y[0], X[1], Y[1], 15, 5, halo); a5_glow(X[1], Y[1], X[2], Y[2], 15, 5, halo); a5_glow(X[2], Y[2], X[3], Y[3], 15, 5, halo);
    a5_glow(X[3], Y[3], X[4], Y[4], 15, 5, halo); a5_glow(X[4], Y[4], X[5], Y[5], 15, 5, halo); a5_glow(X[5], Y[5], X[0], Y[0], 15, 5, halo);
    a5_glow(X[0], Y[0], X[3], Y[3], 12, 5, false);
    a5_glow(X[1], Y[1], X[6], Y[6], 15, 5, halo); a5_glow(X[6], Y[6], X[7], Y[7], 15, 5, halo); a5_glow(X[7], Y[7], X[2], Y[2], 15, 5, halo);
    (void)d;
  }
  // the ship: an arrowhead with a notch, and a flickering flame
  if (rk6_dead <= 0 && rk6_hyper <= 0 && (rk6_safe <= 0 || ((int)(rk6_safe * 8) & 1))) {
    float c = cosf(rk6_sa), s = sinf(rk6_sa), k = rk6_scale * 8;
    #define RK6_P(fx, fy, X, Y) int X = (int)((rk6_sx + ((fx) * c - (fy) * s) * rk6_scale) * 8), Y = (int)((rk6_sy + ((fx) * s + (fy) * c) * rk6_scale) * 8);
    RK6_P(9, 0, x0, y0) RK6_P(-6, -6, x1, y1) RK6_P(-3, 0, x2, y2) RK6_P(-6, 6, x3, y3)
    a5_glow(x0, y0, x1, y1, 15, 1, halo); a5_glow(x1, y1, x2, y2, 15, 1, halo); a5_glow(x2, y2, x3, y3, 15, 1, halo); a5_glow(x3, y3, x0, y0, 15, 1, halo);
    if (rk6_thr > 0 && ((int)(globalTime * 30) & 1)) { RK6_P(-11 - v3_rnd(4), 0, fx, fy) RK6_P(-4, -3, f1x, f1y) RK6_P(-4, 3, f2x, f2y) v3_lineAA(f1x, f1y, fx, fy, 15, 7); v3_lineAA(f2x, f2y, fx, fy, 15, 7); }
    (void)k;
  } else if (rk6_dead > 0 && rk6_dead < 4) {                         // the ship in pieces, drifting apart
    float t = 2.5f - rk6_dead;
    for (int q = 0; q < 4; q++) {
      float a = q * 1.7f + 0.3f, r = 4 + t * 14, x = rk6_sx + cosf(a) * r, y = rk6_sy + sinf(a) * r, ra = a + t * 3;
      if (t < 2.0f) v3_lineAA((int)(x * 8), (int)(y * 8), (int)((x + cosf(ra) * 7) * 8), (int)((y + sinf(ra) * 7) * 8), 15 - (int)(t * 5), 1);
    }
  }
  a5_partDraw();

  a5_post((int)rk6_sx, (int)rk6_sy);
  a5_hud(rk6_score, rk6_hi, A5C(15, 9), A5C(15, 9));
  for (int i = 0; i < v3_clamp(rk6_lives, 0, 6); i++) { int x = 8 + i * 10, y = 20; a5_seg(x, y - 5, x - 3, y + 3, A5C(15, 1)); a5_seg(x, y - 5, x + 3, y + 3, A5C(15, 1)); a5_seg(x - 2, y + 1, x + 2, y + 1, A5C(15, 1)); }
  char s[16]; snprintf(s, 16, "WAVE %d", rk6_wave); a5_textS(W - 54, 2, s, 1, A5C(12, 9));
  if (rk6_banner > 0) a5_banner(H / 2 - 40, s, 2, A5C(15, 9));
  if (rk6_over > 0) a5_banner(H / 2 - 8, "GAME OVER", 2, A5C(15, 9));
  a5_crt();
}
void prog_rocks_draw(int preset) { rocks5_draw(preset); fx5_apply(); }

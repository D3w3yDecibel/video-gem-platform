// =====================================================================
// PROGRAM: TUBE — hold the rim of a glowing vector tube (by Dewey)
// Set 5 "Arcade", slot 2
//
// In the spirit of the 1981 vector tube shooter: you're a claw on the near
// rim of a 3-D tube made of lanes, seen in glowing vector lines. HINGES
// (bow-ties) climb up the lanes, flipping from lane to lane, and creep
// round the rim to grab you; SPIRALS climb half way and leave spike trails
// behind; DIAMONDS split into two hinges when hit. Clear a wave and you
// warp down the tube to the next shape — circles, squares, stars, crosses,
// flat planes, V's and W's — each in a new colour.
//
// It plays itself; turn p0 (it's a spinner: the knob's position is your
// place on the rim) or press k12 and you're playing.
//
// Presets:
//   k0 Classic Blue   k1 Red Rim   k2 Gold   k3 Cyan Deep   k4 Invisible (rims only)
//   k5 Rainbow Lanes   k6 Swarm   k7 Spike Field   k8 Spinning Tube   k9 Long Tube
//   k10 Neon Glow   k11 Tour — every wave a new preset
//   k12 — FIRE (hold for rapid fire). Joins the game.
//
// Knobs:
//   p0 Spinner (your place round the rim; touch to play)
//   p1 Superzapper — flick it to either end to clear the tube (once a wave)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Tube Shape (centre = the wave's own; else 12 shapes)
//   p8 Perspective (flat … very deep)   p9 Enemy Count
//   p10 Tube Spin (◄ anticlockwise | clockwise ►, centre = still)
//   p11 Line Glow (thin … thick halo)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 tube, 2 claw, 3 hinge, 4 spiral, 5 diamond, 6 shots, 7 spikes, 8 bang, 9 HUD, 10 enemy bullets, 11 stars
struct Tb6Pre { uint8_t tube[3], claw[3]; uint8_t look, enemies, spikes, spin, depth, glow, rainbow, invisible, trails; };
static const Tb6Pre tb6_pre[11] = {
  {{40,70,255},{255,230,40}, A5_ARCADE, 10, 0, 0, 4, 1, 0, 0, 110},
  {{255,40,40},{255,230,40}, A5_ARCADE, 12, 1, 0, 6, 1, 0, 0, 110},
  {{255,200,0},{40,140,255}, A5_ARCADE, 12, 1, 0, 6, 1, 0, 0, 110},
  {{0,230,255},{255,80,200}, A5_ARCADE, 12, 1, 0, 10, 1, 0, 0, 140},
  {{120,120,255},{255,230,40}, A5_ARCADE, 12, 1, 0, 6, 1, 0, 1, 150},
  {{255,255,255},{255,255,255}, A5_ARCADE, 12, 1, 0, 6, 1, 1, 0, 110},
  {{60,255,120},{255,230,40}, A5_ARCADE, 24, 0, 0, 6, 1, 0, 0, 90},
  {{200,60,255},{255,230,40}, A5_ARCADE, 12, 3, 0, 6, 1, 0, 0, 110},
  {{40,70,255},{255,230,40}, A5_ARCADE, 12, 1, 1, 6, 1, 0, 0, 170},
  {{255,120,0},{0,200,255}, A5_ARCADE, 12, 1, 0, 14, 1, 0, 0, 130},
  {{255,0,220},{0,255,255}, A5_NEON, 14, 1, 0, 6, 2, 0, 0, 180},
};
static const char* const tb6_names[12] = { "Classic Blue", "Red Rim", "Gold", "Cyan Deep", "Invisible", "Rainbow Lanes", "Swarm", "Spike Field", "Spinning Tube", "Long Tube", "Neon Glow", "Tour" };
static const char* const tb6_lbl[16] = { "Spinner", "Superzapper (flick)", "", "", "", "", "", "Tube Shape", "Perspective", "Enemy Count", "Tube Spin", "Line Glow", "", "", "", "" };
const char* prog_tube_name() { return "TUBE"; }
const char* prog_tube_character() { return "Hold the rim of a glowing vector tube against climbing hinges - plays itself until you touch p0/k12"; }
const char* prog_tube_presetName(int preset) { return (preset >= 0 && preset < 12) ? tb6_names[preset] : NULL; }
const char* prog_tube_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, tb6_lbl); }
uint8_t prog_tube_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ── tube shapes: 16 lanes; open shapes have 17 rim points ──
#define TB6_L 16
#define TB6_NSHAPE 12
static const bool tb6_open[TB6_NSHAPE] = { false, false, false, false, false, true, true, true, false, true, true, false };
static float tb6_sx[TB6_L + 1], tb6_sy[TB6_L + 1];            // the shape, −1 … 1
// walk a closed polygon at equal steps of its perimeter
static void tb6_polyWalk(const float* px, const float* py, int n) {
  float len = 0, seg[16];
  for (int i = 0; i < n; i++) { int j = (i + 1) % n; seg[i] = sqrtf((px[j] - px[i]) * (px[j] - px[i]) + (py[j] - py[i]) * (py[j] - py[i])); len += seg[i]; }
  for (int k = 0; k < TB6_L; k++) {
    float t = len * k / TB6_L; int i = 0;
    while (i < n - 1 && t > seg[i]) { t -= seg[i]; i++; }
    int j = (i + 1) % n; float f = seg[i] > 0 ? t / seg[i] : 0;
    tb6_sx[k] = px[i] + (px[j] - px[i]) * f; tb6_sy[k] = py[i] + (py[j] - py[i]) * f;
  }
  tb6_sx[TB6_L] = tb6_sx[0]; tb6_sy[TB6_L] = tb6_sy[0];
}
static void tb6_shape(int sh) {
  const float PI2 = 6.2831853f;
  switch (sh) {
    case 1: { const float x[4] = {-0.85f, 0.85f, 0.85f, -0.85f}, y[4] = {-0.85f, -0.85f, 0.85f, 0.85f}; tb6_polyWalk(x, y, 4); break; }
    case 2: { const float x[12] = {-0.3f,0.3f,0.3f,0.9f,0.9f,0.3f,0.3f,-0.3f,-0.3f,-0.9f,-0.9f,-0.3f}, y[12] = {-0.9f,-0.9f,-0.3f,-0.3f,0.3f,0.3f,0.9f,0.9f,0.3f,0.3f,-0.3f,-0.3f}; tb6_polyWalk(x, y, 12); break; }
    case 4: { const float x[3] = {0.0f, 0.95f, -0.95f}, y[3] = {-0.95f, 0.7f, 0.7f}; tb6_polyWalk(x, y, 3); break; }
    case 3: case 8: case 11: case 0: default:
      for (int k = 0; k <= TB6_L; k++) {
        float a = PI2 * k / TB6_L - 1.5708f, r = 0.95f;
        if (sh == 3) r = 0.62f + 0.35f * cosf(5 * a + 1.5708f * 5);
        if (sh == 8) r = 0.72f + 0.25f * cosf(2 * a);
        if (sh == 11) r = 0.5f + 0.45f * fabsf(cosf(2 * a));
        tb6_sx[k] = cosf(a) * r; tb6_sy[k] = sinf(a) * r;
      }
      if (sh == 3 || sh == 8 || sh == 11 || sh == 0) break;
      break;
    case 5: for (int k = 0; k <= TB6_L; k++) { tb6_sx[k] = -1.0f + 2.0f * k / TB6_L; tb6_sy[k] = 0.55f; } break;
    case 6: for (int k = 0; k <= TB6_L; k++) { float x = -1.0f + 2.0f * k / TB6_L; tb6_sx[k] = x; tb6_sy[k] = -0.2f + 0.85f * fabsf(x); } break;
    case 7: for (int k = 0; k <= TB6_L; k++) { float a = 3.14159f * k / TB6_L; tb6_sx[k] = -cosf(a) * 0.95f; tb6_sy[k] = 0.05f + 0.8f * sinf(a); } break;
    case 9: for (int k = 0; k <= TB6_L; k++) { float x = -1.0f + 2.0f * k / TB6_L, t = fabsf(fmodf((x + 1.0f) * 2.0f, 2.0f) - 1.0f); tb6_sx[k] = x; tb6_sy[k] = 0.1f + 0.6f * t; } break;
    case 10: for (int k = 0; k <= TB6_L; k++) { tb6_sx[k] = -1.0f + 2.0f * k / TB6_L; tb6_sy[k] = 0.75f - 0.18f * (k / 4); } break;
  }
}
static int tb6_shapeNow = 0;
static bool tb6_isOpen = false;
static float tb6_D = 6, tb6_cam = 0, tb6_spinA = 0;
static float tb6_rx[TB6_L + 1], tb6_ry[TB6_L + 1];            // rim points on screen at z = 0 (before perspective)
static const float tb6_VX = 160, tb6_VY = 92, tb6_R = 120, tb6_OY = 30;
static inline float tb6_s(float z) { float q = 1.0f + (z - tb6_cam) * tb6_D; return q > 0.05f ? 1.0f / q : 20.0f; }
// a point on lane boundary k at depth z (screen px)
static inline void tb6_pt(int k, float z, float* x, float* y) { float s = tb6_s(z); *x = tb6_VX + tb6_rx[k] * s; *y = tb6_VY + tb6_ry[k] * s; }
// the middle of lane l at depth z, with lane position f in 0 … 1 across the lane
static inline void tb6_lanePt(float l, float z, float* x, float* y) {
  int a = (int)floorf(l); float f = l - a;
  if (tb6_isOpen) { if (a < 0) { a = 0; f = 0; } if (a > TB6_L - 1) { a = TB6_L - 1; f = 1; } }
  a = ((a % TB6_L) + TB6_L) % TB6_L;
  float x0, y0, x1, y1; tb6_pt(a, z, &x0, &y0); tb6_pt(a + 1, z, &x1, &y1);
  *x = x0 + (x1 - x0) * f; *y = y0 + (y1 - y0) * f;
}

// ── game state ──
#define TB6_NE 24
#define TB6_NS 8
#define TB6_NB 6
enum { TB6_HINGE, TB6_SPIRAL, TB6_DIAMOND };
static float tb6_ez[TB6_NE], tb6_ef[TB6_NE], tb6_ev[TB6_NE];     // depth, flip progress, speed
static int tb6_el[TB6_NE], tb6_et[TB6_NE], tb6_es[TB6_NE], tb6_edir[TB6_NE], tb6_eft[TB6_NE];   // lane, type, state 0 off 1 climbing 2 rim 3 retreat, flip dir, flip timer
static float tb6_shz[TB6_NS]; static int tb6_shl[TB6_NS];        // shots (z < 0: none)
static float tb6_bz[TB6_NB]; static int tb6_bl[TB6_NB];          // enemy bullets
static float tb6_spike[TB6_L];                                   // spike reaches up to this depth (1 = none)
static float tb6_pp = 0, tb6_warp = 0, tb6_dead = 0, tb6_fireT = 0, tb6_spawnT = 0, tb6_banner = 0, tb6_over = 0;
static int tb6_wave = 1, tb6_lives = 3, tb6_toSpawn = 0, tb6_preset = -1, tb6_look = 0, tb6_zaps = 1, tb6_grabbed = -1;
static uint32_t tb6_score = 0, tb6_hi = 10000;
static float tb6_diff = 0.4f, tb6_p1was = 512, tb6_aiT = 0, tb6_aiTarget = 0;

static int tb6_playerLane() { int l = (int)floorf(tb6_pp); return tb6_isOpen ? v3_clamp(l, 0, TB6_L - 1) : ((l % TB6_L) + TB6_L) % TB6_L; }
static int tb6_laneDist(int a, int b) { int d = b - a; if (!tb6_isOpen) { d = ((d % TB6_L) + TB6_L) % TB6_L; if (d > TB6_L / 2) d -= TB6_L; } return d; }
static void tb6_startWave() {
  for (int i = 0; i < TB6_NE; i++) tb6_es[i] = 0;
  for (int i = 0; i < TB6_NS; i++) tb6_shz[i] = -1;
  for (int i = 0; i < TB6_NB; i++) tb6_bz[i] = -1;
  for (int l = 0; l < TB6_L; l++) tb6_spike[l] = 1.0f;
  tb6_toSpawn = 0; tb6_spawnT = 1.0f; tb6_warp = 0; tb6_cam = 0; tb6_zaps = 1; tb6_banner = 1.6f; tb6_grabbed = -1;
}
static void tb6_newGame() { tb6_score = 0; tb6_lives = 3; tb6_wave = 1; tb6_over = 0; tb6_dead = 0; tb6_startWave(); }
static void tb6_spawn(int type, int lane, float z) {
  for (int i = 0; i < TB6_NE; i++) if (tb6_es[i] == 0) {
    tb6_es[i] = 1; tb6_et[i] = type; tb6_el[i] = lane; tb6_ez[i] = z; tb6_ef[i] = 0; tb6_eft[i] = 0; tb6_edir[i] = v3_rnd(2) ? 1 : -1;
    tb6_ev[i] = (0.07f + tb6_diff * 0.08f + tb6_wave * 0.006f) * (type == TB6_SPIRAL ? 1.3f : (type == TB6_DIAMOND ? 0.8f : 1.0f));
    return;
  }
}
static void tb6_bang(float l, float z, int hue, int n) { float x, y; tb6_lanePt(l, z, &x, &y); a5_burst((int)x, (int)y, n, hue, 30 + (int)(50 * tb6_s(z)), 30); }
static void tb6_killEnemy(int i, bool score) {
  if (score) tb6_score += tb6_et[i] == TB6_HINGE ? 150 : (tb6_et[i] == TB6_SPIRAL ? 50 : 100);
  tb6_bang(tb6_el[i] + 0.5f, tb6_ez[i], tb6_et[i] == TB6_HINGE ? 3 : (tb6_et[i] == TB6_SPIRAL ? 4 : 5), 22);
  if (tb6_et[i] == TB6_DIAMOND && score) {                       // diamonds split into two hinges
    int l = tb6_el[i]; float z = tb6_ez[i]; tb6_es[i] = 0;
    tb6_spawn(TB6_HINGE, l, z); tb6_spawn(TB6_HINGE, tb6_isOpen ? v3_clamp(l + 1, 0, TB6_L - 1) : (l + 1) % TB6_L, z);
    return;
  }
  tb6_es[i] = 0;
}
static void tb6_die() {
  if (tb6_dead > 0) return;
  tb6_dead = 2.2f; a5_shake(6);
  float x, y; tb6_lanePt(tb6_pp, 0, &x, &y); a5_burst((int)x, (int)y, 60, 2, 70, 50);
}

// the computer: go to the lane of the most urgent enemy and fire
static float tb6_ai(bool* fire) {
  *fire = true;
  int pl = tb6_playerLane(), best = -1; float bs = -1e9f;
  for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) {
    float urg = (1.0f - tb6_ez[i]) * 10 - v3_abs(tb6_laneDist(pl, tb6_el[i])) * 0.6f;
    if (tb6_es[i] == 2) urg += 12;
    if (urg > bs) { bs = urg; best = i; }
  }
  float target = tb6_pp;
  if (best >= 0) {
    int d = tb6_laneDist(pl, tb6_el[best]);
    // a rim crawler next door coming our way: hold still and let it walk into the shots
    if (tb6_es[best] == 2 && v3_abs(d) == 1) d = 0;
    target = pl + 0.5f + d;
  }
  // don't sit in a lane with a spike reaching close, during a warp
  return target;
}

void prog_tube_init() { v3_reset(); a5_reset(); tb6_preset = -1; a5_partClear(); }

static void tube5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != tb6_preset) { tb6_preset = preset; tb6_look = preset == 11 ? 0 : preset; tb6_newGame(); }
  const Tb6Pre& P = tb6_pre[tb6_look];
  tb6_diff = a5_diffK(0.35f);
  int shape = v3_centre(7) ? (tb6_wave - 1) % TB6_NSHAPE : v3_clamp(v3_k(7) * TB6_NSHAPE / 1024, 0, TB6_NSHAPE - 1);
  if (shape != tb6_shapeNow || tb6_sx[1] == 0.0f) { tb6_shapeNow = shape; tb6_shape(shape); tb6_isOpen = tb6_open[shape]; tb6_pp = v3_clampf(tb6_pp, 0, TB6_L - 0.01f); }
  float depth = v3_centre(8) ? P.depth : 1.0f + v3_u(8) * 18.0f;
  tb6_D = depth;
  int count = v3_centre(9) ? P.enemies + tb6_wave * 2 : 2 + (int)(v3_u(9) * 40);
  float spin = v3_centre(10) ? (P.spin ? 0.25f : 0.0f) : v3_bi(10) * 1.2f;
  int glow = v3_centre(11) ? P.glow : (int)(v3_u(11) * 2.99f);

  uint8_t hues[12][3] = { {255,255,255},{0},{0},{255,40,40},{60,255,90},{200,80,255},{255,255,200},{80,255,60},{255,160,40},{255,220,40},{255,90,200},{200,210,255} };
  memcpy(hues[1], P.tube, 3); memcpy(hues[2], P.claw, 3);
  if (P.rainbow) { uint8_t c[3]; v3_hsv((int)(globalTime * 200), 255, 255, c); memcpy(hues[1], c, 3); }
  a5_palette(hues, 12, a5_lookK(P.look));

  // the rim on screen (spinning, if asked)
  tb6_spinA += spin * v3_dt * (a5_speedK() > 0 ? 1.0f : 0.0f);
  { float c = cosf(tb6_spinA), s = sinf(tb6_spinA);
    for (int k = 0; k <= TB6_L; k++) { float x = tb6_sx[k], y = tb6_sy[k]; tb6_rx[k] = (x * c - y * s) * tb6_R; tb6_ry[k] = (x * s + y * c) * tb6_R + tb6_OY; } }

  // ── game steps ──
  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (tb6_banner > 0) tb6_banner -= dt;
    if (tb6_over > 0) { tb6_over -= dt; if (tb6_over <= 0) { if (tb6_preset == 11) tb6_look = (tb6_look + 1) % 11; tb6_newGame(); } continue; }
    if (tb6_dead > 0) {
      tb6_dead -= dt;
      if (tb6_grabbed >= 0) tb6_ez[tb6_grabbed] += dt * 0.5f;      // dragged down the tube
      if (tb6_dead <= 0) {
        if (--tb6_lives < 0) { tb6_over = 4; if (tb6_score > tb6_hi) tb6_hi = tb6_score; }
        else { for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) { tb6_es[i] = 1; tb6_ez[i] = 0.8f + v3_rndf() * 0.2f; } for (int i = 0; i < TB6_NB; i++) tb6_bz[i] = -1; tb6_grabbed = -1; }
      }
      continue;
    }
    // warping down to the next wave
    if (tb6_warp > 0) {
      tb6_warp += dt; tb6_cam = tb6_warp * tb6_warp * 0.9f;
      int pl = tb6_playerLane();
      if (tb6_spike[pl] < tb6_cam + 0.02f && tb6_spike[pl] < 1.0f) { tb6_die(); tb6_warp = 0; tb6_cam = 0; tb6_startWave(); continue; }
      if (tb6_warp > 1.0f) { tb6_wave++; if (tb6_preset == 11) tb6_look = (tb6_look + 1) % 11; tb6_startWave(); tb6_score += 500; }
      // you can still steer and shoot spikes while warping
    }
    // controls
    bool fire; float target;
    if (a5_player) {
      float k = v3_u(0) * (tb6_isOpen ? (TB6_L - 0.01f) : TB6_L);
      target = k; fire = a5_fireHeld;
    } else {
      tb6_aiT -= dt;
      if (tb6_aiT <= 0) { tb6_aiTarget = tb6_ai(&fire); tb6_aiT = 0.25f + v3_rndf() * 0.35f; }   // a human-ish reaction time
      target = tb6_aiTarget; fire = true;
    }
    float d = target - tb6_pp;
    if (!tb6_isOpen) { while (d > TB6_L / 2) d -= TB6_L; while (d < -TB6_L / 2) d += TB6_L; }
    float mx = a5_player ? 0.22f : 0.13f, mv = v3_clampf(d, -mx, mx);
    tb6_pp += mv;
    if (tb6_isOpen) tb6_pp = v3_clampf(tb6_pp, 0, TB6_L - 0.01f); else { if (tb6_pp < 0) tb6_pp += TB6_L; if (tb6_pp >= TB6_L) tb6_pp -= TB6_L; }
    int pl = tb6_playerLane();
    // superzapper: p1 flicked hard to an end
    float p1 = v3_sm[1];
    if (tb6_zaps > 0 && ((p1 < 40 && tb6_p1was >= 40) || (p1 > 983 && tb6_p1was <= 983)) && a5_player) {
      tb6_zaps--; for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) tb6_killEnemy(i, true);
      a5_shake(5);
    }
    tb6_p1was = p1;
    // shooting
    tb6_fireT -= dt;
    if (fire && tb6_fireT <= 0) {
      for (int i = 0; i < TB6_NS; i++) if (tb6_shz[i] < 0) { tb6_shz[i] = 0.0f; tb6_shl[i] = pl; tb6_fireT = 0.09f; break; }
    }
    for (int i = 0; i < TB6_NS; i++) if (tb6_shz[i] >= 0) {
      tb6_shz[i] += dt * 2.4f;
      int l = tb6_shl[i];
      if (tb6_shz[i] >= tb6_spike[l] - 0.02f && tb6_spike[l] < 1.0f) { tb6_spike[l] += 0.04f; tb6_score += 3; tb6_shz[i] = -1; continue; }
      if (tb6_shz[i] > 1.0f) { tb6_shz[i] = -1; continue; }
      for (int e = 0; e < TB6_NE; e++) if (tb6_es[e] && tb6_el[e] == l && fabsf(tb6_ez[e] - tb6_shz[i]) < 0.06f + tb6_ez[e] * 0.04f) { tb6_killEnemy(e, true); tb6_shz[i] = -1; break; }
      if (tb6_shz[i] >= 0) for (int b = 0; b < TB6_NB; b++) if (tb6_bz[b] >= 0 && tb6_bl[b] == l && fabsf(tb6_bz[b] - tb6_shz[i]) < 0.05f) { tb6_bz[b] = -1; tb6_shz[i] = -1; tb6_score += 10; break; }
    }
    if (tb6_warp > 0) continue;
    // spawning
    tb6_spawnT -= dt;
    int alive = 0; for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) alive++;
    if (tb6_spawnT <= 0 && tb6_toSpawn < count && alive < 8 + tb6_wave) {
      tb6_spawnT = 0.9f - tb6_diff * 0.5f;
      int r = v3_rnd(100), type = r < 62 ? TB6_HINGE : (r < 62 + 18 + P.spikes * 6 ? TB6_SPIRAL : TB6_DIAMOND);
      tb6_spawn(type, v3_rnd(TB6_L), 1.0f); tb6_toSpawn++;
    }
    if (tb6_toSpawn >= count && alive == 0) { tb6_warp = 0.001f; continue; }
    // enemies
    for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) {
      int s = tb6_es[i];
      if (s == 1) {
        tb6_ez[i] -= tb6_ev[i] * dt;
        if (tb6_et[i] == TB6_SPIRAL) {
          if (tb6_ez[i] < tb6_spike[tb6_el[i]]) tb6_spike[tb6_el[i]] = tb6_ez[i];
          if (tb6_ez[i] < 0.35f + v3_rndf() * 0.2f) tb6_es[i] = 3;
        }
        if (tb6_et[i] == TB6_HINGE) {                              // flip to a neighbouring lane now and then
          if (tb6_eft[i] > 0) { tb6_ef[i] += dt * 4.0f; if (tb6_ef[i] >= 1) { tb6_ef[i] = 0; tb6_eft[i] = 0; tb6_el[i] += tb6_edir[i]; if (tb6_isOpen) { if (tb6_el[i] < 0 || tb6_el[i] > TB6_L - 1) { tb6_edir[i] = -tb6_edir[i]; tb6_el[i] = v3_clamp(tb6_el[i], 0, TB6_L - 1); } } else tb6_el[i] = (tb6_el[i] + TB6_L) % TB6_L; } }
          else if (v3_rnd(1000) < 12 + (int)(tb6_diff * 20)) { tb6_eft[i] = 1; int dd = tb6_laneDist(tb6_el[i], pl); if (dd) tb6_edir[i] = dd > 0 ? 1 : -1; }
        }
        if (tb6_et[i] != TB6_SPIRAL && v3_rnd(1000) < 3 + (int)(tb6_diff * 8) + tb6_wave / 2 && tb6_ez[i] > 0.3f) {
          for (int b = 0; b < TB6_NB; b++) if (tb6_bz[b] < 0) { tb6_bz[b] = tb6_ez[i]; tb6_bl[b] = tb6_el[i]; break; }
        }
        if (tb6_ez[i] <= 0) { tb6_ez[i] = 0; tb6_es[i] = tb6_et[i] == TB6_SPIRAL ? 3 : 2; tb6_edir[i] = tb6_laneDist(tb6_el[i], pl) >= 0 ? 1 : -1; tb6_eft[i] = 0; tb6_ef[i] = 0; }
      } else if (s == 2) {                                          // on the rim: creep towards the player
        tb6_ef[i] += dt * (1.2f + tb6_diff + tb6_wave * 0.08f);
        if (tb6_ef[i] >= 1) {
          tb6_ef[i] = 0; int dd = tb6_laneDist(tb6_el[i], pl); tb6_edir[i] = dd > 0 ? 1 : (dd < 0 ? -1 : tb6_edir[i]);
          if (dd != 0) { tb6_el[i] += tb6_edir[i]; if (!tb6_isOpen) tb6_el[i] = (tb6_el[i] + TB6_L) % TB6_L; else tb6_el[i] = v3_clamp(tb6_el[i], 0, TB6_L - 1); }
        }
        if (tb6_el[i] == pl && tb6_ef[i] > 0.6f) { tb6_grabbed = i; tb6_die(); }
      } else {                                                       // spirals go back down
        tb6_ez[i] += tb6_ev[i] * dt;
        if (tb6_ez[i] > 1.0f) tb6_es[i] = 0;
      }
    }
    for (int b = 0; b < TB6_NB; b++) if (tb6_bz[b] >= 0) {
      tb6_bz[b] -= dt * 0.5f;
      if (tb6_bz[b] <= 0.02f) { if (tb6_bl[b] == pl) tb6_die(); tb6_bz[b] = -1; }
    }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  a5_stars(50, 0, 0, 11, 9);
  bool halo = glow >= 1;
  int tubeLev = P.invisible ? 0 : 9, pl = tb6_playerLane();
  float zFar = 1.0f;
  // lane lines
  int nk = tb6_isOpen ? TB6_L + 1 : TB6_L;
  for (int k = 0; k < nk; k++) {
    float x0, y0, x1, y1; tb6_pt(k, tb6_cam > 0 ? tb6_cam + 0.02f : 0.0f, &x0, &y0); tb6_pt(k, zFar, &x1, &y1);
    bool mine = (k == pl || k == pl + 1 || (k == 0 && pl == TB6_L - 1 && !tb6_isOpen));
    int lev = mine ? 15 : tubeLev;
    if (P.rainbow) { int h = 1 + (k % 10); (void)h; }
    if (lev) a5_glow((int)(x0 * 8), (int)(y0 * 8), (int)(x1 * 8), (int)(y1 * 8), lev, mine ? 2 : 1, halo && glow == 2);
  }
  // rims
  for (int k = 0; k < TB6_L; k++) {
    if (tb6_isOpen && k >= TB6_L) break;
    float ax, ay, bx, by, cx, cy, dx, dy;
    tb6_pt(k, tb6_cam > 0 ? tb6_cam + 0.02f : 0.0f, &ax, &ay); tb6_pt(k + 1, tb6_cam > 0 ? tb6_cam + 0.02f : 0.0f, &bx, &by);
    tb6_pt(k, zFar, &cx, &cy); tb6_pt(k + 1, zFar, &dx, &dy);
    int lev = k == pl ? 15 : 13;
    a5_glow((int)(ax * 8), (int)(ay * 8), (int)(bx * 8), (int)(by * 8), lev, k == pl ? 2 : 1, halo);
    a5_glow((int)(cx * 8), (int)(cy * 8), (int)(dx * 8), (int)(dy * 8), 11, 1, false);
  }
  // spikes
  for (int l = 0; l < TB6_L; l++) if (tb6_spike[l] < 1.0f) {
    float x0, y0, x1, y1; tb6_lanePt(l + 0.5f, tb6_spike[l], &x0, &y0); tb6_lanePt(l + 0.5f, 1.0f, &x1, &y1);
    v3_lineAA((int)(x0 * 8), (int)(y0 * 8), (int)(x1 * 8), (int)(y1 * 8), 14, 7);
    v3_disc((int)x0, (int)y0, 1, A5C(15, 0));
  }
  // enemies
  for (int i = 0; i < TB6_NE; i++) if (tb6_es[i]) {
    float z = tb6_ez[i], l = tb6_el[i] + 0.5f;
    if (tb6_es[i] == 2 || (tb6_et[i] == TB6_HINGE && tb6_eft[i])) l += tb6_edir[i] * tb6_ef[i];
    float cx, cy; tb6_lanePt(l, z, &cx, &cy);
    float ax, ay, bx, by; tb6_lanePt(l - 0.45f, z, &ax, &ay); tb6_lanePt(l + 0.45f, z, &bx, &by);
    float s = tb6_s(z), hgt = 7.0f * s * 3.0f;
    // "up" at this spot: towards the vanishing point
    float ux = tb6_VX - cx, uy = tb6_VY - cy, ul = sqrtf(ux * ux + uy * uy) + 0.01f; ux /= ul; uy /= ul;
    int hue = tb6_et[i] == TB6_HINGE ? 3 : (tb6_et[i] == TB6_SPIRAL ? 4 : 5);
    if (tb6_et[i] == TB6_HINGE) {                                   // a bow-tie across the lane
      float tx = cx + ux * hgt, ty = cy + uy * hgt;
      v3_lineAA((int)(ax * 8), (int)(ay * 8), (int)(bx * 8 + ux * hgt * 8), (int)(by * 8 + uy * hgt * 8), 15, hue);
      v3_lineAA((int)(bx * 8), (int)(by * 8), (int)(ax * 8 + ux * hgt * 8), (int)(ay * 8 + uy * hgt * 8), 15, hue);
      v3_lineAA((int)(ax * 8), (int)(ay * 8), (int)(ax * 8 + ux * hgt * 8), (int)(ay * 8 + uy * hgt * 8), 12, hue);
      v3_lineAA((int)(bx * 8), (int)(by * 8), (int)(bx * 8 + ux * hgt * 8), (int)(by * 8 + uy * hgt * 8), 12, hue);
      (void)tx; (void)ty;
    } else if (tb6_et[i] == TB6_SPIRAL) {                            // a spinning spiral
      float r = 9.0f * s * 2.0f; int a0 = (int)(globalTime * 900);
      int px = (int)(cx * 8), py = (int)(cy * 8);
      for (int q = 1; q < 14; q++) {
        int a = a0 + q * 110; float rr = r * q / 14;
        int nx = (int)((cx + (v3_cos(a) * rr) / 16384) * 8), ny = (int)((cy + (v3_sin(a) * rr) / 16384) * 8);
        v3_lineAA(px, py, nx, ny, 15, hue); px = nx; py = ny;
      }
    } else {                                                          // a diamond with a cross inside
      float tx = cx + ux * hgt, ty = cy + uy * hgt;
      int X[4] = { (int)(ax * 8), (int)(cx * 8 + ux * hgt * 4), (int)(bx * 8), (int)(cx * 8 - ux * hgt * 4) };
      int Y[4] = { (int)(ay * 8), (int)(cy * 8 + uy * hgt * 4), (int)(by * 8), (int)(cy * 8 - uy * hgt * 4) };
      for (int q = 0; q < 4; q++) v3_lineAA(X[q], Y[q], X[(q + 1) & 3], Y[(q + 1) & 3], 15, hue);
      v3_lineAA(X[0], Y[0], X[2], Y[2], 10, hue); v3_lineAA(X[1], Y[1], X[3], Y[3], 10, hue);
      (void)tx; (void)ty;
    }
  }
  // enemy bullets and your shots
  for (int b = 0; b < TB6_NB; b++) if (tb6_bz[b] >= 0) { float x, y; tb6_lanePt(tb6_bl[b] + 0.5f, tb6_bz[b], &x, &y); int r = 1 + (int)(tb6_s(tb6_bz[b]) * 3); a5_circle((int)x, (int)y, r, A5C(15, 10)); }
  for (int i = 0; i < TB6_NS; i++) if (tb6_shz[i] >= 0) {
    float x, y, x2, y2; tb6_lanePt(tb6_shl[i] + 0.5f, tb6_shz[i], &x, &y); tb6_lanePt(tb6_shl[i] + 0.5f, tb6_shz[i] + 0.05f, &x2, &y2);
    v3_lineAA((int)(x * 8), (int)(y * 8), (int)(x2 * 8), (int)(y2 * 8), 15, 6);
    v3_disc((int)x, (int)y, 1, A5C(15, 6));
  }
  // the claw
  if (tb6_dead <= 0 || ((int)(tb6_dead * 10) & 1)) {
    float zc = tb6_grabbed >= 0 && tb6_dead > 0 ? tb6_ez[tb6_grabbed] : (tb6_cam > 0 ? tb6_cam + 0.03f : 0.0f);
    float l = tb6_pp;
    float ax, ay, bx, by, cx, cy; tb6_lanePt(floorf(l) + 0.02f, zc, &ax, &ay); tb6_lanePt(floorf(l) + 0.98f, zc, &bx, &by); tb6_lanePt(l, zc, &cx, &cy);
    float ux = tb6_VX - cx, uy = tb6_VY - cy, ul = sqrtf(ux * ux + uy * uy) + 0.01f; ux /= ul; uy /= ul;
    float sz = 10.0f * tb6_s(zc);
    int X[6] = { (int)(ax * 8), (int)((cx - ux * sz * 0.8f) * 8), (int)(bx * 8), (int)((bx + ux * sz) * 8), (int)(cx * 8), (int)((ax + ux * sz) * 8) };
    int Y[6] = { (int)(ay * 8), (int)((cy - uy * sz * 0.8f) * 8), (int)(by * 8), (int)((by + uy * sz) * 8), (int)(cy * 8), (int)((ay + uy * sz) * 8) };
    for (int q = 0; q < 6; q++) a5_glow(X[q], Y[q], X[(q + 1) % 6], Y[(q + 1) % 6], 15, 2, halo);
  }
  a5_partDraw();

  float fx, fy; tb6_lanePt(tb6_pp, 0, &fx, &fy);
  a5_post((int)((fx + tb6_VX) / 2), (int)((fy + tb6_VY) / 2));
  a5_hud(tb6_score, tb6_hi, A5C(15, 9), A5C(15, 0));
  char s[20];
  for (int i = 0; i < v3_clamp(tb6_lives, 0, 6); i++) { int x = 6 + i * 12, y = 16; a5_seg(x, y + 6, x + 4, y, A5C(15, 2)); a5_seg(x + 4, y, x + 8, y + 6, A5C(15, 2)); a5_seg(x, y + 6, x + 8, y + 6, A5C(15, 2)); }
  snprintf(s, 20, "WAVE %d", tb6_wave); a5_textS(W - 54, 2, s, 1, A5C(15, 9));
  if (tb6_zaps > 0 && a5_player) a5_textS(W - 54, 12, "ZAP", 1, A5C(12, 4));
  if (tb6_banner > 0) { snprintf(s, 20, "WAVE %d", tb6_wave); a5_banner(H / 2 + 50, s, 2, A5C(15, 9)); }
  if (tb6_over > 0) a5_banner(H / 2 - 8, "GAME OVER", 2, A5C(15, 3));
  a5_crt();
}
void prog_tube_draw(int preset) { tube5_draw(preset); fx5_apply(); }

// =====================================================================
// PROGRAM: BRICKS — knock down the wall (by Dewey) — Set 5 "Arcade", slot 6
//
// In the spirit of the 1976 paddle-and-wall cabinet, with some later
// tricks: a ball bounces off your paddle into a wall of bricks that
// shatter into sparks. The ball speeds up as it climbs into the higher
// rows. Some bricks drop capsules — M for multiball, W for a wide paddle,
// L for lasers, S to slow the ball. Walls come in shapes: stripes,
// pyramids, space invaders, diamonds, armoured fortresses, and a wall that
// creeps down towards you.
//
// It plays itself; touch p0 or k12 and the paddle is yours (the knob's
// position is the paddle's position).
//
// Presets:
//   k0 Classic   k1 Pyramid   k2 Invaders   k3 Rainbow   k4 Multiball Mania
//   k5 Descent (the wall creeps down)   k6 Fortress (armoured bricks)
//   k7 Neon Outline   k8 Phosphor   k9 Candy   k10 Diamond
//   k11 Tour — every wall a new preset
//   k12 — SERVE / FIRE LASERS. Joins the game.
//
// Knobs:
//   p0 Paddle (its position; touch to play)   p1 Ball Spin (◄ curve left | curve right ►)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Wall Pattern (centre = preset; else 8 patterns)   p8 Paddle Width
//   p9 Balls in Play (centre = preset)   p10 Capsules (none … lots)
//   p11 Brick Style (centre = preset: bevelled, flat, outline, rounded)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1-8 brick rows, 9 paddle, 10 ball, 11 capsule, 12 HUD, 13 armour, 14 laser, 15 background
struct Br6Pre { uint8_t look, pattern, balls, caps, style, descent, armour, trails; uint8_t bg[3]; };
static const Br6Pre br6_pre[11] = {
  { A5_ARCADE, 0, 1, 30, 0, 0, 0, 60, {0,0,0} },     { A5_ARCADE, 1, 1, 40, 0, 0, 0, 60, {10,10,40} },
  { A5_ARCADE, 2, 1, 40, 1, 0, 0, 60, {0,0,0} },     { A5_ARCADE, 3, 1, 40, 3, 0, 0, 90, {20,0,30} },
  { A5_ARCADE, 0, 4, 60, 0, 0, 0, 120, {0,0,0} },    { A5_ARCADE, 0, 1, 40, 0, 1, 0, 60, {30,10,10} },
  { A5_ARCADE, 4, 2, 40, 0, 0, 1, 60, {10,20,20} },  { A5_NEON, 5, 2, 40, 2, 0, 0, 150, {0,0,0} },
  { A5_PHOSPHOR, 0, 1, 30, 2, 0, 0, 170, {0,0,0} },  { A5_PASTEL, 6, 2, 50, 3, 0, 0, 60, {0,0,0} },
  { A5_ARCADE, 7, 1, 40, 3, 0, 1, 80, {0,10,30} },
};
static const char* const br6_names[12] = { "Classic", "Pyramid", "Invaders", "Rainbow", "Multiball Mania", "Descent", "Fortress", "Neon Outline", "Phosphor", "Candy", "Diamond", "Tour" };
static const char* const br6_lbl[16] = { "Paddle", "Ball Spin", "", "", "", "", "", "Wall Pattern", "Paddle Width", "Balls in Play", "Capsules", "Brick Style", "", "", "", "" };
const char* prog_bricks_name() { return "BRICKS"; }
const char* prog_bricks_character() { return "Paddle, ball and a wall of shattering bricks - plays itself until you touch p0/k12"; }
const char* prog_bricks_presetName(int preset) { return (preset >= 0 && preset < 12) ? br6_names[preset] : NULL; }
const char* prog_bricks_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, br6_lbl); }
uint8_t prog_bricks_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static const uint8_t br6_rowCols[3][8][3] = {
  {{255,40,40},{255,40,40},{255,140,0},{255,140,0},{40,200,40},{40,200,40},{255,230,0},{255,230,0}},
  {{255,0,0},{255,128,0},{255,255,0},{0,255,0},{0,200,255},{0,80,255},{140,0,255},{255,0,200}},
  {{255,150,180},{255,200,150},{255,250,170},{170,255,190},{170,230,255},{190,180,255},{240,170,255},{255,180,220}},
};

#define BR6_C 16
#define BR6_R 12
#define BR6_BW 20
#define BR6_BH 8
#define BR6_TOP 30
#define BR6_PY 226
#define BR6_NB 8
#define BR6_NC 4
static uint8_t br6_b[BR6_R][BR6_C];            // brick hits left (0 = gone), bit 0x40 = armoured, 0x80 = capsule inside
static uint8_t br6_flash[BR6_R][BR6_C];
static float br6_x[BR6_NB], br6_y[BR6_NB], br6_vx[BR6_NB], br6_vy[BR6_NB]; static bool br6_on[BR6_NB];
static float br6_cx[BR6_NC], br6_cy[BR6_NC]; static int br6_ck[BR6_NC];   // capsules: kind 1 M 2 W 3 L 4 S, 0 none
static float br6_lx[4], br6_ly[4];                                       // laser bolts (y < 0 = none)
static float br6_px = 160, br6_pw = 40, br6_wideT = 0, br6_laserT = 0, br6_slowT = 0, br6_serveT = 1, br6_banner = 0, br6_over = 0, br6_dropT = 0, br6_rowOff = 0;
static int br6_wall = 1, br6_lives = 3, br6_preset = -1, br6_look = 0, br6_pattern = 0, br6_hits = 0, br6_style = 0;
static bool br6_held = true;
static uint32_t br6_score = 0, br6_hi = 5000;
static float br6_since = 0;
static float br6_diff = 0.4f, br6_aiOff = 0, br6_aiT = 0, br6_spd = 2.6f;

static void br6_build(int pat, bool armour, int capPct) {
  memset(br6_b, 0, sizeof(br6_b)); memset(br6_flash, 0, sizeof(br6_flash));
  static const char inv[8][17] = { "....1.....1.....", ".....1...1......", "....1111111.....", "...11.111.11....", "..11111111111...", "..1.1111111.1...", "..1.1.....1.1...", ".....11.11......" };
  for (int r = 0; r < 8; r++) for (int c = 0; c < BR6_C; c++) {
    bool on = false;
    switch (pat) {
      case 0: on = true; break;                                             // stripes
      case 1: on = c >= a5_max(0, 7 - r) && c <= a5_min(15, 8 + r); break;  // pyramid
      case 2: on = inv[r][c] == '1'; break;                                 // invader
      case 3: on = ((r + c) % 3) != 0; break;                               // rainbow lattice
      case 4: on = true; break;                                             // fortress
      case 5: on = (r == 0 || r == 7 || c == 0 || c == 15 || (r == 3 && c > 3 && c < 12)); break;   // outline boxes
      case 6: on = ((r & 1) == 0) || (c & 1); break;                        // candy checks
      default: on = v3_abs(c * 2 + 1 - 16) / 2 + v3_abs(r * 2 + 1 - 8) / 2 <= 5; break;   // diamond
    }
    if (!on) continue;
    uint8_t v = 1;
    if (armour && (r == 0 || (pat == 4 && (c == 0 || c == 15 || r == 7)))) v = 0x40 | 3;
    if (v3_rnd(100) < capPct / 3) v |= 0x80;
    br6_b[r + 1][c] = v;
  }
  br6_rowOff = 0; br6_dropT = 0; br6_hits = 0;
}
static int br6_left() { int n = 0; for (int r = 0; r < BR6_R; r++) for (int c = 0; c < BR6_C; c++) if (br6_b[r][c] & 0x3F) n++; return n; }
static void br6_serve() {
  memset(br6_on, 0, sizeof(br6_on));
  br6_on[0] = true; br6_x[0] = br6_px; br6_y[0] = BR6_PY - 4; br6_vx[0] = 0; br6_vy[0] = 0; br6_held = true; br6_serveT = 1.0f;
  br6_spd = 2.4f + br6_diff * 0.8f; br6_hits = 0;
}
static void br6_launch(int i, float ang) { br6_vx[i] = sinf(ang) * br6_spd; br6_vy[i] = -cosf(ang) * br6_spd; }
static void br6_startWall(int capPct) {
  const Br6Pre& P = br6_pre[br6_look];
  br6_build(br6_pattern, P.armour, capPct);
  br6_serve(); br6_banner = 1.5f;
  memset(br6_ck, 0, sizeof(br6_ck)); for (int k = 0; k < 4; k++) br6_ly[k] = -1;
}
static void br6_newGame(int capPct) { br6_score = 0; br6_lives = 3; br6_wall = 1; br6_over = 0; br6_wideT = br6_laserT = br6_slowT = 0; br6_startWall(capPct); }
static void br6_hitBrick(int r, int c) {
  uint8_t& v = br6_b[r][c];
  int h = (v & 0x3F) - 1;
  br6_flash[r][c] = 6; br6_since = 0;
  int x = c * BR6_BW + BR6_BW / 2, y = (int)(BR6_TOP + br6_rowOff) + r * BR6_BH + BR6_BH / 2;
  if (h > 0) { v = (uint8_t)((v & 0xC0) | h); return; }
  bool cap = v & 0x80; v = 0;
  int row = (r - 1) & 7;
  br6_score += 10 + (7 - row) * 5;
  a5_burst(x, y, 12, 1 + row, 30, 24);
  br6_hits++;
  if (cap) for (int k = 0; k < BR6_NC; k++) if (!br6_ck[k]) { br6_ck[k] = 1 + v3_rnd(4); br6_cx[k] = (float)x; br6_cy[k] = (float)y; break; }
  // classic speed-ups: after 4 and 12 hits, and on reaching the top rows
  if (br6_hits == 4 || br6_hits == 12 || row <= 1) br6_spd = v3_clampf(br6_spd + (row <= 1 ? 0.15f : 0.4f), 2.0f, 5.5f);
}
// where will ball i cross the paddle line? (bouncing off the side walls, ignoring bricks)
static float br6_landX(int i) {
  if (br6_vy[i] <= 0.01f) return br6_x[i];
  float t = (BR6_PY - br6_y[i]) / br6_vy[i], x = br6_x[i] + br6_vx[i] * t;
  float span = W - 6;
  x -= 3; x = fmodf(x, 2 * span); if (x < 0) x += 2 * span; if (x > span) x = 2 * span - x;
  return x + 3;
}

void prog_bricks_init() { v3_reset(); a5_reset(); br6_preset = -1; a5_partClear(); }

static void bricks5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  const Br6Pre& P0 = br6_pre[preset == 11 ? 0 : preset];
  int caps = v3_centre(10) ? P0.caps : (int)(v3_u(10) * 100);
  if (preset != br6_preset) { br6_preset = preset; br6_look = preset == 11 ? 0 : preset; br6_pattern = br6_pre[br6_look].pattern; br6_newGame(caps); }
  const Br6Pre& P = br6_pre[br6_look];
  br6_diff = a5_diffK(0.35f);
  int pat = v3_centre(7) ? P.pattern : v3_clamp(v3_k(7) * 8 / 1024, 0, 7);
  if (pat != br6_pattern) { br6_pattern = pat; br6_build(pat, P.armour, caps); }
  float baseW = v3_centre(8) ? 40.0f : 16.0f + v3_u(8) * 90.0f;
  int nBalls = v3_centre(9) ? P.balls : 1 + (int)(v3_u(9) * 5.99f);
  br6_style = v3_pick(11, P.style, 4);
  float spin = v3_centre(1) ? 0.0f : v3_bi(1) * 0.006f;

  int set = (br6_look == 3 || br6_look == 7) ? 1 : (br6_look == 9 ? 2 : 0);
  uint8_t hues[16][3];
  hues[0][0] = hues[0][1] = hues[0][2] = 255;
  for (int k = 0; k < 8; k++) memcpy(hues[1 + k], br6_rowCols[set][k], 3);
  const uint8_t extra[7][3] = { {80,160,255},{255,255,255},{255,255,255},{255,230,60},{170,170,190},{255,60,60},{0,0,0} };
  for (int k = 0; k < 7; k++) memcpy(hues[9 + k], extra[k], 3);
  memcpy(hues[15], P.bg, 3);
  a5_palette(hues, 16, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    for (int r = 0; r < BR6_R; r++) for (int c = 0; c < BR6_C; c++) if (br6_flash[r][c]) br6_flash[r][c]--;
    if (br6_banner > 0) br6_banner -= dt;
    if (br6_over > 0) { br6_over -= dt; if (br6_over <= 0) { if (br6_preset == 11) br6_look = (br6_look + 1) % 11; br6_pattern = br6_pre[br6_look].pattern; br6_newGame(caps); } continue; }
    br6_since += dt;
    if (br6_wideT > 0) br6_wideT -= dt; if (br6_laserT > 0) br6_laserT -= dt; if (br6_slowT > 0) br6_slowT -= dt;
    br6_pw += ((br6_wideT > 0 ? baseW * 1.8f : baseW) - br6_pw) * 0.1f;
    // paddle
    float tx;
    if (a5_player) tx = br6_pw / 2 + v3_u(0) * (W - br6_pw);
    else {
      // follow the lowest ball that's coming down; aim the bounce at the bricks
      int bi = -1; float by = -1;
      for (int i = 0; i < BR6_NB; i++) if (br6_on[i] && (br6_vy[i] > 0 || br6_held) && br6_y[i] > by) { by = br6_y[i]; bi = i; }
      tx = br6_px;
      br6_aiT -= dt;
      if (bi >= 0) {
        float lx = br6_held ? br6_px : br6_landX(bi);
        if (br6_aiT <= 0) {
          float sx = 0, n = 0; for (int r = 0; r < BR6_R; r++) for (int c = 0; c < BR6_C; c++) if (br6_b[r][c] & 0x3F) { sx += c * BR6_BW + 10; n++; }
          float aim = n ? sx / n : 160;
          br6_aiOff = v3_clampf((lx - aim) / 160.0f, -0.7f, 0.7f) * br6_pw * 0.4f + (v3_rndf() - 0.5f) * br6_pw * (0.3f + (1 - br6_diff) * 0.25f);
          br6_aiT = 0.4f;
        }
        tx = lx + br6_aiOff;
      }
      for (int k = 0; k < BR6_NC; k++) if (br6_ck[k] && br6_cy[k] > 170 && (bi < 0 || br6_y[bi] < 120)) tx = br6_cx[k];   // grab a capsule when it's safe
    }
    float mv = a5_player ? 12.0f : 4.5f + br6_diff * 2;
    br6_px += v3_clampf(tx - br6_px, -mv, mv);
    br6_px = v3_clampf(br6_px, br6_pw / 2, W - br6_pw / 2);
    // serve
    if (br6_held) {
      br6_x[0] = br6_px; br6_y[0] = BR6_PY - 4; br6_serveT -= dt;
      if ((a5_player && a5_fire) || (!a5_player && br6_serveT <= 0) || (a5_player && br6_serveT < -3)) {
        br6_held = false; br6_launch(0, (v3_rndf() - 0.5f) * 0.8f);
        for (int k = 1; k < nBalls && k < BR6_NB; k++) { br6_on[k] = true; br6_x[k] = br6_px; br6_y[k] = BR6_PY - 4; br6_launch(k, (v3_rndf() - 0.5f) * 1.6f); }
      }
    }
    // lasers
    if (br6_laserT > 0 && ((a5_player && a5_fire) || (!a5_player && v3_rnd(12) == 0))) {
      for (int k = 0; k < 4; k += 2) if (br6_ly[k] < 0) { br6_lx[k] = br6_px - br6_pw / 2 + 3; br6_ly[k] = BR6_PY - 2; br6_lx[k + 1] = br6_px + br6_pw / 2 - 3; br6_ly[k + 1] = BR6_PY - 2; break; }
    }
    for (int k = 0; k < 4; k++) if (br6_ly[k] >= 0) {
      br6_ly[k] -= 6;
      int c = (int)br6_lx[k] / BR6_BW, r = (int)((br6_ly[k] - BR6_TOP - br6_rowOff) / BR6_BH);
      if (r >= 0 && r < BR6_R && c >= 0 && c < BR6_C && (br6_b[r][c] & 0x3F)) { br6_hitBrick(r, c); br6_ly[k] = -1; }
      else if (br6_ly[k] < 12) br6_ly[k] = -1;
    }
    // balls
    int alive = 0;
    float spdK = br6_slowT > 0 ? 0.6f : 1.0f;
    for (int i = 0; i < BR6_NB; i++) if (br6_on[i] && !(br6_held && i == 0)) {
      float sp = sqrtf(br6_vx[i] * br6_vx[i] + br6_vy[i] * br6_vy[i]) + 0.0001f;
      br6_vx[i] *= br6_spd * spdK / sp; br6_vy[i] *= br6_spd * spdK / sp;
      // spin: curve the path (p1)
      if (spin != 0.0f) { float a = spin; float vx = br6_vx[i] * cosf(a) - br6_vy[i] * sinf(a), vy = br6_vx[i] * sinf(a) + br6_vy[i] * cosf(a); br6_vx[i] = vx; br6_vy[i] = vy; }
      if (fabsf(br6_vy[i]) < 0.6f) br6_vy[i] = br6_vy[i] < 0 ? -0.6f : 0.6f;
      int sub = 1 + (int)(sp / 2.0f);
      for (int s = 0; s < sub; s++) {
        float nx = br6_x[i] + br6_vx[i] / sub;
        if (nx < 3) { nx = 3; br6_vx[i] = fabsf(br6_vx[i]); } if (nx > W - 3) { nx = W - 3; br6_vx[i] = -fabsf(br6_vx[i]); }
        int c = (int)nx / BR6_BW, r = (int)((br6_y[i] - BR6_TOP - br6_rowOff) / BR6_BH);
        if (br6_y[i] - BR6_TOP - br6_rowOff >= 0 && r < BR6_R && c >= 0 && c < BR6_C && (br6_b[r][c] & 0x3F)) { br6_hitBrick(r, c); br6_vx[i] = -br6_vx[i]; nx = br6_x[i]; }
        br6_x[i] = nx;
        float ny = br6_y[i] + br6_vy[i] / sub;
        if (ny < 13) { ny = 13; br6_vy[i] = fabsf(br6_vy[i]); if (br6_wideT <= 0 && br6_pw > 24 && a5_player) br6_pw *= 0.995f; }
        c = (int)br6_x[i] / BR6_BW; r = (int)((ny - BR6_TOP - br6_rowOff) / BR6_BH);
        if (ny - BR6_TOP - br6_rowOff >= 0 && r < BR6_R && c >= 0 && c < BR6_C && (br6_b[r][c] & 0x3F)) { br6_hitBrick(r, c); br6_vy[i] = -br6_vy[i]; ny = br6_y[i]; }
        // the paddle: where you hit it sets the angle
        if (br6_vy[i] > 0 && br6_y[i] <= BR6_PY - 3 && ny > BR6_PY - 3 && fabsf(br6_x[i] - br6_px) <= br6_pw / 2 + 3) {
          float off = v3_clampf((br6_x[i] - br6_px) / (br6_pw / 2 + 3), -1, 1);
          br6_launch(i, v3_clampf(off * 1.05f + (v3_rndf() - 0.5f) * (br6_since > 8 ? 0.9f : 0.12f), -1.15f, 1.15f)); ny = BR6_PY - 3;
          a5_burst((int)br6_x[i], BR6_PY - 2, 4, 9, 20, 10);
        }
        br6_y[i] = ny;
      }
      if (br6_y[i] > H + 4) br6_on[i] = false; else alive++;
    }
    if (br6_held) alive++;
    if (!alive) {
      if (--br6_lives < 0) { br6_over = 4; if (br6_score > br6_hi) br6_hi = br6_score; continue; }
      br6_serve(); a5_shake(3);
    }
    // capsules fall
    for (int k = 0; k < BR6_NC; k++) if (br6_ck[k]) {
      br6_cy[k] += 1.2f;
      if (br6_cy[k] > BR6_PY - 4 && br6_cy[k] < BR6_PY + 4 && fabsf(br6_cx[k] - br6_px) < br6_pw / 2 + 6) {
        int kind = br6_ck[k]; br6_ck[k] = 0; br6_score += 100;
        if (kind == 1) { int src = -1; for (int i = 0; i < BR6_NB; i++) if (br6_on[i]) { src = i; break; }
          if (src >= 0) for (int n = 0, i = 0; i < BR6_NB && n < 2; i++) if (!br6_on[i]) { br6_on[i] = true; br6_x[i] = br6_x[src]; br6_y[i] = br6_y[src]; br6_launch(i, (n ? 0.6f : -0.6f)); n++; } }
        if (kind == 2) br6_wideT = 12; if (kind == 3) br6_laserT = 10; if (kind == 4) br6_slowT = 8;
        a5_burst((int)br6_cx[k], BR6_PY, 16, 11, 30, 20);
      } else if (br6_cy[k] > H) br6_ck[k] = 0;
    }
    // the creeping wall
    if (P.descent) { br6_dropT += dt; if (br6_dropT > 6.0f - br6_diff * 2) { br6_dropT = 0; br6_rowOff += 4; } }
    // wall cleared (or it reached the paddle)
    int left = br6_left();
    int lowest = 0; for (int r = 0; r < BR6_R; r++) for (int c = 0; c < BR6_C; c++) if (br6_b[r][c] & 0x3F) lowest = r;
    if (BR6_TOP + br6_rowOff + (lowest + 1) * BR6_BH > BR6_PY - 8 && left) { br6_lives = 0; for (int i = 0; i < BR6_NB; i++) br6_on[i] = false; br6_held = false; }
    if (!left) { br6_wall++; br6_score += 500; if (br6_preset == 11) br6_look = (br6_look + 1) % 11; br6_pattern = v3_centre(7) ? (br6_pre[br6_look].pattern + (br6_preset == 11 ? 0 : br6_wall - 1)) % 8 : br6_pattern; br6_startWall(caps); }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (P.bg[0] | P.bg[1] | P.bg[2]) for (int y = 12; y < H; y += 3) for (int x = ((y / 3) & 1) * 8; x < W; x += 16) v3_put(x, y, A5C(10, 15));
  for (int y = 12; y < H; y++) { v3_put(0, y, A5C(9, 13)); v3_put(1, y, A5C(6, 13)); v3_put(W - 1, y, A5C(9, 13)); v3_put(W - 2, y, A5C(6, 13)); }
  for (int x = 0; x < W; x++) v3_put(x, 12, A5C(9, 13));
  // bricks
  for (int r = 0; r < BR6_R; r++) for (int c = 0; c < BR6_C; c++) if (br6_b[r][c] & 0x3F) {
    int x = c * BR6_BW, y = (int)(BR6_TOP + br6_rowOff) + r * BR6_BH, hue = 1 + ((r - 1) & 7);
    bool arm = br6_b[r][c] & 0x40;
    if (arm) hue = 13;
    if (br6_flash[r][c]) { v3_rect(x + 1, y + 1, x + BR6_BW - 1, y + BR6_BH - 1, A5C(15, 0)); continue; }
    switch (br6_style) {
      case 0: v3_rect(x + 1, y + 1, x + BR6_BW - 1, y + BR6_BH - 1, A5C(11, hue)); v3_hspan(x + 1, x + BR6_BW - 1, y + 1, A5C(15, hue)); v3_rect(x + 1, y + 1, x + 1, y + BR6_BH - 1, A5C(14, hue)); v3_hspan(x + 1, x + BR6_BW - 1, y + BR6_BH - 1, A5C(6, hue)); v3_rect(x + BR6_BW - 1, y + 1, x + BR6_BW - 1, y + BR6_BH - 1, A5C(7, hue)); break;
      case 1: v3_rect(x + 1, y + 1, x + BR6_BW - 1, y + BR6_BH - 1, A5C(13, hue)); break;
      case 2: { uint8_t k = A5C(15, hue); v3_hspan(x + 1, x + BR6_BW - 1, y + 1, k); v3_hspan(x + 1, x + BR6_BW - 1, y + BR6_BH - 1, k); v3_rect(x + 1, y + 1, x + 1, y + BR6_BH - 1, k); v3_rect(x + BR6_BW - 1, y + 1, x + BR6_BW - 1, y + BR6_BH - 1, k); break; }
      default: v3_rect(x + 2, y + 1, x + BR6_BW - 2, y + BR6_BH - 1, A5C(12, hue)); v3_hspan(x + 1, x + BR6_BW - 1, y + 3, A5C(12, hue)); v3_hspan(x + 1, x + BR6_BW - 1, y + 5, A5C(12, hue)); v3_hspan(x + 3, x + 9, y + 2, A5C(15, hue)); break;
    }
    if (arm) v3_hspan(x + 4, x + BR6_BW - 4, y + BR6_BH / 2, A5C(15, 0));
  }
  // capsules
  for (int k = 0; k < BR6_NC; k++) if (br6_ck[k]) {
    int x = (int)br6_cx[k], y = (int)br6_cy[k];
    v3_rect(x - 8, y - 3, x + 8, y + 3, A5C(13, 11)); v3_hspan(x - 7, x + 7, y - 3, A5C(15, 11));
    static const char* const lt = "MWLS"; char s[2] = { lt[br6_ck[k] - 1], 0 }; a5_text(x - 2, y - 3, s, 1, A5C(2, 15));
  }
  for (int k = 0; k < 4; k++) if (br6_ly[k] >= 0) v3_rect((int)br6_lx[k], (int)br6_ly[k], (int)br6_lx[k], (int)br6_ly[k] + 5, A5C(15, 14));
  // balls
  for (int i = 0; i < BR6_NB; i++) if (br6_on[i]) { int x = (int)br6_x[i], y = (int)br6_y[i]; v3_disc(x, y, 2, A5C(15, 10)); v3_put(x - 1, y - 1, A5C(15, 0)); }
  // paddle
  { int x0 = (int)(br6_px - br6_pw / 2), x1 = (int)(br6_px + br6_pw / 2);
    v3_rect(x0, BR6_PY, x1, BR6_PY + 4, A5C(12, 9)); v3_hspan(x0, x1, BR6_PY, A5C(15, 9)); v3_hspan(x0, x1, BR6_PY + 4, A5C(6, 9));
    v3_rect(x0, BR6_PY, x0 + 3, BR6_PY + 4, A5C(15, br6_laserT > 0 ? 14 : 0)); v3_rect(x1 - 3, BR6_PY, x1, BR6_PY + 4, A5C(15, br6_laserT > 0 ? 14 : 0)); }
  a5_partDraw();

  int fy = BR6_PY - 40; for (int i = 0; i < BR6_NB; i++) if (br6_on[i] && br6_y[i] > fy) fy = (int)br6_y[i];
  a5_post((int)br6_px, (fy + BR6_PY) / 2 - 30);
  a5_hud(br6_score, br6_hi, A5C(15, 12), A5C(15, 0));
  char s[16]; for (int i = 0; i < v3_clamp(br6_lives, 0, 6); i++) v3_rect(W - 12 - i * 12, 4, W - 4 - i * 12, 6, A5C(14, 9));
  snprintf(s, 16, "WALL %d", br6_wall);
  if (br6_banner > 0) a5_banner(H / 2 + 20, s, 2, A5C(15, 12));
  if (br6_over > 0) a5_banner(H / 2 + 20, "GAME OVER", 2, A5C(15, 1));
  a5_crt();
}
void prog_bricks_draw(int preset) { bricks5_draw(preset); fx5_apply(); }

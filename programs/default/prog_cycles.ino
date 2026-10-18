// =====================================================================
// PROGRAM: CYCLES — light-trail duel on a glowing grid (by Dewey)
// Set 5 "Arcade", slot 11
//
// In the spirit of the late-'70s / early-'80s wall-trail games: riders race
// round an arena leaving walls of light behind them, turning only at right
// angles. Hit a wall — anyone's, even your own — and you're out. Last rider
// moving wins the round. Each computer rider looks ahead and picks the turn
// that leaves it the most room, and tries to cut the others off. Switch to
// the tilted 3-D view and the trails stand up as glowing walls.
//
// It plays itself; touch p0 or k12 and you're the blue rider.
//
// Presets:
//   k0 Classic Duel   k1 Four-Way   k2 Six Pack   k3 Snakes (short trails)
//   k4 Wrap-Around   k5 Tilted 3-D   k6 Neon Sunset (3-D)   k7 Obstacles
//   k8 Phosphor   k9 Ice Grid   k10 Fast Lane   k11 Tour — every round a new preset
//   k12 — TURN RIGHT (or turn p0: the knob's angle points your rider N/E/S/W). Joins the game.
//
// Knobs:
//   p0 Heading (N/E/S/W round the dial; touch to play)   p1 Boost (◄ normal | faster ►)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Riders (centre = preset; 2-6)   p8 Arena (centre = preset: walls, wrap-around, obstacles)
//   p9 Trail Length (centre = preset; short snakes … endless)
//   p10 View (centre = preset: top-down, tilted 3-D)   p11 Grid (none … bright)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1-6 riders, 7 grid, 8 walls, 9 HUD, 10 sky, 11 sun
struct Cy6Pre { uint8_t look, riders, arena, trail, view, grid, speed, trails; uint8_t gridc[3], sky[3]; };
static const Cy6Pre cy6_pre[11] = {
  { A5_ARCADE, 2, 0, 0, 0, 50, 0, 60, {0,90,160}, {0,0,0} },     { A5_ARCADE, 4, 0, 0, 0, 50, 0, 60, {0,90,160}, {0,0,0} },
  { A5_ARCADE, 6, 0, 0, 0, 40, 0, 60, {60,60,120}, {0,0,0} },    { A5_ARCADE, 4, 0, 60, 0, 40, 0, 90, {40,120,60}, {0,0,0} },
  { A5_ARCADE, 4, 1, 0, 0, 40, 0, 60, {120,40,140}, {0,0,0} },   { A5_ARCADE, 4, 0, 0, 1, 60, 0, 0, {0,120,200}, {0,10,30} },
  { A5_NEON, 4, 0, 0, 1, 70, 0, 0, {255,0,200}, {60,0,60} },     { A5_ARCADE, 4, 2, 0, 0, 40, 0, 60, {90,90,90}, {0,0,0} },
  { A5_PHOSPHOR, 3, 0, 0, 0, 40, 0, 170, {100,100,100}, {0,0,0} }, { A5_ARCADE, 4, 0, 0, 1, 60, 0, 0, {150,200,255}, {20,30,60} },
  { A5_ARCADE, 4, 0, 0, 0, 40, 1, 60, {0,90,160}, {0,0,0} },
};
static const char* const cy6_names[12] = { "Classic Duel", "Four-Way", "Six Pack", "Snakes", "Wrap-Around", "Tilted 3-D", "Neon Sunset", "Obstacles", "Phosphor", "Ice Grid", "Fast Lane", "Tour" };
static const char* const cy6_lbl[16] = { "Heading (N/E/S/W)", "Boost", "", "", "", "", "", "Riders", "Arena", "Trail Length", "View", "Grid", "", "", "", "" };
const char* prog_cycles_name() { return "CYCLES"; }
const char* prog_cycles_character() { return "Light-trail duel on a glowing grid, top-down or tilted 3-D - plays itself until you touch p0/k12"; }
const char* prog_cycles_presetName(int preset) { return (preset >= 0 && preset < 12) ? cy6_names[preset] : NULL; }
const char* prog_cycles_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, cy6_lbl); }
uint8_t prog_cycles_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static const uint8_t cy6_cols[6][3] = { {40,160,255}, {255,150,20}, {255,40,90}, {60,255,90}, {255,240,40}, {200,90,255} };
#define CY6_W 80
#define CY6_H 56
#define CY6_N 6
#define CY6_RING 240
#define CY6_QN 1200
#define CY6_WALL 7
static uint8_t* cy6_g;                  // CY6_W × CY6_H: 0 empty, 1-6 a rider's trail, 7 wall (scratch)
static uint16_t* cy6_ring;              // each rider's trail cells, for short trails (scratch)
static uint16_t* cy6_q;                 // flood-fill queue (scratch)
static uint8_t* cy6_seen;               // flood-fill marks (scratch)
static int cy6_x[CY6_N], cy6_y[CY6_N], cy6_d[CY6_N], cy6_head[CY6_N], cy6_len[CY6_N], cy6_wins[CY6_N], cy6_pend = -1;
static bool cy6_alive[CY6_N];
static float cy6_deadT[CY6_N];
static int cy6_n = 2, cy6_arena = 0, cy6_trailLen = 0, cy6_preset = -1, cy6_look = 0, cy6_round = 1, cy6_seenMark = 1;
static float cy6_acc = 0, cy6_over = 0, cy6_banner = 0, cy6_diff = 0.4f;
static const int8_t cy6_dx[4] = { 0, 1, 0, -1 }, cy6_dy[4] = { -1, 0, 1, 0 };   // N E S W

static inline int cy6_wrapX(int x) { return (x + CY6_W) % CY6_W; }
static inline int cy6_wrapY(int y) { return (y + CY6_H) % CY6_H; }
static inline bool cy6_free(int x, int y) {
  if (cy6_arena == 1) { x = cy6_wrapX(x); y = cy6_wrapY(y); }
  else if (x < 0 || x >= CY6_W || y < 0 || y >= CY6_H) return false;
  return cy6_g[y * CY6_W + x] == 0;
}
// how much room is there from (x, y)? (flood fill, capped)
static int cy6_room(int x, int y, int cap) {
  if (!cy6_free(x, y)) return 0;
  if (++cy6_seenMark > 250) { memset(cy6_seen, 0, CY6_W * CY6_H); cy6_seenMark = 1; }
  if (cy6_arena == 1) { x = cy6_wrapX(x); y = cy6_wrapY(y); }
  int h = 0, t = 0, n = 0;
  cy6_q[t++] = (uint16_t)(y * CY6_W + x); cy6_seen[y * CY6_W + x] = (uint8_t)cy6_seenMark;
  while (h < t && n < cap) {
    int c = cy6_q[h++], cx = c % CY6_W, cy = c / CY6_W; n++;
    for (int d = 0; d < 4; d++) {
      int nx = cx + cy6_dx[d], ny = cy + cy6_dy[d];
      if (cy6_arena == 1) { nx = cy6_wrapX(nx); ny = cy6_wrapY(ny); } else if (nx < 0 || nx >= CY6_W || ny < 0 || ny >= CY6_H) continue;
      int k = ny * CY6_W + nx;
      if (cy6_g[k] || cy6_seen[k] == cy6_seenMark || t >= CY6_QN) continue;
      cy6_seen[k] = (uint8_t)cy6_seenMark; cy6_q[t++] = (uint16_t)k;
    }
  }
  return n;
}
static void cy6_newRound() {
  memset(cy6_g, 0, CY6_W * CY6_H); memset(cy6_seen, 0, CY6_W * CY6_H); cy6_seenMark = 1;
  if (cy6_arena == 2) for (int k = 0; k < 14; k++) {                       // obstacle blocks
    int bx = 6 + v3_rnd(CY6_W - 16), by = 6 + v3_rnd(CY6_H - 16), w = 2 + v3_rnd(6), h = 2 + v3_rnd(6);
    for (int y = by; y < by + h; y++) for (int x = bx; x < bx + w; x++) cy6_g[y * CY6_W + x] = CY6_WALL;
  }
  // riders start round the edge, facing in
  static const int sx[6] = { 20, 60, 60, 20, 40, 40 }, sy[6] = { 46, 10, 46, 10, 50, 6 }, sd[6] = { 0, 2, 3, 1, 0, 2 };
  for (int i = 0; i < CY6_N; i++) {
    cy6_alive[i] = i < cy6_n; cy6_deadT[i] = 0; cy6_head[i] = 0; cy6_len[i] = 0;
    cy6_x[i] = sx[i]; cy6_y[i] = sy[i]; cy6_d[i] = sd[i];
    if (cy6_n == 2 && i < 2) { cy6_x[i] = i ? 60 : 20; cy6_y[i] = 28; cy6_d[i] = i ? 3 : 1; }
    if (cy6_alive[i]) { for (int y = cy6_y[i] - 1; y <= cy6_y[i] + 1; y++) for (int x = cy6_x[i] - 1; x <= cy6_x[i] + 1; x++) if (x >= 0 && x < CY6_W && y >= 0 && y < CY6_H && cy6_g[y * CY6_W + x] == CY6_WALL) cy6_g[y * CY6_W + x] = 0; }
  }
  cy6_banner = 1.5f; cy6_pend = -1;
}
// the computer rider: the move that leaves the most room, with a taste for cutting others off
static int cy6_ai(int i) {
  int best = cy6_d[i], bs = -(1 << 30);
  int cap = 300 + (int)(cy6_diff * 500);
  for (int t = -1; t <= 1; t++) {
    int d = (cy6_d[i] + t + 4) & 3, nx = cy6_x[i] + cy6_dx[d], ny = cy6_y[i] + cy6_dy[d];
    if (!cy6_free(nx, ny)) continue;
    int room = cy6_room(nx, ny, cap);
    int s = room * 10 + (t == 0 ? 15 : 0) + v3_rnd(12);
    // look a few cells ahead: a wall coming soon?
    int ahead = 0; for (int k = 1; k <= 6; k++) { if (!cy6_free(cy6_x[i] + cy6_dx[d] * k, cy6_y[i] + cy6_dy[d] * k)) break; ahead++; }
    s += ahead * 4;
    // head towards the nearest rival's path, to cut it off
    for (int j = 0; j < cy6_n; j++) if (j != i && cy6_alive[j]) {
      int px = cy6_x[j] + cy6_dx[cy6_d[j]] * 6, py = cy6_y[j] + cy6_dy[cy6_d[j]] * 6;
      int dist = v3_abs(px - nx) + v3_abs(py - ny);
      if (dist < 25) s += (25 - dist) * (int)(2 + cy6_diff * 6);
    }
    if (s > bs) { bs = s; best = d; }
  }
  if (v3_rnd(1000) < (int)((1.0f - cy6_diff) * 6)) best = (cy6_d[i] + (v3_rnd(2) ? 1 : 3)) & 3;   // the odd mistake
  return best;
}

void prog_cycles_init() {
  v3_reset(); a5_reset(); cy6_preset = -1; a5_partClear();
  cy6_g = v3_scratch; cy6_seen = v3_scratch + CY6_W * CY6_H; cy6_q = (uint16_t*)(v3_scratch + 2 * CY6_W * CY6_H);
  cy6_ring = (uint16_t*)(v3_scratch + 2 * CY6_W * CY6_H + 2 * CY6_QN);   // 8960 + 2400 + 2880 = 14240 bytes
}

// ── tilted 3-D: floor point (cell x, y) → screen ──
static inline float cy6_z(float y) { return (CY6_H - y) + 28.0f; }
static inline int cy6_psx(float x, float y) { return 160 + (int)((x - CY6_W / 2) * 140.0f / cy6_z(y)); }
static inline int cy6_psy(float y) { return -28 + (int)(7392.0f / cy6_z(y)); }

static void cycles5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  bool fresh = preset != cy6_preset;
  if (fresh) { cy6_preset = preset; cy6_look = preset == 11 ? 0 : preset; memset(cy6_wins, 0, sizeof(cy6_wins)); cy6_round = 1; }
  const Cy6Pre& P = cy6_pre[cy6_look];
  cy6_diff = a5_diffK(0.45f);
  int n = v3_centre(7) ? P.riders : 2 + (int)(v3_u(7) * 4.99f);
  int arena = v3_centre(8) ? P.arena : v3_clamp(v3_k(8) * 3 / 1024, 0, 2);
  cy6_trailLen = v3_centre(9) ? P.trail : (v3_k(9) > 980 ? 0 : 20 + (int)(v3_u(9) * 300));
  if (cy6_trailLen > CY6_RING) cy6_trailLen = CY6_RING;
  int view = v3_pick(10, P.view, 2);
  int gridLev = v3_centre(11) ? P.grid * 15 / 100 : (int)(v3_u(11) * 15);
  if (fresh || n != cy6_n || arena != cy6_arena) { cy6_n = n; cy6_arena = arena; cy6_newRound(); }

  uint8_t hues[12][3]; for (int i = 0; i < 6; i++) memcpy(hues[1 + i], cy6_cols[i], 3);
  hues[0][0] = hues[0][1] = hues[0][2] = 255; memcpy(hues[7], P.gridc, 3);
  hues[8][0] = 150; hues[8][1] = 160; hues[8][2] = 190; hues[9][0] = 255; hues[9][1] = 255; hues[9][2] = 255;
  memcpy(hues[10], P.sky, 3); hues[11][0] = 255; hues[11][1] = 110; hues[11][2] = 60;
  a5_palette(hues, 12, a5_lookK(P.look));

  // ── game ──
  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    for (int i = 0; i < CY6_N; i++) if (cy6_deadT[i] > 0) {           // a crashed rider's trail fades away
      cy6_deadT[i] -= dt;
      if (cy6_deadT[i] < 1.2f && (v3_rnd(2) == 0)) for (int k = 0; k < 40; k++) { int c = v3_rnd(CY6_W * CY6_H); if (cy6_g[c] == 1 + i) cy6_g[c] = 0; }
      if (cy6_deadT[i] <= 0) for (int c = 0; c < CY6_W * CY6_H; c++) if (cy6_g[c] == 1 + i) cy6_g[c] = 0;
    }
    if (cy6_banner > 0) { cy6_banner -= dt; continue; }
    if (cy6_over > 0) { cy6_over -= dt; if (cy6_over <= 0) { cy6_round++; if (cy6_preset == 11) cy6_look = (cy6_look + 1) % 11; cy6_newRound(); } continue; }
    // player input (rider 0): the knob's angle picks a heading; k12 turns right
    if (a5_player && cy6_alive[0]) {
      if (a5_touched && !a5_fire) { int want = ((int)(v3_u(0) * 4.0f + 0.5f)) & 3; if (want != ((cy6_d[0] + 2) & 3)) cy6_pend = want; }
      if (a5_fire) cy6_pend = (cy6_d[0] + 1) & 3;
    }
    float rate = (P.speed ? 26.0f : 18.0f) * (1.0f + (a5_player ? v3_clampf(a5_stick(1), 0, 1) * 0.6f : 0));
    cy6_acc += dt * rate;
    while (cy6_acc >= 1.0f) {
      cy6_acc -= 1.0f;
      for (int i = 0; i < cy6_n; i++) if (cy6_alive[i]) {
        if (i == 0 && a5_player) { if (cy6_pend >= 0) { cy6_d[0] = cy6_pend; cy6_pend = -1; } }
        else cy6_d[i] = cy6_ai(i);
        // leave a trail where we are, then move
        int c = cy6_y[i] * CY6_W + cy6_x[i];
        cy6_g[c] = (uint8_t)(1 + i);
        if (cy6_trailLen > 0) {
          cy6_ring[i * CY6_RING + cy6_head[i]] = (uint16_t)c; cy6_head[i] = (cy6_head[i] + 1) % CY6_RING;
          if (++cy6_len[i] > cy6_trailLen) { int old = cy6_ring[i * CY6_RING + (cy6_head[i] - cy6_len[i] + 2 * CY6_RING) % CY6_RING]; if (cy6_g[old] == 1 + i) cy6_g[old] = 0; cy6_len[i]--; }
        }
        int nx = cy6_x[i] + cy6_dx[cy6_d[i]], ny = cy6_y[i] + cy6_dy[cy6_d[i]];
        if (cy6_arena == 1) { nx = cy6_wrapX(nx); ny = cy6_wrapY(ny); }
        if (!cy6_free(nx, ny)) {
          cy6_alive[i] = false; cy6_deadT[i] = 2.0f; a5_shake(i == 0 || !a5_player ? 3 : 1.5f);
          int sx = view ? cy6_psx(cy6_x[i] + 0.5f, cy6_y[i] + 0.5f) : cy6_x[i] * 4 + 2, sy = view ? cy6_psy(cy6_y[i] + 0.5f) - 4 : 16 + cy6_y[i] * 4 + 2;
          a5_burst(sx, sy, 50, 1 + i, 50, 50); a5_burst(sx, sy, 20, 0, 30, 30);
          continue;
        }
        cy6_x[i] = nx; cy6_y[i] = ny;
      }
      // two riders into the same cell: both out
      for (int i = 0; i < cy6_n; i++) for (int j = i + 1; j < cy6_n; j++) if (cy6_alive[i] && cy6_alive[j] && cy6_x[i] == cy6_x[j] && cy6_y[i] == cy6_y[j]) { cy6_alive[i] = cy6_alive[j] = false; cy6_deadT[i] = cy6_deadT[j] = 2.0f; }
      int left = 0, who = -1; for (int i = 0; i < cy6_n; i++) if (cy6_alive[i]) { left++; who = i; }
      if (left <= 1) { if (who >= 0) cy6_wins[who]++; cy6_over = 2.5f; break; }
    }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  int fx = 160, fy = 120;
  if (view == 0) {
    // top-down: a faint grid, glowing trails, bright heads
    if (gridLev > 0) { for (int x = 0; x <= CY6_W; x += 4) v3_rect(x * 4, 16, x * 4, H - 1, A5C(gridLev, 7)); for (int y = 0; y <= CY6_H; y += 4) v3_hspan(0, W - 1, 16 + y * 4, A5C(gridLev, 7)); }
    for (int y = 0; y < CY6_H; y++) for (int x = 0; x < CY6_W; x++) {
      uint8_t v = cy6_g[y * CY6_W + x]; if (!v) continue;
      int px = x * 4, py = 16 + y * 4;
      if (v == CY6_WALL) { v3_rect(px, py, px + 3, py + 3, A5C(8, 8)); v3_hspan(px, px + 3, py, A5C(12, 8)); continue; }
      v3_rect(px, py, px + 3, py + 3, A5C(9, v)); v3_rect(px + 1, py + 1, px + 2, py + 2, A5C(15, v));
    }
    if (cy6_arena != 1) { a5_seg(0, 16, W - 1, 16, A5C(12, 7)); a5_seg(0, H - 1, W - 1, H - 1, A5C(12, 7)); a5_seg(0, 16, 0, H - 1, A5C(12, 7)); a5_seg(W - 1, 16, W - 1, H - 1, A5C(12, 7)); }
    for (int i = 0; i < cy6_n; i++) if (cy6_alive[i]) {
      int px = cy6_x[i] * 4 + 2, py = 16 + cy6_y[i] * 4 + 2;
      v3_disc(px, py, 3, A5C(15, 1 + i)); v3_disc(px, py, 1, A5C(15, 0));
      if (i == 0) { fx = px; fy = py; }
    }
  } else {
    // tilted 3-D: sky, the floor grid in perspective, trails as standing walls drawn back to front
    int hor = cy6_psy(0) - 2;
    for (int y = 12; y < hor; y++) { int lev = v3_clamp((y - 12) * 10 / (hor - 12 + 1), 0, 10); memset(v3_buf + y * W, A5C(lev, 10), W); }
    if (cy6_look == 6) { int cx = 160, cyy = hor - 4; for (int r = 30; r > 0; r -= 1) { int lev = 15 - r / 3; for (int dy = -r; dy <= 0; dy++) { if (((cyy + dy) & 3) == 0 && dy > -r / 2) continue; int w = (int)v3_isqrt((uint32_t)(r * r - dy * dy)); v3_hspan(cx - w, cx + w, cyy + dy, A5C(lev > 0 ? lev : 1, 11)); } r -= 2; } }
    if (gridLev > 0) {
      for (int x = 0; x <= CY6_W; x += 4) v3_lineAA(cy6_psx((float)x, 0) * 8, cy6_psy(0) * 8, cy6_psx((float)x, CY6_H) * 8, cy6_psy(CY6_H) * 8, gridLev, 7);
      for (int y = 0; y <= CY6_H; y += 4) { int sy = cy6_psy((float)y); v3_lineAA(cy6_psx(0, (float)y) * 8, sy * 8, cy6_psx(CY6_W, (float)y) * 8, sy * 8, gridLev, 7); }
    }
    for (int y = 0; y < CY6_H; y++) {
      int sy0 = cy6_psy((float)y), sy1 = cy6_psy((float)y + 1);
      float z = cy6_z(y + 0.5f); int wallH = (int)(260.0f / z);
      for (int x = 0; x < CY6_W; x++) {
        uint8_t v = cy6_g[y * CY6_W + x]; if (!v) continue;
        int xa = cy6_psx((float)x, y + 0.5f), xb = cy6_psx((float)x + 1, y + 0.5f) - 1; if (xb < xa) xb = xa;
        int top = sy1 - wallH - (v == CY6_WALL ? wallH : 0);
        int hue = v == CY6_WALL ? 8 : v;
        v3_rect(xa, top, xb, sy1, A5C(v == CY6_WALL ? 7 : 10, hue));
        v3_hspan(xa, xb, top, A5C(15, hue));
        (void)sy0;
      }
      for (int i = 0; i < cy6_n; i++) if (cy6_alive[i] && cy6_y[i] == y) {
        int px = cy6_psx(cy6_x[i] + 0.5f, y + 0.5f), py = sy1 - wallH / 2, r = v3_clamp(wallH / 2, 2, 7);
        v3_disc(px, py, r, A5C(15, 1 + i)); v3_disc(px, py - r / 2, r / 3, A5C(15, 0));
        if (i == 0) { fx = px; fy = py; }
      }
    }
  }
  a5_partDraw();
  a5_post(fx, fy);
  // HUD: each rider's wins
  if (a5_player) a5_textS(4, 2, "1UP", 1, A5C(15, 1)); else if (((int)(globalTime * 2.5f)) & 1) a5_textS(4, 2, "DEMO", 1, A5C(15, 9));
  for (int i = 0; i < cy6_n; i++) { char s[6]; snprintf(s, 6, "%d", cy6_wins[i] % 100); a5_textS(60 + i * 40, 2, s, 1, A5C(cy6_alive[i] ? 15 : 8, 1 + i)); v3_rect(50 + i * 40, 3, 55 + i * 40, 8, A5C(cy6_alive[i] ? 15 : 6, 1 + i)); }
  char s[16]; snprintf(s, 16, "ROUND %d", cy6_round); a5_textS(W - 4 - a5_textW(s, 1), 2, s, 1, A5C(13, 9));
  if (a5_join > 0) a5_textC(H / 2 - 30, "PLAYER 1", 2, A5C(15, 1));
  if (cy6_banner > 0) a5_banner(H / 2 - 30, s, 2, A5C(15, 9));
  if (cy6_over > 0) {
    int who = -1; for (int i = 0; i < cy6_n; i++) if (cy6_alive[i]) who = i;
    static const char* const nm[6] = { "BLUE WINS", "ORANGE WINS", "RED WINS", "GREEN WINS", "YELLOW WINS", "VIOLET WINS" };
    a5_banner(H / 2 - 30, who >= 0 ? nm[who] : "NO WINNER", 2, A5C(15, who >= 0 ? 1 + who : 9));
  }
  a5_crt();
}
void prog_cycles_draw(int preset) { cycles5_draw(preset); fx5_apply(); }

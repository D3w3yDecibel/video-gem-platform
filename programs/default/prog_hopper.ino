// =====================================================================
// PROGRAM: HOPPER — hop round a pyramid of cubes, changing their colour
// (by Dewey) — Set 5 "Arcade", slot 9
//
// In the spirit of the 1982 pyramid-hopping cabinet: BOING, a round green
// critter with springy feet, hops diagonally from cube to cube. Every top it
// lands on changes colour; turn them all to the target colour to clear the
// round. Red balls bounce down the pyramid; a purple egg rolls down and
// hatches into a COILER spring that chases you — lure it after you as you
// jump onto a spinning disc and it bounces off the edge. A green ball
// freezes everything for a moment; little grey SMUDGES undo your work.
// Hop off the edge without a disc and you fall. All characters original.
//
// It plays itself; touch p0/p1 and you steer (p0 right + p1 up = hop up-right, etc).
//
// Presets:
//   k0 Classic   k1 Sky   k2 Candy   k3 Toggle (cubes flip back!)   k4 Big Pyramid
//   k5 Neon   k6 Towers (tall cubes)   k7 Night   k8 Phosphor   k9 Lava   k10 Ice
//   k11 Tour — every round a new preset
//   k12 — joins the game (hop straight down-right).
//
// Knobs:
//   p0 Hop ◄ left | right ►   p1 Hop ◄ down | up ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Pyramid Rows (centre = preset)   p8 Enemies (few … lots)
//   p9 Cube Height (flat tiles … tall towers)   p10 Colour Rule (centre = the round's: one hop, two hops, toggle)
//   p11 Background (centre = preset: black, stars, glow, chequer)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 top start, 2 top target, 3 top between, 4 left face, 5 right face, 6 Boing, 7 red ball,
//       8 coiler, 9 green ball, 10 smudge, 11 disc, 12 HUD, 13 background, 14 dark, 15 stars
struct Hp6Pre { uint8_t sides[2][3]; uint8_t tops[4][3]; uint8_t look, rows, rule, height, bg, trails, enemies; };
static const Hp6Pre hp6_pre[11] = {
  {{{210,180,130},{120,95,65}}, {{60,80,220},{255,230,40},{230,130,40},{0,0,0}}, A5_ARCADE, 7, 0, 18, 0, 0, 50},
  {{{240,240,250},{170,190,220}}, {{100,200,255},{255,120,170},{255,220,120},{0,0,0}}, A5_ARCADE, 7, 0, 18, 2, 0, 50},
  {{{255,150,200},{200,100,160}}, {{150,255,220},{255,255,140},{200,170,255},{0,0,0}}, A5_ARCADE, 7, 1, 18, 3, 0, 50},
  {{{120,60,40},{80,40,30}}, {{230,200,60},{40,200,90},{0,0,0},{0,0,0}}, A5_ARCADE, 7, 2, 18, 0, 0, 55},
  {{{60,140,90},{30,90,60}}, {{220,220,220},{255,90,60},{120,120,255},{0,0,0}}, A5_ARCADE, 8, 0, 14, 1, 0, 60},
  {{{255,0,200},{120,0,160}}, {{0,255,255},{255,255,0},{0,255,90},{0,0,0}}, A5_NEON, 7, 1, 18, 1, 90, 50},
  {{{120,120,140},{80,80,100}}, {{255,120,60},{100,240,255},{255,240,120},{0,0,0}}, A5_ARCADE, 6, 0, 46, 2, 0, 50},
  {{{40,60,140},{20,30,90}}, {{160,180,255},{255,240,120},{255,140,200},{0,0,0}}, A5_ARCADE, 7, 1, 18, 1, 60, 50},
  {{{200,200,200},{120,120,120}}, {{255,255,255},{90,90,90},{170,170,170},{0,0,0}}, A5_PHOSPHOR, 7, 0, 18, 0, 150, 50},
  {{{120,30,20},{70,15,10}}, {{255,90,20},{255,230,60},{200,40,40},{0,0,0}}, A5_ARCADE, 7, 1, 18, 2, 0, 55},
  {{{170,210,255},{110,150,220}}, {{230,245,255},{90,170,255},{170,230,255},{0,0,0}}, A5_ARCADE, 7, 2, 18, 1, 0, 50},
};
static const char* const hp6_names[12] = { "Classic", "Sky", "Candy", "Toggle", "Big Pyramid", "Neon", "Towers", "Night", "Phosphor", "Lava", "Ice", "Tour" };
static const char* const hp6_lbl[16] = { "Hop: Left|Right", "Hop: Down|Up", "", "", "", "", "", "Pyramid Rows", "Enemies", "Cube Height", "Colour Rule", "Background", "", "", "", "" };
const char* prog_hopper_name() { return "HOPPER"; }
const char* prog_hopper_character() { return "Hop round a pyramid of cubes changing their colour, dodging balls and a coiled spring - plays itself until you touch p0/p1/k12"; }
const char* prog_hopper_presetName(int preset) { return (preset >= 0 && preset < 12) ? hp6_names[preset] : NULL; }
const char* prog_hopper_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, hp6_lbl); }
uint8_t prog_hopper_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define HP6_MR 8
#define HP6_NE 6
static uint8_t hp6_cube[HP6_MR][HP6_MR];                // 0 start, 1 between, 2 target
static int hp6_rows = 7, hp6_rule = 0, hp6_faceH = 18;
// moves: 0 up-right, 1 up-left, 2 down-right, 3 down-left
static const int8_t hp6_dr[4] = { -1, -1, 1, 1 }, hp6_dc[4] = { 0, -1, 1, 0 };
static inline bool hp6_onPyr(int r, int c) { return r >= 0 && r < hp6_rows && c >= 0 && c <= r; }
static inline float hp6_cx(float r, float c) { return 160 + (2 * c - r) * 16; }
static inline float hp6_cy(float r) { int step = 8 + (hp6_faceH < 18 ? hp6_faceH : 18); return 46 + r * step - (hp6_rows - 7) * step / 2 + (18 - (hp6_faceH < 18 ? hp6_faceH : 18)) * 3; }
// Boing and the others: where they are (r, c), where they're hopping to, how far through the hop (0 … 1)
static int hp6_pr = 0, hp6_pc = 0, hp6_pnr = 0, hp6_pnc = 0, hp6_pdir = 2; static float hp6_pt = 1;
static int hp6_er[HP6_NE], hp6_ec[HP6_NE], hp6_enr[HP6_NE], hp6_enc[HP6_NE], hp6_ek[HP6_NE]; static float hp6_et[HP6_NE], hp6_ewait[HP6_NE];   // kind 0 none 1 red 2 egg 3 coiler 4 green 5 smudge
static int hp6_discR[2] = { 4, 3 }, hp6_discSide[2] = { 0, 1 }; static bool hp6_discOn[2];
static float hp6_ride = 0, hp6_fall = 0, hp6_dead = 0, hp6_freeze = 0, hp6_spawnT = 2, hp6_banner = 0, hp6_over = 0, hp6_clearT = 0, hp6_fallY = 0;
static int hp6_rideDisc = -1, hp6_lives = 3, hp6_round = 1, hp6_level = 1, hp6_preset = -1, hp6_look = 0;
static uint32_t hp6_score = 0, hp6_hi = 20000;
static float hp6_roundT = 0;
static float hp6_diff = 0.4f, hp6_enemyK = 0.5f;

static const char hp6_sBoing[2][257] = {
  "................" "......6666......" "....66666666...." "...66dd666dd6..." "...6dfd66fdfd6.." "..66ddfd66dfd66." "..66666666666666" "..6666611666666." "...66661166666.." "...66666666666.." "....666666666..." ".....6666666...." "......e...e....." ".....e.....e...." "....ee.....ee..." "................",
  "................" "................" "......6666......" "....66666666...." "...66dd666dd6..." "...6dfd66fdfd6.." "..66ddfd66dfd66." "..66666666666666" "..6666611666666." "...66661166666.." "...66666666666.." "....666666666..." ".....e.e.e.e...." "......e...e....." ".....ee...ee...." "................"
};
static const char hp6_sCoiler[2][257] = {
  "......8888......" ".....888888....." ".....8f88f8....." ".....8d88d.8...." "......8888......" "......1111......" ".....888888....." "......1111......" ".....888888....." "......1111......" ".....888888....." "......1111......" ".....888888....." "....88888888...." "................" "................",
  "................" "................" "......8888......" ".....888888....." ".....8f88f8....." ".....8d88d.8...." "......8888......" ".....11111......" "....8888888....." ".....11111......" "....8888888....." ".....11111......" "....8888888....." "...888888888...." "................" "................"
};
static uint8_t hp6_cmap[16];

static void hp6_newRound() {
  memset(hp6_cube, 0, sizeof(hp6_cube));
  for (int i = 0; i < HP6_NE; i++) hp6_ek[i] = 0;
  hp6_pr = hp6_pc = hp6_pnr = hp6_pnc = 0; hp6_pt = 1; hp6_fall = 0; hp6_dead = 0; hp6_ride = 0; hp6_rideDisc = -1; hp6_freeze = 0;
  hp6_discR[0] = v3_clamp(hp6_rows - 3 - v3_rnd(2), 1, hp6_rows - 1); hp6_discR[1] = v3_clamp(hp6_rows - 2 - v3_rnd(3), 1, hp6_rows - 1);
  hp6_discSide[0] = 0; hp6_discSide[1] = 1; hp6_discOn[0] = hp6_discOn[1] = true;
  hp6_spawnT = 2.0f; hp6_banner = 1.5f; hp6_clearT = 0; hp6_roundT = 0;
}
static void hp6_newGame() { hp6_score = 0; hp6_lives = 3; hp6_round = 1; hp6_level = 1; hp6_over = 0; hp6_newRound(); }
static int hp6_ruleNow() { int r = v3_centre(10) ? (hp6_pre[hp6_look].rule + (hp6_level - 1)) % 3 : v3_clamp(v3_k(10) * 3 / 1024, 0, 2); return (r == 2 && hp6_roundT > 40 && !a5_player) ? 1 : r; }
static void hp6_land(int r, int c) {
  uint8_t& v = hp6_cube[r][c];
  int rule = hp6_ruleNow();
  if (rule == 0) { if (v != 2) { v = 2; hp6_score += 25; } }
  else if (rule == 1) { if (v < 2) { v++; hp6_score += 15; } }
  else { v = v == 2 ? 0 : 2; hp6_score += 15; }
}
static bool hp6_allDone() { for (int r = 0; r < hp6_rows; r++) for (int c = 0; c <= r; c++) if (hp6_cube[r][c] != 2) return false; return true; }
static int hp6_discAt(int r, int c) {               // a disc beside the pyramid at this off-pyramid spot?
  for (int d = 0; d < 2; d++) if (hp6_discOn[d] && r == hp6_discR[d] && ((hp6_discSide[d] == 0 && c == -1) || (hp6_discSide[d] == 1 && c == r + 1))) return d;
  return -1;
}
static void hp6_spawn() {
  for (int i = 0; i < HP6_NE; i++) if (!hp6_ek[i]) {
    int r = v3_rnd(100), k = 1;
    bool coiler = false; for (int j = 0; j < HP6_NE; j++) if (hp6_ek[j] == 2 || hp6_ek[j] == 3) coiler = true;
    if (!coiler && r < 35) k = 2; else if (r < 50 && hp6_round > 1) k = 4; else if (r < 65 && hp6_level > 1) k = 5; else k = 1;
    hp6_ek[i] = k; hp6_er[i] = 1; hp6_ec[i] = v3_rnd(2); hp6_enr[i] = hp6_er[i]; hp6_enc[i] = hp6_ec[i]; hp6_et[i] = 1; hp6_ewait[i] = 0.4f;
    return;
  }
}
// how dangerous is cube (r, c) about to be?
static int hp6_danger(int r, int c) {
  int d = 0;
  for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i] && hp6_ek[i] != 4 && hp6_ek[i] != 5) {
    int er = hp6_et[i] < 1 ? hp6_enr[i] : hp6_er[i], ec = hp6_et[i] < 1 ? hp6_enc[i] : hp6_ec[i];
    if (er == r && ec == c) d += 100;
    for (int m = 0; m < 4; m++) if (er + hp6_dr[m] == r && ec + hp6_dc[m] == c) d += (hp6_ek[i] == 3 ? 70 : (hp6_dr[m] > 0 ? 45 : 0));
  }
  return d;
}
// the computer: hop to the nearest cube still to change, keep away from trouble, use a disc when the coiler is close
static int hp6_ai() {
  int best = -1, bs = -(1 << 30);
  bool coilerNear = false;
  for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i] == 3 && v3_abs(hp6_er[i] - hp6_pr) + v3_abs(hp6_ec[i] - hp6_pc) <= 3) coilerNear = true;
  for (int m = 0; m < 4; m++) {
    int r = hp6_pr + hp6_dr[m], c = hp6_pc + hp6_dc[m], s = v3_rnd(10);
    if (!hp6_onPyr(r, c)) { if (hp6_discAt(r, c) >= 0) s += coilerNear ? 400 : -300; else s -= 5000; }
    else {
      s -= hp6_danger(r, c) * 4;
      if (hp6_cube[r][c] != 2) s += 60; else if (hp6_ruleNow() == 2) s -= 90;
      int nd = 99; for (int rr = 0; rr < hp6_rows; rr++) for (int cc = 0; cc <= rr; cc++) if (hp6_cube[rr][cc] != 2) { int d = v3_abs(rr - r) + v3_abs((cc - rr / 2) - (c - r / 2)); if (d < nd) nd = d; }
      s -= nd * 12;
      for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i] == 4 && hp6_er[i] == r && hp6_ec[i] == c) s += 80;
    }
    if (s > bs) { bs = s; best = m; }
  }
  return best;
}
static void hp6_die(bool fell) {
  if (hp6_dead > 0 || hp6_fall > 0) return;
  if (fell) { hp6_fall = 1.6f; hp6_fallY = 0; } else { hp6_dead = 1.8f; a5_shake(3); }
}

void prog_hopper_init() { v3_reset(); a5_reset(); hp6_preset = -1; a5_partClear(); }

static void hopper5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != hp6_preset) { hp6_preset = preset; hp6_look = preset == 11 ? 0 : preset; hp6_rows = hp6_pre[hp6_look].rows; hp6_newGame(); }
  const Hp6Pre& P = hp6_pre[hp6_look];
  hp6_diff = a5_diffK(0.4f);
  int rows = v3_centre(7) ? P.rows : 4 + (int)(v3_u(7) * 4.99f);
  if (rows != hp6_rows) { hp6_rows = rows; hp6_newRound(); }
  hp6_enemyK = v3_centre(8) ? P.enemies / 100.0f : v3_u(8);
  hp6_faceH = v3_centre(9) ? P.height : 2 + (int)(v3_u(9) * 50);
  int bg = v3_pick(11, P.bg, 4);

  uint8_t hues[16][3] = { {255,255,255},{0},{0},{0},{0},{0},{120,230,60},{255,40,40},{190,80,255},{60,255,90},{150,150,160},{0},{255,220,60},{0},{20,20,30},{200,210,255} };
  memcpy(hues[1], P.tops[0], 3); memcpy(hues[2], P.tops[1], 3); memcpy(hues[3], P.tops[2], 3); memcpy(hues[4], P.sides[0], 3); memcpy(hues[5], P.sides[1], 3);
  uint8_t dc[3]; v3_hsv((int)(globalTime * 700), 200, 255, dc); memcpy(hues[11], dc, 3);
  hues[13][0] = P.sides[1][0] / 3; hues[13][1] = P.sides[1][1] / 3; hues[13][2] = P.sides[1][2] / 3;
  a5_palette(hues, 16, a5_lookK(P.look));
  static const uint8_t cmBase[16] = { 0, A5C(13,8), 0, 0, 0, 0, A5C(14,6), 0, A5C(14,8), 0, 0, 0, 0, A5C(1,14), A5C(10,6), A5C(15,0) };
  memcpy(hp6_cmap, cmBase, 16);

  float dt = 1.0f / 60.0f, hopT = 0.30f - hp6_diff * 0.06f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (hp6_banner > 0) hp6_banner -= dt;
    if (hp6_over > 0) { hp6_over -= dt; if (hp6_over <= 0) { if (hp6_preset == 11) hp6_look = (hp6_look + 1) % 11; hp6_rows = hp6_pre[hp6_look].rows; hp6_newGame(); } continue; }
    if (hp6_clearT > 0) {                                                      // round cleared: the pyramid flashes
      hp6_clearT -= dt;
      if (hp6_clearT <= 0) { hp6_round++; if (hp6_round > 4) { hp6_round = 1; hp6_level++; } if (hp6_preset == 11) { hp6_look = (hp6_look + 1) % 11; hp6_rows = hp6_pre[hp6_look].rows; } hp6_newRound(); }
      continue;
    }
    if (hp6_dead > 0 || hp6_fall > 0) {
      if (hp6_fall > 0) { hp6_fall -= dt; hp6_fallY += (1.6f - hp6_fall) * 6; }
      if (hp6_dead > 0) hp6_dead -= dt;
      if (hp6_dead <= 0 && hp6_fall <= 0) {
        if (--hp6_lives < 0) { hp6_over = 4; if (hp6_score > hp6_hi) hp6_hi = hp6_score; }
        else { for (int i = 0; i < HP6_NE; i++) hp6_ek[i] = 0; hp6_pr = hp6_pc = hp6_pnr = hp6_pnc = 0; hp6_pt = 1; hp6_spawnT = 1.5f; }
        hp6_dead = 0; hp6_fall = 0;
      }
      continue;
    }
    if (hp6_freeze > 0) hp6_freeze -= dt;
    hp6_roundT += dt;
    // riding a disc up to the top
    if (hp6_rideDisc >= 0) {
      hp6_ride += dt / 1.6f;
      if (hp6_ride >= 1) { hp6_rideDisc = -1; hp6_pr = hp6_pc = hp6_pnr = hp6_pnc = 0; hp6_pt = 1; hp6_land(0, 0); }
      continue;
    }
    // Boing
    if (hp6_pt < 1) {
      hp6_pt += dt / hopT;
      if (hp6_pt >= 1) {
        hp6_pt = 1; hp6_pr = hp6_pnr; hp6_pc = hp6_pnc;
        if (hp6_onPyr(hp6_pr, hp6_pc)) hp6_land(hp6_pr, hp6_pc);
        else {
          int d = hp6_discAt(hp6_pr, hp6_pc);
          if (d >= 0) { hp6_rideDisc = d; hp6_discOn[d] = false; hp6_ride = 0; hp6_score += 50;
            for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i] == 3 && v3_abs(hp6_er[i] - hp6_pr) <= 2) { hp6_ek[i] = 0; hp6_score += 500; a5_burst((int)hp6_cx(hp6_er[i], hp6_ec[i]), (int)hp6_cy(hp6_er[i]), 30, 8, 40, 40); }   // the coiler follows you off the edge
          } else hp6_die(true);
        }
      }
    } else {
      int m = -1;
      if (a5_player) {
        float sx = a5_stick(0), sy = a5_stick(1);
        if (sx != 0 || sy != 0) { bool right = sx > 0 || (sx == 0 && hp6_pdir % 2 == 0), up = sy > 0; m = up ? (right ? 0 : 1) : (right ? 2 : 3); }
        if (a5_fire) m = 2;
      } else if (v3_rnd(100) < 70 + (int)(hp6_diff * 30)) m = hp6_ai();
      if (m >= 0) { hp6_pdir = m; hp6_pnr = hp6_pr + hp6_dr[m]; hp6_pnc = hp6_pc + hp6_dc[m]; hp6_pt = 0; }
    }
    // enemies
    hp6_spawnT -= dt;
    if (hp6_spawnT <= 0) { hp6_spawn(); hp6_spawnT = (3.5f - hp6_diff * 1.5f) / (0.3f + hp6_enemyK * 1.4f); }
    for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i]) {
      if (hp6_freeze > 0 && hp6_ek[i] != 4) continue;
      int k = hp6_ek[i];
      if (hp6_et[i] < 1) {
        hp6_et[i] += dt / (k == 3 ? hopT * 1.5f : hopT * 1.25f);
        if (hp6_et[i] >= 1) {
          hp6_et[i] = 1; hp6_er[i] = hp6_enr[i]; hp6_ec[i] = hp6_enc[i]; hp6_ewait[i] = k == 3 ? 0.15f : 0.25f;
          if (!hp6_onPyr(hp6_er[i], hp6_ec[i])) { hp6_ek[i] = 0; continue; }
          if (k == 5) hp6_cube[hp6_er[i]][hp6_ec[i]] = 0;                       // smudges undo your work
          if (k == 2 && hp6_er[i] == hp6_rows - 1) { hp6_ek[i] = 3; hp6_ewait[i] = 0.8f; }   // the egg hatches
        }
      } else {
        hp6_ewait[i] -= dt; if (hp6_ewait[i] > 0) continue;
        int m;
        if (k == 3) {                                                            // the coiler chases
          int tr = hp6_rideDisc >= 0 ? hp6_discR[hp6_rideDisc] : hp6_pr, tc = hp6_pc;
          m = tr < hp6_er[i] ? (tc > hp6_ec[i] - (hp6_er[i] - tr) / 2 ? 0 : 1) : (tc > hp6_ec[i] ? 2 : 3);
          if (tr == hp6_er[i]) m = tc > hp6_ec[i] ? (v3_rnd(2) ? 0 : 2) : (v3_rnd(2) ? 1 : 3);
          if (!hp6_onPyr(hp6_er[i] + hp6_dr[m], hp6_ec[i] + hp6_dc[m]) && v3_rnd(4)) m = (m ^ 1);
        } else m = 2 + v3_rnd(2);                                                // balls just bounce down
        hp6_enr[i] = hp6_er[i] + hp6_dr[m]; hp6_enc[i] = hp6_ec[i] + hp6_dc[m]; hp6_et[i] = 0;
      }
      // touching Boing?
      if (hp6_pt >= 1 && hp6_et[i] >= 1 && hp6_er[i] == hp6_pr && hp6_ec[i] == hp6_pc && hp6_rideDisc < 0) {
        if (k == 4) { hp6_freeze = 3; hp6_ek[i] = 0; hp6_score += 100; }
        else if (k == 5) { hp6_ek[i] = 0; hp6_score += 300; a5_burst((int)hp6_cx(hp6_pr, hp6_pc), (int)hp6_cy(hp6_pr) - 6, 14, 10, 30, 20); }
        else hp6_die(false);
      }
    }
    if (hp6_allDone()) { hp6_clearT = 2.0f; hp6_score += 1000 + 250 * hp6_level; for (int i = 0; i < HP6_NE; i++) hp6_ek[i] = 0; for (int d = 0; d < 2; d++) if (hp6_discOn[d]) hp6_score += 50; }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (bg == 1) a5_stars(80, 0, 0, 15, 41);
  if (bg == 2) for (int y = 0; y < H; y += 2) { int lev = 5 - v3_abs(y - 140) / 24; if (lev > 0) memset(v3_buf + y * W, A5C(lev, 13), W); }
  if (bg == 3) for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if ((((x + (int)(globalTime * 10)) >> 4) + (y >> 4)) & 1) v3_buf[y * W + x] = A5C(3, 13);
  bool flash = hp6_clearT > 0 && ((int)(hp6_clearT * 10) & 1);
  // cubes, top row first
  for (int r = 0; r < hp6_rows; r++) for (int c = 0; c <= r; c++) {
    int x = (int)hp6_cx(r, c), y = (int)hp6_cy(r), fh = hp6_faceH;
    a5_quad(x - 16, y, x, y + 8, x, y + 8 + fh, x - 16, y + fh, A5C(14, 4));
    a5_quad(x, y + 8, x + 16, y, x + 16, y + fh, x, y + 8 + fh, A5C(14, 5));
    int v = hp6_cube[r][c], hue = v == 2 ? 2 : (v == 1 ? 3 : 1);
    if (flash) hue = (r + c) & 1 ? 2 : 0;
    a5_quad(x, y - 8, x + 16, y, x, y + 8, x - 16, y, A5C(15, hue));
  }
  // discs
  for (int d = 0; d < 2; d++) if (hp6_discOn[d] || hp6_rideDisc == d) {
    float r = hp6_discR[d], c = hp6_discSide[d] ? r + 1 : -1;
    float x = hp6_cx(r, c), y = hp6_cy(r) - 4;
    if (hp6_rideDisc == d) { float t = hp6_ride; x += (hp6_cx(0, 0) - x) * t; y += (hp6_cy(0) - 20 - y) * t; }
    int w = 9 + (int)(2 * sinf(globalTime * 8));
    for (int q = -2; q <= 2; q++) v3_hspan((int)x - w + v3_abs(q), (int)x + w - v3_abs(q), (int)y + q, A5C(15 - v3_abs(q) * 2, 11));
    if (hp6_rideDisc == d) a5_spr(hp6_sBoing[0], 16, 16, (int)x - 8, (int)y - 16, hp6_cmap, 0, 0);
  }
  // enemies
  for (int i = 0; i < HP6_NE; i++) if (hp6_ek[i]) {
    float t = hp6_et[i], r0 = hp6_er[i], c0 = hp6_ec[i], r1 = hp6_enr[i], c1 = hp6_enc[i];
    float x = hp6_cx(r0, c0) + (hp6_cx(r1, c1) - hp6_cx(r0, c0)) * t, y = hp6_cy(r0) + (hp6_cy(r1) - hp6_cy(r0)) * t - sinf(t * 3.14159f) * 14;
    int k = hp6_ek[i], ix = (int)x, iy = (int)y;
    if (k == 3) { a5_spr(hp6_sCoiler[(int)(globalTime * 6) & 1], 16, 16, ix - 8, iy - 15, hp6_cmap, (hp6_pc < hp6_ec[i]) ? A5_FX : 0, hp6_freeze > 0 ? A5C(10, 0) : 0); continue; }
    int hue = k == 1 ? 7 : (k == 2 ? 8 : (k == 4 ? 9 : 10)), rad = k == 2 ? 6 : (k == 5 ? 4 : 5);
    v3_disc(ix, iy - rad, rad, A5C(hp6_freeze > 0 && k != 4 ? 9 : 13, hue)); v3_disc(ix - rad / 3, iy - rad - rad / 3, rad / 3, A5C(15, hue));
    if (k == 5) { v3_put(ix - 2, iy - 5, A5C(15, 0)); v3_put(ix + 1, iy - 5, A5C(15, 0)); }
  }
  // Boing
  if (hp6_rideDisc < 0 && hp6_over <= 0 && (hp6_dead <= 0 || ((int)(hp6_dead * 12) & 1))) {
    float t = hp6_pt, x = hp6_cx(hp6_pr, hp6_pc) + (hp6_cx(hp6_pnr, hp6_pnc) - hp6_cx(hp6_pr, hp6_pc)) * t;
    float y = hp6_cy(hp6_pr) + (hp6_cy(hp6_pnr) - hp6_cy(hp6_pr)) * t - sinf(t * 3.14159f) * 16 + hp6_fallY;
    int fl = (hp6_pdir == 1 || hp6_pdir == 3) ? A5_FX : 0;
    a5_spr(hp6_sBoing[t < 1 ? 1 : 0], 16, 16, (int)x - 8, (int)y - 15, hp6_cmap, fl, 0);
    if (hp6_dead > 0) a5_textS((int)x - 14, (int)y - 30, "OOPS!", 1, A5C(15, 12));
  }
  a5_partDraw();
  float fx = hp6_cx(hp6_pr, hp6_pc), fy = hp6_cy(hp6_pr);
  a5_post((int)fx, (int)fy);
  a5_hud(hp6_score, hp6_hi, A5C(15, 12), A5C(15, 0));
  char s[24]; snprintf(s, 24, "LEVEL %d ROUND %d", hp6_level, hp6_round); a5_textS(W - 4 - a5_textW(s, 1), 14, s, 1, A5C(13, 12));
  // the target colour, as in the original's corner hint
  a5_textS(6, 24, "TARGET", 1, A5C(12, 12)); a5_quad(54, 26, 62, 30, 54, 34, 46, 30, A5C(15, 2));
  for (int i = 0; i < v3_clamp(hp6_lives, 0, 5); i++) a5_spr(hp6_sBoing[0], 16, 16, 4 + i * 16, 36, hp6_cmap, 0, 0);
  if (hp6_freeze > 0) a5_textS(W / 2 - 24, H - 14, "FROZEN!", 1, A5C(15, 9));
  if (hp6_banner > 0) { a5_banner(H / 2 + 60, s, 1, A5C(15, 12)); }
  if (hp6_clearT > 0) a5_banner(H - 30, "ROUND CLEAR!", 2, A5C(15, 2));
  if (hp6_over > 0) a5_banner(H / 2 + 50, "GAME OVER", 2, A5C(15, 7));
  a5_crt();
}
void prog_hopper_draw(int preset) { hopper5_draw(preset); fx5_apply(); }

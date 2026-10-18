// =====================================================================
// PROGRAM: DIGGER — tunnel through the earth, puff up the critters
// (by Dewey) — Set 5 "Arcade", slot 0
//
// In the spirit of the 1982 dig-'em-up: a little miner in a hard hat digs
// tunnels through four layers of earth. Round one-eyed PUFFS and fire-
// spitting SALAMANDERS wander the tunnels — and drift through solid dirt as
// a pair of eyes when they want to reach you. Fire the air hose (k12) to
// hook one, keep pumping until it pops. Dig under a boulder and it wobbles
// and drops, flattening whatever is below. Clear every critter for the
// next round. All characters are original.
//
// It plays itself; turn p0/p1 or press k12 and you're the miner.
//
// Presets:
//   k0 Classic     k1 Deep Earth (more salamanders)   k2 Candy
//   k3 Night Shift (trails)   k4 Lava Core   k5 Glacier   k6 Swarm
//   k7 Boulder Run   k8 Neon   k9 Phosphor   k10 Toy Box
//   k11 Tour — a new look every round
//   k12 — PUMP (fire the hose; keep pressing to pump). Joins the game.
//
// Knobs:
//   p0 Stick ◄ left | right ►   p1 Stick ◄ down | up ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Critters (centre = preset)   p8 Boulders (centre = preset)
//   p9 Ghosting (how often critters drift through dirt)
//   p10 Dirt Pattern (centre = preset: speckle, strata, bricks, pebbles, checks)
//   p11 Pumps to Pop (centre = 4)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 yellow, 2 orange suit, 3 skin, 4 hose, 5 puff, 6 puff shade, 7 salamander,
//       8 fire, 9 rock, 10-13 the four strata, 14 flower/accent, 15 dark detail
struct Dg6Pre {
  uint8_t strata[4][3]; uint8_t look, crit, spit, rocks, diff, pattern, trails;
};
static const Dg6Pre dg6_pre[11] = {
  {{{232,170,40},{200,110,30},{170,60,20},{120,30,20}}, A5_ARCADE, 4, 1, 3, 40, 0, 0},
  {{{140,100,70},{100,70,60},{80,50,60},{60,30,50}}, A5_ARCADE, 5, 3, 3, 50, 1, 0},
  {{{255,170,210},{190,240,210},{200,190,255},{255,220,170}}, A5_ARCADE, 4, 1, 3, 35, 4, 0},
  {{{40,60,150},{30,40,120},{40,30,100},{50,20,80}}, A5_ARCADE, 5, 2, 3, 45, 0, 140},
  {{{255,120,30},{230,60,20},{190,20,20},{120,0,30}}, A5_ARCADE, 5, 4, 3, 55, 3, 60},
  {{{220,240,255},{150,200,240},{90,150,220},{50,90,180}}, A5_ARCADE, 4, 2, 4, 40, 2, 0},
  {{{200,150,60},{170,110,50},{140,80,40},{100,50,40}}, A5_ARCADE, 8, 2, 2, 70, 0, 0},
  {{{180,160,140},{150,130,120},{120,100,100},{90,70,80}}, A5_ARCADE, 4, 1, 8, 40, 3, 0},
  {{{255,0,200},{0,220,255},{120,255,0},{255,200,0}}, A5_NEON, 5, 2, 3, 50, 2, 120},
  {{{200,200,200},{160,160,160},{120,120,120},{90,90,90}}, A5_PHOSPHOR, 5, 2, 3, 45, 0, 170},
  {{{255,220,0},{255,60,40},{40,140,255},{60,200,60}}, A5_CGA, 4, 2, 3, 45, 4, 0},
};
static const char* const dg6_names[12] = { "Classic", "Deep Earth", "Candy", "Night Shift", "Lava Core", "Glacier", "Swarm", "Boulder Run", "Neon", "Phosphor", "Toy Box", "Tour" };
static const char* const dg6_lbl[16] = { "Stick: Left|Right", "Stick: Down|Up", "", "", "", "", "", "Critters", "Boulders", "Ghosting", "Dirt Pattern", "Pumps to Pop", "", "", "", "" };
const char* prog_digger_name() { return "DIGGER"; }
const char* prog_digger_character() { return "Dig tunnels, pump up the critters, drop the boulders - plays itself until you touch p0/p1/k12"; }
const char* prog_digger_presetName(int preset) { return (preset >= 0 && preset < 12) ? dg6_names[preset] : NULL; }
const char* prog_digger_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, dg6_lbl); }
uint8_t prog_digger_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ── sprites (16 × 16, original) ──
static const char dg6_sMiner[2][257] = {
  "................"
  ".....11111......"
  "....11111113...."
  "...2222222222..."
  ".....444484....."
  ".....44444......"
  "....555559999..."
  "...6555555999..."
  "...655555599...."
  "...65555555....."
  "....555555......"
  "....55..55......"
  "....55..55......"
  "...777..777....."
  "................"
  "................",
  "................"
  ".....11111......"
  "....11111113...."
  "...2222222222..."
  ".....444484....."
  ".....44444......"
  "....555559999..."
  "...6555555999..."
  "...655555599...."
  "...65555555....."
  "....555555......"
  "...55....55....."
  "..55......55...."
  ".777......777..."
  "................"
  "................"
};
static const char dg6_sPuff[2][257] = {
  "................"
  "........5......."
  ".......4........"
  ".....333333....."
  "....33333333...."
  "...3331113333..."
  "...3311221333..."
  "...3311221333..."
  "...3331113333..."
  "...4333333334..."
  "...4433333344..."
  "....44444444...."
  ".....4....4....."
  "....44....44...."
  "................"
  "................",
  "................"
  ".......5........"
  "........4......."
  ".....333333....."
  "....33333333...."
  "...3331113333..."
  "...3311221333..."
  "...3311221333..."
  "...3331113333..."
  "...4333333334..."
  "...4433333344..."
  "....44444444...."
  "......4..4......"
  ".....44..44....."
  "................"
  "................"
};
static const char dg6_sSal[2][257] = {
  "................"
  "................"
  "..........3333.."
  ".........333333."
  ".........3312333"
  "..3.....33311333"
  "..33..3333333366"
  "...333333333333."
  "...3355555553..."
  "....355555533..."
  ".....3333333...."
  ".....4..4.4....."
  "....44.44.44...."
  "................"
  "................"
  "................",
  "................"
  "................"
  "..........3333.."
  ".........333333."
  ".........3312333"
  ".3......33311333"
  "..3...333333336."
  "..3333333333333."
  "...3355555553..."
  "....355555533..."
  ".....3333333...."
  "......4.4..4...."
  ".....44.44.44..."
  "................"
  "................"
  "................"
};
static const char dg6_sRock[257] =
  "................"
  ".....999999....."
  "...9999999999..."
  "..999aaa999999.."
  "..99aaa9999999.."
  ".9999a99999b999."
  ".999999999bb999."
  ".99999999999999."
  ".99b99999999999."
  ".9bb9999999b999."
  "..9999999999b9.."
  "..999b99999999.."
  "...9999999999..."
  ".....999999....."
  "................"
  "................";
static const uint8_t dg6_cMiner[10] = { 0, A5C(15,1), A5C(10,1), A5C(15,0), A5C(14,3), A5C(14,2), A5C(9,2), A5C(6,15), A5C(3,15), A5C(12,4) };
static const uint8_t dg6_cPuff[6] = { 0, A5C(15,0), A5C(2,15), A5C(14,5), A5C(9,5), A5C(15,14) };
static const uint8_t dg6_cSal[7] = { 0, A5C(15,0), A5C(2,15), A5C(13,7), A5C(8,7), A5C(13,1), A5C(13,8) };
static const uint8_t dg6_cRock[12] = { 0,0,0,0,0,0,0,0,0, A5C(10,9), A5C(15,9), A5C(5,9) };
static const uint8_t dg6_hue[16][3] = {
  {255,255,255},{255,220,40},{255,120,30},{255,200,160},{220,230,255},{190,70,230},{120,40,160},{70,210,90},
  {255,90,20},{170,160,150},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{255,60,90},{20,30,90}
};

// ── the world ──
#define DG6_CW 80                // dug map: 4 px cells, 80 × 48, below y = 40
#define DG6_CH 48
#define DG6_TOP 40
#define DG6_Y0 384               // grid origin y (24 px) in 1/16 px
#define DG6_G 256                // one tile, 1/16 px
#define DG6_NE 10
static uint8_t* dg6_dug;         // DG6_CW × DG6_CH (scratch)
static uint8_t* dg6_pat;         // 4 strata × 16 rows × 64 px of dirt levels (scratch)
static int dg6_patType = -1;
enum { DG6_WALK, DG6_GHOST, DG6_CAUGHT, DG6_POP, DG6_DEAD, DG6_FIRE };
static int dg6_ex[DG6_NE], dg6_ey[DG6_NE], dg6_ed[DG6_NE], dg6_et[DG6_NE], dg6_em[DG6_NE], dg6_etm[DG6_NE], dg6_ei[DG6_NE], dg6_hx[DG6_NE], dg6_hy[DG6_NE], dg6_gt[DG6_NE];
static int dg6_ne = 0;
#define DG6_NR 8
static int dg6_rx[DG6_NR], dg6_ry[DG6_NR], dg6_rs[DG6_NR], dg6_rt[DG6_NR], dg6_nr = 0;   // rocks: px, state 0 rest 1 wobble 2 fall 3 crumble 4 gone
static int dg6_px, dg6_py, dg6_pd = 0, dg6_anim = 0, dg6_dying = 0, dg6_lives = 3, dg6_round = 1, dg6_over = 0, dg6_banner = 0;
static int dg6_hs = 0, dg6_hl = 0, dg6_hd = 0, dg6_ht = -1, dg6_pumpT = 0, dg6_aiPump = 0, dg6_aiWander = 0, dg6_aiWdir = 0;
static uint32_t dg6_score = 0, dg6_hi = 10000;
static int dg6_popX[4], dg6_popY[4], dg6_popV[4], dg6_popT[4];
static int dg6_preset = -1, dg6_look = 0, dg6_crit = 4, dg6_spit = 1, dg6_rocks = 3, dg6_pumps = 4;
static float dg6_diff = 0.4f, dg6_ghostK = 1.0f;
static const int dg6_dx[4] = { 1, 0, -1, 0 }, dg6_dy[4] = { 0, 1, 0, -1 };

static inline bool dg6_cellOpen(int cx, int cy) {
  if (cy < 0) return true;
  if ((unsigned)cx >= DG6_CW || cy >= DG6_CH) return false;
  return dg6_dug[cy * DG6_CW + cx] != 0;
}
// is the 16 × 16 box at (x, y) (1/16 px) all tunnel?
static bool dg6_boxOpen(int x, int y) {
  int px = x >> 4, py = y >> 4;
  if (px < 0 || px > W - 16 || py < 24 || py > 216) return false;
  int c0 = px >> 2, c1 = (px + 15) >> 2, r0 = (py - DG6_TOP) >> 2, r1 = (py + 15 - DG6_TOP) >> 2;
  if (py < DG6_TOP) r0 = -1;
  for (int r = r0; r <= r1; r++) { if (r < 0) continue; for (int c = c0; c <= c1; c++) if (!dg6_cellOpen(c, r)) return false; }
  return true;
}
static void dg6_digBox(int x, int y) {
  int px = x >> 4, py = y >> 4;
  for (int yy = py; yy < py + 16; yy += 4) {
    int r = (yy - DG6_TOP) >> 2; if (yy < DG6_TOP || r >= DG6_CH) continue;
    for (int xx = px; xx < px + 16; xx += 4) { int c = xx >> 2; if ((unsigned)c < DG6_CW) dg6_dug[r * DG6_CW + c] = 1; }
  }
  // cells only partly covered at the edges (x not a multiple of 4)
  if (px & 3) for (int yy = py; yy < py + 16; yy += 4) { int r = (yy - DG6_TOP) >> 2; int c = (px + 15) >> 2; if (yy >= DG6_TOP && r < DG6_CH && (unsigned)c < DG6_CW) dg6_dug[r * DG6_CW + c] = 1; }
  if (py & 3) for (int xx = px; xx < px + 16; xx += 4) { int r = (py + 15 - DG6_TOP) >> 2; int c = xx >> 2; if (py + 15 >= DG6_TOP && r < DG6_CH && (unsigned)c < DG6_CW) dg6_dug[r * DG6_CW + c] = 1; }
}
static bool dg6_rockAt(int x, int y) {        // does a box at (x, y) overlap a resting or wobbling boulder?
  for (int i = 0; i < dg6_nr; i++) if (dg6_rs[i] <= 1) {
    if (v3_abs(dg6_rx[i] - x) < 15 * 16 && v3_abs(dg6_ry[i] - y) < 15 * 16) return true;
  }
  return false;
}
static inline bool dg6_alignedX(int x) { return (x % DG6_G) == 0; }
static inline bool dg6_alignedY(int y) { return ((y - DG6_Y0) % DG6_G) == 0; }
static inline int dg6_depthScore(int y) { int r = ((y >> 4) - DG6_TOP) / 48; return 200 + 100 * v3_clamp(r, 0, 3); }

static void dg6_addPop(int x, int y, int v) {
  int k = 0; for (int i = 1; i < 4; i++) if (dg6_popT[i] < dg6_popT[k]) k = i;
  dg6_popX[k] = x; dg6_popY[k] = y; dg6_popV[k] = v; dg6_popT[k] = 70;
  dg6_score += v;
}

// dirt patterns: levels for a 64 × 16 tile of each stratum
static void dg6_makePattern(int type) {
  dg6_patType = type;
  for (int s = 0; s < 4; s++) for (int y = 0; y < 16; y++) for (int x = 0; x < 64; x++) {
    uint32_t h = v3_hash((uint32_t)(x * 7 + y * 131 + s * 977));
    int lev = 8;
    switch (type) {
      case 0: lev = (h & 15) == 0 ? 12 : ((h & 15) == 1 ? 4 : 8); if (((x + y * 3) & 7) == 0 && (h & 32)) lev = 11; break;
      case 1: { int w = (v3_sin(x * 16 + s * 100) >> 12); lev = ((y + w + 16) & 3) == 0 ? 11 : 7; if ((h & 31) == 0) lev = 4; break; }
      case 2: lev = (y % 8 == 7 || ((x + ((y / 8) & 1) * 8) % 16) == 15) ? 4 : ((h & 7) == 0 ? 10 : 9); break;
      case 3: { uint32_t g = v3_hash((uint32_t)((x >> 2) + (y >> 2) * 97 + s * 31)); lev = (g & 3) == 0 ? ((x & 3) == 0 || (y & 3) == 0 ? 11 : 6) : 9; break; }
      default: lev = (((x >> 3) + (y >> 3)) & 1) ? 9 : 6; if ((h & 15) == 0) lev = 11; break;
    }
    dg6_pat[(s * 16 + y) * 64 + x] = (uint8_t)A5C(lev, 10 + s);
  }
}

static void dg6_carve(int tx, int ty, int dir, int len) {      // a tunnel from tile (tx, ty)
  for (int i = 0; i < len; i++) {
    int x = tx + dg6_dx[dir] * i, y = ty + dg6_dy[dir] * i;
    if (x < 0 || x > 19 || y < 1 || y > 12) break;
    dg6_digBox(x * DG6_G, DG6_Y0 + y * DG6_G);
  }
}
static void dg6_resetPositions() {
  dg6_px = 9 * DG6_G; dg6_py = DG6_Y0; dg6_pd = 1; dg6_hs = 0; dg6_ht = -1;
  for (int i = 0; i < dg6_ne; i++) if (dg6_em[i] != DG6_DEAD) { dg6_ex[i] = dg6_hx[i]; dg6_ey[i] = dg6_hy[i]; dg6_em[i] = DG6_WALK; dg6_ei[i] = 0; dg6_gt[i] = 0; dg6_etm[i] = (int)((200 + v3_rnd(300)) / dg6_ghostK); }
}
static void dg6_newRound() {
  memset(dg6_dug, 0, DG6_CW * DG6_CH);
  dg6_carve(9, 1, 1, 5);                                        // the start shaft
  int n = v3_clamp(dg6_crit + (dg6_round - 1) / 2, 1, DG6_NE);
  dg6_ne = n;
  for (int i = 0; i < n; i++) {
    int tx, ty, dir, len, tries = 0;
    do { tx = v3_rnd(18) + 1; ty = 3 + v3_rnd(9); tries++; } while (tries < 30 && (v3_abs(tx - 9) < 3 && ty < 8));
    dir = v3_rnd(2) ? 0 : 1; len = 3 + v3_rnd(2);
    if (dir == 0 && tx + len > 19) tx = 19 - len;
    if (dir == 1 && ty + len > 12) ty = 12 - len;
    dg6_carve(tx, ty, dir, len);
    dg6_hx[i] = (tx + dg6_dx[dir] * (len - 1)) * DG6_G; dg6_hy[i] = DG6_Y0 + (ty + dg6_dy[dir] * (len - 1)) * DG6_G;
    dg6_et[i] = i < dg6_spit ? 1 : 0;
    dg6_em[i] = DG6_WALK; dg6_ed[i] = (dir + 2) & 3;
  }
  dg6_nr = 0;
  int nr = v3_clamp(dg6_rocks, 0, DG6_NR);
  for (int k = 0, tries = 0; k < nr && tries < 200; tries++) {
    int tx = v3_rnd(20), ty = 2 + v3_rnd(8);
    int x = tx * DG6_G, y = DG6_Y0 + ty * DG6_G;
    if (dg6_boxOpen(x, y) || dg6_boxOpen(x, y + DG6_G) || dg6_rockAt(x, y) || tx == 9) continue;
    bool nearE = false; for (int i = 0; i < n; i++) if (v3_abs(dg6_hx[i] - x) < 2 * DG6_G && v3_abs(dg6_hy[i] - y) < 2 * DG6_G) nearE = true;
    if (nearE) continue;
    dg6_rx[dg6_nr] = x; dg6_ry[dg6_nr] = y; dg6_rs[dg6_nr] = 0; dg6_rt[dg6_nr] = 0; dg6_nr++; k++;
  }
  dg6_resetPositions();
  dg6_banner = 120;
  a5_partClear();
}
static void dg6_newGame() { dg6_score = 0; dg6_lives = 3; dg6_round = 1; dg6_over = 0; dg6_dying = 0; dg6_newRound(); }

// move a box one step of `spd` (1/16 px) in direction d, stopping at the next grid node if `stopAtNode`
static void dg6_step(int* x, int* y, int d, int spd, bool stopAtNode) {
  int nx = *x + dg6_dx[d] * spd, ny = *y + dg6_dy[d] * spd;
  if (stopAtNode) {
    if (d == 0) { int b = (*x / DG6_G + 1) * DG6_G; if (nx > b) nx = b; }
    if (d == 2) { int b = ((*x + DG6_G - 1) / DG6_G - 1) * DG6_G; if (*x % DG6_G == 0) b = *x - DG6_G; if (nx < b) nx = b; }
    if (d == 1) { int r = *y - DG6_Y0; int b = (r / DG6_G + 1) * DG6_G + DG6_Y0; if (ny > b) ny = b; }
    if (d == 3) { int r = *y - DG6_Y0; int b = (r % DG6_G == 0) ? *y - DG6_G : (r / DG6_G) * DG6_G + DG6_Y0; if (ny < b) ny = b; }
  }
  *x = v3_clamp(nx, 0, 19 * DG6_G); *y = v3_clamp(ny, DG6_Y0, DG6_Y0 + 12 * DG6_G);
}
// the miner: try to go `want` (−1 = stand still), turning only on grid lines
static void dg6_movePlayer(int want) {
  if (want < 0) return;
  int d = want;
  bool perp = (want & 1) != (dg6_pd & 1);
  if (perp) {
    bool ok = (want & 1) ? dg6_alignedX(dg6_px) : dg6_alignedY(dg6_py);
    if (!ok) d = dg6_pd;
  }
  bool digging = false;
  { int tx = dg6_px + dg6_dx[d] * 4 * 16, ty = dg6_py + dg6_dy[d] * 4 * 16; if (!dg6_boxOpen(tx, ty)) digging = true; }
  int spd = digging ? 11 : 16;
  int ox = dg6_px, oy = dg6_py;
  dg6_step(&dg6_px, &dg6_py, d, spd, perp);
  if (dg6_rockAt(dg6_px, dg6_py)) { dg6_px = ox; dg6_py = oy; }
  dg6_pd = d;
  if (dg6_px != ox || dg6_py != oy) { dg6_anim++; dg6_digBox(dg6_px, dg6_py); }
}

static void dg6_kill() {
  if (dg6_dying) return;
  dg6_dying = 110; dg6_hs = 0;
  if (dg6_ht >= 0 && dg6_em[dg6_ht] == DG6_CAUGHT) dg6_ei[dg6_ht] = v3_clamp(dg6_ei[dg6_ht], 0, 9);
  dg6_ht = -1;
  a5_shake(4);
}

// ── critters ──
static bool dg6_canGo(int i, int d) {
  int x = dg6_ex[i] + dg6_dx[d] * DG6_G, y = dg6_ey[i] + dg6_dy[d] * DG6_G;
  return dg6_boxOpen(x, y) && !dg6_rockAt(x, y);
}
static void dg6_critters() {
  int espd = 8 + (int)(dg6_diff * 8) + v3_clamp(dg6_round - 1, 0, 6);
  for (int i = 0; i < dg6_ne; i++) {
    int m = dg6_em[i];
    if (m == DG6_DEAD) continue;
    if (m == DG6_POP) { if (--dg6_etm[i] <= 0) dg6_em[i] = DG6_DEAD; continue; }
    if (m == DG6_CAUGHT) {
      if (dg6_ht != i || dg6_hs != 2) { if (--dg6_etm[i] <= 0) { dg6_ei[i]--; dg6_etm[i] = 40; if (dg6_ei[i] <= 0) { dg6_em[i] = DG6_WALK; dg6_ei[i] = 0; } } }
      continue;
    }
    if (m == DG6_FIRE) {                                          // windup, then a long breath of flame
      if (--dg6_etm[i] <= 0) { dg6_em[i] = DG6_WALK; dg6_etm[i] = 200 + v3_rnd(200); }
      continue;
    }
    if (m == DG6_GHOST) {
      dg6_gt[i]++;
      int gs = 6 + (int)(dg6_diff * 4);
      int tx = dg6_px, ty = dg6_py;
      if (dg6_gt[i] > 150) {                                      // time to come out: head for the nearest open spot not right on the miner
        int bestD = 1 << 30; tx = dg6_ex[i]; ty = dg6_ey[i];
        for (int r = 0; r <= 12; r++) for (int c = 0; c < 20; c++) {
          int nx = c * DG6_G, ny = DG6_Y0 + r * DG6_G;
          if (v3_abs(nx - dg6_px) + v3_abs(ny - dg6_py) < 40 * 16) continue;
          if (!dg6_boxOpen(nx, ny) || dg6_rockAt(nx, ny)) continue;
          int d = v3_abs(nx - dg6_ex[i]) + v3_abs(ny - dg6_ey[i]) + (v3_abs(nx - dg6_px) + v3_abs(ny - dg6_py)) / 3;
          if (d < bestD) { bestD = d; tx = nx; ty = ny; }
        }
        int ddx = tx - dg6_ex[i], ddy = ty - dg6_ey[i];
        if (v3_abs(ddx) <= gs && v3_abs(ddy) <= gs) { dg6_ex[i] = tx; dg6_ey[i] = ty; dg6_em[i] = DG6_WALK; dg6_etm[i] = (int)((250 + v3_rnd(400)) / dg6_ghostK); continue; }
      }
      int ddx = tx - dg6_ex[i], ddy = ty - dg6_ey[i];
      int ad = v3_abs(ddx) + v3_abs(ddy) + 1;
      if (dg6_gt[i] <= 150 && ad < 24 * 16) { int t = ddx; ddx = -ddy; ddy = t; }   // circle round the miner rather than sit on it
      int mx = ddx * gs / ad, my = ddy * gs / ad;
      if (ad < gs) { mx = ddx; my = ddy; }
      dg6_ex[i] += mx; dg6_ey[i] += my;
      continue;
    }
    // walking the tunnels
    if (--dg6_etm[i] <= 0) { dg6_em[i] = DG6_GHOST; dg6_gt[i] = 0; continue; }
    if (dg6_alignedX(dg6_ex[i]) && dg6_alignedY(dg6_ey[i])) {
      int best = -1, bs = -(1 << 30), n = 0;
      for (int d = 0; d < 4; d++) {
        if (!dg6_canGo(i, d)) continue;
        n++;
        int s = v3_rnd(100);
        if (d == ((dg6_ed[i] + 2) & 3)) s -= 300;                   // don't turn back unless it's the only way
        int tox = (dg6_px - dg6_ex[i]) * dg6_dx[d] + (dg6_py - dg6_ey[i]) * dg6_dy[d];
        if (v3_rnd(100) < 55 + (int)(dg6_diff * 35)) s += tox > 0 ? 120 : 0;
        if (s > bs) { bs = s; best = d; }
      }
      if (best < 0) { if (dg6_etm[i] > 60) dg6_etm[i] = 60; continue; }   // boxed in: drift soon
      dg6_ed[i] = best;
      // salamanders breathe fire along a row
      if (dg6_et[i] == 1 && v3_abs(dg6_py - dg6_ey[i]) < 64 && v3_abs(dg6_px - dg6_ex[i]) < 5 * DG6_G && v3_rnd(100) < 30 + (int)(dg6_diff * 40)) {
        dg6_ed[i] = dg6_px > dg6_ex[i] ? 0 : 2; dg6_em[i] = DG6_FIRE; dg6_etm[i] = 100; continue;
      }
    }
    int ox = dg6_ex[i], oy = dg6_ey[i];
    dg6_step(&dg6_ex[i], &dg6_ey[i], dg6_ed[i], espd, true);
    if (!dg6_boxOpen(dg6_ex[i], dg6_ey[i])) { dg6_ex[i] = ox; dg6_ey[i] = oy; dg6_ed[i] = (dg6_ed[i] + 2) & 3; }
  }
}
// where a salamander's flame reaches: returns length in px (0 = none)
static int dg6_flameLen(int i) {
  if (dg6_em[i] != DG6_FIRE || dg6_etm[i] > 55) return 0;
  int t = 55 - dg6_etm[i];
  return v3_clamp(t * 4, 0, 52);
}

// ── the boulders ──
static void dg6_boulders() {
  for (int k = 0; k < dg6_nr; k++) {
    int x = dg6_rx[k], y = dg6_ry[k];
    if (dg6_rs[k] == 0) {
      int py = (y >> 4) + 16, r = (py - DG6_TOP) >> 2, c0 = (x >> 4) >> 2;
      bool open = r < DG6_CH;
      for (int c = c0; c < c0 + 4 && open; c++) if (!dg6_cellOpen(c, r)) open = false;
      if (open) { dg6_rs[k] = 1; dg6_rt[k] = 50; }
    } else if (dg6_rs[k] == 1) {
      if (--dg6_rt[k] <= 0) dg6_rs[k] = 2;
    } else if (dg6_rs[k] == 2) {
      dg6_ry[k] += 36;
      int py = (dg6_ry[k] >> 4) + 16, r = (py - DG6_TOP) >> 2, c0 = (x >> 4) >> 2;
      bool open = r < DG6_CH;
      for (int c = c0; c < c0 + 4 && open; c++) if (!dg6_cellOpen(c, r)) open = false;
      for (int i = 0; i < dg6_ne; i++) {
        if (dg6_em[i] == DG6_DEAD || dg6_em[i] == DG6_POP || dg6_em[i] == DG6_GHOST) continue;
        if (v3_abs(dg6_ex[i] - x) < 12 * 16 && v3_abs(dg6_ey[i] - dg6_ry[k]) < 12 * 16) {
          dg6_em[i] = DG6_POP; dg6_etm[i] = 24; dg6_addPop(dg6_ex[i] >> 4, dg6_ey[i] >> 4, 1000);
          a5_burst((dg6_ex[i] >> 4) + 8, (dg6_ey[i] >> 4) + 8, 24, dg6_et[i] ? 7 : 5, 40, 30);
          if (dg6_ht == i) { dg6_hs = 0; dg6_ht = -1; }
        }
      }
      if (!dg6_dying && v3_abs(dg6_px - x) < 12 * 16 && v3_abs(dg6_py - dg6_ry[k]) < 12 * 16) dg6_kill();
      if (!open) { dg6_rs[k] = 3; dg6_rt[k] = 30; a5_shake(3); a5_burst((x >> 4) + 8, (dg6_ry[k] >> 4) + 14, 20, 9, 30, 26); }
    } else if (dg6_rs[k] == 3) {
      if (--dg6_rt[k] <= 0) dg6_rs[k] = 4;
    }
  }
}

// ── the hose ──
static void dg6_hose(bool fire, bool held) {
  if (dg6_dying) return;
  if (dg6_hs == 0 && fire) { dg6_hs = 1; dg6_hl = 0; dg6_hd = dg6_pd; }
  if (dg6_hs == 1) {
    dg6_hl += 4;
    int cx = (dg6_px >> 4) + 8 + dg6_dx[dg6_hd] * (6 + dg6_hl), cy = (dg6_py >> 4) + 8 + dg6_dy[dg6_hd] * (6 + dg6_hl);
    if (cy >= DG6_TOP && !dg6_cellOpen(cx >> 2, (cy - DG6_TOP) >> 2)) { dg6_hs = 3; return; }
    for (int i = 0; i < dg6_ne; i++) {
      int m = dg6_em[i];
      if (m != DG6_WALK && m != DG6_FIRE && m != DG6_CAUGHT) continue;
      int ecx = (dg6_ex[i] >> 4) + 8, ecy = (dg6_ey[i] >> 4) + 8, pcx = (dg6_px >> 4) + 8, pcy = (dg6_py >> 4) + 8;
      bool hit;
      if (dg6_hd & 1) hit = v3_abs(ecx - cx) < 9 && ecy >= a5_min(pcy, cy) - 8 && ecy <= a5_max(pcy, cy) + 8;
      else hit = v3_abs(ecy - cy) < 9 && ecx >= a5_min(pcx, cx) - 8 && ecx <= a5_max(pcx, cx) + 8;
      if (hit) {
        dg6_hs = 2; dg6_ht = i; dg6_em[i] = DG6_CAUGHT; if (dg6_ei[i] < 1) dg6_ei[i] = 1; dg6_etm[i] = 40; dg6_pumpT = 12; return;
      }
    }
    if (dg6_hl >= 56) dg6_hs = 3;
  } else if (dg6_hs == 2) {
    int i = dg6_ht;
    if (i < 0 || dg6_em[i] != DG6_CAUGHT) { dg6_hs = 0; dg6_ht = -1; return; }
    dg6_hl = v3_clamp((v3_abs(dg6_ex[i] - dg6_px) + v3_abs(dg6_ey[i] - dg6_py)) / 16 - 6, 4, 60);
    if (dg6_pumpT > 0) dg6_pumpT--;
    if (fire || (held && dg6_pumpT == 0)) {
      dg6_pumpT = 14; dg6_ei[i]++; dg6_etm[i] = 50;
      if (dg6_ei[i] > dg6_pumps) {
        dg6_em[i] = DG6_POP; dg6_etm[i] = 24;
        int v = dg6_depthScore(dg6_ey[i]) * (dg6_et[i] && dg6_hd % 2 == 0 ? 2 : 1);
        dg6_addPop(dg6_ex[i] >> 4, dg6_ey[i] >> 4, v);
        a5_burst((dg6_ex[i] >> 4) + 8, (dg6_ey[i] >> 4) + 8, 30, dg6_et[i] ? 7 : 5, 50, 34);
        a5_shake(2.5f);
        dg6_hs = 0; dg6_ht = -1;
      }
    }
  } else if (dg6_hs == 3) {
    dg6_hl -= 8; if (dg6_hl <= 0) dg6_hs = 0;
  }
}

// ── the computer miner ──
// is the straight run of tunnel between two centre points (px) open?
static bool dg6_clear(int x0, int y0, int x1, int y1) {
  int n = (v3_abs(x1 - x0) + v3_abs(y1 - y0)) / 3 + 1;
  for (int k = 1; k < n; k++) {
    int x = x0 + (x1 - x0) * k / n, y = y0 + (y1 - y0) * k / n;
    if (y >= DG6_TOP && !dg6_cellOpen(x >> 2, (y - DG6_TOP) >> 2)) return false;
  }
  return true;
}
static inline bool dg6_active(int i) { int m = dg6_em[i]; return m == DG6_WALK || m == DG6_FIRE || m == DG6_GHOST; }
// can the miner start moving in d right now (turns only on grid lines)?
static bool dg6_canTurn(int d) {
  if ((d & 1) == (dg6_pd & 1)) return true;
  return (d & 1) ? dg6_alignedX(dg6_px) : dg6_alignedY(dg6_py);
}
static int dg6_ai(bool* fire) {
  *fire = false;
  int pcx = (dg6_px >> 4) + 8, pcy = (dg6_py >> 4) + 8;
  bool flee = false;
  // 1) threats: shoot what's lined up, run from what's close or about to breathe fire
  int shootDir = -1, shootDist = 999;
  for (int i = 0; i < dg6_ne; i++) {
    if (!dg6_active(i) && !(dg6_em[i] == DG6_CAUGHT && i != dg6_ht)) continue;
    int ecx = (dg6_ex[i] >> 4) + 8, ecy = (dg6_ey[i] >> 4) + 8, dx = ecx - pcx, dy = ecy - pcy;
    int dist = v3_abs(dx) + v3_abs(dy);
    bool ghost = dg6_em[i] == DG6_GHOST;
    if (!ghost) {
      if (dist < shootDist && ((v3_abs(dy) < 5 && v3_abs(dx) < 60) || (v3_abs(dx) < 5 && v3_abs(dy) < 60)) && dg6_clear(pcx, pcy, ecx, ecy)) {
        int dh = dx > 0 ? 0 : 2, dv = dy > 0 ? 1 : 3, pick = -1;
        bool okH = v3_abs(dy) < 5, okV = v3_abs(dx) < 5;
        if (dist < 9) pick = dg6_pd;                                   // right on top of us: the hose catches it anyway
        else if (okH && dg6_pd == dh) pick = dh; else if (okV && dg6_pd == dv) pick = dv;
        else if (okH && dg6_canTurn(dh)) pick = dh; else if (okV && dg6_canTurn(dv)) pick = dv;
        if (pick >= 0) { shootDist = dist; shootDir = pick; }
      }
      if (dg6_et[i] == 1 && v3_abs(dy) < 14 && v3_abs(dx) < 100 && shootDir < 0) flee = true;   // a salamander's row
    }
    if (!ghost && dist < 38 && dg6_em[i] != DG6_CAUGHT) flee = true;
  }
  if (dg6_hs == 2) {                                            // pumping: keep at it unless something else is close
    if (flee && shootDir < 0) { dg6_hs = 0; dg6_ht = -1; }
    else { if (++dg6_aiPump >= 10) { dg6_aiPump = 0; *fire = true; } return -1; }
  }
  if (dg6_hs == 1) return -1;
  if (shootDir >= 0) {
    if (dg6_pd == shootDir) { *fire = true; return -1; }
    if (dg6_canTurn(shootDir)) return shootDir;
  }
  // 2) a boulder coming down overhead: step aside
  for (int k = 0; k < dg6_nr; k++) if ((dg6_rs[k] == 1 || dg6_rs[k] == 2) && v3_abs((dg6_rx[k] >> 4) + 8 - pcx) < 18 && (dg6_ry[k] >> 4) < pcy) {
    int d = pcx > 150 ? 2 : 0;
    return dg6_canTurn(d) ? d : dg6_pd;
  }
  if (flee) {
    int best = -1, bs = -(1 << 30);
    for (int d = 0; d < 4; d++) {
      if (!dg6_canTurn(d)) continue;
      int nx = pcx + dg6_dx[d] * 14, ny = pcy + dg6_dy[d] * 14;
      if (nx < 8 || nx > W - 8 || ny < 32 || ny > 224) continue;
      int s = 2000;
      for (int i = 0; i < dg6_ne; i++) if (dg6_active(i)) {
        int ecx = (dg6_ex[i] >> 4) + 8, ecy = (dg6_ey[i] >> 4) + 8;
        int dd = v3_abs(ecx - nx) + v3_abs(ecy - ny);
        if (dg6_et[i] == 1 && v3_abs(ecy - ny) < 14) dd -= 40;          // out of the flame's row
        if (dd < s) s = dd;
      }
      if (d == dg6_pd) s += 3;
      if (s > bs) { bs = s; best = d; }
    }
    if (best >= 0) return best;
  }
  if (dg6_aiWander > 0) { dg6_aiWander--; return dg6_canTurn(dg6_aiWdir) ? dg6_aiWdir : dg6_pd; }
  if (v3_rnd(1200) == 0) { dg6_aiWander = 30 + v3_rnd(40); dg6_aiWdir = v3_rnd(4); }
  // 3) go after the nearest critter (solid ones first), lining up a row or column with it
  int best = -1, bd = 1 << 30;
  for (int i = 0; i < dg6_ne; i++) {
    int m = dg6_em[i]; if (m == DG6_DEAD || m == DG6_POP) continue;
    int d = v3_abs((dg6_ex[i] >> 4) + 8 - pcx) + v3_abs((dg6_ey[i] >> 4) + 8 - pcy) + (m == DG6_GHOST ? 150 : 0) + (dg6_et[i] ? 40 : 0);
    if (d < bd) { bd = d; best = i; }
  }
  if (best < 0) return -1;
  int tx = (dg6_ex[best] >> 4) + 8, ty = (dg6_ey[best] >> 4) + 8, dx = tx - pcx, dy = ty - pcy;
  if (dg6_em[best] == DG6_GHOST && v3_abs(dx) + v3_abs(dy) < 70) return -1;   // let it come out of the dirt
  int want;
  if (dg6_et[best] == 1) want = v3_abs(dx) > 4 ? (dx > 0 ? 0 : 2) : (dy > 0 ? 1 : 3);   // salamanders: line up a column, not a row
  else want = v3_abs(dy) > 4 ? (dy > 0 ? 1 : 3) : (dx > 0 ? 0 : 2);
  if (v3_abs(dx) + v3_abs(dy) < 44) return -1;                   // close enough: wait for a clean shot
  return dg6_canTurn(want) ? want : dg6_pd;
}

void prog_digger_init() {
  v3_reset(); a5_reset();
  dg6_dug = v3_scratch; dg6_pat = v3_scratch + DG6_CW * DG6_CH;
  dg6_patType = -1; dg6_preset = -1; dg6_ne = 0; dg6_banner = 0; dg6_over = 0; dg6_dying = 0;
}

static void digger5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != dg6_preset) { dg6_preset = preset; dg6_look = preset == 11 ? 0 : preset; dg6_patType = -1; dg6_hi = dg6_hi < 10000 ? 10000 : dg6_hi; }
  const Dg6Pre& P = dg6_pre[dg6_look];
  // knobs
  dg6_diff = a5_diffK(P.diff / 100.0f);
  dg6_crit = v3_centre(7) ? P.crit : 1 + (int)(v3_u(7) * 9.99f);
  dg6_spit = v3_centre(7) ? P.spit : dg6_crit / 3;
  dg6_rocks = v3_centre(8) ? P.rocks : (int)(v3_u(8) * 8.99f);
  dg6_ghostK = v3_centre(9) ? 1.0f : 0.2f + v3_u(9) * 3.0f;
  int pat = v3_pick(10, P.pattern, 5);
  dg6_pumps = v3_centre(11) ? 4 : 1 + (int)(v3_u(11) * 5.99f);
  if (pat != dg6_patType) dg6_makePattern(pat);
  if (dg6_ne == 0 && dg6_over == 0 && dg6_banner == 0) dg6_newGame();

  // palette
  uint8_t hues[16][3];
  memcpy(hues, dg6_hue, sizeof(hues));
  for (int s = 0; s < 4; s++) memcpy(hues[10 + s], P.strata[s], 3);
  a5_palette(hues, 16, a5_lookK(P.look));

  // ── game steps ──
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    if (dg6_banner > 0) { dg6_banner--; if (dg6_banner > 60) continue; }
    if (dg6_over > 0) {
      if (--dg6_over == 0) { if (dg6_preset == 11) dg6_look = (dg6_look + 1) % 11; dg6_newGame(); }
      a5_partStep(); continue;
    }
    if (dg6_dying > 0) {
      if (--dg6_dying == 0) {
        if (--dg6_lives < 0) { dg6_over = 220; if (dg6_score > dg6_hi) dg6_hi = dg6_score; }
        else { dg6_resetPositions(); dg6_banner = 90; }
      }
      a5_partStep(); continue;
    }
    bool fire = false;
    int want;
    if (a5_player) {
      int d4 = a5_dir4(); want = d4 ? d4 - 1 : -1;
      fire = a5_fire;
      if (want >= 0 && dg6_hs == 2) { dg6_hs = 0; dg6_ht = -1; }
    } else {
      want = dg6_ai(&fire);
    }
    if (dg6_hs == 0 || dg6_hs == 3) dg6_movePlayer(want);
    dg6_hose(fire, a5_player ? a5_fireHeld : false);
    // ghosting rate
    dg6_critters();
    dg6_boulders();
    // touching a critter or its flame
    for (int i = 0; i < dg6_ne; i++) {
      int m = dg6_em[i];
      if (m == DG6_WALK || m == DG6_FIRE) {
        if (v3_abs(dg6_ex[i] - dg6_px) < 11 * 16 && v3_abs(dg6_ey[i] - dg6_py) < 11 * 16) dg6_kill();
      }
      int fl = dg6_flameLen(i);
      if (fl) {
        int fx0 = (dg6_ex[i] >> 4) + (dg6_ed[i] == 0 ? 16 : -fl), fy = (dg6_ey[i] >> 4) + 8;
        int pcx = (dg6_px >> 4) + 8, pcy = (dg6_py >> 4) + 8;
        if (pcx >= fx0 - 4 && pcx <= fx0 + fl + 4 && v3_abs(pcy - fy) < 9) dg6_kill();
      }
    }
    for (int k = 0; k < 4; k++) if (dg6_popT[k] > 0) dg6_popT[k]--;
    a5_partStep();
    // round cleared?
    bool any = false; for (int i = 0; i < dg6_ne; i++) if (dg6_em[i] != DG6_DEAD) any = true;
    if (!any && !dg6_dying) { dg6_round++; dg6_score += 1000; if (dg6_preset == 11) dg6_look = (dg6_look + 1) % 11; dg6_newRound(); }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  // sky and surface
  a5_stars(30, -3, 0, 0, 77);
  for (int x = 0; x < W; x++) { int g = (v3_hash(x * 31) & 3); v3_put(x, DG6_TOP - 1, A5C(9 + g, 7)); if (g == 0) v3_put(x, DG6_TOP - 2, A5C(8, 7)); }
  // earth
  const uint8_t* pat0 = dg6_pat;
  for (int y = DG6_TOP; y < DG6_TOP + DG6_CH * 4; y++) {
    int r = (y - DG6_TOP) >> 2, s = v3_clamp((y - DG6_TOP) / 48, 0, 3);
    const uint8_t* dr = dg6_dug + r * DG6_CW;
    const uint8_t* pr = pat0 + (s * 16 + (y & 15)) * 64;
    uint8_t* d = v3_buf + y * W;
    for (int c = 0; c < DG6_CW; c++) if (!dr[c]) { int x = c * 4; d[x] = pr[x & 63]; d[x + 1] = pr[(x + 1) & 63]; d[x + 2] = pr[(x + 2) & 63]; d[x + 3] = pr[(x + 3) & 63]; }
  }
  // boulders
  for (int k = 0; k < dg6_nr; k++) if (dg6_rs[k] < 4) {
    int x = dg6_rx[k] >> 4, y = dg6_ry[k] >> 4;
    if (dg6_rs[k] == 1) x += ((dg6_rt[k] >> 2) & 1) ? 1 : -1;
    if (dg6_rs[k] == 3) { int t = dg6_rt[k]; for (int j = 0; j < 6; j++) v3_disc(x + 3 + j * 2, y + 14 - (30 - t) / 6, 2, A5C(9, 9)); }
    else a5_spr(dg6_sRock, 16, 16, x, y, dg6_cRock, 0, 0);
  }
  // hose
  if (dg6_hs) {
    int x0 = (dg6_px >> 4) + 8 + dg6_dx[dg6_hd] * 6, y0 = (dg6_py >> 4) + 8 + dg6_dy[dg6_hd] * 6;
    int x1 = x0 + dg6_dx[dg6_hd] * dg6_hl, y1 = y0 + dg6_dy[dg6_hd] * dg6_hl;
    a5_seg(x0, y0, x1, y1, A5C(15, 4));
    if (dg6_hd & 1) a5_seg(x0 + 1, y0, x1 + 1, y1, A5C(9, 4)); else a5_seg(x0, y0 + 1, x1, y1 + 1, A5C(9, 4));
    v3_rect(x1 - 1, y1 - 1, x1 + 1, y1 + 1, A5C(15, 1));
  }
  // critters
  int fr = ((int)(globalTime * 5)) & 1;
  for (int i = 0; i < dg6_ne; i++) {
    int m = dg6_em[i]; if (m == DG6_DEAD) continue;
    int x = dg6_ex[i] >> 4, y = dg6_ey[i] >> 4, fl = dg6_ed[i] == 2 ? A5_FX : 0;
    int hue = dg6_et[i] ? 7 : 5;
    if (m == DG6_GHOST) {                                          // just the eyes, drifting through the earth
      int bl = ((dg6_gt[i] >> 3) & 1) ? 15 : 11;
      v3_rect(x + 3, y + 6, x + 6, y + 9, A5C(bl, 0)); v3_rect(x + 9, y + 6, x + 12, y + 9, A5C(bl, 0));
      v3_rect(x + 5, y + 7, x + 6, y + 8, A5C(3, 15)); v3_rect(x + 11, y + 7, x + 12, y + 8, A5C(3, 15));
      continue;
    }
    if (m == DG6_CAUGHT || m == DG6_POP) {
      int st = m == DG6_POP ? dg6_pumps + 2 : dg6_ei[i];
      int r = 7 + st * 12 / v3_clamp(dg6_pumps, 1, 9);
      if (m == DG6_POP) { int t = 24 - dg6_etm[i]; if (t > 6) { a5_circle(x + 8, y + 8, r + t, A5C(15 - t / 3 > 0 ? 15 - t / 3 : 1, hue)); continue; } }
      v3_disc(x + 8, y + 8, r, A5C(13, hue)); v3_disc(x + 8 - r / 3, y + 8 - r / 3, r / 3, A5C(15, hue));
      v3_disc(x + 8, y + 8, r / 3 + 1, A5C(15, 0)); v3_disc(x + 8, y + 8, r / 6 + 1, A5C(2, 15));
      if (st >= dg6_pumps && fr) a5_circle(x + 8, y + 8, r, A5C(15, 0));
      continue;
    }
    if (dg6_et[i]) {
      bool windup = m == DG6_FIRE && dg6_etm[i] > 55;
      a5_spr(dg6_sSal[fr], 16, 16, x, y, dg6_cSal, fl, windup && (dg6_etm[i] & 4) ? A5C(15, 8) : 0);
      int len = dg6_flameLen(i);
      if (len) {
        int dir = dg6_ed[i] == 0 ? 1 : -1, x0 = dir > 0 ? x + 16 : x;
        for (int k = 0; k < len; k += 3) {
          int fx = x0 + dir * k, wob = (v3_hash(k * 7 + (uint32_t)(globalTime * 30)) & 3) - 1;
          int rr = 1 + k / 14;
          v3_disc(fx, y + 9 + wob, rr + 1, A5C(12, 8)); v3_disc(fx, y + 9 + wob, rr, A5C(15, 1));
        }
      }
    } else a5_spr(dg6_sPuff[fr], 16, 16, x, y, dg6_cPuff, fl, 0);
  }
  // the miner
  {
    int x = dg6_px >> 4, y = dg6_py >> 4, d = dg6_pd, f = 0;
    if (d == 2) f = A5_FX; else if (d == 1) f = A5_ROT; else if (d == 3) f = A5_ROT | A5_FX | A5_FY;
    if (d == 3) f = A5_ROT | A5_FY;
    int frame = (dg6_anim >> 3) & 1;
    if (dg6_dying) {
      int t = 110 - dg6_dying;
      if (t < 60) { static const int spin[4] = { 0, A5_ROT, A5_FX, A5_ROT | A5_FY }; a5_spr(dg6_sMiner[0], 16, 16, x, y, dg6_cMiner, spin[(t >> 3) & 3], 0); }
      else if (t < 90) { int hgt = 16 - (t - 60) / 2; v3_rect(x + 2, y + 16 - hgt / 2, x + 13, y + 15, A5C(14, 2)); }
    } else a5_spr(dg6_sMiner[frame], 16, 16, x, y, dg6_cMiner, f, 0);
  }
  a5_partDraw();
  for (int k = 0; k < 4; k++) if (dg6_popT[k] > 0) { char s[8]; snprintf(s, 8, "%d", dg6_popV[k]); a5_textS(v3_clamp(dg6_popX[k], 2, W - 30), dg6_popY[k] - (70 - dg6_popT[k]) / 6, s, 1, A5C(15, 0)); }

  a5_post((dg6_px >> 4) + 8, (dg6_py >> 4) + 8);
  // HUD: score, round flowers, lives
  a5_hud(dg6_score, dg6_hi, A5C(15, 14), A5C(15, 0));
  for (int i = 0; i < v3_clamp(dg6_round, 0, 8); i++) { int fx = W - 8 - i * 9; v3_rect(fx, 30, fx, 38, A5C(10, 7)); v3_disc(fx, 28, 3, A5C(15, 14)); v3_put(fx, 28, A5C(15, 1)); }
  for (int i = 0; i < v3_clamp(dg6_lives, 0, 5); i++) a5_spr(dg6_sMiner[0], 16, 16, 4 + i * 14, 14, dg6_cMiner, 0, 0);
  if (dg6_banner > 0) { char s[16]; snprintf(s, 16, "ROUND %d", dg6_round); a5_banner(H / 2 - 8, s, 2, A5C(15, 1)); }
  if (dg6_over > 0) a5_banner(H / 2 - 8, "GAME OVER", 2, A5C(15, 14));
  a5_crt();
}
void prog_digger_draw(int preset) { digger5_draw(preset); fx5_apply(); }

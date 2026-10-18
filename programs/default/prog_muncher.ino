// =====================================================================
// PROGRAM: MUNCHER — eat every dot in the maze, dodge the bloops
// (by Dewey) — Set 5 "Arcade", slot 10
//
// In the spirit of the 1980 maze-chase cabinet: CHOMP, a square little
// monster with a big toothy grin, munches the dots along the corridors of a
// maze. Four jellyfish BLOOPS hunt him — each its own way: one chases, one
// cuts him off ahead, one flanks, one is shy. Eat a big glowing pellet and
// they turn blue and run; catch them for 200, 400, 800, 1600 and their eyes
// swim home. Every maze is generated fresh (mirror-symmetric, with a tunnel
// through the sides), and each new one gets new colours. All characters
// and mazes are original.
//
// It plays itself; touch p0/p1 and you steer Chomp (4-way stick).
//
// Presets:
//   k0 Classic   k1 Neon Maze   k2 Candy   k3 Haunted (dark, short sight)
//   k4 Speedy   k5 Gem Mine   k6 Phosphor   k7 Ocean   k8 Lava   k9 Pastel
//   k10 Big Bloop Pack (6 bloops)   k11 Tour — every maze a new preset
//   k12 — joins the game.
//
// Knobs:
//   p0 Stick ◄ left | right ►   p1 Stick ◄ down | up ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Bloops (centre = preset; 1-6)   p8 Maze Style (centre = preset: loopy, twisty, open, grid)
//   p9 Fright Time (short … long)   p10 Wall Style (centre = preset: double line, solid, dots, glow)
//   p11 Spotlight (centre = off: only see round Chomp — or always on in Haunted)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 walls, 2 dots, 3 Chomp, 4 bloop A, 5 bloop B, 6 bloop C, 7 bloop D, 8 frightened, 9 HUD, 10 door, 11 gem, 12 bloop E, 13 bloop F, 14 eyes dark, 15 wall inner
struct Mu6Pre { uint8_t wall[3], dots[3], chomp[3]; uint8_t look, ghosts, style, wstyle, spot, speed, trails; };
static const Mu6Pre mu6_pre[11] = {
  {{40,60,255},{255,200,170},{90,220,80}, A5_ARCADE, 4, 0, 0, 0, 0, 0},
  {{255,0,200},{0,255,255},{255,255,0}, A5_NEON, 4, 0, 3, 0, 0, 120},
  {{255,150,200},{255,255,255},{120,220,255}, A5_ARCADE, 4, 2, 1, 0, 0, 0},
  {{90,70,120},{200,180,140},{120,220,90}, A5_ARCADE, 4, 1, 0, 1, 0, 0},
  {{40,200,120},{255,255,180},{255,140,40}, A5_ARCADE, 4, 0, 0, 0, 1, 60},
  {{120,120,140},{120,255,255},{255,200,60}, A5_ARCADE, 4, 1, 1, 0, 0, 0},
  {{255,255,255},{255,255,255},{255,255,255}, A5_PHOSPHOR, 4, 0, 0, 0, 0, 160},
  {{0,140,200},{220,250,255},{255,160,60}, A5_ARCADE, 4, 2, 3, 0, 0, 0},
  {{220,60,0},{255,230,120},{120,220,255}, A5_ARCADE, 4, 1, 1, 0, 0, 40},
  {{180,170,255},{255,220,240},{140,230,160}, A5_PASTEL, 4, 0, 0, 0, 0, 0},
  {{40,60,255},{255,200,170},{90,220,80}, A5_ARCADE, 6, 3, 2, 0, 0, 0},
};
static const char* const mu6_names[12] = { "Classic", "Neon Maze", "Candy", "Haunted", "Speedy", "Gem Mine", "Phosphor", "Ocean", "Lava", "Pastel", "Big Bloop Pack", "Tour" };
static const char* const mu6_lbl[16] = { "Stick: Left|Right", "Stick: Down|Up", "", "", "", "", "", "Bloops", "Maze Style", "Fright Time", "Wall Style", "Spotlight", "", "", "", "" };
const char* prog_muncher_name() { return "MUNCHER"; }
const char* prog_muncher_character() { return "Maze chase: eat the dots, turn the tables on the jellyfish bloops - plays itself until you touch p0/p1/k12"; }
const char* prog_muncher_presetName(int preset) { return (preset >= 0 && preset < 12) ? mu6_names[preset] : NULL; }
const char* prog_muncher_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, mu6_lbl); }
uint8_t prog_muncher_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define MU6_TW 39
#define MU6_TH 27
#define MU6_X0 4
#define MU6_Y0 14
#define MU6_NG 6
enum { MU6_WALL = 1, MU6_DOT = 2, MU6_PEL = 4, MU6_HOUSE = 8, MU6_DOOR = 16 };
static uint8_t mu6_t[MU6_TH][MU6_TW];
static uint16_t *mu6_dist, *mu6_q;      // BFS work space (scratch)
static const int8_t mu6_dx[4] = { 1, 0, -1, 0 }, mu6_dy[4] = { 0, 1, 0, -1 };
static const int mu6_hcx = 19, mu6_hcy = 13;   // the bloop house centre (tile)

static inline bool mu6_open(int x, int y) { if (y < 0 || y >= MU6_TH) return false; x = (x + MU6_TW) % MU6_TW; return !(mu6_t[y][x] & MU6_WALL); }
static inline bool mu6_walk(int x, int y, bool ghost) { if (!mu6_open(x, y)) return false; x = (x + MU6_TW) % MU6_TW; uint8_t v = mu6_t[y][x]; if (!ghost && (v & (MU6_DOOR | MU6_HOUSE))) return false; return true; }

// ── a mirror-symmetric maze with loops, a bloop house and side tunnels ──
static void mu6_makeMaze(int style) {
  memset(mu6_t, MU6_WALL, sizeof(mu6_t));
  const int CW = 10, CH = 13;                       // cells in the left half: cell (i, j) → tile (2i+1, 2j+1)
  uint8_t* vis = v3_scratch + 6600;                  // CW × CH
  memset(vis, 0, CW * CH);
  // depth-first carve
  int16_t* st = (int16_t*)(v3_scratch + 6800); int sp = 0;
  st[sp++] = 0; vis[0] = 1; mu6_t[1][1] = 0;
  while (sp) {
    int cur = st[sp - 1], ci = cur % CW, cj = cur / CW, opts[4], no = 0;
    for (int d = 0; d < 4; d++) { int ni = ci + mu6_dx[d], nj = cj + mu6_dy[d]; if (ni >= 0 && ni < CW && nj >= 0 && nj < CH && !vis[nj * CW + ni]) opts[no++] = d; }
    if (!no) { sp--; continue; }
    int d = opts[style == 1 ? (v3_rnd(3) == 0 ? v3_rnd(no) : 0) : v3_rnd(no)];
    int ni = ci + mu6_dx[d], nj = cj + mu6_dy[d];
    mu6_t[2 * cj + 1 + mu6_dy[d]][2 * ci + 1 + mu6_dx[d]] = 0; mu6_t[2 * nj + 1][2 * ni + 1] = 0;
    vis[nj * CW + ni] = 1; st[sp++] = (int16_t)(nj * CW + ni);
  }
  // loops: knock out extra walls (dead ends first)
  int extra = style == 2 ? 70 : (style == 1 ? 10 : 30);
  for (int pass = 0; pass < 2; pass++) for (int j = 0; j < CH; j++) for (int i = 0; i < CW; i++) {
    int x = 2 * i + 1, y = 2 * j + 1, n = 0;
    for (int d = 0; d < 4; d++) if (!(mu6_t[y + mu6_dy[d]][x + mu6_dx[d]] & MU6_WALL)) n++;
    if ((n <= 1 && pass == 0) || v3_rnd(100) < extra / 3) {
      int d = v3_rnd(4), wx = x + mu6_dx[d], wy = y + mu6_dy[d];
      if (wx > 0 && wx < 19 && wy > 0 && wy < MU6_TH - 1) mu6_t[wy][wx] = 0;
    }
  }
  if (style == 3) for (int y = 1; y < MU6_TH - 1; y++) for (int x = 1; x < 20; x++) if ((y % 4 == 1) || (x % 4 == 1)) mu6_t[y][x] = 0;   // grid style
  for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < 19; x++) mu6_t[y][MU6_TW - 1 - x] = mu6_t[y][x];
  // the house: a box with a door on top, a corridor round it
  for (int y = mu6_hcy - 3; y <= mu6_hcy + 3; y++) for (int x = mu6_hcx - 5; x <= mu6_hcx + 5; x++) mu6_t[y][x] = 0;
  for (int y = mu6_hcy - 2; y <= mu6_hcy + 2; y++) for (int x = mu6_hcx - 4; x <= mu6_hcx + 4; x++) mu6_t[y][x] = (y == mu6_hcy - 2 || y == mu6_hcy + 2 || x == mu6_hcx - 4 || x == mu6_hcx + 4) ? MU6_WALL : MU6_HOUSE;
  mu6_t[mu6_hcy - 2][mu6_hcx] = MU6_DOOR;
  // a tunnel through the sides
  for (int x = 0; x < 4; x++) { mu6_t[mu6_hcy][x] = 0; mu6_t[mu6_hcy][MU6_TW - 1 - x] = 0; }
  mu6_t[mu6_hcy - 1][0] = mu6_t[mu6_hcy + 1][0] = MU6_WALL; mu6_t[mu6_hcy - 1][MU6_TW - 1] = mu6_t[mu6_hcy + 1][MU6_TW - 1] = MU6_WALL;
  for (int x = 1; x < 4; x++) { mu6_t[mu6_hcy - 1][x] = MU6_WALL; mu6_t[mu6_hcy + 1][x] = MU6_WALL; mu6_t[mu6_hcy - 1][MU6_TW - 1 - x] = MU6_WALL; mu6_t[mu6_hcy + 1][MU6_TW - 1 - x] = MU6_WALL; }
  mu6_t[mu6_hcy][4] = 0; mu6_t[mu6_hcy][MU6_TW - 5] = 0;
  // dots everywhere open (not in the house or the tunnel ends), pellets near the corners
  for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < MU6_TW; x++) if (!mu6_t[y][x] && !(y == mu6_hcy && (x < 4 || x > MU6_TW - 5))) mu6_t[y][x] = MU6_DOT;
  const int px[4] = { 1, MU6_TW - 2, 1, MU6_TW - 2 }, py[4] = { 3, 3, MU6_TH - 4, MU6_TH - 4 };
  for (int k = 0; k < 4; k++) { int x = px[k], y = py[k]; while (mu6_t[y][x] & MU6_WALL) y += k < 2 ? 1 : -1; mu6_t[y][x] = MU6_PEL; }
  // the start: the corridor below the house
  mu6_t[mu6_hcy + 4][mu6_hcx] &= ~MU6_WALL;
}
// how far every tile is from (sx, sy) along open corridors; `avoid` tiles count as walls
static void mu6_bfs(int sx, int sy, bool ghost, const uint8_t* avoid) {
  for (int i = 0; i < MU6_TW * MU6_TH; i++) mu6_dist[i] = 0xFFFF;
  int h = 0, t = 0;
  mu6_dist[sy * MU6_TW + sx] = 0; mu6_q[t++] = (uint16_t)(sy * MU6_TW + sx);
  while (h < t) {
    int c = mu6_q[h++], x = c % MU6_TW, y = c / MU6_TW;
    for (int d = 0; d < 4; d++) {
      int nx = (x + mu6_dx[d] + MU6_TW) % MU6_TW, ny = y + mu6_dy[d];
      if (!mu6_walk(nx, ny, ghost)) continue;
      int n = ny * MU6_TW + nx;
      if (mu6_dist[n] != 0xFFFF) continue;
      if (avoid && avoid[n]) continue;
      mu6_dist[n] = (uint16_t)(mu6_dist[c] + 1); mu6_q[t++] = (uint16_t)n;
    }
  }
}

// ── the cast: [0] Chomp, [1…] bloops; positions in 1/16 px of the tile grid ──
enum { MU6_IN, MU6_LEAVE, MU6_CHASE, MU6_FRIGHT, MU6_EYES };
static int mu6_x[MU6_NG + 1], mu6_y[MU6_NG + 1], mu6_d[MU6_NG + 1], mu6_m[MU6_NG + 1], mu6_tm[MU6_NG + 1];
static int mu6_want = 2, mu6_ng = 4, mu6_dots = 0, mu6_eatChain = 0, mu6_preset = -1, mu6_look = 0, mu6_level = 1, mu6_lives = 3, mu6_style = 0;
static float mu6_fright = 0, mu6_frightLen = 6, mu6_modeT = 0, mu6_dead = 0, mu6_clear = 0, mu6_banner = 0, mu6_over = 0, mu6_chompA = 0, mu6_gemT = 0;
static bool mu6_scatter = true, mu6_gem = false;
static uint32_t mu6_score = 0, mu6_hi = 10000;
static float mu6_diff = 0.4f;
static int mu6_popX = 0, mu6_popY = 0, mu6_popV = 0; static float mu6_popT = 0;
static uint8_t* mu6_avoid;
static int mu6_goal = -1;
static float mu6_stall = 0;

static inline int mu6_tx(int k) { return ((mu6_x[k] + 128) >> 8); }
static inline int mu6_ty(int k) { return ((mu6_y[k] + 128) >> 8); }
static inline bool mu6_centred(int k) { return (mu6_x[k] & 255) == 0 && (mu6_y[k] & 255) == 0; }
static void mu6_place(int k, int tx, int ty) { mu6_x[k] = tx << 8; mu6_y[k] = ty << 8; }
static void mu6_resetActors() {
  mu6_place(0, mu6_hcx, mu6_hcy + 4); mu6_d[0] = 2; mu6_want = 2;
  for (int g = 1; g <= mu6_ng; g++) {
    if (g == 1) { mu6_place(g, mu6_hcx, mu6_hcy - 3); mu6_m[g] = MU6_CHASE; }
    else { mu6_place(g, mu6_hcx - 3 + ((g - 2) % 3) * 3, mu6_hcy); mu6_m[g] = MU6_IN; }
    mu6_d[g] = 2; mu6_tm[g] = 60 + g * 180 - (int)(mu6_diff * 100);
  }
  mu6_fright = 0; mu6_modeT = 0; mu6_scatter = true;
}
static void mu6_newMaze() {
  mu6_makeMaze(mu6_style);
  mu6_bfs(mu6_hcx, mu6_hcy + 4, false, 0);                       // wall up any pocket Chomp can't reach
  for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < MU6_TW; x++) if (!(mu6_t[y][x] & (MU6_WALL | MU6_HOUSE | MU6_DOOR)) && mu6_dist[y * MU6_TW + x] == 0xFFFF) mu6_t[y][x] = MU6_WALL;
  mu6_dots = 0; for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < MU6_TW; x++) if (mu6_t[y][x] & (MU6_DOT | MU6_PEL)) mu6_dots++;
  mu6_resetActors(); mu6_banner = 1.6f; mu6_gem = false; mu6_gemT = 0;
}
static void mu6_newGame() { mu6_score = 0; mu6_level = 1; mu6_lives = 3; mu6_over = 0; mu6_dead = 0; mu6_clear = 0; mu6_newMaze(); }

// a bloop's target tile
static void mu6_target(int g, int* tx, int* ty) {
  int cx = mu6_tx(0), cy = mu6_ty(0);
  if (mu6_m[g] == MU6_EYES) { *tx = mu6_hcx; *ty = mu6_hcy - 3; return; }
  if (mu6_m[g] == MU6_LEAVE || mu6_m[g] == MU6_IN) { *tx = mu6_hcx; *ty = mu6_hcy - 3; return; }
  if (mu6_scatter) { const int sx[6] = { MU6_TW, 0, MU6_TW, 0, MU6_TW / 2, MU6_TW / 2 }, sy[6] = { -2, -2, MU6_TH + 1, MU6_TH + 1, -2, MU6_TH + 1 }; *tx = sx[(g - 1) % 6]; *ty = sy[(g - 1) % 6]; return; }
  switch ((g - 1) % 4) {
    case 0: *tx = cx; *ty = cy; break;                                          // chaser
    case 1: *tx = cx + mu6_dx[mu6_d[0]] * 4; *ty = cy + mu6_dy[mu6_d[0]] * 4; break;   // ambusher
    case 2: { int ax = cx + mu6_dx[mu6_d[0]] * 2, ay = cy + mu6_dy[mu6_d[0]] * 2; *tx = ax * 2 - mu6_tx(1); *ty = ay * 2 - mu6_ty(1); break; }   // flanker
    default: { int dx = mu6_tx(g) - cx, dy = mu6_ty(g) - cy; if (dx * dx + dy * dy > 64) { *tx = cx; *ty = cy; } else { *tx = 0; *ty = MU6_TH + 1; } break; }   // shy
  }
}
static void mu6_moveGhost(int g, int spd) {
  for (int s = 0; s < spd; s++) {
    if (mu6_centred(g)) {
      int x = mu6_tx(g), y = mu6_ty(g);
      if (mu6_m[g] == MU6_EYES && x == mu6_hcx && y == mu6_hcy - 3) { mu6_m[g] = MU6_LEAVE; mu6_place(g, mu6_hcx, mu6_hcy); mu6_tm[g] = 60; return; }
      if (mu6_m[g] == MU6_LEAVE && y == mu6_hcy - 3) mu6_m[g] = (mu6_fright > 0 ? MU6_FRIGHT : MU6_CHASE);
      int best = -1; int bd = 1 << 30, tx, ty; mu6_target(g, &tx, &ty);
      for (int k = 0; k < 4; k++) {
        int d = (k + 3) & 3;                                      // tie order: up, right, down, left
        if (d == ((mu6_d[g] + 2) & 3) && mu6_m[g] != MU6_LEAVE) continue;
        int nx = (x + mu6_dx[d] + MU6_TW) % MU6_TW, ny = y + mu6_dy[d];
        bool ok = mu6_open(nx, ny) && (!(mu6_t[ny][nx] & (MU6_DOOR | MU6_HOUSE)) || mu6_m[g] == MU6_EYES || mu6_m[g] == MU6_LEAVE);
        if (!ok) continue;
        int dist = (mu6_m[g] == MU6_FRIGHT) ? v3_rnd(1000) : (nx - tx) * (nx - tx) + (ny - ty) * (ny - ty);
        if (dist < bd) { bd = dist; best = d; }
      }
      if (best < 0) best = (mu6_d[g] + 2) & 3;
      mu6_d[g] = best;
    }
    mu6_x[g] += mu6_dx[mu6_d[g]]; mu6_y[g] += mu6_dy[mu6_d[g]];
    if (mu6_x[g] < -128) mu6_x[g] += MU6_TW << 8; if (mu6_x[g] > ((MU6_TW - 1) << 8) + 128) mu6_x[g] -= MU6_TW << 8;
  }
}
// the computer's Chomp: pick a goal (stick with it), then at each junction take the way that gets
// closest to it, unless a bloop is in that direction
static int mu6_ghostPen(int x, int y) {
  int p = 0;
  for (int g = 1; g <= mu6_ng; g++) {
    int m = mu6_m[g];
    if (!(m == MU6_CHASE || m == MU6_LEAVE || (m == MU6_FRIGHT && mu6_fright < 1.0f))) continue;
    int dx = v3_abs(mu6_tx(g) - x); if (dx > MU6_TW / 2) dx = MU6_TW - dx;
    int d = dx + v3_abs(mu6_ty(g) - y);
    if (d < 6) p += (6 - d) * (6 - d) * 12;
  }
  return p;
}
static uint16_t* mu6_gdist;          // how soon the nearest dangerous bloop could be at each tile
static void mu6_ghostField() {
  for (int i = 0; i < MU6_TW * MU6_TH; i++) mu6_gdist[i] = 0xFFFF;
  int h = 0, t = 0;
  for (int g = 1; g <= mu6_ng; g++) {
    int m = mu6_m[g];
    if (!(m == MU6_CHASE || m == MU6_LEAVE || (m == MU6_FRIGHT && mu6_fright < 1.2f))) continue;
    int n = mu6_ty(g) * MU6_TW + ((mu6_tx(g) + MU6_TW) % MU6_TW);
    if (n < 0 || n >= MU6_TW * MU6_TH || mu6_gdist[n] == 0) continue;
    mu6_gdist[n] = 0; mu6_q[t++] = (uint16_t)n;
  }
  while (h < t) {
    int c = mu6_q[h++], x = c % MU6_TW, y = c / MU6_TW;
    for (int d = 0; d < 4; d++) {
      int nx = (x + mu6_dx[d] + MU6_TW) % MU6_TW, ny = y + mu6_dy[d];
      if (!mu6_open(nx, ny)) continue;
      int n = ny * MU6_TW + nx;
      if (mu6_gdist[n] != 0xFFFF) continue;
      mu6_gdist[n] = (uint16_t)(mu6_gdist[c] + 1); mu6_q[t++] = (uint16_t)n;
    }
  }
}
// follow a corridor from (x, y) heading d: how safe is it? (the smallest lead we keep over the bloops; negative = caught)
static int mu6_safety(int x, int y, int d) {
  int lead = 99;
  for (int k = 1; k <= 14; k++) {
    x = (x + mu6_dx[d] + MU6_TW) % MU6_TW; y += mu6_dy[d];
    int n = y * MU6_TW + x, gd = mu6_gdist[n] == 0xFFFF ? 99 : mu6_gdist[n];
    int l = gd * 9 / 10 - k; if (l < lead) lead = l;
    // at a junction (more than two ways out) we can turn off: stop looking
    int ways = 0, nd = -1;
    for (int e = 0; e < 4; e++) if (e != ((d + 2) & 3) && mu6_walk((x + mu6_dx[e] + MU6_TW) % MU6_TW, y + mu6_dy[e], false)) { ways++; nd = e; }
    if (ways == 0) return lead - 6;                               // a dead end
    if (ways > 1) break;
    d = nd;
  }
  return lead;
}
static int mu6_ai() {
  int cx = (mu6_tx(0) + MU6_TW) % MU6_TW, cy = mu6_ty(0);
  mu6_ghostField();
  int here = mu6_gdist[cy * MU6_TW + cx];
  bool danger = here < 7;
  int goal = -1;
  if (!danger && mu6_goal >= 0 && (mu6_t[mu6_goal / MU6_TW][mu6_goal % MU6_TW] & (MU6_DOT | MU6_PEL))) goal = mu6_goal;
  if (mu6_stall > 6) {                                              // going round in circles: pick somewhere else entirely
    mu6_stall = 0; goal = -1;
    for (int tries = 0; tries < 200 && goal < 0; tries++) { int x = v3_rnd(MU6_TW), y = v3_rnd(MU6_TH); if (mu6_t[y][x] & (MU6_DOT | MU6_PEL)) goal = y * MU6_TW + x; }
    mu6_goal = goal;
  }
  if (goal < 0) {
    mu6_bfs(cx, cy, false, 0);
    int gd = 1 << 30;
    if (mu6_fright > 1.5f) for (int g = 1; g <= mu6_ng; g++) if (mu6_m[g] == MU6_FRIGHT) { int n = mu6_ty(g) * MU6_TW + ((mu6_tx(g) + MU6_TW) % MU6_TW); if (n >= 0 && n < MU6_TW * MU6_TH && mu6_dist[n] < 12 && mu6_dist[n] < gd) { gd = mu6_dist[n]; goal = n; } }
    if (goal < 0) for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < MU6_TW; x++) {
      uint8_t v = mu6_t[y][x]; if (!(v & (MU6_DOT | MU6_PEL))) continue;
      int n = y * MU6_TW + x, d = mu6_dist[n]; if (d == 0xFFFF) continue;
      int g = mu6_gdist[n] == 0xFFFF ? 99 : mu6_gdist[n];
      d = d * 4 + (g < 8 ? (8 - g) * 12 : 0);
      if (v & MU6_PEL) d = danger ? d / 8 : d + 160;
      if (d < gd) { gd = d; goal = n; }
    }
    mu6_goal = goal;
  }
  if (goal >= 0) mu6_bfs(goal % MU6_TW, goal / MU6_TW, false, 0);
  int best = -1, bc = 1 << 30, bestLead = 0, curLead = -99;
  bool curOk = false;
  for (int d = 0; d < 4; d++) {
    int nx = (cx + mu6_dx[d] + MU6_TW) % MU6_TW, ny = cy + mu6_dy[d];
    if (!mu6_walk(nx, ny, false)) continue;
    int dist = goal >= 0 ? mu6_dist[ny * MU6_TW + nx] : 0; if (dist == 0xFFFF) dist = 500;
    int lead = mu6_safety(cx, cy, d);
    if (d == mu6_d[0]) { curOk = true; curLead = lead; }
    int c = dist * 10 + (lead < 2 ? (2 - lead) * 400 : 0) + (d == ((mu6_d[0] + 2) & 3) ? 60 : 0);
    if (c < bc) { bc = c; best = d; bestLead = lead; }
  }
  if (curOk && bestLead < 2 && curLead >= bestLead - 1) return mu6_d[0];   // no good way out: don't dither, keep going
  return best;
}
static void mu6_die() { if (mu6_dead > 0) return; mu6_dead = 2.5f; a5_shake(3); }

// ── sprites ──
static const char mu6_sBloop[2][197] = {           // 14 × 14 jellyfish
  "....444444...." "..4444444444.." ".444444444444." ".4411444114444" "441ff1441ff144" "441ff1441ff144" "44411444411444" "44444444444444" "44444444444444" "4.44.444.44.44" "4.4..4.4..4.4." ".4...4..4..4.." ".4..4...4...4." "..............",
  "....444444...." "..4444444444.." ".444444444444." ".4411444114444" "441ff1441ff144" "441ff1441ff144" "44411444411444" "44444444444444" "44444444444444" "44.44.444.44.4" ".4.4..4.4..4.4" "..4..4..4..4.." ".4..4..4...4.." ".............."
};
static const char mu6_sChomp[2][197] = {           // 14 × 14 Chomp: a square monster, eyes on top, a big grin
  ".333333333333." "33333333333333" "33ff33333ff333" "33fe33333fe333" "33333333333333" "33333333333333" "3eeeeeeeeeee33" "3e1e1e1e1e1e33" "3eeeeeeeeeee33" "3e1e1e1e1e1e33" "3eeeeeeeeeee33" "33333333333333" ".333333333333." "..33......33..",
  ".333333333333." "33333333333333" "33ff33333ff333" "33fe33333fe333" "33333333333333" "33333333333333" "33333333333333" "3e1e1e1e1e1e33" "3eeeeeeeeeee33" "33333333333333" "33333333333333" "33333333333333" ".333333333333." "..33......33.."
};
static const char mu6_sGem[101] = "....bb...." "...bbbb..." "..bbfbbb.." ".bbbbbbbb." "bbbbbbbbbb" ".bbbbbbbb." "..bbbbbb.." "...bbbb..." "....bb...." "..........";

void prog_muncher_init() { v3_reset(); a5_reset(); mu6_preset = -1; mu6_dist = (uint16_t*)v3_scratch; mu6_q = (uint16_t*)(v3_scratch + 2112); mu6_avoid = v3_scratch + 4224 + 1200; mu6_gdist = (uint16_t*)(v3_scratch + 8000); a5_partClear(); }

static void muncher5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != mu6_preset) { mu6_preset = preset; mu6_look = preset == 11 ? 0 : preset; mu6_style = mu6_pre[mu6_look].style; mu6_ng = mu6_pre[mu6_look].ghosts; mu6_newGame(); }
  const Mu6Pre& P = mu6_pre[mu6_look];
  mu6_diff = a5_diffK(0.4f);
  int ng = v3_centre(7) ? P.ghosts : 1 + (int)(v3_u(7) * 5.99f);
  if (ng != mu6_ng) { mu6_ng = ng; mu6_resetActors(); }
  int style = v3_centre(8) ? P.style : v3_clamp(v3_k(8) * 4 / 1024, 0, 3);
  if (style != mu6_style) { mu6_style = style; mu6_newMaze(); }
  mu6_frightLen = v3_centre(9) ? 6.0f - mu6_level * 0.5f : 1.0f + v3_u(9) * 12.0f;
  if (mu6_frightLen < 1.5f) mu6_frightLen = 1.5f;
  int wstyle = v3_pick(10, P.wstyle, 4);
  bool spot = v3_centre(11) ? P.spot : v3_k(11) > 600;

  // the maze colour changes every level
  uint8_t wall[3]; memcpy(wall, P.wall, 3);
  if (mu6_level > 1 && P.look != A5_PHOSPHOR) { uint8_t c[3]; v3_hsv((mu6_level * 330 + 900) % 1536, 220, 255, c); memcpy(wall, c, 3); }
  uint8_t hues[16][3] = { {255,255,255},{0},{0},{0},{255,40,60},{220,90,255},{40,230,230},{255,190,40},{40,60,255},{255,230,60},{255,170,200},{120,255,255},{120,255,90},{255,120,40},{10,10,40},{0} };
  memcpy(hues[1], wall, 3); memcpy(hues[2], P.dots, 3); memcpy(hues[3], P.chomp, 3);
  hues[15][0] = wall[0] / 4; hues[15][1] = wall[1] / 4; hues[15][2] = wall[2] / 4;
  a5_palette(hues, 16, a5_lookK(P.look));

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep();
    if (mu6_banner > 0) { mu6_banner -= dt; continue; }
    if (mu6_popT > 0) mu6_popT -= dt;
    if (mu6_over > 0) { mu6_over -= dt; if (mu6_over <= 0) { if (mu6_preset == 11) { mu6_look = (mu6_look + 1) % 11; mu6_style = mu6_pre[mu6_look].style; } mu6_newGame(); } continue; }
    if (mu6_clear > 0) { mu6_clear -= dt; if (mu6_clear <= 0) { mu6_level++; if (mu6_preset == 11) { mu6_look = (mu6_look + 1) % 11; mu6_style = mu6_pre[mu6_look].style; } mu6_newMaze(); } continue; }
    if (mu6_dead > 0) {
      mu6_dead -= dt;
      if (mu6_dead <= 0) { if (--mu6_lives < 0) { mu6_over = 4; if (mu6_score > mu6_hi) mu6_hi = mu6_score; } else { mu6_resetActors(); mu6_banner = 1.0f; } }
      continue;
    }
    // scatter / chase rhythm
    if (mu6_fright <= 0) { mu6_modeT += dt; float lim = mu6_scatter ? 7.0f - mu6_level * 0.5f : 20.0f; if (mu6_modeT > lim) { mu6_modeT = 0; mu6_scatter = !mu6_scatter; for (int g = 1; g <= mu6_ng; g++) if (mu6_m[g] == MU6_CHASE) mu6_d[g] = (mu6_d[g] + 2) & 3; } }
    else { mu6_fright -= dt; if (mu6_fright <= 0) for (int g = 1; g <= mu6_ng; g++) if (mu6_m[g] == MU6_FRIGHT) mu6_m[g] = MU6_CHASE; }
    // Chomp
    mu6_stall += dt;
    if (a5_player) { int d4 = a5_dir4(); if (d4) mu6_want = d4 - 1; if (a5_fire) mu6_want = (mu6_d[0] + 2) & 3; }
    int spd = 36 + P.speed * 10;
    for (int s = 0; s < spd; s++) {
      int x = mu6_tx(0), y = mu6_ty(0);
      if (mu6_centred(0)) {
        uint8_t& v = mu6_t[y][(x + MU6_TW) % MU6_TW];
        if (v & MU6_DOT) { v &= ~MU6_DOT; mu6_score += 10; mu6_dots--; mu6_stall = 0; }
        if (v & MU6_PEL) { v &= ~MU6_PEL; mu6_score += 50; mu6_dots--; mu6_fright = mu6_frightLen; mu6_eatChain = 0; for (int g = 1; g <= mu6_ng; g++) if (mu6_m[g] == MU6_CHASE) { mu6_m[g] = MU6_FRIGHT; mu6_d[g] = (mu6_d[g] + 2) & 3; } }
        if (!a5_player) { int d = mu6_ai(); if (d >= 0) mu6_want = d; }   // the computer decides at every tile
        if (mu6_walk(x + mu6_dx[mu6_want], y + mu6_dy[mu6_want], false)) mu6_d[0] = mu6_want;
        if (!mu6_walk(x + mu6_dx[mu6_d[0]], y + mu6_dy[mu6_d[0]], false)) break;
      } else if (mu6_want == ((mu6_d[0] + 2) & 3)) mu6_d[0] = mu6_want;   // reversing is always allowed
      mu6_x[0] += mu6_dx[mu6_d[0]]; mu6_y[0] += mu6_dy[mu6_d[0]];
      if (mu6_x[0] < -128) mu6_x[0] += MU6_TW << 8; if (mu6_x[0] > ((MU6_TW - 1) << 8) + 128) mu6_x[0] -= MU6_TW << 8;
      mu6_chompA += 1.0f / 64;
    }
    // bloops
    for (int g = 1; g <= mu6_ng; g++) {
      int m = mu6_m[g];
      if (m == MU6_IN) { if (--mu6_tm[g] <= 0) { mu6_m[g] = MU6_LEAVE; mu6_y[g] = mu6_hcy << 8; mu6_x[g] = mu6_tx(g) << 8; continue; } mu6_y[g] = (mu6_hcy << 8) + (v3_sin((int)(globalTime * 600) + g * 300) >> 7); continue; }
      int gs = m == MU6_EYES ? 72 : (m == MU6_FRIGHT ? 18 : (m == MU6_LEAVE ? 16 : 24 + (int)(mu6_diff * 8) + a5_min(mu6_level, 6)));
      bool tunnel = mu6_ty(g) == mu6_hcy && (mu6_tx(g) < 5 || mu6_tx(g) > MU6_TW - 6);
      if (tunnel && m != MU6_EYES) gs = 16;
      mu6_moveGhost(g, gs);
      // contact
      int dx = mu6_x[g] - mu6_x[0], dy = mu6_y[g] - mu6_y[0];
      if (v3_abs(dx) < 160 && v3_abs(dy) < 160) {
        if (m == MU6_FRIGHT) { int v = 200 << a5_min(mu6_eatChain, 3); mu6_eatChain++; mu6_score += v; mu6_m[g] = MU6_EYES; mu6_popX = mu6_x[g]; mu6_popY = mu6_y[g]; mu6_popV = v; mu6_popT = 1.0f; a5_burst(MU6_X0 + (mu6_x[g] >> 5) + 4, MU6_Y0 + (mu6_y[g] >> 5) + 4, 16, 8, 30, 24); }
        else if (m == MU6_CHASE || m == MU6_LEAVE) mu6_die();
      }
    }
    // a bonus gem appears now and then
    mu6_gemT += dt;
    if (!mu6_gem && mu6_gemT > 25) { mu6_gem = true; mu6_gemT = 0; }
    if (mu6_gem && mu6_gemT > 10) { mu6_gem = false; mu6_gemT = 0; }
    if (mu6_gem && mu6_tx(0) == mu6_hcx && mu6_ty(0) == mu6_hcy + 4) { mu6_gem = false; mu6_gemT = 0; mu6_score += 100 * (mu6_level + 2); mu6_popX = mu6_x[0]; mu6_popY = mu6_y[0]; mu6_popV = 100 * (mu6_level + 2); mu6_popT = 1.5f; }
    if (mu6_dots <= 0) { mu6_clear = 2.2f; mu6_score += 500; }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  bool flash = mu6_clear > 0 && ((int)(mu6_clear * 6) & 1);
  // walls: lines along every wall edge that faces a corridor
  for (int y = 0; y < MU6_TH; y++) for (int x = 0; x < MU6_TW; x++) {
    uint8_t v = mu6_t[y][x];
    int px = MU6_X0 + x * 8, py = MU6_Y0 + y * 8;
    if (v & MU6_DOOR) { v3_hspan(px, px + 7, py + 4, A5C(15, 10)); continue; }
    if (!(v & MU6_WALL)) {
      if (v & MU6_DOT) v3_rect(px + 3, py + 3, px + 4, py + 4, A5C(13, 2));
      if ((v & MU6_PEL) && (((int)(globalTime * 4)) & 1)) v3_disc(px + 3, py + 3, 3, A5C(15, 2));
      continue;
    }
    uint8_t wc = flash ? A5C(15, 0) : A5C(15, 1);
    bool l = x > 0 && !(mu6_t[y][x - 1] & MU6_WALL), r = x < MU6_TW - 1 && !(mu6_t[y][x + 1] & MU6_WALL);
    bool u = y > 0 && !(mu6_t[y - 1][x] & MU6_WALL), d = y < MU6_TH - 1 && !(mu6_t[y + 1][x] & MU6_WALL);
    switch (wstyle) {
      case 1: v3_rect(px, py, px + 7, py + 7, flash ? A5C(15, 0) : A5C(9, 1)); if (u) v3_hspan(px, px + 7, py, wc); if (d) v3_hspan(px, px + 7, py + 7, wc); if (l) v3_rect(px, py, px, py + 7, wc); if (r) v3_rect(px + 7, py, px + 7, py + 7, wc); break;
      case 2: if (u || d || l || r) v3_rect(px + 3, py + 3, px + 4, py + 4, wc); break;
      case 3: v3_rect(px + 1, py + 1, px + 6, py + 6, A5C(3, 15)); if (u) { v3_hspan(px, px + 7, py + 1, wc); v3_hspan(px, px + 7, py, A5C(7, 1)); } if (d) { v3_hspan(px, px + 7, py + 6, wc); v3_hspan(px, px + 7, py + 7, A5C(7, 1)); } if (l) { v3_rect(px + 1, py, px + 1, py + 7, wc); v3_rect(px, py, px, py + 7, A5C(7, 1)); } if (r) { v3_rect(px + 6, py, px + 6, py + 7, wc); v3_rect(px + 7, py, px + 7, py + 7, A5C(7, 1)); } break;
      default: if (u) { v3_hspan(px, px + 7, py + 1, wc); v3_hspan(px, px + 7, py + 4, A5C(9, 1)); } if (d) { v3_hspan(px, px + 7, py + 6, wc); v3_hspan(px, px + 7, py + 3, A5C(9, 1)); }
               if (l) { v3_rect(px + 1, py, px + 1, py + 7, wc); v3_rect(px + 4, py, px + 4, py + 7, A5C(9, 1)); } if (r) { v3_rect(px + 6, py, px + 6, py + 7, wc); v3_rect(px + 3, py, px + 3, py + 7, A5C(9, 1)); } break;
    }
  }
  if (mu6_gem) { static const uint8_t gm[16] = { 0,0,0,0,0,0,0,0,0,0,0, A5C(14,11), 0,0,0, A5C(15,0) }; a5_spr(mu6_sGem, 10, 10, MU6_X0 + mu6_hcx * 8 - 1, MU6_Y0 + (mu6_hcy + 4) * 8 - 1, gm, 0, 0); }
  // bloops
  for (int g = 1; g <= mu6_ng; g++) {
    int x = MU6_X0 + (mu6_x[g] >> 5) - 3, y = MU6_Y0 + (mu6_y[g] >> 5) - 3, m = mu6_m[g];
    if (m == MU6_EYES) { v3_rect(x + 2, y + 4, x + 5, y + 7, A5C(15, 0)); v3_rect(x + 8, y + 4, x + 11, y + 7, A5C(15, 0)); v3_rect(x + 3 + mu6_dx[mu6_d[g]], y + 5 + mu6_dy[mu6_d[g]], x + 4 + mu6_dx[mu6_d[g]], y + 6 + mu6_dy[mu6_d[g]], A5C(15, 8)); v3_rect(x + 9 + mu6_dx[mu6_d[g]], y + 5 + mu6_dy[mu6_d[g]], x + 10 + mu6_dx[mu6_d[g]], y + 6 + mu6_dy[mu6_d[g]], A5C(15, 8)); continue; }
    static const uint8_t gh[6] = { 4, 5, 6, 7, 12, 13 };
    int hue = gh[(g - 1) % 6];
    uint8_t cm[16] = { 0, A5C(15, 0), 0, 0, A5C(14, hue), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A5C(2, 14) };
    if (m == MU6_FRIGHT) { bool blink = mu6_fright < 1.5f && (((int)(mu6_fright * 8)) & 1); cm[4] = blink ? A5C(15, 0) : A5C(13, 8); cm[1] = blink ? A5C(13, 4) : A5C(15, 9); cm[15] = cm[1]; }
    a5_spr(mu6_sBloop[((int)(globalTime * 6) + g) & 1], 14, 14, x, y, cm, 0, 0);
  }
  // Chomp
  if (mu6_dead <= 0 || mu6_dead > 1.8f) {
    int x = MU6_X0 + (mu6_x[0] >> 5) - 3, y = MU6_Y0 + (mu6_y[0] >> 5) - 3;
    uint8_t cm[16] = { 0, A5C(15, 0), 0, A5C(15, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A5C(1, 14), A5C(15, 0) };
    int fr = ((int)(mu6_chompA * 1.5f)) & 1;
    a5_spr(mu6_sChomp[fr], 14, 14, x, y, cm, mu6_d[0] == 2 ? A5_FX : 0, 0);
  } else {                                                           // popping like a balloon
    int x = MU6_X0 + (mu6_x[0] >> 5) + 4, y = MU6_Y0 + (mu6_y[0] >> 5) + 4, t = (int)((1.8f - mu6_dead) * 20);
    if (t < 20) a5_circle(x, y, 2 + t, A5C(15 - t / 2, 3)); if (t == 1) a5_burst(x, y, 30, 3, 40, 30);
  }
  // the spotlight: darkness beyond a few tiles of Chomp
  if (spot) {
    int cx = MU6_X0 + (mu6_x[0] >> 5) + 4, cy = MU6_Y0 + (mu6_y[0] >> 5) + 4, R = 34 + (int)(4 * sinf(globalTime * 3));
    for (int y = 12; y < H; y++) { int dy = y - cy, w = R * R - dy * dy; uint8_t* row = v3_buf + y * W; if (w < 0) { memset(row, 0, W); continue; } int hw = (int)v3_isqrt((uint32_t)w); int a = cx - hw, b = cx + hw; if (a > 0) memset(row, 0, a); if (b < W - 1) memset(row + b + 1, 0, W - 1 - b); }
  }
  a5_partDraw();
  if (mu6_popT > 0) { char s[8]; snprintf(s, 8, "%d", mu6_popV); a5_textS(MU6_X0 + (mu6_popX >> 5) - 6, MU6_Y0 + (mu6_popY >> 5) - 4, s, 1, A5C(15, 6)); }
  a5_post(MU6_X0 + (mu6_x[0] >> 5) + 4, MU6_Y0 + (mu6_y[0] >> 5) + 4);
  a5_hud(mu6_score, mu6_hi, A5C(15, 9), A5C(15, 0));
  for (int i = 0; i < v3_clamp(mu6_lives, 0, 5); i++) { int x = W - 12 - i * 11; v3_rect(x, 3, x + 7, 9, A5C(15, 3)); v3_hspan(x + 1, x + 6, 7, A5C(1, 14)); }
  char s[16]; snprintf(s, 16, "MAZE %d", mu6_level);
  if (mu6_banner > 0) { a5_banner(MU6_Y0 + (mu6_hcy + 0) * 8 - 3, "READY!", 1, A5C(15, 9)); }
  if (mu6_clear > 0) a5_banner(H / 2 - 4, s, 1, A5C(15, 9));
  if (mu6_over > 0) a5_banner(MU6_Y0 + mu6_hcy * 8 - 3, "GAME OVER", 1, A5C(15, 4));
  a5_crt();
}
void prog_muncher_draw(int preset) { muncher5_draw(preset); fx5_apply(); }

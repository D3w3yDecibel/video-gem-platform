// =====================================================================
// PROGRAM: AUTOMATA — living cellular automata (by Dewey) — Set 3, slot 5
//
// A grid of cells, each following a simple rule that looks only at its
// neighbours — and out of that come spiral waves, wars between colours,
// gliders, forest fires, crystals and mazes. Every preset is a different
// rule; all of them keep going for ever (if one dies down, fresh life rains
// in). k12 drops a splash of new cells into the grid.
//
// Presets:
//   k0 Spirals    — cyclic automaton: every colour eats the one before it, and spirals form
//   k1 Battle     — five colours at war (rock-paper-scissors), sweeping fronts
//   k2 Brain      — Brian's Brain: sparks that fire, rest and fly off as gliders
//   k3 Waves      — excitable medium: rings and spiral waves, like a heart or a chemical clock
//   k4 Life       — Conway's Game of Life, with glowing trails
//   k5 Day & Night — blobs that grow and dissolve in a symmetric rule
//   k6 Ants       — Langton's ants painting colours as they walk
//   k7 Forest Fire — trees grow, lightning strikes, fire sweeps through
//   k8 Crystal    — a frost crystal growing from a seed, band by band
//   k9 Anneal     — two colours separating like oil and water
//   k10 Maze      — corridors growing into a maze
//   k11 Evolve    — switches rule every ~20 s, keeping the picture as the seed for the next
//
//   k12 — SPLASH: drop new cells into the grid
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (generations a second; far left = freeze)
//   p2 Cell Size (centre = preset, 2-6 px)   p3 Threshold (how many neighbours it takes)
//   p4 States (colours / how long cells rest)   p5 Sparks (random cells that light up)
//   p6 Seed Density   p7 Seed Shape (centre = preset: noise, blob, rings, cross, kaleidoscope)
//   p8 Trails (how long dead cells glow)   p9 Ants (how many ants)   p10 Rule (centre = preset)
//   p11 Colour Cycle   p12 Cell Style (blocks, gaps, dots)   p13 Evolve Time
//   p14 Mirror (seed with left/right or four-way symmetry)   p15 Rain (automatic splashes)
// =====================================================================

#include "vg3.h"

// ─── Shared memory: the grid, 4 bits per cell, plus rolling rows ───────
#define CA_MAXG 9600                               // 160 × 120 cells / 2
static uint8_t* ca_g;                              // packed states
static uint8_t* ca_rows;                           // 4 unpacked rows of 164 (prev, cur, next, first)
static uint8_t* ca_col;                            // per-column neighbour sums
static void ca_mem() { ca_g = v3_scratch; ca_rows = v3_scratch + CA_MAXG; ca_col = v3_scratch + CA_MAXG + 4 * 164; }
static int ca_cs = 2, ca_gw = 160, ca_gh = 120, ca_ox = 0, ca_oy = 0;
static inline int ca_get(int x, int y) { uint8_t b = ca_g[(y * ca_gw + x) >> 1]; return (x & 1) ? (b >> 4) : (b & 15); }
static inline void ca_set(int x, int y, int s) {
  uint8_t* p = &ca_g[(y * ca_gw + x) >> 1];
  if (x & 1) *p = (uint8_t)((*p & 0x0F) | (s << 4)); else *p = (uint8_t)((*p & 0xF0) | (s & 15));
}
static void ca_unpack(int y, uint8_t* out) {                      // out[1 … gw] = states, out[0] and out[gw+1] wrap round
  const uint8_t* p = ca_g + ((y * ca_gw) >> 1);
  for (int x = 0; x < ca_gw; x += 2) { uint8_t b = *p++; out[x + 1] = b & 15; out[x + 2] = b >> 4; }
  out[0] = out[ca_gw]; out[ca_gw + 1] = out[1];
}
static void ca_pack(int y, const uint8_t* in) {
  uint8_t* p = ca_g + ((y * ca_gw) >> 1);
  for (int x = 0; x < ca_gw; x += 2) *p++ = (uint8_t)(in[x + 1] | (in[x + 2] << 4));
}

// ─── Rules ────────────────────────────────────────────────────────────
enum { CR_CYCLIC, CR_BRAIN, CR_WAVES, CR_LIFE, CR_ANTS, CR_FIRE, CR_CRYSTAL };
struct CaLook { uint8_t rule, n, thresh, cs, speed, pal, seed, density; uint16_t birth, survive; uint8_t trail, sparks, ants, style; };
//  life-like rules: birth/survive are bit masks of neighbour counts (bit n = n neighbours)
#define CA_B(a) (1u << (a))
static const CaLook ca_looks[11] = {
  // rule        n  thr cs spd pal seed dens birth                         survive                                   trail sparks ants style
  {CR_CYCLIC,   14, 1, 2, 30, 0,  0, 100, 0, 0,                                                                      0,  0, 0, 0},   // Spirals
  {CR_CYCLIC,    5, 3, 2, 24, 1,  0, 100, 0, 0,                                                                      0,  8, 0, 0},   // Battle
  {CR_BRAIN,     3, 2, 2, 18, 3,  1,  30, 0, 0,                                                                      6,  0, 0, 0},   // Brain
  {CR_WAVES,     9, 2, 2, 24, 2,  0,   4, 0, 0,                                                                      0, 10, 0, 0},   // Waves
  {CR_LIFE,      2, 0, 3, 12, 4,  0,  35, CA_B(3), CA_B(2) | CA_B(3),                                                9,  2, 0, 1},   // Life
  {CR_LIFE,      2, 0, 2, 18, 5,  0,  50, CA_B(3) | CA_B(6) | CA_B(7) | CA_B(8), CA_B(3) | CA_B(4) | CA_B(6) | CA_B(7) | CA_B(8), 8, 0, 0, 0},   // Day & Night
  {CR_ANTS,      6, 0, 3, 40, 6,  0,   0, 0, 0,                                                                      0,  0, 5, 0},   // Ants
  {CR_FIRE,      0, 0, 2, 20, 7,  0,  55, 0, 0,                                                                      0,  4, 0, 0},   // Forest Fire
  {CR_CRYSTAL,  14, 1, 2, 14, 3,  5,   0, 0, 0,                                                                      0,  0, 0, 0},   // Crystal
  {CR_LIFE,      2, 0, 2, 16, 1,  0,  50, CA_B(4) | CA_B(6) | CA_B(7) | CA_B(8), CA_B(3) | CA_B(5) | CA_B(6) | CA_B(7) | CA_B(8), 12, 1, 0, 0},  // Anneal
  {CR_LIFE,      2, 0, 3, 14, 2,  1,  45, CA_B(3), CA_B(1) | CA_B(2) | CA_B(3) | CA_B(4) | CA_B(5),               7,  0, 0, 1},   // Maze
};
static const char* const ca_presetNames[12] = {
  "Spirals", "Battle", "Brain", "Waves", "Life", "Day & Night",
  "Ants", "Forest Fire", "Crystal", "Anneal", "Maze", "Evolve"
};
static const char* const ca_labels[16] = {
  "Palette", "Speed", "Cell Size", "Threshold", "States", "Sparks", "Seed Density", "Seed Shape",
  "Trails", "Ants", "Rule", "Colour Cycle", "Cell Style", "Evolve Time", "Mirror", "Rain"
};
const char* prog_automata_name() { return "AUTOMATA"; }
const char* prog_automata_character() { return "Cellular automata: spirals, colour wars, gliders, fires, crystals and mazes growing from simple rules"; }
const char* prog_automata_presetName(int preset) { return (preset >= 0 && preset < 12) ? ca_presetNames[preset] : NULL; }
const char* prog_automata_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? ca_labels[pot] : ""; }
uint8_t prog_automata_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── Palettes: six colours each, used round a colour wheel ────────────
#define CA_NPAL 8
static const uint8_t ca_pals[CA_NPAL][6][3] = {
  {{255,30,30},{255,220,0},{0,230,60},{0,220,255},{40,60,255},{230,0,230}},     // 0 Spectrum
  {{255,0,160},{0,240,255},{255,240,0},{60,40,255},{120,255,0},{255,110,0}},    // 1 Neon
  {{255,255,255},{255,220,60},{255,110,0},{220,20,10},{110,0,40},{40,0,80}},    // 2 Ember
  {{230,255,255},{0,220,255},{0,110,255},{20,30,160},{120,0,200},{255,40,140}}, // 3 Glacier
  {{180,255,60},{0,255,200},{0,120,255},{120,40,255},{255,40,200},{255,200,0}}, // 4 Aurora
  {{255,120,190},{170,100,255},{90,230,200},{255,200,140},{120,200,255},{255,250,140}}, // 5 Candy
  {{255,80,0},{255,200,0},{0,200,255},{150,60,255},{255,0,120},{0,255,120}},    // 6 Pop
  {{40,170,40},{120,220,60},{255,140,0},{255,40,0},{90,90,90},{20,60,20}},      // 7 Forest
};

// ─── State ────────────────────────────────────────────────────────────
static int ca_preset = -1, ca_look = 0, ca_rule = 0, ca_n = 14, ca_th = 1, ca_trailStep = 1, ca_pal = 0;
static uint16_t ca_birth = 0, ca_surv = 0;
static float ca_genAcc = 0, ca_cycT = 0, ca_rainT = 0, ca_evoT = 0, ca_quietT = 0;
static uint32_t ca_changed = 0, ca_alive = 0, ca_gen = 0;
static bool ca_k12was = false;
static int ca_sparks = 0;
#define CA_MAXANT 16
static int16_t ca_ax[CA_MAXANT], ca_ay[CA_MAXANT];
static uint8_t ca_ad[CA_MAXANT];
static int ca_nAnts = 0;
static uint16_t ca_antTurn = 0;                    // bit s = turn right on colour s
static const uint16_t ca_antRules[6] = { 0x1, 0x5, 0x3, 0x19, 0xCC, 0x1E3 };   // RL, RLR(L), RRLL, LRRLL…, multi-colour turmites
static uint8_t ca_stateCol[18][3];

static void ca_setPalette(float cyc) {
  const uint8_t (*P)[3] = ca_pals[ca_pal];
  uint8_t c[16][3];
  for (int s = 0; s < 16; s++) { c[s][0] = c[s][1] = c[s][2] = 0; }
  // a point on the palette's colour wheel, 0 … 6 (smoothly between colours)
  #define CA_WHEEL(pos, out) { float p_ = (pos) - floorf((pos) / 6.0f) * 6.0f; int i0_ = (int)p_, i1_ = (i0_ + 1) % 6; int f_ = (int)((p_ - i0_) * 256); \
    for (int k_ = 0; k_ < 3; k_++) (out)[k_] = v3_mix8(P[i0_][k_], P[i1_][k_], f_); }
  switch (ca_rule) {
    case CR_CYCLIC: case CR_ANTS:
      for (int s = 0; s < ca_n; s++) CA_WHEEL(s * 6.0f / ca_n + cyc, c[s]);
      if (ca_rule == CR_ANTS) { c[0][0] = c[0][1] = c[0][2] = 0; }   // the ants paint on black
      break;
    case CR_CRYSTAL:
      for (int s = 1; s < 16; s++) CA_WHEEL((s - 1) * 6.0f / 15.0f + cyc, c[s]);
      break;
    case CR_FIRE: {
      uint8_t t[3];
      for (int s = 1; s <= 9; s++) { CA_WHEEL(cyc + 0.0f, t); int k = 110 + s * 8; for (int q = 0; q < 3; q++) c[s][q] = (uint8_t)(t[q] * k / 255); }   // trees, a little darker when young
      c[15][0] = 255; c[15][1] = 240; c[15][2] = 120;                // flames
      c[14][0] = 255; c[14][1] = 120; c[14][2] = 0;
      for (int s = 10; s <= 13; s++) { int k = (s - 9) * 40; c[s][0] = (uint8_t)(220 * k / 160); c[s][1] = (uint8_t)(40 * k / 160); c[s][2] = (uint8_t)(10 * k / 160); }  // embers cooling to ash
      break;
    }
    default: {                                                     // alive = bright colour, then a trail fading through the palette
      CA_WHEEL(cyc, c[15]);
      if (ca_rule == CR_BRAIN) { CA_WHEEL(cyc + 2.0f, c[14]); }
      if (ca_rule == CR_WAVES) {
        for (int s = 15 - ca_n; s < 15; s++) {
          float u = (15 - s) / (float)ca_n;                       // 0 just fired … 1 rested
          CA_WHEEL(cyc + 1.0f + u * 3.0f, c[s]);
          int k = (int)(255 * (1.0f - u * 0.75f)); for (int q = 0; q < 3; q++) c[s][q] = (uint8_t)(c[s][q] * k / 255);
        }
        uint8_t t[3]; CA_WHEEL(cyc + 4.5f, t); for (int q = 0; q < 3; q++) c[0][q] = (uint8_t)(t[q] / 7);
        break;
      }
      int top = ca_rule == CR_BRAIN ? 13 : 14;
      for (int s = 1; s <= top; s++) {
        float u = (top - s) / (float)top;                         // 0 just died … 1 long ago
        CA_WHEEL(cyc + 1.5f + u * 3.5f, c[s]);
        int k = (int)(255 * powf(1.0f - u, 1.3f)); for (int q = 0; q < 3; q++) c[s][q] = (uint8_t)(c[s][q] * k / 255);
      }
    }
  }
  // keep the living colours bright, whatever the palette passes through
  for (int s = 0; s < 16; s++) {
    bool on = (ca_rule == CR_CYCLIC || ca_rule == CR_CRYSTAL || (ca_rule == CR_ANTS && s > 0)) ? s < ca_n || ca_rule == CR_CRYSTAL : s == 15;
    if (ca_rule == CR_CRYSTAL && s == 0) on = false;
    int mx = c[s][0] > c[s][1] ? c[s][0] : c[s][1]; if (c[s][2] > mx) mx = c[s][2];
    int want = s == 15 && ca_rule != CR_CYCLIC ? 255 : 200;
    if (on && mx > 0 && mx < want) for (int q = 0; q < 3; q++) c[s][q] = (uint8_t)v3_clamp(c[s][q] * want / mx, 0, 255);
    if (on && mx == 0) c[s][0] = c[s][1] = c[s][2] = (uint8_t)want;
  }
  for (int s = 0; s < 16; s++) display.setColor((uint8_t)s, c[s][0], c[s][1], c[s][2]);
  display.setColor(16, 0, 0, 0);                                   // gaps between cells
  display.setColor(17, 255, 255, 255);                             // the ants
  for (int s = 0; s < 16; s++) for (int q = 0; q < 3; q++) ca_stateCol[s][q] = c[s][q];
  #undef CA_WHEEL
}

// rule-specific: is this state "on" (counted as a neighbour)?
static inline bool ca_isOn(int s) { return s == 15; }

static void ca_splash(int cx, int cy, int r, int mode) {
  for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
    if (dx * dx + dy * dy > r * r) continue;
    int x = ((cx + dx) % ca_gw + ca_gw) % ca_gw, y = ((cy + dy) % ca_gh + ca_gh) % ca_gh;
    int s = ca_get(x, y);
    switch (ca_rule) {
      case CR_CYCLIC: s = v3_rnd(ca_n); break;
      case CR_BRAIN: case CR_LIFE: if (v3_rnd(100) < (mode ? 50 : 35)) s = 15; break;
      case CR_WAVES: if (v3_rnd(100) < 8) s = 15; break;
      case CR_FIRE: if (s >= 1 && s <= 9 && v3_rnd(100) < 30) s = 15; break;
      case CR_CRYSTAL: break;
      default: break;
    }
    ca_set(x, y, s);
  }
  if (ca_rule == CR_CRYSTAL) ca_set(((cx % ca_gw) + ca_gw) % ca_gw, ((cy % ca_gh) + ca_gh) % ca_gh, 1);
}

static void ca_addAnt(int x, int y) {
  if (ca_nAnts >= CA_MAXANT) return;
  ca_ax[ca_nAnts] = (int16_t)x; ca_ay[ca_nAnts] = (int16_t)y; ca_ad[ca_nAnts] = (uint8_t)v3_rnd(4); ca_nAnts++;
}

static void ca_step(int sparks);
// seed the whole grid
static void ca_seed(int shape, int density, int mirror) {
  memset(ca_g, 0, CA_MAXG);
  ca_nAnts = 0;
  int cx = ca_gw / 2, cy = ca_gh / 2;
  for (int y = 0; y < ca_gh; y++) for (int x = 0; x < ca_gw; x++) {
    int sx = x, sy = y;
    if (mirror >= 1 && x >= ca_gw / 2) sx = ca_gw - 1 - x;
    if (mirror >= 2 && y >= ca_gh / 2) sy = ca_gh - 1 - y;
    uint32_t h = v3_hash((uint32_t)(sx * 7919 + sy * 104729) ^ v3_seed);
    int dx = x - cx, dy = y - cy, r = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy));
    bool in;
    switch (shape) {
      case 1: in = r < ca_gh / 4; break;                           // a blob in the middle
      case 2: in = (r % 12) < 3; break;                            // rings
      case 3: in = v3_abs(dx) < 4 || v3_abs(dy) < 4; break;        // a cross
      case 5: in = r < 2; break;                                   // a single seed
      default: in = true;
    }
    int s = 0;
    if (in && (int)(h % 100) < density) {
      switch (ca_rule) {
        case CR_CYCLIC: s = (int)((h >> 8) % ca_n); break;
        case CR_WAVES: s = (h >> 8) % 4 == 0 ? 15 : (int)(15 - (h >> 10) % ca_n); break;
        case CR_FIRE: s = 1 + (int)((h >> 8) % 9); break;
        case CR_ANTS: s = 0; break;
        default: s = 15;
      }
    } else if (ca_rule == CR_CYCLIC) s = (int)((h >> 12) % ca_n);   // cyclic: every cell has a colour
    ca_set(x, y, s);
  }
  if (ca_rule == CR_CRYSTAL) {                                     // the crystal starts already a little grown
    ca_set(cx, cy, 1);
    ca_n = 14; ca_th = 1; ca_gen = 0;
    for (int g = 0; g < 28; g++) { ca_step(0); watchdog_update(); }
  }
  if (ca_rule == CR_FIRE) for (int i = 0; i < 6; i++) ca_set(v3_rnd(ca_gw), v3_rnd(ca_gh), 15);
}

// one generation
static void ca_step(int sparks) {
  uint8_t* P = ca_rows; uint8_t* C = ca_rows + 164; uint8_t* N = ca_rows + 328; uint8_t* F = ca_rows + 492;
  int gw = ca_gw, gh = ca_gh;
  uint32_t changed = 0, alive = 0;
  ca_unpack(gh - 1, P); ca_unpack(0, C); memcpy(F, C, gw + 2);
  const int n = ca_n, th = ca_th, ts = ca_trailStep;
  uint8_t* col = ca_col;
  for (int y = 0; y < gh; y++) {
    if (y + 1 < gh) ca_unpack(y + 1, N); else memcpy(N, F, gw + 2);
    uint8_t out[164];
    switch (ca_rule) {
      case CR_CYCLIC: {
        for (int x = 1; x <= gw; x++) {
          int s = C[x], nx = s + 1 == n ? 0 : s + 1;
          int k = (P[x - 1] == nx) + (P[x] == nx) + (P[x + 1] == nx) + (C[x - 1] == nx) + (C[x + 1] == nx) + (N[x - 1] == nx) + (N[x] == nx) + (N[x + 1] == nx);
          int o = k >= th ? nx : s;
          out[x] = (uint8_t)o; changed += o != s;
        }
        break;
      }
      case CR_CRYSTAL: {                                           // freeze when exactly one of the four side neighbours is frozen
        int band = 1 + (int)((ca_gen / 6) % 15);
        for (int x = 1; x <= gw; x++) {
          int s = C[x];
          if (s) { out[x] = (uint8_t)s; alive++; continue; }
          int k = (P[x] != 0) + (N[x] != 0) + (C[x - 1] != 0) + (C[x + 1] != 0);
          int o = k == th ? band : 0;
          out[x] = (uint8_t)o; changed += o != 0;
        }
        break;
      }
      default: {                                                   // count "on" neighbours with column sums
        if (ca_rule == CR_FIRE) for (int x = 0; x <= gw + 1; x++) col[x] = (uint8_t)((P[x] >= 14) + (C[x] >= 14) + (N[x] >= 14));   // flames and glowing trunks spread fire
        else for (int x = 0; x <= gw + 1; x++) col[x] = (uint8_t)((P[x] == 15) + (C[x] == 15) + (N[x] == 15));
        for (int x = 1; x <= gw; x++) {
          int s = C[x];
          int k = col[x - 1] + col[x] + col[x + 1] - (s == 15);
          int o;
          switch (ca_rule) {
            case CR_LIFE:
              if (s == 15) o = (ca_surv >> k) & 1 ? 15 : (14 - ts < 0 ? 0 : 15 - ts);
              else { o = (ca_birth >> k) & 1 ? 15 : (s > ts ? s - ts : 0); }
              break;
            case CR_BRAIN:
              if (s == 15) o = 14;
              else if (s == 14) o = 13;
              else o = k == 2 ? 15 : (s > ts ? s - ts : 0);
              break;
            case CR_WAVES:
              if (s == 15) o = 14;
              else if (s > 15 - n) o = s - 1;
              else o = k >= th ? 15 : 0;
              break;
            default: {                                             // forest fire
              if (s == 15) o = 14;
              else if (s >= 10) o = s == 10 ? 0 : s - 1;
              else if (s >= 1) o = k > 0 ? 15 : (s < 9 && (v3_rand() & 127) == 0 ? s + 1 : s);
              else o = (v3_rand() & 255) < 3 ? 1 : 0;
            }
          }
          out[x] = (uint8_t)o; changed += o != s; alive += o == 15;
        }
      }
    }
    ca_pack(y, out);
    uint8_t* t = P; P = C; C = N; N = t;
  }
  // sparks: random cells that light up (keeps waves and wars going)
  for (int i = 0; i < sparks; i++) {
    int x = v3_rnd(gw), y = v3_rnd(gh);
    switch (ca_rule) {
      case CR_CYCLIC: ca_set(x, y, v3_rnd(ca_n)); break;
      case CR_WAVES: if (ca_get(x, y) == 0) { ca_set(x, y, 15); ca_set((x + 1) % gw, y, 15); ca_set(x, (y + 1) % gh, 15); ca_set((x + 1) % gw, (y + 1) % gh, 15); } break;
      case CR_FIRE: if (ca_get(x, y) >= 1 && ca_get(x, y) <= 9 && v3_rnd(8) == 0) ca_set(x, y, 15); break;
      case CR_CRYSTAL: break;
      default: if (v3_rnd(4) == 0) { ca_set(x, y, 15); ca_set((x + 1) % gw, y, 15); ca_set(x, (y + 1) % gh, 15); } break;
    }
  }
  ca_changed = changed; ca_alive = alive; ca_gen++;
}

static void ca_antSteps(int steps) {
  for (int a = 0; a < ca_nAnts; a++) {
    int x = ca_ax[a], y = ca_ay[a], d = ca_ad[a];
    for (int i = 0; i < steps; i++) {
      int s = ca_get(x, y);
      d = (d + (((ca_antTurn >> s) & 1) ? 1 : 3)) & 3;
      ca_set(x, y, s + 1 >= ca_n ? 0 : s + 1);
      if (d == 0) { x++; if (x >= ca_gw) x = 0; } else if (d == 1) { y++; if (y >= ca_gh) y = 0; }
      else if (d == 2) { x--; if (x < 0) x = ca_gw - 1; } else { y--; if (y < 0) y = ca_gh - 1; }
    }
    ca_ax[a] = (int16_t)x; ca_ay[a] = (int16_t)y; ca_ad[a] = (uint8_t)d;
  }
  ca_changed = (uint32_t)(steps * ca_nAnts); ca_gen++;
}

static void ca_render(int style) {
  int cs = ca_cs, gw = ca_gw, gh = ca_gh;
  uint8_t row[164];
  if (ca_ox > 0 || ca_oy > 0) {                                    // the margin round a grid that doesn't divide the screen
    for (int y = 0; y < ca_oy; y++) memset(v3_buf + y * W, 16, W);
    for (int y = ca_oy + gh * cs; y < H; y++) memset(v3_buf + y * W, 16, W);
  }
  for (int cy = 0; cy < gh; cy++) {
    ca_unpack(cy, row);
    for (int r = 0; r < cs; r++) {
      uint8_t* p = v3_buf + (ca_oy + cy * cs + r) * W;
      if (ca_ox) { memset(p, 16, ca_ox); memset(p + ca_ox + gw * cs, 16, W - ca_ox - gw * cs); }
      p += ca_ox;
      if (cs == 2 && style == 0) {
        uint16_t* q = (uint16_t*)p;
        for (int x = 0; x < gw; x++) { uint8_t s = row[x + 1]; q[x] = (uint16_t)(s | (s << 8)); }
      } else {
        bool edgeRow = style >= 1 && cs >= 3 && r == cs - 1;
        bool cornerRow = style == 2 && cs >= 4 && (r == 0 || r == cs - 2);   // dots: round off the corners
        for (int x = 0; x < gw; x++) {
          uint8_t s = row[x + 1];
          if (edgeRow) { memset(p, 16, cs); p += cs; continue; }
          for (int c = 0; c < cs; c++) {
            bool gap = (style >= 1 && cs >= 3 && c == cs - 1) || (cornerRow && (c == 0 || c == cs - 2));
            p[c] = gap ? 16 : s;
          }
          p += cs;
        }
      }
    }
  }
  for (int a = 0; a < ca_nAnts; a++) {                             // the ants themselves
    int px = ca_ox + ca_ax[a] * ca_cs, py = ca_oy + ca_ay[a] * ca_cs;
    for (int y = 0; y < ca_cs; y++) for (int x = 0; x < ca_cs; x++) v3_put(px + x, py + y, 17);
  }
}

static void ca_setGrid(int cs) {
  ca_cs = cs;
  ca_gw = (W / cs) & ~1; ca_gh = H / cs;
  if (ca_gw > 160) ca_gw = 160;
  if (ca_gw * ca_gh > CA_MAXG * 2) ca_gh = CA_MAXG * 2 / ca_gw;
  ca_ox = (W - ca_gw * cs) / 2; ca_oy = (H - ca_gh * cs) / 2;
}

static void ca_applyLook(int look, bool reseed, int cs, int shape, int density, int mirror) {
  const CaLook& L = ca_looks[look];
  int oldRule = ca_rule, oldN = ca_n;
  ca_look = look; ca_rule = L.rule; ca_birth = L.birth; ca_surv = L.survive;
  if (reseed) {
    ca_setGrid(cs);
    ca_seed(shape, density, mirror);
  } else {
    // Evolve: keep the picture, translated into the new rule's states
    for (int y = 0; y < ca_gh; y++) for (int x = 0; x < ca_gw; x++) {
      int s = ca_get(x, y), o;
      bool on = (oldRule == CR_CYCLIC || oldRule == CR_ANTS) ? (s * 2 >= oldN) : (s >= 13);
      switch (ca_rule) {
        case CR_CYCLIC: case CR_ANTS: o = (oldRule == CR_CYCLIC || oldRule == CR_ANTS) ? s % (L.n) : (s * L.n / 16); break;
        case CR_CRYSTAL: o = on ? 1 + (s % 15) : 0; break;
        case CR_FIRE: o = on ? 1 + (s % 9) : 0; break;
        default: o = on ? 15 : 0;
      }
      ca_set(x, y, o);
    }
    if (ca_rule == CR_CRYSTAL) ca_splash(ca_gw / 2, ca_gh / 2, 3, 0);
  }
  if (ca_rule == CR_ANTS) {
    ca_nAnts = 0;
    for (int i = 0; i < L.ants; i++) ca_addAnt(ca_gw / 2 + v3_rnd(21) - 10, ca_gh / 2 + v3_rnd(21) - 10);
  } else ca_nAnts = 0;
}

void prog_automata_init() {
  ca_mem();
  v3_reset();
  ca_preset = -1;
}

void prog_automata_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  ca_mem();
  v3_frame();
  float dt = v3_dt;
  bool evolve = preset == 11;
  // ── which rule, and (re)start ──
  int ruleK = v3_pick(10, -1, 11);
  int look = evolve ? ca_look : preset;
  if (ruleK >= 0) look = ruleK;
  {
    const CaLook& S = ca_looks[look];
    int csWant = v3_clamp(v3_centre(2) ? S.cs : 2 + v3_k(2) * 5 / 1024, 2, 6);
    int shape = v3_pick(7, S.seed, 6);
    int density = v3_centre(6) ? S.density : 10 + v3_k(6) * 82 / 1024;
    if (shape == 5 && S.rule != CR_CRYSTAL) shape = 1;               // a single seed only makes sense for the crystal
    int mirror = v3_pick(14, 0, 3);
    if (preset != ca_preset || csWant != ca_cs || (look != ca_look && (!evolve || ruleK >= 0))) {
      ca_preset = preset;
      ca_rule = S.rule; ca_n = S.n;
      ca_applyLook(look, true, csWant, shape, density, mirror);
      ca_evoT = 0; ca_quietT = 0;
    }
    if (evolve && ruleK < 0) {
      float evoTime = v3_centre(13) ? 20.0f : 6.0f + v3_u(13) * 40.0f;
      ca_evoT += dt;
      if (ca_evoT > evoTime) {
        ca_evoT = 0;
        int nl = (ca_look + 1 + v3_rnd(10)) % 11;
        ca_n = ca_looks[nl].n;
        ca_applyLook(nl, false, ca_cs, shape, density, mirror);
      }
    }
  }
  // ── knobs, for the rule now running ──
  const CaLook& L = ca_looks[ca_look];
  ca_rule = L.rule;
  float speed = v3_k(1) < 30 ? 0.0f : L.speed * powf(4.0f, v3_bi(1));          // generations per second
  int th = v3_centre(3) ? L.thresh : 1 + v3_k(3) * 4 / 1024;
  int nStates = v3_centre(4) ? L.n : 3 + v3_k(4) * 13 / 1024;
  int sparks = v3_centre(5) ? L.sparks : v3_k(5) * 40 / 1024;
  int trailStep = v3_centre(8) ? (L.trail ? (15 + L.trail - 1) / L.trail : 15) : v3_clamp(15 - v3_k(8) * 15 / 1024, 1, 15);
  int nAnts = v3_centre(9) ? L.ants : v3_k(9) * 17 / 1024;
  float cycSpeed = v3_k(11) < 30 ? 0.0f : (v3_centre(11) ? 0.08f : v3_u(11) * 0.8f);
  int style = v3_pick(12, L.style, 3);
  float rain = v3_centre(15) ? 0.15f : v3_u(15);
  int pal = v3_centre(0) ? L.pal : v3_clamp(v3_k(0) * CA_NPAL / 1024, 0, CA_NPAL - 1);
  if (L.rule == CR_CRYSTAL) th = v3_centre(3) ? 1 : 1 + (v3_k(3) > 700);    // crystal: 1 (snowflake) or 2 (blocky)
  if (L.rule == CR_CYCLIC && th > 3 && nStates > 8) th = 3;
  if (L.rule == CR_WAVES) nStates = v3_clamp(nStates, 3, 14);
  if (L.rule == CR_CYCLIC || L.rule == CR_ANTS) nStates = v3_clamp(nStates, 2, 16);
  if (L.rule == CR_CYCLIC || L.rule == CR_ANTS || L.rule == CR_WAVES) {
    if (nStates != ca_n) {
      if (L.rule != CR_WAVES) for (int y = 0; y < ca_gh; y++) for (int x = 0; x < ca_gw; x++) { int q = ca_get(x, y); if (q >= nStates) ca_set(x, y, q % nStates); }
      ca_n = nStates;
    }
  }
  ca_th = th;
  ca_trailStep = trailStep;
  ca_pal = pal;
  bool k12 = keysPressed[KEY_MOD_A];
  bool splash = k12 && !ca_k12was;
  ca_k12was = k12;
  // ants: as many as the knob says
  if (ca_rule == CR_ANTS) {
    int want = v3_clamp(nAnts, 1, CA_MAXANT);
    while (ca_nAnts < want) ca_addAnt(v3_rnd(ca_gw), v3_rnd(ca_gh));
    if (ca_nAnts > want) ca_nAnts = want;
    ca_antTurn = ca_antRules[(ca_n - 2) % 6];
    if (ca_n == 2) ca_antTurn = 0x1;
  }

  // ── splashes: k12 and rain ──
  ca_rainT += dt * rain * 1.5f;
  if (ca_rainT > 1.0f) { ca_rainT = 0; splash = true; }
  if (splash) {
    int x = v3_rnd(ca_gw), y = v3_rnd(ca_gh);
    if (ca_rule == CR_ANTS) { for (int i = 0; i < 2; i++) ca_addAnt(x, y); if (ca_nAnts > CA_MAXANT) ca_nAnts = CA_MAXANT; }
    else ca_splash(x, y, 4 + v3_rnd(8), 1);
  }

  // ── generations ──
  int cells = ca_gw * ca_gh;
  int maxPerFrame = v3_clamp(19200 * 2 / cells, 1, 8);
  ca_genAcc += dt * speed;
  int gens = (int)ca_genAcc; ca_genAcc -= gens;
  if (gens > maxPerFrame) { gens = maxPerFrame; ca_genAcc = 0; }
  for (int g = 0; g < gens; g++) {
    if (ca_rule == CR_ANTS) ca_antSteps(v3_clamp((int)(speed * 6), 4, 400));
    else ca_step(sparks);
    watchdog_update();
  }
  // never let it die: if nothing is happening, rain in new life (the crystal starts again)
  if (gens > 0) {
    bool quiet = ca_changed < (uint32_t)(cells / 400 + 2);
    if (ca_rule == CR_CRYSTAL) quiet = ca_changed == 0 || ca_alive > (uint32_t)(cells * 7 / 10);
    ca_quietT = quiet ? ca_quietT + dt : 0;
    if (ca_quietT > (ca_rule == CR_CRYSTAL ? 2.5f : 1.0f)) {
      ca_quietT = 0;
      if (ca_rule == CR_CRYSTAL) { memset(ca_g, 0, CA_MAXG); ca_set(ca_gw / 2, ca_gh / 2, 1); for (int i = 0; i < 2; i++) ca_set(v3_rnd(ca_gw), v3_rnd(ca_gh), 1); ca_gen = 0; for (int g = 0; g < 22; g++) { ca_step(0); watchdog_update(); } }
      else for (int i = 0; i < 4; i++) ca_splash(v3_rnd(ca_gw), v3_rnd(ca_gh), 6 + v3_rnd(8), 1);
    }
  }

  // ── colours and pixels ──
  ca_cycT += dt * cycSpeed * 6.0f;
  if (ca_cycT > 600.0f) ca_cycT -= 600.0f;
  ca_setPalette(ca_cycT);
  ca_render(style);
}

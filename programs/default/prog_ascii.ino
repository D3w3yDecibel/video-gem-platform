// =====================================================================
// PROGRAM: ASCII WORLDS (by Dewey)
// Little floating island worlds drawn entirely in tiny text characters,
// seen from an isometric angle: grass made of commas and quotes, pine
// trees of ^ marks, lakes rippling with ~ waves, people wandering,
// campfires, birds — plus a roguelike dungeon. Inspired by zakloyd's
// "ASCII LAKE" (https://www.instagram.com/p/DdRrSlsv50Y/). The code
// is original.
//
// Presets:
//   k0 Lake        — a forest island with a rippling lake, a fisher and a campfire
//   k1 River       — dense woods cut by a winding river, deer grazing
//   k2 Winter      — snow, a frozen lake with skaters, a cabin with smoke
//   k3 Volcano     — a smoking volcano with lava running down to the sea
//   k4 Archipelago — little islands in an open sea, boats sailing
//   k5 Oasis       — desert dunes, palms round a pool, camels plodding
//   k6 Night Rain  — the lake island at night in the rain, with lightning
//   k7 Dungeon     — a roguelike dungeon: @ explores room by room
//
//   k12 — NEW WORLD (a fresh island / dungeon)
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette, left → right: Green CRT, Amber CRT, C64, Ice, Original
//      (centre), Autumn, Vapor, Blood Moon    p1 Speed (far left = freeze)
//   p2 Zoom   p3 Spin (turns the island; far left = still)
//   p4 Water (wave activity)   p5 Trees   p6 Life (people and animals)
//   p7 Weather (rain / snow / ash)   p8 Wind   p9 Birds
//   p10 Char Size (small / big letters)   p11 Height (island thickness)
//   p12 Time of Day (night … day [centre] … sunset … night)
//   p13 Pop (contrast)   p14 Glitch (flickering characters)
// =====================================================================

// ─── The font: tiny 3×5 characters ───────────────────────────────────
enum {
  AG_SP, AG_DOT, AG_COMMA, AG_APOS, AG_QUOTE, AG_BTICK, AG_COLON, AG_SEMI, AG_CARET, AG_TILDE,
  AG_DASH, AG_EQ, AG_STAR, AG_HASH, AG_PCT, AG_AMP, AG_AT, AG_BAR, AG_SLASH, AG_BSLASH,
  AG_PLUS, AG_V, AG_A, AG_T, AG_Y, AG_o, AG_O, AG_x, AG_M, AG_n,
  AG_g, AG_r, AG_k, AG_D, AG_S, AG_BANG, AG_Q, AG_DOLLAR, AG_GT, AG_LT, AG_UNDER, AG_N
};
static const char* const aw_fontSrc[AG_N] = {
  "000000000000000", "000000000000010", "000000000010100", "010010000000000", "101101000000000",
  "100010000000000", "000010000010000", "000010000010100", "010101000000000", "000001111100000",
  "000000111000000", "000111000111000", "101010111010101", "101111101111101", "101001010100101",
  "010101010101011", "111101111100111", "010010010010010", "001001010100100", "100100010001001",
  "000010111010000", "000000101101010", "010101111101101", "111010010010010", "101101010010010",
  "000010101010000", "111101101101111", "000101010101000", "101111111101101", "000110101101101",
  "011101011001110", "000110100100100", "100101110101101", "110101101101110", "011100010001110",
  "010010010000010", "010101101111011", "011110010011110", "100010001010100", "001010100010001",
  "000000000000111"
};
static uint16_t aw_font[AG_N];

// ─── Colours: one palette slot per "material" ─────────────────────────
enum {
  AM_NONE,
  AM_GRASS0, AM_GRASS1, AM_GRASS2, AM_GRASS3,
  AM_TREE0, AM_TREE1, AM_TREE2, AM_TREE3,
  AM_TRUNK, AM_SAND0, AM_SAND1,
  AM_WATER0, AM_WATER1, AM_WAVE, AM_FOAM,
  AM_CLIFF0, AM_CLIFF1, AM_CLIFF2,
  AM_PERSON, AM_PERSON2, AM_CRIT,
  AM_FIRE0, AM_FIRE1, AM_FIRE2,
  AM_BIRD, AM_STAR, AM_ROCK0, AM_ROCK1, AM_SNOW, AM_ICE,
  AM_LAVA0, AM_LAVA1, AM_LAVA2, AM_RAIN, AM_SMOKE,
  AM_WALL, AM_FLOOR, AM_HERO, AM_MONST, AM_GOLD, AM_DOOR,
  AM_BOAT, AM_SAIL, AM_WINDOW, AM_CABIN, AM_FLOWER,
  AM_BG_WATER, AM_BG_LAVA, AM_BG_ICE,
  AM_BG_GRASS, AM_BG_SAND, AM_BG_SNOW, AM_BG_ROCK, AM_BG_CLIFF, AM_BG_DUNE, AM_BG_FLOOR, AM_BG_WALL,
  AM_N
};
// base colours (the "Original" scheme); last column = glows in the dark
static const uint8_t aw_base[AM_N][4] = {
  {0,0,0,0},
  {20,90,20,0}, {40,140,30,0}, {80,190,40,0}, {150,230,60,0},
  {10,90,30,0}, {20,140,40,0}, {40,190,60,0}, {110,235,90,0},
  {200,110,40,0}, {200,170,40,0}, {250,225,90,0},
  {10,20,80,0}, {30,60,170,0}, {60,120,255,0}, {150,220,255,0},
  {15,50,15,0}, {40,70,30,0}, {70,60,40,0},
  {230,60,230,0}, {80,200,255,0}, {230,120,40,0},
  {255,60,0,1}, {255,150,0,1}, {255,240,120,1},
  {235,235,235,0}, {230,230,255,1}, {110,110,120,0}, {170,170,180,0}, {235,245,255,0}, {170,220,255,0},
  {200,20,0,1}, {255,100,0,1}, {255,220,60,1}, {110,140,200,0}, {130,130,140,0},
  {120,110,100,0}, {60,55,50,0}, {255,255,90,1}, {240,60,60,0}, {255,210,0,1}, {170,110,50,0},
  {140,90,50,0}, {240,240,220,0}, {255,210,90,1}, {120,70,40,0}, {255,110,200,0},
  {0,5,35,0}, {60,5,0,1}, {40,60,90,0},
  {14,52,14,0}, {70,52,12,0}, {70,82,100,0}, {34,32,36,0}, {40,26,14,0}, {92,62,16,0}, {26,22,20,0}, {52,46,40,0}
};
#define AW_NSCHEME 8
static uint8_t aw_curve[256];

// p0 schemes: 0 Original, 1 Green CRT, 2 Amber CRT, 3 Autumn, 4 Vapor, 5 Ice, 6 Blood Moon, 7 C64
static void aw_scheme(int sc, int m, int* r, int* g, int* b) {
  int R = *r, G = *g, B = *b, l = (R * 77 + G * 150 + B * 29) >> 8;
  switch (sc) {
    case 1: R = l / 6; G = l; B = l / 4; break;
    case 2: R = l; G = l * 170 >> 8; B = l / 10; break;
    case 3: if ((m >= AM_GRASS0 && m <= AM_TREE3) || (m >= AM_CLIFF0 && m <= AM_CLIFF2)) {      // leaves turn
              int k = (m * 37) % 3;
              R = l + 60 > 255 ? 255 : l + 60; G = k == 0 ? l * 3 / 4 : k == 1 ? l / 2 : l / 4; B = l / 8;
            } break;
    case 4: { int t = R; R = (R + B) / 2 + 40 > 255 ? 255 : (R + B) / 2 + 40; B = G + 40 > 255 ? 255 : G + 40; G = t / 3 + l / 4; } break;
    case 5: R = l * 3 / 4; G = l * 9 / 10; B = l + 40 > 255 ? 255 : l + 40; break;
    case 6: R = l + 30 > 255 ? 255 : l + 30; G = l / 6; B = l / 5; break;
    case 7: { static const uint8_t c64[4][3] = {{53,40,121},{108,94,181},{112,164,178},{255,255,255}};
              int q = l < 50 ? 0 : l < 110 ? 1 : l < 170 ? 2 : 3; R = c64[q][0]; G = c64[q][1]; B = c64[q][2]; } break;
    default: break;
  }
  *r = R; *g = G; *b = B;
}

// time of day: tint for lit-by-the-sun things (glowing things stay bright)
static void aw_tint(int tod, int* tr, int* tg, int* tb) {
  static const int16_t key[5][4] = {        // knob pos, r, g, b (Q8)
    {0, 100, 110, 180}, {330, 230, 170, 190}, {512, 256, 256, 256}, {800, 256, 150, 90}, {1023, 100, 110, 180}
  };
  int k = 0;
  while (k < 3 && tod > key[k + 1][0]) k++;
  int f = (tod - key[k][0]) * 256 / (key[k + 1][0] - key[k][0]);
  *tr = key[k][1] + ((key[k + 1][1] - key[k][1]) * f >> 8);
  *tg = key[k][2] + ((key[k + 1][2] - key[k][2]) * f >> 8);
  *tb = key[k][3] + ((key[k + 1][3] - key[k][3]) * f >> 8);
}

static void aw_buildPalette(int sc, int tod, int pop, int flash) {
  int k = pop >> 2;
  for (int v = 0; v < 256; v++) {
    int s = v * v * (765 - 2 * v) / 65025;
    aw_curve[v] = (uint8_t)(v + ((s - v) * k >> 8));
  }
  int tr, tg, tb;
  aw_tint(tod, &tr, &tg, &tb);
  for (int i = 0; i < 256; i++) display.setColor(i, 0, 0, 0);
  for (int m = 1; m < AM_N; m++) {
    int r = aw_base[m][0], g = aw_base[m][1], b = aw_base[m][2];
    aw_scheme(sc, m, &r, &g, &b);
    if (!aw_base[m][3]) { r = r * tr >> 8; g = g * tg >> 8; b = b * tb >> 8; }
    if (flash) { r = (r + 510) / 3; g = (g + 510) / 3; b = (b + 540) / 3; }
    r = r > 255 ? 255 : r; g = g > 255 ? 255 : g; b = b > 255 ? 255 : b;
    display.setColor(m, aw_curve[r], aw_curve[g], aw_curve[b]);
  }
  display.setColor(255, 255, 255, 255);
}

// ─── Maths ────────────────────────────────────────────────────────────
static int16_t aw_sinT[1024];
static inline int aw_sin(int a) { return aw_sinT[a & 1023]; }
static inline int aw_cos(int a) { return aw_sinT[(a + 256) & 1023]; }
static uint32_t aw_seed = 1;
static inline uint32_t aw_hash(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + (uint32_t)(c + (int)aw_seed * 7919) * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static uint32_t aw_rng = 99;
static inline uint32_t aw_rand() { aw_rng ^= aw_rng << 13; aw_rng ^= aw_rng >> 17; aw_rng ^= aw_rng << 5; return aw_rng; }

// smooth value noise; x, y in Q8 units → 0 … 255
static int aw_noise(int x, int y, int salt) {
  int ix = x >> 8, iy = y >> 8, fx = x & 255, fy = y & 255;
  fx = fx * fx * (768 - 2 * fx) >> 16; fy = fy * fy * (768 - 2 * fy) >> 16;
  int a = aw_hash(ix, iy, salt) & 255, b = aw_hash(ix + 1, iy, salt) & 255;
  int c = aw_hash(ix, iy + 1, salt) & 255, d = aw_hash(ix + 1, iy + 1, salt) & 255;
  int ab = a + ((b - a) * fx >> 8), cd = c + ((d - c) * fx >> 8);
  return ab + ((cd - ab) * fy >> 8);
}

// ─── The character grid ───────────────────────────────────────────────
#define AW_GCMAX 80
#define AW_GRMAX 40
static uint8_t aw_gch[AW_GCMAX * AW_GRMAX];      // glyph
static uint8_t aw_gfg[AW_GCMAX * AW_GRMAX];      // glyph colour (material)
static uint8_t aw_gbg[AW_GCMAX * AW_GRMAX];      // cell background colour (0 = none) — makes the ground read as solid
static int aw_cs = 1, aw_GC = 80, aw_GR = 40, aw_cw = 4, aw_chh = 6;

static inline void aw_put(int c, int r, int g, int m) {
  if ((unsigned)c >= (unsigned)aw_GC || (unsigned)r >= (unsigned)aw_GR) return;
  int i = r * aw_GC + c;
  aw_gch[i] = (uint8_t)g;
  aw_gfg[i] = (uint8_t)m;
}
static void aw_renderGrid(uint8_t* buf) {
  memset(buf, 0, W * H);
  int s = aw_cs;
  for (int r = 0; r < aw_GR; r++)
    for (int c = 0; c < aw_GC; c++) {
      int i = r * aw_GC + c;
      int g = aw_gch[i], bg = aw_gbg[i];
      int x0 = c * aw_cw, y0 = r * aw_chh;
      if (bg) {                                                 // cells start on 4-byte boundaries: fill a word at a time
        uint32_t w4 = (uint32_t)bg * 0x01010101u;
        uint32_t* p = (uint32_t*)(buf + y0 * W + x0);
        for (int y = 0; y < aw_chh; y++, p += W / 4) { p[0] = w4; if (s > 1) p[1] = w4; }
      }
      if (!g) continue;
      uint16_t bits = aw_font[g];
      uint8_t col = aw_gfg[i];
      for (int y = 0; y < 5; y++)
        for (int x = 0; x < 3; x++)
          if (bits & (1 << (14 - y * 3 - x))) {
            if (s == 1) buf[(y0 + y) * W + x0 + x] = col;
            else {
              uint8_t* p = buf + (y0 + y * s) * W + x0 + x * s;
              for (int yy = 0; yy < s; yy++, p += W) for (int xx = 0; xx < s; xx++) p[xx] = col;
            }
          }
    }
}

// ─── Settings ─────────────────────────────────────────────────────────
static float aw_sm[16];
static bool aw_smReady = false;
static inline int aw_k(int i) { int v = (int)aw_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
static int aw_t = 0, aw_preset = 0;
static int aw_TW = 14;                // tile width on screen (px)
static int aw_rot = 0;                // island turn (1024 = full turn)
static int aw_ca = 16384, aw_sa = 0;
static int aw_water = 512, aw_trees = 512, aw_life = 512, aw_weather = 0, aw_wind = 512, aw_birds = 512;
static int aw_height = 4, aw_tod = 512, aw_glitch = 0;
static int aw_cx = W / 2, aw_cy = H / 2;
#define AW_R 10                        // island half-size (tiles)

// world (Q8 tiles) → screen pixel; depth for sorting
static inline void aw_toScreen(int u, int v, int* sx, int* sy, int* depth) {
  int ru = (u * aw_ca - v * aw_sa) >> 14, rv = (u * aw_sa + v * aw_ca) >> 14;
  *sx = aw_cx + ((ru - rv) * aw_TW >> 9);
  *sy = aw_cy + ((ru + rv) * aw_TW >> 10);
  *depth = ru + rv;
}
// screen pixel → world (Q8 tiles), on the ground plane
static inline void aw_toWorld(int sx, int sy, int* u, int* v) {
  int a = (sx - aw_cx) * 512 / aw_TW, b = (sy - aw_cy) * 1024 / aw_TW;
  int ru = (a + b) >> 1, rv = (b - a) >> 1;
  *u = (ru * aw_ca + rv * aw_sa) >> 14;
  *v = (-ru * aw_sa + rv * aw_ca) >> 14;
}

// ─── Terrain ──────────────────────────────────────────────────────────
// kinds of ground
enum { AT_VOID, AT_GRASS, AT_SAND, AT_WATER, AT_SNOW, AT_ICE, AT_ROCK, AT_LAVA, AT_DUNE };

static int aw_island(int u, int v, int salt, int R) {        // >0 inside (distance to edge, Q8)
  int au = u < 0 ? -u : u, av = v < 0 ? -v : v;
  int mx = au > av ? au : av;
  int d = (mx * 3 + ((au + av) >> 1)) >> 2;                   // a rounded square
  int n = aw_noise(u * 2 / 3 + 5000, v * 2 / 3 + 5000, salt) - 128;
  return (R << 8) - d - n * 3;
}

// what's on the ground at (u, v)?
static int aw_ground(int u, int v) {
  switch (aw_preset) {
    case 1: {                                                  // river
      if (aw_island(u, v, 11, AW_R) <= 0) return AT_VOID;
      int cv = aw_sin(u * 3 / 2 + 200) * 3 * 256 >> 14;       // river centre line
      int d = v - cv - (u >> 3);
      if (d < 0) d = -d;
      if (d < 300) return AT_WATER;
      if (d < 400) return AT_SAND;
      return AT_GRASS;
    }
    case 2: {                                                  // winter
      if (aw_island(u, v, 12, AW_R) <= 0) return AT_VOID;
      int du = u - 256, dv = v + 200, e = du * du / 256 + dv * dv * 3 / 512;
      int r = 5 * 256 + (aw_noise(u + 900, v, 3) - 128) * 3;
      if (e < r * r / 256 * 1) return AT_ICE;
      return AT_SNOW;
    }
    case 3: {                                                  // volcano
      int in = aw_island(u, v, 13, AW_R);
      if (in <= 0) return AT_VOID;
      int d2 = (u * u + v * v) >> 8;
      // lava rivers winding out from the cone
      for (int k = 0; k < 3; k++) {
        int a = k * 341 + 100;
        int ca = aw_cos(a), sa = aw_sin(a);
        int along = (u * ca + v * sa) >> 14, across = (-u * sa + v * ca) >> 14;
        across += aw_sin(along * 3 + k * 200) * 160 >> 14;
        if (along > 400 && across > -150 && across < 150) return AT_LAVA;
      }
      if (in < 200) return AT_SAND;
      return d2 < 900 ? AT_ROCK : (aw_noise(u, v, 5) > 110 ? AT_ROCK : AT_GRASS);
    }
    case 4: {                                                  // archipelago: sea with islands
      if (aw_island(u, v, 14, AW_R + 1) <= 0) return AT_VOID;
      int n = aw_noise(u * 3 / 2, v * 3 / 2, 7);
      if (n > 170) return AT_GRASS;
      if (n > 155) return AT_SAND;
      return AT_WATER;
    }
    case 5: {                                                  // oasis
      if (aw_island(u, v, 15, AW_R) <= 0) return AT_VOID;
      int e = (u * u + (v - 200) * (v - 200)) >> 8;
      int r = 700 + (aw_noise(u + 300, v, 9) - 128) * 2;
      if (e < r * 3) return AT_WATER;
      if (e < r * 5) return AT_GRASS;
      return AT_DUNE;
    }
    default: {                                                 // lake (k0, k6)
      if (aw_island(u, v, 10, AW_R) <= 0) return AT_VOID;
      int du = u - 150, dv = v + 250;
      int e = du * du / 256 + dv * dv * 2 / 256;
      int r = (int)(5.2f * 256) + (aw_noise(u + 777, v + 333, 2) - 128) * 4;
      if (e < r * r / 256) return AT_WATER;
      int rs = r + 180;
      if (e < rs * rs / 256) return AT_SAND;
      return AT_GRASS;
    }
  }
}

static inline bool aw_walkable(int u, int v, int type) {
  int g = aw_ground(u, v);
  if (type == 2) return g == AT_WATER;                        // boats
  if (type == 3) return g == AT_ICE;                          // skaters
  return g != AT_VOID && g != AT_WATER && g != AT_LAVA && g != AT_ICE;
}

// The ground under every cell is worked out once and kept (kind in the
// low 4 bits, grass shade in the high bits) until the view changes —
// spinning, zooming or a new world.
static uint8_t aw_kind[AW_GCMAX * AW_GRMAX];
static uint32_t aw_kindKey = 0xFFFFFFFFu;

static void aw_updateKinds() {
  uint32_t key = (uint32_t)aw_rot * 2654435761u ^ (uint32_t)aw_TW * 40503u ^ (uint32_t)aw_cs * 977u ^
                 (uint32_t)aw_preset * 131u ^ aw_seed * 7u ^ (uint32_t)aw_cy * 65537u;
  if (key == aw_kindKey) return;
  aw_kindKey = key;
  for (int r = 0; r < aw_GR; r++) {
    int py = r * aw_chh + aw_chh / 2;
    for (int c = 0; c < aw_GC; c++) {
      int u, v;
      aw_toWorld(c * aw_cw + aw_cw / 2, py, &u, &v);
      int k = aw_ground(u, v);
      int shade = k == AT_GRASS ? (aw_noise(u * 2, v * 2, 21) >> 6) : 0;
      aw_kind[r * aw_GC + c] = (uint8_t)(k | (shade << 4));
    }
  }
}

// fill the grid with the ground, the island's sides and the sky
static void aw_drawGround() {
  int t = aw_t;
  int wa = 1 + (aw_water >> 7);                                // wave activity
  int T = aw_height;
  aw_updateKinds();
  bool night = aw_tod < 300 || aw_tod > 880 || aw_preset == 6;
  for (int r = 0; r < aw_GR; r++) {
    int py = r * aw_chh + aw_chh / 2;
    for (int c = 0; c < aw_GC; c++) {
      int i = r * aw_GC + c;
      int gk = aw_kind[i] & 15;
      uint32_t h = aw_hash(c, r, 1);
      if (gk == AT_VOID) {
        // the island's side wall: is there ground just above this cell?
        int side = 0;
        for (int k = 1; k <= T && k <= r; k++)
          if ((aw_kind[i - k * aw_GC] & 15) != AT_VOID) { side = k; break; }
        if (side) {
          static const uint8_t sg[5] = {AG_COLON, AG_DOT, AG_APOS, AG_BAR, AG_COMMA};
          int m = side == 1 ? AM_CLIFF2 : side < T ? AM_CLIFF1 : AM_CLIFF0;
          if (aw_preset == 2) m = side == 1 ? AM_SNOW : AM_ROCK0;
          if (aw_preset == 5) m = side == 1 ? AM_SAND0 : AM_CLIFF2;
          aw_gch[i] = (uint8_t)((h & 3) ? sg[h % 5] : AG_SP); aw_gfg[i] = (uint8_t)m;
          aw_gbg[i] = (uint8_t)(aw_preset == 2 && side == 1 ? AM_BG_SNOW : aw_preset == 5 && side == 1 ? AM_BG_DUNE : AM_BG_CLIFF);
          continue;
        }
        aw_gbg[i] = 0;
        if (night) {                                    // stars at night
          bool star = (h & 63) == 0 && ((t >> 3) + (int)(h >> 8)) % 7 != 0;
          aw_gch[i] = star ? ((h >> 7) & 1 ? AG_DOT : AG_APOS) : AG_SP; aw_gfg[i] = AM_STAR;
        } else { aw_gch[i] = AG_SP; aw_gfg[i] = 0; }
        continue;
      }
      int u, v;
      aw_toWorld(c * aw_cw + aw_cw / 2, py, &u, &v);
      int g = AG_SP, m = AM_GRASS1, bg = 0;
      switch (gk) {
        case AT_GRASS: {
          static const uint8_t gg[8] = {AG_QUOTE, AG_COMMA, AG_APOS, AG_QUOTE, AG_SEMI, AG_DOT, AG_SEMI, AG_COLON};
          g = gg[(h >> 3) & 7];
          bg = AM_BG_GRASS;
          m = AM_GRASS0 + (aw_kind[i] >> 4);
          // grass bends in the wind
          if (aw_wind > 600 && ((u + v + t * 4) & 511) < 60) g = AG_SLASH;
          if ((h & 255) == 7) { g = AG_STAR; m = AM_FLOWER; }
        } break;
        case AT_SAND: bg = AM_BG_SAND; g = (h & 3) ? AG_DOT : AG_COLON; m = (h & 4) ? AM_SAND1 : AM_SAND0; break;
        case AT_DUNE: {
          int w = aw_sin((u + v) * 2 + (aw_noise(u, v, 4) << 1)) >> 12;   // dune ripples
          g = w > 1 ? AG_TILDE : (h & 1) ? AG_DOT : AG_SP;
          m = w > 2 ? AM_SAND1 : AM_SAND0;
          bg = AM_BG_DUNE;
        } break;
        case AT_WATER: {
          bg = AM_BG_WATER;
          // diagonal wave streaks sliding across
          int ph = aw_sin(((u * 3 - v * 5) >> 1) - t * wa * 6) + (aw_sin((u + v * 2) - t * 4) >> 1);
          int sp = (aw_hash(c, r, 9) & 7);
          if (ph > 13000 - sp * 400) { g = AG_TILDE; m = ph > 15500 ? AM_FOAM : AM_WAVE; }
          else if (ph > 6000) { g = (h & 1) ? AG_DASH : AG_TILDE; m = AM_WATER1; }
          else { g = (h & 7) == 0 ? AG_DOT : AG_SP; m = AM_WATER1; }
          if (aw_preset == 6 && (aw_hash(c, r, t >> 1) & 31) == 0) { g = AG_o; m = AM_FOAM; }   // raindrop rings
        } break;
        case AT_SNOW: bg = AM_BG_SNOW; g = (h & 3) == 0 ? AG_SP : (h & 4) ? AG_DOT : (h & 8) ? AG_STAR : AG_APOS;
                      m = (h & 16) ? AM_SNOW : AM_ICE; break;
        case AT_ICE: bg = AM_BG_ICE; g = (h & 7) == 0 ? AG_DASH : (h & 7) == 1 ? AG_EQ : AG_SP; m = AM_ICE;
                     if (((u - v + t * 2) & 1023) < 40) { g = AG_SLASH; m = AM_SNOW; } break;   // glints
        case AT_ROCK: bg = AM_BG_ROCK; g = (h & 3) == 0 ? AG_PCT : (h & 3) == 1 ? AG_DOT : (h & 3) == 2 ? AG_COLON : AG_SP;
                      m = (h & 4) ? AM_ROCK1 : AM_ROCK0; break;
        case AT_LAVA: {
          bg = AM_BG_LAVA;
          int ph = aw_sin((u + v) * 4 - t * 12 + (int)(h & 255));
          g = ph > 8000 ? AG_TILDE : ph > 0 ? AG_AMP : AG_STAR;
          m = ph > 10000 ? AM_LAVA2 : ph > 0 ? AM_LAVA1 : AM_LAVA0;
        } break;
      }
      aw_gch[i] = (uint8_t)g;
      aw_gfg[i] = (uint8_t)m;
      aw_gbg[i] = (uint8_t)bg;
    }
  }
}

// ─── Things on the island ─────────────────────────────────────────────
enum { AO_PINE, AO_ROUND, AO_PALM, AO_DEAD, AO_ROCK, AO_BUSH, AO_CABIN, AO_FIRE, AO_FISHER, AO_VOLCANO };
#define AW_NOBJ 110
static int16_t aw_ou[AW_NOBJ], aw_ov[AW_NOBJ];
static uint8_t aw_ot[AW_NOBJ], aw_os[AW_NOBJ];
static int aw_nObj = 0, aw_nTrees = 0;

#define AW_NWALK 16
static int16_t aw_wu[AW_NWALK], aw_wv[AW_NWALK];
static int16_t aw_wa[AW_NWALK];
static uint8_t aw_wt[AW_NWALK];            // 0 person, 1 critter, 2 boat, 3 skater, 4 camel

static void aw_addObj(int u, int v, int type, int s) {
  if (aw_nObj >= AW_NOBJ) return;
  aw_ou[aw_nObj] = (int16_t)u; aw_ov[aw_nObj] = (int16_t)v; aw_ot[aw_nObj] = (uint8_t)type; aw_os[aw_nObj] = (uint8_t)s;
  aw_nObj++;
}

static void aw_generate() {
  aw_nObj = 0;
  aw_rng = aw_seed * 2654435761u + 17;
  if (!aw_rng) aw_rng = 1;
  int R = AW_R << 8;
  // special things first (always shown)
  int p = aw_preset;
  if (p == 0 || p == 6) {
    // a campfire and a fisher on the shore
    for (int tries = 0; tries < 400; tries++) {
      int u = (int)(aw_rand() % (2 * R)) - R, v = (int)(aw_rand() % (2 * R)) - R;
      if (aw_ground(u, v) == AT_SAND && aw_ground(u + 300, v + 300) != AT_WATER) { aw_addObj(u, v, AO_FISHER, 0); break; }
    }
    for (int tries = 0; tries < 400; tries++) {
      int u = (int)(aw_rand() % (2 * R)) - R, v = (int)(aw_rand() % (2 * R)) - R;
      if (aw_ground(u, v) == AT_GRASS && aw_island(u, v, 10, AW_R) > 400) { aw_addObj(u, v, AO_FIRE, 0); aw_addObj(u + 260, v - 60, AO_CABIN, 0); break; }
    }
  }
  if (p == 2) aw_addObj(-5 * 256, 3 * 256, AO_CABIN, 1);
  if (p == 3) aw_addObj(0, 0, AO_VOLCANO, 0);
  aw_nTrees = aw_nObj;
  // trees and scenery, shuffled so the Trees knob thins them evenly
  int want = 95;
  for (int tries = 0; tries < 2500 && aw_nObj < AW_NOBJ && want > 0; tries++) {
    int u = (int)(aw_rand() % (2 * R)) - R, v = (int)(aw_rand() % (2 * R)) - R;
    int g = aw_ground(u, v);
    int type = -1;
    if (p == 5) { if (g == AT_GRASS) type = AO_PALM; else if (g == AT_DUNE && (aw_rand() & 7) == 0) type = AO_ROCK; }
    else if (p == 4) { if (g == AT_GRASS) type = (aw_rand() & 1) ? AO_PALM : AO_BUSH; }
    else if (p == 3) { if (g == AT_GRASS) type = (aw_rand() & 1) ? AO_DEAD : AO_ROCK; else if (g == AT_ROCK && (aw_rand() & 3) == 0) type = AO_ROCK; }
    else if (g == AT_GRASS || g == AT_SNOW) {
      int r = aw_rand() & 15;
      type = r < 10 ? AO_PINE : r < 12 ? (p == 2 ? AO_PINE : AO_ROUND) : r < 14 ? AO_BUSH : AO_ROCK;
      if (p == 1 && r < 13) type = (r & 1) ? AO_ROUND : AO_PINE;
    }
    if (type < 0) continue;
    // keep a little space between things
    bool ok = true;
    for (int j = 0; j < aw_nObj && ok; j++) {
      int du = aw_ou[j] - u, dv = aw_ov[j] - v;
      if (du * du + dv * dv < (type == AO_ROCK || type == AO_BUSH ? 150 * 150 : 230 * 230)) ok = false;
    }
    if (p == 3 && u * u + v * v < 900 * 900) ok = false;       // not on the volcano
    if (!ok) continue;
    aw_addObj(u, v, type, (int)(aw_rand() % 4));
    want--;
  }
  // wanderers
  for (int i = 0; i < AW_NWALK; i++) {
    int type = 0;
    if (p == 1) type = (i & 1) ? 1 : 0;
    else if (p == 2) type = (i < 6) ? 3 : 0;
    else if (p == 4) type = 2;
    else if (p == 5) type = (i & 1) ? 4 : 0;
    else if (p == 3) type = 1;
    else type = (i % 3 == 2) ? 1 : 0;
    aw_wt[i] = (uint8_t)type;
    aw_wu[i] = 0; aw_wv[i] = 0;
    for (int tries = 0; tries < 300; tries++) {
      int u = (int)(aw_rand() % (2 * R)) - R, v = (int)(aw_rand() % (2 * R)) - R;
      if (aw_walkable(u, v, type)) { aw_wu[i] = (int16_t)u; aw_wv[i] = (int16_t)v; break; }
    }
    aw_wa[i] = (int16_t)(aw_rand() & 1023);
  }
}

static void aw_moveWalkers(int steps) {
  int n = aw_life * AW_NWALK / 1023;
  for (int i = 0; i < n; i++) {
    for (int s = 0; s < steps; s++) {
      int sp = aw_wt[i] == 2 ? 10 : aw_wt[i] == 3 ? 14 : aw_wt[i] == 1 ? 8 : 5;
      if ((aw_rand() & 63) == 0) aw_wa[i] = (int16_t)(aw_wa[i] + (int)(aw_rand() % 300) - 150);
      if (aw_wt[i] == 1 && (aw_rand() & 31) < 20) continue;      // critters stop to graze
      int nu = aw_wu[i] + (aw_cos(aw_wa[i]) * sp >> 14), nv = aw_wv[i] + (aw_sin(aw_wa[i]) * sp >> 14);
      if (aw_walkable(nu, nv, aw_wt[i])) { aw_wu[i] = (int16_t)nu; aw_wv[i] = (int16_t)nv; }
      else aw_wa[i] = (int16_t)(aw_wa[i] + 400 + (int)(aw_rand() % 224));
    }
  }
}

// draw one thing with its base at grid cell (c, r)
static void aw_drawObj(int type, int s, int c, int r, int idx) {
  int t = aw_t;
  int sway = aw_wind > 700 ? ((aw_sin(t * 6 + idx * 90) > 8000) ? 1 : 0) : 0;
  switch (type) {
    case AO_PINE: {
      int hgt = 3 + s + (aw_cs == 1 ? 1 : 0);
      bool snowy = aw_preset == 2;
      aw_put(c, r, AG_BAR, AM_TRUNK);
      for (int i = 0; i < hgt; i++) {
        int y = r - 1 - i, hw = (hgt - i) / 2;
        if (i == hgt - 1) hw = 0;
        int sx = (i > hgt / 2) ? sway : 0;
        for (int x = -hw; x <= hw; x++) {
          int m = x < 0 ? AM_TREE3 : x == 0 ? AM_TREE2 : AM_TREE1;
          if (i == 0) m = AM_TREE0 + (x < 0);
          int g = (x == 0 && i == hgt - 1) ? AG_CARET : ((idx + i + x) & 3) == 0 ? AG_A : AG_CARET;
          if (snowy && (i >= hgt - 2 || ((idx + x + i) & 3) == 0)) { m = AM_SNOW; g = AG_STAR; }
          aw_put(c + x + sx, y, g, m);
        }
      }
    } break;
    case AO_ROUND:
      aw_put(c, r, AG_BAR, AM_TRUNK);
      for (int x = -1; x <= 1; x++) {
        aw_put(c + x + sway, r - 1, (x + idx) & 1 ? AG_AMP : AG_PCT, x < 0 ? AM_TREE3 : AM_TREE1);
        aw_put(c + x + sway, r - 2, (x + idx) & 1 ? AG_PCT : AG_AT, x < 0 ? AM_TREE3 : AM_TREE2);
      }
      if (s > 1) aw_put(c + sway, r - 3, AG_AMP, AM_TREE3);
      break;
    case AO_PALM: {
      int hgt = 2 + (s >> 1);
      for (int i = 0; i < hgt; i++) aw_put(c + (i > 0 ? (s & 1) : 0), r - i, i ? AG_SLASH : AG_BAR, AM_TRUNK);
      int tc = c + (s & 1) + sway, tr = r - hgt;
      aw_put(tc, tr, AG_STAR, AM_TREE2);
      aw_put(tc - 1, tr, AG_DASH, AM_TREE3); aw_put(tc + 1, tr, AG_DASH, AM_TREE1);
      aw_put(tc - 1, tr + 1, AG_SLASH, AM_TREE3); aw_put(tc + 1, tr + 1, AG_BSLASH, AM_TREE1);
      aw_put(tc, tr - 1, AG_CARET, AM_TREE3);
    } break;
    case AO_DEAD:
      aw_put(c, r, AG_BAR, AM_ROCK0); aw_put(c, r - 1, AG_Y, AM_ROCK1);
      if (s & 1) aw_put(c + 1, r - 2, AG_SLASH, AM_ROCK0);
      break;
    case AO_ROCK: aw_put(c, r, s & 1 ? AG_O : AG_o, (s & 2) ? AM_ROCK1 : AM_ROCK0); break;
    case AO_BUSH: aw_put(c, r, AG_PCT, AM_TREE1); if (s & 1) aw_put(c + 1, r, AG_AMP, AM_TREE2); break;
    case AO_CABIN: {
      // roof, walls, glowing window, smoke from the chimney
      for (int x = -2; x <= 2; x++) aw_put(c + x, r, x == 0 ? AG_BAR : AG_HASH, AM_CABIN);
      aw_put(c + 1, r, AG_o, AM_WINDOW);
      for (int x = -2; x <= 2; x++) aw_put(c + x, r - 1, x < 0 ? AG_SLASH : x > 0 ? AG_BSLASH : AG_CARET, s ? AM_SNOW : AM_TRUNK);
      aw_put(c - 1, r - 2, AG_SLASH, s ? AM_SNOW : AM_TRUNK); aw_put(c, r - 2, AG_CARET, s ? AM_SNOW : AM_TRUNK);
      aw_put(c + 1, r - 2, AG_BSLASH, s ? AM_SNOW : AM_TRUNK); aw_put(c + 2, r - 2, AG_BAR, AM_ROCK0);
      for (int k = 0; k < 4; k++) {
        int ph = (t / 3 + k * 5) % 20;
        aw_put(c + 2 + ph / 6 + (aw_wind >> 9), r - 3 - ph / 3, ph < 8 ? AG_o : AG_DOT, AM_SMOKE);
      }
    } break;
    case AO_FIRE: {
      int f = (t >> 1) + idx;
      aw_put(c, r, AG_x, AM_TRUNK);
      aw_put(c, r - 1, (f & 1) ? AG_AMP : AG_STAR, (f & 2) ? AM_FIRE1 : AM_FIRE2);
      if (f & 1) aw_put(c, r - 2, AG_CARET, AM_FIRE0);
      for (int k = 0; k < 3; k++) {
        int ph = (t / 2 + k * 7) % 21;
        aw_put(c + ph / 7, r - 2 - ph / 3, AG_DOT, ph < 6 ? AM_FIRE1 : AM_SMOKE);
      }
    } break;
    case AO_FISHER: {
      aw_put(c, r, AG_A, AM_PERSON); aw_put(c, r - 1, AG_o, AM_PERSON);
      int bob = (aw_sin(t * 5) > 0);
      aw_put(c + 1, r - 2, AG_SLASH, AM_ROCK1); aw_put(c + 2, r - 3, AG_DASH, AM_ROCK1);
      aw_put(c + 3, r - 2 + bob, AG_APOS, AM_ROCK1);
      if ((t & 63) < 5) aw_put(c + 3, r + bob, AG_o, AM_FOAM);                    // a ripple
    } break;
    case AO_VOLCANO: {
      int hgt = (4 + aw_TW / 2) / aw_cs;                       // grows with the zoom
      for (int i = 0; i < hgt; i++) {
        int y = r - i, hw = (hgt - i) * 5 / 4 + 1;
        for (int x = -hw; x <= hw; x++) {
          uint32_t h = aw_hash(x, i, 77);
          int g = (h & 3) == 0 ? AG_M : (h & 3) == 1 ? AG_CARET : (h & 3) == 2 ? AG_PCT : AG_COLON;
          int m = x < -hw / 3 ? AM_ROCK1 : x < hw / 3 ? AM_CLIFF2 : AM_ROCK0;
          // lava tongues down the slope
          int lane = (x * 5 + i * 2 + 400) % 7;
          if (lane == 0 && ((i * 3 - (t >> 1)) & 7) < 5) { g = AG_TILDE; m = ((i + (t >> 2)) & 1) ? AM_LAVA1 : AM_LAVA2; }
          aw_put(c + x, y, g, m);
        }
      }
      // crater and eruption
      for (int x = -1; x <= 1; x++) aw_put(c + x, r - hgt, AG_AMP, (t >> 2) & 1 ? AM_LAVA2 : AM_LAVA1);
      int n = 4 + (aw_weather >> 7);
      for (int k = 0; k < n; k++) {
        uint32_t h = aw_hash(k, 3, 3);
        int life = (t + (int)(h & 63)) & 63;
        int dx = ((int)(h >> 8) % 9) - 4;
        int y = r - hgt - 1 - (life * (16 - life / 4)) / 20;
        aw_put(c + dx * life / 24, y, life < 20 ? AG_STAR : AG_DOT, life < 12 ? AM_LAVA2 : life < 30 ? AM_LAVA1 : AM_SMOKE);
      }
    } break;
  }
}

static void aw_drawWalker(int type, int c, int r, int i) {
  int t = aw_t;
  switch (type) {
    case 0: aw_put(c, r, ((t >> 3) + i) & 1 ? AG_A : AG_T, i & 1 ? AM_PERSON : AM_PERSON2); aw_put(c, r - 1, AG_o, i & 1 ? AM_PERSON : AM_PERSON2); break;
    case 1: aw_put(c, r, AG_n, AM_CRIT); aw_put(c + 1, r - 1, ((t >> 4) + i) & 1 ? AG_Y : AG_V, AM_CRIT); break;
    case 2: aw_put(c - 1, r, AG_BSLASH, AM_BOAT); aw_put(c, r, AG_UNDER, AM_BOAT); aw_put(c + 1, r, AG_SLASH, AM_BOAT);
            aw_put(c, r - 1, AG_BAR, AM_TRUNK); aw_put(c + 1, r - 1, AG_GT, AM_SAIL); aw_put(c + 1, r - 2, AG_GT, AM_SAIL);
            if ((t >> 2) & 1) aw_put(c - 2, r, AG_TILDE, AM_FOAM); break;
    case 3: aw_put(c, r, ((t >> 2) + i) & 1 ? AG_SLASH : AG_BSLASH, AM_PERSON2); aw_put(c, r - 1, AG_o, AM_PERSON); break;
    case 4: aw_put(c, r, AG_n, AM_CRIT); aw_put(c + 1, r, AG_n, AM_CRIT); aw_put(c + 2, r - 1, AG_Q, AM_CRIT); break;
  }
}

// birds and weather, in screen space
static void aw_drawSky() {
  int t = aw_t;
  int nb = aw_birds * 8 / 1023;
  for (int i = 0; i < nb; i++) {
    uint32_t h = aw_hash(i, 44, 0);
    int spd = 1 + (int)(h % 3);
    int span = aw_GC + 10;
    int c = (int)((t * spd / 6 + (int)(h % (uint32_t)span)) % span) - 5;
    int r = 1 + (int)((h >> 8) % (uint32_t)(aw_GR / 4 + 1)) + (aw_sin(t * 4 + i * 100) >> 13);
    aw_put(c, r, ((t >> 2) + i) & 1 ? AG_V : AG_DASH, AM_BIRD);
  }
  // weather: rain (k0, k1, k6), snow (k2), ash (k3), heat dust (k5), spray (k4)
  int n = aw_weather * (aw_GC * aw_GR / 12) / 1023;
  if (aw_preset == 6) n += aw_GC * aw_GR / 20;
  int kind = aw_preset == 2 ? 1 : aw_preset == 3 ? 2 : aw_preset == 5 ? 3 : 0;
  int windX = (aw_wind - 512) >> 7;                            // −4 … 3
  for (int i = 0; i < n; i++) {
    uint32_t h = aw_hash(i, 55, 1);
    int spd = kind == 0 ? 2 + (int)(h & 1) : 1;
    int y = (int)((t * spd / (kind == 0 ? 1 : 3) + (int)(h % (uint32_t)(aw_GR * 4))) % (aw_GR * 4)) / 4 * (kind == 0 ? 1 : 1);
    int x = (int)((h >> 8) % (uint32_t)(aw_GC + 20)) - 10 + y * windX / 4;
    if (kind == 1) x += aw_sin(t * 3 + i * 50) >> 13;
    switch (kind) {
      case 0: aw_put(x, y, windX > 0 ? AG_BSLASH : windX < 0 ? AG_SLASH : AG_BAR, AM_RAIN); break;
      case 1: aw_put(x, y, (h >> 20) & 1 ? AG_STAR : AG_DOT, AM_SNOW); break;
      case 2: aw_put(x, y, (h >> 20) & 1 ? AG_DOT : AG_COMMA, (h >> 21) & 1 ? AM_SMOKE : AM_FIRE0); break;
      default: aw_put(x, y, AG_DOT, AM_SAND1); break;
    }
  }
}

// ─── k7: the dungeon ─────────────────────────────────────────────────
#define AW_DW 80
#define AW_DH 40
static uint8_t aw_map[AW_DW * AW_DH];       // bits 0–2 tile, bit 3 seen, bits 4–7 spare
#define aw_dist aw_kind                  // path distances toward the target (shares the ground cache's memory)
enum { AD_ROCK, AD_WALL, AD_FLOOR, AD_DOOR, AD_GOLD, AD_STAIRS, AD_TORCH };
static int aw_hx = 0, aw_hy = 0, aw_tx = 0, aw_ty = 0;
#define AW_NROOM 9
static int16_t aw_rx[AW_NROOM], aw_ry[AW_NROOM], aw_rw[AW_NROOM], aw_rh[AW_NROOM];
static int aw_nRoom = 0, aw_goal = 0;
#define AW_NMON 8
static int16_t aw_mx[AW_NMON], aw_my[AW_NMON];
static uint8_t aw_mk[AW_NMON];

static inline int aw_tile(int x, int y) { return ((unsigned)x < AW_DW && (unsigned)y < AW_DH) ? (aw_map[y * AW_DW + x] & 7) : AD_ROCK; }
static inline void aw_setTile(int x, int y, int k) { if ((unsigned)x < AW_DW && (unsigned)y < AW_DH) aw_map[y * AW_DW + x] = (uint8_t)((aw_map[y * AW_DW + x] & 0xF8) | k); }
static inline bool aw_open(int x, int y) { int k = aw_tile(x, y); return k == AD_FLOOR || k == AD_DOOR || k == AD_GOLD || k == AD_STAIRS; }

static void aw_carve(int x0, int y0, int x1, int y1) {       // an L-shaped corridor
  int x = x0, y = y0;
  while (x != x1) { if (aw_tile(x, y) != AD_FLOOR) aw_setTile(x, y, AD_FLOOR); x += x < x1 ? 1 : -1; }
  while (y != y1) { if (aw_tile(x, y) != AD_FLOOR) aw_setTile(x, y, AD_FLOOR); y += y < y1 ? 1 : -1; }
}

static void aw_pathTo(int tx, int ty) {                      // flood fill of step counts from the target
  memset(aw_dist, 255, sizeof(aw_dist));
  static uint16_t q[512];                                    // ring buffer (the frontier stays small)
  int qh = 0, qt = 0;
  aw_dist[ty * AW_DW + tx] = 0; q[qt++ & 511] = (uint16_t)(ty * AW_DW + tx);
  while (qh < qt) {
    int p = q[qh++ & 511], x = p % AW_DW, y = p / AW_DW, d = aw_dist[p];
    if (d >= 254) continue;
    static const int8_t dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    for (int k = 0; k < 4; k++) {
      int nx = x + dx[k], ny = y + dy[k];
      if (!aw_open(nx, ny)) continue;
      int np = ny * AW_DW + nx;
      if (aw_dist[np] != 255) continue;
      aw_dist[np] = (uint8_t)(d + 1);
      if (qt - qh < 511) q[qt++ & 511] = (uint16_t)np;
    }
  }
}

static void aw_genDungeon() {
  aw_rng = aw_seed * 2246822519u + 5;
  if (!aw_rng) aw_rng = 1;
  memset(aw_map, AD_ROCK, sizeof(aw_map));
  aw_nRoom = 0;
  for (int tries = 0; tries < 200 && aw_nRoom < AW_NROOM; tries++) {
    int w = 6 + (int)(aw_rand() % 10), h = 4 + (int)(aw_rand() % 6);
    int x = 1 + (int)(aw_rand() % (uint32_t)(AW_DW - w - 2)), y = 1 + (int)(aw_rand() % (uint32_t)(AW_DH - h - 2));
    bool ok = true;
    for (int j = 0; j < aw_nRoom && ok; j++)
      if (x < aw_rx[j] + aw_rw[j] + 2 && x + w + 2 > aw_rx[j] && y < aw_ry[j] + aw_rh[j] + 2 && y + h + 2 > aw_ry[j]) ok = false;
    if (!ok) continue;
    aw_rx[aw_nRoom] = (int16_t)x; aw_ry[aw_nRoom] = (int16_t)y; aw_rw[aw_nRoom] = (int16_t)w; aw_rh[aw_nRoom] = (int16_t)h;
    aw_nRoom++;
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) aw_setTile(xx, yy, AD_FLOOR);
  }
  for (int j = 1; j < aw_nRoom; j++)
    aw_carve(aw_rx[j - 1] + aw_rw[j - 1] / 2, aw_ry[j - 1] + aw_rh[j - 1] / 2, aw_rx[j] + aw_rw[j] / 2, aw_ry[j] + aw_rh[j] / 2);
  // walls round every floor
  for (int y = 0; y < AW_DH; y++) for (int x = 0; x < AW_DW; x++) {
    if (aw_tile(x, y) != AD_ROCK) continue;
    bool nearF = false;
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) if (aw_tile(x + dx, y + dy) == AD_FLOOR) nearF = true;
    if (nearF) aw_setTile(x, y, (aw_rand() % 23) == 0 ? AD_TORCH : AD_WALL);
  }
  // doors where corridors meet rooms, gold, the stairs down
  for (int j = 0; j < aw_nRoom; j++) {
    for (int k = 0; k < 2; k++) {
      int gx = aw_rx[j] + 1 + (int)(aw_rand() % (uint32_t)(aw_rw[j] - 2)), gy = aw_ry[j] + 1 + (int)(aw_rand() % (uint32_t)(aw_rh[j] - 2));
      if (aw_rand() & 1) aw_setTile(gx, gy, AD_GOLD);
    }
  }
  if (aw_nRoom) aw_setTile(aw_rx[aw_nRoom - 1] + aw_rw[aw_nRoom - 1] - 2, aw_ry[aw_nRoom - 1] + 1, AD_STAIRS);
  aw_hx = aw_rx[0] + 1; aw_hy = aw_ry[0] + 1;
  aw_goal = 1 % (aw_nRoom ? aw_nRoom : 1);
  aw_tx = aw_rx[aw_goal] + aw_rw[aw_goal] / 2; aw_ty = aw_ry[aw_goal] + aw_rh[aw_goal] / 2;
  aw_pathTo(aw_tx, aw_ty);
  static const uint8_t kinds[5] = {AG_g, AG_r, AG_k, AG_S, AG_D};
  for (int i = 0; i < AW_NMON; i++) {
    int j = 1 + (int)(aw_rand() % (uint32_t)(aw_nRoom > 1 ? aw_nRoom - 1 : 1));
    if (j >= aw_nRoom) j = aw_nRoom - 1;
    aw_mx[i] = (int16_t)(aw_rx[j] + (int)(aw_rand() % (uint32_t)aw_rw[j]));
    aw_my[i] = (int16_t)(aw_ry[j] + (int)(aw_rand() % (uint32_t)aw_rh[j]));
    aw_mk[i] = kinds[aw_rand() % 5];
  }
}

static void aw_stepDungeon() {
  // the hero follows the path to the next room; then picks another
  if (aw_hx == aw_tx && aw_hy == aw_ty) {
    if (aw_tile(aw_hx, aw_hy) == AD_STAIRS) { aw_seed++; aw_genDungeon(); return; }
    aw_goal = (aw_goal + 1 + (int)(aw_rand() % 3)) % aw_nRoom;
    bool allSeen = true;
    for (int j = 0; j < aw_nRoom; j++) if (!(aw_map[(aw_ry[j] + 1) * AW_DW + aw_rx[j] + 1] & 8)) { allSeen = false; aw_goal = j; break; }
    if (allSeen) {                                            // explored everything: head for the stairs
      aw_goal = aw_nRoom - 1;
      aw_tx = aw_rx[aw_goal] + aw_rw[aw_goal] - 2; aw_ty = aw_ry[aw_goal] + 1;
    } else { aw_tx = aw_rx[aw_goal] + aw_rw[aw_goal] / 2; aw_ty = aw_ry[aw_goal] + aw_rh[aw_goal] / 2; }
    aw_pathTo(aw_tx, aw_ty);
  }
  static const int8_t dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
  int best = aw_dist[aw_hy * AW_DW + aw_hx], bk = -1;
  for (int k = 0; k < 4; k++) {
    int nx = aw_hx + dx[k], ny = aw_hy + dy[k];
    if (!aw_open(nx, ny)) continue;
    int d = aw_dist[ny * AW_DW + nx];
    if (d < best) { best = d; bk = k; }
  }
  if (bk >= 0) { aw_hx += dx[bk]; aw_hy += dy[bk]; }
  else { aw_tx = aw_hx; aw_ty = aw_hy; }
  if (aw_tile(aw_hx, aw_hy) == AD_GOLD) aw_setTile(aw_hx, aw_hy, AD_FLOOR);   // picks up the gold
  for (int j = 0; j < aw_nRoom; j++)                          // walking into a room lights it all up
    if (aw_hx >= aw_rx[j] && aw_hx < aw_rx[j] + aw_rw[j] && aw_hy >= aw_ry[j] && aw_hy < aw_ry[j] + aw_rh[j])
      for (int y = aw_ry[j] - 1; y <= aw_ry[j] + aw_rh[j]; y++)
        for (int x = aw_rx[j] - 1; x <= aw_rx[j] + aw_rw[j]; x++)
          if ((unsigned)x < AW_DW && (unsigned)y < AW_DH && (aw_map[y * AW_DW + x] & 7) != AD_ROCK) aw_map[y * AW_DW + x] |= 8;
  // monsters shuffle about
  int nm = 1 + aw_life * (AW_NMON - 1) / 1023;
  for (int i = 0; i < nm; i++) {
    if (aw_rand() & 1) continue;
    int k = (int)(aw_rand() & 3);
    int nx = aw_mx[i] + dx[k], ny = aw_my[i] + dy[k];
    if (aw_open(nx, ny) && !(nx == aw_hx && ny == aw_hy)) { aw_mx[i] = (int16_t)nx; aw_my[i] = (int16_t)ny; }
  }
}

static void aw_drawDungeon() {
  int t = aw_t;
  // view: whole map when the letters are small, else a window following @
  int ox = 0, oy = 0;
  if (aw_GC < AW_DW) { ox = aw_hx - aw_GC / 2; if (ox < 0) ox = 0; if (ox > AW_DW - aw_GC) ox = AW_DW - aw_GC; }
  if (aw_GR < AW_DH) { oy = aw_hy - aw_GR / 2; if (oy < 0) oy = 0; if (oy > AW_DH - aw_GR) oy = AW_DH - aw_GR; }
  int light = 5 + (aw_water >> 7);                               // how far the torchlight reaches (p4)
  for (int r = 0; r < aw_GR; r++)
    for (int c = 0; c < aw_GC; c++) {
      int x = c + ox, y = r + oy, i = r * aw_GC + c;
      aw_gch[i] = AG_SP; aw_gfg[i] = 0; aw_gbg[i] = 0;
      if ((unsigned)x >= AW_DW || (unsigned)y >= AW_DH) continue;
      int dx = x - aw_hx, dy = (y - aw_hy) * 2;
      bool lit = dx * dx + dy * dy <= light * light;
      uint8_t* cell = &aw_map[y * AW_DW + x];
      if (lit && (*cell & 7) != AD_ROCK) *cell |= 8;              // remember what's been seen
      if (!(*cell & 8) && !lit) continue;
      int k = *cell & 7, g = AG_SP, m = AM_FLOOR;
      uint32_t h = aw_hash(x, y, 3);
      switch (k) {
        case AD_WALL: g = AG_HASH; m = AM_WALL; break;
        case AD_TORCH: g = AG_BANG; m = ((t >> 2) + (int)h) & 1 ? AM_FIRE1 : AM_FIRE2; break;
        case AD_FLOOR: g = (h & 3) ? AG_DOT : AG_SP; m = AM_FLOOR; break;
        case AD_DOOR: g = AG_PLUS; m = AM_DOOR; break;
        case AD_GOLD: g = AG_DOLLAR; m = AM_GOLD; break;
        case AD_STAIRS: g = AG_GT; m = AM_STAR; break;
      }
      if (!lit) m = (k == AD_WALL || k == AD_TORCH) ? AM_CLIFF1 : AM_CLIFF0;   // remembered, dim
      aw_gch[i] = (uint8_t)g; aw_gfg[i] = (uint8_t)m;
      if (lit) aw_gbg[i] = (uint8_t)((k == AD_WALL || k == AD_TORCH) ? AM_BG_WALL : AM_BG_FLOOR);
    }
  int nm = 1 + aw_life * (AW_NMON - 1) / 1023;
  for (int i = 0; i < nm; i++) {
    int dx = aw_mx[i] - aw_hx, dy = (aw_my[i] - aw_hy) * 2;
    if (dx * dx + dy * dy > light * light) continue;              // only seen by torchlight
    aw_put(aw_mx[i] - ox, aw_my[i] - oy, aw_mk[i], AM_MONST);
    if (dx * dx + dy * dy <= 4 && ((t >> 1) & 1)) aw_put(aw_mx[i] - ox, aw_my[i] - oy, AG_STAR, AM_FIRE2);   // a scuffle!
  }
  aw_put(aw_hx - ox, aw_hy - oy, AG_AT, AM_HERO);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_ascii_name() { return "ASCII WORLDS"; }
const char* prog_ascii_character() { return "Tiny isometric islands and a dungeon, drawn entirely in text characters"; }

static const char* const aw_presetNames[] = {
  "Lake", "River", "Winter", "Volcano", "Archipelago", "Oasis", "Night Rain", "Dungeon"
};
#define AW_NUM_PRESETS 8

const char* prog_ascii_presetName(int preset) {
  if (preset >= 0 && preset < AW_NUM_PRESETS) return aw_presetNames[preset];
  return NULL;
}

static const char* const aw_labels[16] = {
  "Palette", "Speed", "Zoom", "Spin", "Water", "Trees", "Life", "Weather",
  "Wind", "Birds", "Char Size", "Height", "Time of Day", "Pop", "Glitch", ""
};
const char* prog_ascii_potLabel(int preset, int pot) {
  if (preset == 7 && pot == 4) return "Torchlight";
  if (preset == 7 && pot == 6) return "Monsters";
  if (pot >= 0 && pot < 16) return aw_labels[pot];
  return "";
}

uint8_t prog_ascii_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;
}

void prog_ascii_init() {
  static bool ready = false;
  if (ready) return;
  ready = true;
  for (int i = 0; i < 1024; i++) aw_sinT[i] = (int16_t)lroundf(sinf(i * 6.2831853f / 1024.0f) * 16384.0f);
  for (int g = 0; g < AG_N; g++) {
    uint16_t b = 0;
    for (int k = 0; k < 15; k++) if (aw_fontSrc[g][k] == '1') b |= (uint16_t)(1 << (14 - k));
    aw_font[g] = b;
  }
}

void prog_ascii_draw(int preset) {
  if (aw_sinT[256] == 0) prog_ascii_init();
  uint8_t* buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!aw_smReady) aw_sm[i] = pots[i];
      else aw_sm[i] += (pots[i] - aw_sm[i]) * a;
    }
    aw_smReady = true;
  }

  float sk = aw_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);
  static float tf = 0, stepAcc = 0, rotF = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e7f) tf = 0;
  aw_t = (int)tf;
  stepAcc += dt * 60.0f * speed;
  int steps = (int)stepAcc;
  stepAcc -= steps;
  if (steps > 4) steps = 4;

  // knobs
  aw_cs = aw_k(10) < 512 ? 1 : 2;
  aw_cw = 4 * aw_cs; aw_chh = 6 * aw_cs;
  aw_GC = W / aw_cw; aw_GR = H / aw_chh;
  aw_TW = 6 + aw_k(2) * 16 / 1023;                              // 6 … 22 px tiles (centre = 14)
  float sp = aw_sm[3] / 1023.0f;
  rotF += dt * (sp < 0.03f ? 0.0f : sp * sp * 0.25f) * 1024.0f;   // up to a quarter turn a second
  if (rotF > 1024.0f) rotF -= 1024.0f;
  aw_rot = (int)rotF;
  aw_ca = aw_cos(aw_rot); aw_sa = aw_sin(aw_rot);
  aw_water = aw_k(4);
  aw_trees = aw_k(5);
  aw_life = aw_k(6);
  aw_weather = aw_k(7);
  aw_wind = aw_k(8);
  aw_birds = aw_k(9);
  aw_height = 1 + aw_k(11) * 7 / 1023;                           // 1 … 8 rows of island side
  aw_tod = aw_k(12);
  aw_glitch = aw_k(14);
  aw_cx = W / 2; aw_cy = H / 2 - aw_height * aw_chh / 3;

  // a new world when the preset changes, or on k12
  static int lastPreset = -1;
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  bool regen = preset != lastPreset || (k12 && !k12Was);
  if (k12 && !k12Was) aw_seed++;
  k12Was = k12;
  aw_preset = preset;
  if (regen) {
    lastPreset = preset;
    if (preset == 7) aw_genDungeon(); else aw_generate();
    aw_kindKey = 0xFFFFFFFFu;
  }

  // palette: p0 scheme (centre = Original), p12 time of day, p13 pop; lightning in Night Rain
  static const uint8_t order[AW_NSCHEME] = {1, 2, 7, 5, 0, 3, 4, 6};   // knob: Green CRT, Amber, C64, Ice, Original (centre), Autumn, Vapor, Blood Moon
  int pos = aw_k(0) * AW_NSCHEME / 1024;
  if (pos > AW_NSCHEME - 1) pos = AW_NSCHEME - 1;
  int tod = aw_tod;
  if (preset == 6) tod = tod < 512 ? tod / 3 : 1023 - (1023 - tod) / 3;   // mostly night
  if (preset == 7) tod = 512;
  static int flashT = 0;
  if (flashT > 0) flashT--;
  else if (preset == 6 && speed > 0 && (aw_rand() % 1000) < 3 + aw_weather / 100) flashT = 4;
  aw_buildPalette(order[pos], tod, aw_k(13), flashT > 0 && (flashT & 1));

  if (preset == 7) {
    static float mvAcc = 0;
    mvAcc += dt * speed * 14.0f;
    while (mvAcc >= 1.0f) { aw_stepDungeon(); mvAcc -= 1.0f; }
    aw_drawDungeon();
  } else {
    aw_moveWalkers(steps);
    aw_drawGround();
    // gather trees, scenery and wanderers, sort far → near, and draw
    static int16_t dep[AW_NOBJ + AW_NWALK];
    static uint8_t ord[AW_NOBJ + AW_NWALK];
    int n = 0;
    int nt = aw_nTrees + (aw_nObj - aw_nTrees) * aw_trees / 1023;
    for (int j = 0; j < nt; j++) {
      int sx, sy, d; aw_toScreen(aw_ou[j], aw_ov[j], &sx, &sy, &d);
      dep[n] = (int16_t)d; ord[n] = (uint8_t)j; n++;
    }
    int nw = aw_life * AW_NWALK / 1023;
    for (int j = 0; j < nw; j++) {
      int sx, sy, d; aw_toScreen(aw_wu[j], aw_wv[j], &sx, &sy, &d);
      dep[n] = (int16_t)d; ord[n] = (uint8_t)(AW_NOBJ + j); n++;
    }
    for (int a = 1; a < n; a++) {                                  // insertion sort by depth
      int16_t dv = dep[a]; uint8_t ov = ord[a]; int b = a - 1;
      while (b >= 0 && dep[b] > dv) { dep[b + 1] = dep[b]; ord[b + 1] = ord[b]; b--; }
      dep[b + 1] = dv; ord[b + 1] = ov;
    }
    for (int a = 0; a < n; a++) {
      int j = ord[a], sx, sy, d;
      if (j < AW_NOBJ) {
        aw_toScreen(aw_ou[j], aw_ov[j], &sx, &sy, &d);
        aw_drawObj(aw_ot[j], aw_os[j], sx / aw_cw, sy / aw_chh, j);
      } else {
        j -= AW_NOBJ;
        aw_toScreen(aw_wu[j], aw_wv[j], &sx, &sy, &d);
        aw_drawWalker(aw_wt[j], sx / aw_cw, sy / aw_chh, j);
      }
    }
    aw_drawSky();
  }

  // glitch: characters flicker to random ones
  if (aw_glitch > 20) {
    int ng = aw_glitch * aw_GC * aw_GR / 8000;
    for (int i = 0; i < ng; i++) {
      int c = (int)(aw_rand() % (uint32_t)aw_GC), r = (int)(aw_rand() % (uint32_t)aw_GR);
      int k = r * aw_GC + c;
      if (!aw_gfg[k]) continue;
      aw_gch[k] = (uint8_t)(1 + aw_rand() % (AG_N - 1));
      if (aw_glitch > 700) aw_gfg[k] = (uint8_t)(1 + aw_rand() % (AM_N - 4));
    }
  }
  aw_renderGrid(buf);
}

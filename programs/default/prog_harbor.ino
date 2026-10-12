// =====================================================================
// PROGRAM: HARBOR NIGHT (by Dewey)
// A cozy little harbor in crisp pixel art: a plank dock running out over
// dark water, a fisherman in a yellow raincoat at the end of it with a
// fire barrel and a lantern, a hillside village across the bay with warm
// windows and chimney smoke, mountains behind, a red-and-white lighthouse
// sweeping its beam, the moon, stars and drifting snow. The fisherman
// fishes: the bobber nibbles, goes under, the rod bends, a fish leaps out
// and the catch counter on the little game-style status panel goes up.
// Turn Style to the right to see the whole thing rebuilt from ASCII
// characters. Inspired by the vibe of ascii_and_pixels' "The Fisherman"
// (https://www.instagram.com/p/Dd7uLlTRypw/). The scene and code are original.
//
// Presets:
//   k0 Snow Harbor   — snowy night, fire barrel, lantern, village lights
//   k1 Dawn Mist     — pink dawn, mist on the water, gulls
//   k2 Storm         — rain, lightning, big waves, the lighthouse working hard
//   k3 Ice Fishing   — the bay frozen over, fishing through a hole, heavy snow
//   k4 Aurora        — clear cold night, northern lights over the bay
//   k5 Summer Dusk   — the sun going down, fireflies, warm water
//   k6 Close Up      — zoomed in on the fisherman, the fish bite often
//   k7 Lighthouse    — the camera on the point and its sweeping beam
//
//   k12 — FISH! (a bite right now)
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8 looks, centre = Original colours)   p1 Speed (far left = freeze)
//   p2 Style: left 60 % = crisp pixel art; right 40 % = ASCII characters
//      (further right = less background behind the characters)
//   p3 Char Size (ASCII only: left = fine 3×5, middle = 4×6, right = big)   p4 Time (sky light)
//   p5 Weather (snow / rain amount)   p6 Wind   p7 Waves   p8 Village Lights
//   p9 Bite Rate   p10 Lighthouse   p11 Status Panel (off / panel / + icons)
//   p12 Fire   p13 Pan   p14 Zoom (left half = whole scene, right half 1× … 3×)
//   p15 Fog
//
// How it works: the scene is drawn as pixel art in "world" pixels (the
// whole harbor is 320×240 at zoom 1), through a zoom/pan camera: dithered
// sky, mountains, houses with outlined roofs and framed windows, the
// lighthouse, sea with wave dashes and reflections, the dock, the fire,
// and a hand-drawn fisherman sprite with a dark outline. Every pixel is
// (shade << 4) | colour; colour 0 is the dark outline ink. In ASCII style
// each character cell is then replaced by a glyph chosen by its brightness.
// =====================================================================

// ─── Font: 3×5 glyphs (15 bits, top-left first) ───────────────────────
static const char hb_fontChars[] = " .,:;-=+*#%@~'|/\\^o0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$!?><";
static const uint16_t hb_fontBits[60] = {
  0x0000,0x0002,0x0014,0x0410,0x0414,0x01C0,0x0E38,0x05D0,0x55D5,0x5F7D,0x5455,0x7F7F,0x00F0,0x2400,0x2492,0x12A4,0x4889,0x2A00,0x0550,0x7B6F,0x2C97,0x73E7,0x72CF,0x5BC9,0x79CF,0x79EF,0x7292,0x7BEF,0x7BCF,0x2BED,0x6BAE,0x3923,0x6B6E,0x79A7,0x79A4,0x396B,0x5BED,0x7497,0x126A,0x5BAD,0x4927,0x5FED,0x6B6D,0x2B6A,0x6BA4,0x2B73,0x6BAD,0x388E,0x7492,0x5B6F,0x5B6A,0x5BFD,0x5AAD,0x5A92,0x72A7,0x3C9E,0x2482,0x6282,0x4454,0x1511,
};
static int hb_gi(char c) {                     // glyph index of a character (0 = space)
  for (int i = 0; hb_fontChars[i]; i++) if (hb_fontChars[i] == c) return i;
  return 0;
}
static uint8_t hb_ramp[10];                     // glyph indices for " .:-=+*#%@"

// ─── Maths ────────────────────────────────────────────────────────────
#include "vg_sin.h"
#define hb_sinT vg_sin256                        // shared sine table (in flash: no RAM)
static inline int hb_sin(int a) { return hb_sinT[a & 255]; }
static inline int hb_cos(int a) { return hb_sinT[(a + 64) & 255]; }
static inline uint32_t hb_hash(int a, int b) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static inline int hb_floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static inline int hb_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }


// ─── Colours ──────────────────────────────────────────────────────────
// colour ids (low nibble of a pixel); the shade is the high nibble
#define HB_INK    0      // outlines: always near-black navy
#define HB_VOID   0
#define HB_SKY    1
#define HB_MOON   2
#define HB_FAR    3      // mountains, far hills, rocks
#define HB_WIN    4      // warm window / lamp light
#define HB_SEA    5
#define HB_FOAM   6
#define HB_WOOD   7
#define HB_COAT   8
#define HB_SKIN   9
#define HB_DARK   10     // trousers, boots, slate roofs
#define HB_FIRE   11
#define HB_SNOW   12
#define HB_AUR    13
#define HB_RED    14
#define HB_HUD    15

#define HB_NPRESET 8
// per preset: sky, sea, far-shore colours
static const uint8_t hb_presetCols[HB_NPRESET][3][3] = {
  {{40,60,135},  {30,60,125},  {70,85,130}},   // Snow Harbor
  {{250,150,165},{140,130,175},{120,95,135}},  // Dawn Mist
  {{75,85,110},  {45,65,85},   {65,70,90}},    // Storm
  {{60,85,160},  {175,205,240},{80,95,140}},   // Ice Fishing (sea = ice)
  {{20,45,105},  {25,55,115},  {55,70,110}},   // Aurora
  {{255,125,80}, {110,75,140}, {120,70,100}},  // Summer Dusk
  {{40,60,135},  {30,60,125},  {70,85,130}},   // Close Up
  {{35,55,125},  {28,55,115},  {65,80,125}},   // Lighthouse
};
// shared colours
static const uint8_t hb_baseCols[16][3] = {
  {14,14,30},{0,0,0},{255,244,200},{0,0,0},{255,195,85},{0,0,0},{205,228,255},{160,100,58},
  {255,200,35},{240,180,140},{58,66,98},{255,120,25},{238,243,255},{70,255,160},{215,45,50},{255,236,195},
};

#define HB_NSCHEME 8
static const char* const hb_schemeNames[HB_NSCHEME] = {
  "Mono", "Green CRT", "Amber", "Sepia", "Original", "Ice", "Vapor", "Pocket"
};
static const uint8_t hb_monoRamp[HB_NSCHEME][2][3] = {
  {{0,0,0},{255,255,255}}, {{0,10,0},{120,255,140}}, {{10,4,0},{255,190,70}}, {{12,8,4},{255,225,180}},
  {{0,0,0},{0,0,0}}, {{0,6,20},{180,230,255}}, {{0,0,0},{0,0,0}}, {{15,56,15},{155,188,15}},
};
static int hb_flash = 0;

// shade l (1..15) of a colour: 1-11 dark → full colour, 12-15 toward white
static void hb_buildPalette(int scheme, int preset, int timeShift) {
  static const uint8_t gain[12] = {0, 30, 50, 70, 92, 114, 136, 160, 184, 208, 232, 255};
  for (int c = 0; c < 16; c++) {
    int r, g, b;
    if (c == HB_SKY || c == HB_SEA || c == HB_FAR) {
      int k = c == HB_SKY ? 0 : (c == HB_SEA ? 1 : 2);
      r = hb_presetCols[preset][k][0]; g = hb_presetCols[preset][k][1]; b = hb_presetCols[preset][k][2];
      if (timeShift > 0) { r += (255 - r) * timeShift >> 9; g += (200 - g) * timeShift >> 10; }
      else if (timeShift < 0) { int t = -timeShift; r -= r * t >> 9; g -= g * t >> 9; b += (255 - b) * t >> 11; }
    } else { r = hb_baseCols[c][0]; g = hb_baseCols[c][1]; b = hb_baseCols[c][2]; }
    r = hb_clamp(r, 0, 255); g = hb_clamp(g, 0, 255); b = hb_clamp(b, 0, 255);
    for (int l = 0; l < 16; l++) {
      int rr, gg, bb;
      if (c == HB_INK) { rr = r; gg = g; bb = b; if (l == 0) rr = gg = bb = 0; }
      else if (l == 0) { rr = gg = bb = 0; }
      else if (l <= 11) { int m = gain[l]; rr = r * m >> 8; gg = g * m >> 8; bb = b * m >> 8; }
      else { int wv = (l - 11) * 34; rr = r + ((255 - r) * wv >> 8); gg = g + ((255 - g) * wv >> 8); bb = b + ((255 - b) * wv >> 8); }
      if (scheme == 6) {                                 // Vapor: toward pink and cyan
        int nr = (rr * 3 + bb * 2) / 4, ng = (gg * 2 + bb) / 4, nb = (bb * 3 + rr) / 4 + (l ? 30 : 0);
        rr = hb_clamp(nr, 0, 255); gg = hb_clamp(ng, 0, 255); bb = hb_clamp(nb, 0, 255);
      } else if (scheme != 4) {                          // one-colour looks, by brightness
        int y = (rr * 77 + gg * 150 + bb * 29) >> 8;
        if (scheme == 7) y = (y >> 6) * 85;
        const uint8_t* d = hb_monoRamp[scheme][0];
        const uint8_t* e = hb_monoRamp[scheme][1];
        rr = d[0] + ((e[0] - d[0]) * y >> 8); gg = d[1] + ((e[1] - d[1]) * y >> 8); bb = d[2] + ((e[2] - d[2]) * y >> 8);
      }
      if (hb_flash && l) { rr = (rr + 230) >> 1; gg = (gg + 235) >> 1; bb = (bb + 255) >> 1; }
      display.setColor((l << 4) | c, rr, gg, bb);
    }
  }
}
#define HB_V(c, l) ((uint8_t)(((l) << 4) | (c)))

// ─── Camera and drawing (all in world pixels) ─────────────────────────
// a 4×4 ordered dither threshold (0..15)
static const uint8_t hb_bayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
static uint8_t* hb_buf;
static int hb_t = 0, hb_preset = 0;
static int hb_zq = 256;                  // zoom, Q8
static int hb_vx = HALFW, hb_vy = HALFH; // world point at the screen centre
static int hb_waves = 512, hb_lights = 512, hb_beam = 512, hb_fire = 512, hb_fog = 0, hb_weather = 512, hb_wind = 512;
static int hb_lightning = 0;
static bool hb_frozen = false, hb_snowy = true;

static inline int hb_X(int wx) { return HALFW + (((wx - hb_vx) * hb_zq) >> 8); }
static inline int hb_Y(int wy) { return HALFH + (((wy - hb_vy) * hb_zq) >> 8); }
static inline int hb_worldX(int sx) { return hb_vx + (((sx - HALFW) * 256) / hb_zq); }
static inline int hb_worldY(int sy) { return hb_vy + (((sy - HALFH) * 256) / hb_zq); }

static inline void hb_spx(int sx, int sy, uint8_t v) {         // one screen pixel
  if ((unsigned)sx < (unsigned)W && (unsigned)sy < (unsigned)H) hb_buf[sy * W + sx] = v;
}
// world rectangle [x0,x1) × [y0,y1)
static void hb_rect(int x0, int y0, int x1, int y1, uint8_t v) {
  int sx0 = hb_X(x0), sx1 = hb_X(x1), sy0 = hb_Y(y0), sy1 = hb_Y(y1);
  if (sx0 < 0) sx0 = 0;
  if (sy0 < 0) sy0 = 0;
  if (sx1 > W) sx1 = W;
  if (sy1 > H) sy1 = H;
  if (sx1 <= sx0 || sy1 <= sy0) return;
  for (int y = sy0; y < sy1; y++) memset(hb_buf + y * W + sx0, v, sx1 - sx0);
}
static inline void hb_dot(int wx, int wy, uint8_t v) { hb_rect(wx, wy, wx + 1, wy + 1, v); }
// outlined rectangle: fill + 1-world-pixel ink border
static void hb_orect(int x0, int y0, int x1, int y1, uint8_t fill) {
  hb_rect(x0, y0, x1, y1, HB_V(HB_INK, 1));
  hb_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, fill);
}
// a line in world pixels (thickness grows with zoom)
static void hb_line(int x0, int y0, int x1, int y1, uint8_t v) {
  int sx0 = hb_X(x0), sy0 = hb_Y(y0), sx1 = hb_X(x1), sy1 = hb_Y(y1);
  if ((sx0 < 0 && sx1 < 0) || (sx0 >= W && sx1 >= W) || (sy0 < 0 && sy1 < 0) || (sy0 >= H && sy1 >= H)) return;
  int th = hb_zq >> 8; if (th < 1) th = 1;
  int dx = abs(sx1 - sx0), sx = sx0 < sx1 ? 1 : -1;
  int dy = -abs(sy1 - sy0), sy = sy0 < sy1 ? 1 : -1;
  int err = dx + dy, n = 0;
  for (;;) {
    for (int a = 0; a < th; a++) for (int b = 0; b < th; b++) hb_spx(sx0 + a, sy0 + b, v);
    if ((sx0 == sx1 && sy0 == sy1) || ++n > 1500) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; sx0 += sx; }
    if (e2 <= dx) { err += dx; sy0 += sy; }
  }
}
// filled disc (world centre / radius); outline ink ring if ink
static void hb_disc(int cx, int cy, int r, uint8_t v, bool ink) {
  int scx = hb_X(cx), scy = hb_Y(cy), sr = (r * hb_zq) >> 8;
  if (sr < 1) sr = 1;
  if (scx + sr < 0 || scx - sr >= W || scy + sr < 0 || scy - sr >= H) return;
  int r2 = sr * sr + sr, ri = (sr - 1) * (sr - 1) + (sr - 1);
  for (int y = -sr; y <= sr; y++)
    for (int x = -sr; x <= sr; x++) {
      int d = x * x + y * y;
      if (d <= r2) hb_spx(scx + x, scy + y, (ink && d > ri) ? HB_V(HB_INK, 1) : v);
    }
}
// soft light: a dithered halo that thins out toward its edge (only lifts darker pixels)
static void hb_glow(int cx, int cy, int r, int col, int lvl) {
  int scx = hb_X(cx), scy = hb_Y(cy), sr = (r * hb_zq) >> 8;
  if (sr < 2) return;
  int r2 = sr * sr;
  int y0 = scy - sr < 0 ? 0 : scy - sr, y1 = scy + sr >= H ? H - 1 : scy + sr;
  int x0 = scx - sr < 0 ? 0 : scx - sr, x1 = scx + sr >= W ? W - 1 : scx + sr;
  for (int y = y0; y <= y1; y++) {
    uint8_t* row = hb_buf + y * W;
    int dy = y - scy;
    for (int x = x0; x <= x1; x++) {
      int dx = x - scx, d2 = dx * dx + dy * dy;
      if (d2 >= r2) continue;
      int inten = 256 - (d2 << 8) / r2;                         // 256 centre … 0 edge
      inten = inten * inten >> 8;                               // falls off fast
      if ((hb_bayer[(y & 3) * 4 + (x & 3)] << 4) >= inten) continue;   // thinner further out
      int l = 1 + (lvl * inten >> 8);
      if (l > (row[x] >> 4) && (row[x] & 15) != HB_INK) row[x] = HB_V(col, l);
    }
  }
}
// sprite: rows of characters, each a colour code (or '.' = see-through), drawn s world px per char
static void hb_sprite(const char* const* rows, int n, int w, int wx, int wy, int s, bool flip) {
  int sx0 = hb_X(wx), sy0 = hb_Y(wy), sx1 = hb_X(wx + w * s), sy1 = hb_Y(wy + n * s);
  if (sx1 <= 0 || sx0 >= W || sy1 <= 0 || sy0 >= H) return;
  for (int sy = sy0 < 0 ? 0 : sy0; sy < sy1 && sy < H; sy++) {
    int v = (sy - sy0) * n / (sy1 - sy0);
    const char* row = rows[v];
    for (int sx = sx0 < 0 ? 0 : sx0; sx < sx1 && sx < W; sx++) {
      int u = (sx - sx0) * w / (sx1 - sx0);
      if (flip) u = w - 1 - u;
      char c = row[u];
      uint8_t px;
      switch (c) {
        case 'k': px = HB_V(HB_INK, 1); break;
        case 'Y': px = HB_V(HB_COAT, 11); break;
        case 'y': px = HB_V(HB_COAT, 7); break;
        case 'h': px = HB_V(HB_COAT, 13); break;
        case 'S': px = HB_V(HB_SKIN, 11); break;
        case 's': px = HB_V(HB_SKIN, 8); break;
        case 'P': px = HB_V(HB_DARK, 9); break;
        case 'p': px = HB_V(HB_DARK, 6); break;
        case 'B': px = HB_V(HB_DARK, 5); break;
        case 'b': px = HB_V(HB_DARK, 9); break;
        case 'W': px = HB_V(HB_FOAM, 12); break;    // fish belly / white
        case 'F': px = HB_V(HB_FOAM, 9); break;     // fish back
        case 'O': px = HB_V(HB_FIRE, 11); break;    // orange fish
        case 'o': px = HB_V(HB_FIRE, 7); break;
        case 'R': px = HB_V(HB_RED, 10); break;
        case 'w': px = HB_V(HB_SNOW, 13); break;
        default: continue;
      }
      hb_buf[sy * W + sx] = px;
    }
  }
}

// ─── Sprites ──────────────────────────────────────────────────────────
// the fisherman, facing right, rod held out in front (drawn 2 world px per char)
static const char* const hb_man[28] = {
  ".......kkkkk........",
  ".....kkhYYYYkk......",
  "....khhYYYYYYYk.....",
  "...kYYYYYYYYYYYk....",
  "..kkkkkkkYYYkkkkk...",
  "......kSSSSSk.......",
  "......kSSSkSk.......",
  "......kSSSSSk.......",
  ".......ksSSk........",
  ".....kkYYYYYkk......",
  "....khYYYYYYYYk.....",
  "...khYyYYYYYYYYk....",
  "...kYYyYYYYYYkkkk...",
  "...kYYyYYYYYYYkSSk..",
  "...kYYyYYYYYYkkkk...",
  "...kYYyYYYYYYYk.....",
  "...kYYyYYYYYYYk.....",
  "...kYYyYYYYYYYk.....",
  "...kYYyYYYYYYyk.....",
  "...kYYyYYYYYYyk.....",
  "...kyyyyyyyyyyk.....",
  "....kPPpkPPPpk......",
  "....kPPpkPPPpk......",
  "....kPPpkPPPpk......",
  "....kPPpkPPPpk......",
  "....kBBBkBBBbk......",
  "...kBBBBkBBBBbk.....",
  "...kkkkkkkkkkkk.....",
};
// reeling: arms pulled up and back
static const char* const hb_manReel[3] = {
  "...kYYyYYYYYkkSSk...",
  "...kYYyYYYYYYkkkk...",
  "...kYYyYYYYYYYk.....",
};
static const char* const hb_fishSpr[5] = {
  "....kkkk..k",
  "..kkFFFFkkF",
  "kkWWWFkFFFk",
  "..kkWWWWkkF",
  "....kkkk..k",
};
static const char* const hb_fishOr[5] = {
  "....kkkk..k",
  "..kkooooko.",
  "kkOOOOkOOok",
  "..kkOOOOko.",
  "....kkkk..k",
};

// ─── The scene ────────────────────────────────────────────────────────
#define HB_HORIZON 132
#define HB_MOONX 200
#define HB_MOONY 36
#define HB_DECK 176        // top of the dock deck
#define HB_DOCKEND 182
#define HB_MANX 126        // fisherman sprite left
#define HB_BARX 86         // fire barrel left
#define HB_LANX 30         // lantern post
#define HB_LHX 268         // lighthouse centre

// fishing state
static int hb_state = 0, hb_stateT = 0, hb_waitT = 300, hb_catch = 0, hb_fishKind = 0;
static int hb_bobX = 238, hb_bobY = 192, hb_bobDip = 0, hb_bend = 0;


static int hb_mtnY(int x) {                       // far mountains (world y of the ridge)
  return 96 + ((hb_sin(x * 2) + hb_sin(x * 5 + 70) + (hb_sin(x * 11 + 20) >> 1)) >> 11);
}
static int hb_hillY(int x) {                      // near hillside the village sits on
  if (x > 250) return HB_HORIZON;
  int y = 117 + ((hb_sin(x * 3 + 30) + (hb_sin(x * 7) >> 1)) >> 12);
  if (x > 220) y += (x - 220) / 2;
  return y;
}

// the sky, sea and land as background rows (per screen row: fast)
static void hb_background() {
  int night = (hb_preset != 1 && hb_preset != 5);
  for (int sy = 0; sy < H; sy++) {
    int wy = hb_worldY(sy);
    uint8_t* row = hb_buf + sy * W;
    if (wy < HB_HORIZON) {
      // sky: dithered gradient, darker at the top
      int lv16 = night ? (32 + wy * 64 / HB_HORIZON) : (64 + wy * 80 / HB_HORIZON);   // shade × 16
      if (hb_lightning > 0) lv16 += 80;
      int base = lv16 >> 4, frac = lv16 & 15;
      for (int sx = 0; sx < W; sx++) {
        int l = base + (frac > hb_bayer[(sy & 3) * 4 + (sx & 3)] ? 1 : 0);
        row[sx] = HB_V(HB_SKY, l > 15 ? 15 : l);
      }
    } else {
      // sea (or ice): darker just below the horizon, lighter close up
      int d = wy - HB_HORIZON;
      int lv16 = hb_frozen ? 104 + d / 3 : 36 + d * 40 / 108;
      int base = lv16 >> 4, frac = lv16 & 15;
      for (int sx = 0; sx < W; sx++) {
        int l = base + (frac > hb_bayer[(sy & 3) * 4 + (sx & 3)] ? 1 : 0);
        row[sx] = HB_V(HB_SEA, l);
      }
    }
  }
}

static void hb_skyThings() {
  int t = hb_t;
  // aurora curtains
  if (hb_preset == 4) {
    for (int sx = 0; sx < W; sx++) {
      int wx = hb_worldX(sx);
      int a = hb_sin(wx * 2 + t + (hb_sin(wx - t / 2) >> 9));
      int top = 18 + (a >> 11), len = 40 + ((hb_sin(wx * 3 + t * 2) + 16384) >> 10);
      for (int k = 0; k < len; k++) {
        int wy = top + k;
        int sy = hb_Y(wy);
        if ((unsigned)sy >= (unsigned)H) continue;
        int l = 13 - k * 10 / len;
        if (k < 3) l = 14;
        if (l > (hb_buf[sy * W + sx] >> 4) && (hb_bayer[(sy & 3) * 4 + (sx & 3)] < 9 || k < 6)) hb_buf[sy * W + sx] = HB_V(HB_AUR, l);
      }
    }
  }
  // stars
  if (hb_preset != 1 && hb_preset != 2 && hb_preset != 5) {
    for (int i = 0; i < 90; i++) {
      uint32_t h = hb_hash(i, 17);
      int x = (int)(h % 320u), y = (int)((h >> 9) % 100u);
      int tw = (hb_sin(t * 3 + (int)(h >> 16)) + 16384) >> 11;   // 0..16
      int l = 7 + (tw >> 1);
      hb_dot(x, y, HB_V(HB_MOON, l > 15 ? 15 : l));
      if (((h >> 28) & 3) == 0 && tw > 12) {                      // a few bright twinklers
        hb_dot(x - 1, y, HB_V(HB_MOON, 8)); hb_dot(x + 1, y, HB_V(HB_MOON, 8));
        hb_dot(x, y - 1, HB_V(HB_MOON, 8)); hb_dot(x, y + 1, HB_V(HB_MOON, 8));
      }
    }
  }
  // moon with a soft halo and craters
  if (hb_preset != 1 && hb_preset != 2 && hb_preset != 5) {
    hb_glow(HB_MOONX, HB_MOONY, 34, HB_SKY, 9);
    hb_disc(HB_MOONX, HB_MOONY, 12, HB_V(HB_MOON, 13), false);
    hb_disc(HB_MOONX - 4, HB_MOONY - 3, 3, HB_V(HB_MOON, 10), false);
    hb_disc(HB_MOONX + 4, HB_MOONY + 4, 2, HB_V(HB_MOON, 10), false);
    hb_disc(HB_MOONX + 5, HB_MOONY - 5, 1, HB_V(HB_MOON, 11), false);
  }
  // setting sun
  if (hb_preset == 5) {
    hb_glow(150, 104, 60, HB_FIRE, 9);
    hb_disc(150, 104, 18, HB_V(HB_FIRE, 13), false);
    hb_disc(150, 104, 13, HB_V(HB_FIRE, 14), false);
  }
  // clouds: soft rounded bands drifting with the wind
  int nc = (hb_preset == 2) ? 9 : (hb_preset == 4 ? 0 : 5);
  for (int i = 0; i < nc; i++) {
    uint32_t h = hb_hash(i, 23);
    int w = 50 + (int)(h % 60u), y = 14 + (int)((h >> 8) % 60u);
    int x = (int)((h >> 16) % 420u) + (t * (40 + (hb_wind >> 4)) >> 8) % 420 - 60;
    x = ((x + 60) % 420) - 60;
    int l = (hb_preset == 2) ? 4 : (hb_preset == 1 || hb_preset == 5 ? 9 : 4);
    int col = (hb_preset == 1 || hb_preset == 5) ? HB_SKY : HB_FAR;
    hb_rect(x + 6, y, x + w - 6, y + 6, HB_V(col, l));
    hb_rect(x, y + 3, x + w, y + 9, HB_V(col, l));
    hb_rect(x + 10, y - 2, x + w / 2, y + 2, HB_V(col, l));
    hb_rect(x + 6, y, x + w - 6, y + 1, HB_V(col, l + 2));     // lit top edge
  }
}

static void hb_land() {
  int t = hb_t;
  // far mountains with snow caps
  for (int sx = 0; sx < W; sx++) {
    int wx = hb_worldX(sx);
    int my = hb_mtnY(wx);
    int syTop = hb_Y(my), syBot = hb_Y(HB_HORIZON);
    if (syTop < 0) syTop = 0;
    for (int sy = syTop; sy < syBot && sy < H; sy++) {
      int wy = hb_worldY(sy);
      int l = (wy - my < 1) ? 5 : 3;                                // a lit ridge line
      int col = HB_FAR;
      if (wy - my < 4 && my < 96 && hb_snowy) { col = HB_SNOW; l = 8; }
      hb_buf[sy * W + sx] = HB_V(col, l);
    }
  }
  // the hillside
  for (int sx = 0; sx < W; sx++) {
    int wx = hb_worldX(sx);
    int hy = hb_hillY(wx);
    if (hy >= HB_HORIZON) continue;
    int syTop = hb_Y(hy), syBot = hb_Y(HB_HORIZON);
    if (syTop < 0) syTop = 0;
    for (int sy = syTop; sy < syBot && sy < H; sy++) {
      int wy = hb_worldY(sy);
      uint8_t v = HB_V(HB_FAR, 2);
      if (wy - hy < 2) v = hb_snowy ? HB_V(HB_SNOW, 7) : HB_V(HB_FAR, 4);
      hb_buf[sy * W + sx] = v;
    }
  }
  // village houses on the hill
  for (int i = 0; i < 9; i++) {
    uint32_t h = hb_hash(i, 41);
    int x = 6 + i * 24 + (int)(h % 8u);
    int w = 14 + (int)((h >> 4) % 8u), hh = 10 + (int)((h >> 8) % 7u);
    int base = hb_hillY(x + w / 2) + 3;
    int top = base - hh;
    // walls: alternate warm colours
    int wc = (h >> 12) % 3u == 0 ? HB_RED : ((h >> 12) % 3u == 1 ? HB_WOOD : HB_FAR);
    hb_orect(x, top, x + w, base, HB_V(wc, wc == HB_FAR ? 6 : 6));
    // roof: a triangle from the wall top up to a ridge, outlined, with snow
    int rh = 5 + w / 4;
    for (int k = 0; k <= rh; k++) {
      int y = top - k, inset = k * (w / 2 + 2) / rh;
      int x0 = x - 2 + inset, x1 = x + w + 2 - inset;
      if (x1 <= x0) break;
      hb_rect(x0, y, x1, y + 1, HB_V(HB_INK, 1));
      if (x1 - x0 > 2) hb_rect(x0 + 1, y, x1 - 1, y + 1, (hb_snowy && k > 0) ? HB_V(HB_SNOW, 11 - (k == rh ? 0 : 1)) : HB_V(HB_DARK, 7));
    }
    hb_rect(x - 2, top, x + w + 2, top + 1, HB_V(HB_INK, 1));
    // chimney
    if ((h >> 14) & 1) hb_orect(x + w - 6, top - rh + 1, x + w - 2, top - 2, HB_V(HB_DARK, 6));
    // windows (lit by the knob, some switching over time), with a frame cross
    int nw = w > 18 ? 2 : 1;
    for (int k = 0; k < nw; k++) {
      int wx0 = x + 3 + k * (w - 10), wy0 = top + 3;
      bool lit = (int)(hb_hash(i * 4 + k, t / 400) & 1023) < hb_lights;
      bool flick = ((hb_hash(i, k + 7) & 3) == 0) && (hb_hash(i * 9 + k, t / 5) & 1);
      hb_rect(wx0, wy0, wx0 + 5, wy0 + 5, HB_V(HB_INK, 1));
      hb_rect(wx0 + 1, wy0 + 1, wx0 + 4, wy0 + 4, lit ? HB_V(HB_WIN, flick ? 11 : 13) : HB_V(HB_DARK, 4));
      if (lit) { hb_rect(wx0 + 2, wy0 + 1, wx0 + 3, wy0 + 4, HB_V(HB_WIN, 9)); hb_rect(wx0 + 1, wy0 + 2, wx0 + 4, wy0 + 3, HB_V(HB_WIN, 9)); }
    }
    // door
    if (hh > 12) hb_rect(x + w / 2 - 1, base - 6, x + w / 2 + 2, base - 1, HB_V(HB_DARK, 3));
    // chimney smoke
    if ((h >> 14) & 1) {
      for (int k = 0; k < 4; k++) {
        int age = (t / 2 + k * 15 + i * 7) % 60;
        int sx = x + w - 4 + ((age * (hb_wind - 300)) >> 11), sy = top - rh - age / 3;
        hb_disc(sx, sy, 1 + age / 20, HB_V(HB_FAR, 6 - age / 15), false);
      }
    }
  }
}

static void hb_lighthouse() {
  int t = hb_t;
  int cx = HB_LHX;
  // beam: a soft wedge swinging round (long when sideways, a flash when facing us)
  if (hb_beam > 30) {
    int ph = (t * (2 + (hb_beam >> 8))) & 255;
    int dir = hb_cos(ph);
    int reach = abs(dir) * 220 >> 14;
    int side = dir > 0 ? 1 : -1;
    for (int k = 4; k < reach; k += 1) {
      int half = 2 + k / 7;
      int x = cx + side * k;
      int l = 9 - k * 6 / (reach + 1);
      for (int y = 58 - half; y <= 58 + half; y++) {
        int sx = hb_X(x), sy = hb_Y(y);
        if ((unsigned)sx >= (unsigned)W || (unsigned)sy >= (unsigned)H) continue;
        uint8_t* p = hb_buf + sy * W + sx;
        if ((hb_bayer[(sy & 3) * 4 + (sx & 3)] & 1) == 0 && (*p >> 4) < l && (*p & 15) != HB_INK) *p = HB_V(HB_WIN, l);
      }
    }
    hb_glow(cx, 58, 16 + (abs(dir) < 4000 ? 14 : 0), HB_WIN, abs(dir) < 4000 ? 12 : 7);
  }
  // rocks
  hb_disc(cx - 14, 134, 11, HB_V(HB_FAR, 6), true);
  hb_disc(cx + 13, 135, 9, HB_V(HB_FAR, 5), true);
  hb_disc(cx, 130, 13, HB_V(HB_FAR, 7), true);
  hb_rect(cx - 12, 120, cx + 6, 122, HB_V(HB_FAR, 10));           // moonlit tops
  hb_rect(cx - 20, 125, cx - 10, 126, HB_V(HB_FAR, 9));
  hb_rect(cx + 8, 128, cx + 16, 129, HB_V(HB_FAR, 8));
  if (hb_snowy) hb_rect(cx - 10, 119, cx + 4, 120, HB_V(HB_SNOW, 12));
  // tapered tower with red bands
  for (int y = 66; y < 124; y++) {
    int half = 6 + (y - 66) * 4 / 58;
    bool band = ((y - 66) / 10) & 1;
    hb_rect(cx - half - 1, y, cx + half + 1, y + 1, HB_V(HB_INK, 1));
    hb_rect(cx - half, y, cx + half, y + 1, band ? HB_V(HB_RED, 9) : HB_V(HB_SNOW, 12));
    hb_rect(cx + half - 3, y, cx + half, y + 1, band ? HB_V(HB_RED, 6) : HB_V(HB_SNOW, 8));   // shade
  }
  // gallery and lamp room
  hb_orect(cx - 10, 62, cx + 10, 67, HB_V(HB_DARK, 7));
  hb_orect(cx - 6, 52, cx + 6, 63, hb_beam > 30 ? HB_V(HB_WIN, 14) : HB_V(HB_WIN, 5));
  hb_rect(cx - 1, 53, cx + 1, 62, HB_V(HB_INK, 1));
  for (int k = 0; k < 6; k++) hb_rect(cx - 7 + k, 51 - k, cx + 7 - k, 52 - k, HB_V(HB_INK, 1));
  hb_rect(cx - 5, 50, cx + 5, 51, HB_V(HB_RED, 8));
  // door
  hb_orect(cx - 3, 114, cx + 3, 124, HB_V(HB_DARK, 4));
}

static void hb_sea() {
  int t = hb_t;
  if (hb_frozen) {
    // ice: cracks, drifts, a fishing hole
    for (int i = 0; i < 40; i++) {
      uint32_t h = hb_hash(i, 61);
      int x = (int)(h % 320u), y = HB_HORIZON + 4 + (int)((h >> 9) % 104u);
      int len = 6 + (int)((h >> 20) % 18u);
      hb_line(x, y, x + len, y + (int)((h >> 25) & 3) - 1, HB_V(HB_SEA, 6));
      if (((h >> 28) & 3) == 0) hb_rect(x, y - 2, x + len * 2, y, HB_V(HB_SNOW, 13));
    }
    hb_disc(hb_bobX, hb_bobY + 2, 7, HB_V(HB_INK, 1), false);
    hb_disc(hb_bobX, hb_bobY + 2, 6, HB_V(HB_SEA, 3), false);
    hb_rect(hb_bobX - 5, hb_bobY + 1, hb_bobX + 5, hb_bobY + 2, HB_V(HB_FOAM, 7));
    return;
  }
  // wave dashes: short light strokes that drift and twinkle
  int n = 70 + (hb_waves * 160 >> 10);
  for (int i = 0; i < n; i++) {
    uint32_t h = hb_hash(i, 71);
    int y = HB_HORIZON + 2 + (int)(((h >> 9) % 1000u) * ((h >> 9) % 1000u) / 9300u);   // denser near the horizon
    int depth = y - HB_HORIZON;
    int len = 3 + depth / 9 + (int)((h >> 20) & 3);
    int x = ((int)(h % 360u) + (t * (2 + depth / 25)) / 3) % 360 - 20;
    int ph = (t * 5 + (int)(h >> 24)) & 255;
    if (hb_sin(ph) < -4000) continue;                             // each one comes and goes
    int l = 6 + depth / 20 + ((hb_waves > 700) ? 2 : 0);
    hb_rect(x, y, x + len, y + 1, HB_V(HB_FOAM, l > 11 ? 11 : l));
  }
  // moon / sun path: broken stripes widening toward us
  if (hb_preset != 1 && hb_preset != 2) {
    int px = (hb_preset == 5) ? 150 : HB_MOONX;   // (sun / moon x)
    int col = (hb_preset == 5) ? HB_FIRE : HB_MOON;
    for (int y = HB_HORIZON + 2; y < 240; y += 3) {
      int d = y - HB_HORIZON;
      uint32_t h = hb_hash(y, t / 3);
      int w = 4 + d / 4 + (int)(h & 7);
      int off = (int)((h >> 4) % 7u) - 3;
      hb_rect(px - w / 2 + off, y, px + w / 2 + off, y + 1, HB_V(col, 12 - d / 14));
    }
  }
  // lighthouse lamp and window reflections near the far shore
  for (int y = HB_HORIZON + 1; y < HB_HORIZON + 20; y += 2) {
    if (hb_beam > 30 && (hb_hash(y, t / 5) & 1)) hb_rect(HB_LHX - 2, y, HB_LHX + 3, y + 1, HB_V(HB_WIN, 9 - (y - HB_HORIZON) / 3));
    for (int i = 0; i < 9; i++) {
      uint32_t h = hb_hash(i, 41);
      int x = 6 + i * 24 + (int)(h % 8u) + 4;
      if ((int)(hb_hash(i * 4, t / 400) & 1023) < hb_lights && (hb_hash(y + i, t / 7) & 3) == 0)
        hb_rect(x, y, x + 3, y + 1, HB_V(HB_WIN, 8 - (y - HB_HORIZON) / 4));
    }
  }
  // aurora reflection
  if (hb_preset == 4) {
    for (int i = 0; i < 40; i++) {
      uint32_t h = hb_hash(i, t / 8);
      int x = (int)(h % 320u), y = HB_HORIZON + 2 + (int)((h >> 9) % 30u);
      hb_rect(x, y, x + 6 + (int)((h >> 20) % 10u), y + 1, HB_V(HB_AUR, 8 - (y - HB_HORIZON) / 5));
    }
  }
  // fire reflection under the dock
  if (hb_fire > 30) {
    for (int y = HB_DECK + 12; y < HB_DECK + 40; y += 3) {
      uint32_t h = hb_hash(y, t / 4);
      int w = 6 + (int)(h % 8u);
      hb_rect(HB_BARX + 8 - w / 2 + (int)((h >> 4) % 5u) - 2, y, HB_BARX + 8 + w / 2, y + 1, HB_V(HB_FIRE, 9 - (y - HB_DECK - 12) / 5));
    }
  }
}

static void hb_boat() {
  if (hb_frozen) return;
  int t = hb_t;
  int P = 2600;
  int x = -50 + (t % P) * 420 / P;
  int y = 150 + (hb_sin(t * 4) >> 13);
  int tilt = hb_sin(t * 3) >> 13;
  // hull, cabin, mast, lantern
  for (int k = 0; k < 6; k++) {
    int inset = k * 2;
    hb_rect(x - 18 + inset, y + k, x + 18 - inset, y + k + 1, k == 0 ? HB_V(HB_RED, 9) : HB_V(HB_RED, 5));
    hb_dot(x - 19 + inset, y + k, HB_V(HB_INK, 1)); hb_dot(x + 18 - inset, y + k, HB_V(HB_INK, 1));
  }
  hb_rect(x - 18, y - 1, x + 18, y, HB_V(HB_INK, 1));
  hb_orect(x - 8, y - 9 + tilt, x + 4, y, HB_V(HB_WOOD, 8));
  hb_rect(x - 5, y - 7 + tilt, x - 1, y - 4 + tilt, HB_V(HB_WIN, 13));          // lit cabin window
  hb_line(x + 8, y, x + 8 + tilt, y - 22, HB_V(HB_INK, 1));
  int fl = (int)(hb_hash(t / 6, 13) & 1);
  hb_glow(x + 8 + tilt, y - 22, 9, HB_WIN, 8 + fl);
  hb_disc(x + 8 + tilt, y - 22, 2, HB_V(HB_WIN, 14 + fl), true);
  // its reflection in the water
  for (int k = 0; k < 6; k++) {
    uint32_t h = hb_hash(k, t / 4);
    int w = 3 + (int)(h % 5u);
    hb_rect(x + 7 - w / 2 + (int)((h >> 4) % 3u) - 1, y + 8 + k * 3, x + 9 + w / 2, y + 9 + k * 3, HB_V(HB_WIN, 9 - k));
  }
}

static void hb_shootingStar() {
  if (hb_preset == 1 || hb_preset == 2 || hb_preset == 5) return;
  int P = 520, c = hb_t % P, n = hb_t / P;
  if (c >= 24) return;
  uint32_t h = hb_hash(n, 91);
  int x0 = 20 + (int)(h % 200u), y0 = 8 + (int)((h >> 9) % 40u);
  int hx = x0 + c * 5, hy = y0 + c * 2;
  for (int k = 0; k < 10; k++) hb_dot(hx - k * 3, hy - k, HB_V(HB_MOON, 15 - k));
}

static void hb_dock() {
  // posts into the water, with foam where they meet it
  for (int x = 4; x < HB_DOCKEND; x += 36) {
    hb_orect(x, HB_DECK + 6, x + 7, 244, HB_V(HB_WOOD, 6));
    hb_rect(x + 1, HB_DECK + 7, x + 3, 244, HB_V(HB_WOOD, 8));
    if (!hb_frozen) hb_rect(x - 2, 214 + ((hb_sin(hb_t * 4 + x) >> 13)), x + 9, 215 + ((hb_sin(hb_t * 4 + x) >> 13)), HB_V(HB_FOAM, 10));
  }
  // deck: planks with seams, lit top edge, outline, snow
  hb_rect(-40, HB_DECK - 1, HB_DOCKEND + 1, HB_DECK + 9, HB_V(HB_INK, 1));
  hb_rect(-40, HB_DECK, HB_DOCKEND, HB_DECK + 8, HB_V(HB_WOOD, 8));
  hb_rect(-40, HB_DECK, HB_DOCKEND, HB_DECK + 1, HB_V(HB_WOOD, 11));
  hb_rect(-40, HB_DECK + 6, HB_DOCKEND, HB_DECK + 8, HB_V(HB_WOOD, 5));
  for (int x = -36; x < HB_DOCKEND; x += 12) hb_rect(x, HB_DECK + 1, x + 1, HB_DECK + 8, HB_V(HB_WOOD, 4));
  if (hb_snowy) for (int x = -38; x < HB_DOCKEND; x += 7) hb_rect(x, HB_DECK - 2, x + 4 + (int)(hb_hash(x, 5) % 4u), HB_DECK, HB_V(HB_SNOW, 13));
  // lantern post with a hanging lantern and its glow
  hb_orect(HB_LANX, 138, HB_LANX + 5, HB_DECK, HB_V(HB_WOOD, 7));
  hb_orect(HB_LANX - 1, 136, HB_LANX + 14, 140, HB_V(HB_WOOD, 7));
  hb_line(HB_LANX + 11, 140, HB_LANX + 11, 144, HB_V(HB_INK, 1));
  int fl = (int)(hb_hash(hb_t / 6, 9) & 1);
  hb_glow(HB_LANX + 11, 150, 30, HB_WIN, 6 + fl);
  hb_orect(HB_LANX + 7, 144, HB_LANX + 16, 156, HB_V(HB_WIN, 14 + fl));
  hb_rect(HB_LANX + 7, 144, HB_LANX + 16, 146, HB_V(HB_DARK, 5));
  hb_rect(HB_LANX + 11, 146, HB_LANX + 12, 155, HB_V(HB_INK, 1));
}

static void hb_fireBarrel() {
  if (hb_fire <= 30) return;
  int t = hb_t;
  int x = HB_BARX, top = 156;
  int fh = 10 + (hb_fire * 22 >> 10);                               // flame height
  hb_glow(x + 8, top - 8, 20 + (hb_fire >> 6), HB_FIRE, 6 + (int)(hb_hash(t / 5, 3) & 1));
  // the drum
  hb_orect(x, top, x + 17, HB_DECK, HB_V(HB_RED, 6));
  hb_rect(x + 1, top + 1, x + 4, HB_DECK - 1, HB_V(HB_RED, 8));       // lit side
  hb_rect(x + 12, top + 1, x + 16, HB_DECK - 1, HB_V(HB_RED, 4));     // shaded side
  hb_rect(x, top + 6, x + 17, top + 7, HB_V(HB_INK, 1));             // rings
  hb_rect(x, top + 13, x + 17, top + 14, HB_V(HB_INK, 1));
  hb_rect(x - 1, top - 1, x + 18, top + 2, HB_V(HB_INK, 1));          // rim
  hb_rect(x, top, x + 17, top + 1, HB_V(HB_RED, 10));
  // flames: three tongues of nested shapes, flickering
  for (int k = 0; k < 3; k++) {
    uint32_t h = hb_hash(k, t / 3);
    int cx = x + 4 + k * 4 + (int)(h % 3u) - 1;
    int hk = fh - (k == 1 ? 0 : 5) + (int)((h >> 4) % 5u);
    for (int y = 0; y < hk; y++) {
      int half = (hk - y) * 4 / hk + 1;
      int sway = (hb_sin(t * 6 + y * 12 + k * 80) * y) >> 17;
      int yy = top - 1 - y;
      hb_rect(cx - half + sway, yy, cx + half + sway + 1, yy + 1, HB_V(HB_FIRE, 9));
      if (half > 1) hb_rect(cx - half + 1 + sway, yy, cx + half + sway, yy + 1, HB_V(HB_FIRE, 12));
      if (half > 2 && y < hk * 2 / 3) hb_rect(cx - 1 + sway, yy, cx + 2 + sway, yy + 1, HB_V(HB_FIRE, 15));
    }
  }
  // sparks
  int n = 3 + (hb_fire >> 7);
  for (int i = 0; i < n; i++) {
    uint32_t h = hb_hash(i, 51);
    int life = 40 + (int)(h % 40u);
    int age = (t + (int)(h >> 8) % life) % life;
    int sx = x + 6 + (int)((h >> 3) % 6u) + ((age * ((int)((h >> 12) & 7) - 3 + ((hb_wind - 512) >> 7))) >> 3);
    int sy = top - fh - age * 50 / life;
    hb_dot(sx, sy, HB_V(HB_FIRE, 14 - age * 6 / life));
  }
}

static void hb_fisherman() {
  int mx = hb_frozen ? 150 : HB_MANX, my = (hb_frozen ? 196 : HB_DECK) - 56;
  int bob = ((hb_t / 40) & 1);                                      // breathing
  hb_sprite(hb_man, 28, 20, mx, my + bob, 2, false);
  if (hb_state == 2) hb_sprite(hb_manReel, 3, 20, mx, my + bob + 24, 2, false);
  // the rod: from the hands up and out, bending when a fish is on
  int hx = mx + 34, hy = my + 26 + bob - (hb_state == 2 ? 2 : 0);
  int tipX = hx + 50 - hb_bend / 12, tipY = hy - 48 + (hb_bend * 40 >> 8);
  tipX += hb_sin(hb_t * 2) >> 13; tipY += hb_sin(hb_t * 3 + 50) >> 13;          // the rod tip sways a little
  // casting: rod swings back over the shoulder, snaps forward, the bobber flies out
  bool flying = false;
  int flyX = 0, flyY = 0;
  if (hb_state == 5) {
    int c = hb_stateT;
    if (c < 30) { int p = c * 256 / 30; tipX = hx + 50 - (70 * p >> 8); tipY = hy - 48 - (6 * p >> 8); }
    else if (c < 40) { int p = (c - 30) * 256 / 10; tipX = hx - 20 + (80 * p >> 8); tipY = hy - 54 + (10 * p >> 8); }
    else { tipX = hx + 60; tipY = hy - 44; }
    if (c >= 36 && c < 80) {                                      // the bobber's flight
      int p = (c - 36) * 256 / 44;
      flying = true;
      flyX = tipX + (hb_bobX - tipX) * p / 256;
      flyY = tipY + (hb_bobY - tipY) * p / 256 - (hb_sin(p >> 1) * 40 >> 14);
    }
  }
  int midX = (hx + tipX) / 2 + 2, midY = (hy + tipY) / 2 + (hb_bend * 10 >> 8);
  hb_line(hx, hy, midX, midY, HB_V(HB_INK, 1));
  hb_line(hx, hy - 1, midX, midY - 1, HB_V(HB_WOOD, 9));
  hb_line(midX, midY, tipX, tipY, HB_V(HB_WOOD, 8));
  hb_rect(hx + 2, hy - 3, hx + 5, hy, HB_V(HB_DARK, 8));            // reel
  if (hb_state == 5 && !flying && hb_stateT < 36) {
    hb_disc(tipX, tipY + 4, 2, HB_V(HB_RED, 11), true);           // bobber dangling from the tip
    hb_line(tipX, tipY, tipX, tipY + 2, HB_V(HB_FOAM, 9));
  } else if (flying) {
    hb_line(tipX, tipY, flyX, flyY, HB_V(HB_FOAM, 9));
    hb_disc(flyX, flyY, 3, HB_V(HB_RED, 11), true);
  } else {
    // the line sagging to the bobber, swaying with the wind
    int sagX = (tipX + hb_bobX) / 2 + 3 + (hb_sin(hb_t * 2 + 90) >> 12), sagY = (tipY + hb_bobY) / 2 + 6;
    hb_line(tipX, tipY, sagX, sagY, HB_V(HB_FOAM, 9));
    hb_line(sagX, sagY, hb_bobX, hb_bobY - 3, HB_V(HB_FOAM, 9));
  }
  // splash where the cast lands
  if (hb_state == 5 && hb_stateT >= 78 && !hb_frozen) {
    int a = hb_stateT - 78;
    for (int k = 0; k < 10; k++) {
      uint32_t h = hb_hash(k, 35);
      int dx = (int)(h % 15u) - 7;
      hb_dot(hb_bobX + dx * (1 + a / 6), hb_bobY - 2 - (int)((h >> 5) % 6u) + a * a / 20, HB_V(HB_FOAM, 13));
    }
  }
  // bobber (red over white), ripples when it moves
  if (hb_bobDip < 200 && !(hb_state == 5 && hb_stateT < 80)) {
    hb_disc(hb_bobX, hb_bobY - 2, 3, HB_V(HB_RED, 11), true);
    hb_rect(hb_bobX - 2, hb_bobY, hb_bobX + 3, hb_bobY + 2, HB_V(HB_SNOW, 13));
  }
  if ((hb_state == 1 || hb_state == 2) && !hb_frozen) {
    for (int k = 0; k < 2; k++) {
      int r = ((hb_t / 2 + k * 10) % 20) + 3;
      for (int a = 0; a < 256; a += 6) {
        int px = hb_bobX + (r * hb_cos(a) >> 14), py = hb_bobY + (r * hb_sin(a) >> 16);
        hb_dot(px, py, HB_V(HB_FOAM, 11 - r / 3));
      }
    }
  }
  // the fish: leaps from the bobber into the fisherman's hands, then held up
  if (hb_state == 3 || hb_state == 4) {
    int fx, fy;
    bool flip = false;
    if (hb_state == 3) {
      int p = hb_stateT * 256 / 60; if (p > 256) p = 256;
      fx = hb_bobX + (hx + 6 - hb_bobX) * p / 256;
      fy = hb_bobY - (hb_sin(p >> 1) * 60 >> 14) - (hb_bobY - hy) * p / 256;
      flip = true;
      if (hb_stateT < 24 && !hb_frozen) {                           // splash
        for (int k = 0; k < 12; k++) {
          uint32_t h = hb_hash(k, 33);
          int dx = (int)(h % 21u) - 10, up = (int)((h >> 5) % 10u) + hb_stateT;
          hb_dot(hb_bobX + dx * (1 + hb_stateT / 8), hb_bobY - up + hb_stateT * hb_stateT / 30, HB_V(HB_FOAM, 13));
        }
      }
    } else {
      fx = hx + 4; fy = hy - 4 + ((hb_stateT / 15) & 1);
    }
    if (hb_fishKind == 6) {                                         // an old boot!
      hb_orect(fx - 6, fy - 6, fx, fy + 4, HB_V(HB_DARK, 6));
      hb_orect(fx - 6, fy, fx + 6, fy + 5, HB_V(HB_DARK, 6));
    } else {
      hb_sprite((hb_fishKind == 0 || hb_fishKind == 3) ? hb_fishOr : hb_fishSpr, 5, 11, fx - 11, fy - 5, 2, flip);
    }
  }
}

static void hb_weatherFx() {
  int t = hb_t;
  bool rain = (hb_preset == 2), snow = hb_snowy && hb_preset != 4;
  int amt = (hb_weather * (hb_preset == 3 ? 3 : 2)) >> 2;            // up to ~760 flakes
  if (!rain && !snow) amt = 0;
  int windPx = hb_wind - 512;
  for (int i = 0; i < amt; i++) {
    uint32_t h = hb_hash(i, 81);
    int depth = (int)(h & 3);                                       // 0 far … 3 near
    int spd = rain ? 7 + depth * 2 : 1 + (depth >> 1);
    int fy = ((int)((h >> 3) % 260u) + t * spd / 2) % 260 - 10;
    int fx = (int)((h >> 12) % 360u) - 20 + ((fy * windPx) >> 9) + (rain ? 0 : (hb_sin(t * 2 + (int)(h >> 4)) >> 12));
    fx = ((fx % 360) + 360) % 360 - 20;
    // screen-space (weather stays in front of the camera)
    if (rain) {
      int dx = windPx >> 7;
      for (int k = 0; k < 4 + depth; k++) hb_spx(fx + dx * k / 4, fy + k, HB_V(HB_FOAM, 7 + depth));
    } else {
      hb_spx(fx, fy, HB_V(HB_SNOW, 10 + depth));
      if (depth >= 2) { hb_spx(fx + 1, fy, HB_V(HB_SNOW, 10 + depth)); hb_spx(fx, fy + 1, HB_V(HB_SNOW, 9 + depth)); hb_spx(fx + 1, fy + 1, HB_V(HB_SNOW, 9 + depth)); }
    }
  }
  if (rain && hb_lightning > 2) {                                   // the bolt
    int x = 40 + (int)(hb_hash(t / 20, 4) % 200u), y = 0;
    while (y < 110) {
      int nx = x + (int)(hb_hash(x, y + t / 20) % 9u) - 4;
      hb_line(x, y, nx, y + 8, HB_V(HB_SNOW, 15));
      x = nx; y += 8;
    }
  }
  if (hb_preset == 1) {                                             // gulls
    for (int i = 0; i < 4; i++) {
      int x = ((t + i * 90) % 400) - 40, y = 30 + i * 14 + (hb_sin(t * 4 + i * 60) >> 12);
      int wing = ((t / 8 + i) & 1) ? -2 : 1;
      hb_line(x - 4, y + wing, x, y, HB_V(HB_INK, 1));
      hb_line(x, y, x + 4, y + wing, HB_V(HB_INK, 1));
    }
  }
  if (hb_preset == 5) {                                             // fireflies
    for (int i = 0; i < 24; i++) {
      uint32_t h = hb_hash(i, 71);
      int x = (int)(h % 180u) + ((hb_sin(t + (int)h) * 8) >> 14), y = 130 + (int)((h >> 8) % 45u) + ((hb_cos(t + (int)(h >> 4)) * 5) >> 14);
      if (hb_sin(t * 3 + (int)(h >> 12)) > 6000) { hb_dot(x, y, HB_V(HB_AUR, 14)); hb_glow(x, y, 4, HB_AUR, 6); }
    }
  }
}

// mist: lift the dark pixels near the horizon toward a soft grey-blue
static void hb_fogFx() {
  if (hb_fog < 30) return;
  int band = 20 + (hb_fog >> 4);
  for (int sy = 0; sy < H; sy++) {
    int wy = hb_worldY(sy);
    int d = abs(wy - HB_HORIZON);
    if (d >= band) continue;
    int amt = (band - d) * hb_fog / band;                           // 0..1023
    int lv = 6;
    uint8_t* row = hb_buf + sy * W;
    for (int sx = 0; sx < W; sx++) {
      if ((hb_bayer[(sy & 3) * 4 + (sx & 3)] << 6) >= amt) continue;
      if ((row[sx] >> 4) < lv) row[sx] = HB_V(HB_FAR, lv);
    }
  }
}

// ASCII style: every character cell becomes a glyph picked by the cell's
// brightness, and each lit glyph pixel keeps the colour of the picture
// underneath (so shapes and outlines stay sharp); the pixels between the
// strokes are dimmed. Cells: 3x5 (fine, no gaps), 4x6, or 8x12 (big).
static void hb_asciify(int cw, int ch, int bgQ) {
  for (int cy = 0; cy + ch <= H; cy += ch) {
    for (int cx = 0; cx + cw <= W; cx += cw) {
      int sum = 0;
      for (int y = 0; y < ch; y++) {
        const uint8_t* p = hb_buf + (cy + y) * W + cx;
        for (int x = 0; x < cw; x++) sum += (p[x] & 15) == HB_INK ? 0 : (p[x] >> 4);
      }
      int avg = sum / (cw * ch);
      int gi = (avg * 10 + 8) / 16;
      if (gi > 9) gi = 9;
      uint16_t bits = hb_fontBits[hb_ramp[gi]];
      for (int y = 0; y < ch; y++) {
        uint8_t* p = hb_buf + (cy + y) * W + cx;
        int gy = (cw == 8) ? ((y >= 1 && y <= 10) ? (y - 1) >> 1 : -1) : (y < 5 ? y : -1);
        int row = gy >= 0 ? (bits >> (12 - gy * 3)) & 7 : 0;
        for (int x = 0; x < cw; x++) {
          int gx = (cw == 8) ? ((x >= 1 && x <= 6) ? (x - 1) >> 1 : -1) : (x < 3 ? x : -1);
          bool on = gx >= 0 && (row & (4 >> gx));
          uint8_t v = p[x];
          int c = v & 15, l = v >> 4;
          if (c == HB_INK) { if (!on) p[x] = 0; continue; }
          if (on) { l += 2; if (l > 15) l = 15; if (l < 3) l = 3; }
          else l = l * bgQ >> 8;
          p[x] = l > 0 ? HB_V(c, l) : 0;
        }
      }
    }
  }
}

// ─── Text for the status panel (pixel font, always small) ─────────────
static void hb_text(int x, int y, const char* s, int col, int l) {
  uint8_t fg = (uint8_t)((l << 4) | col);
  for (; *s; s++, x += 4) {
    char c = *s;
    if (c >= 'a' && c <= 'z') c -= 32;
    uint16_t bits = hb_fontBits[hb_gi(c)];
    for (int r = 0; r < 5; r++) {
      int row = (bits >> (12 - r * 3)) & 7;
      for (int k = 0; k < 3; k++)
        if (row & (4 >> k)) { int px = x + k, py = y + r; if ((unsigned)px < (unsigned)W && (unsigned)py < (unsigned)H) hb_buf[py * W + px] = fg; }
    }
  }
}
static void hb_box(int x, int y, int w, int h, int col, int l, bool fill) {
  uint8_t v = (uint8_t)((l << 4) | col);
  for (int j = 0; j < h; j++) {
    int py = y + j;
    if ((unsigned)py >= (unsigned)H) continue;
    for (int i = 0; i < w; i++) {
      int px = x + i;
      if ((unsigned)px >= (unsigned)W) continue;
      bool edge = (j == 0 || j == h - 1 || i == 0 || i == w - 1);
      if (fill || edge) hb_buf[py * W + px] = edge ? v : (uint8_t)((1 << 4) | HB_VOID);
    }
  }
}
static void hb_num(char* out, int v, int digits) {
  for (int i = digits - 1; i >= 0; i--) { out[i] = (char)('0' + v % 10); v /= 10; }
  out[digits] = 0;
}

static const char* const hb_places[HB_NPRESET] = {
  "FROSTWICK HARBOR", "GULLSHORE", "STORMWATCH POINT", "NORTHMERE ICE",
  "LIGHTFALL BAY", "LANTERN COVE", "FROSTWICK HARBOR", "STORMWATCH POINT"
};
static const char* const hb_fishNames[8] = {
  "A COD", "A PERCH", "A HERRING", "A SALMON", "A MACKEREL", "A GLOWFISH", "AN OLD BOOT", "A MOONFISH"
};

static void hb_panel(int mode, int hour, int minute) {
  if (mode <= 0) return;
  int x = mode >= 2 ? 22 : 4, y = 4;
  hb_box(x, y, 118, 44, HB_HUD, 9, true);
  char line[40];
  // place + clock
  int hh = hour % 12; if (hh == 0) hh = 12;
  char tb[3]; hb_num(tb, hh, 2); char mb[3]; hb_num(mb, minute, 2);
  hb_text(x + 4, y + 4, hb_places[hb_preset], HB_HUD, 13);
  line[0] = tb[0] == '0' ? ' ' : tb[0]; line[1] = tb[1]; line[2] = ':'; line[3] = mb[0]; line[4] = mb[1];
  line[5] = ' '; line[6] = hour >= 12 ? 'P' : 'A'; line[7] = 'M'; line[8] = 0;
  hb_text(x + 4 + 4 * 17, y + 4, line, HB_HUD, 10);
  // what's happening
  const char* st = "WAITING FOR A BITE";
  if (hb_state == 1) st = "SOMETHING NIBBLES...";
  else if (hb_state == 2) st = "FISH ON! REEL IT IN";
  else if (hb_state >= 3) st = "CAUGHT ";
  hb_text(x + 4, y + 12, st, HB_WIN, 12);
  if (hb_state >= 3) hb_text(x + 4 + 28, y + 12, hb_fishNames[hb_fishKind], HB_WIN, 13);
  // bars: warmth (fire), luck (bite rate)
  hb_text(x + 4, y + 20, "WARMTH", HB_FIRE, 11);
  hb_text(x + 4, y + 28, "LUCK", HB_AUR, 11);
  int wb = hb_fire * 10 >> 10, lb = 2 + (hb_t / 30 + hb_catch) % 7;
  for (int i = 0; i < 10; i++) {
    hb_box(x + 32 + i * 8, y + 21, 6, 3, HB_FIRE, i < wb ? 12 : 3, true);
    hb_box(x + 32 + i * 8, y + 29, 6, 3, HB_AUR, i < lb ? 11 : 3, true);
  }
  // catch counter
  char cb[4]; hb_num(cb, hb_catch % 1000, 3);
  hb_text(x + 4, y + 36, "CATCH", HB_HUD, 11);
  hb_text(x + 28, y + 36, cb, HB_HUD, 14);
  if (mode >= 2) {                                              // icons: a clock and a compass
    int cx = 11, cy = 13;
    for (int a = 0; a < 256; a += 8) {
      int px = cx + (7 * hb_cos(a) >> 14), py = cy + (7 * hb_sin(a) >> 14);
      if ((unsigned)px < (unsigned)W && (unsigned)py < (unsigned)H) hb_buf[py * W + px] = (uint8_t)((11 << 4) | HB_HUD);
    }
    int ha = ((hour % 12) * 60 + minute) * 256 / 720 - 64, ma = minute * 256 / 60 - 64;
    for (int r = 0; r <= 5; r++) {
      int px = cx + (r * hb_cos(ma) >> 14), py = cy + (r * hb_sin(ma) >> 14);
      hb_buf[py * W + px] = (uint8_t)((13 << 4) | HB_HUD);
      if (r <= 3) { px = cx + (r * hb_cos(ha) >> 14); py = cy + (r * hb_sin(ha) >> 14); hb_buf[py * W + px] = (uint8_t)((13 << 4) | HB_RED); }
    }
    cy = 33;                                                    // compass: needle swings with the wind
    for (int a = 0; a < 256; a += 8) {
      int px = cx + (7 * hb_cos(a) >> 14), py = cy + (7 * hb_sin(a) >> 14);
      hb_buf[py * W + px] = (uint8_t)((11 << 4) | HB_HUD);
    }
    int na = -64 + ((hb_wind - 512) >> 4) + (hb_sin(hb_t * 2) >> 12);
    for (int r = -5; r <= 5; r++) {
      int px = cx + (r * hb_cos(na) >> 14), py = cy + (r * hb_sin(na) >> 14);
      hb_buf[py * W + px] = (uint8_t)(((r > 0 ? 13 : 9) << 4) | (r > 0 ? HB_RED : HB_HUD));
    }
  }
}


// ─── Knobs ────────────────────────────────────────────────────────────
static float hb_sm[16];
static bool hb_smReady = false;
static inline int hb_k(int i) { int v = (int)hb_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_harbor_name() { return "HARBOR NIGHT"; }
const char* prog_harbor_character() { return "A cozy pixel-art harbor: a fisherman on the dock by a fire barrel, a village and a lighthouse across the bay"; }

static const char* const hb_presetNames[] = {
  "Snow Harbor", "Dawn Mist", "Storm", "Ice Fishing", "Aurora", "Summer Dusk", "Close Up", "Lighthouse"
};

const char* prog_harbor_presetName(int preset) {
  if (preset >= 0 && preset < HB_NPRESET) return hb_presetNames[preset];
  return NULL;
}

static const char* const hb_labels[16] = {
  "Palette", "Speed", "Style", "Char Size", "Time", "Weather", "Wind", "Waves",
  "Village Lights", "Bite Rate", "Lighthouse", "Status Panel", "Fire", "Pan", "Zoom", "Fog"
};
const char* prog_harbor_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return hb_labels[pot];
  return "";
}

uint8_t prog_harbor_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we draw every pixel ourselves
}

void prog_harbor_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    const char* r = " .:-=+*#%@";
    for (int i = 0; i < 10; i++) hb_ramp[i] = (uint8_t)hb_gi(r[i]);
  }
  hb_state = 0; hb_stateT = 0; hb_waitT = 240;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_harbor_draw(int preset) {
  hb_buf = display.getBuffer();
  if (!hb_buf) return;
  if (preset < 0 || preset >= HB_NPRESET) preset = 0;
  hb_preset = preset;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!hb_smReady) hb_sm[i] = pots[i];
      else hb_sm[i] += (pots[i] - hb_sm[i]) * a;
    }
    hb_smReady = true;
  }

  // ── clock ──
  float sk = hb_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.9×
  static float tf = 0, steps = 0, gameMin = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e7f) tf = 0;
  hb_t = (int)tf;
  steps += dt * 60.0f * speed;
  int nStep = (int)steps; steps -= nStep;
  if (nStep > 6) nStep = 6;
  gameMin += dt * speed;                                       // one game minute per second
  if (gameMin > 1440.0f) gameMin -= 1440.0f;
  static const int baseHour[HB_NPRESET] = {23, 6, 21, 15, 1, 20, 23, 22};

  // ── knobs ──
  int style = hb_k(2);
  int csz = hb_k(3) < 340 ? 0 : (hb_k(3) < 720 ? 1 : 2);      // ASCII cell: 3x5 / 4x6 / 8x12
  int timeShift = hb_k(4) - 512;
  hb_weather = hb_k(5);
  hb_wind = hb_k(6);
  hb_waves = hb_k(7);
  hb_lights = hb_k(8);
  int biteK = hb_k(9);
  hb_beam = hb_k(10);
  int panelMode = hb_k(11) < 300 ? 0 : (hb_k(11) < 720 ? 1 : 2);
  hb_fire = hb_k(12);
  hb_fog = hb_k(15);
  hb_frozen = (preset == 3);
  hb_snowy = (preset == 0 || preset == 3 || preset == 6 || preset == 7);
  if (preset == 2) { if (hb_waves < 700) hb_waves = 700; if (hb_beam < 600) hb_beam = 600; }
  if (preset == 1 && hb_fog < 450) hb_fog = 450;
  if (preset == 5 || preset == 1) hb_fire = hb_fire * 2 / 3;

  // zoom + pan (some presets frame their own shot)
  float zk = hb_k(14) / 1023.0f;
  float zoom = zk < 0.5f ? 1.0f : 1.0f + (zk - 0.5f) * 4.0f;   // left half = whole scene, right half to 3×
  float cxw = 160.0f, cyw = 120.0f;
  if (preset == 6) { zoom *= 2.2f; cxw = 172.0f; cyw = 158.0f; }
  if (preset == 7) { zoom *= 1.9f; cxw = 262.0f; cyw = 86.0f; }
  float pk = (hb_k(13) - 512) / 512.0f;
  if (pk > -0.04f && pk < 0.04f) pk = 0;
  cxw += pk * 160.0f;
  float halfW = 160.0f / zoom, halfH = 120.0f / zoom;            // keep the view on the scene
  if (cxw < halfW) cxw = halfW;
  if (cxw > 320 - halfW) cxw = 320 - halfW;
  if (cyw < halfH) cyw = halfH;
  if (cyw > 240 - halfH) cyw = 240 - halfH;
  hb_zq = (int)(zoom * 256.0f);
  hb_vx = (int)cxw;
  hb_vy = (int)cyw;

  // ── the fishing ──
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was && hb_state == 0) { hb_state = 2; hb_stateT = 0; }
  k12Was = k12;
  for (int s = 0; s < nStep; s++) {
    hb_stateT++;
    switch (hb_state) {
      case 0: if (--hb_waitT <= 0) { hb_state = 1; hb_stateT = 0; } break;
      case 1: if (hb_stateT > 100) { hb_state = 2; hb_stateT = 0; } break;
      case 2: if (hb_stateT > 130) { hb_state = 3; hb_stateT = 0; hb_fishKind = (int)(hb_hash(hb_t, hb_catch) & 7); } break;
      case 3: if (hb_stateT > 60) { hb_state = 4; hb_stateT = 0; hb_catch++; } break;
      case 4: if (hb_stateT > 150) { hb_state = 5; hb_stateT = 0; } break;
      default:                                                    // 5: casting the line back out
        if (hb_stateT > 96) {
          hb_state = 0; hb_stateT = 0;
          int base = 900 - biteK * 780 / 1023;                    // bite rate: ~15 s … ~2 s
          if (preset == 6) base /= 2;
          hb_waitT = base / 2 + (int)(hb_hash(hb_t, 77) % (uint32_t)(base + 1));
        }
        break;
    }
  }
  hb_bobDip = 0; hb_bend = 0;
  if (hb_state == 1) hb_bobDip = ((hb_hash(hb_t / 6, 5) & 3) == 0) ? 120 : 0;
  if (hb_state == 2) { hb_bobDip = 256; hb_bend = 150 + (hb_sin(hb_t * 9) >> 8); }
  if (hb_state == 3) hb_bend = 60;
  hb_bobX = (hb_frozen ? 214 : 238) + (hb_frozen ? 0 : (hb_sin(hb_t * 2 + 40) >> 13));   // drifts a little
  hb_bobY = (hb_frozen ? 200 : 194) + (hb_frozen ? 0 : (hb_sin(hb_t * 3) >> 12)) + (hb_bobDip >> 6);

  if (preset == 2 && hb_lightning <= 0 && (hb_hash(hb_t / 20, 9) % 40u) == 0 && (hb_t % 20) == 0 && speed > 0) hb_lightning = 6;
  if (hb_lightning > 0) hb_lightning--;

  // ── colours ──
  int sc = hb_k(0) * HB_NSCHEME / 1024;
  if (sc > HB_NSCHEME - 1) sc = HB_NSCHEME - 1;
  hb_flash = hb_lightning > 3 ? 1 : 0;
  hb_buildPalette(sc, preset, timeShift);

  // ── draw, back to front ──
  hb_background();
  hb_skyThings();
  hb_land();
  hb_lighthouse();
  hb_shootingStar();
  hb_sea();
  hb_boat();
  hb_dock();
  hb_fireBarrel();
  hb_fisherman();
  hb_weatherFx();
  hb_fogFx();

  // ── ASCII style (right half of the Style knob) ──
  if (style >= 614) {
    int bgQ = 140 - (style - 614) * 120 / 409;                  // further right = less background
    if (csz == 2) hb_asciify(8, 12, bgQ); else if (csz == 1) hb_asciify(4, 6, bgQ); else hb_asciify(3, 5, bgQ);
  }

  // ── the status panel ──
  int totalMin = (baseHour[preset] * 60 + (int)gameMin) % 1440;
  hb_panel(panelMode, totalMin / 60, totalMin % 60);
}

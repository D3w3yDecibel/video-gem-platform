// =====================================================================
// PROGRAM: MOSAIC — landscape mosaic (by Dewey)
// Twelve bright landscapes — mesas, alpine lakes, oceans, city skylines,
// fields, canyons, volcanoes, northern lights — drawn live and broken into
// a mosaic of tiles with grout lines between them. The tiles drift,
// ripple, shuffle, pixelate, slide, mirror and zoom on their own, and every
// so often a wave of tiles FLIPS over like 3D tiles, revealing the same
// scene in another colour world on the back.
// Inspired by m0.studio's "day 1805 — landscape mosaic"
// (https://www.instagram.com/p/DeUq98NiGIK/). The code is original.
//
// Scenes (each with its own tile motion and colour world):
//   k0 Mesa Sunset     — flat-topped mesas and dunes           (Drift)
//   k1 Alpine Lake     — snowy peaks, pine forest, a lake      (Flip Wave)
//   k2 Ocean           — rolling waves under a big striped sun (Ripple)
//   k3 Skyline         — a city at night, lit windows, stars   (Shuffle)
//   k4 Fields          — striped farmland and hedgerows        (Pixel)
//   k5 Canyon          — stepped terraces and cliffs           (Slices)
//   k6 Volcano Isles   — volcanoes rising from the sea         (Quilt)
//   k7 Northern Lights — peaks and pines under an aurora       (Lens)
//   k8 Dune Sea        — endless dunes                         (Slices)
//   k9 Fjord           — steep peaks over still water          (Drift)
//   k10 Neon City      — a synthwave skyline                   (Flip Wave)
//   k11 Pine Valley    — moonlit forest ridges                 (Ripple)
//
//   k12 — FLIP: send a wave of tile flips across now
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 World (centre = the scene's own colours; turn to step through the 8 worlds;
//      the flips step on from here)
//   p1 Speed (far left = freeze)   p2 Tile Size (10 … 60 px)
//   p3 Irregular (how many tiles split into four)   p4 Motion (how far tiles move)
//   p5 Wave Pattern (diagonal / radial / random / rows / spiral)
//   p6 Grout (none … 3 px)   p7 Pan (left = scroll left, centre = still, right = right)
//   p8 Hills (flat … tall)   p9 Sun (high … setting)
//   p10 Layers (centre = the scene's own; fewer … more ridges)
//   p11 Terrain (reshapes the land)   p12 Tint (centre = pure; turn to blend a neighbour world)
//   p13 Clouds   p14 Water (centre = the scene's own level)   p15 Flip Rate (waves every 40 … 6 s; far left = only on k12)
//
// How it works: the height of every ridge is worked out once a frame into a
// small table (every 2 px across the world), so drawing a tile column is just
// a few look-ups and then filling runs of pixels (sky, sun, clouds, hills,
// water). Palette: three scenes in flight (front, back, the one before) × 32
// colours, each with a dimmed copy for tiles caught mid-flip.
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
#include "vg_sin.h"
#define lm_sinT vg_sin512                        // shared sine table (in flash: no RAM)
static inline int lm_sin(uint32_t a) { return lm_sinT[(a >> 7) & 511]; }
static inline float lm_fsin(float r) {           // radians → −1..1 from the table (no slow sinf)
  int a = (int)(r * (65536.0f / 6.2831853f));
  return lm_sinT[((uint32_t)a >> 7) & 511] * (1.0f / 16384.0f);
}
static inline float lm_fcos(float r) { return lm_fsin(r + 1.5707963f); }
static inline int lm_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline uint32_t lm_hash(uint32_t a, uint32_t b) {
  uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
  h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
  return h;
}
static inline int lm_isqrt(int v) {                // integer square root (small numbers)
  if (v <= 0) return 0;
  int r = 1;
  while (r * r <= v) r <<= 1;
  int lo = r >> 1, hi = r;
  while (lo + 1 < hi) { int m = (lo + hi) >> 1; if (m * m <= v) lo = m; else hi = m; }
  return lo;
}
static inline int lm_mod(int a, int m) { int r = a % m; return r < 0 ? r + m : r; }
static const uint8_t lm_bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

// ─── Worlds (colour themes) ───────────────────────────────────────────
#define LM_NWORLD 8
// sky top, sky horizon, sun, sun glow, cloud, far hills, mid hills, near hills, water dark, water light
static const uint8_t lm_worlds[LM_NWORLD][10][3] = {
  {{25,85,205},{185,225,255},{255,255,232},{255,238,170},{255,255,255},{205,222,255},{40,150,92},{20,92,44},{10,50,95},{175,230,255}},     // 0 Alpine
  {{4,4,30},{70,34,120},{238,238,255},{255,214,120},{200,200,240},{80,48,140},{36,30,90},{12,10,44},{6,6,28},{150,150,235}},            // 1 Night
  {{22,0,52},{255,40,160},{255,232,64},{255,92,40},{130,30,150},{126,0,186},{62,0,124},{0,205,225},{22,0,62},{255,62,205}},             // 2 Synthwave
  {{90,150,222},{255,222,172},{255,250,212},{255,190,92},{255,246,232},{152,112,165},{205,92,30},{122,40,20},{40,52,72},{240,202,142}},  // 3 Autumn
  {{30,190,230},{255,205,225},{255,250,225},{255,150,120},{255,236,242},{232,96,150},{238,52,48},{255,172,36},{70,30,90},{255,150,130}},   // 4 Sunset Desert
  {{0,40,92},{150,255,222},{255,255,255},{150,255,202},{202,240,255},{182,222,255},{232,246,255},{120,170,232},{0,30,62},{162,250,232}}, // 5 Arctic Aurora
  {{10,42,12},{202,255,62},{255,92,202},{255,62,162},{152,255,122},{0,142,122},{92,0,142},{32,255,102},{10,20,30},{182,255,92}},         // 6 Toxic
  {{240,235,220},{250,246,232},{222,32,42},{242,122,112},{255,255,250},{172,172,182},{92,92,102},{22,22,32},{62,62,72},{232,232,232}},   // 7 Ink Print
};

// palette layout: scene slot s (0..2) → 32 colours at s*32; dimmed copy at 96 + s*32
//   0-7 sky bands, 8 sun, 9 glow (also lit windows, aurora), 10 cloud (also snow, foam, stars),
//   11 cloud shade, 12-29 six ridges × (rim, body, shade), 30 water, 31 water light
#define LM_DIM 96
#define LM_BG 192                                 // behind a flipping tile
#define LM_EDGE 193                               // a flipping tile's edge
#define LM_GROUT 194
static inline int lm_mix(int a, int b, int t) { return a + (b - a) * t / 256; }   // t 0..256

static void lm_setWorld(int slot, int world, int nextWorld, int tint, int layers) {
  const uint8_t (*c)[3] = lm_worlds[world];
  const uint8_t (*n)[3] = lm_worlds[nextWorld];
  int col[32][3];
  int k3[10][3];
  for (int i = 0; i < 10; i++) for (int ch = 0; ch < 3; ch++) k3[i][ch] = lm_mix(c[i][ch], n[i][ch], tint);
  for (int b = 0; b < 8; b++) for (int ch = 0; ch < 3; ch++) col[b][ch] = lm_mix(k3[0][ch], k3[1][ch], b * 256 / 7);   // sky bands
  for (int ch = 0; ch < 3; ch++) {
    col[8][ch] = k3[2][ch]; col[9][ch] = k3[3][ch];
    col[10][ch] = k3[4][ch]; col[11][ch] = lm_mix(k3[4][ch], k3[1][ch], 110) * 220 / 256;
    col[30][ch] = k3[8][ch]; col[31][ch] = k3[9][ch];
  }
  for (int k = 0; k < 6; k++) {                                  // far → mid → near across the layers
    int t = layers > 1 ? (k < layers ? k * 512 / (layers - 1) : 512) : 0;
    for (int ch = 0; ch < 3; ch++) {
      int body = t < 256 ? lm_mix(k3[5][ch], k3[6][ch], t) : lm_mix(k3[6][ch], k3[7][ch], t - 256);
      col[12 + k * 3][ch] = lm_mix(body, k3[3][ch], 120);        // sunlit rim
      col[13 + k * 3][ch] = body;
      col[14 + k * 3][ch] = body * 150 / 256;                    // shade
    }
  }
  for (int i = 0; i < 32; i++) {
    int r = lm_clamp(col[i][0], 0, 255), g = lm_clamp(col[i][1], 0, 255), b = lm_clamp(col[i][2], 0, 255);
    display.setColor(slot * 32 + i, r, g, b);
    display.setColor(LM_DIM + slot * 32 + i, r * 120 / 256, g * 120 / 256, b * 120 / 256);
  }
}

// ─── Scenes ───────────────────────────────────────────────────────────
#define LT_PEAKS 0
#define LT_ROLL 1
#define LT_MESA 2
#define LT_PINES 3
#define LT_CITY 4
#define LT_DUNES 5
#define LT_WAVES 6
#define LT_TERRACE 7
#define LT_VOLCANO 8
#define LF_SNOW 1
#define LF_STARS 2
#define LF_WINDOWS 4
#define LF_STRIPES 8
#define LF_AURORA 16
#define LF_BIGSUN 32
#define LM_NSCENE 12
// ridge types far → near (up to 6), how many, colour world, tile motion, features,
// water (px up from the bottom, 0 = none), horizon (world y of the farthest ridge)
static const uint8_t lm_sceneL[LM_NSCENE][6] = {
  {LT_MESA, LT_MESA, LT_DUNES, LT_DUNES, LT_DUNES, LT_DUNES},
  {LT_PEAKS, LT_PEAKS, LT_PINES, LT_PINES, LT_PINES, LT_PINES},
  {LT_ROLL, LT_WAVES, LT_WAVES, LT_WAVES, LT_WAVES, LT_WAVES},
  {LT_CITY, LT_CITY, LT_CITY, LT_CITY, LT_CITY, LT_CITY},
  {LT_ROLL, LT_ROLL, LT_ROLL, LT_PINES, LT_ROLL, LT_ROLL},
  {LT_TERRACE, LT_TERRACE, LT_MESA, LT_TERRACE, LT_TERRACE, LT_TERRACE},
  {LT_VOLCANO, LT_ROLL, LT_ROLL, LT_ROLL, LT_ROLL, LT_ROLL},
  {LT_PEAKS, LT_PEAKS, LT_PINES, LT_PINES, LT_PINES, LT_PINES},
  {LT_DUNES, LT_DUNES, LT_DUNES, LT_DUNES, LT_DUNES, LT_DUNES},
  {LT_PEAKS, LT_PEAKS, LT_PEAKS, LT_PEAKS, LT_PEAKS, LT_PEAKS},
  {LT_CITY, LT_CITY, LT_CITY, LT_CITY, LT_CITY, LT_CITY},
  {LT_PEAKS, LT_PINES, LT_PINES, LT_PINES, LT_PINES, LT_PINES},
};
static const uint8_t lm_sceneN[LM_NSCENE]     = {4, 4, 5, 3, 4, 4, 3, 3, 5, 3, 2, 5};
static const uint8_t lm_sceneWorld[LM_NSCENE] = {4, 0, 2, 1, 3, 7, 4, 5, 3, 0, 2, 1};
static const uint8_t lm_sceneMove[LM_NSCENE]  = {0, 1, 2, 3, 4, 5, 6, 7, 5, 0, 1, 2};
static const uint8_t lm_sceneFlag[LM_NSCENE]  = {0, LF_SNOW, LF_BIGSUN, LF_STARS | LF_WINDOWS, LF_STRIPES, 0,
                                                 0, LF_SNOW | LF_AURORA | LF_STARS, 0, LF_SNOW, LF_WINDOWS | LF_BIGSUN, LF_STARS};
static const uint8_t lm_sceneWater[LM_NSCENE] = {0, 42, 0, 0, 0, 0, 56, 0, 0, 62, 0, 0};
static const uint8_t lm_sceneHz[LM_NSCENE]    = {112, 96, 128, 104, 112, 108, 100, 92, 120, 88, 120, 100};

// ─── The landscape ────────────────────────────────────────────────────
#define LM_MAXL 6
#define LM_TN 208                                 // ridge table: every 2 px from world x = −48 (beyond: worked out directly)
#define LM_TX0 (-48)
static int lm_L = 4;
static uint8_t lm_type[LM_MAXL];
static int lm_base[LM_MAXL], lm_amp[LM_MAXL], lm_par[LM_MAXL], lm_pp[LM_MAXL], lm_wv[LM_MAXL];
static uint32_t lm_f[LM_MAXL][3], lm_o[LM_MAXL][3];
static int16_t lm_rt[LM_MAXL][LM_TN];
static int lm_flags = 0;
static int lm_pan = 0;                            // world scroll (px)
static int lm_sunX = 200, lm_sunY = 90, lm_sunR = 18;
static int lm_skyH = 120;                         // sky gradient height
static int lm_waterY = 9999;
static int lm_clouds = 3;
static int lm_t = 0;                              // animation time (frames-ish)
static int lm_r[LM_MAXL];
static int lm_cx[4], lm_cy[4], lm_cw[4], lm_ch[4], lm_ciw[4];   // this frame's clouds

// the height of ridge k at world column wx (world y of its top edge)
static int lm_ridgeAt(int k, int wx) {
  int x = wx + ((lm_pan * lm_par[k]) >> 8) + lm_wv[k];
  uint32_t ux = (uint32_t)x;
  int s1 = lm_sin(ux * lm_f[k][0] + lm_o[k][0]);
  int s2 = lm_sin(ux * lm_f[k][1] + lm_o[k][1]);
  int h;
  switch (lm_type[k]) {
    case LT_PEAKS: h = (16384 - abs(s1)) * 2 - 16384 + (s2 >> 2); break;          // sharp peaks
    case LT_MESA: {                                                               // flat tops, steep sides
      h = (s1 * 10 + s2 * 4) / 14 * 5 / 2;
      if (h > 7000) h = 7000; else if (h < -16384) h = -16384;
      break;
    }
    case LT_PINES: {                                                              // a row of pointed trees
      int P = lm_pp[k], i = (x >= 0 ? x : x - P + 1) / P, ph = x - i * P;
      int th = 7000 + (int)(lm_hash((uint32_t)i, (uint32_t)k * 77u) % 9384u);
      int tri = th - th * abs(2 * ph - P) / P;
      h = s1 / 4 + tri - 8000;
      break;
    }
    case LT_CITY: {                                                               // blocks with flat roofs
      int P = lm_pp[k], i = (x >= 0 ? x : x - P + 1) / P, ph = x - i * P;
      uint32_t hh = lm_hash((uint32_t)i, (uint32_t)k * 977u + 3u);
      h = -6000 + (int)(hh % 20000u);
      if (ph < 2 && (hh & 8)) h = -16384;                                         // an alley
      if ((hh & 0x70) == 0 && abs(ph - P / 2) < 1) h += 6000;                      // an antenna
      break;
    }
    case LT_DUNES: {
      uint32_t a = ux * lm_f[k][0] + lm_o[k][0];
      h = (lm_sin(a) * 12 + lm_sin(a * 2 + 9000) * 5) / 17;
      break;
    }
    case LT_TERRACE: {
      int r = (s1 * 10 + s2 * 5) / 15;
      const int q = 3000;
      h = (r >= 0 ? r / q : (r - q + 1) / q) * q;
      break;
    }
    case LT_VOLCANO: {
      int px = lm_mod(x - HALFW + 230, 460) - 230;                                  // a cone centred on the screen
      int cone = 16384 - abs(px) * 16384 / 95;
      if (cone < 0) cone = 0;
      if (cone > 14000) cone = 14000 - (cone - 14000) * 3;                         // the crater
      int px2 = lm_mod(x - HALFW + 230 - 200, 460) - 230;                          // a smaller one beside it
      int c2 = 10000 - abs(px2) * 10000 / 70;
      if (c2 > cone) cone = c2;
      h = cone - 8000 + (s2 >> 4);
      break;
    }
    default: {                                                                    // rolling hills, waves
      int s3 = lm_sin(ux * lm_f[k][2] + lm_o[k][2]);
      h = (s1 * 10 + s2 * 5 + s3 * 2) / 17;
      break;
    }
  }
  return lm_base[k] - ((h * lm_amp[k]) >> 14);
}
static void lm_buildRidges() {
  for (int k = 0; k < lm_L; k++) {
    int16_t* t = lm_rt[k];
    for (int i = 0; i < LM_TN; i++) t[i] = (int16_t)lm_ridgeAt(k, LM_TX0 + 2 * i);
  }
}
static inline void lm_ridges(int wx) {
  int ix = wx - LM_TX0;
  if (ix >= 0 && ix < 2 * (LM_TN - 1)) {
    int i = ix >> 1;
    if (ix & 1) for (int k = 0; k < lm_L; k++) lm_r[k] = (lm_rt[k][i] + lm_rt[k][i + 1]) >> 1;
    else for (int k = 0; k < lm_L; k++) lm_r[k] = lm_rt[k][i];
  } else {
    for (int k = 0; k < lm_L; k++) lm_r[k] = lm_ridgeAt(k, wx);
  }
}

// sky band per world row (×16, for dithering), rows −64 … H+63
#define LM_ROWOFF 64
static uint8_t lm_skyT[H + 128];
static void lm_buildSky() {
  for (int i = 0; i < H + 128; i++) {
    int wy = lm_clamp(i - LM_ROWOFF, 0, lm_skyH);
    lm_skyT[i] = (uint8_t)lm_clamp(wy * 8 * 16 / (lm_skyH + 1), 0, 7 * 16);   // + dither (0..15) stays within band 7
  }
}

// render n rows of the landscape at world column wx, starting at world row wy0,
// into d (stepping by stride; stride can be negative for an upside-down tile).
// Filled as runs: sky, then each visible ridge, then water; sun, stars, aurora
// and clouds go on top of the sky part.
static void lm_columnTo(uint8_t* d, int stride, int n, int wx, int wy0, int base) {
  lm_ridges(wx);
  // which ridges show, top to bottom: ridge k shows from its edge until a nearer one starts
  int vs[LM_MAXL], vk[LM_MAXL], nv = 0;
  int ok[LM_MAXL];
  int cut = 1 << 30;
  for (int k = lm_L - 1; k >= 0; k--) { ok[k] = lm_r[k] < cut; if (lm_r[k] < cut) cut = lm_r[k]; }
  for (int k = 0; k < lm_L; k++) if (ok[k]) { vs[nv] = lm_r[k]; vk[nv] = k; nv++; }
  int end = wy0 + n;
  int bc[4];
  for (int i = 0; i < 4; i++) bc[i] = lm_bayer[i][wx & 3];
  int skyEnd = nv ? vs[0] : end;
  if (skyEnd > lm_waterY) skyEnd = lm_waterY;
  if (skyEnd > end) skyEnd = end;
  uint8_t* p = d;
  int wy = wy0;
  // ── sky ──
  {
    int e0 = skyEnd < -LM_ROWOFF ? skyEnd : -LM_ROWOFF;
    uint8_t top = (uint8_t)base;
    for (; wy < e0; wy++, p += stride) *p = top;
    int e1 = skyEnd < H + LM_ROWOFF ? skyEnd : H + LM_ROWOFF;
    const uint8_t* st = lm_skyT + LM_ROWOFF;
    for (; wy < e1; wy++, p += stride) *p = (uint8_t)(base + ((st[wy] + bc[wy & 3]) >> 4));
    uint8_t bot = (uint8_t)(base + 7);
    for (; wy < skyEnd; wy++, p += stride) *p = bot;
  }
  // ── ridges ──
  for (int vi = 0; vi < nv && wy < end; vi++) {
    int s1 = vi + 1 < nv ? vs[vi + 1] : end;
    if (s1 > lm_waterY) s1 = lm_waterY;
    if (s1 > end) s1 = end;
    if (wy >= s1) continue;
    int k = vk[vi], r = lm_r[k], ty = lm_type[k];
    uint8_t rim = (uint8_t)(base + 12 + k * 3), body = rim + 1, shade = rim + 2;
    int T = 12 + k * 5;                                           // body → shade, a few rows down
    int rimN = 2;
    if (ty == LT_WAVES) { rim = (uint8_t)(base + 10); rimN = 1; }  // foam
    else if (ty == LT_CITY || ty == LT_PINES) rimN = 1;
    else if (ty == LT_VOLCANO) { rim = (uint8_t)(base + 9); rimN = 3; }          // glowing lava edge
    else if (ty == LT_PEAKS && (lm_flags & LF_SNOW)) {             // snow on the high ground
      int snowLine = lm_base[k] - lm_amp[k] / 4;
      if (r < snowLine) { rim = (uint8_t)(base + 10); rimN = 2 + (snowLine - r) / 3; if (rimN > 16) rimN = 16; }
    }
    if (ty == LT_CITY && (lm_flags & LF_WINDOWS)) {               // lit windows
      uint8_t win = (uint8_t)(base + 9);
      for (; wy < s1; wy++, p += stride) {
        int dd = wy - r;
        if (dd < rimN) { *p = rim; continue; }
        bool lit = ((wx & 3) < 2) && ((wy & 3) < 2) && ((((uint32_t)(wx >> 2) * 0x9E37u) ^ ((uint32_t)(wy >> 2) * 0x85EBu)) >> 9 & 7) < 2;
        *p = lit ? win : (dd < T ? body : shade);
      }
      continue;
    }
    if (ty == LT_ROLL && (lm_flags & LF_STRIPES) && k > 0) {      // ploughed field rows
      for (; wy < s1; wy++, p += stride) {
        int dd = wy - r;
        *p = dd < rimN ? rim : ((((dd + (wx >> 4)) >> 2) & 1) ? body : shade);   // furrows follow the slope
      }
      continue;
    }
    // plain ridge: rim run, body run, a dithered edge, shade run
    int a = r + rimN;
    for (; wy < s1 && wy < a; wy++, p += stride) *p = rim;
    int b = r + T - 2;
    for (; wy < s1 && wy < b; wy++, p += stride) *p = body;
    int c = r + T + 2;
    for (; wy < s1 && wy < c; wy++, p += stride) *p = (wy - r) * 16 + bc[wy & 3] * 3 < T * 16 ? body : shade;
    for (; wy < s1; wy++, p += stride) *p = shade;
  }
  // ── water, with the sun's glint and a few sparkles ──
  if (wy < end) {
    int dxs = wx - lm_sunX;
    uint8_t dark = (uint8_t)(base + 30), light = (uint8_t)(base + 31);
    int adx = abs(dxs);
    int sp = ((wx + (lm_t >> 1)) >> 3) * 37;
    bool sparkCol = (wx & 7) < 4;
    for (; wy < end; wy++, p += stride) {
      int w = wy - lm_waterY;
      bool on = (adx < 3 + (w >> 1) && ((wy + (lm_t >> 1) + (wx >> 2)) % 5) < 2) || (sparkCol && ((sp + wy * 11) & 31) == 0);
      *p = on ? light : dark;
    }
  }
  // ── sun, stars, aurora and clouds, painted over the sky part of this column ──
  int sk0 = wy0, sk1 = skyEnd;
  if (sk1 <= sk0) return;
  int dxs = wx - lm_sunX, gr = lm_sunR + 5;
  if (abs(dxs) < gr) {
    int hg = lm_isqrt(gr * gr - dxs * dxs);
    int hc = abs(dxs) < lm_sunR ? lm_isqrt(lm_sunR * lm_sunR - dxs * dxs) : -1;
    int a = lm_sunY - hg, b = lm_sunY + hg;
    if (a < sk0) a = sk0;
    if (b >= sk1) b = sk1 - 1;
    bool stripes = (lm_flags & LF_BIGSUN) != 0;
    for (int y = a; y <= b; y++) {
      uint8_t* q = d + (y - wy0) * stride;
      if (hc >= 0 && y >= lm_sunY - hc && y <= lm_sunY + hc) {
        if (stripes && y > lm_sunY && ((y - lm_sunY) % 6) < 1 + (y - lm_sunY) / 8) continue;   // the striped sun
        *q = (uint8_t)(base + 8);
      } else if (bc[y & 3] < 9) *q = (uint8_t)(base + 9);
    }
  }
  if (lm_flags & LF_STARS) {
    uint32_t h = (uint32_t)(wx + (lm_pan >> 3)) * 2654435761u;
    if ((h >> 28) == 0) {
      int sy = (int)((h >> 8) % (uint32_t)(lm_skyH * 3 / 4 + 1));
      if (sy >= sk0 && sy < sk1 && (((h >> 4) + (uint32_t)(lm_t >> 3)) & 7) != 0) d[(sy - wy0) * stride] = (uint8_t)(base + 10);
    }
  }
  if (lm_flags & LF_AURORA) {                                    // curtains of light, waving
    int y0 = 12 + ((lm_sin((uint32_t)(wx * 140 + lm_t * 40)) * 10) >> 14) + ((lm_sin((uint32_t)(wx * 37 + 9000)) * 8) >> 14);
    int len = 26 + ((lm_sin((uint32_t)(wx * 90 - lm_t * 25)) * 14) >> 14);
    int a = y0 < sk0 ? sk0 : y0, b = y0 + len < sk1 ? y0 + len : sk1;
    for (int y = a; y < b; y++) if (((y + wx) & 1) == 0 || y < y0 + len / 3) d[(y - wy0) * stride] = (uint8_t)(base + 9);
  }
  for (int c = 0; c < lm_clouds; c++) {
    int dx = wx - lm_cx[c], cw = lm_cw[c];
    if (dx <= -cw || dx >= cw) continue;
    int kk = 256 - dx * dx * lm_ciw[c] / 256;                      // dome top, flatter base
    int cy = lm_cy[c], ch = lm_ch[c];
    int a = cy - (ch * kk >> 8), b = cy + (ch * kk >> 9);
    if (a < sk0) a = sk0;
    if (b >= sk1) b = sk1 - 1;
    uint8_t lit = (uint8_t)(base + 10), dim = (uint8_t)(base + 11);
    uint8_t* q = d + (a - wy0) * stride;
    for (int y = a; y <= b; y++, q += stride) *q = y > cy + 1 ? dim : lit;
  }
}

// clouds: placed once a frame (they drift slowly, wrapping round)
static void lm_placeClouds() {
  for (int c = 0; c < lm_clouds; c++) {
    uint32_t h = lm_hash(c * 7 + 3, 99);
    int cw = 26 + (int)(h % 40), ch = 5 + (int)((h >> 8) % 7);
    int span = 420 + cw * 2;
    int cx = (int)((h >> 4) % (uint32_t)span) + ((lm_pan * 40 >> 8) + (lm_t >> 2) * (1 + c % 2));
    lm_cx[c] = lm_mod(cx, span) - cw;
    lm_cy[c] = 18 + (int)((h >> 12) % 60);
    lm_cw[c] = cw; lm_ch[c] = ch;
    lm_ciw[c] = 65536 / (cw * cw);
  }
}

static uint8_t lm_sample(int wx, int wy, int base) { uint8_t c; lm_columnTo(&c, 1, 1, wx, wy, base); return c; }

// ─── Knobs ────────────────────────────────────────────────────────────
static float lm_sm[16];
static bool lm_smReady = false;
static inline int lm_k(int i) { int v = (int)lm_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_mosaic_name() { return "MOSAIC"; }
const char* prog_mosaic_character() { return "Twelve bright landscapes broken into tiles that drift, ripple, shuffle and flip between colour worlds"; }

static const char* const lm_presetNames[LM_NSCENE] = {
  "Mesa Sunset", "Alpine Lake", "Ocean", "Skyline", "Fields", "Canyon",
  "Volcano Isles", "Northern Lights", "Dune Sea", "Fjord", "Neon City", "Pine Valley"
};

const char* prog_mosaic_presetName(int preset) {
  if (preset >= 0 && preset < LM_NSCENE) return lm_presetNames[preset];
  return NULL;
}

static const char* const lm_labels[16] = {
  "World", "Speed", "Tile Size", "Irregular", "Motion", "Wave Pattern", "Grout", "Pan",
  "Hills", "Sun", "Layers", "Terrain", "Tint", "Clouds", "Water", "Flip Rate"
};
const char* prog_mosaic_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return lm_labels[pot];
  return "";
}

uint8_t prog_mosaic_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we redraw every pixel each frame
}

void prog_mosaic_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
  }
  if (display.getBuffer()) memset(display.getBuffer(), LM_GROUT, W * H);
}

// a tile's place in the wave (0..1023) for the chosen pattern — whole-number maths
static int lm_delay(int pattern, int cx, int cy, uint32_t h) {
  int dx = cx - HALFW, dy = (cy - HALFH) * 3 / 4;
  int ax = abs(dx), ay = abs(dy);
  int r = (ax > ay ? ax + (ay >> 1) : ay + (ax >> 1));            // roughly the distance
  switch (pattern) {
    case 1: return lm_clamp(r * 1637 / W, 0, 1023);                                      // radial
    case 2: return (int)(h & 1023);                                                      // random
    case 3: return lm_clamp(cy * 1023 / H, 0, 1023);                                     // rows
    case 4: {                                                                            // spiral
      int s = ax + ay;
      int ang = s == 0 ? 0 : (dx >= 0 ? (dy >= 0 ? dy * 256 / s : 1024 + dy * 256 / s) : 512 - dy * 256 / s);
      return lm_mod(ang * 7 / 10 + r * 1023 * 3 / (10 * W) * 4, 1024);
    }
    default: return lm_clamp(cx * 614 / W + cy * 409 / H, 0, 1023);                       // diagonal
  }
}

void prog_mosaic_draw(int preset) {
  uint8_t* buf = display.getBuffer();
  if (!buf) return;
  if (preset < 0 || preset >= LM_NSCENE) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!lm_smReady) lm_sm[i] = pots[i];
      else lm_sm[i] += (pots[i] - lm_sm[i]) * a;
    }
    lm_smReady = true;
  }
  int move = lm_sceneMove[preset];

  // ── clocks ──
  float sk = lm_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.6×
  static float tm = 0, panF = 0, flipClock = 0.75f, flipTarget = 0.75f, flipWait = 0, tF = 0;
  tm += dt * speed;
  if (tm > 3600.0f) tm -= 3600.0f;
  tF += dt * speed * 60.0f;
  if (tF > 1.0e6f) tF = 0;
  lm_t = (int)tF;
  float pk = (lm_k(7) - 512) / 512.0f;
  if (pk > -0.05f && pk < 0.05f) pk = 0;
  panF += dt * speed * pk * fabsf(pk) * 120.0f;
  if (panF > 1.0e6f || panF < -1.0e6f) panF = 0;
  lm_pan = (int)panF;

  // flips come in waves: every so often (p15) — or on k12 — the clock steps on by one
  // world; between waves it rests at n + 0.75, where every tile shows world n
  float fk = lm_k(15) / 1023.0f;
  float period = fk < 0.04f ? 0.0f : 40.0f - fk * 34.0f;          // seconds between waves (40 … 6)
  if (move == 1 && (period == 0.0f || period > 9.0f)) period = 9.0f;   // Flip Wave scenes always flip
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) flipTarget += 1.0f;
  k12Was = k12;
  if (period > 0 && speed > 0) {
    flipWait += dt * speed / 0.6f;
    if (flipWait >= period && flipTarget - flipClock < 0.5f) { flipWait = 0; flipTarget += 1.0f; }
  }
  if (flipClock < flipTarget) {                                    // a wave takes ~2.5 s to sweep across
    flipClock += dt * 0.4f * (speed > 0 ? (0.5f + speed / 1.2f) : 0.5f);
    if (flipClock > flipTarget) flipClock = flipTarget;
  }
  if (flipClock > 8000.0f) { flipClock -= 7998.0f; flipTarget -= 7998.0f; }

  // ── knobs → scene ──
  int wofs = lm_k(0) * 8 / 1024 - 4;                              // centre = the scene's own world
  int world0 = lm_mod(lm_sceneWorld[preset] + wofs, LM_NWORLD);
  int S = 10 + lm_k(2) * 50 / 1023;
  float irregular = lm_k(3) / 1023.0f;
  float motion = lm_k(4) / 1023.0f;
  int pattern = lm_k(5) * 5 / 1024; if (pattern > 4) pattern = 4;
  int gk = lm_k(6);
  int grout = gk < 100 ? 0 : (gk < 700 ? 1 : (gk < 940 ? 2 : 3));
  float hills = 0.25f + lm_k(8) * 1.5f / 1023.0f;
  int sunK = lm_k(9);
  int nat = lm_sceneN[preset];
  lm_L = lm_clamp(nat + (lm_k(10) - 512) * 3 / 512, 2, LM_MAXL);
  uint32_t seed = (uint32_t)(lm_k(11) >> 5);
  int tk = lm_k(12) - 512;                                       // Tint: centre = pure; right → next world, left → previous
  int tint = tk > 24 ? (tk - 24) * 256 / 488 : (tk < -24 ? (tk + 24) * 256 / 488 : 0);
  lm_clouds = lm_k(13) * 5 / 1024; if (lm_clouds > 4) lm_clouds = 4;
  lm_flags = lm_sceneFlag[preset];
  int wpx = lm_sceneWater[preset] + (lm_k(14) - 512) * 90 / 512;   // water: px up from the bottom
  lm_waterY = wpx < 4 ? 9999 : 240 - wpx;
  int hz = lm_sceneHz[preset];

  for (int k = 0; k < lm_L; k++) {
    int si = lm_L > 1 ? (k * (nat - 1) * 2 + (lm_L - 1)) / (2 * (lm_L - 1)) : 0;   // spread the scene's ridges over L
    if (si > nat - 1) si = nat - 1;
    int ty = lm_sceneL[preset][si];
    lm_type[k] = (uint8_t)ty;
    uint32_t h = lm_hash(seed * 31 + k + preset * 101, 7);
    float depth = lm_L > 1 ? (float)k / (lm_L - 1) : 0.0f;       // 0 far … 1 near
    lm_base[k] = hz + (int)(depth * (214 - hz));
    float amp, fb;
    switch (ty) {
      case LT_PEAKS:   amp = 52.0f - 14.0f * depth; fb = 2.6f - depth * 0.8f; break;
      case LT_MESA:    amp = 34.0f;                 fb = 1.1f;                break;
      case LT_PINES:   amp = 24.0f + 12.0f * depth; fb = 0.9f;                break;
      case LT_CITY:    amp = 52.0f - 12.0f * depth; fb = 1.0f;                break;
      case LT_DUNES:   amp = 16.0f + 10.0f * depth; fb = 1.7f - depth * 0.6f; break;
      case LT_WAVES:   amp = 3.0f + 6.0f * depth;   fb = 7.0f - depth * 3.0f; break;
      case LT_TERRACE: amp = 30.0f;                 fb = 1.3f;                break;
      case LT_VOLCANO: amp = 80.0f;                 fb = 1.0f;                break;
      default:         amp = 16.0f + 14.0f * depth; fb = 1.4f - depth * 0.5f; break;
    }
    lm_amp[k] = (int)(amp * hills);
    lm_par[k] = 40 + (int)(depth * 216.0f);
    lm_pp[k] = ty == LT_CITY ? 14 + (int)(depth * 14.0f) + (int)(h % 5u) : 9 + (int)(depth * 10.0f);
    lm_wv[k] = ty == LT_WAVES ? (lm_t * (2 + k)) / 3 : 0;          // waves roll along
    lm_f[k][0] = (uint32_t)(fb * 65536.0f / 320.0f * (0.8f + (h & 255) / 640.0f));
    lm_f[k][1] = (uint32_t)(fb * 2.3f * 65536.0f / 320.0f);
    lm_f[k][2] = (uint32_t)(fb * 5.1f * 65536.0f / 320.0f);
    lm_o[k][0] = h; lm_o[k][1] = h >> 7; lm_o[k][2] = h >> 13;
  }
  lm_buildRidges();
  lm_placeClouds();
  lm_skyH = lm_base[0] - 10;
  lm_buildSky();
  lm_sunR = 16 + (sunK > 700 ? (sunK - 700) / 40 : 0);            // bigger as it sets
  if (lm_flags & LF_BIGSUN) lm_sunR = lm_sunR * 7 / 4;
  lm_sunY = 26 + sunK * 110 / 1023;
  lm_sunX = (lm_flags & LF_BIGSUN) ? HALFW : 200;

  // ── palette: the scenes in flight ──
  int n = (int)floorf(flipClock);
  for (int s = n - 1; s <= n + 1; s++) {
    int slot = lm_mod(s, 3);
    int w = lm_mod(world0 + s, LM_NWORLD);
    lm_setWorld(slot, w, tint >= 0 ? (w + 1) % LM_NWORLD : (w + LM_NWORLD - 1) % LM_NWORLD, tint >= 0 ? tint : -tint, lm_L);
  }
  {
    const uint8_t* g = lm_worlds[lm_mod(world0 + n, LM_NWORLD)][0];
    display.setColor(LM_BG, g[0] / 6, g[1] / 6, g[2] / 6);
    display.setColor(LM_EDGE, 235, 235, 235);
    display.setColor(LM_GROUT, 14, 14, 18);
  }

  // ── tiles ──
  int nx = W / S + 2, ny = H / S + 2;
  int ox = (W - nx * S) / 2, oy = (H - ny * S) / 2;
  float tt = tm;                                                 // seconds of animation
  static uint8_t colTmp[H + 8];
  // things every tile of this frame shares
  float shuf = 0.5f - 0.5f * lm_fcos(tt * 0.5f);                  // Shuffle: scramble … home … scramble
  shuf = shuf * shuf * (3.0f - 2.0f * shuf) * (0.3f + motion);

  for (int gy = 0; gy < ny; gy++) for (int gx = 0; gx < nx; gx++) {
    uint32_t th = lm_hash((uint32_t)(gx * 131 + gy * 977), seed + 5);
    bool split = S >= 16 && (th & 1023) < (uint32_t)(irregular * 560.0f);
    int parts = split ? 2 : 1, ps = S / parts;
    for (int sy = 0; sy < parts; sy++) for (int sx = 0; sx < parts; sx++) {
      int x0 = ox + gx * S + sx * ps, y0 = oy + gy * S + sy * ps;
      int tw = (sx == parts - 1) ? S - sx * ps : ps, thh = (sy == parts - 1) ? S - sy * ps : ps;
      if (x0 >= W || y0 >= H || x0 + tw <= 0 || y0 + thh <= 0) continue;
      int cx = x0 + tw / 2, cy = y0 + thh / 2;
      uint32_t h = lm_hash(th, (uint32_t)(sy * 2 + sx + 1));
      int di = lm_delay(pattern, cx, cy, h);
      float d = di * (1.0f / 1024.0f);

      // flip state: local clock; flipping during the first 15 % of each step
      float local = flipClock - 0.6f * d;
      int ln = (int)floorf(local);
      float fr = local - ln;
      int scene = ln;
      float sc = 1.0f;
      if (fr < 0.15f) {
        float ph = fr / 0.15f;
        ph = ph * ph * (3.0f - 2.0f * ph);
        float cs = lm_fcos(ph * 3.14159265f);
        sc = cs < 0 ? -cs : cs;
        scene = ph < 0.5f ? ln - 1 : ln;
      }
      int base = lm_mod(scene, 3) * 32 + (sc < 0.55f ? LM_DIM : 0);

      // tile motion: where in the world this tile looks (dx, dy), mirroring, zoom, pixelation
      int dx = 0, dy = 0, mirX = 0, mirY = 0, pix = 0, iz = 256;
      float ang = (float)(h & 1023) * (6.2831853f / 1024.0f);
      switch (move) {
        case 0: dx = (int)(motion * 14.0f * lm_fsin(tt * 0.9f + ang)); dy = (int)(motion * 8.0f * lm_fsin(tt * 0.7f + ang * 1.7f)); break;
        case 2: { int ax = abs(cx - HALFW), ay = abs(cy - HALFH);
                  int r = ax > ay ? ax + (ay >> 1) : ay + (ax >> 1);
                  dy = (int)(motion * 18.0f * lm_fsin(r * 0.045f - tt * 2.2f)); break; }
        case 3: { int jx = (int)((h >> 10) % 7u) - 3, jy = (int)((h >> 14) % 5u) - 2;
                  dx = (int)(jx * S * shuf); dy = (int)(jy * S * shuf); break; }
        case 4: { float wv = lm_fsin(d * 6.2831853f - tt * 0.8f);       // pixel wave
                  float m = (wv * 0.5f + 0.5f) * (0.4f + motion * 0.8f);
                  pix = m < 0.25f ? 0 : (m < 0.5f ? 2 : (m < 0.75f ? 3 : 4)); break; }
        case 5: { int row = (cy - oy) / S;
                  dx = (int)(motion * 40.0f * lm_fsin(tt * 0.6f + row * 0.9f) * ((row & 1) ? 1.0f : -1.0f)); break; }
        case 6: mirX = (gx + sx) & 1; mirY = (gy + sy) & 1;
                dx = (int)(motion * 10.0f * lm_fsin(tt * 0.5f + ang)); break;
        case 7: { float z = 1.0f + (0.3f + motion) * 0.9f * (0.5f + 0.5f * lm_fsin(tt * 1.1f + d * 6.2831853f));
                  iz = (int)(256.0f / z); break; }
        default: break;
      }

      // clip to the screen; grout around the tile
      int cl = x0 + grout, ct = y0 + grout, cr = x0 + tw, cb = y0 + thh;
      int vx0 = x0 < 0 ? 0 : x0, vy0 = y0 < 0 ? 0 : y0, vx1 = cr > W ? W : cr, vy1 = cb > H ? H : cb;
      if (grout) {
        for (int y = vy0; y < vy1; y++) {
          uint8_t* p = buf + y * W;
          if (y < ct) { memset(p + vx0, LM_GROUT, vx1 - vx0); continue; }
          for (int x = vx0; x < vx1 && x < cl; x++) p[x] = LM_GROUT;
        }
      }
      int ax0 = cl < 0 ? 0 : cl, ay0 = ct < 0 ? 0 : ct;
      if (ax0 >= vx1 || ay0 >= vy1) continue;
      int cw = cr - cl, chh = cb - ct;                            // content size (unclipped)
      int ccx = cl + cw / 2, ccy = ct + chh / 2;
      int vw = (int)(cw * sc + 0.5f); if (vw < 1) vw = 1;
      int left = ccx - vw / 2;
      int rows = vy1 - ay0;

      if (pix) {                                                   // Pixel: flat cells
        int cs = pix == 2 ? 3 : (pix == 3 ? 6 : (cw > chh ? cw : chh));
        if (cs < 2) cs = 2;
        for (int cy0c = ct + ((ay0 - ct) / cs) * cs; cy0c < vy1; cy0c += cs) {
          int ya = cy0c < ay0 ? ay0 : cy0c, ye = cy0c + cs > vy1 ? vy1 : cy0c + cs;
          for (int cx0c = cl + ((ax0 - cl) / cs) * cs; cx0c < vx1; cx0c += cs) {
            int xa = cx0c < ax0 ? ax0 : cx0c, xe = cx0c + cs > vx1 ? vx1 : cx0c + cs;
            uint8_t c = lm_sample(cx0c + cs / 2 + dx, cy0c + cs / 2 + dy, base);
            for (int y = ya; y < ye; y++) memset(buf + y * W + xa, c, xe - xa);
          }
        }
        continue;
      }

      int wyA = 0, nn = 0;
      if (iz != 256) {                                             // Lens: rows this tile needs
        wyA = ccy + (((ay0 - ccy) * iz) >> 8) + dy - 1;
        nn = ((rows * iz) >> 8) + 3;
        if (nn > H + 8) nn = H + 8;
      }
      for (int x = ax0; x < vx1; x++) {
        uint8_t* col = buf + ay0 * W + x;
        if (x < left || x >= left + vw) {                          // behind a flipping tile
          for (int y = 0; y < rows; y++, col += W) *col = LM_BG;
          continue;
        }
        if (sc < 0.97f && (x == left || x == left + vw - 1)) {     // the tile's edge as it turns
          for (int y = 0; y < rows; y++, col += W) *col = LM_EDGE;
          continue;
        }
        int u = (vw > 1) ? (x - left) * cw / vw : 0;               // column within the tile's own picture
        if (mirX) u = cw - 1 - u;
        int srcX = cl + u;
        if (iz != 256) srcX = ccx + (((srcX - ccx) * iz) >> 8);
        int wx = srcX + dx;
        if (iz != 256) {
          lm_columnTo(colTmp, 1, nn, wx, wyA, base);
          int acc = (ay0 - ccy) * iz;
          int base0 = ccy + dy - wyA;
          for (int y = 0; y < rows; y++, col += W, acc += iz) {
            int idx = lm_clamp(base0 + (acc >> 8), 0, nn - 1);
            *col = colTmp[idx];
          }
        } else if (mirY) {
          lm_columnTo(buf + (vy1 - 1) * W + x, -W, rows, wx, ct + cb - vy1 + dy, base);   // upside down
        } else {
          lm_columnTo(col, W, rows, wx, ay0 + dy, base);
        }
      }
    }
  }
}

// =====================================================================
// PROGRAM: SPECTRUM FLOW (by Dewey)
// Silky ribbons of rainbow light: bundles of glowing strands that
// flow, twist and fold, shedding glitter as they go, with light
// trails that linger like long-exposure photos. Inspired by
// marisdreaming's "It's only everything at once" post
// (https://www.instagram.com/p/DdksXzptbFa/). The code is original.
//
// Presets:
//   k0 Spectrum    — one big S-shaped rainbow ribbon rising, shedding sparkles
//   k1 Cascade     — a waterfall of light strands with pulses running down
//   k2 Silk        — three ribbons folding and twisting around each other
//   k3 Laser Fan   — laser beams sweeping from below through the haze
//   k4 Aurora      — curtains of light rippling across the sky
//   k5 Vortex      — comets spiralling round a centre, painting trails
//   k6 Galaxy      — a ribbon wound into a turning spiral
//   k7 Everything  — ribbon, cascade, lasers and glitter all at once
//
//   k12 — BURST: a shower of glitter and a flash
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8 schemes, centre = Spectrum rainbow)   p1 Speed (far left = freeze)
//   p2 Strands   p3 Width   p4 Flow (how much it swings)   p5 Twist (folds)
//   p6 Trails (how long light lingers)   p7 Glitter   p8 Glow (brightness)
//   p9 Sparkle (star glints)   p10 Mirror (off/2/4)   p11 Drift (trails rise/fall)
//   p12 Color Cycle   p13 Pop (contrast)   p14 Hue Spread (rainbow width)
//   p15 Background (light columns + glowing river)
//
// How the colours work: every pixel stores a brightness (0–15) and a
// hue (0–15). Light is ADDED, so where strands cross they brighten
// toward white, and each frame the brightness fades a little (trails).
// All per-pixel work is whole-number maths (fast on the RP2040).
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
static int16_t sf_sinT[1024];                  // Q14 sine, 1024 steps per turn
static inline int sf_sin(int a) { return sf_sinT[a & 1023]; }
static inline int sf_cos(int a) { return sf_sinT[(a + 256) & 1023]; }
static inline int sf_dist(int dx, int dy) {    // max + 3/8·min
  if (dx < 0) dx = -dx;
  if (dy < 0) dy = -dy;
  return dx > dy ? dx + (dy * 3 >> 3) : dy + (dx * 3 >> 3);
}
static uint32_t sf_rng = 12345;
static inline uint32_t sf_rand() { sf_rng ^= sf_rng << 13; sf_rng ^= sf_rng >> 17; sf_rng ^= sf_rng << 5; return sf_rng; }
static inline uint32_t sf_hash(int a, int b) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

// ─── Colours ──────────────────────────────────────────────────────────
// palette index = (brightness << 4) | hue
#define SF_NSCHEME 8
static const char* const sf_schemeNames[SF_NSCHEME] = {
  "Spectrum", "Neon Laser", "Aurora", "Fire Opal", "Ice", "Candy", "Ultraviolet", "Moonlight"
};
// 8 colour stops per scheme, going round (hues 0–15 blend between them)
static const uint8_t sf_stops[SF_NSCHEME][8][3] = {
  {{255,0,40},{255,120,0},{255,230,0},{0,255,80},{0,220,255},{0,60,255},{150,0,255},{255,0,200}},     // Spectrum
  {{255,0,120},{255,0,255},{140,0,255},{40,40,255},{0,200,255},{120,255,255},{0,120,255},{255,40,200}},// Neon Laser
  {{0,255,120},{0,255,200},{0,180,255},{80,80,255},{180,0,255},{255,0,200},{60,0,180},{0,200,120}},   // Aurora
  {{255,0,0},{255,90,0},{255,190,0},{255,120,160},{220,0,140},{140,0,200},{255,60,60},{255,160,40}},  // Fire Opal
  {{0,40,255},{0,160,255},{120,230,255},{230,250,255},{170,160,255},{90,60,255},{0,220,240},{60,120,255}}, // Ice
  {{255,90,170},{255,170,120},{150,255,190},{190,150,255},{255,120,220},{120,220,255},{255,230,150},{230,120,255}}, // Candy
  {{120,0,255},{60,0,255},{0,60,255},{200,0,255},{255,0,180},{80,0,200},{160,80,255},{255,60,255}},  // Ultraviolet
  {{200,210,255},{255,255,255},{160,190,255},{230,220,255},{255,250,230},{150,170,230},{220,240,255},{255,255,255}}, // Moonlight
};
static uint8_t sf_curve[256];
static int sf_satK = 256;
static int sf_flash = 0;

static void sf_buildPalette(int scheme, int hueOff, int pop) {
  int k = pop >> 2;
  for (int v = 0; v < 256; v++) {
    int sc = v * v * (765 - 2 * v) / 65025;
    sf_curve[v] = (uint8_t)(v + ((sc - v) * k >> 8));
  }
  sf_satK = 256 + pop / 4;
  for (int h = 0; h < 16; h++) {
    // hue h, turned by the colour cycle (hueOff in 1/16ths of a hue step)
    int pos = ((h * 16 + hueOff) & 255) * 8;                  // 0 … 2047 round the 8 stops
    int s0 = pos >> 8, f = pos & 255, s1 = (s0 + 1) & 7;
    int br = (sf_stops[scheme][s0][0] * (256 - f) + sf_stops[scheme][s1][0] * f) >> 8;
    int bg = (sf_stops[scheme][s0][1] * (256 - f) + sf_stops[scheme][s1][1] * f) >> 8;
    int bb = (sf_stops[scheme][s0][2] * (256 - f) + sf_stops[scheme][s1][2] * f) >> 8;
    int l0 = (br * 77 + bg * 150 + bb * 29) >> 8;
    br = l0 + ((br - l0) * sf_satK >> 8); bg = l0 + ((bg - l0) * sf_satK >> 8); bb = l0 + ((bb - l0) * sf_satK >> 8);
    br = br < 0 ? 0 : br > 255 ? 255 : br; bg = bg < 0 ? 0 : bg > 255 ? 255 : bg; bb = bb < 0 ? 0 : bb > 255 ? 255 : bb;
    display.setColor(h, 0, 0, 0);                              // brightness 0 = black
    for (int l = 1; l < 16; l++) {
      int r, g, b;
      if (l <= 11) {                                           // dim → full colour, bright early
        static const uint8_t gain[12] = {0, 34, 62, 88, 112, 136, 158, 180, 200, 220, 238, 255};
        int m = gain[l];
        r = br * m >> 8; g = bg * m >> 8; b = bb * m >> 8;
      } else {                                                 // hot core: toward white
        int wv = (l - 11) * 56;
        r = br + ((255 - br) * wv >> 8); g = bg + ((255 - bg) * wv >> 8); b = bb + ((255 - bb) * wv >> 8);
      }
      if (sf_flash) { r = (r + 255) >> 1; g = (g + 255) >> 1; b = (b + 255) >> 1; }
      display.setColor((l << 4) | h, sf_curve[r], sf_curve[g], sf_curve[b]);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* sf_buf;

// add light (l) of hue h at a pixel; brightness piles up toward white
static inline void sf_add(int x, int y, int h, int l) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t* p = sf_buf + y * W + x;
  int nl = (*p >> 4) + l;
  *p = (uint8_t)(((nl > 15 ? 15 : nl) << 4) | h);
}
// strand light: keeps the brighter of old/new (so trails don't pile up
// into white), and where a different-coloured strand crosses, glows a
// little brighter
static inline void sf_light(int x, int y, int h, int l) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t* p = sf_buf + y * W + x;
  int ol = *p >> 4;
  if (l > ol) *p = (uint8_t)((l << 4) | h);
  else if (ol < 13 && ol > 0 && (*p & 15) != h && ol - l < 3) *p = (uint8_t)(((ol + 1) << 4) | h);
}
// only brighten up to l (for the background, so it never piles up)
static inline void sf_max(int x, int y, int h, int l) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t* p = sf_buf + y * W + x;
  if ((*p >> 4) < l) *p = (uint8_t)((l << 4) | h);
}

// a line of added light; skip=1 leaves out the first pixel (so joined
// segments don't double up at the joints)
static int sf_thick = 1;                                        // 2 = strands drawn 2 px wide

static inline void sf_lightP(uint8_t* p, int h, int l) {        // sf_light without the bounds check
  int ol = *p >> 4;
  if (l > ol) *p = (uint8_t)((l << 4) | h);
  else if (ol < 13 && ol > 0 && (*p & 15) != h && ol - l < 3) *p = (uint8_t)(((ol + 1) << 4) | h);
}

// a line of strand light; skip=1 leaves out the first pixel (so joined
// segments don't double up at the joints)
static void sf_seg(int x0, int y0, int x1, int y1, int h, int l, int skip) {
  // entirely off one side of the screen: nothing to draw
  if ((x0 < 0 && x1 < 0) || (x0 >= W && x1 >= W) || (y0 < 0 && y1 < 0) || (y0 >= H && y1 >= H)) return;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  if (dx > 1000 || dy < -1000) return;                          // wildly off-screen
  int err = dx + dy;
  int tx = dx > -dy ? 0 : 1, ty = 1 - tx;                        // thicken across the line
  int l2 = l - 1;
  bool thick = sf_thick > 1 && l2 > 0;
  // fast path: the whole segment (and its thickening) is on screen
  if (x0 >= 0 && x1 >= 0 && x0 < W - 1 && x1 < W - 1 && y0 >= 0 && y1 >= 0 && y0 < H - 1 && y1 < H - 1) {
    uint8_t* p = sf_buf + y0 * W + x0;
    int side = tx + ty * W, stepY = sy * W;
    for (;;) {
      if (!skip) { sf_lightP(p, h, l); if (thick) sf_lightP(p + side, h, l2); }
      skip = 0;
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; p += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; p += stepY; }
    }
    return;
  }
  for (;;) {
    if (!skip) {
      sf_light(x0, y0, h, l);
      if (thick) sf_light(x0 + tx, y0 + ty, h, l2);
    }
    skip = 0;
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// a star glint: a cross of light fading along its arms
static void sf_star(int x, int y, int h, int len) {
  sf_add(x, y, h, 15);
  for (int k = 1; k <= len; k++) {
    int l = 12 - k * 11 / (len + 1);
    sf_add(x + k, y, h, l); sf_add(x - k, y, h, l);
    sf_add(x, y + k, h, l); sf_add(x, y - k, h, l);
  }
}

// Fade 4 packed pixels at once: each byte's brightness nibble drops by
// dec (stopping at 0), its hue nibble is kept.
static inline uint32_t sf_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;                          // four brightness values
  uint32_t t = (h | 0x80808080u) - d4;                          // borrow can't cross bytes
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;                // bytes that didn't go below 0
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}

// Fade every pixel's brightness by `dec`, optionally sliding the whole
// picture up (drift > 0) or down (drift < 0).
static void sf_fade(int dec, int drift) {
  uint32_t* b = (uint32_t*)sf_buf;
  const int WW = W / 4;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  if (drift > 0) {
    for (int y = 0; y < H - drift; y++) {
      uint32_t* d = b + y * WW; const uint32_t* s = b + (y + drift) * WW;
      for (int i = 0; i < WW; i++) d[i] = sf_fade4(s[i], d4);
    }
    memset(sf_buf + (H - drift) * W, 0, drift * W);
  } else if (drift < 0) {
    int dd = -drift;
    for (int y = H - 1; y >= dd; y--) {
      uint32_t* d = b + y * WW; const uint32_t* s = b + (y - dd) * WW;
      for (int i = 0; i < WW; i++) d[i] = sf_fade4(s[i], d4);
    }
    memset(sf_buf, 0, dd * W);
  } else {
    for (int i = 0; i < WW * H; i++) b[i] = sf_fade4(b[i], d4);
  }
}

static void sf_mirror(int mode) {
  if (mode <= 0) return;
  for (int y = 0; y < H; y++) {
    uint8_t* row = sf_buf + y * W;
    for (int x = 0; x < W / 2; x++) row[W - 1 - x] = row[x];
  }
  if (mode >= 2)
    for (int y = 0; y < H / 2; y++) memcpy(sf_buf + (H - 1 - y) * W, sf_buf + y * W, W);
}

// ─── Knobs & per-frame settings ───────────────────────────────────────
static float sf_sm[16];
static bool sf_smReady = false;
static inline int sf_k(int i) { int v = (int)sf_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

static int sf_t = 0;                 // clock (ticks)
static int sf_strands = 20, sf_width = 512, sf_flow = 512, sf_twist = 512;
static int sf_glitter = 512, sf_glow = 8, sf_sparkle = 512, sf_spread = 16, sf_bg = 512;

// ─── Glitter particles ────────────────────────────────────────────────
#define SF_NP 320
static int16_t sf_px[SF_NP], sf_py[SF_NP], sf_vx[SF_NP], sf_vy[SF_NP];   // Q4
static uint8_t sf_life[SF_NP], sf_ph[SF_NP];
static int sf_pNext = 0;

static void sf_spawn(int x, int y, int vx, int vy, int h) {       // x, y in px; v in Q4 px/tick
  int i = sf_pNext;
  sf_pNext = (sf_pNext + 1) % SF_NP;
  sf_px[i] = (int16_t)(x * 16); sf_py[i] = (int16_t)(y * 16);
  sf_vx[i] = (int16_t)vx; sf_vy[i] = (int16_t)vy;
  sf_life[i] = (uint8_t)(20 + (sf_rand() % 50));
  sf_ph[i] = (uint8_t)(h | ((sf_rand() & 3) << 4));
}

static void sf_particles(int steps) {
  for (int i = 0; i < SF_NP; i++) {
    if (!sf_life[i]) continue;
    for (int s = 0; s < steps && sf_life[i]; s++) {
      sf_px[i] += sf_vx[i]; sf_py[i] += sf_vy[i];
      sf_vy[i] -= 1;                                             // drifts upward, like sparks in a draught
      sf_vx[i] = (int16_t)(sf_vx[i] * 31 / 32);
      sf_life[i]--;
    }
    int x = sf_px[i] >> 4, y = sf_py[i] >> 4;
    if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) { sf_life[i] = 0; continue; }
    int tw = ((sf_t + i * 7) >> ((sf_ph[i] >> 4) + 1)) & 3;     // twinkle
    int l = tw == 0 ? 15 : 6 + (sf_life[i] >> 3);
    sf_add(x, y, sf_ph[i] & 15, l > 15 ? 15 : l);
  }
}

// ─── Ribbons ──────────────────────────────────────────────────────────
// A ribbon = a spine (sample points) with a half-width at each point.
// Each strand runs alongside the spine at its own offset; twisting the
// offsets makes the ribbon fold over like silk.
#define SF_NS 56
static int sf_sx[SF_NS], sf_sy[SF_NS], sf_sw[SF_NS];            // Q4 px

// n samples; twist phase/amount (angle units per sample); hue base and
// spread (1/16 hue steps); light level; glitter chance (0–1023)
// topDis (0–1024): from this far along, strands break up into glitter
static void sf_ribbon(int n, int strands, int twA, int twP, int hueB, int spread, int lvl, int glit, int topDis) {
  int nx[SF_NS], ny[SF_NS], tw[SF_NS];
  for (int i = 0; i < n; i++) {
    int a = i > 0 ? i - 1 : 0, b = i < n - 1 ? i + 1 : n - 1;
    int dx = sf_sx[b] - sf_sx[a], dy = sf_sy[b] - sf_sy[a];
    int len = sf_dist(dx, dy);
    if (len < 1) len = 1;
    nx[i] = -dy * 4096 / len; ny[i] = dx * 4096 / len;          // unit normal, Q12
    tw[i] = sf_cos(twP + i * twA);                               // fold factor, Q14
  }
  if (strands < 1) strands = 1;
  for (int k = 0; k < strands; k++) {
    int f = strands > 1 ? (k * 2 - (strands - 1)) * 1024 / (strands - 1) : 0;   // −1024 … 1024
    int h = ((hueB + k * spread) >> 4) & 15;
    int jig = (int)(sf_hash(k, 3) & 7) - 3;                     // strands aren't perfectly even
    int px = 0, py = 0;
    bool edge = (k == 0 || k == strands - 1);
    for (int i = 0; i < n; i++) {
      int off = ((sf_sw[i] * f >> 10) + jig * 4) * tw[i] >> 14;  // Q4
      int x = (sf_sx[i] + (nx[i] * off >> 12)) >> 4, y = (sf_sy[i] + (ny[i] * off >> 12)) >> 4;
      // strands facing us are brighter than those folding away
      int at = tw[i] < 0 ? -tw[i] : tw[i];
      int l = lvl * (8 + (at >> 11)) >> 4;
      if (l < 1) l = 1;
      int dis = 0;                                              // 0 … 1024: how broken up it is here
      if (topDis && i * 1024 / n > topDis) dis = (i * 1024 / n - topDis) * 1024 / (1024 - topDis);
      bool on = !dis || (int)(sf_hash(k * 64 + i, sf_t >> 2) & 1023) > dis;
      if (i && on) sf_seg(px, py, x, y, h, l, 1);
      else if (on) sf_light(x, y, h, l);
      px = x; py = y;
      if (dis && glit && (int)(sf_rand() & 4095) < glit * dis >> 9) {   // dissolving strands shed glitter
        sf_spawn(x, y, (nx[i] >> 9) * ((k & 1) ? 1 : -1) + (int)(sf_rand() % 25) - 12, (int)(sf_rand() % 25) - 16, h);
      }
      if (edge && glit && (int)(sf_rand() & 1023) < glit) {
        int sgn = k == 0 ? -1 : 1;
        sf_spawn(x, y, (sgn * nx[i] >> 9) + (int)(sf_rand() % 17) - 8, (sgn * ny[i] >> 9) + (int)(sf_rand() % 17) - 8, h);
      }
    }
  }
}

// ─── Background: faint light columns and a glowing river ──────────────
static void sf_background(int t) {
  if (sf_bg < 40) return;
  int l = 1 + (sf_bg >> 8);                                     // 1 … 4
  for (int c = 0; c < 5; c++) {
    uint32_t h = sf_hash(c, 9);
    int cx = (int)(h % W), cw = 6 + (int)((h >> 8) % 18);
    int hue = 10 + (int)((h >> 16) & 3);
    int x1 = cx + cw > W ? W : cx + cw;
    uint8_t v = (uint8_t)((l << 4) | hue);
    for (int y = (int)((h >> 20) & 1); y < H; y += 2) {
      uint8_t* row = sf_buf + y * W;
      for (int x = cx + ((y & 2) ? 1 : 0); x < x1; x += 2)
        if ((row[x] >> 4) < l) row[x] = v;
    }
  }
  // the river along the bottom
  for (int k = 0; k < 6; k++) {
    int px = 0, py = 0;
    for (int x = 0; x <= W; x += 8) {
      int y = H - 14 + k * 2 + (sf_sin(x * 3 + t * 4 + k * 60) * 5 >> 14) + (sf_sin(x * 7 - t * 3) * 2 >> 14);
      if (x) sf_seg(px, py, x, y, 9 + (k & 3), 1 + (l >> 1), 1);
      px = x; py = y;
    }
  }
}

static void sf_sparkles(int t) {
  int n = sf_sparkle >> 7;                                       // 0 … 7 glints a frame
  for (int i = 0; i < n; i++) {
    if ((sf_rand() & 3) != 0) continue;
    int x = (int)(sf_rand() % W), y = (int)(sf_rand() % H);
    // only where there's light already (sparkles live in the glow)
    if ((sf_buf[y * W + x] >> 4) < 3) continue;
    sf_star(x, y, sf_buf[y * W + x] & 15, 2 + (int)(sf_rand() % 5));
  }
}

// ─── Scenes ───────────────────────────────────────────────────────────
static int sf_hueB(int t) { return t * 2; }                     // strands' hues slowly slide

// k0: the big S-ribbon rising from the bottom left, fanning out at the top
static void sf_spineS(int t, int cx, int amp, int wid, int phase) {
  for (int i = 0; i < SF_NS; i++) {
    int s = i * 1024 / (SF_NS - 1);                             // 0 … 1024 along the ribbon
    int y = (H + 30) - s * (H + 60) / 1024;
    int x = cx + (sf_sin(s * 3 / 4 - t * 3 + phase) * amp >> 14) + (sf_sin(s * 2 + t * 2 + phase) * amp / 3 >> 14);
    sf_sx[i] = x * 16; sf_sy[i] = y * 16;
    int fan = 300 + (sf_sin(s / 2) * 724 >> 14);                // wide in the middle/top
    sf_sw[i] = (wid * fan >> 10) * 16;
  }
}

static void sf_sceneSpectrum(int t) {
  sf_background(t);
  int amp = 30 + (sf_flow * 90 >> 10);
  int wid = 16 + (sf_width * 80 >> 10);
  sf_spineS(t, W / 2 + 10, amp, wid, 0);
  sf_ribbon(SF_NS, sf_strands, 4 + (sf_twist >> 6), t * 3, sf_hueB(t), sf_spread, sf_glow, sf_glitter >> 2, 640);
}

// k1: a waterfall of strands with pulses of light running down
static void sf_cascade(int t, int cx, int wid, int nStr, int lvlBase) {
  for (int k = 0; k < nStr; k++) {
    uint32_t hk = sf_hash(k, 1);
    int x0 = cx + (int)(hk % (uint32_t)(2 * wid + 1)) - wid;
    int h = ((sf_hueB(t) + 160 + k * (sf_spread >> 3)) >> 4) & 15;
    int sp = 3 + (int)((hk >> 8) % 4);
    int px = x0, py = -2;
    for (int y = 0; y <= H; y += 6) {
      int sway = sf_sin(y * 3 + t * 2 + k * 40) * (2 + (sf_flow >> 7)) >> 14;
      int x = x0 + sway + (sf_sin(y + t + k * 97) * (sf_flow >> 6) >> 14);
      // pulses running down each strand
      int ph = (y * 4 - t * sp * 6 + (int)(hk >> 16)) & 255;
      int l = lvlBase + (ph < 40 ? (40 - ph) / 6 : 0);
      sf_seg(px, py, x, y, h, l, 1);
      px = x; py = y;
    }
    if (sf_glitter && (int)(sf_rand() & 1023) < sf_glitter / 4)            // splash at the foot
      sf_spawn(px, H - 3, (int)(sf_rand() % 41) - 20, -(int)(sf_rand() % 40) - 10, h);
  }
}

static void sf_sceneCascade(int t) {
  sf_background(t);
  sf_cascade(t, W / 2, 20 + (sf_width * 90 >> 10), sf_strands + 6, sf_glow - 4);
}

// k2: three ribbons weaving round each other
static void sf_sceneSilk(int t) {
  sf_background(t);
  int amp = 30 + (sf_flow * 70 >> 10);
  int wid = 8 + (sf_width * 36 >> 10);
  for (int r = 0; r < 3; r++) {
    int ph = r * 341;
    for (int i = 0; i < SF_NS; i++) {
      int s = i * 1024 / (SF_NS - 1);
      int x = -30 + s * (W + 60) / 1024;                        // left → right
      int y = H / 2 + (sf_sin(s + t * 2 + ph) * amp >> 14) + (sf_sin(s * 3 - t * 3 + ph) * amp / 4 >> 14);
      sf_sx[i] = x * 16; sf_sy[i] = y * 16;
      sf_sw[i] = (wid * (600 + (sf_sin(s * 2 + t + ph) * 424 >> 14)) >> 10) * 16;
    }
    sf_ribbon(SF_NS, (sf_strands + 4) / 2, 8 + (sf_twist >> 5), t * 4 + ph, sf_hueB(t) + r * 85, sf_spread, sf_glow, sf_glitter >> 5, 0);
  }
}

// k3: laser beams fanning up from below, sweeping
static void sf_lasers(int t, int ox, int oy, int n, int lvl, int spreadA) {
  for (int k = 0; k < n; k++) {
    int f = n > 1 ? (k * 2 - (n - 1)) * 1024 / (n - 1) : 0;
    int a = 768 + (f * spreadA >> 10) + (sf_sin(t * 3 + k * 50) * (sf_flow >> 3) >> 14);   // 768 = straight up
    int ex = ox + (sf_cos(a) * 400 >> 14), ey = oy + (sf_sin(a) * 400 >> 14);
    int h = ((sf_hueB(t) + k * sf_spread) >> 4) & 15;
    sf_seg(ox, oy, ex, ey, h, lvl, 0);
    if (sf_width > 300) sf_seg(ox + 1, oy, ex + 1, ey, h, lvl >> 1, 0);     // thicker beams
  }
}

static void sf_sceneLasers(int t) {
  sf_background(t);
  int ox = W / 2 + (sf_sin(t) * W / 4 >> 14);
  sf_lasers(t, ox, H + 4, sf_strands, sf_glow - 2, 120 + (sf_twist >> 2));
  // haze: the beams light up drifting motes
  for (int i = 0; i < (sf_glitter >> 4); i++) {
    int x = (int)(sf_rand() % W), y = (int)(sf_rand() % H);
    if ((sf_buf[y * W + x] >> 4) >= 4) sf_spawn(x, y, (int)(sf_rand() % 9) - 4, -(int)(sf_rand() % 6), sf_buf[y * W + x] & 15);
  }
}

// k4: aurora curtains
static void sf_sceneAurora(int t) {
  sf_background(t);
  for (int c = 0; c < 3; c++) {
    int base = H / 5 + c * H / 7;
    int amp = 10 + (sf_flow * 30 >> 10);
    int len = 20 + (sf_width * 70 >> 10);
    for (int x = 0; x < W; x += 2) {
      int y0 = base + (sf_sin(x * 2 + t * 2 + c * 200) * amp >> 14) + (sf_sin(x * 5 - t * 3) * amp / 3 >> 14);
      int ll = len * (12 + (sf_sin(x * 6 + t * 5 + c * 99) >> 11)) >> 4;   // ray length varies
      int h = ((sf_hueB(t) + x * sf_spread / 20 + c * 60) >> 4) & 15;
      int top = sf_glow - 3;
      for (int y = 0; y < ll; y += 2) {
        int l = top - y * top / (ll + 1);
        if (l <= 0) break;
        sf_light(x, y0 + y, h, l);
        sf_light(x, y0 + y + 1, h, l);
      }
    }
  }
}

// k5: comets spiralling round the centre
static void sf_sceneVortex(int t) {
  sf_background(t);
  int n = sf_strands * 3;
  int cx = W / 2 + (sf_sin(t) * (sf_flow >> 4) >> 14), cy = H / 2;
  for (int i = 0; i < n; i++) {
    uint32_t h = sf_hash(i, 5);
    int spd = 3 + (int)(h % 5) + (sf_twist >> 8);
    int a0 = (int)(h >> 8) + t * spd, a1 = a0 - spd * 8;
    int r0 = 20 + (int)((h >> 16) % (uint32_t)(40 + (sf_width >> 3))) + (sf_sin(t * 2 + i * 31) * (sf_flow >> 5) >> 14);
    int hue = ((sf_hueB(t) + i * sf_spread) >> 4) & 15;
    int x0 = cx + (sf_cos(a0) * r0 >> 14), y0 = cy + (sf_sin(a0) * r0 * 3 / 4 >> 14);
    int x1 = cx + (sf_cos(a1) * r0 >> 14), y1 = cy + (sf_sin(a1) * r0 * 3 / 4 >> 14);
    sf_seg(x1, y1, x0, y0, hue, sf_glow - 2, 0);
    sf_add(x0, y0, hue, 15);
    if ((int)(sf_rand() & 1023) < (sf_glitter >> 3)) sf_spawn(x0, y0, (int)(sf_rand() % 21) - 10, (int)(sf_rand() % 21) - 10, hue);
  }
}

// k6: a ribbon wound into a turning spiral
static void sf_sceneGalaxy(int t) {
  sf_background(t);
  int turns = 900 + (sf_flow * 900 >> 10);
  int wid = 4 + (sf_width * 24 >> 10);
  for (int arm = 0; arm < 2; arm++) {
    for (int i = 0; i < SF_NS; i++) {
      int s = i * 1024 / (SF_NS - 1);
      int r = 6 + s * (H / 2 + 20) / 1024;
      int a = s * turns / 1024 + t * 2 + arm * 512;
      sf_sx[i] = (W / 2 + (sf_cos(a) * r >> 14)) * 16;
      sf_sy[i] = (H / 2 + (sf_sin(a) * r * 4 / 5 >> 14)) * 16;
      sf_sw[i] = (wid * (200 + s) >> 10) * 16;
    }
    sf_ribbon(SF_NS, (sf_strands + 1) / 2 + 2, 6 + (sf_twist >> 6), t * 3 + arm * 300, sf_hueB(t) + arm * 128, sf_spread, sf_glow, sf_glitter >> 5, 820);
  }
}

// k7: everything at once
static void sf_sceneEverything(int t) {
  sf_background(t);
  sf_lasers(t, W / 2 + (sf_sin(t * 2) * W / 3 >> 14), H + 4, 5, sf_glow >> 1, 200);
  sf_cascade(t, W / 6, 18, 10, sf_glow >> 1);
  int amp = 30 + (sf_flow * 80 >> 10);
  sf_spineS(t, W / 2 + 30, amp, 14 + (sf_width * 64 >> 10), 0);
  sf_ribbon(SF_NS, sf_strands, 4 + (sf_twist >> 6), t * 3, sf_hueB(t), sf_spread, sf_glow, sf_glitter >> 2, 640);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_spectrum_name() { return "SPECTRUM FLOW"; }
const char* prog_spectrum_character() { return "Silky rainbow light ribbons, cascades, lasers and glitter with long light trails"; }

static const char* const sf_presetNames[] = {
  "Spectrum", "Cascade", "Silk", "Laser Fan", "Aurora", "Vortex", "Galaxy", "Everything"
};
#define SF_NUM_PRESETS 8

const char* prog_spectrum_presetName(int preset) {
  if (preset >= 0 && preset < SF_NUM_PRESETS) return sf_presetNames[preset];
  return NULL;
}

static const char* const sf_labels[16] = {
  "Palette", "Speed", "Strands", "Width", "Flow", "Twist", "Trails", "Glitter",
  "Glow", "Sparkle", "Mirror", "Drift", "Color Cycle", "Pop", "Hue Spread", "Background"
};
const char* prog_spectrum_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return sf_labels[pot];
  return "";
}

uint8_t prog_spectrum_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we keep the last frame and fade it ourselves (trails)
}

void prog_spectrum_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    for (int i = 0; i < 1024; i++) sf_sinT[i] = (int16_t)lroundf(sinf(i * 6.2831853f / 1024.0f) * 16384.0f);
  }
  memset(sf_life, 0, sizeof(sf_life));
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);   // start from black
}

void prog_spectrum_draw(int preset) {
  if (sf_sinT[256] == 0) prog_spectrum_init();
  sf_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!sf_smReady) sf_sm[i] = pots[i];
      else sf_sm[i] += (pots[i] - sf_sm[i]) * a;
    }
    sf_smReady = true;
  }

  // clock
  float sk = sf_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);
  static float tf = 0, stepAcc = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e7f) tf = 0;
  int t = (int)tf;
  sf_t = t;
  stepAcc += dt * 60.0f * speed;                                // particle steps this frame
  int steps = (int)stepAcc;
  stepAcc -= steps;
  if (steps > 4) steps = 4;

  // knobs
  sf_strands = 3 + sf_k(2) * 37 / 1023;                          // 3 … 40
  sf_thick = 2;
  sf_width = sf_k(3);
  sf_flow = sf_k(4);
  sf_twist = sf_k(5);
  int trails = sf_k(6);
  sf_glitter = sf_k(7);
  sf_glow = 5 + sf_k(8) * 10 / 1023;                             // 5 … 15
  sf_sparkle = sf_k(9);
  int mir = sf_k(10) * 3 / 1024;
  int dk = sf_k(11);
  int drift = dk < 300 ? -1 : dk < 440 ? 0 : dk < 584 ? 0 : dk < 800 ? 1 : 2;   // centre = none
  if (dk < 120) drift = -2;
  sf_spread = sf_k(14) * 48 / 1023;                              // 0 (one colour) … 3 hues per strand
  sf_bg = sf_k(15);

  // k12: BURST
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) {
    for (int i = 0; i < 160; i++) {
      int a = (int)(sf_rand() & 1023), v = 20 + (int)(sf_rand() % 60);
      sf_spawn(W / 2, H / 2, sf_cos(a) * v >> 14, sf_sin(a) * v >> 14, (int)(sf_rand() & 15));
    }
    sf_flash = 3;
  }
  k12Was = k12;

  // colours: p0 scheme (centre = Spectrum), p12 cycle (real time), p13 pop
  static const uint8_t order[SF_NSCHEME] = {7, 4, 2, 1, 0, 5, 3, 6};   // Moonlight … Spectrum (centre) … Ultraviolet
  int pos = sf_k(0) * SF_NSCHEME / 1024;
  if (pos > SF_NSCHEME - 1) pos = SF_NSCHEME - 1;
  static float hueF = 0;
  float cy = sf_sm[12] / 1023.0f;
  hueF += dt * cy * cy * 200.0f;
  if (hueF > 1.0e6f) hueF = 0;
  sf_buildPalette(order[pos], ((int)hueF) & 255, sf_k(13));
  if (sf_flash > 0) sf_flash--;

  // trails: long (knob right) = slow fade; the fade runs on real time
  static float fadeAcc = 0;
  float rate = 12.0f + (1.0f - trails / 1023.0f) * (1.0f - trails / 1023.0f) * 400.0f;   // levels per second
  fadeAcc += dt * rate;
  int dec = (int)fadeAcc;
  if (dec > 15) dec = 15;
  fadeAcc -= (int)fadeAcc;
  if (dec > 0) sf_fade(dec, drift);

  switch (preset) {
    case 1:  sf_sceneCascade(t); break;
    case 2:  sf_sceneSilk(t); break;
    case 3:  sf_sceneLasers(t); break;
    case 4:  sf_sceneAurora(t); break;
    case 5:  sf_sceneVortex(t); break;
    case 6:  sf_sceneGalaxy(t); break;
    case 7:  sf_sceneEverything(t); break;
    default: sf_sceneSpectrum(t); break;
  }
  sf_particles(steps);
  sf_sparkles(t);
  sf_mirror(mir);
}

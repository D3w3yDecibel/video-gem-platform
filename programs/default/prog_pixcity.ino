// =====================================================================
// PROGRAM: PIXEL CITY (by Dewey)
// Chunky 16-bit-style pixel-art cyberpunk scenes, all made from code:
// a rainy night skyline with flying cars, neon alleys with signs
// reflected in wet streets, a wall of glowing terminals, storms and
// apartment windows. Everything is generated — no pictures are stored —
// and every sign says a made-up word.
// Inspired by the look of vaporwave_1980's pixel-art animations (the
// scenes here are original).
//
// Presets:
//   k0 Balcony       — someone watching the city from a balcony
//   k1 Night Ride    — a motorbike tearing through the rain at night
//   k2 Terminal      — a wall of glowing screens in a dark room
//   k3 Skyway        — traffic streaming between the towers
//   k4 Rooftops      — gliding over the rooftops, blimp and searchlights
//   k5 Storm         — lashing rain and lightning
//   k6 Windows       — a big apartment block, lives in every window
//   k7 Neon Alley    — signs, cables and reflections in a wet street
//
//   k12 — press for a new city
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Palette (neon night [centre], matrix, sunset, ice, vapor)
//   p1 Speed (far left = freeze)   p2 Rain   p3 Pixel Size (1–3)
//   p4 Traffic   p5 Lights (lit windows)   p6 Flicker   p7 Haze
//   p8 Pan (how fast the view drifts)   p9 Lightning   p10 Signs
//   p11 Wheelie (Night Ride only: lifts the front wheel)
//
// All drawing is whole-number maths and filled spans (fast on the
// RP2040). The picture is drawn at a low "logical" resolution and each
// logical pixel becomes a P×P block — that's the chunky pixel-art look.
// =====================================================================

// ─── Palette layout ───────────────────────────────────────────────────
#define PC_SKY0     1      // 1–16 sky, top → horizon
#define PC_SKYN     16
#define PC_FAR      17     // far buildings: base, edge, dim window
#define PC_MID      20
#define PC_NEAR     23
#define PC_WARM     26
#define PC_WARM2    27
#define PC_COOL     28
#define PC_COOL2    29
#define PC_PINK     30
#define PC_WHITE    31
#define PC_NEON0    32     // 4 neon hues × 4 levels (32–47): dark glow … bright core
#define PC_RAIN     48
#define PC_RAINB    49
#define PC_SPLASH   50
#define PC_CAR      51
#define PC_HEAD     52
#define PC_TAIL     53
#define PC_TRAIL    54
#define PC_SIL      55     // silhouettes / foreground
#define PC_RAIL     56
#define PC_RIM      57
#define PC_FRAME    58
#define PC_SCREEN   59
#define PC_TEXT     60
#define PC_TEXTD    61
#define PC_TEXTA    62
#define PC_STREET   63
#define PC_STEAM    64
#define PC_CABLE    65
#define PC_BEACON   66
#define PC_BLIMP    67
#define PC_BEAM     68
#define PC_ROOM     69     // dark room behind a window
#define PC_TV       70
#define PC_CHROME   71     // bike chrome
#define PC_ENGINE   72     // bike engine / panels
#define PC_MOON     73
#define PC_MOON2    74     // moon halo / craters
#define PC_HOLO     75     // hologram billboard
#define PC_HOLO2    76
#define PC_BULB     77     // string lights
#define PC_BULB2    78
#define PC_EMBER    79     // cigarette ember

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float pc_sm[16];
static bool  pc_smReady = false;

static void pc_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!pc_smReady) pc_sm[i] = pots[i];
    else pc_sm[i] += (pots[i] - pc_sm[i]) * a;
  }
  pc_smReady = true;
}
static inline float pc_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (pc_sm[idx] / 1023.0f); }
static inline int pc_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (pc_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colours ──────────────────────────────────────────────────────────
// Per scheme: sky top, sky horizon, building tint, warm window, cool window, 4 neon hues
static const uint8_t pc_schemes[5][9][3] = {
  { {8, 6, 30},  {150, 50, 120}, {40, 40, 80},  {255, 190, 90},  {90, 220, 255}, {255, 40, 200}, {0, 230, 255},  {170, 80, 255}, {120, 255, 90} },  // neon night
  { {0, 8, 4},   {20, 90, 50},   {20, 50, 30},  {170, 255, 120}, {60, 255, 170}, {0, 255, 90},   {150, 255, 0},  {0, 200, 160},  {230, 255, 200} }, // matrix
  { {40, 10, 60}, {255, 120, 60}, {70, 35, 60}, {255, 210, 110}, {255, 150, 90}, {255, 60, 150}, {255, 180, 0},  {80, 220, 220}, {255, 90, 40} },   // sunset
  { {2, 10, 30},  {40, 110, 170}, {30, 55, 90}, {220, 240, 255}, {100, 200, 255}, {0, 200, 255},  {200, 240, 255}, {90, 120, 255}, {170, 90, 255} },  // ice
  { {40, 20, 70}, {255, 150, 200}, {70, 60, 110}, {255, 220, 180}, {120, 255, 230}, {255, 110, 220}, {80, 255, 230}, {190, 140, 255}, {255, 240, 150} } // vapor
};

static int pc_palKey = -1;

static inline uint8_t pc_mixc(int a, int b, int t256) { return (uint8_t)(a + ((b - a) * t256 >> 8)); }

static void pc_buildPalette(int scheme, int haze, int flash) {
  int key = (scheme * 256 + haze) * 4 + flash;
  if (key == pc_palKey) return;
  pc_palKey = key;
  const uint8_t (*S)[3] = pc_schemes[scheme];
  display.setColor(0, 0, 0, 0);
  for (int i = 0; i < PC_SKYN; i++) {                       // sky gradient
    int t = i * 256 / (PC_SKYN - 1);
    uint8_t r = pc_mixc(S[0][0], S[1][0], t * t >> 8), g = pc_mixc(S[0][1], S[1][1], t * t >> 8), b = pc_mixc(S[0][2], S[1][2], t * t >> 8);
    if (flash) { r = (uint8_t)((r + 3 * 230) >> 2); g = (uint8_t)((g + 3 * 230) >> 2); b = (uint8_t)((b + 3 * 255) >> 2); }
    display.setColor(PC_SKY0 + i, r, g, b);
  }
  // buildings: far ones fade toward the horizon colour (haze)
  const uint8_t* T = S[2];
  const uint8_t* Hz = S[1];
  int hz[3] = {haze / 2 + 110, haze / 3 + 40, 0};          // how much each layer fades (0–255)
  int dk[3] = {160, 100, 45};                              // darkness of each layer (0–255)
  for (int L = 0; L < 3; L++) {
    int base = PC_FAR + L * 3;
    uint8_t r = (uint8_t)(T[0] * dk[L] >> 8), g = (uint8_t)(T[1] * dk[L] >> 8), b = (uint8_t)(T[2] * dk[L] >> 8);
    r = pc_mixc(r, Hz[0] / 2, hz[L]); g = pc_mixc(g, Hz[1] / 2, hz[L]); b = pc_mixc(b, Hz[2] / 2, hz[L]);
    if (flash) { r = (uint8_t)((r + 150) >> 1); g = (uint8_t)((g + 150) >> 1); b = (uint8_t)((b + 180) >> 1); }
    display.setColor(base, r, g, b);                                               // wall
    display.setColor(base + 1, (uint8_t)(r * 3 / 2 + 10), (uint8_t)(g * 3 / 2 + 10), (uint8_t)(b * 3 / 2 + 14));  // lit edge
    display.setColor(base + 2, (uint8_t)(r * 5 / 4 + 6), (uint8_t)(g * 5 / 4 + 6), (uint8_t)(b * 5 / 4 + 10));   // unlit window
  }
  display.setColor(PC_WARM, S[3][0], S[3][1], S[3][2]);
  display.setColor(PC_WARM2, S[3][0] * 2 / 3, S[3][1] * 2 / 3, S[3][2] * 2 / 3);
  display.setColor(PC_COOL, S[4][0], S[4][1], S[4][2]);
  display.setColor(PC_COOL2, S[4][0] * 2 / 3, S[4][1] * 2 / 3, S[4][2] * 2 / 3);
  display.setColor(PC_PINK, S[5][0], S[5][1] * 3 / 4, S[5][2]);
  display.setColor(PC_WHITE, 240, 240, 230);
  for (int n = 0; n < 4; n++) {                           // neon: glow → core
    const uint8_t* c = S[5 + n];
    static const int lv[4] = {60, 120, 200, 256};
    for (int l = 0; l < 4; l++) {
      int w = (l == 3) ? 110 : 0;                          // the core is whiter
      display.setColor(PC_NEON0 + n * 4 + l, pc_mixc(c[0] * lv[l] >> 8, 255, w), pc_mixc(c[1] * lv[l] >> 8, 255, w), pc_mixc(c[2] * lv[l] >> 8, 255, w));
    }
  }
  display.setColor(PC_RAIN, Hz[0] / 3 + 60, Hz[1] / 3 + 70, Hz[2] / 3 + 90);
  display.setColor(PC_RAINB, 170, 190, 220);
  display.setColor(PC_SPLASH, 200, 210, 230);
  display.setColor(PC_CAR, 25, 25, 35);
  display.setColor(PC_HEAD, 255, 250, 220);
  display.setColor(PC_TAIL, 255, 40, 50);
  display.setColor(PC_TRAIL, 120, 20, 40);
  display.setColor(PC_SIL, 6, 5, 12);
  display.setColor(PC_RAIL, 22, 20, 34);
  display.setColor(PC_RIM, S[5][0] / 2 + 40, S[5][1] / 2 + 40, S[5][2] / 2 + 50);
  display.setColor(PC_FRAME, 50, 52, 62);
  display.setColor(PC_SCREEN, S[4][0] / 10 + 4, S[4][1] / 8 + 6, S[4][2] / 8 + 8);
  display.setColor(PC_TEXT, S[4][0] / 2 + 100, S[4][1] / 2 + 110, S[4][2] / 2 + 100);
  display.setColor(PC_TEXTD, S[4][0] / 3 + 20, S[4][1] / 3 + 30, S[4][2] / 3 + 30);
  display.setColor(PC_TEXTA, S[3][0], S[3][1] / 2 + 40, S[3][2] / 3);
  display.setColor(PC_STREET, T[0] / 6 + 4, T[1] / 6 + 4, T[2] / 5 + 8);
  display.setColor(PC_STEAM, 110, 110, 125);
  display.setColor(PC_CABLE, 14, 12, 20);
  display.setColor(PC_BEACON, 255, 30, 30);
  display.setColor(PC_BLIMP, 40, 38, 55);
  display.setColor(PC_BEAM, Hz[0] / 4 + 60, Hz[1] / 4 + 60, Hz[2] / 4 + 70);
  display.setColor(PC_ROOM, T[0] / 5 + 6, T[1] / 5 + 5, T[2] / 4 + 10);
  display.setColor(PC_TV, 120, 170, 255);
  for (int i = PC_EMBER + 1; i < 256; i++) display.setColor(i, 0, 0, 0);
  display.setColor(PC_MOON, 235, 232, 210);
  display.setColor(PC_MOON2, Hz[0] / 4 + 70, Hz[1] / 4 + 68, Hz[2] / 4 + 85);
  display.setColor(PC_HOLO, pc_mixc(S[4][0], 255, 90), pc_mixc(S[4][1], 255, 90), pc_mixc(S[4][2], 255, 90));
  display.setColor(PC_HOLO2, S[4][0] / 3 + 10, S[4][1] / 3 + 14, S[4][2] / 3 + 20);
  display.setColor(PC_BULB, 255, 200, 120);
  display.setColor(PC_BULB2, 120, 75, 40);
  display.setColor(PC_EMBER, 255, 120, 40);
  display.setColor(PC_CHROME, 175, 180, 200);
  display.setColor(PC_ENGINE, 58, 56, 80);
  display.setColor(255, 255, 255, 255);
}

// ─── Logical-pixel drawing (each logical pixel = P×P screen pixels) ───
static uint8_t* pc_buf;
static int pc_P = 2, pc_LW = 160, pc_LH = 120;

// Fill logical rectangle [x0, x1) × [y0, y1)
static void pc_fill(int x0, int y0, int x1, int y1, uint8_t c) {
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > pc_LW) x1 = pc_LW;
  if (y1 > pc_LH) y1 = pc_LH;
  if (x0 >= x1 || y0 >= y1) return;
  int px0 = x0 * pc_P, pw = (x1 - x0) * pc_P;
  if (px0 + pw > W) pw = W - px0;
  if (pw <= 0) return;
  for (int y = y0 * pc_P, ye = y1 * pc_P; y < ye && y < H; y++) memset(pc_buf + y * W + px0, c, pw);
}
static inline void pc_px(int x, int y, uint8_t c) {
  if ((unsigned)x >= (unsigned)pc_LW || (unsigned)y >= (unsigned)pc_LH) return;
  int px0 = x * pc_P, py0 = y * pc_P;
  for (int yy = 0; yy < pc_P && py0 + yy < H; yy++) {
    uint8_t* r = pc_buf + (py0 + yy) * W + px0;
    for (int xx = 0; xx < pc_P && px0 + xx < W; xx++) r[xx] = c;
  }
}
static inline uint8_t pc_get(int x, int y) {
  if ((unsigned)x >= (unsigned)pc_LW || (unsigned)y >= (unsigned)pc_LH) return 0;
  return pc_buf[(y * pc_P) * W + x * pc_P];
}
static void pc_line(int x0, int y0, int x1, int y1, uint8_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int g = 0; g < 400; g++) {
    pc_px(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// ─── Randomness (repeatable) ──────────────────────────────────────────
static uint16_t pc_seed = 1;
static inline uint32_t pc_hash(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ (uint32_t)c * 83492791u ^ (uint32_t)pc_seed * 2654435761u;
  h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
  return h;
}
static inline int pc_fdiv(int a, int b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }

// Small whole-number sine: 64 steps per turn, −64 … 64
static const int8_t pc_sinT[64] = {
  0, 6, 12, 19, 24, 30, 36, 41, 45, 49, 53, 56, 59, 61, 63, 64, 64, 64, 63, 61, 59, 56, 53, 49, 45, 41, 36, 30, 24, 19, 12, 6,
  0, -6, -12, -19, -24, -30, -36, -41, -45, -49, -53, -56, -59, -61, -63, -64, -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36, -30, -24, -19, -12, -6
};
static inline int pc_sin(int a) { return pc_sinT[a & 63]; }

// ─── Text: tiny 3×5 font and made-up words ────────────────────────────
static const uint16_t pc_font[38] = {
  0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A,
  0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B73, 0x6BAD, 0x388E, 0x7492,
  0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x7B6F, 0x2C97, 0x62A7, 0x628E,
  0x5BC9, 0x798E, 0x39EF, 0x7292, 0x7BEF, 0x7BCE, 0x0007, 0x0002,
};

static const char* const pc_onset[] = {
  "B", "BR", "CH", "D", "DR", "F", "FL", "G", "GR", "GL", "K", "KR", "L", "M", "N", "P", "PL",
  "QU", "R", "S", "SH", "SK", "SL", "ST", "T", "TH", "TR", "V", "VR", "W", "X", "Z", "ZH", "", ""
};
static const char* const pc_vowel[] = {
  "A", "E", "I", "O", "U", "AI", "OU", "EE", "OO", "YA", "AE", "IO", "Y", "UA"
};
static const char* const pc_coda[] = {
  "", "", "", "", "N", "R", "X", "SK", "LT", "M", "TH", "NK", "Z", "B", "RN", "SH"
};
#define PC_NONSET (sizeof(pc_onset) / sizeof(pc_onset[0]))
#define PC_NVOWEL (sizeof(pc_vowel) / sizeof(pc_vowel[0]))
#define PC_NCODA  (sizeof(pc_coda) / sizeof(pc_coda[0]))

// Make a word (2–4 syllables) from the number h; returns its length
static int pc_sylMin = 2, pc_sylSpan = 3;          // syllables: min … min + span − 1

static int pc_word(uint32_t h, char* out, int maxLen) {
  int n = 0;
  int syl = pc_sylMin + (int)(h % pc_sylSpan);
  uint32_t g = h;
  for (int s = 0; s < syl; s++) {
    g = g * 1103515245u + 12345u;
    const char* on = pc_onset[(g >> 8) % PC_NONSET];
    const char* vo = pc_vowel[(g >> 16) % PC_NVOWEL];
    const char* co = (s == syl - 1 || ((g >> 24) & 3) == 0) ? pc_coda[(g >> 26) % PC_NCODA] : "";
    for (const char* p = on; *p && n < maxLen; p++) out[n++] = *p;
    for (const char* p = vo; *p && n < maxLen; p++) out[n++] = *p;
    for (const char* p = co; *p && n < maxLen; p++) out[n++] = *p;
  }
  // now and then a number tag, like a version or a node id
  if (((h >> 29) & 3) == 0 && n + 3 <= maxLen) {
    out[n++] = ((h >> 5) & 1) ? '_' : '.';
    out[n++] = (char)('0' + (h >> 6) % 10);
    out[n++] = (char)('0' + (h >> 10) % 10);
  }
  return n;
}

static inline int pc_glyph(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '0' && c <= '9') return 26 + (c - '0');
  return c == '_' ? 36 : 37;
}


// Draw text with the 3×5 font; each font dot is `sc` logical pixels.
// Dots outside [xmin, xmax) are skipped (for scrolling text in a panel).
static void pc_textClip(int x, int y, const char* s, int n, int sc, uint8_t c, int xmin, int xmax) {
  for (int i = 0; i < n; i++) {
    uint16_t bits = pc_font[pc_glyph(s[i])];
    for (int r = 0; r < 5; r++)
      for (int k = 0; k < 3; k++) {
        int dx = x + (i * 4 + k) * sc;
        if (dx < xmin || dx + sc > xmax) continue;
        if (bits & (1 << (14 - r * 3 - k))) pc_fill(dx, y + r * sc, dx + sc, y + (r + 1) * sc, c);
      }
  }
}
static inline void pc_text(int x, int y, const char* s, int n, int sc, uint8_t c) {
  pc_textClip(x, y, s, n, sc, c, -10000, 10000);
}
// A random 3×5 symbol (for vertical signs)
static void pc_glyphRand(int x, int y, uint32_t h, uint8_t c) {
  uint16_t bits = (uint16_t)((h & 0x7FFF) | 0x2010);
  for (int r = 0; r < 5; r++)
    for (int k = 0; k < 3; k++)
      if (bits & (1 << (14 - r * 3 - k))) pc_px(x + k, y + r, c);
}

// ─── Scene pieces ─────────────────────────────────────────────────────
static int pc_t = 0;             // animation clock (ticks)
static int pc_lit = 60;          // % windows lit
static int pc_flick = 0;         // 0 … 255 flicker amount
static int pc_neonN = 3;         // how many signs

// Sky gradient from top to row `hy`, and below that the horizon colour
static void pc_sky(int hy) {
  for (int y = 0; y < pc_LH; y++) {
    int i = (y >= hy) ? PC_SKYN - 1 : y * (PC_SKYN - 1) / (hy > 0 ? hy : 1);
    pc_fill(0, y, pc_LW, y + 1, (uint8_t)(PC_SKY0 + i));
  }
}

// A row of buildings. layer 0 far … 2 near. `scroll` pans them sideways.
static void pc_city(int layer, int scroll, int baseY, int minH, int maxH, int slotW) {
  uint8_t cWall = (uint8_t)(PC_FAR + layer * 3), cEdge = cWall + 1, cDim = cWall + 2;
  int k0 = pc_fdiv(scroll, slotW) - 1;
  int sx = (layer == 0) ? 2 : 3, sy = (layer == 0) ? 2 : 3;
  for (int k = k0; k * slotW - scroll < pc_LW + slotW; k++) {
    uint32_t h = pc_hash(k, layer, 1);
    int bw = slotW * (55 + (int)(h % 45)) / 100;
    if (bw < 4) bw = 4;
    int bx = k * slotW - scroll + (int)((h >> 8) % (uint32_t)(slotW - bw + 1));
    int bh = minH + (int)((h >> 12) % (uint32_t)(maxH - minH + 1));
    int top = baseY - bh;
    pc_fill(bx, top, bx + bw, baseY, cWall);
    pc_fill(bx, top, bx + 1, baseY, cEdge);                  // lit edge
    int roof = (int)((h >> 20) & 3);
    if (roof == 1) {                                          // antenna with a blinking light
      int ah = 3 + (int)((h >> 22) % 6);
      pc_fill(bx + bw / 2, top - ah, bx + bw / 2 + 1, top, cWall);
      if (((pc_t >> 4) + k) % 3 == 0) pc_px(bx + bw / 2, top - ah, PC_BEACON);
    } else if (roof == 2) {                                   // stepped top
      int sw = bw / 2;
      pc_fill(bx + (bw - sw) / 2, top - 3, bx + (bw - sw) / 2 + sw, top, cWall);
    } else if (roof == 3 && bw > 6) {                         // spire
      for (int s = 0; s < 6 && bw / 2 - s > 0; s++) pc_fill(bx + s, top - 1 - s, bx + bw - s, top - s, cWall);
    }
    // windows
    int wy0 = top + 2, wx0 = bx + 2;
    for (int wy = wy0; wy < baseY - 1; wy += sy) {
      for (int wx = wx0; wx < bx + bw - 1; wx += sx) {
        uint32_t w = pc_hash(k * 131 + wx - bx, wy - top, layer + 7);
        bool on = (int)(w % 100) < pc_lit;
        if (pc_flick && ((w >> 8) & 255) < (uint32_t)(pc_flick >> 3)) on = ((pc_t >> 5) + (w >> 16)) & 1;
        if (on) {
          int kind = (int)((w >> 20) % 10);
          uint8_t c = kind < 5 ? (layer == 2 ? PC_WARM : PC_WARM2) : kind < 8 ? (layer == 2 ? PC_COOL : PC_COOL2) : PC_PINK;
          pc_px(wx, wy, c);
          if (layer == 2 && sx > 2) pc_px(wx + 1, wy, c);
        } else if (layer > 0) {
          pc_px(wx, wy, cDim);
        }
      }
    }
    // some near buildings carry a vertical neon strip
    if (layer >= 1 && pc_neonN > 0 && (int)((h >> 26) % 8) < pc_neonN && bw > 6) {
      int hue = (int)((h >> 29) & 3);
      bool off = pc_flick && ((pc_hash(k, pc_t >> 3, 9) & 255) < (uint32_t)(pc_flick >> 2));
      int sxp = bx + bw - 3, syp = top + 3, n = (bh - 8) / 6;
      if (n > 5) n = 5;
      for (int g = 0; g < n; g++) pc_glyphRand(sxp - 1, syp + g * 6, pc_hash(k, g, 3), (uint8_t)(PC_NEON0 + hue * 4 + (off ? 0 : 3)));
    }
  }
}

// Flying cars in a band of the sky
static void pc_cars(int count, int yTop, int yBot, int trails) {
  for (int i = 0; i < count; i++) {
    uint32_t h = pc_hash(i, 77, 5);
    int dir = (h & 1) ? 1 : -1;
    int spd = 3 + (int)((h >> 1) % 6);                      // logical px per 8 ticks
    int span = pc_LW + 40;
    int x = (pc_t * spd / 8 * dir + (int)((h >> 4) % (uint32_t)span)) % span;
    if (x < 0) x += span;
    x -= 20;
    int y = yTop + (int)((h >> 12) % (uint32_t)(yBot - yTop + 1)) + (pc_sin((pc_t >> 2) + i * 7) >> 5);
    int len = 3 + (int)((h >> 20) & 1);
    // light trail behind it
    if (trails) {
      int tl = spd * trails;
      for (int k = 1; k <= tl; k++) pc_px(x - dir * (len / 2 + k), y, k < tl / 2 ? PC_TAIL : PC_TRAIL);
    }
    pc_fill(x - len / 2, y, x - len / 2 + len, y + 1, PC_CAR);
    pc_px(x + dir * (len / 2), y, PC_HEAD);
    pc_px(x - dir * (len / 2), y, PC_TAIL);
  }
}

// Rain: `count` drops, `heavy` makes them longer
static void pc_rain(int count, int heavy, int groundY) {
  for (int i = 0; i < count; i++) {
    uint32_t h = pc_hash(i, 5, 11);
    int spd = 3 + (int)(h % 3) + heavy;
    int span = pc_LH + 12;
    int y = (int)((pc_t * spd + (int)((h >> 4) % (uint32_t)span)) % span) - 6;
    int x = (int)((h >> 12) % (uint32_t)(pc_LW + 20)) - 10 - y / 3;
    int len = 2 + (int)((h >> 24) & 1) + heavy;
    uint8_t c = ((h >> 26) & 3) == 0 ? PC_RAINB : PC_RAIN;
    for (int k = 0; k < len; k++) pc_px(x - (k + y) / 3 + y / 3, y + k, c);
    if (groundY > 0 && y + len >= groundY && y + len < groundY + spd) {   // splash
      pc_px(x - 1, groundY, PC_SPLASH); pc_px(x + 1, groundY, PC_SPLASH);
    }
  }
}

// A neon sign box with a word inside
static void pc_signWord(int x, int y, uint32_t h, int hue, int sc) {
  char w[16];
  pc_sylMin = 1; pc_sylSpan = 2;
  int n = pc_word(h, w, 7);
  bool off = pc_flick && ((pc_hash((int)h, pc_t >> 3, 13) & 255) < (uint32_t)(pc_flick >> 2));
  uint8_t core = (uint8_t)(PC_NEON0 + hue * 4 + (off ? 1 : 3)), glow = (uint8_t)(PC_NEON0 + hue * 4 + (off ? 0 : 1));
  int wpx = n * 4 * sc + 2 * sc, hpx = 5 * sc + 4 * sc;
  pc_fill(x - 1, y - 1, x + wpx + 1, y + hpx + 1, glow);      // glow
  pc_fill(x, y, x + wpx, y + hpx, PC_SIL);                    // board
  pc_fill(x, y, x + wpx, y + 1, core); pc_fill(x, y + hpx - 1, x + wpx, y + hpx, core);   // frame
  pc_fill(x, y, x + 1, y + hpx, core); pc_fill(x + wpx - 1, y, x + wpx, y + hpx, core);
  pc_text(x + 2 * sc - sc / 2, y + 2 * sc, w, n, sc, core);
}

// A person from behind, elbows on a railing (22×40 half-dots; the
// forearms sit on sprite rows 22–23).
static const char* const pc_person[40] = {
  ".........####.........",
  "........######........",
  ".......########.......",
  "......#########.#.....",
  ".......########.......",
  ".......########.......",
  "........######........",
  ".........####.........",
  ".........####.........",
  "......##########......",
  "....##############....",
  "...################...",
  "..###.##########.###..",
  "..##..##########..##..",
  ".###..##########..###.",
  ".##...##########...##.",
  ".##...##########...##.",
  "##....##########....##",
  "##....##########....##",
  "##....##########....##",
  "###...##########...###",
  ".####.##########.####.",
  "..########..########..",
  "...######....######...",
  "......##########......",
  "......##########......",
  "......##########......",
  "......##########......",
  ".......########.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  ".......###..###.......",
  "......####..####......"
};
#define PC_PERSON_W 22
#define PC_PERSON_H 40

// Draw a '#' sprite scaled by u256/256 screen px per sprite dot, with a
// rim light on the edges that face up and to the left (the city glow).
static void pc_blit(const char* const* spr, int sw, int sh, int x, int y, int u256, int headRows, int sway) {
  int dw = sw * u256 >> 8, dh = sh * u256 >> 8;
  for (int dy = 0; dy < dh; dy++) {
    int r = (dy << 8) / u256;
    const char* row = spr[r];
    int xo = x + (r < headRows ? sway : 0);
    for (int dx = 0; dx < dw; dx++) {
      int c = (dx << 8) / u256;
      if (row[c] != '#') continue;
      bool edge = (c == 0 || row[c - 1] != '#') || (r == 0 || spr[r - 1][c] != '#');
      pc_px(xo + dx, y + dy, edge ? PC_RIM : PC_SIL);
    }
  }
}

// A cat sitting on the rail, seen from behind, tail swishing
static const char* const pc_cat[9] = {
  "#...#", "##.##", "#####", "#####", ".###.", ".###.", "#####", "#####", "#####"
};

// A sagging cable between two points
static void pc_cable(int x0, int y0, int x1, int y1, int sag) {
  int n = abs(x1 - x0);
  if (n < 1) n = 1;
  int px = x0, py = y0;
  for (int i = 1; i <= n; i++) {
    int x = x0 + (x1 - x0) * i / n;
    int y = y0 + (y1 - y0) * i / n + sag * 4 * i * (n - i) / (n * n);
    pc_line(px, py, x, y, PC_CABLE);
    px = x; py = y;
  }
}

// ─── Scenes ───────────────────────────────────────────────────────────
static int pc_pan = 0;           // camera drift (logical px × 16)
static int pc_traffic = 6, pc_rainN = 60;

static void pc_sceneBalcony() {
  int LW = pc_LW, LH = pc_LH, hz = LH * 3 / 5;
  pc_sky(hz);
  int p = pc_pan >> 4;
  int u = LH * 128 / 40;                                    // sprite scale (screen px per half-dot × 256)
  if (u < 256) u = 256;
  int f = u >> 7;                                           // "whole unit" ≈ 2 half-dots

  // moon with a soft dithered halo and a few craters
  int mx = LW * 3 / 4 - p / 16, my = LH / 6, mr = LH / 14 + 2;
  for (int y = my - mr * 2; y <= my + mr * 2; y++)
    for (int x = mx - mr * 2; x <= mx + mr * 2; x++) {
      int dx = x - mx, dy = y - my, d2 = dx * dx + dy * dy;
      if (d2 <= mr * mr) pc_px(x, y, (d2 > (mr - 1) * (mr - 1) && dx < 0) ? PC_MOON2 : PC_MOON);
      else if (d2 <= mr * mr * 9 / 4 && ((x * 3 + y * 5) % 7) == 0) pc_px(x, y, PC_MOON2);
    }
  pc_px(mx + mr / 3, my - mr / 4, PC_MOON2); pc_px(mx - mr / 3, my + mr / 3, PC_MOON2);
  pc_px(mx + mr / 5, my + mr / 2, PC_MOON2);
  // clouds drifting past (see-through dither), lit along the top
  for (int i = 0; i < 5; i++) {
    uint32_t h = pc_hash(i, 61, 2);
    int cw = LW / 5 + (int)(h % (uint32_t)(LW / 4 + 1));
    int span = LW + cw * 2;
    int cx = (int)(((h >> 8) % (uint32_t)span + pc_t / (3 + (int)(h % 3)) + p / 8) % span) - cw;
    int cy = LH / 12 + (int)((h >> 16) % (uint32_t)(LH / 4));
    int ch = 2 + (int)((h >> 24) % 4);
    for (int r = 0; r < ch; r++) {
      int inset = (r == 0 || r == ch - 1) ? cw / 5 : 0;
      for (int x = cx + inset + (r & 1) * cw / 9; x < cx + cw - inset; x++) {
        if (r == 0 && (x & 1) == 0) { pc_px(x, cy, PC_MOON2); continue; }
        if (((x + r) & 1) == 0) pc_px(x, cy + r, PC_BLIMP);
      }
    }
  }

  pc_city(0, p / 4, hz + LH / 12, LH / 8, LH / 3, 9);
  pc_cars(pc_traffic / 2, LH / 6, hz - LH / 6, 0);

  // hologram billboard on its own tower (scanlines, a scrolling made-up
  // word, bouncing bars, the odd glitch)
  {
    int span = LW + LW / 2;
    int bw = LW / 4 + 4, bh = LH / 6 + 4;
    int bx = ((LW / 5 + LW / 4 - p / 3) % span + span) % span - LW / 4;
    int by = hz - LH / 3;
    pc_fill(bx + bw / 3, by + bh, bx + bw * 2 / 3, hz + LH / 5, PC_MID);          // tower
    pc_fill(bx + bw / 3, by + bh, bx + bw / 3 + 1, hz + LH / 5, PC_MID + 1);
    bool glitch = (pc_hash(pc_t >> 3, 5, 99) & 15) == 0;
    for (int y = by; y < by + bh; y++) {
      int sh = glitch && ((y >> 1) & 1) ? 2 : 0;
      if ((y & 1) == 0) pc_fill(bx + sh, y, bx + bw + sh, y + 1, PC_HOLO2);          // scanlines
    }
    pc_fill(bx, by, bx + bw, by + 1, PC_HOLO); pc_fill(bx, by + bh - 1, bx + bw, by + bh, PC_HOLO);
    char w[16];
    pc_sylMin = 2; pc_sylSpan = 2;
    int n = pc_word(pc_hash(pc_t / 400, 8, 8), w, 9);
    int tw = n * 4;
    int tx = bx + bw - ((pc_t / 2) % (bw + tw));
    pc_textClip(tx, by + 3, w, n, 1, PC_HOLO, bx + 1, bx + bw - 1);
    for (int k = 0; k < bw / 3 - 1; k++) {                                          // bars
      int bhh = 1 + ((pc_sin(pc_t + k * 9) + 64) * (bh - 12) >> 7);
      if (bhh > 0) pc_fill(bx + 2 + k * 3, by + bh - 2 - bhh, bx + 4 + k * 3, by + bh - 2, PC_HOLO);
    }
  }

  pc_city(1, p / 2, hz + LH / 5, LH / 5, LH / 2, 14);
  pc_cars(pc_traffic - pc_traffic / 2, LH / 4, hz, 1);

  // elevated train line: pylons, track, and every so often a train
  int ty = hz + LH / 8;
  for (int x = -(((p * 3 / 4) % 24) + 24) % 24; x < LW; x += 24) pc_fill(x, ty + 2, x + 2, LH, PC_RAIL);
  pc_fill(0, ty, LW, ty + 2, PC_RAIL);
  pc_fill(0, ty, LW, ty + 1, PC_FRAME);
  {
    int period = 700, tt = pc_t % period;
    int carW = LW / 5 + 6, nCars = 5, trainW = nCars * (carW + 2);
    int tx = LW + 10 - tt * 3;                              // runs right → left
    if (tx > -trainW - 10) {
      int th = LH / 24 + 3;
      for (int c = 0; c < nCars; c++) {
        int x0 = tx + c * (carW + 2);
        if (x0 > LW || x0 + carW < 0) continue;
        pc_fill(x0, ty - th, x0 + carW, ty, PC_CAR);
        pc_fill(x0, ty - th, x0 + carW, ty - th + 1, PC_FRAME);
        for (int wx = x0 + 2; wx < x0 + carW - 2; wx += 3) {                         // windows
          uint32_t hw = pc_hash(c, wx - x0, 17);
          pc_px(wx, ty - th + 2, (hw & 3) ? PC_WARM : PC_COOL);
          pc_px(wx + 1, ty - th + 2, (hw & 3) ? PC_WARM : PC_COOL);
        }
      }
      pc_px(tx, ty - 2, PC_HEAD); pc_px(tx - 1, ty - 2, PC_HEAD);                     // headlamp
      for (int k = 2; k < 14; k += 2) pc_px(tx - k, ty - 2, PC_BEAM);
      if ((pc_t & 7) < 2) pc_px(tx + trainW / 2, ty + 1, PC_WHITE);               // sparks off the rail
    }
  }

  pc_city(2, p, LH, LH / 8, LH / 4, 22);                   // (kept low, below the railing)
  pc_rain(pc_rainN, 0, 0);

  // ── the balcony ──
  int railY = LH * 3 / 4;
  int floorY = railY + 7 * f;
  int wallW = LW / 9 + 2;
  // the watcher, elbows on the rail (forearms at sprite row 22)
  int px = LW / 2 - (PC_PERSON_W * u >> 9);
  int py = railY - (22 * u >> 8);
  int sway = pc_sin(pc_t >> 3) >> 5;
  pc_blit(pc_person, PC_PERSON_W, PC_PERSON_H, px, py, u, 9, sway);
  // cigarette in the right hand: ember that brightens on a drag, smoke curling up
  int ex = px + (19 * u >> 8), ey = railY - 1;
  bool drag = (pc_t % 220) < 25;
  pc_px(ex, ey, PC_WHITE); pc_px(ex + 1, ey, drag ? PC_EMBER : PC_TAIL);
  for (int i = 0; i < 14; i++) {
    int life = (pc_t / 2 + i * 7) % 60;
    int sx = ex + 1 + life / 5 + (pc_sin(life * 3 + i * 11) * (1 + life / 12) >> 6);
    int sy = ey - 1 - life;
    if (sy > 0 && (life < 20 || ((life + i) & 1))) pc_px(sx, sy, life < 30 ? PC_STEAM : PC_MOON2);
  }
  // an exhaled puff drifting off after each drag
  int pt = pc_t % 220 - 35;
  if (pt >= 0 && pt < 60) {
    int cx = px + (15 * u >> 8) + pt / 3 + sway, cy = py + (4 * u >> 8) - pt / 4, rr = 1 + pt / 12;
    for (int y = cy - rr; y <= cy + rr; y++)
      for (int x = cx - rr; x <= cx + rr; x++)
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr * rr && ((x + y + pt) % (1 + pt / 15)) == 0) pc_px(x, y, PC_STEAM);
  }

  pc_fill(0, floorY, LW, LH, PC_SIL);                       // floor
  // puddle on the floor mirroring the neon beyond the rail, rippling
  int pl = LW / 4, pr = LW * 3 / 4;
  for (int y = floorY + 1; y < LH; y++) {
    int src = railY - 2 - (y - floorY) * 2;
    if (src < 0) break;
    if (((y + (pc_t >> 2)) % 3) == 0) continue;
    int sh = pc_sin(y * 9 + pc_t) >> 5;
    int inset = (y - floorY) * (pr - pl) / (4 * (LH - floorY + 1));
    for (int x = pl + inset; x < pr - inset; x++) {
      uint8_t c = pc_get(x + sh, src);
      if (c >= PC_WARM && c < PC_RAIN) pc_px(x, y, c >= PC_NEON0 ? (uint8_t)(((c - PC_NEON0) & ~3) + PC_NEON0 + 1) : c);
      else if (c == PC_MOON) pc_px(x, y, PC_MOON2);
    }
  }
  // railing
  pc_fill(0, railY, LW, railY + 2, PC_RAIL);                 // top bar
  pc_fill(0, railY, LW, railY + 1, PC_RIM);
  pc_fill(0, railY + 6 * f, LW, railY + 6 * f + 1, PC_RAIL); // bottom bar
  for (int x = 2; x < LW; x += 5) pc_fill(x, railY + 2, x + 1, railY + 6 * f, PC_RAIL);

  // the cat on the rail, tail swinging
  {
    int cu = u;
    int cx = LW * 3 / 4, cy = railY - (9 * cu >> 8);
    pc_blit(pc_cat, 5, 9, cx, cy, cu, 0, 0);
    int bx = cx + (5 * cu >> 8), by = railY - 1;
    int a = pc_sin(pc_t >> 1) >> 3;                          // −8 … 8
    int tl = 5 * cu >> 8;
    pc_line(bx, by, bx + tl / 2, by + tl / 2, PC_SIL);
    pc_line(bx + tl / 2, by + tl / 2, bx + tl / 2 + a * tl / 8, by + tl + (a < 0 ? -a : a) * tl / 16, PC_SIL);
  }

  // potted plant in the corner, leaves nodding in the wind
  {
    int ppx = LW - LW / 8, ppy = floorY;
    int pw = 6 * f / 2 + 3, ph = 5 * f / 2 + 3;
    for (int y = 0; y < ph; y++) pc_fill(ppx - pw / 2 + y / 3, ppy - ph + y, ppx + pw / 2 - y / 3, ppy - ph + y + 1, PC_RAIL);
    pc_fill(ppx - pw / 2, ppy - ph, ppx + pw / 2, ppy - ph + 1, PC_RIM);
    for (int k = 0; k < 7; k++) {
      int len = LH / 7 + (int)(pc_hash(k, 3, 3) % (uint32_t)(LH / 10 + 1));
      int dir = (k - 3) * 64 / 7;
      int wob = pc_sin((pc_t >> 2) + k * 9) >> 5;
      int lx = ppx + dir * len / 64 + wob, ly = ppy - ph - len + (dir < 0 ? -dir : dir) * len / 160;
      pc_line(ppx, ppy - ph, lx, ly, PC_SIL);
      pc_line(ppx + 1, ppy - ph, lx + 1, ly, PC_SIL);
      pc_px(lx, ly, PC_RIM);
    }
  }

  // wall on the left: vertical neon sign and an air-conditioner dripping
  pc_fill(0, 0, wallW, floorY, PC_NEAR);
  pc_fill(wallW - 1, 0, wallW, floorY, PC_NEAR + 1);
  for (int y = 6; y < floorY; y += 5) pc_fill(0, y, wallW - 1, y + 1, PC_NEAR + 2);  // brick courses
  if (pc_neonN > 0) {
    bool off = pc_flick && ((pc_hash(1, pc_t >> 3, 17) & 255) < (uint32_t)(pc_flick >> 2));
    int gx = wallW / 2 - 2;
    pc_fill(gx - 2, LH / 6 - 2, gx + 5, LH / 6 + 32, PC_NEON0 + 8 + (off ? 0 : 1));
    pc_fill(gx - 1, LH / 6 - 1, gx + 4, LH / 6 + 31, PC_SIL);
    for (int g = 0; g < 5; g++) pc_glyphRand(gx, LH / 6 + g * 6, pc_hash(7, g, 3), (uint8_t)(PC_NEON0 + 8 + (off ? 1 : 3)));
  }
  {
    int ax = wallW, ay = railY - LH / 5, aw = LW / 12 + 3, ah = LH / 12 + 2;
    pc_fill(ax, ay, ax + aw, ay + ah, PC_ENGINE);
    pc_fill(ax, ay, ax + aw, ay + 1, PC_FRAME);
    for (int y = ay + 2; y < ay + ah - 1; y += 2) pc_fill(ax + 1, y, ax + aw / 2, y + 1, PC_RAIL);   // grille
    int fr = ah / 2 - 1, fcx = ax + aw * 3 / 4, fcy = ay + ah / 2;
    if (fr > 1) for (int k = 0; k < 3; k++) {                                                  // spinning fan
      int a = pc_t * 3 + k * 21;
      pc_line(fcx, fcy, fcx + (pc_sin(a + 16) * fr >> 6), fcy + (pc_sin(a) * fr >> 6), PC_RAIL);
    }
    int dt = pc_t % 40;                                                                        // drip
    pc_px(ax + aw / 2, ay + ah + dt * dt / 40, PC_RAINB);
  }

  // ceiling overhang with string lights and drips off the edge
  int oh = LH / 14 + 1;
  pc_fill(0, 0, LW, oh, PC_SIL);
  pc_fill(0, oh - 1, LW, oh, PC_RIM);
  {
    int x0 = wallW, y0 = oh, x1 = LW, y1 = oh + 1, sag = LH / 7;
    pc_cable(x0, y0, x1, y1, sag);
    int n = x1 - x0;
    for (int x = x0 + 4; x < x1; x += 8) {
      int i = x - x0;
      int y = y0 + (y1 - y0) * i / n + sag * 4 * i * (n - i) / (n * n);
      uint32_t hb = pc_hash(x, 2, 29);
      bool on = ((pc_t >> 3) + (int)(hb % 13)) % 13 != 0;
      pc_px(x, y + 1, on ? PC_BULB : PC_BULB2);
      if (on) { pc_px(x, y + 2, PC_BULB2); if (f > 2) { pc_px(x - 1, y + 1, PC_BULB2); pc_px(x + 1, y + 1, PC_BULB2); } }
    }
  }
  for (int i = 0; i < 9; i++) {
    uint32_t h = pc_hash(i, 9, 31);
    int dx = wallW + (int)(h % (uint32_t)(LW - wallW));
    int per = 30 + (int)((h >> 8) % 30);
    int tt = (pc_t + (int)(h >> 16)) % per;
    int dy = oh + tt * tt / 12;
    if (dy < floorY) { pc_px(dx, dy, PC_RAINB); pc_px(dx, dy - 1, PC_RAIN); }
    else if (dy < floorY + 4) { pc_px(dx - 1, floorY, PC_SPLASH); pc_px(dx + 1, floorY, PC_SPLASH); }
  }
}

static void pc_sceneAlley() {
  int LW = pc_LW, LH = pc_LH, gy = LH * 3 / 4;
  pc_sky(gy / 2);
  int p = pc_pan >> 4;
  pc_city(1, p / 3, gy - LH / 10, LH / 5, LH / 2, 10);      // towers in the gap
  pc_cars(pc_traffic / 2, LH / 8, LH / 3, 1);
  // left wall with a big word sign, right wall with vertical signs
  int lw = LW * 3 / 10, rw = LW * 3 / 10;
  pc_fill(0, 0, lw, gy, PC_NEAR);
  pc_fill(lw - 1, 0, lw, gy, PC_NEAR + 1);
  for (int y = 4; y < gy - 2; y += 4)
    for (int x = 2; x < lw - 2; x += 4) {
      uint32_t w = pc_hash(x, y, 21);
      if ((int)(w % 100) < pc_lit / 2) pc_fill(x, y, x + 2, y + 2, (w >> 9) & 1 ? PC_WARM : PC_COOL2);
    }
  pc_fill(LW - rw, 0, LW, gy, PC_NEAR);
  pc_fill(LW - rw, 0, LW - rw + 1, gy, PC_NEAR + 1);
  int sc = LH >= 100 ? 2 : 1;
  if (pc_neonN > 0) pc_signWord(2, gy / 4, pc_hash(1, 1, 31), 0, sc);
  if (pc_neonN > 1) pc_signWord(4, gy / 2 + 4, pc_hash(2, 1, 31), 2, 1);
  for (int s = 0; s < pc_neonN && s < 4; s++) {             // vertical signs on the right
    int sx = LW - rw + 3 + s * 8, sy = 6 + s * 9, n = 5 + s;
    int hue = (s + 1) & 3;
    bool off = pc_flick && ((pc_hash(s, pc_t >> 3, 17) & 255) < (uint32_t)(pc_flick >> 2));
    pc_fill(sx - 1, sy - 1, sx + 6, sy + n * 6 + 1, (uint8_t)(PC_NEON0 + hue * 4));
    pc_fill(sx, sy, sx + 5, sy + n * 6, PC_SIL);
    for (int g = 0; g < n; g++) pc_glyphRand(sx + 1, sy + 1 + g * 6, pc_hash(s, g, 19), (uint8_t)(PC_NEON0 + hue * 4 + (off ? 1 : 3)));
  }
  // cables across the alley
  pc_cable(lw, gy / 6, LW - rw, gy / 5, 6);
  pc_cable(lw, gy / 3, LW - rw, gy / 4, 9);
  // wet street: a mirror of what's above, broken up by ripples
  pc_fill(0, gy, LW, LH, PC_STREET);
  for (int y = gy; y < LH; y++) {
    int src = gy - 1 - (y - gy) * 2;
    if (src < 0) break;
    int shift = pc_sin(y * 5 + (pc_t >> 1)) >> 5;
    if (((y + (pc_t >> 2)) & 3) == 0) continue;             // ripple gaps
    for (int x = 0; x < LW; x++) {
      uint8_t c = pc_get(x + shift, src);
      if (c >= PC_WARM && c < PC_RAIN) pc_px(x, y, c >= PC_NEON0 ? (uint8_t)(((c - PC_NEON0) & ~3) + PC_NEON0 + 1) : c);
    }
  }
  // steam from a grate
  for (int i = 0; i < 14; i++) {
    uint32_t h = pc_hash(i, 9, 41);
    int life = (pc_t + (int)(h % 64)) & 63;
    int x = LW / 2 + (int)((h >> 8) % 9) - 4 + (pc_sin(life + i * 5) >> 5);
    int y = LH - 2 - life / 2;
    if (life < 50) pc_px(x, y, PC_STEAM);
  }
  pc_rain(pc_rainN, 1, gy);
}


// ─── Night Ride: a motorbike in the rain ──────────────────────────────
// Sport bike and tucked-in rider, side-on, facing right, drawn in fine
// "half-dots" (two per bike unit s). K = dark body, D = dark engine/panels,
// M = chrome, V = visor/windscreen, N = neon stripe, n = dim neon,
// H = headlight, T = tail light, S = jacket seams.
// Wheels (centres at fine 12,22 and 50,22, radius 8) are drawn
// separately so their spokes can turn and the front can lift.
static const char* const pc_bike[24] = {
  "................................................................",
  "................................................................",
  "........................................KKKK....................",
  ".......................................SKKKKK...................",
  "......................................SKKKKKKK..................",
  ".....................................KKKKKVVVVV.................",
  "...................................K.KKKKKVVVVV.....V...........",
  "................................KKKKKKKKKVVVVVV...VVVV..........",
  ".............................KKKKKKKKKKKKKKKKK.VVVVVVVV.........",
  "..........................KKKSKKKKKKKKKKKKKKKKKKVVVVKKK.........",
  "....................K...KKKSSKKKKKKKKKKKKKKKNKKKKVKKKKKKK.......",
  "......KKKKKKKKKKKKKKKKKKKSSKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK.....",
  "......TTKKKKKKKKnnnnnSSSSSSKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKHKK....",
  "......TKKnnnnnnnKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKDDDDDKKKHHHH...",
  "........KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKDDDDDKKKKKKKHHHHKK..",
  "..................KKKKKKKKKKKKKKKKKKKKKKKKKKKKKMMKKKKKKKHHHHK...",
  "........................DDDDDDKKKKKKKDDDDDDKKKKNMMNNNNNNNNKKK...",
  "........................DDDDDDDDKKKKKKDDDDDKKKKKMMKKKKKKKKKK....",
  ".........DMMM............DDMMMMMKKKMMMMMMMDD..KKKMMKKKKKKKKK....",
  "..........DMMMMMMMM.....MMMDDDKKKKDDDDDDDDDD..K..MM.............",
  "..............MMMMMMMMMMDDDDDMMKMMMMMMMMDDDDD....MM.............",
  "...............MMMMMDDDDMMM.MMMMM.................MM............",
  "............MMM.DDDD....MMM.......................MM............",
  ".............DDD................................................"
};
#define PC_BIKE_W 64
#define PC_BIKE_H 24

static inline char pc_bikeAt(int c, int r) {
  if (c < 0 || r < 0 || c >= PC_BIKE_W || r >= PC_BIKE_H) return '.';
  return pc_bike[r][c];
}

static inline uint8_t pc_bikeCol(char ch) {
  switch (ch) {
    case 'K': return PC_SIL;
    case 'D': return PC_ENGINE;
    case 'M': return PC_CHROME;
    case 'V': return PC_COOL;
    case 'N': return (uint8_t)(PC_NEON0 + 4 + 3);
    case 'n': return (uint8_t)(PC_NEON0 + 4 + 1);
    case 'H': return PC_HEAD;
    case 'T': return PC_TAIL;
    default:  return PC_TRAIL;   // 'S' seams
  }
}

// Where a fine sprite point (fx, fy) lands on screen, for a bike whose
// rear tyre touches the road at (px, py), tilted up by the wheelie angle.
// ca/sa are cos/sin × 256, f2 = screen pixels per two fine dots (= s).
static inline void pc_bikePt(int fx, int fy, int px, int py, int ca, int sa, int f2, int* ox, int* oy) {
  int ux = fx - 12, uy = fy - 30;                         // relative to the rear contact point
  *ox = px + (((ca * ux + sa * uy) * f2) >> 9);
  *oy = py + (((-sa * ux + ca * uy) * f2) >> 9);
}

static void pc_drawWheel(int wx, int wy, int R, int s, int spin) {
  for (int a = 0; a < 64; a++) {
    int cx = pc_sin(a + 16), sy = pc_sin(a);
    pc_px(wx + ((cx * R) >> 6), wy + ((sy * R) >> 6), (a > 34 && a < 62) ? PC_RIM : PC_ENGINE);   // lit on top
    for (int d = 1; d <= s; d++)                                                                // chunky tyre
      pc_px(wx + ((cx * (R - d)) >> 6), wy + ((sy * (R - d)) >> 6), PC_SIL);
  }
  int Rr = R - s - 1;
  for (int a = 0; a < 64; a += 2)                                                               // chrome rim
    pc_px(wx + ((pc_sin(a + 16) * Rr) >> 6), wy + ((pc_sin(a) * Rr) >> 6), PC_CHROME);
  for (int k = 0; k < 5; k++) {                                                                 // 5 turning spokes
    int a = spin + k * 13;
    pc_line(wx, wy, wx + ((pc_sin(a + 16) * Rr) >> 6), wy + ((pc_sin(a) * Rr) >> 6), PC_CHROME);
  }
  int rd = R / 3;                                                                               // brake disc
  for (int a = 0; a < 64; a += 3)
    pc_px(wx + ((pc_sin(a + 16) * rd) >> 6), wy + ((pc_sin(a) * rd) >> 6), PC_CHROME);
}

// Draw the bike with the rear tyre on the road at (px, py). Bike unit = s
// screen pixels (a fine dot is s/2). ang = wheelie tilt in radians.
static void pc_drawBike(int px, int py, int s, int spin, float ang) {
  int ca = (int)(cosf(ang) * 256.0f), sa = (int)(sinf(ang) * 256.0f);
  // wheels first (the swingarm, fork and fender sit on top of them)
  int rx, ry, fx, fy;
  pc_bikePt(12, 22, px, py, ca, sa, s, &rx, &ry);
  pc_bikePt(50, 22, px, py, ca, sa, s, &fx, &fy);
  int R = 4 * s;
  pc_drawWheel(rx, ry, R, s, spin);
  pc_drawWheel(fx, fy, R, s, spin + (ang > 0.05f ? pc_t : 0));   // front free-spins in the air
  // body: walk the screen box and look each pixel up in the sprite
  // (inverse rotation, so no holes whatever the angle)
  int ext = 34 * s;                                        // reach of the sprite from the pivot
  int x0 = px - 12 * s, x1 = px + ext, y0 = py - ext + 2 * s, y1 = py + s;
  int sc = 512 / s;                                        // fine dots per screen pixel × 256
  for (int y = y0; y < y1; y++) {
    int dy = y - py;
    for (int x = x0; x < x1; x++) {
      int dx = x - px;
      int c = (((ca * dx - sa * dy) * sc) >> 16) + 12;     // back into sprite space
      int r = (((sa * dx + ca * dy) * sc) >> 16) + 30;
      char ch = pc_bikeAt(c, r);
      if (ch == '.') continue;
      uint8_t col = pc_bikeCol(ch);
      // neon rim light round the outline (the city glow catching the rider);
      // the underside stays dark
      if (ch == 'K' && (pc_bikeAt(c, r - 1) == '.' || pc_bikeAt(c + 1, r) == '.' || pc_bikeAt(c - 1, r) == '.')) col = PC_RIM;
      pc_px(x, y, col);
    }
  }
  // glowing hubs on top
  uint8_t hub = (uint8_t)(PC_NEON0 + 4 + 3);
  int hs = s / 2;
  pc_fill(rx - hs, ry - hs, rx + hs + 1, ry + hs + 1, hub);
  pc_fill(fx - hs, fy - hs, fx + hs + 1, fy + hs + 1, hub);
}

static void pc_sceneRide() {
  int LW = pc_LW, LH = pc_LH;
  static float rideF = 0;
  static int lastT = 0;
  int dtT = pc_t - lastT;
  if (dtT < 0 || dtT > 10) dtT = 1;
  lastT = pc_t;
  float v = 1.2f + pc_sm[8] / 1023.0f * 5.0f;             // p8 Pan = how fast you ride
  rideF += dtT * v;
  if (rideF > 1.0e7f) rideF = 0;
  int ride = (int)rideF;

  int s = LH / 60 > 0 ? LH / 60 : 1;                       // bike size
  int gy = LH * 4 / 5;                                        // where the tyres touch the road
  int hz = LH / 2;
  pc_sky(hz);
  pc_city(0, ride / 8, hz + LH / 12, LH / 8, LH / 3, 9);
  pc_city(1, ride / 3, gy - 10 * s, LH / 6, LH / 2, 15);

  // overhead gantries with a sign, every so often
  int gs = LW * 2 + 60;
  for (int k = ride / gs - 1; k * gs - ride < LW + 40; k++) {
    int gx = k * gs - ride + LW;
    if (gx < -60 || gx > LW + 60) continue;
    int top = LH / 8;
    pc_fill(gx, top, gx + 2, gy - 8 * s, PC_RAIL);
    pc_fill(gx + 44, top, gx + 46, gy - 8 * s, PC_RAIL);
    pc_fill(gx, top, gx + 46, top + 2, PC_RAIL);
    if (pc_neonN > 0) pc_signWord(gx + 6, top + 3, pc_hash(k, 4, 44), (k & 3), 1);
  }
  // street lights rushing past
  int ls = LW / 2 + 10;
  for (int k = ride / ls - 1; k * ls - ride < LW + 20; k++) {
    int lx = k * ls - ride;
    int top = gy - LH / 3;
    pc_fill(lx, top, lx + 1, gy - 8 * s, PC_RAIL);
    pc_fill(lx, top, lx + 7, top + 1, PC_RAIL);
    pc_fill(lx + 5, top + 1, lx + 8, top + 2, PC_WARM);
    for (int yy = top + 2; yy < gy - 8 * s; yy++) {           // glow cone (dithered)
      int half = (yy - top) / 3;
      for (int xx = lx + 6 - half; xx <= lx + 6 + half; xx++)
        if (((xx + yy * 3) & 3) == 0) pc_px(xx, yy, PC_BEAM);
    }
  }
  // road
  int curb = gy - 8 * s;
  pc_fill(0, curb, LW, curb + 1, PC_RIM);
  pc_fill(0, curb + 1, LW, LH, PC_STREET);
  // wet road: reflections of everything above, rippling
  for (int y = curb + 2; y < LH; y++) {
    int src = curb - 1 - (y - curb) * 2;
    if (src < 0) break;
    if (((y + (pc_t >> 1)) & 3) == 0) continue;
    int shift = pc_sin(y * 7 + pc_t) >> 5;
    for (int x = 0; x < LW; x++) {
      uint8_t c = pc_get(x + shift, src);
      if (c >= PC_WARM && c < PC_RAIN) pc_px(x, y, c >= PC_NEON0 ? (uint8_t)(((c - PC_NEON0) & ~3) + PC_NEON0 + 1) : c);
      else if (c == PC_BEAM && ((x + y) & 3) == 0) pc_px(x, y, PC_BEAM);
    }
  }
  // lane markings streaming past
  int ly = gy + (LH - gy) / 2;
  for (int x = -(ride % 24); x < LW; x += 24) pc_fill(x, ly, x + 10, ly + 1, PC_TEXTD);

  // cars in the far lane (the bike overtakes them)
  int nc = pc_traffic / 4;
  for (int i = 0; i < nc; i++) {
    uint32_t h = pc_hash(i, 12, 70);
    int span = LW + 120;
    int cx = LW + 60 - (int)((ride * 2 / 3 + (int)(h % (uint32_t)span)) % span);
    int cy = curb + 2;
    int cw = 16 * s, ch = 4 * s;
    pc_fill(cx, cy - ch, cx + cw, cy, PC_CAR);
    pc_fill(cx + 3 * s, cy - ch - 2 * s, cx + cw - 4 * s, cy - ch, PC_CAR);
    pc_fill(cx, cy - ch, cx + s, cy - ch + s, PC_TAIL);                  // tail light (facing away)
    pc_fill(cx + cw - s, cy - ch, cx + cw, cy - ch + s, PC_HEAD);
    for (int k = 1; k < 6; k++) pc_px(cx - k, cy - ch, k < 3 ? PC_TAIL : PC_TRAIL);
  }

  // the bike, bobbing on the bumps — p11 Wheelie lifts the front
  static float wAng = 0;
  float wTarget = pc_sm[11] / 1023.0f * 0.55f;               // up to ~32°
  if (wTarget > 0.08f) wTarget += (pc_sin(pc_t >> 1) * 0.04f) / 64.0f;   // balancing wobble
  wAng += (wTarget - wAng) * 0.25f;
  if (wAng < 0) wAng = 0;
  int ca = (int)(cosf(wAng) * 256.0f), sa = (int)(sinf(wAng) * 256.0f);
  int bump = (wAng < 0.05f && (pc_hash(pc_t >> 2, 0, 3) & 7) == 0) ? -1 : 0;
  int px = LW * 2 / 5 - 3 * s + (pc_sin(pc_t >> 3) >> 5);    // rear tyre contact
  int py = gy + bump;
  // headlight beam ahead (dithered), tilted up with the bike
  int hx, hyy;
  pc_bikePt(60, 14, px, py, ca, sa, s, &hx, &hyy);
  int tn = ca > 0 ? sa * 256 / ca : 0;                      // beam slope × 256
  for (int x = hx + 1; x < LW; x++) {
    int d = x - hx, half = d / 4;
    int cy = hyy - ((d * tn) >> 8);
    int ya = cy - half / 2, yb = cy + half;
    if (ya < 0) ya = 0;
    if (yb > gy) yb = gy;
    for (int y = ya; y <= yb; y++)
      if (((x + y + (pc_t >> 1)) & (d < 30 ? 1 : 3)) == 0) pc_px(x, y, d < 12 ? PC_HEAD : PC_BEAM);
  }
  if (wAng < 0.12f)                                          // streak on the wet road (gone when the wheel's up)
    for (int x = hx; x < LW; x += 2) pc_px(x, gy + (x - hx) / 16 + 1, PC_HEAD);
  // tail-light streak behind
  int tx, ty;
  pc_bikePt(6, 12, px, py, ca, sa, s, &tx, &ty);
  for (int k = 1; k < 18 * s; k++) pc_px(tx - k, ty + (k * sa) / 2048, k < 6 * s ? PC_TAIL : PC_TRAIL);
  pc_drawBike(px, py, s, -(ride / 2), wAng);
  // spray off the back wheel (a rooster tail when you're up on one)
  int nSpray = 24 + (int)(wAng * 90);
  for (int i = 0; i < nSpray; i++) {
    uint32_t h = pc_hash(i, 21, 13);
    int life = (pc_t * 2 + (int)(h % 32)) & 31;
    int sx = px - life * (1 + (int)(h % 3)) / 2;
    int lift = 12 + (int)(wAng * 14);
    int sy = gy - (life * (lift - life / 3)) / 16;
    if (sy <= gy) pc_px(sx, sy, (h >> 8) & 1 ? PC_SPLASH : PC_RAIN);
  }
  // rain, blown sideways by the speed
  for (int i = 0; i < pc_rainN + 40; i++) {
    uint32_t h = pc_hash(i, 5, 11);
    int spd = 4 + (int)(h % 3);
    int span = LH + 12;
    int y = (int)((pc_t * spd + (int)((h >> 4) % (uint32_t)span)) % span) - 6;
    int slant = 1 + (int)v;                                   // faster = more sideways
    int x = (int)((h >> 12) % (uint32_t)(LW + 40)) - (y * slant) / 3;
    x = ((x % (LW + 40)) + LW + 40) % (LW + 40) - 20;
    int bc = hyy - (((x - hx) * tn) >> 8);
    bool lit = x > hx && y > bc - (x - hx) / 8 && y < bc + (x - hx) / 4;
    uint8_t c = lit ? PC_WHITE : (((h >> 26) & 3) == 0 ? PC_RAINB : PC_RAIN);
    for (int k = 0; k < 3; k++) pc_px(x - (k * slant) / 3, y + k, c);
  }
}

static void pc_sceneTerminal() {
  int LW = pc_LW, LH = pc_LH;
  pc_fill(0, 0, LW, LH, PC_SIL);
  // hanging cables
  for (int i = 0; i < 7; i++) {
    uint32_t h = pc_hash(i, 3, 51);
    int x0 = (int)(h % (uint32_t)LW), x1 = (int)((h >> 10) % (uint32_t)LW);
    pc_cable(x0, -2, x1, -2, LH / 4 + (int)((h >> 20) % (uint32_t)(LH / 3)));
  }
  // monitors
  int cols = 4, rows = 3;
  int cw = LW / cols, ch = (LH * 2 / 3) / rows;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++) {
      uint32_t h = pc_hash(c, r, 61);
      int w = cw - 2 - (int)(h % 4), hh = ch - 2 - (int)((h >> 4) % 4);
      int x0 = c * cw + 1 + (int)((h >> 8) % 2), y0 = LH / 10 + r * ch + (int)((h >> 10) % 2);
      pc_fill(x0, y0, x0 + w, y0 + hh, PC_FRAME);
      int sx0 = x0 + 2, sy0 = y0 + 2, sw = w - 4, sh = hh - 4;
      if (sw < 3 || sh < 3) continue;
      pc_fill(sx0, sy0, sx0 + sw, sy0 + sh, PC_SCREEN);
      int kind = (int)((h >> 12) % 4);
      if (kind == 3) {                                       // waveform
        int py = 0;
        for (int x = 0; x < sw; x++) {
          int y = sh / 2 + ((pc_sin(x * 3 + (pc_t >> 1) + c * 9) * (sh / 2 - 1)) >> 6);
          if (x) pc_line(sx0 + x - 1, sy0 + py, sx0 + x, sy0 + y, PC_TEXT);
          py = y;
        }
      } else if (kind == 2) {                                // bar graph
        for (int x = 0; x < sw; x += 2) {
          int bh = (int)((pc_hash(x, (pc_t >> 3) + c, r) % (uint32_t)sh));
          pc_fill(sx0 + x, sy0 + sh - bh, sx0 + x + 1, sy0 + sh, (x & 4) ? PC_TEXTA : PC_TEXT);
        }
      } else {                                               // scrolling text lines
        int scroll = (pc_t >> (2 + kind)) + (int)(h >> 20);
        for (int ly = 0; ly < sh; ly += 2) {
          uint32_t lh = pc_hash(scroll + ly / 2, c * 7 + r, 71);
          int len = (int)(lh % (uint32_t)sw);
          int indent = (int)((lh >> 8) & 3);
          uint8_t col = ((lh >> 12) & 7) == 0 ? PC_TEXTA : (((lh >> 15) & 3) == 0 ? PC_TEXT : PC_TEXTD);
          pc_fill(sx0 + indent, sy0 + ly, sx0 + indent + len - indent, sy0 + ly + 1, col);
        }
        if ((pc_t >> 4) & 1) pc_px(sx0 + 1, sy0 + sh - 2, PC_TEXT);   // blinking cursor
      }
    }
  // desk and the person from behind
  int dy = LH * 5 / 6;
  pc_fill(0, dy, LW, LH, PC_RAIL);
  for (int x = LW / 3; x < LW * 2 / 3; x += 2) if (((x + (pc_t >> 2)) % 7) == 0) pc_px(x, dy + 2, PC_TEXTD);   // keyboard lights
  int f = LH / 60 > 0 ? LH / 60 : 1;
  int hx = LW / 2, hy = LH - 22 * f;
  pc_fill(hx - 12 * f, hy + 10 * f, hx + 12 * f, LH, PC_SIL);            // shoulders
  pc_fill(hx - 10 * f, hy + 8 * f, hx + 10 * f, hy + 10 * f, PC_SIL);
  for (int r = 0; r < 9 * f; r++) {                                    // head
    int half = (r < 2 * f) ? 3 * f + r : (r > 7 * f ? 5 * f - (r - 7 * f) : 5 * f);
    pc_fill(hx - half, hy + r, hx + half, hy + r + 1, PC_SIL);
  }
  for (int k = -4; k <= 4; k += 2) pc_px(hx + k * f, hy - 1, PC_SIL);    // messy hair
  pc_fill(hx - 3 * f, hy, hx + 3 * f, hy + 1, PC_RIM);                  // screen light on the head
  pc_fill(hx - 12 * f, hy + 10 * f, hx - 8 * f, hy + 10 * f + 1, PC_RIM);
  pc_fill(hx + 8 * f, hy + 10 * f, hx + 12 * f, hy + 10 * f + 1, PC_RIM);
}

static void pc_sceneSkyway() {
  int LH = pc_LH, hz = LH * 2 / 3;
  pc_sky(hz);
  int p = pc_pan >> 4;
  pc_city(0, p / 4, hz, LH / 6, LH / 2, 8);
  pc_cars(pc_traffic, LH / 8, hz - LH / 8, 2);
  pc_city(1, p / 2, LH - LH / 10, LH / 4, LH * 3 / 5, 13);
  pc_cars(pc_traffic, LH / 3, hz, 3);
  pc_city(2, p, LH, LH / 6, LH / 3, 20);
  pc_rain(pc_rainN / 2, 0, 0);
}

static void pc_searchlights(int n) {
  for (int i = 0; i < n; i++) {
    int bx = pc_LW * (i + 1) / (n + 1);
    int a = pc_sin((pc_t >> 2) + i * 20);
    int tx = bx + a * pc_LH / 64, ty = 0;
    for (int k = -1; k <= 1; k++) pc_line(bx, pc_LH, tx + k * 4, ty, PC_BEAM);
  }
}

static void pc_sceneRooftops() {
  int LW = pc_LW, LH = pc_LH, hz = LH / 2;
  pc_sky(hz);
  int p = (pc_pan >> 4) * 2 + pc_t / 2;                       // always gliding
  pc_searchlights(2);
  pc_city(0, p / 4, hz + LH / 10, LH / 8, LH / 3, 9);
  // blimp with a scrolling ad
  int bw = LW / 3, bh = LH / 9;
  int bxp = LW - ((pc_t / 3) % (LW + bw * 2)) + bw / 2, byp = LH / 6;
  for (int r = 0; r < bh; r++) {
    int half = bw / 2 - (abs(r - bh / 2) * abs(r - bh / 2) * bw / 2) / (bh * bh / 4 + 1);
    pc_fill(bxp - half, byp + r, bxp + half, byp + r + 1, PC_BLIMP);
  }
  pc_fill(bxp - bw / 3, byp + bh / 2 - 3, bxp + bw / 3, byp + bh / 2 + 3, PC_SIL);
  {
    char w[16];
    pc_sylMin = 2; pc_sylSpan = 2;
    int n = pc_word(pc_hash(5, 5, 5), w, 12);
    int tw = n * 4, off = (pc_t / 2) % (tw + bw);
    int tx = bxp - bw / 3 + bw * 2 / 3 - off;
    pc_textClip(tx, byp + bh / 2 - 2, w, n, 1, (uint8_t)(PC_NEON0 + 4 + 3), bxp - bw / 3, bxp + bw / 3);
  }
  pc_cars(pc_traffic / 2, LH / 4, hz, 1);
  pc_city(1, p / 2, LH - LH / 6, LH / 5, LH / 2, 14);
  pc_city(2, p, LH, LH / 10, LH / 4, 24);
  pc_rain(pc_rainN / 3, 0, 0);
}

static int pc_flashT = 0;        // lightning: frames of flash left

static void pc_sceneStorm(int lightning) {
  int LW = pc_LW, LH = pc_LH, hz = LH * 3 / 5;
  pc_sky(hz);
  int p = pc_pan >> 4;
  pc_city(0, p / 4, hz + LH / 12, LH / 6, LH / 2, 10);
  // lightning bolt while flashing
  if (pc_flashT > 0) {
    uint32_t h = pc_hash(pc_t >> 4, 1, 88);
    int x = (int)(h % (uint32_t)LW), y = 0;
    while (y < hz) {
      int nx = x + (int)((pc_hash(x, y, 89) % 7)) - 3, ny = y + 3 + (int)(pc_hash(y, x, 90) % 4);
      pc_line(x, y, nx, ny, PC_WHITE);
      if ((pc_hash(nx, ny, 91) & 7) == 0) pc_line(nx, ny, nx + 5, ny + 6, PC_RAINB);   // branch
      x = nx; y = ny;
    }
  }
  pc_city(1, p / 2, LH - LH / 8, LH / 5, LH / 2, 15);
  pc_city(2, p, LH, LH / 6, LH / 3, 24);
  pc_rain(pc_rainN * 2 + 80, 2, 0);
  (void)lightning;
}

static void pc_sceneWindows() {
  int LW = pc_LW, LH = pc_LH;
  pc_fill(0, 0, LW, LH, PC_NEAR);
  int ww = LW / 10, wh = LH / 7;
  if (ww < 5) ww = 5;
  if (wh < 6) wh = 6;
  int gx = ww + ww / 2, gy = wh + wh / 2;
  int ox = -((pc_pan >> 5) % gx), oy = -((pc_pan >> 6) % gy);
  int cx0 = (pc_pan >> 5) / gx, cy0 = (pc_pan >> 6) / gy;
  for (int r = -1; r * gy + oy < LH; r++)
    for (int c = -1; c * gx + ox < LW; c++) {
      int wc = c + cx0, wr = r + cy0;
      uint32_t h = pc_hash(wc, wr, 101);
      int x0 = c * gx + ox + ww / 4, y0 = r * gy + oy + wh / 4;
      pc_fill(x0 - 1, y0 - 1, x0 + ww + 1, y0 + wh + 1, PC_NEAR + 1);   // frame
      bool on = (int)(h % 100) < pc_lit;
      if (pc_flick && ((h >> 8) & 255) < (uint32_t)(pc_flick >> 3)) on = ((pc_t >> 6) + (h >> 16)) & 1;
      if (!on) { pc_fill(x0, y0, x0 + ww, y0 + wh, PC_NEAR + 2); continue; }
      int kind = (int)((h >> 20) % 4);
      uint8_t room = kind == 0 ? PC_WARM2 : kind == 1 ? PC_COOL2 : kind == 2 ? PC_ROOM : PC_WARM;
      pc_fill(x0, y0, x0 + ww, y0 + wh, room);
      if (kind == 2) {                                       // TV flicker lights the room
        if (((pc_t >> 2) + (int)h) % 5 < 2) pc_fill(x0, y0, x0 + ww, y0 + wh, PC_TV);
      }
      if (((h >> 24) & 3) == 0) {                            // someone at home, pacing
        int px = x0 + 1 + ((pc_sin((pc_t >> 3) + (int)(h & 63)) + 64) * (ww - 3) >> 7);
        pc_fill(px, y0 + wh / 3, px + 2, y0 + wh, PC_SIL);
        pc_fill(px, y0 + wh / 3 - 2, px + 2, y0 + wh / 3, PC_SIL);
      }
      if (((h >> 26) & 3) == 1) pc_fill(x0, y0, x0 + ww / 3, y0 + wh, PC_PINK);   // curtain
      pc_fill(x0, y0 + wh / 2, x0 + ww, y0 + wh / 2 + 1, PC_NEAR + 1);           // window bar
    }
  if (pc_neonN > 0) pc_signWord(LW / 8, LH / 2, pc_hash(9, 9, 9), 1, 1);
  pc_rain(pc_rainN, 1, 0);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_pixcity_name() { return "PIXEL CITY"; }
const char* prog_pixcity_character() { return "Pixel-art cyberpunk scenes: rain, neon, traffic and glowing screens"; }

static const char* const pc_presetNames[] = {
  "Balcony", "Night Ride", "Terminal", "Skyway", "Rooftops", "Storm", "Windows", "Neon Alley"
};
#define PC_NUM_PRESETS 8

const char* prog_pixcity_presetName(int preset) {
  if (preset >= 0 && preset < PC_NUM_PRESETS) return pc_presetNames[preset];
  return NULL;
}

static const char* const pc_labels[12] = {
  "Palette", "Speed", "Rain", "Pixel Size", "Traffic", "Lights", "Flicker", "Haze", "Pan", "Lightning", "Signs", "Wheelie"
};
const char* prog_pixcity_potLabel(int preset, int pot) {
  if (pot == 11) return preset == 1 ? pc_labels[11] : "";   // Wheelie is Night Ride only
  if (pot >= 0 && pot < 11) return pc_labels[pot];
  return "";
}

uint8_t prog_pixcity_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // every scene paints the whole screen
}

void prog_pixcity_init() {
  pc_palKey = -1;
}

void prog_pixcity_draw(int preset) {
  pc_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  pc_smoothKnobs(dt);

  static const uint8_t order[5] = {1, 2, 0, 3, 4};           // knob centre = neon night
  int scheme = order[pc_pot(0, 0, 4)];
  float sk = pc_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);
  pc_rainN = (int)(pc_sm[2] / 1023.0f * pc_sm[2] / 1023.0f * 260);
  pc_P = pc_pot(3, 1, 3);
  pc_LW = (W + pc_P - 1) / pc_P; pc_LH = (H + pc_P - 1) / pc_P;
  pc_traffic = pc_pot(4, 0, 16);
  pc_lit = pc_pot(5, 5, 95);
  pc_flick = (int)(pc_sm[6] / 1023.0f * pc_sm[6] / 1023.0f * 255);
  int haze = pc_pot(7, 0, 255);
  float pan = pc_potf(8, 0.0f, 1.0f);
  float lightning = pc_potf(9, 0.0f, 1.0f);
  pc_neonN = pc_pot(10, 0, 5);

  // clock
  static float tf = 0, panF = 0;
  tf += dt * 60.0f * speed;
  pc_t = (int)tf;
  panF += dt * 16.0f * (2.0f + 40.0f * pan * pan) * (speed > 0 ? 1.0f : 0.0f);
  if (panF > 1.0e7f) panF = 0;
  pc_pan = (int)panF;

  // k12: new city
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) pc_seed++;
  k12Was = k12;

  // lightning (always possible in Storm; elsewhere only if p9 is up)
  float lChance = (preset == 5) ? 0.02f + 0.08f * lightning : 0.06f * lightning * lightning;
  if (pc_flashT > 0) pc_flashT--;
  else if (speed > 0 && random(0, 10000) < (int)(lChance * 10000 * dt * 4)) pc_flashT = 3 + random(0, 4);
  pc_buildPalette(scheme, haze, (pc_flashT > 0 && (pc_flashT & 1)) ? 1 : 0);

  switch (preset) {
    case 1:  pc_sceneRide(); break;
    case 7:  pc_sceneAlley(); break;
    case 2:  pc_sceneTerminal(); break;
    case 3:  pc_sceneSkyway(); break;
    case 4:  pc_sceneRooftops(); break;
    case 5:  pc_sceneStorm(1); break;
    case 6:  pc_sceneWindows(); break;
    default: pc_sceneBalcony(); break;
  }
  // P = 3 leaves a thin strip at the right/bottom edge; keep it black
  if (pc_LW * pc_P > W || pc_LH * pc_P > H) {}  // (fills are clipped to the screen)
}

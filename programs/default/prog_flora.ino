// =====================================================================
// PROGRAM: FLORA — floral formations in a field of dots (by Dewey) — Set 3 — v2
// A grid of glowing dots over a dark aura. Every few seconds the dots
// swell into a flower, hold it, then sink back to a quiet grid — and the
// next bloom is a different formation. Neighbouring petals take opposite
// colours, bands of a third colour flow outward through the bloom, a
// heartbeat pulses through everything and rings ripple out across the
// resting grid, and the whole flower drifts slowly across the dots.
//
// Inspired by patternseeing's "Floral formations"
// (https://www.instagram.com/p/DeHtP0KoCbi/): cyan, green and blue dots
// on navy, swelling into a flower inside a soft round glow. The code here
// is an original take on that look.
//
// Flower types: ROSE (petals, rings, core), BOUQUET (a centre flower with
// buds orbiting it), GALAXY (spiral arms), WAVES (ripples from orbiting
// sources interfering), MANDALA (a flower inside a counter-turning flower).
//
// Presets:
//   k0 Formation  — rose on a square grid, cyan against magenta
//   k1 Sunflower  — golden spiral of seeds, amber against violet
//   k2 Lotus      — hex grid, pink against green, sharp petals
//   k3 Dahlia     — rings of dots, red against cyan, deep flowing ripples
//   k4 Bouquet    — a white flower with blue buds circling it
//   k5 Orchid     — three twisting violet petals, lime edges
//   k6 Galaxy     — spiral arms, blue against orange
//   k7 Pollen     — fine grid, green against magenta, sparkling and beating
//   k8 Mandala    — a flower inside a counter-turning flower
//   k9 Frost      — six sharp ice arms, red sparks
//   k10 Neon      — interfering ripples, magenta against cyan
//   k11 Garden    — every bloom a different type, grid and palette
//
//   k12 — NEXT: close this bloom and start the next formation now
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette       p1 Speed (far left = freeze the bloom where it is)
//   p2 Grid Spacing  p3 Dot Size      p4 Petals (centre = preset, else 3 … 16)
//   p5 Twist         p6 Bloom Size    p7 Spin (left slows, stops, reverses)
//   p8 Ripples       p9 Sharpness
//   p10 Grid Type (square / hex / spiral / rings; centre = preset)
//   p11 Glow (the aura)   p12 Flow (drift + flowing colour bands)
//   p13 Hold (time at full bloom)   p14 Pulse (heartbeat + rings)   p15 Sparkle
//
// Every knob at either end still shows a picture: the aura and the resting
// grid of dots never go away.
// =====================================================================

#include "vg_sin.h"
static inline float fl_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int fl_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static uint32_t fl_rng = 777u;
static inline uint32_t fl_rand() { fl_rng = fl_rng * 1664525u + 1013904223u; return fl_rng >> 8; }
static inline float fl_randf() { return fl_rand() * (1.0f / 16777216.0f); }
static inline float fl_smooth(float u) { u = fl_clampf(u, 0, 1); return u * u * (3 - 2 * u); }
static inline float fl_sinT(int a) { return vg_sin1024[a & 1023] * (1.0f / 16384.0f); }   // a in 1/1024 turns

// ─── Colours ──────────────────────────────────────────────────────────
// hue 0 aura, 1 resting dots, 2 petal A, 3 petal B (the contrast), 4 highlight, 5 core, 6 flowing band
#define FL_NPAL 11
#define FL_NHUE 7
static const uint8_t fl_pal[FL_NPAL][FL_NHUE][3] = {
  {{ 5,20,90},{40,90,200},{0,230,255},{255,40,200},{255,255,255},{255,230,40},{120,255,60}},    // 0 Formation: cyan / magenta
  {{40,15,0},{150,90,30},{255,170,0},{140,60,255},{255,255,220},{255,60,20},{0,200,255}},       // 1 Sunflower: amber / violet
  {{40,0,40},{150,70,160},{255,70,180},{40,255,120},{255,255,255},{255,210,60},{120,120,255}},  // 2 Lotus: pink / green
  {{40,0,5},{170,50,40},{255,30,40},{0,220,255},{255,240,200},{255,220,0},{255,140,0}},         // 3 Dahlia: red / cyan
  {{0,25,30},{90,160,150},{240,250,255},{40,90,255},{255,255,255},{255,200,0},{255,60,120}},    // 4 Bouquet: white / blue
  {{20,0,50},{120,70,200},{180,60,255},{170,255,0},{255,255,255},{255,40,160},{0,220,255}},     // 5 Orchid: violet / lime
  {{0,5,40},{60,80,200},{40,120,255},{255,130,0},{255,255,255},{255,240,180},{230,40,255}},     // 6 Galaxy: blue / orange
  {{0,30,0},{70,150,50},{60,255,40},{255,0,160},{255,255,180},{255,230,0},{0,200,255}},         // 7 Pollen: green / magenta
  {{25,0,60},{110,90,220},{0,220,255},{255,40,200},{255,255,255},{255,210,30},{255,90,30}},     // 8 Mandala: cyan / magenta / gold
  {{0,15,50},{110,160,230},{160,230,255},{255,30,70},{255,255,255},{240,250,255},{120,80,255}}, // 9 Frost: ice / red
  {{10,0,30},{100,40,160},{255,0,200},{0,255,240},{255,255,255},{255,255,0},{100,255,0}},       // 10 Neon: magenta / cyan
};
static int16_t fl_levK[16];
static void fl_buildPalette(int a, int b, float mix) {             // cross-fade palette a → b
  int m = (int)(fl_clampf(mix, 0, 1) * 256);
  for (int h = 0; h < 16; h++) {
    int r = 0, g = 0, bl = 0;
    if (h < FL_NHUE) {
      r = (fl_pal[a][h][0] * (256 - m) + fl_pal[b][h][0] * m) >> 8;
      g = (fl_pal[a][h][1] * (256 - m) + fl_pal[b][h][1] * m) >> 8;
      bl = (fl_pal[a][h][2] * (256 - m) + fl_pal[b][h][2] * m) >> 8;
    }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = fl_levK[l];
      display.setColor((l << 4) | h, r * k >> 8, g * k >> 8, bl * k >> 8);
    }
  }
}
#define FH_AURA 0
#define FH_REST 1
#define FH_A 2
#define FH_B 3
#define FH_HI 4
#define FH_CORE 5
#define FH_BAND 6

// ─── The dots ─────────────────────────────────────────────────────────
#define FL_MAX 900
#define FG_SQUARE 0
#define FG_HEX 1
#define FG_SPIRAL 2
#define FG_RINGS 3
static int16_t fl_x[FL_MAX], fl_y[FL_MAX];        // pixel position (the grid stays put; the flower drifts over it)
static int fl_n = 0, fl_gridType = -1, fl_spacing = -1;
static void fl_addDot(float x, float y) {
  if (fl_n >= FL_MAX) return;
  int ix = (int)lroundf(x), iy = (int)lroundf(y);
  if (ix < -8 || iy < -8 || ix > W + 8 || iy > H + 8) return;
  fl_x[fl_n] = (int16_t)ix; fl_y[fl_n] = (int16_t)iy;
  fl_n++;
}
static void fl_buildGrid(int type, int s) {
  const float cx = W / 2, cy = H / 2;
  fl_n = 0; fl_gridType = type; fl_spacing = s;
  if (type == FG_SQUARE) {
    int nx = W / s / 2 + 2, ny = H / s / 2 + 2;
    for (int j = -ny; j <= ny; j++) for (int i = -nx; i <= nx; i++) fl_addDot(cx + i * s, cy + j * s);
  } else if (type == FG_HEX) {
    float rh = s * 0.866f;
    int ny = (int)(H / rh / 2) + 2, nx = W / s / 2 + 2;
    for (int j = -ny; j <= ny; j++) for (int i = -nx; i <= nx; i++) fl_addDot(cx + i * s + ((j & 1) ? s * 0.5f : 0), cy + j * rh);
  } else if (type == FG_SPIRAL) {                                 // sunflower seeds: the golden angle
    float c = s * 0.56f;
    for (int i = 0; i < FL_MAX * 3 && fl_n < FL_MAX; i++) {
      float r = c * sqrtf((float)i);
      if (r > 205) break;
      float t = i * 2.3999632f;
      fl_addDot(cx + r * cosf(t), cy + r * sinf(t));
    }
  } else {                                                        // rings of dots
    fl_addDot(cx, cy);
    for (int j = 1; j * s < 205 && fl_n < FL_MAX; j++) {
      float r = j * s;
      int m = (int)(6.2831853f * r / s + 0.5f);
      float off = (j & 1) ? 0.5f : 0.0f;
      for (int i = 0; i < m; i++) { float t = (i + off) * 6.2831853f / m; fl_addDot(cx + r * cosf(t), cy + r * sinf(t)); }
    }
  }
}
// a fast angle, 0 … 1023 = one turn (good to well under a degree)
static inline int fl_atan(float dy, float dx) {
  float ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax < 1e-6f && ay < 1e-6f) return 0;
  float z = ax > ay ? ay / ax : ax / ay;                          // 0 … 1
  float a = z * (128.0f + 44.5f * (1.0f - z));                    // atan(z) in 1/1024 turns
  if (ay > ax) a = 256.0f - a;
  if (dx < 0) a = 512.0f - a;
  if (dy < 0) a = 1024.0f - a;
  return ((int)a) & 1023;
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* fl_buf;
static inline void fl_put(int x, int y, int lev, int hue) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || lev <= 0) return;
  if (lev > 15) lev = 15;
  uint8_t* p = fl_buf + y * W + x;
  if (lev > (*p >> 4)) *p = (uint8_t)((lev << 4) | hue);       // brightest wins
}
// a round dot of radius rad (px), soft one-pixel rim — integer maths inside (the RP2040 has no FPU)
static void fl_dot(int cx, int cy, float rad, int lev, int hue) {
  if (rad < 0.75f) { fl_put(cx, cy, (int)(lev * (0.4f + 0.8f * rad)), hue); return; }
  int R = (int)(rad + 0.999f);
  int r2 = (int)(rad * rad * 16.0f);                              // radii² in 1/16 px²
  float in = rad - 0.9f;
  int in2 = in > 0 ? (int)(in * in * 16.0f) : 0;
  int rim = lev - 5; if (rim < 2) rim = 2;
  for (int dy = -R; dy <= R; dy++) {
    int y = cy + dy;
    if ((unsigned)y >= (unsigned)H) continue;
    int dy2 = dy * dy * 16;
    for (int dx = -R; dx <= R; dx++) {
      int d2 = dx * dx * 16 + dy2;
      if (d2 > r2) continue;
      fl_put(cx + dx, y, d2 <= in2 ? lev : rim, hue);
    }
  }
}
// ─── Fixed-point helpers (the RP2040 has no FPU) ──────────────────────
static inline int32_t fl_clampQ(int32_t v) { return v < 0 ? 0 : (v > 4096 ? 4096 : v); }
static inline uint32_t fl_isqrt(uint32_t n) {                       // integer square root
  uint32_t r = 0, b = 1u << 30;
  while (b > n) b >>= 2;
  while (b) { if (n >= r + b) { n -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
  return r;
}
static inline int fl_atanI(int32_t dy, int32_t dx) {              // angle, 0 … 1023 = one turn
  int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax == 0 && ay == 0) return 0;
  int32_t z = ax > ay ? (ay << 12) / ax : (ax << 12) / ay;        // 0 … 4096
  int32_t t = 8192 + ((2848 * (4096 - z)) >> 12);                // (128 + 44.5 (1 − z)) × 64
  int a = (int)((z * t) >> 18);                                  // atan(z) in 1/1024 turns
  if (ay > ax) a = 256 - a;
  if (dx < 0) a = 512 - a;
  if (dy < 0) a = 1024 - a;
  return a & 1023;
}
// a round dot, radius in 1/16 px, soft one-pixel rim — all integer
static void fl_dotQ(int cx, int cy, int32_t rad16, int lev, int hue) {
  if (rad16 < 12) { fl_put(cx, cy, (lev * (6 + rad16)) >> 4, hue); return; }   // smaller than a pixel: one dimmer pixel
  int R = (rad16 + 15) >> 4;
  int32_t r2 = (rad16 * rad16) >> 4;                              // radius², in 1/16 px²
  int32_t in = rad16 - 14; int32_t in2 = in > 0 ? (in * in) >> 4 : 0;
  int rim = lev - 5; if (rim < 2) rim = 2;
  for (int dy = -R; dy <= R; dy++) {
    int y = cy + dy;
    if ((unsigned)y >= (unsigned)H) continue;
    int32_t dy2 = dy * dy * 16;
    for (int dx = -R; dx <= R; dx++) {
      int32_t d2 = dx * dx * 16 + dy2;
      if (d2 > r2) continue;
      fl_put(cx + dx, y, d2 <= in2 ? lev : rim, hue);
    }
  }
}
// the aura: a soft round glow, dithered so it doesn't band
static uint8_t fl_aura[1260];                     // quarter-levels by (distance² >> 7)
static uint16_t fl_auraD[1260];                   // the distance for each entry (px × 8), worked out once
static void fl_buildAura(float glowR, float strength) {
  static bool ready = false;
  if (!ready) { ready = true; for (int i = 0; i < 1260; i++) fl_auraD[i] = (uint16_t)(sqrtf((float)(i << 7)) * 8.0f); }
  int gR = (int)(glowR * 8.0f); if (gR < 8) gR = 8;
  int amp = (int)(strength * 30.0f * 16.0f);                      // ×16
  for (int i = 0; i < 1260; i++) {
    int t = (gR - fl_auraD[i]) * 256 / gR;                        // 256 at the centre … 0 at the rim
    int q = 4;
    if (t > 0) { int f = (t * t >> 8) * (768 - 2 * t) >> 8; q += (f * amp) >> 12; }   // smoothstep(t) × strength
    fl_aura[i] = (uint8_t)(q > 63 ? 63 : q);
  }
}
static void fl_drawAura(int cx, int cy) {
  static const uint8_t bayer[2][2] = {{0, 2}, {3, 1}};
  for (int y = 0; y < H; y++) {
    int dy = y - cy, dy2 = dy * dy;
    uint8_t* row = fl_buf + y * W;
    const uint8_t* b = bayer[y & 1];
    for (int x = 0; x < W; x++) {
      int dx = x - cx;
      int i = (dx * dx + dy2) >> 7;
      if (i > 1259) i = 1259;
      int lev = (fl_aura[i] + b[x & 1]) >> 2;
      row[x] = (uint8_t)(lev << 4);                              // hue 0 = aura
    }
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float fl_sm[16];
static bool fl_smReady = false;
static inline int fl_k(int i) { int v = (int)fl_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
static inline float fl_rel(int i, float own, float range, float lo, float hi) {   // centre = own, ×(1/range … range)
  float v = own * powf(range, (fl_k(i) - 512) / 512.0f);
  return fl_clampf(v, lo, hi);
}
static inline bool fl_centre(int i) { int k = fl_k(i); return k >= 440 && k < 600; }
static inline float fl_bi(int i) { return (fl_k(i) - 512) / 512.0f; }   // −1 … +1

// ─── Presets ──────────────────────────────────────────────────────────
const char* prog_flora_name() { return "FLORA"; }
const char* prog_flora_character() { return "Floral formations: a field of glowing dots swells into contrasting, pulsing, drifting flowers"; }
static const char* const fl_presetNames[] = {
  "Formation", "Sunflower", "Lotus", "Dahlia", "Bouquet", "Orchid",
  "Galaxy", "Pollen", "Mandala", "Frost", "Neon", "Garden"
};
#define FL_NUM_PRESETS 12
#define FM_ROSE 0
#define FM_BOUQUET 1
#define FM_GALAXY 2
#define FM_WAVES 3
#define FM_MANDALA 4
//                                                 For  Sun  Lot  Dah  Bou  Orc  Gal  Pol  Man  Fro  Neo  Gar
static const uint8_t fl_pMode[FL_NUM_PRESETS]  = {FM_ROSE, FM_ROSE, FM_ROSE, FM_ROSE, FM_BOUQUET, FM_ROSE, FM_GALAXY, FM_ROSE, FM_MANDALA, FM_ROSE, FM_WAVES, FM_ROSE};
static const uint8_t fl_pGrid[FL_NUM_PRESETS]  = {FG_SQUARE, FG_SPIRAL, FG_HEX, FG_RINGS, FG_HEX, FG_HEX, FG_SPIRAL, FG_SQUARE, FG_RINGS, FG_SQUARE, FG_SQUARE, FG_SQUARE};
static const uint8_t fl_pSpace[FL_NUM_PRESETS] = { 14,  12,  14,  13,  13,  15,  11,  10,  13,  12,  12,  14};   // px
static const uint8_t fl_pPal[FL_NUM_PRESETS]   = {  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0};
static const uint8_t fl_pPetA[FL_NUM_PRESETS]  = {  6,   8,   8,  12,   5,   3,   2,   6,   6,   6,   3,   3};   // petal choices for each bloom
static const uint8_t fl_pPetB[FL_NUM_PRESETS]  = { 10,  13,  10,  16,   8,   3,   4,   9,  12,   6,   4,  16};
static const int8_t  fl_pTwist[FL_NUM_PRESETS] = {  2,   0,   0,   4,   0,  30,  25,   4,  10,   0,   0,   4};   // ×0.1 turns across the bloom
static const uint8_t fl_pSharp[FL_NUM_PRESETS] = { 40,  30,  85,  50,  60,  40,  70,  30,  60, 100,  40,  50};   // %
static const uint8_t fl_pRipple[FL_NUM_PRESETS]= { 30,  15,  20,  70,  15,  30,  20,  35,  50,  15,  50,  30};   // %
static const int8_t  fl_pSpin[FL_NUM_PRESETS]  = {  4,   2,   3,  -3,   6,  -5,   5,   4,  -4,   1,   8,   5};   // ×0.01 turns/s
static const uint8_t fl_pDot[FL_NUM_PRESETS]   = { 70,  70,  65,  70,  68,  70,  70,  75,  65,  60,  72,  70};   // max radius, % of spacing
static const uint8_t fl_pSparkle[FL_NUM_PRESETS]={ 10,   5,  10,   5,  10,  15,  20,  45,  10,  30,  15,  15};   // %
static const uint8_t fl_pPulse[FL_NUM_PRESETS] = { 50,  35,  45,  60,  45,  40,  45,  80,  55,  35,  70,  55};   // %
static const uint8_t fl_pFlow[FL_NUM_PRESETS]  = { 50,  35,  45,  60,  50,  55,  60,  40,  40,  30,  70,  55};   // %
static const uint8_t fl_pBreath[FL_NUM_PRESETS]= { 50,  30,  60,  40,  40,  70,  50,  50,  50,  40,  30,  55};   // % petal wave around the flower

const char* prog_flora_presetName(int preset) {
  if (preset >= 0 && preset < FL_NUM_PRESETS) return fl_presetNames[preset];
  return NULL;
}
static const char* const fl_labels[16] = {
  "Palette", "Speed", "Grid Spacing", "Dot Size", "Petals", "Twist", "Bloom Size", "Spin",
  "Ripples", "Sharpness", "Grid Type", "Glow", "Flow", "Hold", "Pulse", "Sparkle"
};
const char* prog_flora_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return fl_labels[pot];
  return "";
}
uint8_t prog_flora_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we draw every pixel ourselves each frame
}

// ─── The bloom cycle ──────────────────────────────────────────────────
// grow → hold → shrink → rest; a new formation is chosen at the start of each grow
static float fl_cycT = 0;                         // seconds into this cycle (at Speed 1)
static int fl_petals = 6, fl_gridNow = 0, fl_palNow = 0, fl_palPrev = 0, fl_modeNow = 0;
static float fl_palMix = 1.0f, fl_twistSign = 1.0f, fl_spinSign = 1.0f;
static float fl_rot = 0, fl_layerRot = 0, fl_ripPh = 0, fl_flowT = 0, fl_beatT = 0, fl_breathPh = 0, fl_bandPh = 0;
static int fl_lastPreset = -1;
static void fl_newFormation(int preset, bool first) {
  int a = fl_pPetA[preset], b = fl_pPetB[preset];
  static const uint8_t gardenPet[] = {3, 5, 6, 7, 8, 12, 16};
  int mode = fl_pMode[preset];
  if (preset == 11) {
    fl_petals = gardenPet[fl_rand() % 7];
    if (!first) mode = (int)(fl_rand() % 5);
    if (mode == FM_GALAXY) fl_petals = 2 + (int)(fl_rand() % 3);
    if (mode == FM_WAVES) fl_petals = 3 + (int)(fl_rand() % 2);
  } else fl_petals = a + (int)(fl_rand() % (uint32_t)(b - a + 1));
  fl_twistSign = (fl_rand() & 1) ? 1.0f : -1.0f;
  fl_spinSign = (preset == 11 && (fl_rand() & 1)) ? -1.0f : 1.0f;
  int pal = fl_pPal[preset], grid = fl_pGrid[preset];
  if (preset == 11 && !first) { pal = (int)(fl_rand() % FL_NPAL); grid = (int)(fl_rand() % 4); }
  if (first) { fl_palPrev = pal; fl_palMix = 1.0f; }
  else if (pal != fl_palNow) { fl_palPrev = fl_palNow; fl_palMix = 0.0f; }
  fl_palNow = pal; fl_gridNow = grid; fl_modeNow = mode;
}

void prog_flora_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    fl_levK[0] = 0;
    for (int l = 1; l < 16; l++) fl_levK[l] = (int16_t)(256.0f * (0.05f + 0.95f * powf(l / 15.0f, 1.35f)));
  }
  fl_lastPreset = -1;
}

void prog_flora_draw(int preset) {
  fl_buf = display.getBuffer();
  if (!fl_buf) return;
  if (preset < 0 || preset >= FL_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f; if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!fl_smReady) fl_sm[i] = pots[i];
      else fl_sm[i] += (pots[i] - fl_sm[i]) * a;
    }
    fl_smReady = true;
  }

  // ── a new preset: start mid-bloom so there is a flower straight away ──
  if (preset != fl_lastPreset) {
    fl_lastPreset = preset;
    fl_rng = 777u + (uint32_t)preset * 131u + (uint32_t)now;
    fl_newFormation(preset, true);
    fl_cycT = 2.6f;                                              // just into the hold
    fl_rot = fl_randf(); fl_layerRot = 0; fl_ripPh = 0; fl_flowT = fl_randf() * 20.0f;
  }

  // ── knobs ──
  float sk = fl_sm[1] / 1023.0f;
  float speed = sk < 0.03f ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.85 … far right 3×
  float holdT = 0.4f + fl_k(13) * 5.0f / 1023.0f;                 // 0.4 … 5.4 s (centre ≈ 2.9)
  const float growT = 2.6f, shrinkT = 2.4f, restT = 0.8f;
  float cycleLen = growT + holdT + shrinkT + restT;

  bool k12 = keysPressed[KEY_MOD_A];
  static bool k12Was = false;
  if (k12 && !k12Was) {                                           // NEXT: head straight into the shrink
    if (fl_cycT < growT + holdT) {
      float e = fl_cycT < growT ? fl_smooth(fl_cycT / growT) : 1.0f;
      float u = 0; for (int i = 0; i < 24; i++) { if (1.0f - fl_smooth(u) > e) u += 1.0f / 24; }   // same bloom on the shrink: no jump
      fl_cycT = growT + holdT + u * shrinkT;
    }
  }
  k12Was = k12;

  fl_cycT += dt * speed;
  if (fl_cycT >= cycleLen) { fl_cycT -= cycleLen; fl_newFormation(preset, false); }
  float e;                                                        // bloom 0 … 1
  if (fl_cycT < growT) e = fl_smooth(fl_cycT / growT);
  else if (fl_cycT < growT + holdT) e = 1.0f;
  else if (fl_cycT < growT + holdT + shrinkT) e = 1.0f - fl_smooth((fl_cycT - growT - holdT) / shrinkT);
  else e = 0.0f;

  int pal = fl_palNow;
  if (!fl_centre(0)) pal = (fl_palNow + 1 + fl_k(0) * (FL_NPAL - 1) / 1024) % FL_NPAL;   // left or right of centre: other palettes
  if (fl_palMix < 1.0f) fl_palMix += dt / 1.2f;
  fl_buildPalette(fl_centre(0) ? fl_palPrev : pal, pal, fl_palMix);

  int gridType = fl_centre(10) ? fl_gridNow : fl_clamp(fl_k(10) < 440 ? fl_k(10) * 2 / 440 : 2 + (fl_k(10) - 600) * 2 / 424, 0, 3);
  int spacing = fl_centre(2) ? fl_pSpace[preset] : fl_clamp((int)fl_rel(2, fl_pSpace[preset], 2.0f, 10, 26), 10, 26);
  if (gridType != fl_gridType || spacing != fl_spacing) fl_buildGrid(gridType, spacing);

  int mode = fl_modeNow;
  float dotMax = spacing * fl_pDot[preset] / 100.0f * fl_rel(3, 1.0f, 1.7f, 0.5f, 1.6f);
  int petals = fl_centre(4) ? fl_petals : fl_clamp(3 + fl_k(4) * 14 / 1024, 3, 16);
  float twist = fl_pTwist[preset] * 0.1f * fl_twistSign + fl_bi(5) * 4.0f;   // turns across the bloom
  if (mode == FM_GALAXY && fabsf(twist) < 1.2f) twist = twist < 0 ? -1.2f : 1.2f;   // galaxies always wind
  float bloomR = fl_rel(6, 1.0f, 1.5f, 0.6f, 1.5f);               // × screen half-height
  float sp = fl_bi(7);                                            // right: up to 4× faster; left: slows, stops, reverses
  float spin = fl_pSpin[preset] * 0.01f * fl_spinSign * (sp >= 0 ? 1.0f + 3.0f * sp : 1.0f + 2.0f * sp);
  float ripple = fl_clampf(fl_pRipple[preset] / 100.0f + fl_bi(8) * 0.8f, 0.0f, 0.9f);
  float sharp = fl_clampf(fl_pSharp[preset] / 100.0f + fl_bi(9), 0.0f, 1.0f);
  float glow = 0.3f + fl_k(11) * 2.2f / 1023.0f;                  // 0.3 … 2.5, centre ≈ 1.4
  float flow = fl_clampf(fl_pFlow[preset] / 100.0f + fl_bi(12) * 0.7f, 0.0f, 1.0f);
  float pulse = fl_clampf(fl_pPulse[preset] / 100.0f + fl_bi(14) * 0.7f, 0.0f, 1.0f);
  float sparkle = fl_clampf(fl_pSparkle[preset] / 100.0f + fl_bi(15) * 0.6f, 0.0f, 0.8f);
  float breath = fl_pBreath[preset] / 100.0f;
  const float restR = 1.5f;

  // ── motion (everything keeps a gentle life even when Speed is frozen, except the bloom itself) ──
  float live = speed > 0 ? 1.0f : 0.25f;
  fl_rot += spin * dt * live;
  fl_layerRot -= spin * 1.6f * dt * live;
  fl_ripPh += dt * (1.0f + speed) * 2.0f;
  fl_flowT += dt * live * (0.3f + flow);
  fl_breathPh += dt * live * 1.4f;
  fl_bandPh += dt * live * (0.8f + 2.5f * flow);
  float bpm = 52.0f + 40.0f * pulse;                              // the heartbeat
  fl_beatT += dt * bpm / 60.0f;
  if (fl_rot > 1) fl_rot -= 1; if (fl_rot < 0) fl_rot += 1;
  if (fl_layerRot > 1) fl_layerRot -= 1; if (fl_layerRot < 0) fl_layerRot += 1;
  if (fl_ripPh > 62.831853f) fl_ripPh -= 62.831853f;
  if (fl_flowT > 1000.0f) fl_flowT -= 1000.0f;
  if (fl_breathPh > 6.2831853f) fl_breathPh -= 6.2831853f;
  if (fl_bandPh > 6.2831853f) fl_bandPh -= 6.2831853f;
  if (fl_beatT > 1.0f) fl_beatT -= (int)fl_beatT;
  // lub-dub: a strong beat, then a softer one, each a quick swell and a slow fade
  float bt = fl_beatT;
  float beat = (bt < 0.30f ? expf(-bt * 14.0f) * fl_smooth(bt * 25.0f) : 0.0f)
             + 0.6f * (bt > 0.22f && bt < 0.6f ? expf(-(bt - 0.22f) * 14.0f) * fl_smooth((bt - 0.22f) * 25.0f) : 0.0f);
  beat *= pulse;

  // the flower drifts across the grid in a slow figure of eight
  float cxF = W * 0.5f + flow * 34.0f * sinf(fl_flowT * 0.23f);
  float cyF = H * 0.5f + flow * 20.0f * sinf(fl_flowT * 0.37f + 1.3f);
  int cx = (int)cxF, cy = (int)cyF;

  // ── the aura (it beats too) ──
  float auraR = (85.0f + 95.0f * e + 18.0f * beat) * bloomR;
  fl_buildAura(auraR, glow * (0.5f + 0.45f * e + 0.35f * beat));
  fl_drawAura(cx, cy);

  // ── the dots ── (fixed point: the RP2040 has no floating-point hardware, so the per-dot
  //    maths runs in integers — Q12 (4096 = 1.0), Q14 sines, 1/16-pixel positions)
  const float Rs = 120.0f;                                        // screen half-height, the unit for the flower
  float B = 0.95f * bloomR * e * (1.0f + 0.06f * beat);           // flower radius (in Rs units)
  int rotI = (int)(fl_rot * 1024.0f);
  int layRotI = (int)(fl_layerRot * 1024.0f);
  int32_t twistQ = (int32_t)(twist * 1024.0f);
  int breathI = (int)(fl_breathPh * 162.97f);
  int ripI = (int)(fl_ripPh * 162.97f), ripI2 = (int)(fl_ripPh * 1.2f * 162.97f);
  int bandI = (int)(fl_bandPh * 162.97f), beatI = (int)(fl_beatT * 1024.0f);
  uint32_t slot = (uint32_t)(now / 700);                          // sparkle: each dot may twinkle once per slot
  float tw = fl_sinT((int)((now % 700) * 512 / 700));             // 0 → 1 → 0 across the slot
  const int32_t Q = 4096;
  int32_t BQ = (int32_t)(B * Q), eQ = (int32_t)(e * Q);
  int32_t sharpQ = (int32_t)(sharp * Q), breathQ = (int32_t)(breath * 0.22f * Q), ripQ = (int32_t)(ripple * Q);
  int32_t baseQ = mode == FM_GALAXY ? 614 : 1311;
  int32_t coreK = (int32_t)(Q / (0.2f * bloomR + 0.02f));         // 1 / core radius (Q12)
  int32_t moundA = (int32_t)(0.22f * (1.0f - e) * Q), moundK = (int32_t)(Q / (0.75f * bloomR));
  int32_t coreRQ = (int32_t)(0.17f * bloomR * e * Q);
  int32_t pulseQ = (int32_t)(pulse * Q);
  int32_t bandThr = (int32_t)((1.0f - 0.45f * flow) * 16384.0f);
  bool bandsOn = flow > 0.05f;
  int32_t restQ = (int32_t)(restR * 16), dmaxQ = (int32_t)(dotMax * 16);
  int32_t beatMul = (int32_t)((1.0f + 0.22f * beat) * Q);
  int beatLev = (int)(beat * 2.0f);
  int32_t sparkThr = (int32_t)(sparkle * 60.0f), twLev = 8 + (int)(tw * 7.0f), twRad = (int32_t)(tw * 0.8f * 16);
  int32_t cxQ = (int32_t)(cxF * 16), cyQ = (int32_t)(cyF * 16);   // flower centre, 1/16 px
  const int32_t rnK = 2185;                                       // 1/16 px → Q12 of Rs: 4096 / (120 × 16) × 1024
  // bouquet buds and wave sources: orbiting points
  int32_t budX[6], budY[6]; int nBud = 0;
  if (mode == FM_BOUQUET || mode == FM_WAVES) {
    nBud = mode == FM_BOUQUET ? (petals > 6 ? 6 : (petals < 3 ? 3 : petals)) : (petals > 4 ? 4 : (petals < 2 ? 2 : petals));
    float orbit = (mode == FM_BOUQUET ? 0.62f : 0.32f) * B * Rs;
    for (int j = 0; j < nBud; j++) {
      int a = rotI + j * 1024 / nBud;
      budX[j] = (int32_t)((cxF + orbit * fl_sinT(a + 256)) * 16);
      budY[j] = (int32_t)((cyF + orbit * fl_sinT(a)) * 16);
    }
  }
  int32_t budRQ = (int32_t)(0.30f * B * Rs * 16);                 // bud radius, 1/16 px
  int32_t inBQ = (int32_t)(B * 1.05f * Q);

  for (int i = 0; i < fl_n; i++) {
    int32_t px = fl_x[i] * 16, py = fl_y[i] * 16;
    int32_t dx = px - cxQ, dy = py - cyQ;
    int32_t rn = (int32_t)((fl_isqrt((uint32_t)(dx * dx + dy * dy)) * rnK) >> 10);   // distance in Rs units, Q12
    int32_t v = 0, shape = 0;
    int side = 0;                                                 // which of the two contrasting colours
    if (BQ > 40) {
      int ang = fl_atanI(dy, dx);
      if (mode == FM_WAVES) {
        // ripples spreading from orbiting sources: where they agree the dots swell
        int32_t s = 0, bestV = -20000; int best = 0;
        for (int j = 0; j < nBud; j++) {
          int32_t ex = px - budX[j], ey = py - budY[j];
          int32_t dn = (int32_t)((fl_isqrt((uint32_t)(ex * ex + ey * ey)) * rnK) >> 10);
          int32_t w = vg_sin1024[(((dn * 265) >> 10) - ripI2) & 1023];
          s += w; if (w > bestV) { bestV = w; best = j; }
        }
        s /= nBud;
        int32_t inside = fl_clampQ(((inBQ - rn) * 25 >> 1) / 3 * 2 + 2048);   // ÷ 0.12
        int32_t w01 = 2048 + (s >> 3);
        int32_t w2 = (w01 * w01) >> 12;
        v = (inside * (737 + ((3891 * w2) >> 12))) >> 12;
        if (v > Q) v = Q;
        shape = w01;
        side = best & 1;
      } else {
        int arg = ((petals * (ang + rotI)) >> 1) + (int)((twistQ * rn) >> 12);
        int32_t c = vg_sin1024[arg & 1023] >> 2; if (c < 0) c = -c;   // |sin(k θ / 2)| has k lobes (Q12)
        int lobe = arg >> 9;                                      // which petal this dot is in
        side = lobe & 1;
        int32_t c2 = (c * c) >> 12, c4 = (c2 * c2) >> 12;
        shape = c + (((c4 - c) * sharpQ) >> 12);
        // the petals breathe in a wave that runs round the flower
        int32_t br = Q + ((breathQ * vg_sin1024[(lobe * 2048 / (petals > 0 ? petals : 1) + breathI) & 1023]) >> 14);
        int32_t R = (BQ * (baseQ + (((Q - baseQ) * shape) >> 12))) >> 12;
        R = (R * br) >> 12;
        int32_t inside = fl_clampQ((R - rn) * 10 + 2048);         // ÷ 0.10
        int32_t ring = 2048 + (vg_sin1024[(((rn * 367) >> 10) - ripI) & 1023] >> 3);
        v = (inside * (Q - ripQ + ((ripQ * ring) >> 12))) >> 12;
        if (mode == FM_GALAXY) v = (v * (1434 + ((2662 * shape) >> 12))) >> 12;   // bright arms, dim lanes between
        if (mode == FM_MANDALA) {                                 // an inner flower, twice the petals, turning the other way
          int arg2 = petals * (ang + layRotI) - (int)((twistQ * rn) >> 12);
          int32_t cc = vg_sin1024[arg2 & 1023] >> 2; if (cc < 0) cc = -cc;
          int32_t R2 = (((BQ * 2130) >> 12) * (1638 + ((2458 * cc) >> 12))) >> 12;
          int32_t in2 = fl_clampQ(((R2 - rn) * 25 >> 1) + 2048);  // ÷ 0.08
          if (in2 > v) { v = in2; side = ((arg2 >> 9) & 1) ^ 1; }
        }
        if (mode == FM_BOUQUET) {                                 // buds circling the centre flower
          v = (v * 3) >> 2;
          for (int j = 0; j < nBud; j++) {
            int32_t ex = px - budX[j], ey = py - budY[j];
            if (ex > budRQ + 64 || ex < -budRQ - 64 || ey > budRQ + 64 || ey < -budRQ - 64) continue;
            int32_t d = (int32_t)fl_isqrt((uint32_t)(ex * ex + ey * ey));
            int ba = fl_atanI(ey, ex);
            int32_t cb = vg_sin1024[((5 * (ba - rotI * 2)) >> 1) & 1023] >> 2; if (cb < 0) cb = -cb;
            int32_t Rb = (budRQ * (2048 + (cb >> 1))) >> 12;
            Rb = (Rb * (Q + ((819 * vg_sin1024[(breathI + j * 170) & 1023]) >> 14))) >> 12;
            int32_t inb = fl_clampQ((Rb - d) * 64 + 2048);        // ÷ 4 px
            if (inb > v) { v = inb; side = 1; shape = cb; }
          }
        }
      }
      int32_t core = (eQ * fl_clampQ(Q - ((rn * coreK) >> 12))) >> 12;
      if (core > v) v = core;
    }
    // pulse rings run outward through everything, flower and resting grid alike
    int32_t pr = vg_sin1024[(((rn * 204) >> 10) - beatI) & 1023] >> 2;   // Q12
    int32_t ringPulse = pr > 2458 ? ((((pr - 2458) * 5) >> 1) * pulseQ) >> 12 : 0;
    int32_t mound = (moundA * fl_clampQ(Q - ((rn * moundK) >> 12))) >> 12;   // between blooms the centre stays raised
    int32_t vv = v > mound ? v : mound;
    int32_t rad = restQ + (((dmaxQ - restQ) * vv) >> 12);          // 1/16 px
    rad = (rad * (beatMul + (ringPulse >> 2))) >> 12;
    int lev = 6 + (int)((v * 9 + 2048) >> 12) + beatLev;
    int hue;
    if (v < 164) {                                                // the resting grid
      hue = FH_REST; lev = 10 + (int)((mound * 16) >> 12) + (int)((ringPulse * 6) >> 12);
      if (ringPulse > 1843) hue = FH_BAND;                        // the pulse ring tints the grid as it passes
    }
    else if (rn < coreRQ) hue = FH_CORE;
    else if (v > 3686 && shape > 3686) hue = FH_HI;
    else {
      // flowing bands of the third colour run outward; otherwise petals alternate the two contrasting colours
      int32_t band = vg_sin1024[(((rn * 285) >> 10) - bandI) & 1023];
      if (bandsOn && band > bandThr) hue = FH_BAND;
      else hue = side ? FH_B : FH_A;
    }
    if (sparkThr > 0) {
      uint32_t h = ((uint32_t)i * 2654435761u) ^ (slot * 40503u);
      h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
      if ((int32_t)(h & 1023) < sparkThr) {                       // this dot twinkles in this slot
        if (twLev > lev) { lev = twLev; hue = FH_HI; }
        rad += twRad;
      }
    }
    fl_dotQ(fl_x[i], fl_y[i], rad, lev, hue);
  }
}

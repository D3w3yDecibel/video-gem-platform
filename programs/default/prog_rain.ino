// =====================================================================
// PROGRAM: DIGITAL RAIN (by Dewey)
// Falling-code rain in the style of the sci-fi movies — eight different
// takes on it, one per preset.
//
// Presets:
//   k0  Classic     — columns of glyphs pour down, white heads, flickering code
//   k1  Depth       — three layers (far, mid, near) for a 3D parallax feel
//   k2  Glitch      — the rain, but the signal is breaking up
//   k3  Reveal      — rain that leaves a hidden message behind as it falls
//   k4  Warp        — code streams blasting outward from the center
//   k5  Wave        — every column falls together in rolling wave fronts
//   k6  Data Stream — sideways rivers of binary 0s and 1s
//   k7  Storm       — heavy, wind-blown downpour with lightning
//
// This program OWNS the global knobs (it makes its own colors):
//   p0 Hue    — color of the rain (about a third of the way = classic green)
//   p1 Speed  — overall speed
//   p2 Trail  — trail length (turn DOWN for longer trails)
//   p3 Glow   — how white-hot the heads and fresh glyphs are
//
// Camera knobs (all presets):
//   p8  Zoom      — CENTER = normal size. Left = smaller (zoom out, see the
//                   whole world); right = bigger (up to 8x)
//   p9  Pan X     — slide the view left/right (wraps around forever)
//   p10 Pan Y     — slide the view up/down (wraps around forever)
//   p11 Drift     — camera wanders and breathes by itself (0 = off)
//   p12 Drift Spd — how fast it wanders
// Default view: p8, p9 and p10 centered, p11 all the way down.
//
// How it works: the colors are a single "brightness ramp" (index 0 is
// black, 254 is brightest). The rain lives in a "world" of 120×60 small
// cells (480×360 pixels — bigger than the screen, so there's room to
// zoom out and pan around). Each cell has
// a glyph and a brightness. Heads are bright, every frame all cells fade
// a step (that fade IS the trail), and then the camera paints the grid
// onto the screen at whatever zoom and position you've chosen.
// =====================================================================

// ─── World grid ───────────────────────────────────────────────────────
// Small 3×5 glyphs in 4×6 cells. The world is 1.5x the screen in each
// direction; at normal zoom you see the middle of it.
#define RAIN_CW   4                       // glyph cell width  (px)
#define RAIN_CH   6                       // glyph cell height (px)
#define RAIN_WW   480                     // world width  (px)
#define RAIN_WH   360                     // world height (px)
#define RAIN_COLS (RAIN_WW / RAIN_CW)     // 120 columns
#define RAIN_ROWS (RAIN_WH / RAIN_CH)     // 60 rows
#define RAIN_ZMIN ((float)W / RAIN_WW)    // most zoomed out: whole world fits the screen

// Message for the Reveal preset (k3). Up to about 13 characters fits
// on screen at normal zoom.
#define RAIN_MESSAGE "VIDEOGEM"

static float rain_dt = 0.016f;      // seconds since last frame
static float rain_speed = 1.0f;     // global speed multiplier (p1)

// ─── Glyphs ───────────────────────────────────────────────────────────
// 24 made-up 3×5 symbols (the first two are 0 and 1). Each row is 3 bits,
// left to right. Glyph numbers 24–31 are mirrored copies of glyphs 2–9.
static const uint8_t rain_font[24][5] = {
  {7,5,5,5,7},  // 0
  {2,6,2,2,7},  // 1
  {7,1,2,2,4},
  {2,7,2,5,2},
  {5,2,7,2,5},
  {6,2,7,2,6},
  {1,2,6,2,2},
  {7,5,1,2,4},
  {4,7,5,5,3},
  {2,7,2,5,0},
  {5,5,1,2,4},
  {6,2,7,2,3},
  {3,4,6,2,6},
  {7,1,7,1,7},
  {5,5,7,1,1},
  {2,5,0,7,2},
  {7,2,7,2,6},
  {4,6,5,5,6},
  {1,2,7,2,2},
  {7,0,7,2,4},
  {5,5,3,2,4},
  {7,1,2,4,7},
  {2,7,7,2,2},
  {5,7,5,7,5},
};
#define RAIN_NUM_GLYPHS 32

static inline int rain_randGlyph() { return random(RAIN_NUM_GLYPHS); }

// ─── The "world": a grid of glyph cells ──────────────────────────────
// Instead of drawing glyphs straight onto the screen, every cell of the
// 53×30 grid remembers WHICH glyph it shows and HOW BRIGHT it is. Each
// frame we fade the brightness and then paint the whole screen from the
// grid through a "camera" — which is what makes zoom and pan possible
// without smearing the trails.
#define RAIN_CELLS (RAIN_COLS * RAIN_ROWS)
static uint8_t rw_level[RAIN_CELLS];   // brightness = palette index (0 = off)
static uint8_t rw_ga[RAIN_CELLS];      // glyph (low 5 bits) + size info:
                                       //   bit 7 set = part of a double-size glyph,
                                       //   bits 5–6 = which quarter of it
static bool    rain_noDim = false;     // when true, don't overwrite brighter cells

static void rain_clearWorld() {
  memset(rw_level, 0, sizeof(rw_level));
  memset(rw_ga, 0, sizeof(rw_ga));
}

static void rain_setCell(int col, int row, int g, uint8_t lv, uint8_t attr) {
  if (col < 0 || col >= RAIN_COLS || row < 0 || row >= RAIN_ROWS) return;
  int i = row * RAIN_COLS + col;
  if (rain_noDim && rw_level[i] > lv) return;
  rw_level[i] = lv;
  rw_ga[i]    = (g & 31) | attr;
}

// Round down, also for negative numbers (off the top/left of the world)
static inline int rain_floorDiv(int a, int b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }

// Put a glyph at pixel position (px, py) in the world. Scale 2 makes a
// double-size glyph that covers 2×2 cells.
static void rain_glyph(int px, int py, int g, uint8_t lv, int scale) {
  int col = rain_floorDiv(px, RAIN_CW);
  int row = rain_floorDiv(py, RAIN_CH);
  if (scale == 1) {
    rain_setCell(col, row, g, lv, 0);
  } else {
    for (int q = 0; q < 4; q++)
      rain_setCell(col + (q & 1), row + (q >> 1), g, lv, 0x80 | (q << 5));
  }
}

// Brightness of the cell at pixel position (px, py)
static uint8_t rain_cellLevel(int px, int py) {
  int col = rain_floorDiv(px, RAIN_CW), row = rain_floorDiv(py, RAIN_CH);
  if (col < 0 || col >= RAIN_COLS || row < 0 || row >= RAIN_ROWS) return 0;
  return rw_level[row * RAIN_COLS + col];
}

// "Flicker": swap the glyph in random cells that are still lit, at the
// same brightness. This is what makes the code look alive.
static void rain_flicker(int count) {
  for (int k = 0; k < count * 4; k++) {       // x4: the world has lots of cells
    int i = random(RAIN_CELLS);
    if (rw_level[i] > 8 && !(rw_ga[i] & 0x80)) rw_ga[i] = rain_randGlyph();
  }
}

// The trail: fade every cell a little
static void rain_fadeWorld(int steps) {
  for (int i = 0; i < RAIN_CELLS; i++) {
    uint8_t v = rw_level[i];
    rw_level[i] = (v > steps) ? v - steps : 0;
  }
}

// ─── Camera: zoom and pan ─────────────────────────────────────────────
// Fast lookup tables, built once:
//   rain_gpix  — each glyph as 6 rows of pixel bits (mirroring done already)
//   rain_colOf — which cell column a world x falls in
//   rain_lxOf  — which pixel inside that cell
static uint8_t rain_gpix[RAIN_NUM_GLYPHS][RAIN_CH];
static uint8_t rain_colOf[RAIN_WW], rain_lxOf[RAIN_WW];
static bool    rain_tablesReady = false;

static void rain_buildTables() {
  if (rain_tablesReady) return;
  for (int g = 0; g < RAIN_NUM_GLYPHS; g++) {
    bool mirror = g >= 24;
    int base = mirror ? g - 22 : g;          // 24–31 mirror glyphs 2–9
    for (int cy = 0; cy < RAIN_CH; cy++) {
      uint8_t bits = (cy < 5) ? rain_font[base][cy] : 0;
      uint8_t out = 0;
      for (int cx = 0; cx < 3; cx++)
        if ((bits >> (mirror ? cx : 2 - cx)) & 1) out |= 1 << cx;
      rain_gpix[g][cy] = out;
    }
  }
  for (int x = 0; x < RAIN_WW; x++) {
    rain_colOf[x] = x / RAIN_CW;
    rain_lxOf[x]  = x % RAIN_CW;
  }
  rain_tablesReady = true;
}

static float rain_camX = RAIN_WW / 2, rain_camY = RAIN_WH / 2;  // world point at screen center
static float rain_zoom = 1.0f;         // 1 = normal; smaller = zoomed out; up to 8

// Paint the whole screen from the world grid through the camera. The
// world wraps around at the edges, so you can pan forever in any direction.
static void rain_render() {
  uint8_t* buf = display.getBuffer();
  const int32_t WW = (int32_t)RAIN_WW << 16, WH = (int32_t)RAIN_WH << 16;   // 16.16 fixed point
  int32_t step = (int32_t)(65536.0f / rain_zoom);
  int32_t wx0 = (int32_t)((rain_camX - HALFW / rain_zoom) * 65536.0f) % WW;
  int32_t wy  = (int32_t)((rain_camY - HALFH / rain_zoom) * 65536.0f) % WH;
  if (wx0 < 0) wx0 += WW;
  if (wy < 0) wy += WH;

  for (int sy = 0; sy < H; sy++) {
    int iy = wy >> 16;
    int row = iy / RAIN_CH, ly = iy % RAIN_CH;
    const uint8_t* lvRow = rw_level + row * RAIN_COLS;
    const uint8_t* gaRow = rw_ga + row * RAIN_COLS;
    uint8_t* out = buf + sy * W;
    int32_t wx = wx0;
    for (int sx = 0; sx < W; sx++) {
      int ix = wx >> 16;
      uint8_t v = 0;
      uint8_t c = rain_colOf[ix];
      uint8_t lv = lvRow[c];
      if (lv) {
        int lx = rain_lxOf[ix];
        uint8_t ga = gaRow[c];
        bool on;
        if (!(ga & 0x80)) {
          on = (rain_gpix[ga & 31][ly] >> lx) & 1;
        } else {                                  // one quarter of a big glyph
          int q = (ga >> 5) & 3;
          int gx = ((q & 1) * RAIN_CW + lx) >> 1;
          int gy = ((q >> 1) * RAIN_CH + ly) >> 1;
          on = (rain_gpix[ga & 31][gy] >> gx) & 1;
        }
        if (on) v = lv;
      }
      out[sx] = v;
      wx += step;
      if (wx >= WW) wx -= WW;
    }
    wy += step;
    if (wy >= WH) wy -= WH;
  }
}

// Read the camera knobs (p8–p12) and move the camera
static void rain_updateCamera() {
  float k      = pots[8] / 1023.0f;
  float panX   = potMap(9, 0, RAIN_WW);        // p9: pan left/right
  float panY   = potMap(10, 0, RAIN_WH);       // p10: pan up/down
  float drift  = potMap(11, 0, 100) / 100.0f;  // p11: auto camera drift amount
  float dspeed = potMap(12, 1, 100) / 100.0f;  // p12: auto camera drift speed

  static float t = 0;
  t += rain_dt * dspeed;
  if (t > 10000.0f) t = 0;

  // p8: center = 1x. Left half zooms OUT to the whole world;
  // right half zooms IN to 8x. (Exponential, so it feels even.)
  float z = (k < 0.5f) ? powf(RAIN_ZMIN, (0.5f - k) * 2.0f)
                       : powf(8.0f, (k - 0.5f) * 2.0f);
  // Drift: the camera wanders on a slow looping path and "breathes" in and out
  panX += drift * 160.0f * sinf(t * 0.7f);
  panY += drift * 120.0f * sinf(t * 0.53f + 1.0f);
  z    *= 1.0f + drift * 0.6f * (0.5f + 0.5f * sinf(t * 0.31f));
  if (z < RAIN_ZMIN) z = RAIN_ZMIN;

  rain_zoom = z;
  rain_camX = panX;
  rain_camY = panY;
}

// ─── Colors (this program owns p0–p3) ─────────────────────────────────
static int rain_palHue = -1, rain_palGlow = -1, rain_palBoost = -1;

static void rain_buildPalette(int hue, int glow, int boost) {
  if (hue == rain_palHue && glow == rain_palGlow && boost == rain_palBoost) return;
  rain_palHue = hue; rain_palGlow = glow; rain_palBoost = boost;

  // Hue (0–255) → a fully saturated base color
  float h = hue / 256.0f * 6.0f;
  int sector = (int)h;
  float f = h - sector;
  float r = 0, g = 0, b = 0;
  switch (sector) {
    case 0: r = 1; g = f; b = 0; break;
    case 1: r = 1 - f; g = 1; b = 0; break;
    case 2: r = 0; g = 1; b = f; break;
    case 3: r = 0; g = 1 - f; b = 1; break;
    case 4: r = f; g = 0; b = 1; break;
    default: r = 1; g = 0; b = 1 - f; break;
  }

  for (int i = 1; i < 255; i++) {
    float t = i / 254.0f;
    float lvl = t * t * (1.5f - 0.5f * t);          // soft brightness curve
    lvl = lvl + (1.0f - lvl) * (boost / 255.0f) * 0.6f;  // lightning flash
    float wr = r * lvl, wg = g * lvl, wb = b * lvl;
    // The top of the ramp heats up toward white (Glow knob)
    float hot = (t - 0.8f) / 0.2f;
    if (hot > 0) {
      hot *= glow / 255.0f;
      wr += (1 - wr) * hot; wg += (1 - wg) * hot; wb += (1 - wb) * hot;
    }
    display.setColor(i, (uint8_t)(wr * 255), (uint8_t)(wg * 255), (uint8_t)(wb * 255));
  }
  display.setColor(0, 0, 0, 0);
  display.setColor(255, 255, 255, 255);
}

// ─── Streams (one falling line of code each) ─────────────────────────
// Kept as parallel arrays of plain numbers rather than a struct — the
// Arduino IDE auto-declares functions before any struct in this file.
#define RAIN_MAX 300
static float   rs_pos[RAIN_MAX];    // head position (in cells, along its path)
static float   rs_spd[RAIN_MAX];    // speed (cells per second)
static float   rs_aux[RAIN_MAX];    // extra per-stream value (angle, x, ...)
static int16_t rs_last[RAIN_MAX];   // last cell drawn (so we draw each cell once)
static uint8_t rs_kind[RAIN_MAX];   // layer / type
static int     rs_count = 0;

static int   rain_lastPreset = -1;
static bool  rain_needReset = true;

// Start stream i above the top, with a random delay and speed
static void rain_respawn(int i, float baseSpd, float spread, int maxDelay) {
  rs_pos[i]  = -(float)random(0, maxDelay + 1);
  rs_spd[i]  = baseSpd * (1.0f - spread * random(0, 1000) / 1000.0f);
  if (rs_spd[i] < 1.0f) rs_spd[i] = 1.0f;
  rs_last[i] = (int16_t)floorf(rs_pos[i]);
}

// Move a vertical stream down and draw the cells its head passes.
//   px    = left edge in pixels     scale = 1 or 2
//   head  = head color              body  = color for the cell behind it
//   xDrift = sideways pixels per row (wind), 0 for straight down
// Returns true when the stream has fallen off the bottom.
static bool rain_fall(int i, int px, int scale, uint8_t head, uint8_t body, float xDrift) {
  rs_pos[i] += rs_spd[i] * rain_speed * rain_dt;
  int cell = (int)floorf(rs_pos[i]);
  int rows = RAIN_ROWS / scale;
  int ch = RAIN_CH * scale;
  if (cell > rs_last[i]) {
    // Re-draw the old head as normal code, then draw the new head(s)
    for (int c = rs_last[i]; c <= cell; c++) {
      if (c < 0 || c >= rows) continue;
      int x = px + (int)(c * xDrift);
      bool isHead = (c == cell);
      rain_glyph(x, c * ch, rain_randGlyph(), isHead ? head : body, scale);
    }
    rs_last[i] = cell;
  }
  return cell >= rows;
}

// ─── Preset 0: Classic ────────────────────────────────────────────────
static void rain_classic() {
  int density = potMap(4, 10, 100);   // p4: % of columns raining
  int spread  = potMap(5, 0, 90);     // p5: speed variety (%)
  int flick   = potMap(6, 0, 80);     // p6: code flicker amount
  int gap     = potMap(7, 2, 90);     // p7: pause before a column restarts

  if (rain_needReset) {
    rs_count = RAIN_COLS;
    for (int i = 0; i < rs_count; i++) rain_respawn(i, 22, spread / 100.0f, RAIN_ROWS);
    rain_needReset = false;
  }
  for (int i = 0; i < rs_count; i++) {
    // Density: a fixed pattern of which columns are "on"
    if ((i * 37 + 13) % 100 >= density) continue;
    if (rain_fall(i, i * RAIN_CW + 1, 1, 255, 235, 0))
      rain_respawn(i, 22, spread / 100.0f, gap);
  }
  rain_flicker(flick);
}

// ─── Preset 1: Depth ──────────────────────────────────────────────────
// Three layers: far (small, dim, slow), mid, and near (double-size,
// bright, fast). The far layer is drawn first so nearer code covers it.
static void rain_depth() {
  int farN   = potMap(4, 5, RAIN_COLS);  // p4: far-layer streams
  int nearN  = potMap(5, 0, 24);      // p5: near-layer streams (big glyphs)
  int ratio  = potMap(6, 15, 40);     // p6: depth (speed difference, x10)
  int flick  = potMap(7, 0, 60);      // p7: flicker

  const int MID = RAIN_COLS, NEAR = 2 * RAIN_COLS;   // stream index where each layer starts
  if (rain_needReset) {
    rs_count = NEAR + RAIN_COLS / 2;
    for (int i = 0; i < rs_count; i++) {
      rs_kind[i] = (i < MID) ? 0 : (i < NEAR) ? 1 : 2;
      rain_respawn(i, 8, 0.4f, RAIN_ROWS);
    }
    rain_needReset = false;
  }
  float k = ratio / 10.0f;
  for (int layer = 0; layer < 3; layer++) {
    for (int i = 0; i < rs_count; i++) {
      if (rs_kind[i] != layer) continue;
      int n = i - (layer == 0 ? 0 : layer == 1 ? MID : NEAR);
      if (layer == 0 && n >= farN) continue;
      if (layer == 1 && ((n * 37) % RAIN_COLS) >= farN / 2 + 3) continue;
      if (layer == 2 && ((n * 11) % (RAIN_COLS / 2)) >= nearN) continue;
      float base = (layer == 0) ? 8 : (layer == 1) ? 8 * k : 8 * k * k;
      uint8_t head = (layer == 0) ? 110 : (layer == 1) ? 200 : 255;
      uint8_t body = (layer == 0) ? 90  : (layer == 1) ? 170 : 240;
      int scale = (layer == 2) ? 2 : 1;
      int px = (layer == 2) ? n * RAIN_CW * 2 + 2 : n * RAIN_CW + 1;
      // Speed is set at respawn; scale it by layer here
      float saved = rs_spd[i];
      rs_spd[i] = saved * base / 8.0f;
      rain_noDim = (layer < 2);          // far/mid never cover brighter code
      bool done = rain_fall(i, px, scale, head, body, 0);
      rain_noDim = false;
      rs_spd[i] = saved;
      if (done) rain_respawn(i, 8, 0.4f, layer == 2 ? 30 : 12);
    }
  }
  rain_flicker(flick);
}

// ─── Preset 2: Glitch ─────────────────────────────────────────────────
// Classic rain, but the picture keeps tearing: horizontal slices jump
// sideways, columns stutter backwards, and bright bars flash across.
static int rain_glitchRate = 0, rain_glitchSlice = 2, rain_glitchShift = 4;

static void rain_glitch() {
  int rate   = potMap(4, 0, 100);     // p4: glitch amount
  int slice  = potMap(5, 2, 60);      // p5: max slice height (px)
  int shift  = potMap(6, 4, 160);     // p6: max sideways jump (px)
  int flick  = potMap(7, 0, 80);      // p7: flicker

  if (rain_needReset) {
    rs_count = RAIN_COLS;
    for (int i = 0; i < rs_count; i++) rain_respawn(i, 22, 0.5f, RAIN_ROWS);
    rain_needReset = false;
  }
  for (int i = 0; i < rs_count; i++) {
    if ((i * 37 + 13) % 100 >= 70) continue;
    // Stutter: sometimes a column jumps back up a few cells
    if (random(1000) < rate / 10) { rs_pos[i] -= random(1, 5); rs_last[i] = (int16_t)floorf(rs_pos[i]); }
    if (rain_fall(i, i * RAIN_CW + 1, 1, 255, 235, 0))
      rain_respawn(i, 22, 0.5f, 30);
  }
  rain_flicker(flick);

  rain_glitchRate = rate; rain_glitchSlice = slice; rain_glitchShift = shift;
}

// Screen-level glitches, applied AFTER the camera paints the frame
// (they tear the picture itself, so they last one frame each).
static void rain_glitchPost() {
  int rate = rain_glitchRate, slice = rain_glitchSlice, shift = rain_glitchShift;
  // Slice tear: rotate a band of rows sideways
  static uint8_t rowTmp[W];
  uint8_t* buf = display.getBuffer();
  int tears = (random(100) < rate) ? random(1, 4) : 0;
  for (int k = 0; k < tears; k++) {
    int y0 = random(0, H), h = random(2, slice + 1);
    int off = random(-shift, shift + 1);
    if (off < 0) off += W;
    for (int y = y0; y < y0 + h && y < H; y++) {
      uint8_t* row = buf + y * W;
      for (int x = 0; x < W; x++) rowTmp[(x + off) % W] = row[x];
      memcpy(row, rowTmp, W);
    }
  }
  // Occasional bright scan bar
  if (random(1000) < rate) {
    int y = random(0, H - 2);
    display.fillRect(0, y, W, random(1, 3), 200);
  }
}

// ─── Preset 3: Reveal ─────────────────────────────────────────────────
// Rain falls normally, but when a head passes through part of the hidden
// message, that cell "sticks" and stays lit. The message builds up, holds,
// then dissolves and starts again. Change RAIN_MESSAGE at the top.
static uint8_t rain_mask[(RAIN_CELLS + 7) / 8];    // which cells belong to the message
static uint8_t rain_shown[(RAIN_CELLS + 7) / 8];   // which have been revealed so far
static bool    rain_maskReady = false;
static int     rain_maskTotal = 0;

static inline bool rain_bitGet(const uint8_t* bits, int i) { return (bits[i >> 3] >> (i & 7)) & 1; }
static inline void rain_bitSet(uint8_t* bits, int i) { bits[i >> 3] |= 1 << (i & 7); }

static void rain_buildMask() {
  // Draw the message with the normal text font, then read it back:
  // each font pixel becomes one glyph cell on the rain grid.
  display.fillScreen(0);
  display.setTextSize(1);
  display.setTextColor(255);
  display.setCursor(0, 0);
  display.print(RAIN_MESSAGE);
  int len = 0;
  while (RAIN_MESSAGE[len]) len++;
  int textW = len * 6 - 1;
  if (textW > W) textW = W;
  int col0 = (RAIN_COLS - textW) / 2;
  if (col0 < 0) col0 = 0;
  // Each font pixel covers 1 cell across and 2 cells down, so the letters
  // come out tall enough to read with the small glyphs.
  int row0 = (RAIN_ROWS - 16) / 2;
  rain_maskTotal = 0;
  memset(rain_mask, 0, sizeof(rain_mask));
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < textW && col0 + x < RAIN_COLS; x++) {
      if (display.getPixel(x, y)) {
        for (int dy = 0; dy < 2; dy++) {
          rain_bitSet(rain_mask, (row0 + y * 2 + dy) * RAIN_COLS + col0 + x);
          rain_maskTotal++;
        }
      }
    }
  }
  display.fillScreen(0);
  rain_maskReady = true;
}

static void rain_reveal() {
  int density = potMap(4, 10, 100);   // p4: rain density (%)
  int stick   = potMap(5, 5, 100);    // p5: how easily cells stick (%)
  int hold    = potMap(6, 1, 15);     // p6: seconds to hold the full message
  int bright  = potMap(7, 120, 254);  // p7: message brightness

  static float holdTimer = 0;
  if (!rain_maskReady) rain_buildMask();
  if (rain_needReset) {
    rs_count = RAIN_COLS;
    for (int i = 0; i < rs_count; i++) rain_respawn(i, 20, 0.5f, RAIN_ROWS);
    memset(rain_shown, 0, sizeof(rain_shown));
    holdTimer = 0;
    rain_needReset = false;
  }

  for (int i = 0; i < rs_count; i++) {
    if ((i * 37 + 13) % 100 >= density) continue;
    int before = rs_last[i];
    bool done = rain_fall(i, i * RAIN_CW + 1, 1, 255, 235, 0);
    // Did the head pass through any message cells?
    for (int c = before + 1; c <= rs_last[i]; c++) {
      if (c < 0 || c >= RAIN_ROWS) continue;
      int cell = c * RAIN_COLS + i;
      if (rain_bitGet(rain_mask, cell) && random(100) < stick) rain_bitSet(rain_shown, cell);
    }
    if (done) rain_respawn(i, 20, 0.5f, 15);
  }

  // Keep revealed cells lit (with the odd glyph change)
  int shownCount = 0;
  for (int cell = 0; cell < RAIN_CELLS; cell++) {
    if (!rain_bitGet(rain_shown, cell)) continue;
    shownCount++;
    int r = cell / RAIN_COLS, c = cell % RAIN_COLS;
    if (rw_level[cell] < bright || random(100) < 3)
      rain_setCell(c, r, rain_randGlyph(), bright, 0);
  }

  // Fully revealed? Hold, then dissolve and start over.
  if (shownCount >= rain_maskTotal && rain_maskTotal > 0) {
    holdTimer += rain_dt;
    if (holdTimer > hold) {
      memset(rain_shown, 0, sizeof(rain_shown));
      holdTimer = 0;
    }
  }
}

// ─── Preset 4: Warp ───────────────────────────────────────────────────
// Streams of code fly outward from the center like jumping to hyperspace.
// Glyphs speed up and double in size as they get closer to "you".
static void rain_warp() {
  int count = potMap(4, 8, 120);      // p4: number of streams
  int speed = potMap(5, 10, 120);     // p5: flight speed
  int spin  = potMap(6, 0, 80);       // p6: twist (streams curve around)
  int flick = potMap(7, 0, 40);       // p7: flicker

  if (rain_needReset) {
    rs_count = 120;
    for (int i = 0; i < rs_count; i++) {
      rs_aux[i] = random(0, 256);               // angle (0–255 = full circle)
      rs_pos[i] = random(0, 300);               // distance from center (px)
      rs_last[i] = -1;
    }
    rain_needReset = false;
  }
  for (int i = 0; i < count && i < rs_count; i++) {
    float d = rs_pos[i];
    // Perspective: things move faster as they get closer
    rs_pos[i] += speed * rain_speed * (0.2f + d / 80.0f) * rain_dt;
    rs_aux[i] += spin * rain_dt * (0.5f + (i & 3) * 0.25f);
    d = rs_pos[i];
    int scale = (d > 140) ? 2 : 1;
    int step = (int)(d / (RAIN_CH * scale));
    if (step != rs_last[i]) {
      rs_last[i] = step;
      int a = (int)rs_aux[i] & 255;
      int x = RAIN_WW / 2 + (int)((sinTab[(a + 64) & 255] - 128) * d / 127) - 2 * scale;
      int y = RAIN_WH / 2 + (int)((sinTab[a] - 128) * d / 127) - 3 * scale;
      uint8_t lv = (d < 30) ? 120 : 255;         // dimmer near the center
      rain_glyph(x, y, rain_randGlyph(), lv, scale);
    }
    if (d > 320) {                               // out of the world: relaunch
      rs_pos[i] = random(4, 20);
      rs_aux[i] = random(0, 256);
      rs_last[i] = -1;
    }
  }
  rain_flicker(flick);
}

// ─── Preset 5: Wave ───────────────────────────────────────────────────
// All columns fall at the same speed, but each is offset by a sine wave,
// so the heads form rolling wave-shaped fronts sweeping down the screen.
static void rain_wave() {
  int amp   = potMap(4, 0, 40);       // p4: wave height (rows)
  int freq  = potMap(5, 1, 6);        // p5: waves across the screen
  int roll  = potMap(6, 0, 100);      // p6: how fast the wave shape travels sideways
  int gap   = potMap(7, 4, 60);       // p7: rows between fronts

  static float fall = 0, shape = 0;
  if (rain_needReset) {
    for (int i = 0; i < RAIN_COLS; i++) rs_last[i] = -1000;
    rain_needReset = false;
  }
  fall  += 20.0f * rain_speed * rain_dt;
  shape += roll / 20.0f * rain_dt;
  int period = RAIN_ROWS + gap;
  if (fall > period * 100.0f) fall -= period * 100.0f;

  for (int c = 0; c < RAIN_COLS; c++) {
    float off = sinf(c * freq * TWO_PI / RAIN_COLS + shape) * amp;
    int head = (int)floorf(fall + off);
    if (head == rs_last[c]) continue;
    int from = (rs_last[c] < head && head - rs_last[c] < 6) ? rs_last[c] : head;
    for (int h = from; h <= head; h++) {
      int row = ((h % period) + period) % period;
      if (row >= RAIN_ROWS) continue;
      rain_glyph(c * RAIN_CW + 1, row * RAIN_CH, rain_randGlyph(), (h == head) ? 255 : 235, 1);
    }
    rs_last[c] = head;
  }
  rain_flicker(20);
}

// ─── Preset 6: Data Stream ────────────────────────────────────────────
// Rivers of binary (just 0s and 1s) flowing sideways across the screen,
// like data rushing along a bus. Odd rows can flow the other way.
static void rain_dataStream() {
  int density = potMap(4, 10, 100);   // p4: % of rows flowing
  int spread  = potMap(5, 0, 90);     // p5: speed variety (%)
  int flick   = potMap(6, 0, 60);     // p6: bit flicker
  int dir     = potMap(7, 0, 2);      // p7: direction: 0 right, 1 left, 2 both

  if (rain_needReset) {
    rs_count = RAIN_ROWS;
    for (int i = 0; i < rs_count; i++) rain_respawn(i, 30, spread / 100.0f, RAIN_COLS);
    rain_needReset = false;
  }
  for (int r = 0; r < rs_count; r++) {
    if ((r * 61 + 7) % 100 >= density) continue;
    bool left = (dir == 1) || (dir == 2 && (r & 1));
    rs_pos[r] += rs_spd[r] * rain_speed * rain_dt;
    int cell = (int)floorf(rs_pos[r]);
    if (cell > rs_last[r]) {
      for (int c = rs_last[r]; c <= cell; c++) {
        if (c < 0 || c >= RAIN_COLS) continue;
        int col = left ? (RAIN_COLS - 1 - c) : c;
        rain_glyph(col * RAIN_CW + 1, r * RAIN_CH, random(2), (c == cell) ? 255 : 230, 1);
      }
      rs_last[r] = cell;
    }
    if (cell >= RAIN_COLS) rain_respawn(r, 30, spread / 100.0f, 30);
  }
  // Flicker bits only (keep it binary)
  for (int i = 0; i < flick * 4; i++) {
    int c = random(RAIN_COLS), r = random(RAIN_ROWS);
    int px = c * RAIN_CW + 1, py = r * RAIN_CH;
    uint8_t lv = rain_cellLevel(px, py);
    if (lv > 8) rain_glyph(px, py, random(2), lv, 1);
  }
}

// ─── Preset 7: Storm ──────────────────────────────────────────────────
// A heavy downpour: dense, fast, blown sideways by the wind, with
// lightning flashes that light up the whole screen.
static int rain_flash = 0;   // brightness boost from lightning (0 = none)

static void rain_storm() {
  int density = potMap(4, 40, 100);   // p4: rain density (%)
  int wind    = potMap(5, -30, 30);   // p5: wind (left ↔ right)
  int bolts   = potMap(6, 0, 100);    // p6: lightning frequency
  int flick   = potMap(7, 0, 80);     // p7: flicker

  if (rain_needReset) {
    rs_count = RAIN_COLS + 60;              // extra columns off each side, so wind doesn't leave gaps
    for (int i = 0; i < rs_count; i++) rain_respawn(i, 40, 0.5f, RAIN_ROWS);
    rain_needReset = false;
  }
  float drift = wind / 10.0f;               // px sideways per row
  for (int i = 0; i < rs_count; i++) {
    if ((i * 37 + 13) % 100 >= density) continue;
    int col = i - 30;                       // columns -30 … 149 (some start outside the world)
    if (rain_fall(i, col * RAIN_CW + 1, 1, 255, 235, drift))
      rain_respawn(i, 40, 0.5f, 8);
  }
  rain_flicker(flick);

  // Lightning: a sudden flash that decays over a few frames
  if (rain_flash > 0) rain_flash = (rain_flash * 3) / 4;
  if (random(10000) < bolts) rain_flash = 255;
}

// ─── Public interface ─────────────────────────────────────────────────

const char* prog_rain_name() { return "DIGITAL RAIN"; }

const char* prog_rain_character() {
  return "Falling code: classic, depth, glitch, reveal, warp, wave, data, storm";
}

static const char* const rain_presetNames[] = {
  "Classic", "Depth", "Glitch", "Reveal", "Warp", "Wave", "Data Stream", "Storm"
};
#define RAIN_NUM_PRESETS 8

const char* prog_rain_presetName(int preset) {
  if (preset >= 0 && preset < RAIN_NUM_PRESETS) return rain_presetNames[preset];
  return NULL;
}

static const char* const rain_globalLabels[4] = { "Hue", "Speed", "Trail", "Glow" };

static const char* const rain_potLabels[RAIN_NUM_PRESETS][4] = {
  {"Density", "Variety", "Flicker", "Gap"},       // Classic
  {"Far",     "Near",    "Depth",   "Flicker"},   // Depth
  {"Glitch",  "Slice",   "Jump",    "Flicker"},   // Glitch
  {"Density", "Stick",   "Hold",    "Bright"},    // Reveal
  {"Streams", "Speed",   "Twist",   "Flicker"},   // Warp
  {"Height",  "Waves",   "Roll",    "Gap"},       // Wave
  {"Density", "Variety", "Flicker", "Direction"}, // Data Stream
  {"Density", "Wind",    "Lightning", "Flicker"}, // Storm
};

static const char* const rain_camLabels[5] = { "Zoom", "Pan X", "Pan Y", "Drift", "Drift Spd" };

const char* prog_rain_potLabel(int preset, int pot) {
  if (pot >= 0 && pot < 4) return rain_globalLabels[pot];
  if (pot >= 8 && pot <= 12) return rain_camLabels[pot - 8];
  if (pot < 4 || pot > 7) return "";
  if (preset < 0 || preset >= RAIN_NUM_PRESETS) return "";
  return rain_potLabels[preset][pot - 4];
}

uint8_t prog_rain_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;   // we paint every pixel ourselves (through the camera)
}

// Called by the platform whenever you switch TO this program
void prog_rain_init() {
  rain_palHue = -1;           // force our colors to be rebuilt
  rain_needReset = true;
  rain_lastPreset = -1;
  rain_clearWorld();
}

void prog_rain_draw(int preset) {
  // Frame timing (this program owns p1, so it keeps its own clock)
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  rain_dt = (dt > 0 && dt < 0.1f) ? dt : 0.016f;

  // Global knobs
  int hue   = potMap(0, 0, 255);
  rain_speed = potMap(1, 2, 30) / 10.0f;
  int trail = potMap(2, 20, 600);      // fade steps per second
  int glow  = potMap(3, 0, 255);
  if (preset != 7) rain_flash = 0;
  rain_buildPalette(hue, glow, rain_flash);

  rain_buildTables();
  rain_updateCamera();

  // New preset? Clear the world and restart the streams
  if (preset != rain_lastPreset) {
    rain_lastPreset = preset;
    rain_needReset = true;
    rain_clearWorld();
  }

  // The trail: fade every cell a little every frame
  static float fadeAcc = 0;
  fadeAcc += trail * rain_dt;
  int steps = (int)fadeAcc;
  if (steps > 0) {
    fadeAcc -= steps;
    rain_fadeWorld(steps > 255 ? 255 : steps);
  }

  switch (preset) {
    case 0: rain_classic();    break;
    case 1: rain_depth();      break;
    case 2: rain_glitch();     break;
    case 3: rain_reveal();     break;
    case 4: rain_warp();       break;
    case 5: rain_wave();       break;
    case 6: rain_dataStream(); break;
    case 7: rain_storm();      break;
    default:
      rain_clearWorld();
      break;
  }

  // Paint the frame through the camera, then any screen-level effects
  rain_render();
  if (preset == 2) rain_glitchPost();
  if (preset >= RAIN_NUM_PRESETS) {
    display.setTextColor(255);
    display.setTextSize(1);
    display.setCursor(100, 116);
    display.print("empty preset");
  }
}

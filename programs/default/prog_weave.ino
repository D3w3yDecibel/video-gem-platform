// =====================================================================
// PROGRAM: WEAVE (by Dewey)
// Glowing 3D wireframe forms made of woven strands, turning in space.
// Inspired by the look of neon "creative coding" math art: thin strands,
// a two-tone neon gradient from top to bottom, faint strands at the back,
// bright ones at the front, and a soft glow.
//
// Presets:
//   k0  Weave Sphere — strands wrapping a sphere, twisting as they go
//   k1  Torus Knot   — a braid of strands tied around a donut
//   k2  Mobius       — strands across a twisted band with no inside or outside
//   k3  Lissajous Orb— 3D pendulum curves looping inside a ball
//   k4  Helix Braid  — spirals braided around a vertical axis
//   k5  Wobble Rings — stacked rings that ripple like jelly
//   k6  Lattice      — two sets of spirals crossing to make a woven cage
//
// This program OWNS the global knobs (it makes its own colors):
//   p0 Colors — neon pair: pink/cyan, orange/purple, lime/blue,
//               gold/red, ice/violet, rainbow
//   p1 Speed  — overall speed
//   p2 Trail  — turn DOWN for longer motion trails
//   p3 Glow   — none (left) → soft halo → big halo (right)
// Camera and complexity (all presets):
//   p8  Spin      — center = still; left/right = turn either way
//   p9  Tilt
//   p10 Zoom      — CENTER = normal. Left = zoom out, right = fly in (up to 6x)
//   p11 Pan X     p12 Pan Y  — move around while zoomed in (center = middle)
//   p13 Pulse     — zoom breathes in and out by itself (left = off)
//   p14 Layers    — 1, 2 or 3 nested copies of the form, counter-rotating
//   p15 Over/Under— strands dip under and over each other like real weaving
//   Defaults: p10, p11, p12 centered; p13, p14, p15 all the way left.
//
// Smoothness: strands are drawn anti-aliased (sub-pixel), long segments
// follow smooth curves, and all knobs glide instead of jumping. If the
// chip can't keep up, quality steps down automatically (see wv_quality).
//
// How the colors work: the 256 colors are 31 hues × 8 brightness steps.
// Hue comes from how high the strand is on screen (top color → bottom
// color); brightness comes from depth (front = bright, back = dim).
// The trail fades each pixel one brightness step at a time.
// =====================================================================

#define WV_LEVELS 8                     // brightness steps per hue
#define WV_HUES   31                    // hues from top color to bottom color
#define WV_MAXP   1600                  // max projected points per frame
#define WV_MAXSTR 160                   // max strands per frame (all layers)

// ─── Fast sine (table) ────────────────────────────────────────────────
static float wv_sinT[512];
static bool  wv_ready = false;

static void wv_init() {
  if (wv_ready) return;
  for (int i = 0; i < 512; i++) wv_sinT[i] = sinf(i * TWO_PI / 512.0f);
  wv_ready = true;
}
static inline float wsin(float a) { return wv_sinT[((int)(a * (512.0f / TWO_PI))) & 511]; }
static inline float wcos(float a) { return wsin(a + HALF_PI); }

// ─── Smooth knobs ─────────────────────────────────────────────────────
// Knob readings jitter a little. Fed straight into zoom, tilt or twist,
// that jitter makes the whole picture shimmer. So every knob is passed
// through a gentle low-pass filter (~0.12 s): it glides to new positions.
static float wv_sm[16];
static bool  wv_smReady = false;

static void wv_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!wv_smReady) wv_sm[i] = pots[i];
    else wv_sm[i] += (pots[i] - wv_sm[i]) * a;
  }
  wv_smReady = true;
}

// Smoothed knob mapped to lo…hi, as a float (for continuous settings)
static inline float wv_potf(int idx, float lo, float hi) {
  return lo + (hi - lo) * (wv_sm[idx] / 1023.0f);
}
// …and rounded to a whole number (for counts like strands or lobes)
static inline int wv_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (wv_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colors ───────────────────────────────────────────────────────────
static int wv_palScheme = -1, wv_palGlow = -1;

static void wv_buildPalette(int scheme, int glow) {
  if (scheme == wv_palScheme && glow == wv_palGlow) return;
  wv_palScheme = scheme; wv_palGlow = glow;
  // Top color (A) and bottom color (B) for each scheme
  static const uint8_t pairs[5][6] = {
    {255, 40, 170,   0, 220, 255},   // pink → cyan
    {255, 140, 0,  150, 40, 255},    // orange → purple
    {160, 255, 40,   0, 90, 255},    // lime → blue
    {255, 210, 40, 255, 20, 40},     // gold → red
    {220, 240, 255, 140, 60, 255},   // ice → violet
  };
  for (int h = 0; h < WV_HUES; h++) {
    float f = h / (float)(WV_HUES - 1);
    float r, g, b;
    if (scheme < 5) {
      const uint8_t* p = pairs[scheme];
      r = p[0] + (p[3] - p[0]) * f;
      g = p[1] + (p[4] - p[1]) * f;
      b = p[2] + (p[5] - p[2]) * f;
    } else {                                   // rainbow
      float hh = f * 5.0f;
      int s = (int)hh; float q = hh - s;
      float rr[6] = {1, 1 - q, 0, 0, q, 1}, gg[6] = {q, 1, 1, 1 - q, 0, 0}, bb[6] = {0, 0, q, 1, 1, 1 - q};
      r = rr[s] * 255; g = gg[s] * 255; b = bb[s] * 255;
    }
    for (int l = 0; l < WV_LEVELS; l++) {
      float k = (l + 1) / (float)WV_LEVELS;
      k = k * k;                               // dim levels really dim
      float wr = r * k, wg = g * k, wb = b * k;
      if (l == WV_LEVELS - 1 && glow > 0) {    // brightest step turns white-hot
        float w = 0.25f * glow;
        wr += (255 - wr) * w; wg += (255 - wg) * w; wb += (255 - wb) * w;
      }
      display.setColor(1 + h * WV_LEVELS + l, (uint8_t)wr, (uint8_t)wg, (uint8_t)wb);
    }
  }
  for (int i = 1 + WV_HUES * WV_LEVELS; i < 255; i++) display.setColor(i, 0, 0, 0);
  display.setColor(0, 0, 0, 0);
  display.setColor(255, 255, 255, 255);
}

static inline uint8_t wv_color(int hue, int level) {
  if (level < 0) return 0;
  if (level >= WV_LEVELS) level = WV_LEVELS - 1;
  if (hue < 0) hue = 0;
  if (hue >= WV_HUES) hue = WV_HUES - 1;
  return 1 + hue * WV_LEVELS + level;
}

// Trail: drop every pixel's brightness by n steps (hue stays the same).
// To save time we remember, for each row, the span of pixels that might
// still be lit, and only fade inside it. (After fading we measure the
// span again, so it shrinks as trails die away.)
static int16_t wv_rowMin[H], wv_rowMax[H];

static void wv_spansFull() {                 // "anything could be lit"
  for (int y = 0; y < H; y++) { wv_rowMin[y] = 0; wv_rowMax[y] = W - 1; }
}

static void wv_fade(int n) {
  static uint8_t tab[256];
  static int tabN = -1;
  if (n != tabN) {                           // rebuild the lookup only when n changes
    tabN = n;
    for (int v = 0; v < 256; v++) {
      if (v == 0 || v > WV_HUES * WV_LEVELS) { tab[v] = 0; continue; }
      int l = (v - 1) % WV_LEVELS;
      tab[v] = (l >= n) ? v - n : 0;
    }
  }
  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < H; y++) {
    int x0 = wv_rowMin[y], x1 = wv_rowMax[y];
    if (x1 < x0) continue;                   // row is dark
    uint8_t* row = buf + y * W;
    int nmin = W, nmax = -1;
    for (int x = x0; x <= x1; x++) {
      uint8_t v = tab[row[x]];
      row[x] = v;
      if (v) { if (x < nmin) nmin = x; nmax = x; }
    }
    wv_rowMin[y] = nmin; wv_rowMax[y] = nmax;
  }
}

// ─── 3D → screen ──────────────────────────────────────────────────────
// Points for this frame, already projected. Each strand is a run of
// points; wv_strStart[] remembers where each strand begins.
static int16_t wv_sx[WV_MAXP], wv_sy[WV_MAXP];   // in 1/16ths of a pixel (for smooth motion)
static int8_t  wv_sz[WV_MAXP];          // depth: -100 (front) … +100 (back)
static uint8_t wv_hue[WV_MAXP];         // color along the top→bottom gradient
static int     wv_np = 0;
static int16_t wv_strStart[WV_MAXSTR + 1];
static int     wv_ns = 0;
static int     wv_budget = WV_MAXP;     // points allowed for the current layer

// Camera, set once per frame (and per layer)
static float wv_ca, wv_sa, wv_ct, wv_st;   // spin and tilt
static float wv_size;                      // pixels per unit (zoom)
static float wv_panX, wv_panY;             // screen offset (pan)
static float wv_scale = 1.0f;              // size of the current layer

// Perspective factor 3.2/(3.2+z) for z from -2 to +2, as a table:
// division is slow on this chip, a lookup is fast.
static float wv_persp[256];
static bool  wv_perspReady = false;
static void wv_buildPersp() {
  if (wv_perspReady) return;
  for (int i = 0; i < 256; i++) {
    float z = -2.0f + 4.0f * i / 255.0f;
    wv_persp[i] = 3.2f / (3.2f + z);
  }
  wv_perspReady = true;
}

static void wv_beginStrand() {
  if (wv_ns < WV_MAXSTR) wv_strStart[wv_ns++] = wv_np;
}

// Spin, tilt, layer size and zoom all combined into one 3×3 matrix,
// worked out once per layer — so each point needs ~30% fewer
// floating-point operations (slow on this chip: it has no FPU).
static float wv_m[9];          // rows: screen x (1/16 px), screen y (1/16 px), depth z
static float wv_hueA, wv_hueB; // hue = wv_hueA - screenY * wv_hueB
static float wv_cx16, wv_cy16; // screen centre + pan, in 1/16 px

static void wv_setMatrix() {
  float S = wv_size * 16.0f * wv_scale;              // zoom × layer size, in 1/16 px
  // x1 = x*ca + z*sa ; z1 = -x*sa + z*ca
  // y2 = y*ct - z1*st ; z2 = y*st + z1*ct
  wv_m[0] =  wv_ca * S;           wv_m[1] = 0;               wv_m[2] =  wv_sa * S;
  wv_m[3] =  wv_sa * wv_st * S;   wv_m[4] = wv_ct * S;       wv_m[5] = -wv_ca * wv_st * S;
  wv_m[6] = -wv_sa * wv_ct * wv_scale;
  wv_m[7] =  wv_st * wv_scale;
  wv_m[8] =  wv_ca * wv_ct * wv_scale;
  // Hue from the form's height: y2 = rowY / (size*16) before perspective
  float K = (WV_HUES - 1) / 2.5f;
  wv_hueA = 1.25f * K;
  wv_hueB = K / (wv_size * 16.0f);
  wv_cx16 = (HALFW + wv_panX) * 16.0f;
  wv_cy16 = (HALFH + wv_panY) * 16.0f;
}

static void wv_addPoint(float x, float y, float z) {
  if (wv_np >= WV_MAXP) return;
  float X  = wv_m[0] * x + wv_m[2] * z;               // (m[1] is always 0)
  float Y  = wv_m[3] * x + wv_m[4] * y + wv_m[5] * z;
  float z2 = wv_m[6] * x + wv_m[7] * y + wv_m[8] * z;
  int pi = (int)((z2 + 2.0f) * 63.75f);
  float p = wv_persp[pi < 0 ? 0 : pi > 255 ? 255 : pi];   // perspective: nearer = bigger
  float sx = wv_cx16 + X * p;
  float sy = wv_cy16 - Y * p;
  const float LIM = 1900.0f * 16.0f;                      // keep numbers sane when zoomed way in
  if (sx < -LIM) sx = -LIM; else if (sx > LIM) sx = LIM;
  if (sy < -LIM) sy = -LIM; else if (sy > LIM) sy = LIM;
  wv_sx[wv_np] = (int16_t)sx;                             // 1/16 px: keeps sub-pixel precision
  wv_sy[wv_np] = (int16_t)sy;
  int zq = (int)(z2 * 70);
  wv_sz[wv_np] = (int8_t)(zq > 100 ? 100 : zq < -100 ? -100 : zq);
  int h = (int)(wv_hueA - Y * wv_hueB);                   // gradient follows the form
  wv_hue[wv_np] = (uint8_t)(h < 0 ? 0 : h >= WV_HUES ? WV_HUES - 1 : h);
  wv_np++;
}

// ─── Smooth (anti-aliased) lines ──────────────────────────────────────
// A plain line jumps a whole pixel at a time, so slow motion "crawls".
// This draws each line across TWO pixels per step, splitting the
// brightness between them by how close the true line is to each one
// (Xiaolin Wu's method). Our 8 brightness steps per color make that work.
// Pixels only ever get BRIGHTER ("max blend"), so crossing strands and
// front/back never punch dark holes in each other.
static uint8_t* wv_buf;

// base = 1 + hue * 8 (the darkest color of this hue), lvl = 1–7
static inline void wv_plotMax(int x, int y, int base, int lvl) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || lvl <= 0) return;
  uint8_t* p = wv_buf + y * W + x;
  uint8_t v = *p;
  // Current brightness of this pixel (0 if black; anything else we treat as dim)
  int cur = (uint8_t)(v - 1) < WV_HUES * WV_LEVELS ? ((v - 1) & (WV_LEVELS - 1)) : -1;
  if (lvl > cur) {
    *p = base + lvl;
    if (x < wv_rowMin[y]) wv_rowMin[y] = x;  // remember this row has light here
    if (x > wv_rowMax[y]) wv_rowMax[y] = x;
  }
}

// Coordinates are in 1/16 pixel. lvl = brightness 0–7.
// glow 1 or 2 adds a dimmer halo 1 (or 2) pixels either side, drawn in
// the same pass (much cheaper than drawing extra lines).
static void wv_lineAA(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int hue, int lvl, int glow) {
  if (lvl <= 0) return;
  if (lvl > WV_LEVELS - 1) lvl = WV_LEVELS - 1;
  if (hue < 0) hue = 0;
  if (hue > WV_HUES - 1) hue = WV_HUES - 1;
  hue = 1 + hue * WV_LEVELS;                       // from here on: base color index
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) { int32_t t = x0; x0 = y0; y0 = t; t = x1; x1 = y1; y1 = t; }
  if (x0 > x1) { int32_t t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
  int32_t dx = x1 - x0, dy = y1 - y0;
  int limit = steep ? H : W;                       // the axis we step along
  int xs = (x0 + 8) >> 4, xe = (x1 + 8) >> 4;
  int h1 = (glow > 0 && lvl >= 4) ? lvl - 4 + (glow > 1 ? 1 : 0) : 0;   // halo brightness
  int h2 = (glow > 1 && lvl >= 5) ? lvl - 5 : 0;                        // outer halo
  // Slope in 16.16. 32-bit maths so the chip's hardware divider does it.
  int32_t grad = (dx == 0) ? 0 : ((dy * 4096) / dx) * 16;
  if (xs < 0) xs = 0;                              // clip to the screen
  if (xe > limit - 1) xe = limit - 1;
  if (xs > xe) return;
  // y (16.16) where the line crosses the first pixel column (centre of pixel)
  int32_t yq = y0 * 4096 + (int32_t)(((int64_t)(xs * 16 + 8 - x0) * grad) / 16) - 32768;
  for (int x = xs; x <= xe; x++) {
    int yi = yq >> 16;
    int frac = (yq >> 8) & 255;                    // how far toward the next pixel
    int l1 = (lvl * (255 - frac) + 128) >> 8;
    int l2 = (lvl * frac + 128) >> 8;
    if (steep) {
      wv_plotMax(yi, x, hue, l1); wv_plotMax(yi + 1, x, hue, l2);
      if (h1) { wv_plotMax(yi - 1, x, hue, h1); wv_plotMax(yi + 2, x, hue, h1); }
      if (h2) { wv_plotMax(yi - 2, x, hue, h2); wv_plotMax(yi + 3, x, hue, h2); }
    } else {
      wv_plotMax(x, yi, hue, l1); wv_plotMax(x, yi + 1, hue, l2);
      if (h1) { wv_plotMax(x, yi - 1, hue, h1); wv_plotMax(x, yi + 2, hue, h1); }
      if (h2) { wv_plotMax(x, yi - 2, hue, h2); wv_plotMax(x, yi + 3, hue, h2); }
    }
    yq += grad;
  }
}

// One piece of strand, with its glow
static inline void wv_segmentAA(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int hue, int lvl, int glow) {
  wv_lineAA(x0, y0, x1, y1, hue, lvl, glow);
}

// ─── Automatic quality ────────────────────────────────────────────────
// The program times its own frames. If drawing gets too slow for smooth
// motion, it steps quality down; when there's time to spare, back up.
//   0 = everything   1 = no curve smoothing   2 = + less glow   3 = + fewer points
static int wv_quality = 0;

static void wv_adaptQuality(uint32_t frameUs) {
  static float avg = 0;
  static uint32_t lastChange = 0;
  avg += (frameUs - avg) * 0.1f;
  uint32_t now = millis();
  if (now - lastChange < 500) return;          // change at most twice a second
  if (avg > 13000 && wv_quality < 3) { wv_quality++; lastChange = now; }        // too slow
  else if (avg < 7000 && wv_quality > 0) { wv_quality--; lastChange = now; }    // room to spare
}

// Draw all strands.
// Long segments (e.g. when zoomed in) are split into short pieces that
// follow a smooth curve through the neighbouring points (a Catmull-Rom
// spline), so strands stay round instead of turning into polygons.
//   weave = over/under strength (0 = off): stretches of each strand dip
//           darker, alternating between neighbouring strands, so they
//           look woven over and under each other.
static void wv_drawStrands(int glow, int weave) {
  wv_buf = display.getBuffer();
  if (wv_quality >= 2 && glow > 0) glow--;     // saving time: smaller halo
  wv_strStart[wv_ns] = wv_np;            // end marker
  const int32_t M = 4 * 16;              // off-screen margin (1/16 px)
  for (int st = 0; st < wv_ns; st++) {
    int from = wv_strStart[st], to = wv_strStart[st + 1];
    int len = to - from;
    for (int i = 0; i < len - 1; i++) {
      int a = from + i, b = a + 1;
      int32_t x0 = wv_sx[a], y0 = wv_sy[a], x1 = wv_sx[b], y1 = wv_sy[b];
      // Skip segments completely off screen (matters when zoomed in)
      if ((x0 < -M && x1 < -M) || (x0 > W * 16 + M && x1 > W * 16 + M) ||
          (y0 < -M && y1 < -M) || (y0 > H * 16 + M && y1 > H * 16 + M)) continue;
      int zm = (wv_sz[a] + wv_sz[b]) / 2;
      int hue = (wv_hue[a] + wv_hue[b]) / 2;
      int lvl = 7 - (zm + 100) * 5 / 200;            // front 7 … back 2
      if (weave && ((((i * 8) / (len > 1 ? len : 1)) + st) & 1)) lvl -= weave;  // "under" stretch
      if (lvl < 1) lvl = 1;

      int span = max(abs(x1 - x0), abs(y1 - y0)) >> 4;    // length in pixels
      if (span <= 10 || wv_quality >= 1) {                 // short (or saving time): straight
        wv_segmentAA(x0, y0, x1, y1, hue, lvl, glow);
        continue;
      }
      // Neighbours before and after (repeat the end point at strand ends)
      int pa = (i > 0) ? a - 1 : a;
      int pb = (i < len - 2) ? b + 1 : b;
      float P0x = wv_sx[pa], P0y = wv_sy[pa], P3x = wv_sx[pb], P3y = wv_sy[pb];
      float P1x = x0, P1y = y0, P2x = x1, P2y = y1;
      int pieces = span / 8 + 1;
      if (pieces > 8) pieces = 8;
      int32_t qx = x0, qy = y0;
      for (int k = 1; k <= pieces; k++) {
        float t = (float)k / pieces, t2 = t * t, t3 = t2 * t;
        float nx = 0.5f * (2 * P1x + (-P0x + P2x) * t + (2 * P0x - 5 * P1x + 4 * P2x - P3x) * t2 + (-P0x + 3 * P1x - 3 * P2x + P3x) * t3);
        float ny = 0.5f * (2 * P1y + (-P0y + P2y) * t + (2 * P0y - 5 * P1y + 4 * P2y - P3y) * t2 + (-P0y + 3 * P1y - 3 * P2y + P3y) * t3);
        int32_t rx = (int32_t)nx, ry = (int32_t)ny;
        wv_segmentAA(qx, qy, rx, ry, hue, lvl, glow);
        qx = rx; qy = ry;
      }
    }
  }
}

// ─── The forms ────────────────────────────────────────────────────────
// Each builds `strands` strands of `len` points, in a unit-sized space.
static float wv_t = 0;               // animation time

static float wv_strandMul = 1.0f;   // inner layers use fewer strands
static float wv_detail = 1.0f;      // more points per strand when zoomed in

// Decide how many points each strand gets. Also trims the strand count
// (for inner layers, and if needed) so every strand has at least 24
// points — otherwise curves turn jagged.
static int wv_segs(int& strands, int want) {
  strands = (int)(strands * wv_strandMul + 0.5f);
  if (strands < 1) strands = 1;
  if (wv_budget / strands < 24) strands = wv_budget / 24;
  if (strands < 1) strands = 1;
  want = (int)(want * wv_detail);
  int len = wv_budget / strands;
  return (len < want) ? len : want;
}

static void wv_weaveSphere(int& strands, int& len) {
  strands   = wv_pot(4, 4, 40);                  // p4: strands
  float tw  = wv_potf(5, 0, 150) / 100.0f;        // p5: twist
  int waves = wv_pot(6, 1, 4);                   // p6: waves along each strand
  float brk = wv_potf(7, -20, 20);                // p7: breathing (center and left = none)
  float br  = (brk > 0 ? brk : 0) / 100.0f;
  len = wv_segs(strands, 48);
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float phi0 = TWO_PI * s / strands;
    for (int i = 0; i < len; i++) {
      float th = -HALF_PI + PI * i / (len - 1);
      // Two twisting waves on top of each other = a more intricate weave
      float phi = phi0 + tw * wsin(2 * th * waves + wv_t)
                       + tw * 0.4f * wsin(5 * th - wv_t * 1.7f + phi0 * 2);
      float r = 1.0f + br * wsin(3 * th + wv_t * 1.3f);
      float c = wcos(th) * r;
      wv_addPoint(c * wcos(phi), wsin(th) * r, c * wsin(phi));
    }
  }
}

static void wv_torusKnot(int& strands, int& len) {
  static const int PQ[6][2] = {{2, 3}, {3, 4}, {2, 5}, {3, 5}, {3, 7}, {5, 8}};
  strands  = wv_pot(4, 1, 10);                   // p4: strands in the braid
  int kind = wv_pot(5, 0, 5);                    // p5: knot type
  float tube = wv_potf(6, 5, 60) / 100.0f;        // p6: how far apart strands are
  float wob = wv_potf(7, 0, 40) / 100.0f;         // p7: wobble
  int p = PQ[kind][0], q = PQ[kind][1];
  len = wv_segs(strands, 180);
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float off = TWO_PI * s / strands;
    for (int i = 0; i < len; i++) {
      float u = TWO_PI * i / (len - 1);
      float r = 2.0f + wcos(q * u + off * tube * 3 + wv_t) * (1.0f + wob * wsin(u * 5 + wv_t));
      float x = r * wcos(p * u), y = r * wsin(p * u), z = -wsin(q * u + off * tube * 3 + wv_t);
      wv_addPoint(x / 3.0f, z * 0.6f, y / 3.0f);
    }
  }
}

static void wv_mobius(int& strands, int& len) {
  strands   = wv_pot(4, 3, 24);                  // p4: strands across the band
  float wid = wv_potf(5, 20, 90) / 100.0f;        // p5: band width
  int twists = 1 + 2 * wv_pot(6, 0, 2);          // p6: half-twists (1, 3, 5)
  float wav = wv_potf(7, 0, 30) / 100.0f;         // p7: ripple
  len = wv_segs(strands, 72);
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float w = -1.0f + 2.0f * s / (strands - 1 > 0 ? strands - 1 : 1);
    w *= wid * (1.0f + wav * wsin(wv_t * 2 + s));
    for (int i = 0; i < len; i++) {
      float u = TWO_PI * i / (len - 1);
      float h = u * twists / 2.0f + wv_t * 0.5f;
      float r = 1.0f + 0.5f * w * wcos(h);
      wv_addPoint(r * wcos(u) * 0.95f, 0.6f * w * wsin(h), r * wsin(u) * 0.95f);
    }
  }
}

static void wv_lissajousOrb(int& strands, int& len) {
  static const int R[5][3] = {{1, 2, 3}, {2, 3, 4}, {3, 4, 5}, {1, 3, 5}, {2, 5, 7}};
  strands   = wv_pot(4, 1, 8);                   // p4: number of curves
  int kind  = wv_pot(5, 0, 4);                   // p5: curve family
  float sp  = wv_potf(6, 0, 100) / 100.0f;        // p6: spread between curves
  float lenK = wv_potf(7, 30, 100) / 100.0f;      // p7: how much of each loop to draw
  len = wv_segs(strands, 200);
  int a = R[kind][0], b = R[kind][1], c = R[kind][2];
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float ph = s * sp * 1.3f;
    for (int i = 0; i < len; i++) {
      float u = TWO_PI * lenK * i / (len - 1) + wv_t * 0.3f;
      wv_addPoint(0.7f * wsin(a * u + ph + wv_t), 0.7f * wsin(b * u + ph * 0.5f), 0.7f * wsin(c * u + wv_t * 0.7f));
    }
  }
}

static void wv_helixBraid(int& strands, int& len) {
  strands   = wv_pot(4, 2, 20);                  // p4: strands
  float turns = wv_potf(5, 5, 40) / 10.0f;        // p5: turns top to bottom
  float bulge = wv_potf(6, 0, 60) / 100.0f;       // p6: bulge
  float rad   = wv_potf(7, 20, 70) / 100.0f;      // p7: radius
  len = wv_segs(strands, 64);
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float a0 = TWO_PI * s / strands;
    float dir = (s & 1) ? 1.0f : -1.0f;          // alternate directions = braid
    for (int i = 0; i < len; i++) {
      float y = -1.0f + 2.0f * i / (len - 1);
      float r = rad * (1.0f + bulge * wsin(y * 3 + wv_t));
      float a = a0 + dir * turns * PI * y + wv_t;
      wv_addPoint(r * wcos(a), y, r * wsin(a));
    }
  }
}

static void wv_wobbleRings(int& strands, int& len) {
  strands   = wv_pot(4, 3, 30);                  // p4: rings
  float amp = wv_potf(5, 0, 40) / 100.0f;         // p5: wobble size
  int lobes = wv_pot(6, 2, 8);                   // p6: lobes per ring
  float ph  = wv_potf(7, 0, 100) / 100.0f;        // p7: twist between rings
  len = wv_segs(strands, 48);
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float th = -HALF_PI * 0.9f + PI * 0.9f * s / (strands - 1);
    for (int i = 0; i < len; i++) {
      float u = TWO_PI * i / (len - 1);
      float r = wcos(th) * (1.0f + amp * wsin(lobes * u + wv_t * 2 + s * ph * 2));
      wv_addPoint(r * wcos(u), wsin(th), r * wsin(u));
    }
  }
}

static void wv_lattice(int& strands, int& len) {
  int n     = wv_pot(4, 2, 20);                  // p4: spirals in each direction
  float k   = wv_potf(5, 5, 40) / 10.0f;          // p5: how far they wind
  float br  = wv_potf(6, 0, 30) / 100.0f;         // p6: breathing
  float sq  = wv_potf(7, 50, 100) / 100.0f;       // p7: squash (flatten the poles)
  strands = n * 2;
  len = wv_segs(strands, 48);
  n = strands / 2;                               // keep both directions balanced
  if (n < 1) n = 1;
  strands = n * 2;
  for (int s = 0; s < strands; s++) {
    wv_beginStrand();
    float dir = (s < n) ? 1.0f : -1.0f;          // half go one way, half the other
    float phi0 = TWO_PI * (s % n) / n;
    for (int i = 0; i < len; i++) {
      float th = -HALF_PI * 0.95f + PI * 0.95f * i / (len - 1);
      float phi = phi0 + dir * (k * th + 0.3f * wsin(3 * th + wv_t)) + wv_t * 0.5f;
      float r = 1.0f + br * wsin(wv_t * 1.5f + th * 2);
      float c = wcos(th) * r;
      wv_addPoint(c * wcos(phi), wsin(th) * r * sq, c * wsin(phi));
    }
  }
}

// ─── Public interface ─────────────────────────────────────────────────

const char* prog_weave_name() { return "WEAVE"; }

const char* prog_weave_character() {
  return "Glowing 3D woven wireframes: sphere, knot, mobius, braid";
}

static const char* const wv_presetNames[] = {
  "Weave Sphere", "Torus Knot", "Mobius", "Lissajous Orb",
  "Helix Braid", "Wobble Rings", "Lattice"
};
#define WV_NUM_PRESETS 7

const char* prog_weave_presetName(int preset) {
  if (preset >= 0 && preset < WV_NUM_PRESETS) return wv_presetNames[preset];
  return NULL;
}

static const char* const wv_globalLabels[4] = { "Colors", "Speed", "Trail", "Glow" };
static const char* const wv_camLabels[8] = {
  "Spin", "Tilt", "Zoom", "Pan X", "Pan Y", "Pulse", "Layers", "Over/Under"
};

static const char* const wv_potLabels[WV_NUM_PRESETS][4] = {
  {"Strands", "Twist",  "Waves",   "Breathe"},  // Weave Sphere
  {"Strands", "Knot",   "Spacing", "Wobble"},   // Torus Knot
  {"Strands", "Width",  "Twists",  "Ripple"},   // Mobius
  {"Curves",  "Family", "Spread",  "Length"},   // Lissajous Orb
  {"Strands", "Turns",  "Bulge",   "Radius"},   // Helix Braid
  {"Rings",   "Wobble", "Lobes",   "Twist"},    // Wobble Rings
  {"Spirals", "Wind",   "Breathe", "Squash"},   // Lattice
};

const char* prog_weave_potLabel(int preset, int pot) {
  if (pot >= 0 && pot < 4) return wv_globalLabels[pot];
  if (pot >= 8 && pot <= 15) return wv_camLabels[pot - 8];
  if (pot < 4 || pot > 7) return "";
  if (preset < 0 || preset >= WV_NUM_PRESETS) return "";
  return wv_potLabels[preset][pot - 4];
}

uint8_t prog_weave_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;   // we fade the old frame ourselves (the trail)
}

void prog_weave_init() {
  wv_palScheme = -1;        // force our colors to be rebuilt
  display.fillScreen(0);
  wv_spansFull();
}

void prog_weave_draw(int preset) {
  uint32_t frameStart = micros();
  wv_init();

  wv_buildPersp();

  // Own clock (this program owns p1, so globalTime isn't updated for us).
  // The frame time is smoothed too, so one slow frame doesn't cause a hitch.
  static unsigned long lastMs = 0;
  static float dtS = 0.016f;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  dtS += (dt - dtS) * 0.15f;
  dt = dtS;
  wv_smoothKnobs(dt);

  // Global knobs
  int scheme  = wv_pot(0, 0, 5);
  float speed = powf(4.0f, (wv_sm[1] / 1023.0f) * 2.0f - 1.0f);   // 0.25x … 4x
  float trail = 4.0f * powf(75.0f, wv_sm[2] / 1023.0f);           // fade steps/sec
  int glow    = wv_pot(3, 0, 2);
  wv_buildPalette(scheme, glow);

  // Camera
  // Each layer keeps its own angle: outer turns one way, the middle
  // layer the other way (faster), the inner one a bit faster again.
  static float layerRot[3] = {0, 0, 0};
  static const float LSPIN[3] = {1.0f, -1.3f, 1.4f};
  float spin = wv_potf(8, -100, 100) / 100.0f;                    // p8: spin
  if (spin > -0.08f && spin < 0.08f) spin = 0;                   // dead zone at center
  for (int L = 0; L < 3; L++) {
    layerRot[L] += spin * 1.5f * speed * dt * LSPIN[L];
    if (layerRot[L] > TWO_PI) layerRot[L] -= TWO_PI;
    if (layerRot[L] < 0) layerRot[L] += TWO_PI;
  }
  float tilt = wv_potf(9, -60, 60) * (PI / 180.0f);               // p9: tilt
  wv_ct = cosf(tilt); wv_st = sinf(tilt);

  // p10 zoom: center = 1x, left = out to 0.4x, right = in to 6x
  float k = wv_sm[10] / 1023.0f;
  float zoom = (k < 0.5f) ? powf(0.4f, (0.5f - k) * 2.0f) : powf(6.0f, (k - 0.5f) * 2.0f);
  static float pulseT = 0;
  pulseT += speed * dt * 0.6f;
  if (pulseT > TWO_PI) pulseT -= TWO_PI;
  float pulse = wv_potf(13, 0, 100) / 100.0f;                     // p13: pulse
  zoom *= 1.0f + pulse * 1.5f * (0.5f - 0.5f * cosf(pulseT));    // breathes in, never smaller
  wv_size = 90.0f * zoom;
  wv_detail = (zoom < 1.0f) ? 1.0f : (zoom > 3.0f ? 3.0f : zoom);
  // Pan moves the view by up to one form-width either way, scaled by zoom
  wv_panX = -wv_potf(11, -100, 100) / 100.0f * wv_size;           // p11: pan X
  wv_panY = wv_potf(12, -100, 100) / 100.0f * wv_size;            // p12: pan Y

  int layers = wv_pot(14, 1, 3);                                 // p14: layers
  int weave  = wv_pot(15, 0, 4);                                 // p15: over/under

  wv_t += speed * dt;
  if (wv_t > 1000.0f * TWO_PI) wv_t -= 1000.0f * TWO_PI;

  // New preset? Start from black
  static int lastPreset = -1;
  if (preset != lastPreset) { lastPreset = preset; display.fillScreen(0); wv_spansFull(); }

  // Other things can draw on the screen (info overlay, hints, the FX
  // layer). We can't track those, so fade the whole screen while they're
  // active, and do a full sweep every couple of seconds just in case.
  static int sweep = 0;
  if (k13Holding || hintsEnabled || g_fxActive || ++sweep >= 120) { wv_spansFull(); sweep = 0; }

  // Trail fade
  static float fadeAcc = 0;
  fadeAcc += trail * dt;
  int n = (int)fadeAcc;
  if (n > 0) { fadeAcc -= n; wv_fade(n > WV_LEVELS ? WV_LEVELS : n); }

  // Build this frame's strands (one pass per layer), then draw them.
  // Inner layers are smaller, turn the other way, and run a little out of step.
  wv_np = 0;
  wv_ns = 0;
  static const float LSCALE[3] = {1.0f, 0.62f, 0.38f};
  float baseT = wv_t;
  for (int L = 0; L < layers; L++) {
    wv_ca = wcos(layerRot[L]); wv_sa = wsin(layerRot[L]);
    // Share of the points for this layer (outer layers get more)
    static const float SHARE[3][3] = {{1.0f, 0, 0}, {0.62f, 0.38f, 0}, {0.5f, 0.3f, 0.2f}};
    static const float SMUL[3] = {1.0f, 0.7f, 0.5f};
    wv_scale = LSCALE[L];
    wv_budget = (int)(WV_MAXP * SHARE[layers - 1][L] * (wv_quality >= 3 ? 0.6f : 1.0f));
    wv_strandMul = SMUL[L];
    wv_setMatrix();
    wv_t = baseT + L * 1.7f;
    int strands = 0, len = 0;
    switch (preset) {
      case 0: wv_weaveSphere(strands, len);  break;
      case 1: wv_torusKnot(strands, len);    break;
      case 2: wv_mobius(strands, len);       break;
      case 3: wv_lissajousOrb(strands, len); break;
      case 4: wv_helixBraid(strands, len);   break;
      case 5: wv_wobbleRings(strands, len);  break;
      case 6: wv_lattice(strands, len);      break;
      default:
        display.fillScreen(0);
        display.setTextColor(255);
        display.setTextSize(1);
        display.setCursor(100, 116);
        display.print("empty preset");
        wv_t = baseT;
        return;
    }
  }
  wv_t = baseT;
  wv_scale = 1.0f;
  wv_strandMul = 1.0f;
  wv_drawStrands(glow, weave);
  wv_adaptQuality(micros() - frameStart);
}

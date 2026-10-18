// =====================================================================
// vg5c.h — the Set 5 clay-block engine (by Dewey)
//
// After ducky3d's "Cycles": a field of soft, matte shapes packed on an
// isometric grid — cubes, cylinders, square tubes and round tubes in two
// colours of clay, white ones flecked with red — with chrome balls.
// Used by BLOCKS, MARBLES, MORPH and PRESS (Set 5, slots 8-11).
//
// One renderer draws every shape. In iso, a cube's top is a rhombus and a
// cylinder's top is an ellipse; both are "superellipses" |u|^n + |v|^n ≤ 1
// with a different n, so a table of 8 profiles from round (0) to square (7)
// lets a shape morph smoothly from one to the other. A shape can also have
// a hole (a tube). Bodies are shaded column by column like soft clay; tops
// get a gentle gradient and, on white clay, red speckles.
//
// Palette:
//   0-63    clay A (usually white)      64-127  clay B (usually red)
//   128-191 chrome (dark → bright)       192-207 the gaps between shapes
// =====================================================================
#ifndef VG5C_H
#define VG5C_H
#include "vg3.h"
#include "vg5fx.h"

struct C5Look { uint8_t aLo[3], aHi[3], bLo[3], bHi[3], speck[3], gap[3]; };

static void c5_palette(const C5Look& L) {
  for (int i = 0; i < 64; i++) {
    int k = i * 255 / 63;
    display.setColor((uint8_t)i, v3_mix8(L.aLo[0], L.aHi[0], k), v3_mix8(L.aLo[1], L.aHi[1], k), v3_mix8(L.aLo[2], L.aHi[2], k));
    display.setColor((uint8_t)(64 + i), v3_mix8(L.bLo[0], L.bHi[0], k), v3_mix8(L.bLo[1], L.bHi[1], k), v3_mix8(L.bLo[2], L.bHi[2], k));
    // chrome: dark steel → white, with a hint of clay B reflected in the middle
    int c = i * 4, rr = c, gg = c, bb = v3_clamp(c + 12, 0, 255);
    if (i > 18 && i < 34) { int w = 60 - v3_abs(i - 26) * 7; rr = v3_mix8(rr, L.bHi[0], w); gg = v3_mix8(gg, L.bHi[1], w); bb = v3_mix8(bb, L.bHi[2], w); }
    display.setColor((uint8_t)(128 + i), v3_clamp(rr, 0, 255), v3_clamp(gg, 0, 255), v3_clamp(bb, 0, 255));
  }
  for (int i = 0; i < 16; i++) display.setColor((uint8_t)(192 + i), L.gap[0] * i / 15, L.gap[1] * i / 15, L.gap[2] * i / 15);
  display.setColor(208, L.speck[0], L.speck[1], L.speck[2]);
}
#define C5_SPECK 208

// ─── Profiles: half-widths of the top shape, row by row, for 8 levels of round → square ──
#define C5_MAXS 40
static int c5_S = 0;                                 // cell half-width in px (a shape is 2S wide, S tall on top)
static int8_t c5_w[8][C5_MAXS / 2 + 2];              // half-width at row offset dy (0 … S/2) from the top's centre; −1 = the row is empty
static uint8_t c5_yb[8][C5_MAXS + 2];                // lowest top-row (dy) at column offset |dx| (0 … S): where the body starts
static void c5_profiles(int S) {
  if (S == c5_S) return;
  if (S > C5_MAXS) S = C5_MAXS;
  c5_S = S;
  static const float ex[8] = {2.0f, 2.4f, 3.0f, 4.0f, 5.5f, 8.0f, 14.0f, 40.0f};
  int hh = S / 2;
  for (int L = 0; L < 8; L++) {
    // in the shape's own axes the top is |u|^n + |v|^n ≤ 1; on screen x = (u − v)·S, y = (u + v)·S/2
    for (int dy = 0; dy <= hh; dy++) {
      float yy = hh > 0 ? (float)dy / hh : 0;                     // 0 … 1
      int best = -1;
      for (int dx = 0; dx <= S; dx++) {
        float xx = (float)dx / S;
        // on screen the footprint square is the rhombus |x| + |y| ≤ 1; in its own axes that is max(|x+y|, |y−x|) ≤ 1,
        // and the superellipse of power n inside the same square is |x+y|^n + |y−x|^n ≤ 1 (n = 2: the inscribed circle)
        bool in = L == 7 ? (xx + yy <= 1.0f) : (powf(xx + yy, ex[L]) + powf(fabsf(yy - xx), ex[L]) <= 1.0f);
        if (in) best = dx;
      }
      c5_w[L][dy] = (uint8_t)best;
    }
    for (int dx = 0; dx <= S; dx++) {
      int b = -1;
      for (int dy = hh; dy >= 0; dy--) if (c5_w[L][dy] >= dx) { b = dy; break; }
      c5_yb[L][dx] = (uint8_t)(b < 0 ? 0 : b);
    }
  }
}

// ─── Drawing one shape ────────────────────────────────────────────────
// (cx, cy): screen centre of the top; h: body height (px, down to the ground); L: 0 round … 7 square;
// hole: 0 = solid, else the hole's size /256 of the shape; clay: 0 = A, 1 = B; seed: for the speckles
static uint8_t* c5_buf;
static int c5_light = 1;                              // 1 = light from the left, −1 from the right
static int c5_speckles = 40;                          // speckles per 1000 top pixels (on clay A)
static uint8_t c5_rnd[256];                           // a fixed table of random bytes, for speckles
static bool c5_rndOk = false;
static void c5_shape(int cx, int cy, int h, int L, int hole, int clay, uint32_t seed, int scale256, int bodyLen = -1) {
  if (!c5_rndOk) { uint32_t x = 0x9E3779B9u; for (int i = 0; i < 256; i++) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; c5_rnd[i] = (uint8_t)x; } c5_rndOk = true; }
  int S = (c5_S * scale256) >> 8; if (S < 3) S = 3; if (S > C5_MAXS) S = C5_MAXS;
  if (cx + S < 0 || cx - S >= W) return;
  int hh = S / 2, base = clay ? 64 : 0;
  const int8_t* wtab = c5_w[L]; const uint8_t* ybt = c5_yb[L];
  int sc = scale256;
  int inv = (1 << 16) / (S + 1);                                   // 1 / (S+1), Q16
  int invSc = (256 << 16) / sc;                                    // 256 / scale, Q16: profile lookups without divides
  int invHh = hh > 0 ? ((c5_S / 2) << 16) / hh : 0;
  if (bodyLen < 0 || bodyLen > h) bodyLen = h;
  // body: column by column, from the bottom edge of the top down to the ground
  if (h > 0) {
    int xa = cx - S < 0 ? -cx : -S, xb = cx + S > W - 1 ? W - 1 - cx : S;
    for (int dx = xa; dx <= xb; dx++) {
      int adx = dx < 0 ? -dx : dx;
      int pdx = (adx * invSc) >> 16; if (pdx > c5_S) pdx = c5_S;
      int yb = (ybt[pdx] * sc) >> 8;
      int y0 = cy + yb, y1 = y0 + bodyLen, ground = y0 + h;
      int top = y0 < 0 ? 0 : y0, bot = y1 > H - 1 ? H - 1 : y1;
      if (top > bot) continue;
      // shading across: a cylinder rolls smoothly, a cube has two flat faces; blend by L
      int t = (dx * 256 * inv >> 16) * c5_light;                    // −256 (lit side) … 256
      int cyl = 44 - (t * 22 >> 8) - ((t * t >> 8) * 10 >> 8);
      int cube = dx * c5_light < 0 ? 44 : 30;
      int tone = (cyl * (7 - L) + cube * L) / 7;
      uint8_t* p = c5_buf + top * W + cx + dx;
      int y = top;
      for (; y <= bot && y < y0 + 3; y++, p += W) *p = (uint8_t)(base + tone + 3 - (y - y0));   // a bright lip under the rim
      int fadeAt = ground - 24;
      uint8_t c = (uint8_t)(base + tone);
      for (; y <= bot && y < fadeAt; y++, p += W) *p = c;
      int fstep = (14 << 12) / 24, f = (y - fadeAt) * fstep;        // darker towards the ground
      for (; y <= bot; y++, p += W, f += fstep) { int tt = tone - (f >> 12); *p = (uint8_t)(base + (tt < 2 ? 2 : tt)); }
    }
  }
  // top: row by row; a ramp across the row, a little darker towards the front
  uint8_t ramp[2 * C5_MAXS + 1];
  for (int dx = -S; dx <= S; dx++) { int tn = 58 - ((dx * c5_light * 4 * inv) >> 16); ramp[dx + S] = (uint8_t)(base + (tn > 60 ? 60 : (tn < 4 ? 4 : tn))); }   // kept clear of the ends: the row shading adds ±3
  int hl = (hole * S) >> 8, hhl = (hole * hh) >> 8;
  bool speck = !clay && c5_speckles > 0;
  int spThr = c5_speckles * 256 / 1000;
  for (int dy = -hh; dy <= hh; dy++) {
    int y = cy + dy; if ((unsigned)y >= H) continue;
    int ady = dy < 0 ? -dy : dy;
    int pdy = (ady * invHh) >> 16; if (pdy > c5_S / 2) pdy = c5_S / 2;
    if (wtab[pdy] < 0) continue;                                    // above or below a round top: nothing on this row
    int w = (wtab[pdy] * sc) >> 8;
    int rowAdj = v3_clamp(-((dy * 3 * inv) >> 16) * 2, -3, 3);
    int iw = -1;
    if (hole > 0 && ady <= hhl && hhl > 0) { int q = (ady * (c5_S / 2)) / hhl; if (q > c5_S / 2) q = c5_S / 2; iw = wtab[q] < 0 ? -1 : (wtab[q] * sc * hole) >> 16; }
    uint8_t* row = c5_buf + y * W;
    int x0 = cx - w < 0 ? 0 : cx - w, x1 = cx + w > W - 1 ? W - 1 : cx + w;
    int hx0 = iw >= 0 ? cx - iw : 1 << 20, hx1 = iw >= 0 ? cx + iw : -(1 << 20);
    uint32_t r = (seed * 2654435761u + (uint32_t)(dy + 64) * 40503u) >> 8;
    for (int x = x0; x <= x1; x++) {
      if (x >= hx0 && x <= hx1) {                                   // the hole: dark, the far wall a little lit lower down
        int tone = 6 + (dy + hhl) * 10 / (2 * hhl + 1);
        int e = hx1 < x1 ? hx1 : x1;
        memset(row + x, base + tone, e - x + 1); x = e; continue;
      }
      if (speck && c5_rnd[(r + (uint32_t)(x - cx) * 167u) & 255] < spThr && c5_rnd[(r * 7u + (uint32_t)x) & 255] < 96) { row[x] = C5_SPECK; continue; }
      row[x] = (uint8_t)(ramp[x - cx + S] + rowAdj);
    }
  }
  // a soft line where the top meets the body (the lower rim), so shapes read against each other
  int xa = cx - S + 1 < 0 ? -cx : -S + 1, xb = cx + S - 1 > W - 1 ? W - 1 - cx : S - 1;
  for (int dx = xa; dx <= xb; dx++) {
    int adx = dx < 0 ? -dx : dx, pdx = (adx * invSc) >> 16; if (pdx > c5_S) pdx = c5_S;
    int y = cy + ((ybt[pdx] * sc) >> 8);
    if ((unsigned)y < H) { uint8_t* p = c5_buf + y * W + cx + dx; int t = (*p & 63); *p = (uint8_t)(base + (t > 6 ? t - 6 : 0)); }
  }
}
// a chrome ball: sky above, the clay floor below, a dark horizon, a highlight
static void c5_ball(int cx, int cy, int r) {
  if (r < 2) return;
  for (int dy = -r; dy <= r; dy++) {
    int y = cy + dy; if ((unsigned)y >= H) continue;
    int w = (int)v3_isqrt((uint32_t)(r * r - dy * dy));
    for (int dx = -w; dx <= w; dx++) {
      int x = cx + dx; if ((unsigned)x >= W) continue;
      int ny = dy * 64 / r, nx = dx * 64 / r;                    // −64 … 64
      int t;
      if (ny < -8) t = 50 + (-ny - 8) / 6 - (nx * c5_light > 20 ? 6 : 0);   // the sky
      else if (ny < 6) t = 8 + (ny + 8);                            // the horizon: dark
      else t = 26 + ((ny * 7) >> 6) + ((nx * c5_light) >> 4);       // the floor, red and white smeared
      int d2 = (dx + r / 3) * (dx + r / 3) + (dy + r / 3) * (dy + r / 3);
      if (d2 < (r * r) / 16) t = 63;                               // the highlight
      if (dx * dx + dy * dy > (r - 1) * (r - 1)) t -= 10;           // a dark rim
      c5_buf[y * W + x] = (uint8_t)(128 + v3_clamp(t, 0, 63));
    }
  }
}
// the soft shadow a ball casts on the top beneath it
static void c5_ballShadow(int cx, int cy, int r) {
  for (int dy = -r / 2; dy <= r / 2; dy++) {
    int y = cy + dy; if ((unsigned)y >= H) continue;
    int w = (int)v3_isqrt((uint32_t)(r * r / 4 - dy * dy)) * 2;
    for (int dx = -w; dx <= w; dx++) {
      int x = cx + dx; if ((unsigned)x >= W) continue;
      uint8_t* p = c5_buf + y * W + x;
      if (*p >= 128) continue;
      int t = (*p & 63) - 14; *p = (uint8_t)((*p & 64) | (t < 0 ? 0 : t));
    }
  }
}

// ─── The whole field ──────────────────────────────────────────────────
// Walks the iso grid back to front (diagonal d = i + j, then k = i − j across). For each cell the
// program says its height and shape; `between` (optional) is called after each diagonal, so balls
// can be drawn in their proper place. (i, j) are world cells: offset by (oi, oj) to pan.
struct C5Cell { int h, L, hole, clay, scale; uint32_t seed; bool skip; };
typedef void (*c5_cellFn)(int i, int j, void* c);   // c is a C5Cell* (void* keeps Arduino's auto-prototypes happy)
typedef void (*c5_diagFn)(int d);
static int c5_top = 0, c5_cx = 160;                  // screen position of cell (0, 0)'s top centre, before its height
static void c5_field(int hmax, c5_cellFn cell, c5_diagFn between) {
  int S = c5_S, hs = S / 2;
  int d0 = ((-c5_top) * 2) / S - 2, d1 = ((H + hmax + S - c5_top) * 2) / S + 1;
  int k0 = -(c5_cx + S) / S - 1, k1 = (W - c5_cx + S) / S + 1;
  for (int d = d0; d <= d1; d++) {
    int ka = k0 + (((k0 + d) & 1) ? 1 : 0);
    for (int k = ka; k <= k1; k += 2) {
      int i = (d + k) >> 1, j = (d - k) >> 1;
      C5Cell c; c.h = 0; c.L = 7; c.hole = 0; c.clay = 0; c.scale = 224; c.seed = ((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u); c.skip = false;
      cell(i, j, &c);
      if (c.skip) continue;
      int cx = c5_cx + k * S, cy = c5_top + d * hs - c.h;
      if (cy - S > H || cy + c.h + S < 0) continue;
      // the two cells in front hide everything below their tops: only draw the body that can show
      C5Cell f; int hn = 1 << 20;
      f.h = 0; f.skip = false; cell(i + 1, j, &f); if (!f.skip && f.h < hn) hn = f.h;
      f.h = 0; f.skip = false; cell(i, j + 1, &f); if (!f.skip && f.h < hn) hn = f.h;
      int body = c.h - hn + S; if (body < S / 2) body = S / 2;
      c5_shape(cx, cy, c.h, c.L, c.hole, c.clay, c.seed, c.scale, body);
    }
    if (between) between(d);
  }
}
// screen position of a point on the field: world cell coordinates (fi, fj in 1/256 cell) at height z
static inline void c5_toScreen(int32_t fi, int32_t fj, int z, int* sx, int* sy) {
  *sx = c5_cx + (int)(((fi - fj) * c5_S) >> 8);
  *sy = c5_top + (int)(((fi + fj) * (c5_S / 2)) >> 8) - z;
}
#endif

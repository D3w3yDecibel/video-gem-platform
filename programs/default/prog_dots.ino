// =====================================================================
// PROGRAM: DOTS — Vasarely's dot fields (by Dewey) — Set 7, slot 6
//
// A grid of dots, every one a different size: big where a hidden shape
// swells and small where it sinks, so a ball, a wave or a ripple seems to
// rise out of a flat sheet of dots. Vasarely built whole series this way
// ("Vega-Nor", "Supernovae"), and so does every newspaper photo — halftone.
// Here the hidden shapes move: a sphere rolls, waves cross, rings spread,
// a gradient turns, plasma boils; the dots can be squares, diamonds, rings
// or crosses, packed square or in hexagons.
//
// How it works: each frame the hidden shape is worked out once per dot
// (a few thousand dots, whole numbers), giving each dot its size. Then for
// every pixel: which dot am I in, and where inside it — both from column and
// row tables — and one more table gives the shade (with soft edges) and hue
// for that distance from the middle at that size.
//
// Presets: k0 Sphere · k1 Wave · k2 Gradient · k3 Ripple · k4 Hexagons · k5 Squares ·
//          k6 Diamonds · k7 Rings · k8 Plasma · k9 Crosses · k10 Breathe · k11 Tour
//   k12 — DROP: a ripple spreads out from the middle
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (dot spacing) · p3 Colour Flow · p4 Hue Spread
//   p5 Dot Size · p6 Contrast (how much sizes vary) · p7 Field · p8 Shape · p9 Field Speed · p10 Packing (square | hex)
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { DK_SPHERE, DK_WAVE, DK_GRADIENT, DK_RIPPLE, DK_TWIN, DK_PLASMA, DK_BREATHE, DK_NFIELD };
enum { DK_CIRCLE, DK_SQUARE, DK_DIAMOND, DK_RING, DK_CROSS, DK_NSHAPE };
// cell: dot spacing px · size: biggest dot (% of touching) · range: smallest dot (% of the biggest) ·
// speed: field speed · hex: hexagonal packing · invert: paper dots on ink · hue: colour by size (1) or by row (2)
struct DkLook { uint8_t field, shape, cell, size, range, speed, hex, invert, hue, pal, flow, spread, sat; };
static const DkLook dk_looks[11] = {
  // field         shape       cell size rng spd hex inv hue pal            flow spr sat
  { DK_SPHERE  , DK_CIRCLE , 12, 100,  8, 30, 0, 0, 0, O7P_BW      , 0,  50,   0 },   // Sphere
  { DK_WAVE    , DK_CIRCLE , 11,  95, 10, 40, 0, 0, 1, O7P_NEON    , 3,  90, 100 },   // Wave
  { DK_GRADIENT, DK_CIRCLE , 14, 100, 10, 25, 0, 0, 1, O7P_VEGA    , 3,  90, 100 },   // Gradient
  { DK_RIPPLE  , DK_CIRCLE , 10, 100,  5, 40, 0, 1, 1, O7P_KITAOKA , 3,  90, 100 },   // Ripple
  { DK_TWIN    , DK_CIRCLE , 12, 100, 10, 35, 1, 0, 1, O7P_AGAM    , 3, 100,  90 },   // Hexagons
  { DK_SPHERE  , DK_SQUARE , 13,  95, 10, 30, 0, 0, 1, O7P_RA      , 2,  70, 100 },   // Squares
  { DK_WAVE    , DK_DIAMOND, 12, 100,  5, 35, 0, 0, 1, O7P_SPECTRUM, 3,  90, 100 },   // Diamonds
  { DK_RIPPLE  , DK_RING   , 14, 100, 20, 35, 1, 0, 1, O7P_NEON    , 4, 100, 100 },   // Rings
  { DK_PLASMA  , DK_CIRCLE , 10, 100,  5, 35, 1, 0, 1, O7P_SPECTRUM, 4, 100, 100 },   // Plasma
  { DK_GRADIENT, DK_CROSS  , 14,  70, 10, 30, 0, 0, 1, O7P_ANUSZ   , 3,  90, 100 },   // Crosses
  { DK_BREATHE , DK_CIRCLE , 16,  90, 15, 25, 1, 0, 2, O7P_INK     , 0,  40, 100 },   // Breathe
};
static const char* const dk_names[12] = { "Sphere", "Wave", "Gradient", "Ripple", "Hexagons", "Squares", "Diamonds", "Rings", "Plasma", "Crosses", "Breathe", "Tour" };
static const char* const dk_lbl[6] = { "Dot Size", "Contrast", "Field", "Shape", "Field Speed", "Packing: Square|Hex" };
const char* prog_dots_name() { return "DOTS"; }
const char* prog_dots_character() { return "Vasarely's dot fields: a grid of dots swelling and shrinking over hidden shapes"; }
const char* prog_dots_presetName(int p) { return (p >= 0 && p < 12) ? dk_names[p] : NULL; }
const char* prog_dots_potLabel(int p, int pot) { (void)p; return o7_label(pot, dk_lbl); }
uint8_t prog_dots_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: dot sizes (one per dot, 0-31), the distance-in-a-dot table (32 × 32 positions → 0-63),
// the shade table (size × distance → colour, 32 × 64), the column tables (even and odd rows of dots)
#define DK_MAXD 3000
static uint8_t *dk_rad, *dk_dist, *dk_out, *dk_sq; static uint16_t *dk_colE, *dk_colO;
static void dk_mem() {
  dk_rad = v6_mem;                                   // 3000
  dk_dist = v6_mem + 3008;                           // 1024
  dk_out = v6_mem + 4032;                            // 2048
  dk_colE = (uint16_t*)(v6_mem + 6080); dk_colO = dk_colE + W;   // 1280
  dk_sq = v6_mem + 7360;                             // 1024: √(q · 64), q = squared distance / 64
}
static inline int dk_r(int32_t q) { q >>= 6; return dk_sq[q > 1023 ? 1023 : (q < 0 ? 0 : q)]; }
static float dk_size, dk_range, dk_speedK, dk_hexK, dk_t = 0, dk_drop = -1;
static int dk_shapeMade = -1;
void prog_dots_init() { dk_mem(); v6_init(); dk_shapeMade = -1; dk_drop = -1; for (int i = 0; i < 1024; i++) dk_sq[i] = (uint8_t)v3_clamp((int)v3_isqrt((uint32_t)i * 64 + 32), 0, 255); }

// where each pixel sits in its dot: 32 × 32 positions → distance from the dot's middle in 1/64 of a dot (by shape)
static void dk_makeShape(int shape) {
  for (int v = 0; v < 32; v++) for (int u = 0; u < 32; u++) {
    int a = 2 * u - 31, b = 2 * v - 31;                                    // −31 … 31 (half-steps)
    int aa = a < 0 ? -a : a, ab = b < 0 ? -b : b, d;
    switch (shape) {
      case DK_SQUARE: d = aa > ab ? aa : ab; break;
      case DK_DIAMOND: d = (aa + ab) * 3 / 4; break;
      case DK_CROSS: { int m = aa < ab ? aa : ab, M = aa > ab ? aa : ab; d = m * 3 + M / 3; } break;
      default: d = (int)v3_isqrt((uint32_t)(a * a + b * b));
    }
    dk_dist[(v << 5) | u] = (uint8_t)(d > 63 ? 63 : d);
  }
}

// one row: the dot (column table + the row's first dot), its size, the distance inside it → colour
__attribute__((optimize("O2"))) static void dk_row(uint8_t* out, const uint16_t* col, const uint8_t* rad, const uint8_t* drow) {
  const uint8_t* ot = dk_out;
  for (int x = 0; x < W; x++) { uint16_t c = col[x]; out[x] = ot[(rad[c >> 5] << 6) | drow[c & 31]]; }
}

void prog_dots_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  dk_mem();
  const DkLook& L = dk_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&dk_size, o7_kn(5, L.size / 100.0f), rate);
  v6_ease(&dk_range, v3_centre(6) ? o7_vary(6, 1.0f - L.range / 100.0f) : v3_u(6), rate);
  v6_ease(&dk_speedK, o7_kn(9, L.speed / 100.0f), rate);
  v6_ease(&dk_hexK, v3_centre(10) ? (float)L.hex : (v3_u(10) > 0.5f ? 1.0f : 0.0f), rate);
  int field = o7_pick(7, L.field, DK_NFIELD), shape = o7_pick(8, L.shape, DK_NSHAPE);
  if (shape != dk_shapeMade) { dk_makeShape(shape); dk_shapeMade = shape; }
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt;
  dk_t += sdt * (0.15f + dk_speedK * 2.5f);
  if (dk_t > 6283.0f) dk_t -= 6283.0f;
  if (v6_k12hit) dk_drop = 0;
  if (dk_drop >= 0) { dk_drop += v6_f.dt * 0.55f; if (dk_drop > 1.5f) dk_drop = -1; }
  float z = v6_zoom(1.0f);
  float cell = L.cell * z; if (cell < 5.0f) cell = 5.0f;
  bool hex = dk_hexK > 0.5f;
  float rowH = hex ? cell * 0.866f : cell;
  int nx = (int)(W / cell) + 3, ny = (int)(H / rowH) + 3;
  if (nx * ny > DK_MAXD) { ny = DK_MAXD / nx; }
  // ── the shade table: size r (0-31, in 1/64 of a dot … ×2) and distance d (0-63) → colour ──
  int edge = (int)(64.0f / cell * 1.4f) + 1;                                // about a pixel and a half, in 1/64 dot
  for (int r = 0; r < 32; r++) for (int d = 0; d < 64; d++) {
    int R = r * 2, s;
    if (shape == DK_RING) { int w = 4 + edge; int e = d - R; if (e < 0) e = -e; s = v3_clamp(15 - (e - w / 2) * 15 / edge, 0, 15); if (R < 4) s = 0; }
    else s = v3_clamp((R - d) * 15 / edge + 8, 0, 15);
    int hue = L.hue == 1 ? o7_mod15[r * 3 / 4] : (L.hue == 2 ? 0 : O7_GREY);
    if (s < 8 && L.hue == 1) hue = O7_GREY;                                 // coloured dots on black
    if (L.invert) s = 15 - s;
    dk_out[(r << 6) | d] = O7C(s, hue);
  }
  // ── the hidden field: one value 0 … 255 per dot → size (whole numbers: thousands of dots) ──
  float t = dk_t;
  int rmax = (int)(dk_size * 17.0f + 0.5f), rmin = (int)(rmax * (1.0f - dk_range));   // 16 = dots just touching
  int cx = (int)(HALFW + 70.0f * o7_nsin(t * 0.37f)), cy = (int)(HALFH + 45.0f * o7_nsin(t * 0.29f));
  float ga = t * 0.2f; int32_t gcx = (int32_t)(cosf(ga) * 256), gcy = (int32_t)(sinf(ga) * 256);
  int tt = (int)(t * 160.0f);
  int sx2 = (int)(HALFW + 90.0f * o7_nsin(t * 0.23f)), sy2 = (int)(HALFH - 50.0f * o7_ncos(t * 0.31f));
  const int32_t R2 = 110 * 110;
  int dropR = (int)(dk_drop * 260.0f);
  int32_t cellQ = (int32_t)(cell * 16.0f), rowQ = (int32_t)(rowH * 16.0f);
  for (int j = 0; j < ny; j++) {
    int py = ((j - 1) * rowQ) >> 4;
    int32_t pxq = -cellQ + ((hex && (j & 1)) ? cellQ / 2 : 0);
    uint8_t* rr = dk_rad + j * nx;
    for (int i = 0; i < nx; i++, pxq += cellQ) {
      int px = pxq >> 4, dx = px - cx, dy = py - cy, ox = px - HALFW, oy = py - HALFH, v;
      switch (field) {
        case DK_SPHERE: { int32_t q = dx * dx + dy * dy; v = q < R2 ? 255 - (int)(q * 255 / R2) : 0; v = 255 - (((255 - v) * (255 - v)) >> 8); } break;   // a dome
        case DK_WAVE: v = 128 + (v3_sin(px * 3 + py * 2 + tt) >> 7); break;
        case DK_GRADIENT: v = 128 + ((ox * gcx + oy * gcy) >> 9); break;
        case DK_RIPPLE: v = 128 + (v3_sin(dk_r(ox * ox + oy * oy) * 12 - tt) >> 7); break;
        case DK_TWIN: { int ex = px - sx2, ey = py - sy2;
                        v = 128 + ((v3_sin(dk_r(dx * dx + dy * dy) * 14 - tt) + v3_sin(dk_r(ex * ex + ey * ey) * 14 - tt)) >> 8); } break;
        case DK_PLASMA: v = 128 + ((v3_sin(px * 4 + tt) + v3_sin(py * 5 - tt / 2) + v3_sin((px + py) * 3 + tt / 3)) / 384); break;
        default: v = 128 + (v3_sin(tt / 2 - dk_r(ox * ox + oy * oy) * 4) >> 7);
      }
      if (dropR > 0) {
        int e = dk_r(ox * ox + oy * oy) - dropR;
        if (e > -30 && e < 30) v += (v3_cos(e * 17) >> 7) * (30 - (e < 0 ? -e : e)) / 30;
      }
      v = v3_clamp(v, 0, 255);
      rr[i] = (uint8_t)(rmin + ((rmax - rmin) * v >> 8));
    }
  }
  // ── columns: which dot across and where inside it (32 steps), for even and odd rows of dots ──
  for (int x = 0; x < W; x++) {
    for (int o = 0; o < 2; o++) {
      float fx = (x + cell) - ((hex && o) ? cell * 0.5f : 0.0f) + cell * 0.5f;     // (dot middles at (i − 1)·cell)
      int i = (int)(fx / cell); int u = (int)((fx - i * cell) * 32.0f / cell);
      if (i >= nx) i = nx - 1;
      (o ? dk_colO : dk_colE)[x] = (uint16_t)((i << 5) | v3_clamp(u, 0, 31));
    }
  }
  // ── rows ──
  for (int y = 0; y < H; y++) {
    float fy = y + rowH + rowH * 0.5f;
    int j = (int)(fy / rowH); int v = (int)((fy - j * rowH) * 32.0f / rowH);
    if (j >= ny) j = ny - 1;
    const uint16_t* col = (hex && (j & 1)) ? dk_colO : dk_colE;
    const uint8_t* rad = dk_rad + j * nx;
    dk_row(v3_buf + y * W, col, rad, dk_dist + (v3_clamp(v, 0, 31) << 5));
  }
  o7_end();
}

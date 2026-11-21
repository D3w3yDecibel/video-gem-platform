// =====================================================================
// PROGRAM: ALBERS — squares of colour that glow (by Dewey) — Set 7, slot 11
//
// Josef Albers painted the same picture over a thousand times: "Homage to
// the Square", three or four flat squares of colour, each set a little low
// inside the next, to show how one colour changes another beside it.
// Richard Anuszkiewicz, his student, made the squares vibrate: each band
// filled with fine lines of a second colour, so the colours mix in the eye
// and the middle seems to give off light ("Translumina", "Temple").
// Here the colours drift slowly round the palette, the squares breathe,
// and the line bands flow outward or glow from the centre.
//
// How it works: Homage is a handful of filled rectangles — almost free.
// The line pictures need one measure per pixel (from a middle that wanders a little) (the bigger of |x| and |y|,
// their sum, or a circle from a table) to know which band it is in, then
// whether it is on a line (across in the top and bottom triangles, down in
// the side ones, like a picture frame), and a small table gives the colour.
//
// Presets: k0 Homage · k1 Homage II · k2 Translumina · k3 Temple · k4 Interchange ·
//          k5 Diamond Temple · k6 Inner Light · k7 Breathe · k8 Flow · k9 Circles ·
//          k10 Complement · k11 Tour
//   k12 — SWAP: the colours turn inside out
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Squares · p6 Line Spacing · p7 Line Weight · p8 Drop (Albers' low middle) · p9 Breathe
//   p10 Shape (square | diamond | circle) · p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { AL_HOMAGE, AL_LINES };
enum { AL_SQUARE, AL_DIAMOND, AL_CIRCLE, AL_NSHAPE };
// mode · shape · n: squares (bands) · spacing: line period px · weight: line % · drop: the middle sits low (%) ·
// breathe % · flow: bands drift outward · hstep: hue step per band · glow: lines thicker toward the middle
struct AlLook { uint8_t mode, shape, n, spacing, weight, drop, breathe; int8_t flow; uint8_t hstep, glow, pal, cflow, spread, sat; };
static const AlLook al_looks[11] = {
  // mode       shape        n  spc wgt drop brth flow hst glow pal              cflow spr sat
  { AL_HOMAGE, AL_SQUARE ,  4, 0,  0, 50,  0,  0, 2,  0, O7P_ALBERS,  2,  50, 100 },   // Homage
  { AL_HOMAGE, AL_SQUARE ,  3, 0,  0, 60,  0,  0, 3,  0, O7P_VEGA  ,  2,  60, 100 },   // Homage II
  { AL_LINES , AL_SQUARE ,  8, 3, 40,  0,  0,  0, 2, 60, O7P_ANUSZ ,  2,  70, 100 },   // Translumina
  { AL_LINES , AL_SQUARE ,  6, 4, 50,  0,  0,  0, 3,  0, O7P_ANUSZ ,  3,  80, 100 },   // Temple
  { AL_HOMAGE, AL_SQUARE ,  4, 0,  0, 50,  0,  0, 2,  0, O7P_ALBERS, 12,  60, 100 },   // Interchange
  { AL_LINES , AL_DIAMOND,  7, 3, 45,  0,  0,  0, 2, 40, O7P_VEGA  ,  2,  80, 100 },   // Diamond Temple
  { AL_LINES , AL_SQUARE , 12, 2, 35,  0,  0,  0, 1, 80, O7P_NEON  ,  3, 100, 100 },   // Inner Light
  { AL_HOMAGE, AL_SQUARE ,  4, 0,  0, 50, 40,  0, 2,  0, O7P_RA    ,  2,  60,  90 },   // Breathe
  { AL_LINES , AL_SQUARE , 10, 3, 45,  0,  0, 30, 1, 30, O7P_AGAM  ,  2, 100, 100 },   // Flow
  { AL_LINES , AL_CIRCLE ,  9, 3, 45,  0,  0, 20, 2, 50, O7P_ANUSZ ,  3,  80, 100 },   // Circles
  { AL_LINES , AL_SQUARE ,  5, 2, 50,  0,  0,  0, 7,  0, O7P_ANUSZ ,  2,  30, 100 },   // Complement
};
static const char* const al_names[12] = { "Homage", "Homage II", "Translumina", "Temple", "Interchange", "Diamond Temple", "Inner Light", "Breathe", "Flow", "Circles", "Complement", "Tour" };
static const char* const al_lbl[6] = { "Squares", "Line Spacing", "Line Weight", "Drop", "Breathe", "Shape: Square|Diamond|Circle" };
const char* prog_albers_name() { return "ALBERS"; }
const char* prog_albers_character() { return "Albers' Homage to the Square and Anuszkiewicz's glowing line squares"; }
const char* prog_albers_presetName(int p) { return (p >= 0 && p < 12) ? al_names[p] : NULL; }
const char* prog_albers_potLabel(int p, int pot) { (void)p; return o7_label(pot, al_lbl); }
uint8_t prog_albers_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: band number for each distance (½ px, 2048), the colour table (band × on-line → colour, 64 × 2),
// line patterns across and down (2 × 640), √ table, |x| and x² per column
static uint8_t *al_side, *al_rb, *al_rl; static int al_c2x = W;
static uint8_t *al_band, *al_col, *al_lx, *al_ly; static uint16_t* al_sq; static int16_t* al_ax; static int32_t* al_x2;
static void al_mem() {
  al_band = v6_mem;                          // 2048
  al_col = v6_mem + 2048;                    // 128
  al_lx = v6_mem + 2176; al_ly = al_lx + 320;   // 640
  al_sq = (uint16_t*)(v6_mem + 2816);        // 2560 × 2 (r² / 64 in ½ px → r in ½ px)
  al_ax = (int16_t*)(v6_mem + 7936);         // 640
  al_x2 = (int32_t*)(v6_mem + 8576);         // 1280
  al_side = v6_mem + 9856;                   // 320: square pictures, the colour of each column at the sides
  al_rb = v6_mem + 10176; al_rl = al_rb + W;  // 640: a row's bands and its line bits (diamonds, circles)
}
static bool al_sqOK = false;
static float al_n, al_spc, al_wgt, al_drop, al_breK, al_ph = 0, al_flowP = 0, al_swap = 0, al_swapT = 0;
void prog_albers_init() {
  al_mem(); v6_init(); al_swap = al_swapT = 0;
  for (int i = 0; i < 2560; i++) al_sq[i] = (uint16_t)v3_isqrt((uint32_t)i * 64 + 32);
  al_sqOK = true;
}

// a line picture row: band from the measure out of the (wandering) middle, then lines across (top and
// bottom) or down (the sides), like a picture frame; one loop per shape. The lines keep their own
// positions as the middle moves, so the picture is never a mirror image of itself.
static inline __attribute__((always_inline)) void al_rowS(int shape, uint8_t* out, int ady, int32_t dy2, uint8_t lyOn) {
  const uint8_t *band = al_band, *col = al_col, *lx = al_lx; const int16_t* ax = al_ax; const int32_t* x2 = al_x2;
  if (shape == AL_SQUARE) {
    // squares: the top and bottom of the frame are one colour across; the sides come from the column
    int x0 = (al_c2x - ady) >> 1, x1 = (al_c2x + ady - 1) >> 1;           // the columns with |dx| <= ady
    if (x0 < 0) x0 = 0; if (x1 > W - 1) x1 = W - 1; if (x1 < x0 - 1) x1 = x0 - 1;
    if (x0 > 0) memcpy(out, al_side, x0 < W ? x0 : W);
    if (x1 >= x0 && x0 < W) memset(out + x0, col[(band[ady > 2047 ? 2047 : ady] << 1) | lyOn], x1 - x0 + 1);
    if (x1 + 1 < W) memcpy(out + x1 + 1, al_side + x1 + 1, W - x1 - 1);
    return;
  }
  // diamonds and circles, in two passes: (1) each pixel's band (×2) and line bit, (2) the colours
  uint8_t* rb = al_rb;
  if (shape == AL_DIAMOND) {
    for (int x = 0; x < W; x++) { int d = ((ax[x] + ady) * 181) >> 8; rb[x] = (uint8_t)(band[d > 2047 ? 2047 : d] << 1); }
  } else {
    // circles: the distance changes smoothly — exactly every other pixel, halfway between
    uint32_t q = (uint32_t)(x2[0] + dy2) >> 6; int d0 = al_sq[q < 2560 ? q : 2559];
    for (int x = 0; x < W; x += 2) {
      int xn = x + 2 < W ? x + 2 : W - 1;
      q = (uint32_t)(x2[xn] + dy2) >> 6; int d2 = al_sq[q < 2560 ? q : 2559], d1 = (d0 + d2) >> 1;
      rb[x] = (uint8_t)(band[d0 > 2047 ? 2047 : d0] << 1); rb[x + 1] = (uint8_t)(band[d1 > 2047 ? 2047 : d1] << 1);
      d0 = d2;
    }
  }
  // the line bits: down the sides (each column's own), across the top and bottom (this row's)
  uint8_t* rl = al_rl;
  memcpy(rl, lx, W);
  int x0 = (al_c2x - ady) >> 1, x1 = (al_c2x + ady - 1) >> 1;
  if (x0 < 0) x0 = 0; if (x1 > W - 1) x1 = W - 1;
  if (x1 >= x0) memset(rl + x0, lyOn, x1 - x0 + 1);
  for (uint8_t* const oe = out + W; out < oe; out++, rb++, rl++) *out = col[*rb | *rl];
}
__attribute__((optimize("O2"))) static void al_row(uint8_t* out, int shape, int ady, int32_t dy2, uint8_t lyOn) {
  if (shape == AL_SQUARE) al_rowS(AL_SQUARE, out, ady, dy2, lyOn);
  else if (shape == AL_DIAMOND) al_rowS(AL_DIAMOND, out, ady, dy2, lyOn);
  else al_rowS(AL_CIRCLE, out, ady, dy2, lyOn);
}

void prog_albers_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  al_mem();
  if (!al_sqOK) prog_albers_init();
  const AlLook& L = al_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&al_n, v3_centre(5) ? (float)L.n : 2.0f + v3_u(5) * 14.0f, rate);
  v6_ease(&al_spc, v3_centre(6) ? (float)L.spacing : 2.0f + v3_u(6) * 8.0f, rate);
  v6_ease(&al_wgt, o7_kn(7, L.weight / 100.0f), rate);
  v6_ease(&al_drop, o7_kn(8, L.drop / 100.0f), rate);
  v6_ease(&al_breK, o7_kn(9, L.breathe / 100.0f), rate);
  int shape = o7_pick(10, L.shape, AL_NSHAPE);
  o7_colours(L.pal, L.cflow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt;
  al_ph += sdt * 0.6f;
  al_flowP += sdt * L.flow * 0.02f; al_flowP -= floorf(al_flowP);
  if (v6_k12hit) al_swapT = al_swapT > 0.5f ? 0.0f : 1.0f;
  v6_ease(&al_swap, al_swapT, 2.0f);
  bool swapped = al_swap > 0.5f;
  float z = v6_zoom(1.0f);
  int n = v3_clamp((int)(al_n + 0.5f), 1, 16);
  // each band's hue (and, for Homage, which squares)
  uint8_t hue[17];
  for (int k = 0; k < 17; k++) { int kk = swapped ? (n - 1 - (k < n ? k : n - 1)) : k; hue[k] = o7_mod15[(kk * L.hstep) % 60]; }

  if (L.mode == AL_HOMAGE) {
    // Albers' proportions: each square 2 units smaller than the last, the gap below half the gap above
    float br = 1.0f + 0.06f * al_breK * sinf(al_ph);
    float unit = 220.0f / (2.0f * n + 2.0f) * z * br;                      // the outer square is 2n + 2 units
    float side = (2.0f * n + 2.0f) * unit, x0 = HALFW - side / 2, y0 = HALFH - side / 2;
    memset(v3_buf, O7C(10, O7_GREY), W * H);                               // the wall
    for (int k = 0; k <= n; k++) {
      float s = side - 2.0f * k * unit;
      float gx = k * unit, gyTop = k * unit * (1.0f + al_drop), gyBot = k * unit * (1.0f - al_drop);
      (void)gyBot;
      int ax0 = (int)(x0 + gx), ax1 = (int)(x0 + gx + s) - 1;
      int ay0 = (int)(y0 + gyTop), ay1 = (int)(y0 + gyTop + s) - 1;
      if (ax1 < ax0 || ay1 < ay0) break;
      uint8_t c = k == 0 ? O7C(3, hue[0]) : O7C(15 - (k & 1), hue[k]);
      if (k == 0) c = O7C(15, hue[0]);
      v3_rect(ax0, ay0, ax1, ay1, c);
    }
    o7_end();
    return;
  }
  // ── the line pictures ──
  float br = 1.0f + 0.08f * al_breK * sinf(al_ph);
  float bandPx = 230.0f / (2.0f * n) * z * br;                              // ½-px distance per band = bandPx
  int per = v3_clamp((int)(al_spc * z + 0.5f), 2, 30);
  for (int d = 0; d < 2048; d++) {
    float b = d / (2.0f * bandPx) - al_flowP;
    int k = (int)floorf(b); if (k < 0) k = 0;
    if (k >= n) k = n;                                                    // outside: the frame
    al_band[d] = (uint8_t)k;
  }
  // colours: each band's ground and lines (the lines carry the next band's hue: they mix in the eye)
  for (int k = 0; k <= n && k < 64; k++) {
    int hk = hue[k], hl = hue[(k + 1) % (n ? n : 1)];
    uint8_t ground = O7C(12, hk), line = O7C(15, hl);
    if (k == n) { ground = O7C(1, O7_GREY); line = ground; }                // outside: a plain dark frame
    al_col[k << 1] = ground; al_col[(k << 1) | 1] = line;
  }
  // line weight per band: thicker toward the middle if glowing → done through the line tables per row/column
  int lw = v3_clamp((int)(per * al_wgt + 0.5f), 1, per - 1);
  // the middle wanders a little; the lines drift slowly on their own
  float tw0 = v6_t * 0.1f;
  int c2x = (int)(2.0f * (HALFW + 30.0f * z * o7_nsin(tw0 * 1.3f))), c2y = (int)(2.0f * (HALFH + 20.0f * z * o7_ncos(tw0)));
  int lo = (int)(v6_t * 2.0f) % per, ro = (int)(v6_t * 1.5f) % per;
  for (int x = 0; x < W; x++) {
    int dx = 2 * x + 1 - c2x;
    al_ax[x] = (int16_t)(dx < 0 ? -dx : dx);
    al_x2[x] = (int32_t)dx * dx;
    int g = lw;
    if (L.glow) { float f = 1.0f - (float)(al_ax[x] / 2) / HALFW; if (f < 0) f = 0; g = v3_clamp(lw + (int)(f * L.glow * 0.02f * per), 1, per - 1); }
    al_lx[x] = (uint8_t)(((x + lo) % per) < g ? 1 : 0);
    int a = al_ax[x] > 2047 ? 2047 : al_ax[x];
    al_side[x] = al_col[(al_band[a] << 1) | al_lx[x]];
  }
  al_c2x = c2x;
  for (int y = 0; y < H; y++) {
    int dy = 2 * y + 1 - c2y, ady = dy < 0 ? -dy : dy, g = lw;
    if (L.glow) { float f = 1.0f - (float)(ady / 2) / HALFH; if (f < 0) f = 0; g = v3_clamp(lw + (int)(f * L.glow * 0.02f * per), 1, per - 1); }
    uint8_t lyOn = (uint8_t)(((y + ro) % per) < g ? 1 : 0);
    al_row(v3_buf + y * W, shape, ady, (int32_t)ady * ady, lyOn);
  }
  o7_end();
}

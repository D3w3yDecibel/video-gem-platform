// =====================================================================
// PROGRAM: SOTO — Jesús Rafael Soto's vibrations (by Dewey) — Set 7, slot 9
//
// Soto hung things in front of a screen of fine black and white lines:
// black squares, painted rods, wires, more line-screens. Walk past and
// everything in front starts to shimmer and dissolve — "vibrations" — the
// background lines beating against the edges and the lines in front. His
// "Penetrables" are rooms of hanging rods you walk through, swaying.
// Here the squares drift and turn, the rods sway like a curtain or swing
// like pendulums, and line screens slide over line screens.
//
// How it works: whole rows at a time. The background is one row of lines,
// copied down the screen (memcpy); each shape in front works out where it
// crosses the row — a span — and fills it from its own line pattern (solid,
// lines across, lines down, slanting lines). No per-pixel maths at all.
//
// Presets: k0 Vibration · k1 Penetrable · k2 Tes · k3 Orbit · k4 Curtain · k5 Kinetic ·
//          k6 Colour · k7 Rain · k8 Nested · k9 Pendulums · k10 Ghost · k11 Tour
//   k12 — SCATTER: everything jumps and settles again
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Line Spacing · p6 Shapes (how many) · p7 Shape Size · p8 Motion · p9 Fill · p10 Sway
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { SO_SQUARES, SO_RODS, SO_ORBIT, SO_CURTAIN, SO_SCREEN, SO_RAIN, SO_NESTED, SO_PENDULUM };
enum { SO_SOLID, SO_HLINES, SO_VLINES, SO_DIAG, SO_NFILL };
// layout · fill · n: shapes · size px · spacing: background line period px · motion · sway · turn: squares rotate ·
// hues: shapes in colour · bgdiag: slanting background
struct SoLook { uint8_t layout, fill, n, size, spacing, motion, sway, turn, hues, bgdiag, pal, flow, spread, sat; };
static const SoLook so_looks[11] = {
  // layout       fill        n  size spc mot sway turn hue bgd pal            flow spr sat
  { SO_SQUARES , SO_SOLID ,  7,  44, 4, 30,  0,  0, 0, 0, O7P_BW      , 0,  50,   0 },   // Vibration
  { SO_RODS    , SO_SOLID , 28,   4, 4, 25, 40,  0, 1, 0, O7P_SOTO    , 2, 100, 100 },   // Penetrable
  { SO_SQUARES , SO_HLINES,  5,  60, 4, 25,  0, 30, 1, 0, O7P_KITAOKA , 3,  90, 100 },   // Tes
  { SO_ORBIT   , SO_DIAG  ,  8,  36, 5, 35,  0, 40, 1, 0, O7P_SPECTRUM, 3,  90, 100 },   // Orbit
  { SO_CURTAIN , SO_SOLID , 40,   3, 4, 30, 70,  0, 1, 0, O7P_SOTO    , 2, 100, 100 },   // Curtain
  { SO_SCREEN  , SO_VLINES,  1, 200, 4, 20,  0,  0, 1, 0, O7P_ANUSZ   , 3,  90, 100 },   // Kinetic
  { SO_SQUARES , SO_SOLID ,  9,  40, 5, 30,  0, 20, 1, 0, O7P_SOTO    , 2, 100, 100 },   // Colour
  { SO_RAIN    , SO_SOLID , 40,   3, 4, 45,  0,  0, 1, 0, O7P_NEON    , 3, 100, 100 },   // Rain
  { SO_NESTED  , SO_HLINES,  5,  40, 4, 20,  0, 40, 1, 0, O7P_AGAM    , 3,  90, 100 },   // Nested
  { SO_PENDULUM, SO_SOLID , 20,   5, 4, 30, 60,  0, 1, 0, O7P_SOTO    , 2, 100,  90 },   // Pendulums
  { SO_SQUARES , SO_DIAG  ,  6,  56, 4, 25,  0, 25, 0, 1, O7P_INK     , 0,  40, 100 },   // Ghost
};
static const char* const so_names[12] = { "Vibration", "Penetrable", "Tes", "Orbit", "Curtain", "Kinetic", "Colour", "Rain", "Nested", "Pendulums", "Ghost", "Tour" };
static const char* const so_lbl[6] = { "Line Spacing", "Shapes", "Shape Size", "Motion", "Fill: Solid|Across|Down|Slant", "Sway" };
const char* prog_soto_name() { return "SOTO"; }
const char* prog_soto_character() { return "Soto's vibrations: squares and hanging rods shimmering in front of fine lines"; }
const char* prog_soto_presetName(int p) { return (p >= 0 && p < 12) ? so_names[p] : NULL; }
const char* prog_soto_potLabel(int p, int pot) { (void)p; return o7_label(pot, so_lbl); }
uint8_t prog_soto_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: pattern rows (each 2 × W + 64, so a span can start anywhere in them): background,
// lines down, slanting lines; and the shapes (corners, 1/16 px), with their bounding rows
#define SO_PW (2 * W + 64)
#define SO_MAX 48
// each edge kept ready for row-by-row use: from ya to yb (1/16 px, ya < yb), x at ya, and x per 1/16 px down (×256)
struct SoShape { int32_t ya[4], yb[4], xa[4], sl[4]; int16_t y0, y1; uint8_t fill, col, ne; };
static uint8_t *so_bg, *so_vl, *so_dl; static SoShape* so_sh;
static void so_mem() { so_bg = v6_mem; so_vl = so_bg + SO_PW; so_dl = so_vl + SO_PW; so_sh = (SoShape*)(v6_mem + 3 * SO_PW + 8); }
static float so_spc, so_n, so_size, so_mot, so_sway, so_t = 0, so_kick = 0;
static uint32_t so_seed = 1;
void prog_soto_init() { so_mem(); v6_init(); so_kick = 0; }

// a convex shape's span on row y (centre of the row), x0 … x1 inclusive; false if it misses.
// (sy − ya) < (yb − ya), so (sy − ya) × slope stays below |dx| × 256: no overflow, no division)
static bool so_span(int k, int y, int* x0, int* x1) {                // (by number: Arduino can't see the struct here)
  const SoShape& s = so_sh[k];
  int32_t sy = y * 16 + 8, lo = 1 << 30, hi = -(1 << 30);
  for (int i = 0; i < s.ne; i++) {
    if (sy < s.ya[i] || sy >= s.yb[i]) continue;
    int32_t x = s.xa[i] + (((sy - s.ya[i]) * s.sl[i]) >> 8);
    if (x < lo) lo = x; if (x > hi) hi = x;
  }
  if (hi < lo) return false;
  int a = (lo + 8) >> 4, b = (hi - 8) >> 4;
  if (a < 0) a = 0; if (b > W - 1) b = W - 1;
  if (a > b) return false;
  *x0 = a; *x1 = b; return true;
}
// corners (1/16 px, kept within ±1000 px) → edges
static void so_edges(int k, const int32_t* xs, const int32_t* ys) {     // (by number, for Arduino)
  SoShape& s = so_sh[k];
  s.ne = 0;
  int32_t lo = 1 << 30, hi = -(1 << 30);
  for (int i = 0, j = 3; i < 4; j = i++) {
    int32_t xa = xs[j], ya = ys[j], xb = xs[i], yb = ys[i];
    if (ya < lo) lo = ya; if (ya > hi) hi = ya;
    if (ya == yb) continue;
    if (ya > yb) { int32_t t = xa; xa = xb; xb = t; t = ya; ya = yb; yb = t; }
    int e = s.ne++;
    s.ya[e] = ya; s.yb[e] = yb; s.xa[e] = xa; s.sl[e] = ((xb - xa) * 256) / (yb - ya);
  }
  s.y0 = (int16_t)(lo >> 4); s.y1 = (int16_t)((hi >> 4) + 1);
}
static void so_quad(int k, float cx, float cy, float hw, float hh, float ang, int fill, int col) {
  SoShape& s = so_sh[k];
  float c = cosf(ang), sn = sinf(ang);
  float px[4] = { -hw, hw, hw, -hw }, py[4] = { -hh, -hh, hh, hh };
  int32_t xs[4], ys[4];
  for (int i = 0; i < 4; i++) {
    float x = cx + px[i] * c - py[i] * sn, y = cy + px[i] * sn + py[i] * c;
    xs[i] = (int32_t)(v3_clampf(x, -1000, 1000) * 16); ys[i] = (int32_t)(v3_clampf(y, -1000, 1000) * 16);
  }
  so_edges(k, xs, ys); s.fill = (uint8_t)fill; s.col = (uint8_t)col;
}
// a hanging rod: fixed at the top (x0), its foot swung to x1 at the bottom
static void so_rod(int k, float x0, float x1, float yTop, float yBot, float w, int fill, int col) {
  SoShape& s = so_sh[k];
  x0 = v3_clampf(x0, -900, 900); x1 = v3_clampf(x1, -900, 900); yTop = v3_clampf(yTop, -900, 900); yBot = v3_clampf(yBot, -900, 900);
  int32_t xs[4] = { (int32_t)((x0 - w) * 16), (int32_t)((x0 + w) * 16), (int32_t)((x1 + w) * 16), (int32_t)((x1 - w) * 16) };
  int32_t ys[4] = { (int32_t)(yTop * 16), (int32_t)(yTop * 16), (int32_t)(yBot * 16), (int32_t)(yBot * 16) };
  so_edges(k, xs, ys); s.fill = (uint8_t)fill; s.col = (uint8_t)col;
}

void prog_soto_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  so_mem();
  const SoLook& L = so_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&so_spc, v3_centre(5) ? o7_vary(5, (float)L.spacing) : 2.0f + v3_u(5) * 10.0f, rate);
  v6_ease(&so_n, v3_centre(6) ? (float)L.n : 1.0f + v3_u(6) * (SO_MAX - 1), rate);
  v6_ease(&so_size, v3_centre(7) ? o7_vary(7, (float)L.size) : 4.0f + v3_u(7) * 120.0f, rate);
  v6_ease(&so_mot, o7_kn(8, L.motion / 100.0f), rate);
  v6_ease(&so_sway, o7_kn(10, L.sway / 100.0f), rate);
  int fill = o7_pick(9, L.fill, SO_NFILL);
  o7_tint = 0.55f;                                                         // the black & white lines take a little of the palette's colour
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt;
  so_t += sdt * (0.1f + so_mot * 2.0f); if (so_t > 10000.0f) so_t -= 10000.0f;
  if (v6_k12hit) { so_kick = 1.0f; so_seed = v3_rand() | 1; }
  if (so_kick > 0) { so_kick -= v6_f.dt * 0.8f; if (so_kick < 0) so_kick = 0; }
  float t = so_t, z = v6_zoom(1.0f);
  int per = v3_clamp((int)(so_spc * z + 0.5f), 2, 40);
  int lw = per / 2;
  uint8_t ink = O7C(0, O7_GREY), paper = O7C(15, O7_GREY);
  // ── the pattern rows: background lines down, lines down for shapes (half a period out), slanting lines ──
  for (int x = 0; x < SO_PW; x++) {
    so_bg[x] = ((x % per) < lw) ? ink : paper;
    so_vl[x] = (((x + per / 2) % (per + 1)) < lw) ? ink : paper;          // a slightly different spacing: moiré
    so_dl[x] = ((x % (per + 2)) < lw + 1) ? ink : paper;
  }
  // ── the shapes ──
  int n = v3_clamp((int)(so_n + 0.5f), 1, SO_MAX);
  float sz = so_size * z, kick = so_kick * so_kick;
  int ns = 0;
  switch (L.layout) {
    case SO_SQUARES: case SO_NESTED: {
      for (int k = 0; k < n && ns < SO_MAX; k++) {
        uint32_t h = v3_hash(so_seed * 31u + k * 7919u);
        float hx = (h & 1023) / 1023.0f, hy = ((h >> 10) & 1023) / 1023.0f, ph = ((h >> 20) & 255) / 40.0f;
        float cx = 20 + hx * (W - 40) + 40 * o7_nsin(t * 0.4f + ph), cy = 20 + hy * (H - 40) + 25 * o7_nsin(t * 0.33f + ph * 1.3f);
        if (L.layout == SO_NESTED) { cx = HALFW; cy = HALFH; }
        cx += kick * ((int)((h >> 3) % 200) - 100); cy += kick * ((int)((h >> 5) % 160) - 80);
        float s = L.layout == SO_NESTED ? sz * (n - k) / n * 2.0f : sz * (0.7f + 0.6f * (((h >> 12) & 255) / 255.0f));
        float ang = L.turn * 0.01f * (t * ((k & 1) ? -0.5f : 0.5f) + ph);
        int f = L.layout == SO_NESTED ? ((k & 1) ? SO_VLINES : fill) : fill;
        so_quad(ns++, cx, cy, s * 0.5f, s * 0.5f, ang, f, L.hues ? o7_mod15[k] : O7_GREY);
      }
    } break;
    case SO_ORBIT: {
      for (int k = 0; k < n && ns < SO_MAX; k++) {
        float a = t * 0.5f + k * 6.2832f / n, R = 80 * z;
        so_quad(ns++, HALFW + R * cosf(a) * 1.3f, HALFH + R * sinf(a), sz * 0.5f, sz * 0.5f, a * L.turn * 0.02f, fill, L.hues ? o7_mod15[k] : O7_GREY);
      }
    } break;
    case SO_RODS: case SO_CURTAIN: case SO_PENDULUM: case SO_RAIN: {
      for (int k = 0; k < n && ns < SO_MAX; k++) {
        uint32_t h = v3_hash(so_seed * 131u + k * 104729u);
        float x0 = (k + 0.5f) * W / n + (L.layout == SO_RODS ? (int)(h & 31) - 16 : 0);
        float sw;
        if (L.layout == SO_CURTAIN) sw = 40.0f * so_sway * o7_nsin(t * 1.1f - k * 0.35f);
        else if (L.layout == SO_PENDULUM) sw = 60.0f * so_sway * o7_nsin(t * (1.0f + k * 0.07f));   // each a little faster: they go in and out of step
        else sw = 25.0f * so_sway * o7_nsin(t * 0.7f + ((h >> 8) & 255) / 40.0f);
        sw += kick * ((int)((h >> 4) % 160) - 80);
        float top = 0, bot = H;
        if (L.layout == SO_RAIN) { float len = 20 + (h & 63); top = fmodf(t * (60.0f + ((h >> 6) & 63)) + (h >> 12) % 300, H + len) - len; bot = top + len; }
        if (L.layout == SO_PENDULUM) bot = H * (0.55f + 0.4f * (((h >> 9) & 255) / 255.0f));
        so_rod(ns++, x0, x0 + sw, top, bot, v3_clampf(sz * 0.5f, 0.6f, 20.0f), fill, L.hues ? o7_mod15[(h >> 16) % 15] : O7_GREY);
      }
    } break;
    default: {                                                            // a big line screen sliding over the background
      float off = 60.0f * o7_nsin(t * 0.5f);
      so_quad(ns++, HALFW + off, HALFH, sz * 0.6f, sz * 0.45f, 0.08f * o7_nsin(t * 0.3f), fill, O7_GREY);
    }
  }
  // ── rows ──
  int dsh = (int)(t * 6.0f);
  for (int y = 0; y < H; y++) {
    uint8_t* out = v3_buf + y * W;
    int bo = L.bgdiag ? (y + dsh) % per : 0;
    memcpy(out, so_bg + bo, W);
    bool hl = ((y + (int)(t * 4.0f)) % per) < lw;                          // lines across, for this row
    for (int k = 0; k < ns; k++) {
      const SoShape& s = so_sh[k];
      if (y < s.y0 || y > s.y1) continue;
      int x0, x1;
      if (!so_span(k, y, &x0, &x1)) continue;
      int len = x1 - x0 + 1;
      uint8_t col = s.col;
      switch (s.fill) {
        case SO_SOLID: memset(out + x0, col == O7_GREY ? ink : O7C(15, col), len); break;
        case SO_HLINES: memset(out + x0, hl ? ink : (col == O7_GREY ? paper : O7C(15, col)), len); break;
        case SO_VLINES: memcpy(out + x0, so_vl + x0, len); break;
        default: memcpy(out + x0, so_dl + x0 + (y % (per + 2)), len);
      }
    }
  }
  o7_end();
}

// =====================================================================
// PROGRAM: WIPES — the vision mixer's wipe generator (by Dewey) — Set 7, slot 6
//
// Every TV studio of the 1970s and 80s had a vision mixer with a row of
// wipe buttons: irises, clock sweeps, diamonds, boxes, stars, Venetian
// blinds — each one an analog pattern generator racing a comparator
// against the T-bar. Add a coloured border, soften the edge, wobble it with
// the modulator, repeat it across the screen, and the humble transition
// becomes a light show. Here the wipe never finishes: it breathes back and
// forth, or rolls out from the middle in endless rings.
//
// How it works: each pixel gets a pattern value (its distance, its angle,
// how far it is along a diamond or a box — from small tables, no square
// roots), the modulator adds a wobble, Multiple repeats it, and one colour
// table turns "value minus T-bar position" into picture A, the border, or
// picture B.
//
// Presets: k0 Iris · k1 Clock · k2 Diamond · k3 Box · k4 Star · k5 Venetian ·
//          k6 Checker · k7 Spiral · k8 Wave · k9 Barn Door · k10 Matrix · k11 Tour
//   k12 — TAKE: an instant transition (A and B swap)
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 T-Bar (centre = automatic, otherwise yours) · p6 Border · p7 Softness
//   p8 Multiple (1-12) · p9 Modulation · p10 Pattern · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { WI_IRIS, WI_CLOCK, WI_DIAMOND, WI_BOX, WI_STAR, WI_VENETIAN, WI_CHECKER, WI_SPIRAL, WI_WAVE, WI_BARN, WI_MATRIX, WI_NPAT };
struct WpLook { uint8_t pat, border, soft, mult, mod, modK, rate, wander, hueA, hueB, levA, levB, roll, pal, flow, spread, scan; };
static const WpLook wp_looks[11] = {
  // pat          bord soft mult mod modK rate wand hA  hB  lA  lB roll pal              flow spr scan
  { WI_IRIS,      12,  10,  1,   0,  6,  30,  40,  2,  9,  5, 10, 0, V7P_STUDIO,       3, 100, 2 },   // Iris
  { WI_CLOCK,     10,   6,  1,   0,  0,  25,   0, 12,  4,  6, 11, 0, V7P_PAIK,         4, 100, 2 },   // Clock
  { WI_DIAMOND,    8,   4,  6,   0,  0,  40,  30,  3, 10,  4, 12, 1, V7P_SCANIMATE,    5, 100, 2 },   // Diamond
  { WI_BOX,       10,   2,  5,   0,  0,  35,  20,  6, 14,  3, 11, 1, V7P_CHICAGO,      5, 100, 2 },   // Box
  { WI_STAR,      14,  12,  1,  45,  5,  25,  30,  8,  1,  5, 11, 0, V7P_BLACKLIGHT,   4, 100, 2 },   // Star
  { WI_VENETIAN,   6,   3,  8,   0,  0,  40,   0,  5, 13,  3, 12, 1, V7P_SUNSET,       4,  90, 2 },   // Venetian
  { WI_CHECKER,    6,   2,  1,   0,  0,  30,   0,  9,  2,  4, 12, 0, V7P_STUDIO,       3, 100, 2 },   // Checker
  { WI_SPIRAL,     8,   8,  3,  20,  3,  45,  30, 11,  3,  4, 11, 1, V7P_LUMIA,        5, 100, 2 },   // Spiral
  { WI_WAVE,      14,  10,  1,  60,  4,  25,   0,  1,  7,  5, 11, 0, V7P_VASULKA,      4,  90, 2 },   // Wave
  { WI_BARN,      16,   8,  1,  25,  3,  30,   0, 14,  6,  4, 12, 0, V7P_KODACHROME,   3,  90, 2 },   // Barn Door
  { WI_MATRIX,     6,   4,  2,   0,  0,  50,   0,  4, 12,  3, 12, 1, V7P_PAIK,         5, 100, 2 },   // Matrix
};
static const char* const wp_names[12] = { "Iris", "Clock", "Diamond", "Box", "Star", "Venetian", "Checker", "Spiral", "Wave", "Barn Door", "Matrix", "Tour" };
static const char* const wp_lbl[6] = { "T-Bar (centre = auto)", "Border", "Softness", "Multiple", "Modulation", "Pattern" };
const char* prog_wipes_name() { return "WIPES"; }
const char* prog_wipes_character() { return "A vision mixer's wipe generator: irises, clocks, stars and blinds that never finish"; }
const char* prog_wipes_presetName(int p) { return (p >= 0 && p < 12) ? wp_names[p] : NULL; }
const char* prog_wipes_potLabel(int p, int pot) { (void)p; return v7_label(pot, wp_lbl); }
uint8_t prog_wipes_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: distance and angle tables for a quarter of the screen (half size), the colour table, one row of values
#define WI_QW 100
#define WI_QH 80
static uint8_t *wp_rq, *wp_aq, *wp_lut, *wp_big, *wp_cix, *wp_mlx; static int16_t *wp_row, *wp_cax; static int8_t* wp_sin;
static void wp_mem() {
  wp_rq = v6_mem; wp_aq = v6_mem + WI_QW * WI_QH; wp_lut = v6_mem + 2 * WI_QW * WI_QH;   // 1024: (value − position) → colour
  wp_big = wp_lut + 1024;                                                                // 4096: final value → colour, made each frame
  wp_row = (int16_t*)(wp_big + 4096); wp_cax = wp_row + W;                              // one row of values; |x − centre| per column
  wp_sin = (int8_t*)(wp_cax + W); wp_cix = (uint8_t*)(wp_sin + 256); wp_mlx = wp_cix + W;
}

static float wp_pos = 0, wp_ph = 0, wp_bw, wp_soft, wp_mult, wp_mod, wp_take = -1, wp_wph = 0;
static bool wp_swap = false;

void prog_wipes_init() {
  wp_mem(); v6_init(); wp_take = -1; wp_swap = false;
  for (int j = 0; j < WI_QH; j++) for (int i = 0; i < WI_QW; i++) {
    int x = 2 * i + 1, y = 2 * j + 1;
    wp_rq[j * WI_QW + i] = (uint8_t)v3_clamp((int)v3_isqrt((uint32_t)(x * x + y * y)), 0, 255);
    wp_aq[j * WI_QW + i] = (uint8_t)(v3_atan(y, x) >> 2);           // 0 … 64: a quarter turn in 256 steps
  }
  for (int i = 0; i < 256; i++) wp_sin[i] = (int8_t)v3_clamp(v3_sin(i * 4) >> 7, -127, 127);
  for (int x = 0; x < W; x++) { int lx = (x % 40) - 20; wp_mlx[x] = (uint8_t)((lx < 0 ? -lx : lx) >> 1); }
}

// the raw pattern value for one row (each pattern its own tight loop)
__attribute__((optimize("O2"))) static void wp_pattern(int pat, int y, int cx, int cy, int modA, int modPh) {
  int16_t* row = wp_row; const int16_t* cax = wp_cax; const uint8_t* cix = wp_cix;
  int dy = y - cy, ay = dy < 0 ? -dy : dy;
  int jy = ay >> 1; if (jy >= WI_QH) jy = WI_QH - 1;
  const uint8_t* rq = wp_rq + jy * WI_QW; const uint8_t* aq = wp_aq + jy * WI_QW;
  int xc = v3_clamp(cx, 0, W);
  switch (pat) {
    case WI_IRIS: for (int x = 0; x < W; x++) row[x] = rq[cix[x]]; break;
    case WI_CLOCK: case WI_STAR: case WI_SPIRAL: {
      // full angle 0 … 255, 0 at twelve o'clock, clockwise: a = base ± the quarter angle
      int bR = 64, sR = dy < 0 ? -1 : 1, bL = 192, sL = dy < 0 ? 1 : -1;
      const int8_t* sn = wp_sin;
      for (int half = 0; half < 2; half++) {
        int x0 = half ? xc : 0, x1 = half ? W : xc, b = half ? bR : bL, sg = half ? sR : sL;
        for (int x = x0; x < x1; x++) {
          int a = (b + sg * aq[cix[x]]) & 255, r = rq[cix[x]];
          if (pat == WI_CLOCK) row[x] = (int16_t)(a + (a >> 3));
          else if (pat == WI_SPIRAL) row[x] = (int16_t)(r + a);
          else row[x] = (int16_t)(r + ((r * sn[(a * 5 + modPh) & 255] * modA) >> 13));
        }
      }
    } break;
    case WI_DIAMOND: for (int x = 0; x < W; x++) row[x] = (int16_t)(((cax[x] + ay) * 3) >> 2); break;
    case WI_BOX: { int by = ay * 4 / 3; for (int x = 0; x < W; x++) row[x] = (int16_t)(cax[x] > by ? cax[x] : by); } break;
    case WI_VENETIAN: for (int x = 0; x < W; x++) row[x] = (int16_t)(y + 40); break;
    case WI_WAVE: for (int x = 0; x < W; x++) row[x] = (int16_t)(x + 40); break;
    case WI_BARN: for (int x = 0; x < W; x++) row[x] = (int16_t)((cax[x] * 3) >> 1); break;
    case WI_CHECKER: {
      int ly = (y & 31) - 16; ly = ly < 0 ? -ly : ly;
      for (int x = 0; x < W; x++) {
        int lx = (x & 31) - 16; lx = lx < 0 ? -lx : lx;
        row[x] = (int16_t)((lx > ly ? lx : ly) * 16 + ((((x >> 5) + (y >> 5)) & 1) ? 0 : 128));
      }
    } break;
    default: {                                                          // a matrix of little irises
      int ly = (y % 40) - 20; ly = (ly < 0 ? -ly : ly) >> 1;
      const uint8_t* rr = wp_rq + ly * WI_QW; const uint8_t* mlx = wp_mlx;
      for (int x = 0; x < W; x++) row[x] = (int16_t)(rr[mlx[x]] * 9);
    }
  }
}
// values → colours: the wobble (modulation) added, scaled (zoom × multiple), one table lookup
static int16_t wp_mt[256];                                                // the wobble, scaled, for this frame
__attribute__((optimize("O2"))) static void wp_out(uint8_t* out, int zm, int rowMod, bool mod, int modK, int phase) {
  const int16_t* row = wp_row; const uint8_t* big = wp_big;
  if (mod) {
    const int16_t* mt = wp_mt; int idx = phase;
    for (int x = 0; x < W; x++, idx += modK) {
      int v = ((row[x] + rowMod + mt[idx & 255]) * zm) >> 8;
      if ((unsigned)v > 4095) v = v < 0 ? 0 : 4095;
      out[x] = big[v];
    }
  } else for (int x = 0; x < W; x++) {
    int v = ((row[x] + rowMod) * zm) >> 8;
    if ((unsigned)v > 4095) v = v < 0 ? 0 : 4095;
    out[x] = big[v];
  }
}

void prog_wipes_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  wp_mem();
  const WpLook& L = wp_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&wp_bw, v3_centre(6) ? L.border : v3_u(6) * 40.0f, rate);
  v6_ease(&wp_soft, v3_centre(7) ? L.soft : v3_u(7) * 40.0f, rate);
  v6_ease(&wp_mult, v3_centre(8) ? (float)L.mult : 1.0f + v3_u(8) * 11.0f, rate);
  v6_ease(&wp_mod, v7_kn(9, L.mod / 100.0f), rate);
  int pat = v3_centre(10) ? L.pat : v3_clamp(v3_k(10) * WI_NPAT / 1024, 0, WI_NPAT - 1);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 100);

  float sdt = v6_f.sdt, t = v6_t;
  int mult = (int)(wp_mult + 0.5f); if (mult < 1) mult = 1;
  bool multi = mult > 1 || L.roll;
  // ── the T-bar: automatic (back and forth, or rolling for repeats), or the knob ──
  wp_ph += sdt * L.rate * 0.01f;
  if (wp_ph > 1000) wp_ph -= 1000;
  float autoPos;
  if (multi) autoPos = (wp_ph - floorf(wp_ph)) * 256.0f;                // rings roll outward for ever
  else { float f = wp_ph - floorf(wp_ph), tri = f < 0.5f ? f * 2 : 2 - f * 2; tri = v3_clampf(tri * 1.12f - 0.06f, 0, 1); autoPos = -30 + tri * tri * (3 - 2 * tri) * 330; }
  float target = v3_centre(5) ? autoPos : (multi ? v3_u(5) * 256.0f : -30 + v3_u(5) * 330.0f);
  if (v6_k12hit) wp_take = 0;
  if (wp_take >= 0) {                                                    // TAKE: sweep straight through, then swap A and B
    wp_take += v6_f.dt * 2.5f;
    target = -30 + wp_take * 330;
    if (wp_take >= 1.0f) { wp_take = -1; wp_swap = !wp_swap; }
  }
  if (multi && !v3_centre(5)) v6_ease(&wp_pos, target, 6.0f); else wp_pos = target;
  if (multi) { wp_pos = fmodf(wp_pos, 256.0f); if (wp_pos < 0) wp_pos += 256.0f; }

  // ── the colour table: picture A | soft edge | border | soft edge | picture B ──
  int hA = wp_swap ? L.hueB : L.hueA, hB = wp_swap ? L.hueA : L.hueB, lA = wp_swap ? L.levB : L.levA, lB = wp_swap ? L.levA : L.levB;
  int hb = (L.hueA + L.hueB) / 2 + 4;
  int half = (int)(wp_bw * (multi ? 0.6f : 1.0f)), soft = (int)wp_soft + 1;
  for (int d = -512; d < 512; d++) {
    int ad = d < 0 ? -d : d, c;
    if (ad <= half) c = V6C(15, hb);
    else if (ad <= half + soft) { int l = 15 - (ad - half) * 15 / soft, base = d < 0 ? lA : lB; c = l > base ? V6C(l, hb) : V6C(base, d < 0 ? hA : hB); }
    else c = d < 0 ? V6C(lA, hA) : V6C(lB, hB);
    wp_lut[d + 512] = (uint8_t)c;
  }
  // ── the final table: every value 0 … 4095 straight to its colour ──
  for (int v = 0; v < 4096; v++) {
    uint8_t c;
    if (!multi) {                                                          // one wipe; picture A is a wash of the pattern's own rings
      int d = v - (int)wp_pos; d = d < -512 ? -512 : (d > 511 ? 511 : d);
      c = wp_lut[d + 512];
      if (d < 0) c = (uint8_t)((c & 0xF0) | ((c + (v >> 5)) & 15));
    } else {                                                               // repeated: each band its own hue
      int d = (v & 255) - (int)wp_pos; d &= 255; if (d >= 128) d -= 256;
      c = wp_lut[d + 512];
      c = (uint8_t)((c & 0xF0) | ((c + (v >> 8)) & 15));
    }
    wp_big[v] = c;
  }
  // ── where the pattern is centred (a gentle wander) ──
  wp_wph += sdt * 0.3f;
  int cx = HALFW + (int)(L.wander * 0.5f * sinf(wp_wph * 1.3f)), cy = HALFH + (int)(L.wander * 0.35f * sinf(wp_wph * 0.9f));
  for (int x = 0; x < W; x++) { int ax = v3_abs(x - cx); wp_cax[x] = (int16_t)ax; wp_cix[x] = (uint8_t)v6_min(ax >> 1, WI_QW - 1); }
  int zm = (int)(256 / v6_zoom(1.0f)) * mult;
  int modA = (int)(wp_mod * 120), modPh = (int)(t * 40) & 255;
  int modK = L.modK ? L.modK : 4;
  bool rowWave = pat == WI_WAVE || pat == WI_VENETIAN;
  for (int i = 0; i < 256; i++) wp_mt[i] = (int16_t)((wp_sin[i] * modA) >> 7);
  for (int y = 0; y < H; y++) {
    wp_pattern(pat, y, cx, cy, modA, modPh);
    int rowMod = rowWave && modA ? (wp_sin[(y * modK + modPh) & 255] * modA) >> 7 : 0;
    bool pixMod = modA && !rowWave && pat != WI_STAR;                     // (a star wobbles its own points)
    wp_out(v3_buf + y * W, zm, rowMod, pixMod, modK, (y * modK + modPh) & 255);
  }
  v7_end(L.scan);
}

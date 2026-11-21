// =====================================================================
// PROGRAM: WEAVE — woven ribbons (by Dewey) — Set 7, slot 5
//
// Ribbons running up and down pass over and under ribbons running across:
// plain weave, twill, basket weave, satin. Shade each ribbon like a tube,
// let one cast a shadow where it dips under the other, and the flat screen
// turns into cloth you could pick at with a fingernail — then let the
// ribbons wave, bulge and slide, and it turns into op art (Riley's
// "Rustle", the striped weaves of the 1960s, a tartan, a cane chair).
//
// How it works: the cheapest picture in the set. Everything about a row —
// which across-ribbon it is in, how far across it, which ribbon is on top at
// each crossing, the shading and the shadows — goes into a 128-entry table
// at the start of the row. Then each pixel is just: where am I across the
// columns (one step) → that table. Waves and bulges only change where each
// row starts and how the columns are spaced.
//
// Presets: k0 Op Weave · k1 Twill · k2 Basket · k3 Wave · k4 Tartan · k5 Lattice ·
//          k6 Bulge · k7 Ribbons · k8 Satin · k9 Drift · k10 Kinetic · k11 Tour
//   k12 — PULL: the ribbons slide along by one crossing
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Ribbon Width · p6 Lines (stripes along the ribbons) · p7 Wave · p8 Pattern
//   p9 Relief · p10 Drift · p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { WV_PLAIN, WV_TWILL, WV_BASKET, WV_SATIN, WV_NPAT };
static const uint8_t wv_pats[WV_NPAT][4] = {                             // per across-ribbon (row bits = down-ribbon 0-3): 1 = the up-down ribbon is on top
  { 0x5, 0xA, 0x5, 0xA },   // plain
  { 0x3, 0x6, 0xC, 0x9 },   // twill
  { 0x3, 0x3, 0xC, 0xC },   // basket
  { 0x1, 0x4, 0x2, 0x8 },   // satin (one in four over)
};
// size: px a ribbon + gap · width: ribbon %, gap rest · lines: stripes along each ribbon · wave px · relief % ·
// drift: sliding · warp: 1 bulge · hues: 0 two colours (ink & paper), 1 colours by ribbon
struct WvLook { uint8_t pat, size, width, lines, wave, relief, drift, warp, hues, pal, flow, spread, sat; };
static const WvLook wv_looks[11] = {
  // pattern     size wid lines wave rel drift warp hue pal            flow spr sat
  { WV_PLAIN , 32,  86, 4,  0,  30, 10, 0, 0, O7P_BW      , 0,  50,   0 },   // Op Weave
  { WV_TWILL , 26,  92, 0,  0,  70, 12, 0, 1, O7P_RA      , 2,  60,  90 },   // Twill
  { WV_BASKET, 22,  90, 0,  0,  65,  8, 0, 1, O7P_INK     , 0,  40, 100 },   // Basket
  { WV_PLAIN , 30,  84, 3, 22,  30,  0, 0, 1, O7P_STIJL   , 3,  90, 100 },   // Wave
  { WV_TWILL , 20, 100, 2,  0,  25,  6, 0, 1, O7P_STIJL   , 2,  80, 100 },   // Tartan
  { WV_PLAIN , 40,  45, 0,  0, 100, 10, 0, 1, O7P_NEON    , 3, 100, 100 },   // Lattice
  { WV_PLAIN , 28,  86, 0,  0,  80,  0, 1, 1, O7P_VEGA    , 3,  70, 100 },   // Bulge
  { WV_PLAIN , 56,  80, 0, 10, 100,  6, 0, 1, O7P_SPECTRUM, 3, 100, 100 },   // Ribbons
  { WV_SATIN , 24,  96, 0,  0,  60, 10, 0, 1, O7P_ALBERS  , 2,  60,  90 },   // Satin
  { WV_PLAIN , 34,  80, 2,  0,  50, 45, 0, 1, O7P_KITAOKA , 2,  60, 100 },   // Drift
  { WV_PLAIN , 36,  90, 6, 14,  20, 12, 0, 1, O7P_RA      , 3,  90, 100 },   // Kinetic
};
static const char* const wv_names[12] = { "Op Weave", "Twill", "Basket", "Wave", "Tartan", "Lattice", "Bulge", "Ribbons", "Satin", "Drift", "Kinetic", "Tour" };
static const char* const wv_lbl[6] = { "Ribbon Width", "Lines", "Wave", "Pattern", "Relief", "Drift" };
const char* prog_weave_name() { return "WEAVE"; }
const char* prog_weave_character() { return "Woven ribbons over and under: plain, twill, basket, satin, waving and bulging"; }
const char* prog_weave_presetName(int p) { return (p >= 0 && p < 12) ? wv_names[p] : NULL; }
const char* prog_weave_potLabel(int p, int pot) { (void)p; return o7_label(pot, wv_lbl); }
uint8_t prog_weave_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the column states over W + 96 (which up-down ribbon, how far across: (ix & 3) << 5 | 32nds),
// and every possible row table: 4 across-ribbons × 32 positions down one × 128 column states = 16 KB,
// remade only when the weave itself changes (not every frame)
#define WV_CW (W + 96)
static uint8_t *wv_col, *wv_rows;
static void wv_mem() { wv_col = v6_mem; wv_rows = v6_mem + 512; }
static int32_t wv_madeKey = -1;
static float wv_width, wv_lines, wv_wave, wv_relief, wv_driftK, wv_pu = 0, wv_pv = 0, wv_pull = 0, wv_pullT = 0, wv_wt = 0;
void prog_weave_init() { wv_mem(); v6_init(); wv_pull = wv_pullT = 0; wv_madeKey = -1; }

__attribute__((optimize("O2"))) static void wv_row(uint8_t* out, const uint8_t* col, const uint8_t* rt) {
  for (int x = 0; x < W; x += 4) O7_PUT4(out + x, rt[col[x]], rt[col[x + 1]], rt[col[x + 2]], rt[col[x + 3]]);
}

void prog_weave_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  wv_mem();
  const WvLook& L = wv_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&wv_width, o7_kn(5, L.width / 100.0f), rate);
  v6_ease(&wv_lines, v3_centre(6) ? (float)L.lines : v3_u(6) * 8.0f, rate);
  v6_ease(&wv_wave, v3_centre(7) ? o7_vary(7, (float)L.wave) : v3_u(7) * 40.0f, rate);
  v6_ease(&wv_relief, o7_kn(9, L.relief / 100.0f), rate);
  v6_ease(&wv_driftK, v3_centre(10) ? o7_vary(10, L.drift / 100.0f) : v3_bi(10), rate);
  int pat = o7_pick(8, L.pat, WV_NPAT);
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt, t = v6_t;
  if (v6_k12hit) wv_pullT += 1.0f;
  v6_ease(&wv_pull, wv_pullT, 3.0f);
  wv_pu += sdt * wv_driftK * 0.7f; wv_pv += sdt * wv_driftK * 0.45f;
  if (wv_pu > 400) { wv_pu -= 400; } if (wv_pv > 400) { wv_pv -= 400; }
  if (wv_pu < -400) { wv_pu += 400; } if (wv_pv < -400) { wv_pv += 400; }
  if (wv_pullT > 400) { wv_pullT -= 400; wv_pull -= 400; }
  wv_wt += sdt;
  float z = v6_zoom(1.0f);
  float cell = L.size * z;
  int wr = v3_clamp((int)(wv_width * 32 + 0.5f), 4, 32);                  // ribbon width in 32nds of a cell
  int nl = (int)(wv_lines + 0.5f);
  // ── a ribbon's look across its width (32nds): tube shading and stripes along it ──
  int prof[32];
  for (int f = 0; f < 32; f++) {
    if (f >= wr) { prof[f] = -1; continue; }                               // the gap
    int c = (2 * f + 1 - wr) * 16 / wr;                                    // −16 … 16 across
    int tube = 15 - (int)(wv_relief * (c * c) * 10 / 256);                 // brighter in the middle
    if (nl > 0 && ((f * nl * 2 / wr) & 1)) tube = (int)(tube * (1.0f - 0.85f * (1.0f - wv_relief * 0.4f)));   // dark stripes along
    prof[f] = v3_clamp(tube, 1, 15);
  }
  // ── columns: which up-down ribbon and how far across, squeezed by the bulge ──
  float bc = HALFW + 90.0f * o7_nsin(t * 0.27f), bcy = HALFH + 60.0f * o7_nsin(t * 0.33f);
  bool bulge = L.warp == 1;
  uint32_t U = (uint32_t)(int32_t)((wv_pu + wv_pull) * 65536.0f) - (uint32_t)(int32_t)(65536.0f * (HALFW + 48) / cell) + (65536u << 8);
  for (int x = 0; x < WV_CW; x++) {
    float d = 65536.0f / cell;
    if (bulge) { float u = (x - 48 - bc) / 60.0f; d *= 1.0f - 0.55f / (1.0f + u * u); }
    wv_col[x] = (uint8_t)((U >> 11) & 127);
    U += (uint32_t)(int32_t)d;
  }
  // ── the row tables (only when the weave changes) ──
  int hl = (int)(wv_relief * 9);                                           // the shadow under a crossing ribbon
  int32_t key = wr | (nl << 6) | ((int)(wv_relief * 31) << 10) | (pat << 15) | (L.hues << 17) | ((L.pal == O7P_BW) << 18);
  if (key != wv_madeKey || v6_first) {
    wv_madeKey = key;
    for (int iy = 0; iy < 4; iy++) for (int fy = 0; fy < 32; fy++) {
      bool hOn = prof[fy] >= 0;
      int hHue = L.hues ? o7_mod15[iy * 4 + 1] : O7_GREY;
      uint8_t over = wv_pats[pat][iy];
      uint8_t* rt = wv_rows + ((iy * 32 + fy) << 7);
      for (int s = 0; s < 128; s++) {
        int ix = s >> 5, fx = s & 31;
        bool vOn = prof[fx] >= 0, vTop = (over >> ix) & 1;
        int vHue = L.hues ? o7_mod15[ix * 3 + 7] : O7_GREY;
        uint8_t c;
        if (vOn && (vTop || !hOn)) {
          int sh = prof[fx];
          if (!vTop && hl) { int e = fy < wr - 1 - fy ? fy : wr - 1 - fy; if (e < 3) sh -= hl * (3 - e) / 3; }   // under: shadowed where it dips
          c = O7C(v3_clamp(sh, 0, 15), vHue);
        } else if (hOn) {
          int sh = prof[fy];
          if (vTop && hl && fx >= wr) { int e = fx - wr < 31 - fx ? fx - wr : 31 - fx; if (e < 3) sh -= hl * (3 - e) / 3; }   // shadow beside the ribbon on top
          c = O7C(v3_clamp(sh, 0, 15), hHue);
        } else c = O7C(0, L.pal == O7P_BW ? O7_GREY : 0);                  // the gaps: ink
        rt[s] = c;
      }
    }
  }
  // ── rows: pick the table, slide for the wave ──
  uint32_t V = (uint32_t)(int32_t)((wv_pv - wv_pull * 0.5f) * 65536.0f) - (uint32_t)(int32_t)(65536.0f * HALFH / cell) + (65536u << 8);
  for (int y = 0; y < H; y++) {
    int iy = (int)((V >> 16) & 3), fy = (int)((V >> 11) & 31);
    int sft = 48;
    if (wv_wave > 0.5f) sft = 48 + (int)(wv_wave * sinf(y * 0.045f + wv_wt * 1.3f));
    wv_row(v3_buf + y * W, wv_col + v3_clamp(sft, 0, 96), wv_rows + ((iy * 32 + fy) << 7));
    float dV = 65536.0f / cell;
    if (bulge) { float u = (y - bcy) / 50.0f; dV *= 1.0f - 0.55f / (1.0f + u * u); }
    V += (uint32_t)(int32_t)dV;
  }
  o7_end();
}

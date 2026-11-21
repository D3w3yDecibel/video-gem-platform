// =====================================================================
// PROGRAM: CUBES — tumbling blocks (by Dewey) — Set 7, slot 4
//
// Three diamonds — light, middle, dark — make a hexagon that the eye can't
// help seeing as a cube; tile them and the cubes stack into a staircase
// that keeps flipping over: now you look down on them, now up from below.
// It's the oldest op-art pattern there is (Roman floors, quilts, Escher,
// Vasarely's "Tlinko" and "Kezdi"). Here the cubes slide, flip in waves,
// bulge, fill with stripes and take on colours.
//
// How it works: the cubes sit on a hexagonal grid. With the grid upright,
// where a pixel falls across the grid depends only on its column, and down
// it only on its row — so both are tables, made once a frame (which is
// also how the grid can be squeezed and bulged for free). Per pixel: pick
// the nearer of two hexagon centres, then which of the cube's three faces
// by two comparisons, then one table gives the shade and hue.
//
// Presets: k0 Tumbling · k1 Escher · k2 Striped · k3 Necker · k4 Bulge · k5 Rainbow ·
//          k6 Outline · k7 Breathe · k8 Stairs · k9 Wave · k10 Mosaic · k11 Tour
//   k12 — FLIP: a wave of flipping runs across the cubes
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (cube size) · p3 Colour Flow · p4 Hue Spread
//   p5 Shading · p6 Stripes · p7 Flip (◄ from above | from below ►) · p8 Warp · p9 Slide · p10 Edges
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { TB_PLAIN, TB_STRIPED };
// size: cube width px · shade: contrast % · stripes: lines per face (0 = plain) · flip: −1 from above … sweeps ·
// warp: 0 none, 1 bulge, 2 wave (rows slide), 3 breathe · slide: drift speed · edges: hexagon outlines ·
// hues: 0 = one colour per face, 1 = each cube its own colour · cycle: face shades rotate (stairs)
struct TbLook { uint8_t size, shade, stripes, flip, warp, slide, edges, hues, cycle, pal, flow, spread, sat; };
enum { TB_UP, TB_DOWN, TB_SWEEP };
static const TbLook tb_looks[11] = {
  // size shade strp flip      warp slide edg hue cyc pal             flow spr sat
  { 40, 100, 0, TB_UP   , 0, 15,  0, 0, 0, O7P_BW      , 0,  50,   0 },   // Tumbling
  { 44, 100, 0, TB_UP   , 0, 10,  0, 0, 0, O7P_STIJL   , 2, 100, 100 },   // Escher
  { 56, 100, 3, TB_UP   , 0, 12,  0, 1, 0, O7P_SPECTRUM, 3,  90, 100 },   // Striped
  { 40, 100, 0, TB_SWEEP, 0,  8,  0, 1, 0, O7P_ANUSZ   , 3,  90, 100 },   // Necker
  { 34, 100, 0, TB_UP   , 1,  0,  0, 0, 0, O7P_VEGA    , 3,  60,  90 },   // Bulge
  { 36,  90, 0, TB_UP   , 0, 15,  0, 1, 0, O7P_SPECTRUM, 3, 100,  90 },   // Rainbow
  { 44,  45, 0, TB_UP   , 0, 12, 60, 0, 0, O7P_INK     , 0,  50, 100 },   // Outline
  { 40, 100, 0, TB_UP   , 3,  0,  0, 1, 0, O7P_RA      , 2,  70, 100 },   // Breathe
  { 40, 100, 0, TB_UP   , 0,  0,  0, 1, 1, O7P_AGAM    , 3,  90, 100 },   // Stairs
  { 36, 100, 0, TB_UP   , 2,  0,  0, 0, 0, O7P_KITAOKA , 2,  60, 100 },   // Wave
  { 20,  90, 0, TB_UP   , 0, 20,  0, 1, 0, O7P_AGAM    , 4, 100, 100 },   // Mosaic
};
static const char* const tb_names[12] = { "Tumbling", "Escher", "Striped", "Necker", "Bulge", "Rainbow", "Outline", "Breathe", "Stairs", "Wave", "Mosaic", "Tour" };
static const char* const tb_lbl[6] = { "Shading", "Stripes", "Flip: Above|Below", "Warp", "Slide", "Edges" };
const char* prog_cubes_name() { return "CUBES"; }
const char* prog_cubes_character() { return "Tumbling blocks: a staircase of cubes that keeps flipping inside out"; }
const char* prog_cubes_presetName(int p) { return (p >= 0 && p < 12) ? tb_names[p] : NULL; }
const char* prog_cubes_potLabel(int p, int pot) { (void)p; return o7_label(pot, tb_lbl); }
uint8_t prog_cubes_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// The hexagonal pattern repeats every cube across (U, 65536) and every row of cubes down (V, 65536 = √3 widths),
// so what a pixel shows — which grid's cube (A, or B half a step over), which face, where across its stripes,
// whether it is on an outline — depends only on its position inside one repeat. That is one table:
// 128 rows × 128 columns, made only when the outlines change. Per pixel: column position (a table, so
// bulges are free) + the row's line of the table → which face → its colour.
#define TB_CW (W + 128)
#define TB_TR 128
#define TB_TC 128
static uint8_t *tb_tab, *tb_cs, *tb_ia, *tb_ib, *tb_face;
static void tb_mem() {
  tb_tab = v6_mem;                                      // 16384: bit 0-1 face (3 = outline), bit 2 grid B, bits 3-7 stripe position
  tb_cs = v6_mem + 16384;                               // 448: column → position across (0-127)
  tb_ia = v6_mem + 16832; tb_ib = tb_ia + TB_CW;         // 896: the cube number across, grid A and B
  tb_face = v6_mem + 17728;                             // 4 faces × 16 hues × 32 stripe positions = 2048
}
static int tb_madeEdge = -1;
static float tb_shade, tb_strp, tb_flipK, tb_warpK, tb_slideK, tb_edgeK, tb_pu = 0, tb_pv = 0, tb_sweep = -1, tb_cyc = 0, tb_wt = 0;

void prog_cubes_init() { tb_mem(); v6_init(); tb_sweep = -1; tb_madeEdge = -1; }

static void tb_makeTable(int edgeT) {
  for (int r = 0; r < TB_TR; r++) {
    int32_t V = (r * 65536 + 32768) / TB_TR;
    int32_t ay = ((int16_t)(uint16_t)V) >> 5, by = ((int16_t)(uint16_t)(V + 32768)) >> 5;
    for (int c = 0; c < TB_TC; c++) {
      int32_t U = (c * 65536 + 32768) / TB_TC;
      int32_t ax = ((int16_t)(uint16_t)U) >> 5, bx = ((int16_t)(uint16_t)(U + 32768)) >> 5;
      int32_t da = ax * ax + 3 * ay * ay, db = bx * bx + 3 * by * by;
      int lx, ly, g;
      if (da <= db) { lx = ax; ly = ay; g = 0; } else { lx = bx; ly = by; g = 4; }
      int alx = lx < 0 ? -lx : lx;
      int f = (ly < 0 && alx < -3 * ly) ? 0 : (lx < 0 ? 1 : 2);             // top, left, right
      int st;                                                             // position across the face's stripes
      if (f == 0) st = (lx - 3 * ly) >> 5; else if (f == 1) st = (-lx) >> 5; else st = (lx + 3 * ly) >> 5;
      if (edgeT) { int e = db - da; if (e < 0) e = -e; if (e < edgeT) f = 3; }
      tb_tab[(r << 7) | c] = (uint8_t)(f | g | ((st & 31) << 3));
    }
  }
}
// one row: the table row (already chosen for this row of the screen), the column positions, the hues
__attribute__((optimize("O2"))) static void tb_row(uint8_t* out, const uint8_t* trow, const uint8_t* cs, const uint8_t* ft) {
  for (int x = 0; x < W; x += 2) {
    uint8_t a = trow[cs[x]], b = trow[cs[x + 1]];
    out[x] = ft[((a & 3) << 9) | (a >> 3)]; out[x + 1] = ft[((b & 3) << 9) | (b >> 3)];
  }
}
// … with each cube its own hue
__attribute__((optimize("O2"))) static void tb_rowHue(uint8_t* out, const uint8_t* trow, const uint8_t* cs, const uint8_t* ia, const uint8_t* ib, int ha, int hb) {
  const uint8_t* ft = tb_face;
  for (int x = 0; x < W; x++) {
    uint8_t e = trow[cs[x]];
    int h = (e & 4) ? ib[x] + hb : ia[x] + ha;
    out[x] = ft[((e & 3) << 9) | ((h & 15) << 5) | (e >> 3)];
  }
}

void prog_cubes_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  tb_mem();
  const TbLook& L = tb_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&tb_shade, o7_kn(5, L.shade / 100.0f), rate);
  v6_ease(&tb_strp, v3_centre(6) ? (float)L.stripes : v3_u(6) * 12.0f, rate);
  v6_ease(&tb_flipK, v3_centre(7) ? (L.flip == TB_DOWN ? 1.0f : 0.0f) : v3_u(7), rate);
  v6_ease(&tb_warpK, v3_centre(8) ? (L.warp ? 1.0f : 0.0f) : v3_u(8), rate);
  v6_ease(&tb_slideK, v3_centre(9) ? o7_vary(9, L.slide / 100.0f) : v3_bi(9), rate);
  v6_ease(&tb_edgeK, o7_kn(10, L.edges / 100.0f), rate);
  int warp = L.warp ? L.warp : (tb_warpK > 0.02f ? 1 : 0);
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt, t = v6_t;
  float z = v6_zoom(1.0f);
  float w = L.size * z;                                                    // cube width (px between neighbours across)
  if (warp == 3) w *= 1.0f + 0.35f * tb_warpK * o7_nsin(t * 0.9f);
  int edgeT = (int)(tb_edgeK * tb_edgeK * 9000.0f);
  if (edgeT != tb_madeEdge) { tb_makeTable(edgeT); tb_madeEdge = edgeT; }
  // ── the face colours: shade by face (light top, dark left, middle right), stripes optional; face 3 = outline ──
  int sh[3] = { 15, 2, 8 };
  if (L.cycle) { tb_cyc += sdt * 0.25f; int k = ((int)tb_cyc) % 3; int r[3]; for (int i = 0; i < 3; i++) r[i] = sh[(i + k) % 3]; for (int i = 0; i < 3; i++) sh[i] = r[i]; }
  for (int i = 0; i < 3; i++) sh[i] = 8 + (int)((sh[i] - 8) * tb_shade);
  int ns = (int)(tb_strp + 0.5f);
  for (int f = 0; f < 4; f++) for (int h = 0; h < 16; h++) {
    int ff = f < 3 ? f : 0;
    int hue = L.hues ? (L.pal == O7P_BW ? O7_GREY : o7_mod15[(h * 4 + ff) % 60]) : (L.pal == O7P_BW ? O7_GREY : o7_mod15[ff * 5]);
    uint8_t* d = tb_face + (f << 9) + (h << 5);
    for (int st = 0; st < 32; st++) {
      int v = sh[ff];
      if (ns > 0) { int ph = (st * ns * 2) & 63; v = ph < 32 ? (ff == 0 ? 15 : sh[ff]) : (ff == 0 ? 0 : sh[ff] / 3); }   // stripes on each face
      if (f == 3) v = 0;                                                    // the outline: ink
      d[st] = (uint8_t)((v3_clamp(v, 0, 15) << 4) | hue);
    }
  }
  // ── columns: position across (U, 65536 a cube), squeezed by the bulge if on ──
  tb_pu += sdt * tb_slideK * 0.6f; tb_pv += sdt * tb_slideK * 0.25f;
  if (tb_pu > 1000 || tb_pu < -1000) tb_pu = 0; if (tb_pv > 1000 || tb_pv < -1000) tb_pv = 0;
  float bc = HALFW + 100.0f * o7_nsin(t * 0.23f), bcy = HALFH + 60.0f * o7_nsin(t * 0.31f);
  float bw = 0.6f * tb_warpK;
  int32_t U = (int32_t)(tb_pu * 65536.0f) - (int32_t)(65536.0f * (HALFW + 64) / w) + 65536 * 64;
  int32_t dU0 = (int32_t)(65536.0f / w);
  for (int x = 0; x < TB_CW; x++) {
    tb_cs[x] = (uint8_t)(((uint32_t)U >> 9) & 127);
    tb_ia[x] = (uint8_t)((uint32_t)U >> 16); tb_ib[x] = (uint8_t)((uint32_t)(U + 32768) >> 16);
    if (warp == 1) { float u = (x - 64 - bc) / 70.0f; U += (int32_t)(dU0 * (1.0f - bw / (1.0f + u * u))); }   // bigger cubes under the bulge
    else U += dU0;
  }
  // ── rows: position down (V, 65536 = √3 cube widths), flip ──
  if (v6_k12hit) tb_sweep = 0;
  if (tb_sweep >= 0) { tb_sweep += v6_f.dt * 0.6f; if (tb_sweep > 1.4f) tb_sweep = -1; }
  float hV = w * 1.7320508f;
  int32_t V = (int32_t)(tb_pv * 65536.0f) - (int32_t)(65536.0f * HALFH / hV) + 65536 * 64;
  int32_t dV0 = (int32_t)(65536.0f / hV);
  tb_wt += sdt;
  float sweepY = HALFH + 110.0f * o7_nsin(t * 0.35f), sweepA = 40.0f * o7_nsin(t * 0.6f);
  float swY = tb_sweep >= 0 ? tb_sweep * (H + 120) - 60 : -1e9f;
  for (int y = 0; y < H; y++) {
    // seen from below? (the knob, the Necker sweep, or the FLIP wave)
    bool below = tb_flipK > 0.5f;
    if (L.flip == TB_SWEEP && v3_centre(7)) below = (y + sweepA) > sweepY;
    if (y < swY && y > swY - 120) below = !below;
    int r = (int)(((uint32_t)V >> 9) & 127);
    if (below) r = 127 - r;                                                  // from below: the repeat upside down
    int s = 64;
    if (warp == 2) s = 64 + (int)(40.0f * tb_warpK * sinf(y * 0.035f + tb_wt * 1.2f));
    const uint8_t* trow = tb_tab + (r << 7);
    if (L.hues) {
      int ha = (int)(((uint32_t)V >> 16) * 7), hb = (int)(((uint32_t)(V + 32768) >> 16) * 7 + 3);
      tb_rowHue(v3_buf + y * W, trow, tb_cs + s, tb_ia + s, tb_ib + s, ha, hb);
    } else tb_row(v3_buf + y * W, trow, tb_cs + s, tb_face);
    if (warp == 1) { float u = (y - bcy) / 60.0f; V += (int32_t)(dV0 * (1.0f - bw / (1.0f + u * u))); }
    else V += dV0;
  }
  o7_end();
}

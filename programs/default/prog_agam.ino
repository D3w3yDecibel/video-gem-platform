// =====================================================================
// PROGRAM: AGAM — Yaacov Agam's changing pictures (by Dewey) — Set 7, slot 10
//
// Agam painted on a corrugated surface — a row of folded slats like a
// zig-zag fan — one picture on the left-facing sides and another on the
// right. Walk past and the first picture dissolves, slat by slat, into the
// second; stand in the middle and you see both, woven into each other.
// His later works use three-sided prisms for three pictures. Here your
// "walk past" is the View knob — or an automatic sweep — and the pictures
// move as well.
//
// How it works: each frame, for every column, which face of its slat you
// see from this angle, and how brightly that face is lit. For each row the
// pictures (rings, squares, stripes, chequers… all cheap sums and table
// lookups) are made side by side, and each pixel takes its colour from the
// picture its column shows, through that face's light.
//
// Presets: k0 Agamograph · k1 Day & Night · k2 Prism · k3 Flags · k4 Rainbow & Grey ·
//          k5 Pulse · k6 Fine · k7 Wide · k8 Sweep · k9 Waves · k10 Tricolour · k11 Tour
//   k12 — TURN: the view swings right across and back
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Slat Width · p6 View (centre = automatic sweep) · p7 Sweep Speed · p8 Shading
//   p9 Picture A · p10 Picture B · p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { GA_RINGS, GA_SQUARES, GA_DIAMONDS, GA_VSTRIPES, GA_HSTRIPES, GA_CHECKER, GA_DIAG, GA_WAVES, GA_NPIC };
// pictures A, B (and C for prisms) · slat px · faces 2 or 3 · sweep speed · shading % · motion: pictures move ·
// colour: per picture, hue step (0 = black & white)
struct GaLook { uint8_t a, b, c, slat, faces, sweep, shade, motion, hueA, hueB, hueC, pal, flow, spread, sat; };
static const GaLook ga_looks[11] = {
  //  A            B            C           slat fc swp shd mot hA hB hC pal             flow spr sat
  { GA_SQUARES , GA_HSTRIPES,           0, 16, 2, 30, 50, 20, 1, 0, 0, O7P_AGAM    , 2, 100, 100 },   // Agamograph
  { GA_RINGS   , GA_DIAMONDS,           0, 10, 2, 25, 50, 25, 1, 3, 0, O7P_VEGA    , 3,  80,  90 },   // Day & Night
  { GA_SQUARES , GA_RINGS   , GA_CHECKER , 15, 3, 30, 45, 20, 1, 2, 0, O7P_AGAM    , 3, 100, 100 },   // Prism
  { GA_HSTRIPES, GA_VSTRIPES,           0, 12, 2, 30, 35, 10, 1, 0, 0, O7P_STIJL   , 2,  80, 100 },   // Flags
  { GA_DIAG    , GA_CHECKER ,           0, 10, 2, 30, 40, 20, 1, 0, 0, O7P_SPECTRUM, 3, 100, 100 },   // Rainbow & Grey
  { GA_RINGS   , GA_SQUARES ,           0, 12, 2, 20, 40, 45, 1, 2, 0, O7P_NEON    , 4, 100, 100 },   // Pulse
  { GA_SQUARES , GA_DIAG    ,           0,  8, 2, 30, 30, 20, 1, 0, 0, O7P_AGAM    , 2, 100, 100 },   // Fine
  { GA_RINGS   , GA_HSTRIPES,           0, 32, 2, 25, 80, 20, 1, 4, 0, O7P_RA      , 2,  90,  90 },   // Wide
  { GA_DIAMONDS, GA_VSTRIPES,           0, 12, 2, 50, 50, 20, 1, 0, 0, O7P_AGAM    , 3, 100, 100 },   // Sweep
  { GA_WAVES   , GA_RINGS   ,           0, 12, 2, 30, 40, 40, 1, 3, 0, O7P_KITAOKA , 2,  80, 100 },   // Waves
  { GA_HSTRIPES, GA_DIAG    , GA_VSTRIPES, 15, 3, 30, 40, 20, 1, 4, 7, O7P_STIJL   , 2, 100, 100 },   // Tricolour
};
static const char* const ga_names[12] = { "Agamograph", "Day & Night", "Prism", "Flags", "Rainbow & Grey", "Pulse", "Fine", "Wide", "Sweep", "Waves", "Tricolour", "Tour" };
static const char* const ga_lbl[6] = { "Slat Width", "View (centre = auto)", "Sweep Speed", "Shading", "Picture A", "Picture B" };
const char* prog_agam_name() { return "AGAM"; }
const char* prog_agam_character() { return "Agam's folded pictures: one image turns into another as the view swings across"; }
const char* prog_agam_presetName(int p) { return (p >= 0 && p < 12) ? ga_names[p] : NULL; }
const char* prog_agam_potLabel(int p, int pot) { (void)p; return o7_label(pot, ga_lbl); }
uint8_t prog_agam_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: each picture's stripe field (3 × 1024: 16 stripes × 64 steps), the face per column,
// each face's lighting table (3 × 256), the row of each picture (3 × 320), √ table, |x| and x² per column
static uint8_t *ga_comb, *ga_face, *ga_lit, *ga_rows; static uint16_t *ga_sq, *ga_xl; static int16_t* ga_ax; static int32_t* ga_x2;
static void ga_mem() {
  ga_comb = v6_mem;                          // 3072
  ga_face = v6_mem + 3072;                   // 320: the picture (bits 0-1) and the fold (bit 2) per column
  ga_lit = v6_mem + 3392;                    // 8 × 256 lighting tables (face, fold)
  ga_rows = v6_mem + 5440;                   // 960
  ga_sq = (uint16_t*)(v6_mem + 6400);        // 2560 × 2 = 5120  (r² / 16 → r × 4)
  ga_ax = (int16_t*)(v6_mem + 11520);        // 640
  ga_x2 = (int32_t*)(v6_mem + 12160);        // 1280
  ga_xl = (uint16_t*)(v6_mem + 13440);       // 640: thin slats — the columns of each face, face by face
}
static bool ga_sqOK = false;
static float ga_slat, ga_view = 0, ga_sweepK, ga_shadeK, ga_sph = 0, ga_t = 0, ga_turn = -1;
void prog_agam_init() {
  ga_mem(); v6_init(); ga_turn = -1;
  for (int i = 0; i < 2560; i++) ga_sq[i] = (uint16_t)v3_isqrt((uint32_t)(i * 16 + 8) * 16);
  ga_sqOK = true;
}

// one stretch [x0, x1) of a row of a picture, through a face's lighting:
// phase (65536 a stripe) → the picture's stripe field (1024: 16 stripes × 64 steps) → lit colour
__attribute__((optimize("O2"))) static void ga_pic(uint8_t* out, int x0, int x1, int pic, int y, int dy, uint32_t ph, uint32_t k, const uint8_t* comb, const uint8_t* lit, int t) {
  const int16_t* ax = ga_ax; const int32_t* x2 = ga_x2;
  int ady = dy < 0 ? -dy : dy;
  switch (pic) {
    case GA_RINGS: { int32_t dy2 = dy * dy; for (int x = x0; x < x1; x++) { uint32_t q = (uint32_t)(x2[x] + dy2) >> 4; out[x] = lit[comb[((ga_sq[q < 2560 ? q : 2559] * k + ph) >> 10) & 1023]]; } } break;
    case GA_SQUARES: for (int x = x0; x < x1; x++) { int m = ax[x] > ady ? ax[x] : ady; out[x] = lit[comb[((m * 4 * k + ph) >> 10) & 1023]]; } break;
    case GA_DIAMONDS: for (int x = x0; x < x1; x++) out[x] = lit[comb[(((ax[x] + ady) * 3 * k + ph) >> 10) & 1023]]; break;
    case GA_VSTRIPES: for (int x = x0; x < x1; x++) out[x] = lit[comb[(((uint32_t)x * 4 * k + ph) >> 10) & 1023]]; break;
    case GA_HSTRIPES: { uint8_t c = lit[comb[(((uint32_t)y * 4 * k + ph) >> 10) & 1023]]; memset(out + x0, c, x1 - x0); } break;
    case GA_CHECKER: { uint32_t yy = ((uint32_t)y * 4 * k + ph) >> 15; uint8_t c0 = lit[comb[12]], c1 = lit[comb[512 + 44]];
                       for (int x = x0; x < x1; x++) out[x] = ((((uint32_t)x * 4 * k) >> 15) ^ yy) & 1 ? c1 : c0; } break;
    case GA_DIAG: for (int x = x0; x < x1; x++) out[x] = lit[comb[((((uint32_t)(x + y)) * 3 * k + ph) >> 10) & 1023]]; break;
    default: { int s = v3_sin(y * 8 + t) >> 9; for (int x = x0; x < x1; x++) out[x] = lit[comb[((((uint32_t)(x + s + 64)) * 4 * k + ph) >> 10) & 1023]]; }
  }
}
// the same, for a list of columns (thin slats: all the columns that show one face)
__attribute__((optimize("O2"))) static void ga_picL(uint8_t* out, const uint16_t* xl, int n, int pic, int y, int dy, uint32_t ph, uint32_t k, const uint8_t* comb, const uint8_t* lit, int t) {
  const int16_t* ax = ga_ax; const int32_t* x2 = ga_x2;
  int ady = dy < 0 ? -dy : dy;
  const uint16_t* e = xl + n;
  switch (pic) {
    case GA_RINGS: { int32_t dy2 = dy * dy; for (; xl < e; xl++) { int x = *xl; uint32_t q = (uint32_t)(x2[x] + dy2) >> 4; out[x] = lit[comb[((ga_sq[q < 2560 ? q : 2559] * k + ph) >> 10) & 1023]]; } } break;
    case GA_SQUARES: { uint32_t k4 = 4 * k, my = ady * k4 + ph;
                       for (; xl < e; xl++) { int x = *xl, a = ax[x]; out[x] = lit[comb[((a > ady ? a * k4 + ph : my) >> 10) & 1023]]; } } break;
    case GA_DIAMONDS: { uint32_t k3 = 3 * k, b = ady * k3 + ph; for (; xl < e; xl++) { int x = *xl; out[x] = lit[comb[((ax[x] * k3 + b) >> 10) & 1023]]; } } break;
    case GA_VSTRIPES: { uint32_t k4 = 4 * k; for (; xl < e; xl++) { int x = *xl; out[x] = lit[comb[((x * k4 + ph) >> 10) & 1023]]; } } break;
    case GA_HSTRIPES: { uint8_t c = lit[comb[(((uint32_t)y * 4 * k + ph) >> 10) & 1023]]; for (; xl < e; xl++) out[*xl] = c; } break;
    case GA_CHECKER: { uint32_t yy = ((uint32_t)y * 4 * k + ph) >> 15, k4 = 4 * k; uint8_t c0 = lit[comb[12]], c1 = lit[comb[512 + 44]];
                       for (; xl < e; xl++) { int x = *xl; out[x] = (((x * k4) >> 15) ^ yy) & 1 ? c1 : c0; } } break;
    case GA_DIAG: { uint32_t k3 = 3 * k, b = (uint32_t)y * k3 + ph; for (; xl < e; xl++) { int x = *xl; out[x] = lit[comb[((x * k3 + b) >> 10) & 1023]]; } } break;
    default: { int s = v3_sin(y * 8 + t) >> 9; uint32_t k4 = 4 * k, b = (uint32_t)(s + 64) * k4 + ph;
               for (; xl < e; xl++) { int x = *xl; out[x] = lit[comb[((x * k4 + b) >> 10) & 1023]]; } }
  }
}
#define GA_MAXS 330
static int16_t ga_sx[GA_MAXS]; static uint8_t ga_sf[GA_MAXS]; static int ga_ns = 0;   // the stretches of columns that show one face

void prog_agam_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  ga_mem();
  if (!ga_sqOK) prog_agam_init();
  const GaLook& L = ga_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&ga_slat, v3_centre(5) ? (float)L.slat : 3.0f + v3_u(5) * 40.0f, rate);
  v6_ease(&ga_sweepK, o7_kn(7, L.sweep / 100.0f), rate);
  v6_ease(&ga_shadeK, o7_kn(8, L.shade / 100.0f), rate);
  int pa = o7_pick(9, L.a, GA_NPIC), pb = o7_pick(10, L.b, GA_NPIC), pc = L.c;
  int faces = L.faces;
  o7_tint = 0.6f;                                                          // the 'black & white' pictures take a two-tone of the palette
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt;
  ga_sph += sdt * (0.05f + ga_sweepK * 1.2f);
  ga_t += sdt * L.motion * 0.03f;
  if (ga_t > 1000.0f) ga_t -= 1000.0f;
  if (v6_k12hit) ga_turn = 0;
  float view;                                                              // −1 (from the left) … 1 (from the right)
  if (v3_centre(6)) {
    float s = sinf(ga_sph); view = s * fabsf(s) * 0.4f + s * 0.6f;         // lingers a little at each side
  } else view = v3_bi(6);
  if (ga_turn >= 0) { ga_turn += v6_f.dt * 0.5f; view = sinf(ga_turn * 6.2832f); if (ga_turn >= 1.0f) ga_turn = -1; }
  v6_ease(&ga_view, view, 6.0f);
  // ── per column: which face shows, from this view ──
  float z = v6_zoom(1.0f);
  int S = v3_clamp((int)(ga_slat * z), 3, 80);
  float v = ga_view;
  // two faces: the left face shows over (1 − v) / 2 of the slat; three: left, middle, right
  float fl, fm;
  if (faces == 2) { fl = (1.0f - v) * 0.5f; fm = 0; }
  else { fl = v < 0 ? -v * 0.6f : 0.0f; float fr = v > 0 ? v * 0.6f : 0.0f; fm = 1.0f - fl - fr; }
  int b0 = (int)(fl * S + 0.5f), b1 = (int)((fl + fm) * S + 0.5f);
  int sOff = (int)(ga_t * 3.0f) % S;
  for (int x = 0; x < W; x++) {
    int p = (x + sOff) % S;
    int f = faces == 2 ? (p < b0 ? 0 : 1) : (p < b0 ? 0 : (p < b1 ? 2 : 1));
    bool fold = p == 0 || (p == b0 && b0 > 0 && b0 < S) || (faces == 3 && p == b1 && b1 > b0 && b1 < S);
    ga_face[x] = (uint8_t)(f | (fold ? 4 : 0));
  }
  // ── each face's light: brighter as it turns toward you ──
  for (int f = 0; f < 8; f++) {
    int ff = f & 3; if (ff == 3) ff = 0;
    float lit = ff == 0 ? 0.5f - 0.5f * v : (ff == 1 ? 0.5f + 0.5f * v : 1.0f - fabsf(v) * 0.5f);
    int k = (int)(256 * (1.0f - ga_shadeK * (1.0f - lit) * 0.8f));
    if (f & 4) k = k * (100 - (int)(ga_shadeK * 70)) / 100;                // the fold: a dark crease
    for (int i = 0; i < 256; i++) ga_lit[f * 256 + i] = (uint8_t)((((i >> 4) * k >> 8) << 4) | (i & 15));
  }
  // ── the pictures' stripe fields ──
  uint8_t band[64];
  o7_band(band, 64, 128, o7_edge(26.0f * z, 64, 0.1f));
  int hues[3] = { L.hueA, L.hueB, L.hueC };
  for (int p = 0; p < 3; p++) for (int j = 0; j < 16; j++) for (int i = 0; i < 64; i++) {
    int h = hues[p] ? o7_mod15[(j * hues[p] + p * 5) % 60] : O7_GREY;
    ga_comb[p * 1024 + (j << 6) + i] = (uint8_t)((band[i] << 4) | h);
  }
  // ── columns ──
  for (int x = 0; x < W; x++) { int d = x - HALFW; ga_ax[x] = (int16_t)(d < 0 ? -d : d); ga_x2[x] = d * d; }
  uint32_t kq = (uint32_t)(65536.0f / (26.0f * z * 4.0f));               // stripes about 26 px wide: phase (65536 a stripe) per ¼ px
  int tt = (int)(ga_t * 200);
  uint32_t phs[3] = { (uint32_t)(int32_t)(ga_t * 65536.0f), (uint32_t)(int32_t)(-ga_t * 45000.0f), (uint32_t)(int32_t)(ga_t * 30000.0f) };
  int pics[3] = { pa, pb, pc };
  // the stretches: runs of columns showing the same face (and fold)
  ga_ns = 0;
  for (int x = 0; x < W && ga_ns < GA_MAXS - 1; ) {
    int f = ga_face[x], e = x + 1;
    while (e < W && ga_face[e] == f) e++;
    ga_sx[ga_ns] = (int16_t)x; ga_sf[ga_ns] = (uint8_t)f; ga_ns++;
    x = e;
  }
  ga_sx[ga_ns] = W;
  if (ga_ns <= 24) {
    // wide slats: each stretch is drawn from its own picture, through its own light
    for (int y = 0; y < H; y++) {
      int dy = y - HALFH;
      uint8_t* out = v3_buf + y * W;
      for (int i = 0; i < ga_ns; i++) {
        int f = ga_sf[i], p = f & 3;
        ga_pic(out, ga_sx[i], ga_sx[i + 1], pics[p], y, dy, phs[p], kq, ga_comb + p * 1024, ga_lit + (f << 8), tt);
      }
    }
  } else {
    // thin slats (many short stretches): the columns of each face (and fold) are listed once, then
    // each row draws each list straight from its picture through its light
    int st[9] = { 0 };
    for (int x = 0; x < W; x++) st[ga_face[x] + 1]++;
    for (int f = 0; f < 8; f++) st[f + 1] += st[f];
    int at[8]; for (int f = 0; f < 8; f++) at[f] = st[f];
    for (int x = 0; x < W; x++) ga_xl[at[ga_face[x]]++] = (uint16_t)x;
    for (int y = 0; y < H; y++) {
      int dy = y - HALFH;
      uint8_t* out = v3_buf + y * W;
      for (int f = 0; f < 8; f++) {
        int n = st[f + 1] - st[f]; if (!n) continue;
        int p = f & 3;
        ga_picL(out, ga_xl + st[f], n, pics[p], y, dy, phs[p], kq, ga_comb + p * 1024, ga_lit + (f << 8), tt);
      }
    }
  }
  o7_end();
}

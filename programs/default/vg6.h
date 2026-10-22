// =====================================================================
// vg6.h — the Set 6 "Siren" engine (by Dewey)
//
// Set 6 is twelve programs on one theme: sacred geometry, hexagons and
// six-fold symmetry, glowing in shifting bands of colour, after Burt Vera
// Cruz's "Call of the Virgo Siren". Everything the twelve share is here:
//
//   • COLOURS — every pixel is (level << 4) | hue: 16 brightness levels of
//     16 hues. The 16 hues are a slice of one of twelve gradient PALETTES,
//     and the slice slowly slides along the gradient (Colour Flow), so the
//     whole picture's colours drift without redrawing anything. The top
//     levels turn to a pearly sheen, like light on enamel.
//   • COMMON KNOBS on all twelve programs:
//       p0 Palette (centre = the preset's own)   p1 Speed (far left = freeze)
//       p2 Zoom (centre = preset)                p3 Colour Flow (◄ backwards | forwards ►)
//       p4 Hue Spread (how much of the palette is on screen at once)
//       p12-p15 the live FX from vg5fx.h (warp, mirror, colour, glitch)
//     p1 and p12-p15 do nothing until you actually turn them, so a knob
//     left at an end can't freeze or scramble a program you switch to.
//   • TOUR — preset k11 on every program moves through presets k0-k10 by
//     itself, about every 20 seconds, fading between them.
//   • HEX MATHS — a fast hexagonal lattice: every pixel finds its nearest
//     hexagon centre and its position inside that hexagon with a handful of
//     integer operations, no square roots.
// =====================================================================
#ifndef VG6_H
#define VG6_H
#include "vg3.h"
#include "vg5fx.h"
#include "vg6_geo.h"

// ─── Shared working memory ────────────────────────────────────────────
// Only one program runs at a time, so the twelve share one block (Set 6's
// own, a little bigger than vg3's v3_scratch, which Set 6 doesn't use — so
// the linker leaves that one out). Each program sets up what it keeps here
// in its _init and never assumes what another program left behind.
#define V6_MEM 24576
static uint8_t v6_mem[V6_MEM] __attribute__((aligned(4)));

static inline int v6_min(int a, int b) { return a < b ? a : b; }
static inline int v6_max(int a, int b) { return a > b ? a : b; }
#define V6_Q 65
#define V6_QN (V6_Q * V6_Q)
static const uint8_t v6_fu[128] = {                                  // (c >> 9) & 127 → |… − 64|
  64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,
  32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63 };
static uint16_t v6_fv[128];                                          // the same × 65 (filled by v6_init)
#define V6C(l, h) ((uint8_t)(((l) << 4) | (h)))

// ─── Palettes: twelve gradients of six colours (they wrap round) ───────
#define V6_NPAL 12
static const uint8_t v6_pals[V6_NPAL][6][3] = {
  {{ 74,  10, 120}, {200,  40, 150}, {240, 170,  40}, {120, 150,  30}, { 20, 160, 150}, { 30,  50, 200}},   //  0 Siren: violet, magenta, gold, olive, teal, cobalt
  {{ 60,   0,  10}, {170,  30,  20}, {250, 170,  40}, {255, 230, 150}, {150,  90,  20}, { 20,  10,  30}},   //  1 Temple: oxblood and gold
  {{  0,  70,  90}, {  0, 170, 140}, { 40,  60, 200}, {220, 180,  40}, { 20, 120,  60}, { 90,  30, 160}},   //  2 Peacock
  {{255, 120, 170}, {255, 230, 240}, {200,  60, 120}, { 60,  30, 140}, {255, 170, 120}, {130,  90, 220}},   //  3 Lotus
  {{ 10,  20, 120}, { 90,  40, 220}, {  0, 200, 255}, {230, 250, 255}, { 60, 120, 255}, { 40,   0, 100}},   //  4 Oracle
  {{ 30,   0,   0}, {200,  20,   0}, {255, 120,   0}, {255, 230,  60}, {255,  70,  20}, { 90,   0,  20}},   //  5 Ember
  {{  0,  40,  30}, { 30, 160,  90}, {170, 240, 170}, { 20, 110, 110}, {200, 220,  90}, {  0,  80,  60}},   //  6 Jade
  {{250, 200, 220}, {180, 230, 255}, {210, 255, 210}, {255, 245, 190}, {220, 200, 255}, {255, 210, 180}},   //  7 Opal
  {{ 20,   0,  60}, {150,   0, 160}, {255,  60, 120}, { 30, 200, 255}, {  0,  40, 140}, {255, 200, 255}},   //  8 Nebula
  {{ 40,  20,   0}, {160, 100,  10}, {255, 210,  80}, {255, 250, 210}, {200, 140,  30}, { 90,  50,   5}},   //  9 Gilded
  {{ 30,   0,  80}, {140,   0, 255}, {255,   0, 200}, {  0, 255, 220}, { 80,  60, 255}, {200, 255, 255}},   // 10 Ultraviolet
  {{255,  30,  30}, {255, 220,   0}, { 30, 255,  60}, {  0, 200, 255}, { 40,  60, 255}, {230,   0, 255}},   // 11 Spectrum
};
static const char* const v6_palNames[V6_NPAL] = { "Siren", "Temple", "Peacock", "Lotus", "Oracle", "Ember", "Jade", "Opal", "Nebula", "Gilded", "Ultraviolet", "Spectrum" };

// ─── State shared by all twelve (only one program runs at a time) ──────
static float v6_t = 0;                      // the program's clock (Speed applied)
static float v6_flow = 0;                   // where the hue slice sits on the gradient, 0 … 1
static int v6_palA = 0, v6_palB = 0;        // fading from palette A to B
static float v6_palX = 1.0f;
static int v6_look = 0, v6_lastPreset = -1; // the preset in use (the tour picks one)
static float v6_tourT = 0;
static float v6_fade = 1.0f;                // 0 → 1 after each preset change (programs may ease with it)
static bool v6_k12 = false, v6_k12was = false, v6_k12hit = false;
static float v6_ref[5]; static bool v6_armed[5]; static bool v6_refOK = false;   // p1, p12 … p15
static bool v6_first = true;               // the first frame after a program switch (snap, don't fade)
static int v6_frames = 0;

// a knob that only takes effect once it has been turned (index into v6_ref: 0 = p1, 1-4 = p12-p15)
static bool v6_arm(int slot, int pot) {
  if (!v6_armed[slot] && fabsf(v3_sm[pot] - v6_ref[slot]) > 40.0f) v6_armed[slot] = true;
  return v6_armed[slot];
}
// call from the program's _init
static void v6_init() {
  for (int t = 0; t < 128; t++) v6_fv[t] = (uint16_t)(v6_fu[t] * V6_Q);
  v6_first = true; v6_frames = 0;
  v3_reset(); v6_refOK = false; v6_lastPreset = -1; v6_t = 0; v6_tourT = 0; v6_fade = 1.0f; v6_palX = 1.0f;
}

// ─── Palette ──────────────────────────────────────────────────────────
static int v6_gamma[16];
static bool v6_gammaOK = false;
// a colour at position p (0 … 1535 = once round the six stops) of palette `pal`
static inline void v6_grad(int pal, int p, int* o) {
  p %= 1536; if (p < 0) p += 1536;
  int i = p >> 8, f = p & 255, j = i == 5 ? 0 : i + 1;
  const uint8_t* a = v6_pals[pal][i]; const uint8_t* b = v6_pals[pal][j];
  for (int c = 0; c < 3; c++) o[c] = a[c] + (((b[c] - a[c]) * f) >> 8);
}
// rebuild all 256 entries. spread256: 256 = the 16 hues span the whole gradient once.
// sheen 0 … 255: how much the brightest levels whiten.
static void v6_palette(int spread256, int sheen) {
  if (!v6_gammaOK) { for (int l = 0; l < 16; l++) v6_gamma[l] = (int)(256.0f * powf(l / 15.0f, 1.25f)); v6_gammaOK = true; }
  int base = (int)(v6_flow * 1536.0f);
  int xf = (int)(v6_palX * 256);
  for (int h = 0; h < 16; h++) {
    int p = base + ((h * spread256 * 6) >> 4);
    int ca[3], cb[3], c[3];
    v6_grad(v6_palA, p, ca); v6_grad(v6_palB, p, cb);
    for (int q = 0; q < 3; q++) c[q] = ca[q] + (((cb[q] - ca[q]) * xf) >> 8);
    display.setColor((uint8_t)h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = v6_gamma[l], w = l > 11 ? ((l - 11) * sheen) >> 2 : 0;          // whitening on levels 12-15
      int r = (c[0] * k) >> 8, g = (c[1] * k) >> 8, b = (c[2] * k) >> 8;
      r += ((255 - r) * w) >> 8; g += ((255 - g) * w) >> 8; b += ((255 - b) * w) >> 8;
      display.setColor((uint8_t)((l << 4) | h), (uint8_t)v3_clamp(r, 0, 255), (uint8_t)v3_clamp(g, 0, 255), (uint8_t)v3_clamp(b, 0, 255));
    }
  }
}

// ─── Every frame: begin and end ───────────────────────────────────────
// v6_begin returns the look (0 … 10) to draw; `preset` 11 is the tour.
// ownPal / ownFlow / ownSpread are the look's own palette, flow speed and hue spread.
struct V6Frame { float dt, sdt, speed, zoom, flowRate; int spread; bool fresh; };
static V6Frame v6_f;
static int v6_begin(int preset, int nLooks) {
  v3_buf = display.getBuffer();
  v3_frame();
  if (!v6_refOK) {
    v6_ref[0] = v3_sm[1]; for (int i = 0; i < 4; i++) v6_ref[1 + i] = v3_sm[12 + i];
    for (int i = 0; i < 5; i++) v6_armed[i] = false;
    v6_refOK = true;
  }
  for (int i = 0; i < 4; i++) if (!v6_arm(1 + i, 12 + i)) v3_sm[12 + i] = 512.0f;   // FX knobs asleep until turned
  fx5_take();
  if (preset < 0 || preset > 11) preset = 0;
  v6_f.fresh = false;
  if (preset != v6_lastPreset) {
    v6_lastPreset = preset; v6_tourT = 0; v6_fade = 0; v6_f.fresh = true;
    v6_look = preset == 11 ? v3_rnd(nLooks) : (preset < nLooks ? preset : 0);
  }
  if (preset == 11) {
    v6_tourT += v3_dt;
    if (v6_tourT > 20.0f) { v6_tourT = 0; v6_fade = 0; v6_f.fresh = true; v6_look = (v6_look + 1 + v3_rnd(nLooks - 1)) % nLooks; }
  }
  if (v6_fade < 1.0f) { v6_fade += v3_dt / 2.0f; if (v6_fade > 1.0f) v6_fade = 1.0f; }
  float sp = v6_arm(0, 1) ? v3_speed(1, 4.0f) : 1.0f;
  v6_f.dt = v3_dt; v6_f.speed = sp; v6_f.sdt = v3_dt * sp;
  v6_t += v6_f.sdt; if (v6_t > 100000.0f) v6_t -= 100000.0f;
  bool k = keysPressed[KEY_MOD_A];
  v6_k12hit = k && !v6_k12was; v6_k12 = k; v6_k12was = k;
  v6_first = v6_frames == 0; v6_frames++;
  if (v6_first) { v6_fade = 1.0f; int pal = v6_palB; v6_palA = pal; v6_palX = 1.0f; }
  return v6_look;
}
// palette, zoom and colour flow from the common knobs, with the look's own values in the centre band
static void v6_colours(int ownPal, float ownFlow, float ownSpread, int sheen) {
  int pal = v3_centre(0) ? ownPal : v3_clamp(v3_k(0) * V6_NPAL / 1024, 0, V6_NPAL - 1);
  if (v6_first) { v6_palA = v6_palB = pal; v6_palX = 1.0f; }
  if (pal != v6_palB) {                                   // fade to the new palette
    if (v6_palX < 1.0f) v6_palA = v6_palB;                // (mid-fade: start again from where it was heading)
    else v6_palA = v6_palB;
    v6_palB = pal; v6_palX = 0;
  }
  if (v6_palX < 1.0f) { v6_palX += v3_dt / 1.6f; if (v6_palX >= 1.0f) { v6_palX = 1.0f; v6_palA = v6_palB; } }
  float fl = v3_centre(3) ? ownFlow : v3_bi(3) * fabsf(v3_bi(3)) * 0.6f;
  v6_f.flowRate = fl;
  v6_flow += fl * v3_dt * (v6_f.speed > 0 ? 1.0f : 0.0f);
  v6_flow -= floorf(v6_flow);
  float sp = v3_centre(4) ? ownSpread : 0.08f + v3_u(4) * 1.4f;
  v6_f.spread = (int)(sp * 256);
  v6_palette(v6_f.spread, sheen);
}
static float v6_zoom(float own) { return v3_centre(2) ? own : own * powf(3.0f, v3_bi(2)); }
// at the very end of the draw
static inline void v6_end() { fx5_apply(); }

// a value that eases toward a target (call each frame); on the first frame it jumps straight there
static inline void v6_ease(float* v, float target, float rate) { if (v6_first) *v = target; else *v += (target - *v) * v3_clampf(v3_dt * rate, 0.0f, 1.0f); }

// common knob labels; programs give labels for p5-p11
static const char* const v6_commonLbl[5] = { "Palette", "Speed", "Zoom", "Colour Flow", "Hue Spread" };
static const char* v6_label(int pot, const char* const* own) {
  if (pot >= 0 && pot <= 4) return v6_commonLbl[pot];
  if (pot >= 12 && pot <= 15) return fx5_labels[pot - 12];
  if (pot >= 5 && pot <= 11) return own[pot - 5];
  return "";
}

// ─── The hexagonal lattice ────────────────────────────────────────────
// Lattice coordinates U, V (32-bit, wrapping): one hexagon spacing a is
// 65536 in U, and √3·a is 65536 in V. The lattice is two rectangular grids,
// one through the rectangle centres (A) and one through the corners (B);
// the nearer of the two centres is the hexagon a point is in.
//
// Everything about where a point sits inside its hexagon depends only on
// its offset from the rectangle centre, folded into one quarter — so one
// 65 × 65 table answers it (vg6_geo.h, in flash): no square roots, no
// multiplies. A program turns that table into colours once a frame
// (V6_QN entries), so each pixel is just:  colour = table[V6_QIDX(U, V)].
#define V6_QIDX(U, V) (v6_fv[((V) >> 9) & 127] + v6_fu[((U) >> 9) & 127])
// which hexagon a point is in (an id), for colouring cells differently; e = its geometry entry
static inline uint32_t v6_cell(uint32_t U, uint32_t V, int e) {
  if (e & 0x8000) { U += 32768u; V += 32768u; }
  return ((U >> 16) * 2654435761u) ^ ((V >> 16) * 40503u) ^ (uint32_t)((e >> 15) & 1);
}
// the per-pixel steps for a view: `spacing` px between hexagon centres, turned by
// `ang` (radians), the lattice slid by (panU, panV) spacings; u0, v0 = the screen
// point (cx, cy) — pass 0, 0 to measure from the screen centre
struct V6View { int32_t du_x, dv_x, du_y, dv_y; uint32_t u0, v0; };
static void v6_view(V6View* v, float spacing, float ang, float panU, float panV, float cx, float cy) {
  float c = cosf(ang), s = sinf(ang), ku = 65536.0f / spacing, kv = 65536.0f / (spacing * 1.7320508f);
  v->du_x = (int32_t)(c * ku); v->dv_x = (int32_t)(-s * kv);
  v->du_y = (int32_t)(s * ku); v->dv_y = (int32_t)(c * kv);
  v->u0 = (uint32_t)(int32_t)(-(cx * c + cy * s) * ku + panU * 65536.0f);
  v->v0 = (uint32_t)(int32_t)(-(-cx * s + cy * c) * kv + panV * 65536.0f);
}

// ─── Glowing lines and arcs ───────────────────────────────────────────
// Light ADDS where lines cross (so the crossings of circles glow brighter),
// up to level 15; the hue of the stronger light wins.
static inline __attribute__((always_inline)) void v6_add(int x, int y, int lev, int hue) {
  if ((unsigned)x >= W || (unsigned)y >= H || lev <= 0) return;
  uint8_t* p = v3_buf + y * W + x;
  int old = *p >> 4, n = old + lev;
  if (n > 15) n = 15;
  *p = (uint8_t)((n << 4) | (lev >= old ? hue : (*p & 15)));
}
// light added without the edge check (the caller knows it's on screen)
static inline __attribute__((always_inline)) void v6_addU(uint8_t* p, int lev, int hue) {
  int old = *p >> 4, n = old + lev;
  if (n > 15) n = 15;
  *p = (uint8_t)((n << 4) | (lev >= old ? hue : (*p & 15)));
}
// an anti-aliased line, ends in 1/8 px, adding light (compiled for speed: lines are most of a wireframe)
__attribute__((optimize("O2"))) static void v6_lineAdd(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (lev <= 0) return;
  if (lev > 15) lev = 15;
  int dx = x1 - x0, dy = y1 - y0, adx = v3_abs(dx), ady = v3_abs(dy);
  if (adx > 8 * 2000 || ady > 8 * 2000) return;
  if (adx < 8 && ady < 8) { v6_add((x0 + 4) >> 3, (y0 + 4) >> 3, lev, hue); return; }
  uint8_t lt[9];                                                    // the light for each eighth of a pixel
  for (int k = 0; k <= 8; k++) lt[k] = (uint8_t)((lev * k + 4) >> 3);
  uint8_t* buf = v3_buf;
  if (adx >= ady) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dy = -dy; }
    int32_t grad = (int32_t)dy * 65536 / (x1 - x0);
    int xs = (x0 + 7) >> 3, xe = x1 >> 3;
    if (xs < 0) xs = 0; if (xe > W - 1) xe = W - 1;
    int32_t y = y0 * 65536 + (int32_t)(xs * 8 - x0) * grad, step = grad * 8;
    for (int x = xs; x <= xe; x++, y += step) {
      int yy = (int)(y >> 16), yi = yy >> 3, f = yy & 7;
      if ((unsigned)yi >= H - 1) { if (yi == H - 1) v6_add(x, yi, lt[8 - f], hue); else if (yi == -1 && f) v6_add(x, 0, lt[f], hue); continue; }
      uint8_t* p = buf + yi * W + x;
      v6_addU(p, lt[8 - f], hue);
      if (f) v6_addU(p + W, lt[f], hue);
    }
  } else {
    if (y1 < y0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dx = -dx; }
    int32_t grad = (int32_t)dx * 65536 / (y1 - y0);
    int ys = (y0 + 7) >> 3, ye = y1 >> 3;
    if (ys < 0) ys = 0; if (ye > H - 1) ye = H - 1;
    int32_t x = x0 * 65536 + (int32_t)(ys * 8 - y0) * grad, step = grad * 8;
    uint8_t* row = buf + ys * W;
    for (int y = ys; y <= ye; y++, x += step, row += W) {
      int xx = (int)(x >> 16), xi = xx >> 3, f = xx & 7;
      if ((unsigned)xi >= W - 1) { if (xi == W - 1) v6_add(xi, y, lt[8 - f], hue); else if (xi == -1 && f) v6_add(0, y, lt[f], hue); continue; }
      v6_addU(row + xi, lt[8 - f], hue);
      if (f) v6_addU(row + xi + 1, lt[f], hue);
    }
  }
}
static const uint16_t v6_circT[258] = {                            // √(1 − u²) for u = 0 … 1/√2, × 65535
  65535,65535,65534,65533,65531,65529,65526,65523,65519,65515,65510,65505,65499,65493,65486,65479,
  65471,65463,65454,65445,65435,65425,65414,65403,65391,65379,65366,65352,65339,65324,65310,65294,
  65279,65262,65245,65228,65210,65192,65173,65154,65134,65113,65093,65071,65049,65027,65004,64980,
  64956,64932,64907,64882,64855,64829,64802,64774,64746,64718,64689,64659,64629,64598,64567,64535,
  64503,64470,64437,64403,64369,64334,64298,64262,64226,64189,64151,64113,64075,64036,63996,63956,
  63915,63874,63832,63790,63747,63703,63659,63615,63570,63524,63478,63431,63384,63336,63287,63239,
  63189,63139,63088,63037,62985,62933,62880,62827,62773,62718,62663,62607,62551,62494,62437,62379,
  62320,62261,62201,62141,62080,62018,61956,61894,61830,61766,61702,61637,61571,61505,61438,61370,
  61302,61234,61164,61094,61024,60953,60881,60808,60735,60662,60587,60512,60437,60361,60284,60206,
  60128,60049,59970,59890,59809,59728,59645,59563,59479,59395,59310,59225,59139,59052,58965,58877,
  58788,58698,58608,58517,58425,58333,58240,58146,58052,57957,57861,57764,57667,57569,57470,57370,
  57270,57169,57067,56964,56861,56757,56652,56546,56440,56333,56225,56116,56006,55896,55785,55673,
  55560,55446,55332,55217,55100,54983,54866,54747,54627,54507,54386,54264,54141,54017,53892,53766,
  53640,53512,53384,53254,53124,52993,52861,52727,52593,52458,52322,52185,52047,51908,51768,51627,
  51485,51342,51198,51053,50907,50759,50611,50461,50311,50159,50006,49853,49698,49541,49384,49226,
  49066,48905,48743,48580,48415,48249,48082,47914,47744,47574,47401,47228,47053,46877,46699,46521,
  46340,46159,
};
// a whole anti-aliased circle (Wu's method, by eighths): centre in 1/8 px (rounded to a pixel), radius in 1/8 px
static void v6_circle(int cx8, int cy8, int r8, int lev, int hue) {
  if (r8 < 8 || lev <= 0) return;
  int cx = (cx8 + 4) >> 3, cy = (cy8 + 4) >> 3, rp = (r8 >> 3) + 2;
  if (cx + rp < 0 || cx - rp >= W || cy + rp < 0 || cy - rp >= H) return;
  bool inside = cx - rp >= 0 && cx + rp < W && cy - rp >= 0 && cy + rp < H;
  int xmax = (r8 * 181) >> 11;                                      // r / √2, in px
  int32_t k = (int32_t)((256LL * 8 * 65536) / ((int64_t)r8 * 181 / 256 + 1));   // px → table index (×65536)
  for (int xi = 0; xi <= xmax; xi++) {
    int idx = (int)((xi * k) >> 16); if (idx > 257) idx = 257;
    int y8 = (int)(((int32_t)r8 * v6_circT[idx]) >> 16);
    int yi = y8 >> 3, f = y8 & 7;
    int la = (lev * (8 - f) + 4) >> 3, lb = (lev * f + 4) >> 3;
    if (inside) {
      uint8_t* c = v3_buf + cy * W + cx;
      int oyW = yi * W, oyW1 = oyW + W, oxW = xi * W;
      v6_addU(c + xi - oyW, la, hue); v6_addU(c - xi - oyW, la, hue); v6_addU(c + xi + oyW, la, hue); v6_addU(c - xi + oyW, la, hue);
      v6_addU(c + yi - oxW, la, hue); v6_addU(c - yi - oxW, la, hue); v6_addU(c + yi + oxW, la, hue); v6_addU(c - yi + oxW, la, hue);
      if (lb) {
        v6_addU(c + xi - oyW1, lb, hue); v6_addU(c - xi - oyW1, lb, hue); v6_addU(c + xi + oyW1, lb, hue); v6_addU(c - xi + oyW1, lb, hue);
        v6_addU(c + yi + 1 - oxW, lb, hue); v6_addU(c - yi - 1 - oxW, lb, hue); v6_addU(c + yi + 1 + oxW, lb, hue); v6_addU(c - yi - 1 + oxW, lb, hue);
      }
    } else {
      v6_add(cx + xi, cy - yi, la, hue); v6_add(cx - xi, cy - yi, la, hue); v6_add(cx + xi, cy + yi, la, hue); v6_add(cx - xi, cy + yi, la, hue);
      v6_add(cx + yi, cy - xi, la, hue); v6_add(cx - yi, cy - xi, la, hue); v6_add(cx + yi, cy + xi, la, hue); v6_add(cx - yi, cy + xi, la, hue);
      if (lb) {
        v6_add(cx + xi, cy - yi - 1, lb, hue); v6_add(cx - xi, cy - yi - 1, lb, hue); v6_add(cx + xi, cy + yi + 1, lb, hue); v6_add(cx - xi, cy + yi + 1, lb, hue);
        v6_add(cx + yi + 1, cy - xi, lb, hue); v6_add(cx - yi - 1, cy - xi, lb, hue); v6_add(cx + yi + 1, cy + xi, lb, hue); v6_add(cx - yi - 1, cy + xi, lb, hue);
      }
    }
  }
}
// an arc (or whole circle): centre and radius in 1/8 px, angles 1024 per turn from a0 for `len`
// (len ≥ 1024 = whole circle). Drawn as short straight pieces about 3 px long.
static void v6_arc(int cx, int cy, int r, int a0, int len, int lev, int hue) {
  if (r < 8 || lev <= 0 || len <= 0) return;
  if (len >= 1024) { v6_circle(cx, cy, r, lev, hue); return; }
  int rp = r >> 3;
  if (cx + r < -16 || cx - r > W * 8 + 16 || cy + r < -16 || cy - r > H * 8 + 16) return;   // off screen
  if (len > 1024) len = 1024;
  int n = v3_clamp((rp * 2 * len) >> 10, 3, 160);                  // pieces
  int px = cx + ((v3_cos(a0) * r) >> 14), py = cy + ((v3_sin(a0) * r) >> 14);
  for (int i = 1; i <= n; i++) {
    int a = a0 + (len * i) / n;
    int qx = cx + ((v3_cos(a) * r) >> 14), qy = cy + ((v3_sin(a) * r) >> 14);
    v6_lineAdd(px, py, qx, qy, lev, hue);
    px = qx; py = qy;
  }
}
// a glowing circle: a bright line with a soft halo either side (halo 0 … 3 px)
static void v6_ring(int cx, int cy, int r, int a0, int len, int lev, int hue, int halo) {
  for (int h = halo; h >= 1; h--) {
    int hl = lev / (2 + 2 * h);
    v6_arc(cx, cy, r + h * 8, a0, len, hl, hue);
    if (r - h * 8 > 8) v6_arc(cx, cy, r - h * 8, a0, len, hl, hue);
  }
  v6_arc(cx, cy, r, a0, len, lev, hue);
}
// a soft dot (radius in px, centre in 1/8 px), adding light
static void v6_dot(int cx8, int cy8, int r, int lev, int hue) {
  int cx = cx8 >> 3, cy = cy8 >> 3;
  if (cx < -r || cx > W + r || cy < -r || cy > H + r) return;
  int r2 = r * r + 1;
  for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
    int d2 = dx * dx + dy * dy;
    if (d2 <= r2) v6_add(cx + dx, cy + dy, lev - (lev * d2) / (r2 + r), hue);
  }
}

// ─── Filled shapes ────────────────────────────────────────────────────
// a run of one colour, written a word at a time where it can be
static inline void v6_span(uint8_t* p, int n, uint8_t c) {
  while (n > 0 && ((uintptr_t)p & 3)) { *p++ = c; n--; }
  uint32_t w = c * 0x01010101u; uint32_t* q = (uint32_t*)p;
  while (n >= 4) { *q++ = w; n -= 4; }
  p = (uint8_t*)q; while (n-- > 0) *p++ = c;
}
// a convex polygon (n ≤ 8), corners in 1/16 px, one flat colour (one divide per edge)
static int16_t v6_xl[H], v6_xr[H];
__attribute__((optimize("O2"))) static void v6_fill(const int32_t* xs0, const int32_t* ys0, int n, uint8_t c) {
  if (n < 3 || n > 8) return;
  int32_t xs[8], ys[8], ymin = 1 << 30, ymax = -(1 << 30);
  for (int i = 0; i < n; i++) {
    xs[i] = v3_clamp(xs0[i], -8000, 13000); ys[i] = v3_clamp(ys0[i], -8000, 12000);
    if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i];
  }
  int r0 = (int)((ymin + 7) >> 4), r1 = (int)((ymax - 8) >> 4);
  if (r0 < 0) r0 = 0; if (r1 > H - 1) r1 = H - 1;
  if (r0 > r1) return;
  for (int y = r0; y <= r1; y++) { v6_xl[y] = 32767; v6_xr[y] = -32768; }
  for (int i = 0; i < n; i++) {
    int j = i + 1 == n ? 0 : i + 1;
    int32_t xa = xs[i], ya = ys[i], xb = xs[j], yb = ys[j];
    if (ya == yb) continue;
    if (ya > yb) { int32_t t = xa; xa = xb; xb = t; t = ya; ya = yb; yb = t; }
    int e0 = (int)((ya + 7) >> 4), e1 = (int)((yb - 8) >> 4);
    if (e0 < r0) e0 = r0; if (e1 > r1) e1 = r1;
    if (e0 > e1) continue;
    int32_t step = (xb - xa) * 4096 / (yb - ya);
    int32_t x8 = xa * 256 + (((int32_t)e0 * 16 + 8 - ya) * step >> 4);
    for (int y = e0; y <= e1; y++, x8 += step) {
      int32_t x16 = x8 >> 8;
      int xl = (int)((x16 + 8) >> 4), xr = (int)((x16 - 8) >> 4);
      if (xl < v6_xl[y]) v6_xl[y] = (int16_t)xl;
      if (xr > v6_xr[y]) v6_xr[y] = (int16_t)xr;
    }
  }
  for (int y = r0; y <= r1; y++) {
    int a = v6_xl[y], b = v6_xr[y];
    if (a < 0) a = 0; if (b > W - 1) b = W - 1;
    if (a <= b) v6_span(v3_buf + y * W + a, b - a + 1, c);
  }
}

// ─── A 5×7 font for the occasional title ──────────────────────────────
static void v6_title(const char* s, uint8_t c) {
  int n = (int)strlen(s), w = n * 12 - 2, x = (W - w) / 2;
  v3_text(x + 1, H - 22, s, 2, 0); v3_text(x, H - 23, s, 2, c);
}
#endif

// =====================================================================
// PROGRAM: LIQUID — a 1960s liquid light show (by Dewey) — Set 3, slot 3
//
// Coloured dyes and oil in a glass dish on an overhead projector: blobs of
// colour swirl as the dish turns, merge where they touch and split as they
// pull apart. Where two dyes overlap their colours mix like filters on a
// lamp, every blob has a crisp meniscus edge, the whole liquid wobbles, and
// now and then an unseen hand presses the glass and everything squishes out
// and springs back. A colour wheel can turn in front of the lamp, air
// bubbles drift through, and new drops of dye bloom in as old ones fade.
//
// How it works: blobs are summed into three coarse "dye" fields (one per
// colour), the fields are sampled through a slowly flowing wobble, and each
// pixel is coloured by which dyes are present and how deep they are.
// Fixed point in the pixel loop; floats only per blob.
//
// Presets:
//   k0 Fillmore    — red, yellow and blue dyes on white light in a round dish
//   k1 Oil & Water — dozens of oil droplets with dark rims over coloured water
//   k2 Lava        — big warm blobs rising and sinking like a lava lamp
//   k3 Cells       — crowded cells with thick membranes, like a microscope slide
//   k4 Amoeba      — a few huge wobbling bodies
//   k5 Bubbles     — dye in a dish full of air bubbles
//   k6 Ink Drop    — drops of ink bloom on white and fade as new ones fall
//   k7 Kaleido     — neon dyes in a four-way mirror
//   k8 Moonbeam    — one colour, mirrored, on blue light
//   k9 Acid        — maximum swirl, mixing and a fast colour wheel
//   k10 Oil Slick  — rainbow contour bands, like oil on a puddle
//   k11 Light Show — moves through all the others, a new look every ~18 s
//
//   k12 — SQUISH: press the glass
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Blobs   p3 Blob Size
//   p4 Swirl (how far the liquid wobbles)   p5 Wobble (fine ↔ broad ripples)
//   p6 Squeeze (automatic presses on the glass)   p7 Rims (meniscus width; far left = none)
//   p8 Colour Mix (left: dyes sit on top of each other, right: they mix)
//   p9 Bubbles   p10 Light (lamp brightness)   p11 Colour Wheel (far left = off)
//   p12 Rotation (dish spin; centre = preset, left reverses)   p13 Depth (thick dye shading)
//   p14 Mirror (centre = preset; left none, right 2-way / 4-way)   p15 Drops (new dye drops)
// =====================================================================

#include "vg3.h"

// ─── Shared-memory layout (v3_scratch) ────────────────────────────────
#define LQ_GW 41                                   // coarse dye fields: one point every 8 px
#define LQ_GH 31
#define LQ_WN 81                                   // wobbled fields: one point every 4 px
#define LQ_MAXB 24
#define LQ_MAXBUB 16
struct LqBlob { float x, y, vx, vy, r, rt, age, life, heat, str; uint8_t layer, state; uint16_t seed; };
struct LqBub { float x, y, r, ph; };
static uint8_t* lq_g;                              // [3][LQ_GH][LQ_GW]
static int16_t* lq_w;                              // [2][3][LQ_WN]
static LqBlob* lq_b;
static LqBub* lq_bub;
static void lq_mem() {
  lq_g = v3_scratch;                                              // 3813 B
  lq_w = (int16_t*)(v3_scratch + 3840);                           // 972 B
  lq_b = (LqBlob*)(v3_scratch + 4864);                            // 24 × 48 B
  lq_bub = (LqBub*)(v3_scratch + 6080);                           // 16 × 16 B
}

// ─── Palettes ─────────────────────────────────────────────────────────
// hues: 0 lamp light, 1-3 dyes A B C, 4 A+B, 5 A+C, 6 B+C, 7 all three, 8 rim, 9 highlight
#define LQ_NPAL 8
struct LqPal { uint8_t bg[3], a[3], b[3], c[3], rim[3], hi[3]; uint8_t light; };   // light: 1 = dyes filter a bright lamp
static const LqPal lq_pals[LQ_NPAL] = {
  {{255,244,215},{235,25,35},{255,205,0},{25,80,235},{20,10,45},{255,255,255},1},     // 0 Fillmore
  {{70,0,95},{120,255,40},{255,40,160},{0,220,255},{255,255,255},{255,255,255},0},     // 1 Haight
  {{45,0,70},{255,110,0},{255,30,40},{255,220,60},{255,255,190},{255,255,255},0},      // 2 Lava
  {{205,255,240},{215,0,145},{0,170,60},{115,40,225},{0,40,45},{255,255,255},1},       // 3 Petri
  {{0,12,70},{0,235,205},{50,95,255},{190,235,255},{255,205,40},{255,255,255},0},      // 4 Ocean
  {{255,185,70},{205,0,125},{95,0,175},{225,20,20},{25,0,20},{255,255,230},1},         // 5 Sunset
  {{0,0,0},{255,0,205},{0,255,240},{255,240,0},{255,255,255},{255,255,255},0},         // 6 Neon
  {{252,250,244},{20,30,95},{175,0,30},{0,115,115},{0,0,0},{255,255,255},1},           // 7 Ink
};
static const uint8_t lq_tints[4][3] = {{255,150,150},{150,255,160},{150,170,255},{255,255,130}};   // the colour wheel

// ─── Presets ──────────────────────────────────────────────────────────
enum { LQ_ORBIT, LQ_LAVA, LQ_DRIFT, LQ_INK };
struct LqLook { uint8_t pal, n, size, layers, swirl, wob, rim, mix, bub, light, wheel; int8_t rot; uint8_t depth, mirror, drops, motion, dish, bands; };
static const LqLook lq_looks[11] = {
  // pal  n  size L swirl wob rim mix bub light wheel rot depth mir drops motion   dish bands
  {  0,   9, 46, 3, 40, 40, 50, 80,  3, 100, 25,  35, 50, 0, 25, LQ_ORBIT, 1, 0},   // Fillmore
  {  1,  18, 26, 2, 25, 60, 70,  0,  8,  30,  0,  25, 30, 0, 10, LQ_ORBIT, 1, 0},   // Oil & Water
  {  2,   8, 58, 2, 20, 30, 45, 60,  0,  35,  0,   0, 70, 0,  0, LQ_LAVA,  0, 0},   // Lava
  {  3,  22, 34, 3, 20, 50, 90, 30,  0, 100,  0,  15, 80, 0, 40, LQ_DRIFT, 0, 0},   // Cells
  {  4,   5, 70, 2, 80, 25, 60, 70,  2,  35, 10,  20, 40, 0, 10, LQ_ORBIT, 0, 0},   // Amoeba
  {  5,   8, 50, 3, 40, 40, 40, 80, 16, 100, 15,  40, 40, 0, 20, LQ_ORBIT, 1, 0},   // Bubbles
  {  7,  10, 40, 3, 35, 45, 25, 90,  0, 100,  0,  10, 60, 0, 80, LQ_INK,   0, 0},   // Ink Drop
  {  6,  10, 50, 3, 60, 40, 60, 80,  4,  20, 30,  50, 40, 2, 30, LQ_ORBIT, 0, 0},   // Kaleido
  {  4,   8, 50, 1, 50, 30, 70,  0,  5,  45,  0,  25, 50, 1, 15, LQ_ORBIT, 1, 0},   // Moonbeam
  {  6,  14, 50, 3,100, 70, 50,100,  0,  25, 80,  70, 30, 0, 50, LQ_ORBIT, 0, 0},   // Acid
  {  1,  10, 62, 3, 50, 35, 30, 50,  0,  15,  0,  30,  0, 0, 20, LQ_DRIFT, 0, 1},   // Oil Slick
};
static const char* const lq_presetNames[12] = {
  "Fillmore", "Oil & Water", "Lava", "Cells", "Amoeba", "Bubbles",
  "Ink Drop", "Kaleido", "Moonbeam", "Acid", "Oil Slick", "Light Show"
};
static const char* const lq_labels[16] = {
  "Palette", "Speed", "Blobs", "Blob Size", "Swirl", "Wobble", "Squeeze", "Rims",
  "Colour Mix", "Bubbles", "Light", "Colour Wheel", "Rotation", "Depth", "Mirror", "Drops"
};
const char* prog_liquid_name() { return "LIQUID"; }
const char* prog_liquid_character() { return "Liquid light show: dyes and oil swirl, merge and split in a glass dish on a projector"; }
const char* prog_liquid_presetName(int preset) { return (preset >= 0 && preset < 12) ? lq_presetNames[preset] : NULL; }
const char* prog_liquid_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? lq_labels[pot] : ""; }
uint8_t prog_liquid_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int lq_preset = -1, lq_look = 0, lq_palNow = 0, lq_palPrev = 0;
static float lq_palX = 1.0f;                        // palette cross-fade 0 → 1
static float lq_t = 0, lq_showT = 0, lq_wheelT = 0, lq_bandT = 0;
static float lq_sq = 0, lq_sqx = 160, lq_sqy = 120, lq_sqT = 0, lq_sqNext = 3;
static bool lq_k12was = false, lq_snap = false, lq_full = true;
static int lq_field = 0;
static int lq_nb = 0, lq_nbub = 0;
static float lq_dropT = 0, lq_cover = 1.0f;
// per-frame pixel tables
static int lq_T = 4096;
static uint8_t lq_inLev[128], lq_bgLev[96], lq_bgLev2[96];
static uint8_t lq_hueOf[8] = {0, 1, 2, 4, 3, 5, 6, 7};
static uint8_t lq_rimPix = 0xF8, lq_linePix = 0x10, lq_haloPix[4];
static uint8_t lq_bandHue[8] = {1, 4, 2, 6, 3, 5, 8, 7};
static int lq_bandOff = 0;
static bool lq_bands = false;

static float lq_cx = 160, lq_cy = 120, lq_rd = 105;
static void lq_spawn(int i, int look, bool grow) {             // (indices, not pointers: Arduino's prototype generator)
  LqBlob* b = &lq_b[i]; const LqLook& L = lq_looks[look];
  float rd = lq_rd;
  switch (L.motion) {
    case LQ_LAVA: b->x = 30 + v3_rnd(260); b->y = 40 + v3_rnd(170); break;
    case LQ_DRIFT: b->x = v3_rnd(W); b->y = v3_rnd(H); break;
    default: {
      float a = v3_rndf() * 6.2831853f, r = rd * sqrtf(v3_rndf()) * (L.dish ? 0.85f : 1.25f);
      b->x = lq_cx + r * cosf(a); b->y = lq_cy + r * sinf(a) * (L.dish ? 1.0f : 0.8f);
    }
  }
  b->vx = (v3_rndf() - 0.5f) * 20; b->vy = (v3_rndf() - 0.5f) * 20;
  b->rt = L.size * (0.65f + 0.7f * v3_rndf());
  b->r = grow ? 0.0f : b->rt;
  b->age = 0; b->life = 10 + v3_rndf() * 20; b->heat = v3_rndf();
  b->str = 190 + v3_rnd(60);
  b->layer = (uint8_t)(i % (L.layers ? L.layers : 1));
  b->state = grow ? 0 : 1;
  b->seed = (uint16_t)v3_rand();
}

static void lq_setLook(int look, bool fresh) {
  const LqLook& L = lq_looks[look];
  lq_look = look;
  if (fresh) {
    lq_nb = 0;
    for (int i = 0; i < LQ_MAXB; i++) lq_spawn(i, look, false);
    for (int i = 0; i < LQ_MAXBUB; i++) { lq_bub[i].x = v3_rnd(W); lq_bub[i].y = v3_rnd(H); lq_bub[i].r = 3 + v3_rnd(7); lq_bub[i].ph = v3_rndf() * 6.28f; }
  } else {
    // a new look in the light show: the old blobs drain away one after another and new ones bloom in
    for (int i = 0; i < LQ_MAXB; i++) { lq_b[i].state = 2; lq_b[i].age = -v3_rndf() * 4.0f; }
  }
}

void prog_liquid_init() {
  lq_mem();
  v3_reset();
  lq_preset = -1;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

static void lq_palette(float bgK, float mixK, float wheelK) {
  const LqPal& P0 = lq_pals[lq_palPrev];
  const LqPal& P1 = lq_pals[lq_palNow];
  int xf = (int)(lq_palX * 256);
  uint8_t col[10][3];
  const uint8_t* s0[6] = {P0.bg, P0.a, P0.b, P0.c, P0.rim, P0.hi};
  const uint8_t* s1[6] = {P1.bg, P1.a, P1.b, P1.c, P1.rim, P1.hi};
  uint8_t base[6][3];
  for (int k = 0; k < 6; k++) for (int c = 0; c < 3; c++) base[k][c] = v3_mix8(s0[k][c], s1[k][c], xf);
  int light = xf < 128 ? P0.light : P1.light;
  // colour wheel tint (smoothly between four filters)
  int tint[3] = {255, 255, 255};
  if (wheelK > 0) {
    float u = lq_wheelT - floorf(lq_wheelT / 4.0f) * 4.0f;
    int i0 = (int)u, i1 = (i0 + 1) & 3, f = (int)((u - i0) * 256);
    for (int c = 0; c < 3; c++) {
      int t = v3_mix8(lq_tints[i0][c], lq_tints[i1][c], f);
      tint[c] = 255 - (int)((255 - t) * wheelK);
    }
  }
  for (int c = 0; c < 3; c++) {
    col[0][c] = (uint8_t)v3_clamp((int)(base[0][c] * bgK), 0, 255);
    col[1][c] = base[1][c]; col[2][c] = base[2][c]; col[3][c] = base[3][c];
    col[8][c] = base[4][c]; col[9][c] = base[5][c];
  }
  // overlaps: filters multiply on a bright lamp, light adds on a dark one; Colour Mix blends from "top dye only" to "mixed"
  static const uint8_t pairs[3][3] = {{1, 2, 4}, {1, 3, 5}, {2, 3, 6}};
  for (int c = 0; c < 3; c++) {
    for (int p = 0; p < 3; p++) {
      int a = col[pairs[p][0]][c], b = col[pairs[p][1]][c];
      int mixed = light ? v3_clamp((a * b / 255) * 3 / 4 + (a + b) / 4, 0, 255) : v3_clamp(a + b, 0, 255);   // filters on a lamp: darker, never black
      col[pairs[p][2]][c] = (uint8_t)(b + (int)((mixed - b) * mixK));
    }
    int A = col[1][c], B = col[2][c], C = col[3][c];
    int mixed = light ? v3_clamp((A * B / 255 + B * C / 255 + A * C / 255) / 2 + 20, 0, 255) : v3_clamp(A + B + C, 0, 255);
    col[7][c] = (uint8_t)(C + (int)((mixed - C) * mixK));
  }
  for (int h = 0; h < 10; h++) {
    int r = col[h][0] * tint[0] / 255, g = col[h][1] * tint[1] / 255, b = col[h][2] * tint[2] / 255;
    if (h == 8 || h == 9) { r = col[h][0]; g = col[h][1]; b = col[h][2]; }     // rims and highlights stay true
    v3_setLH(h, r, g, b, 1.25f, 0);
  }
}

// one row of the wobbled dye fields (points every 4 px), into out[3][LQ_WN], values 0 … 1020
static int lq_wAmp = 0, lq_wk1 = 0, lq_wk2 = 0, lq_t1 = 0, lq_t2 = 0, lq_t3 = 0, lq_t4 = 0;
static void lq_wrow(int j, int16_t* out, int imax) {
  int y = j * 4;
  for (int i = 0; i <= imax; i++) {
    int x = i * 4;
    int32_t wx = ((v3_sin(((y * lq_wk1) >> 6) + lq_t1) + ((v3_sin((((x + y) * lq_wk2) >> 6) + lq_t2) * 3) >> 2)) * lq_wAmp) >> 14;
    int32_t wy = ((v3_sin(((x * lq_wk1) >> 6) + lq_t3) + ((v3_sin((((x - y) * lq_wk2) >> 6) + lq_t4) * 3) >> 2)) * lq_wAmp) >> 14;
    int32_t sx = x * 16 + wx, sy = y * 16 + wy;                   // 1/16 px
    if (sx < 0) sx = 0; if (sx > 40 * 128 - 1) sx = 40 * 128 - 1;
    if (sy < 0) sy = 0; if (sy > 30 * 128 - 1) sy = 30 * 128 - 1;
    int gi = sx >> 7, fx = sx & 127, gj = sy >> 7, fy = sy & 127;
    const uint8_t* p = lq_g + gj * LQ_GW + gi;
    for (int L = 0; L < 3; L++, p += LQ_GW * LQ_GH) {
      int32_t top = p[0] * 128 + (p[1] - p[0]) * fx;
      int32_t bot = p[LQ_GW] * 128 + (p[LQ_GW + 1] - p[LQ_GW]) * fx;
      out[L * LQ_WN + i] = (int16_t)((top * 128 + (bot - top) * fy) >> 12);
    }
  }
}

// colour one pixel from its three dye depths (all in 1/16 steps)
static int lq_haloT, lq_lineT, lq_rimT;
static inline uint8_t lq_pix(int a, int b, int c, uint8_t bg) {
  if (a < lq_haloT && b < lq_haloT && c < lq_haloT) return bg;    // plain lamp (most pixels)
  const int T = lq_T;
  int m = (a > T) | ((b > T) << 1) | ((c > T) << 2);
  if (!m) {
    int mx = a > b ? a : b; if (c > mx) mx = c;
    if (mx > lq_lineT) return lq_linePix;                         // the meniscus line just outside a blob
    return lq_haloPix[(a >= b && a >= c) ? 1 : (b >= c ? 2 : 3)];
  }
  const int R = lq_rimT;
  if ((a > T && a < R) || (b > T && b < R) || (c > T && c < R)) return lq_rimPix;
  int mx = (m & 1) ? a : 0; if ((m & 2) && b > mx) mx = b; if ((m & 4) && c > mx) mx = c;
  int d = (mx - T) >> 7; if (d > 127) d = 127;
  if (lq_bands) return lq_inLev[d] | lq_bandHue[((mx + lq_bandOff) >> 10) & 7];
  return lq_inLev[d] | lq_hueOf[m];
}

static void lq_ring(int cx, int cy, int r, uint8_t c) {          // a circle outline
  int x = r, y = 0, e = 1 - r;
  while (x >= y) {
    v3_put(cx + x, cy + y, c); v3_put(cx - x, cy + y, c); v3_put(cx + x, cy - y, c); v3_put(cx - x, cy - y, c);
    v3_put(cx + y, cy + x, c); v3_put(cx - y, cy + x, c); v3_put(cx + y, cy - x, c); v3_put(cx - y, cy - x, c);
    y++; if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
  }
}

void prog_liquid_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  lq_mem();
  v3_frame();
  float dt = v3_dt;
  if (preset != lq_preset) {
    lq_preset = preset;
    int look = preset == 11 ? v3_rnd(11) : preset;
    lq_palNow = lq_palPrev = lq_looks[look].pal; lq_palX = 1.0f;
    lq_setLook(look, true);
    lq_showT = 0;
    lq_snap = true;
    lq_full = true;
  }
  // the light show moves on to a new look
  if (preset == 11) {
    lq_showT += dt;
    if (lq_showT > 18.0f) {
      lq_showT = 0;
      int nl = (lq_look + 1 + v3_rnd(10)) % 11;
      lq_palPrev = lq_palNow; lq_palNow = lq_looks[nl].pal; lq_palX = 0;
      lq_setLook(nl, false);
    }
  }
  if (lq_palX < 1.0f) { lq_palX += dt / 3.0f; if (lq_palX > 1.0f) lq_palX = 1.0f; }
  const LqLook& L = lq_looks[lq_look];

  // ── knobs ──
  float speed = v3_speed(1, 3.5f);
  int nWant = v3_centre(2) ? L.n : v3_clamp(3 + v3_k(2) * 22 / 1024, 3, LQ_MAXB);
  float sizeK = v3_rel(3, 1.0f, 2.0f);
  float swirl = v3_centre(4) ? L.swirl / 100.0f : v3_u(4) * 1.1f;
  float wob = v3_centre(5) ? L.wob / 100.0f : v3_u(5);
  float squeeze = v3_u(6);
  float rimK = v3_centre(7) ? L.rim / 100.0f : v3_u(7) * 1.3f;
  float mixK = v3_centre(8) ? L.mix / 100.0f : v3_u(8);
  int nBub = v3_centre(9) ? L.bub : v3_clamp(v3_k(9) * 17 / 1024, 0, LQ_MAXBUB);
  float bgK = v3_clampf((L.light / 100.0f) * powf(4.0f, v3_bi(10)), 0.15f, 1.0f);
  float wheelK = v3_k(11) < 40 ? 0.0f : (v3_centre(11) ? L.wheel / 100.0f : v3_u(11));
  float rot = v3_centre(12) ? L.rot / 100.0f : v3_bi(12) * 1.2f;
  float depthK = v3_centre(13) ? L.depth / 100.0f : v3_u(13);
  int mirror = v3_pick(14, L.mirror, 3);
  float drops = v3_centre(15) ? L.drops / 100.0f : v3_u(15);
  if (!v3_centre(0)) { int p = v3_clamp(v3_k(0) * LQ_NPAL / 1024, 0, LQ_NPAL - 1); if (p != lq_palNow) { lq_palPrev = lq_palNow; lq_palNow = p; lq_palX = 0; } }
  else if (preset != 11 && lq_palNow != L.pal && lq_palX >= 1.0f) { lq_palPrev = lq_palNow; lq_palNow = L.pal; lq_palX = 0; }
  bool dish = L.dish;
  { float tcx = mirror >= 1 ? 80.0f : 160.0f, tcy = mirror >= 2 ? 60.0f : 120.0f, trd = mirror >= 2 ? 60.0f : (mirror == 1 ? 75.0f : 105.0f);
    float k = lq_snap ? 1.0f : v3_clampf(dt * 0.7f, 0, 1); lq_cx += (tcx - lq_cx) * k; lq_cy += (tcy - lq_cy) * k; lq_rd += (trd - lq_rd) * k;
    if (lq_snap) { lq_snap = false; lq_setLook(lq_look, true); } }
  lq_bands = L.bands;
  float sdt = dt * speed;
  lq_t += sdt;

  // ── squish: k12, or automatic presses when Squeeze is up ──
  bool k12 = keysPressed[KEY_MOD_A];
  bool press = k12 && !lq_k12was;
  lq_k12was = k12;
  if (squeeze > 0.05f) {
    lq_sqNext -= sdt;
    if (lq_sqNext <= 0) { press = true; lq_sqNext = 1.5f + (1.0f - squeeze) * 8.0f + v3_rndf() * 2.0f; }
  }
  if (press) { lq_sqT = 0; lq_sqx = 90 + v3_rnd(140); lq_sqy = 60 + v3_rnd(120); }
  lq_sqT += dt;
  lq_sq = lq_sqT < 0.18f ? lq_sqT / 0.18f : expf(-(lq_sqT - 0.18f) * 2.2f) * cosf((lq_sqT - 0.18f) * 5.0f);   // press, then spring back
  if (lq_sqT > 4.0f) lq_sq = 0;

  // ── blobs ──
  float cx = lq_cx, cy = lq_cy;
  lq_dropT += sdt * (0.2f + drops * 1.6f);
  for (int i = 0; i < LQ_MAXB; i++) {
    LqBlob* b = &lq_b[i];
    bool want = i < nWant;
    // life: grow → live → drain away; drops replace old blobs
    b->age += sdt;
    if (b->state == 0) { b->r += (b->rt - b->r) * v3_clampf(sdt * 0.9f, 0, 1); if (b->r > b->rt * 0.95f) b->state = 1; }
    else if (b->state == 1) {
      if (!want) b->state = 2;
      else if (drops > 0.02f && b->age > b->life * (1.2f - drops)) { b->state = 2; }
      if (L.motion == LQ_INK) b->rt += sdt * 2.0f;                 // ink keeps spreading
      b->r += (b->rt - b->r) * v3_clampf(sdt, 0, 1);
    } else if (b->state == 2) {
      if (b->age >= 0) b->r -= sdt * (8.0f + b->rt * 0.25f);
      if (b->r <= 0) {
        b->r = 0;
        if (want && lq_dropT > 1.0f) { lq_dropT = 0; lq_spawn(i, lq_look, true); }
        else if (want && b->age > 3.0f) { lq_spawn(i, lq_look, true); }
        else b->state = 3;
      }
    } else if (want) { lq_spawn(i, lq_look, true); }
    if (b->r <= 0.5f) continue;
    // motion
    float dx = b->x - cx, dy = b->y - cy;
    float n1 = v3_noise1((uint32_t)(lq_t * 60) + b->seed * 37u, b->seed) / 16384.0f;
    float n2 = v3_noise1((uint32_t)(lq_t * 60) + b->seed * 91u, b->seed + 7) / 16384.0f;
    float ax = n1 * 30, ay = n2 * 30;
    if (L.motion == LQ_ORBIT || L.motion == LQ_INK) {
      float r = sqrtf(dx * dx + dy * dy) + 1;
      float w = rot * (1.3f - r / 180.0f);                          // the dish turns: the middle a little faster
      ax += -dy * w * 0.9f - b->vx * 0.8f; ay += dx * w * 0.9f - b->vy * 0.8f;
      float lim = lq_rd * (dish ? 0.9f : 1.4f);
      ax -= dx * 0.10f; ay -= dy * 0.10f;                           // the liquid gathers loosely in the middle
      if (r > lim) { ax -= dx / r * (r - lim) * 2; ay -= dy / r * (r - lim) * 2; }
      b->vx += ax * sdt; b->vy += ay * sdt;
      b->x += (b->vx - dy * w * 0.6f) * sdt; b->y += (b->vy + dx * w * 0.6f) * sdt;
    } else if (L.motion == LQ_LAVA) {
      if (b->y > 200) b->heat += sdt * 0.25f; else if (b->y < 60) b->heat -= sdt * 0.25f;
      b->heat = v3_clampf(b->heat, 0, 1);
      ay += -(b->heat - 0.5f) * 60 - b->vy * 0.6f; ax += -b->vx * 0.5f + (cx - b->x) * 0.02f;
      b->vx += ax * sdt; b->vy += ay * sdt;
      b->x += b->vx * sdt; b->y += b->vy * sdt;
      if (b->y > 230) { b->y = 230; b->vy = -fabsf(b->vy) * 0.3f; } if (b->y < 10) { b->y = 10; b->vy = fabsf(b->vy) * 0.3f; }
      if (b->x < 20) b->vx += 20 * sdt; if (b->x > 300) b->vx -= 20 * sdt;
    } else {                                                        // drift, wrapping round the edges
      b->vx += (ax - b->vx * 0.3f + rot * 8) * sdt; b->vy += (ay - b->vy * 0.3f) * sdt;
      b->x += b->vx * sdt; b->y += b->vy * sdt;
      if (b->x < -40) b->x += W + 80; if (b->x > W + 40) b->x -= W + 80;
      if (b->y < -40) b->y += H + 80; if (b->y > H + 40) b->y -= H + 80;
    }
  }
  // with a mirror, keep the liquid in the part of the picture that gets mirrored
  if (mirror >= 1) for (int i = 0; i < LQ_MAXB; i++) {
    LqBlob* b = &lq_b[i];
    float xm = 165.0f, ym = mirror >= 2 ? 125.0f : 245.0f;
    if (b->x > xm) { b->x = xm; b->vx = -fabsf(b->vx); } if (b->x < -5) { b->x = -5; b->vx = fabsf(b->vx); }
    if (b->y > ym) { b->y = ym; b->vy = -fabsf(b->vy); } if (b->y < -5) { b->y = -5; b->vy = fabsf(b->vy); }
  }
  // oil and water: blobs of different dyes push apart a little
  for (int i = 0; i < nWant; i++) for (int j = i + 1; j < nWant; j++) {
    LqBlob* a = &lq_b[i]; LqBlob* b = &lq_b[j];
    if (a->layer == b->layer || a->r < 1 || b->r < 1) continue;
    float dx = b->x - a->x, dy = b->y - a->y, d2 = dx * dx + dy * dy, rr = (a->r + b->r) * 0.7f;
    if (d2 < rr * rr && d2 > 1) { float d = sqrtf(d2), f = (rr - d) / d * sdt * 0.6f; a->vx -= dx * f; a->vy -= dy * f; b->vx += dx * f; b->vy += dy * f; }
  }

  // ── dye fields: every blob adds its shape into its dye's coarse field ──
  memset(lq_g, 0, 3 * LQ_GW * LQ_GH);
  // keep some lamp showing: if the blobs would cover more than ~70 % of the picture, shrink them all
  {
    float area = 0;
    for (int i = 0; i < LQ_MAXB; i++) if (lq_b[i].state != 3 && lq_b[i].r > 0.5f) { float R = lq_b[i].r * sizeK; area += 1.37f * R * R; }
    float src = (dish ? 42270.0f : 76800.0f) / (mirror == 0 ? 1.0f : (mirror == 1 ? 2.0f : 4.0f));
    float cov = area / src;
    float want = cov > 0.7f ? sqrtf(0.7f / cov) : 1.0f;
    lq_cover += (want - lq_cover) * v3_clampf(dt * 2.0f, 0, 1);
    sizeK *= lq_cover;
  }
  for (int i = 0; i < LQ_MAXB; i++) {
    LqBlob* b = &lq_b[i];
    if (b->r <= 0.5f || b->state == 3) continue;
    float bx = b->x, by = b->y, R = b->r * sizeK;
    if (lq_sq != 0) {                                              // the press pushes nearby liquid out and flattens it
      float dx = bx - lq_sqx, dy = by - lq_sqy, d = sqrtf(dx * dx + dy * dy) + 1;
      float k = v3_clampf(1.0f - d / 170.0f, 0, 1) * lq_sq;
      bx += dx / d * 55 * k; by += dy / d * 55 * k;
      R *= 1.0f + 0.4f * k;
    }
    if (R < 2) continue;
    int32_t x4 = (int32_t)(bx * 4), y4 = (int32_t)(by * 4), R4 = (int32_t)(R * 4);
    int32_t R2 = R4 * R4; if (R2 < 64) continue;
    uint32_t inv = (1u << 24) / (uint32_t)R2;
    int gx0 = v3_clamp((int)((bx - R) / 8) , 0, LQ_GW - 1), gx1 = v3_clamp((int)((bx + R) / 8) + 1, 0, LQ_GW - 1);
    int gy0 = v3_clamp((int)((by - R) / 8), 0, LQ_GH - 1), gy1 = v3_clamp((int)((by + R) / 8) + 1, 0, LQ_GH - 1);
    int str = (int)b->str;
    uint8_t* base = lq_g + b->layer * LQ_GW * LQ_GH;
    for (int gy = gy0; gy <= gy1; gy++) {
      int32_t ddy = gy * 32 - y4, dy2 = ddy * ddy;
      if (dy2 >= R2) continue;
      uint8_t* row = base + gy * LQ_GW;
      for (int gx = gx0; gx <= gx1; gx++) {
        int32_t ddx = gx * 32 - x4, d2 = ddx * ddx + dy2;
        if (d2 >= R2) continue;
        int32_t t = 256 - (int32_t)(((uint32_t)d2 * inv) >> 16);
        int v = (((t * t) >> 8) * str) >> 8;
        int s = row[gx] + v; row[gx] = (uint8_t)(s > 255 ? 255 : s);
      }
    }
  }

  // ── colours and per-frame tables ──
  lq_wheelT += dt * wheelK * 0.5f;
  lq_palette(bgK, mixK, wheelK);
  bool light = lq_pals[lq_palNow].light;
  int rimW = (int)(rimK * 46), lineW = rimK < 0.05f ? 0 : 6 + (int)(rimK * 18), haloW = lineW + 70;
  lq_T = 256 * 16; lq_rimT = (256 + rimW) * 16; lq_lineT = (256 - lineW) * 16; lq_haloT = (256 - haloW) * 16;
  lq_rimPix = (uint8_t)(0xF0 | 8);
  lq_linePix = light ? (uint8_t)(0xF0 | 9) : (uint8_t)(0x10 | 0);   // on a lamp a bright line outside the dark rim; on black a dark one
  for (int k = 1; k <= 3; k++) lq_haloPix[k] = light ? (uint8_t)(0xB0 | 0) : (uint8_t)(0x50 | k);
  {
    static int key = -1;
    int nk = (light ? 1000 : 0) + (int)(depthK * 100);
    if (nk != key) {                                               // rebuilt only when Depth or the lamp type changes
      key = nk;
      for (int d = 0; d < 128; d++) {
        float u = d / 127.0f;
        float lv = light ? 15.0f - depthK * 7.0f * sqrtf(u) : 9.0f + 6.0f * powf(u, 0.6f) * (0.3f + 0.7f * depthK) + (1 - depthK) * 2.0f;
        lq_inLev[d] = (uint8_t)(v3_clamp((int)(lv + 0.5f), 1, 15) << 4);
      }
      for (int i = 0; i < 96; i++) {                               // the lamp: brightest in the middle
        float u = i / 80.0f;
        float lv = light ? 15.0f - 4.0f * u * u : 3.0f + 3.0f * (1 - u);
        int l0 = v3_clamp((int)lv, 1, 15), l1 = v3_clamp((int)(lv + 0.5f), 1, 15);   // two levels, checker-dithered: no rings
        lq_bgLev[i] = (uint8_t)(l0 << 4); lq_bgLev2[i] = (uint8_t)(l1 << 4);
      }
    }
  }
  lq_bandT += sdt * 40; if (lq_bandT > 1024) lq_bandT -= 1024; lq_bandOff = ((int)lq_bandT & 1023) << 4;
  // the wobble
  lq_wAmp = (int)(swirl * 22 * 16);                               // up to ~22 px
  lq_wk1 = 200 + (int)((1.0f - wob) * 450); lq_wk2 = 160 + (int)((1.0f - wob) * 400);
  lq_t1 = (int)(lq_t * 70); lq_t2 = (int)(-lq_t * 53) + 300; lq_t3 = (int)(lq_t * 61) + 600; lq_t4 = (int)(-lq_t * 47) + 150;

  // ── pixels ──
  int xMax = mirror >= 1 ? 159 : 319, yMax = mirror >= 2 ? 119 : 239;
  int imax = (xMax >> 2) + 1;
  // interlaced, like video: even rows one frame, odd rows the next (the liquid moves slowly, so this is invisible
  // and halves the work). After a preset change both fields are drawn at once.
  lq_field ^= 1;
  int jr = -2;
  const int dishR = 116;
  for (int y = lq_full ? 0 : lq_field; y <= yMax; y += lq_full ? 1 : 2) {
    int j = y >> 2, fy = y & 3;
    if (jr != j) {
      if (jr == j - 1) lq_wrow(j + 1, lq_w + ((j + 1) & 1) * 3 * LQ_WN, imax);
      else { lq_wrow(j, lq_w + (j & 1) * 3 * LQ_WN, imax); lq_wrow(j + 1, lq_w + ((j + 1) & 1) * 3 * LQ_WN, imax); }
      jr = j;
    }
    const int16_t* A = lq_w + (j & 1) * 3 * LQ_WN;
    const int16_t* B = lq_w + ((j + 1) & 1) * 3 * LQ_WN;
    uint8_t* row = v3_buf + y * W;
    int xs = 0, xe = xMax;
    int ddy = y - 120;
    if (dish) {
      int r2 = dishR * dishR - ddy * ddy;
      if (r2 <= 0) { memset(row, 0, xMax + 1); continue; }
      int hw = (int)v3_isqrt((uint32_t)r2);
      xs = v3_clamp(160 - hw, 0, xMax + 1); xe = v3_clamp(160 + hw, -1, xMax);
      if (xs > 0) memset(row, 0, xs);
      if (xe < xMax) memset(row + xe + 1, 0, xMax - xe);
      if (xs > xe) continue;
    }
    int dy2 = ddy * ddy;
    const int haloT = lq_haloT;
    const int16_t* A1 = A + LQ_WN; const int16_t* B1 = B + LQ_WN; const int16_t* A2 = A + 2 * LQ_WN; const int16_t* B2 = B + 2 * LQ_WN;
    int i0 = xs >> 2;
    int aR = (A[i0] << 4) + ((B[i0] - A[i0]) * fy * 4), bR = (A1[i0] << 4) + ((B1[i0] - A1[i0]) * fy * 4), cR = (A2[i0] << 4) + ((B2[i0] - A2[i0]) * fy * 4);
    for (int i = i0; i <= (xe >> 2); i++) {
      int aL = aR, bL = bR, cL = cR;
      aR = (A[i + 1] << 4) + ((B[i + 1] - A[i + 1]) * fy * 4);
      bR = (A1[i + 1] << 4) + ((B1[i + 1] - A1[i + 1]) * fy * 4);
      cR = (A2[i + 1] << 4) + ((B2[i + 1] - A2[i + 1]) * fy * 4);
      int ddx = i * 4 + 2 - 160;
      int vi = (ddx * ddx + dy2) >> 9; if (vi > 95) vi = 95;
      uint8_t bgA = lq_bgLev[vi], bgB = lq_bgLev2[vi];
      if ((y & 1)) { uint8_t t = bgA; bgA = bgB; bgB = t; }
      int x0 = i * 4, x1 = x0 + 3;
      if (x0 < xs) x0 = xs; if (x1 > xe) x1 = xe;
      if (aL < haloT && aR < haloT && bL < haloT && bR < haloT && cL < haloT && cR < haloT) {   // nothing but lamp here
        for (int x = x0; x <= x1; x++) row[x] = (x & 1) ? bgB : bgA;
        continue;
      }
      int da = (aR - aL) >> 2, db = (bR - bL) >> 2, dc = (cR - cL) >> 2, f = x0 & 3;
      int a = aL + da * f, b = bL + db * f, c = cL + dc * f;
      for (int x = x0; x <= x1; x++, a += da, b += db, c += dc) row[x] = lq_pix(a, b, c, (x & 1) ? bgB : bgA);
    }
  }
  lq_full = false;
  // the dish's glass edge
  if (dish && mirror == 0) {
    for (int a = 0; a < 1024; a += 1) {
      int x = 160 + ((v3_cos(a) * (dishR + 1)) >> 14), y = 120 + ((v3_sin(a) * (dishR + 1)) >> 14);
      v3_put(x, y, (uint8_t)(0x70 | 9));
    }
  }
  // mirrors
  if (mirror >= 1) for (int y = 0; y <= yMax; y++) { uint8_t* r = v3_buf + y * W; for (int x = 0; x < 160; x++) r[319 - x] = r[x]; }
  if (mirror >= 2) for (int y = 0; y < 120; y++) memcpy(v3_buf + (239 - y) * W, v3_buf + y * W, W);

  // ── air bubbles ──
  for (int i = 0; i < nBub; i++) {
    LqBub* u = &lq_bub[i];
    u->ph += sdt;
    float dx = u->x - cx, dy = u->y - cy;
    u->x += (-dy * rot * 0.35f + sinf(u->ph * 0.9f + i) * 6) * sdt;
    u->y += (dx * rot * 0.35f - 7 + cosf(u->ph * 0.7f) * 4) * sdt;
    if (u->y < -12 || u->x < -12 || u->x > W + 12 || (dish && (u->x - 160) * (u->x - 160) + (u->y - 120) * (u->y - 120) > 108.0f * 108.0f)) {
      if (dish) { float a = v3_rndf() * 6.28f, r = 95 * sqrtf(v3_rndf()); u->x = 160 + r * cosf(a); u->y = 120 + r * sinf(a); }
      else { u->x = v3_rnd(W); u->y = H + 10; }
      u->r = 3 + v3_rnd(8); u->ph = 0;
    }
    int bx = (int)u->x, by = (int)u->y, br = (int)u->r;
    for (int m = 0; m < (mirror ? (mirror == 1 ? 2 : 4) : 1); m++) {
      int px = (m & 1) ? 319 - bx : bx, py = (m & 2) ? 239 - by : by;
      lq_ring(px, py, br + 1, light ? (uint8_t)(0xF0 | 8) : (uint8_t)(0x40 | 9));
      lq_ring(px, py, br, light ? (uint8_t)(0xF0 | 9) : (uint8_t)(0xD0 | 9));
      v3_disc(px - br / 3, py - br / 3, br / 4, (uint8_t)(0xF0 | 9));
    }
  }
}

// =====================================================================
// PROGRAM: RUTT — the Rutt/Etra scan processor (by Dewey) — Set 7, slot 1
//
// In 1972 Steve Rutt and Bill Etra built a machine that took a video
// picture apart into its scan lines and drew them again on a vector
// screen — with the brightness of the picture pushing each line upward.
// Faces and words turned into glowing wire landscapes that could be
// tilted, turned and bent. Steina and Woody Vasulka made it famous.
// The same look turned up again on a record cover: the pulsar CP 1919,
// drawn as stacked lines with the ones behind hidden.
//
// How it works: a small picture (64 × 48) is made each frame — words,
// ripples, mountains, a spiral, static, a bouncing ball, a pulsar. Each
// point is lifted by its brightness, the whole raster is tilted and turned
// in 3-D, and the scan lines are drawn as glowing anti-aliased lines (or,
// for "hidden" looks, front to back with the lines behind cut away).
//
// Presets: k0 Rutt/Etra · k1 Ripple · k2 Pulsar · k3 Terrain · k4 Mesh ·
//          k5 Dome · k6 Spiral · k7 Vasulka · k8 Ball · k9 Static · k10 Melt ·
//          k11 Tour
//   k12 — DROP: a ripple runs out through the picture from the middle
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Height (◄ down | up ►) · p6 Lines · p7 Tilt · p8 Turn · p9 Source
//   p10 Wobble · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define RT_W 64
#define RT_H 48
enum { RT_TEXT, RT_RIPPLE, RT_PULSAR, RT_TERRAIN, RT_PLAID, RT_DOME, RT_SPIRAL, RT_STATIC, RT_BALL, RT_NSRC };
enum { RT_LINES, RT_VLINES, RT_MESH, RT_DOTS, RT_HIDDEN };
struct RtLook { uint8_t src, style, word; int8_t height; uint8_t lines, tilt, turn, wobble, glow, pal, flow, spread, scan; };
static const RtLook rt_looks[11] = {
  // src         style      word hgt lines tilt turn wob glow pal              flow spr scan
  { RT_TEXT,    RT_LINES,   0,  55, 85,  35,  30,  0,  40, V7P_PHOSPHOR,     3,  40, 1 },   // Rutt/Etra
  { RT_RIPPLE,  RT_LINES,   0,  45, 60,  55,  20,  0,  30, V7P_VASULKA,      4,  60, 1 },   // Ripple
  { RT_PULSAR,  RT_HIDDEN,  0,  70, 70,  30,   0,  0,   0, V7P_GHOST,        2,  20, 0 },   // Pulsar
  { RT_TERRAIN, RT_HIDDEN,  0,  60, 60,  62,  10,  0,   0, V7P_SUNSET,       4,  70, 1 },   // Terrain
  { RT_PLAID,   RT_MESH,    0,  50, 40,  60,  50,  0,  20, V7P_BLACKLIGHT,   5,  80, 1 },   // Mesh
  { RT_DOME,    RT_VLINES,  0,  70, 60,  45,  35,  0,  30, V7P_CHICAGO,      6,  80, 1 },   // Dome
  { RT_SPIRAL,  RT_LINES,   0,  40, 60,  75,  40,  0,  40, V7P_PAIK,         5,  90, 1 },   // Spiral
  { RT_TEXT,    RT_LINES,   1,  50, 80,  50,  70, 20,  60, V7P_VASULKA,      4,  50, 1 },   // Vasulka
  { RT_BALL,    RT_DOTS,    0,  80, 70,  50,  25,  0,  50, V7P_SCANIMATE,    6,  80, 1 },   // Ball
  { RT_STATIC,  RT_LINES,   0,  35, 60,  40,  15,  0,  20, V7P_AMBER,        3,  40, 1 },   // Static
  { RT_RIPPLE,  RT_MESH,    0,  50, 45,  45,  30, 80,  40, V7P_LUMIA,        5,  80, 1 },   // Melt
};
static const char* const rt_names[12] = { "Rutt/Etra", "Ripple", "Pulsar", "Terrain", "Mesh", "Dome", "Spiral", "Vasulka", "Ball", "Static", "Melt", "Tour" };
static const char* const rt_lbl[6] = { "Height", "Lines", "Tilt", "Turn", "Source", "Wobble" };
static const char* const rt_words[2] = { "VIDEO", "ANALOG" };
const char* prog_rutt_name() { return "RUTT"; }
const char* prog_rutt_character() { return "The Rutt/Etra scan processor: scan lines lifted by brightness into glowing 3-D"; }
const char* prog_rutt_presetName(int p) { return (p >= 0 && p < 12) ? rt_names[p] : NULL; }
const char* prog_rutt_potLabel(int p, int pot) { (void)p; return v7_label(pot, rt_lbl); }
uint8_t prog_rutt_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the source picture, its distance and angle from the middle, the projected points
static uint8_t *rt_src, *rt_r, *rt_a; static int16_t* rt_pt;
static void rt_mem() { rt_src = v6_mem; rt_r = v6_mem + 3072; rt_a = v6_mem + 6144; rt_pt = (int16_t*)(v6_mem + 9216); }

static float rt_hgt, rt_lines, rt_tilt, rt_turn, rt_wob, rt_yaw = 0, rt_drop = -1, rt_pulseT = 0, rt_scroll = 0;
static int rt_lastSrc = -1;

void prog_rutt_init() {
  rt_mem(); v6_init(); rt_lastSrc = -1; rt_drop = -1; rt_yaw = 0;
  for (int j = 0; j < RT_H; j++) for (int i = 0; i < RT_W; i++) {
    int dx = 2 * i - (RT_W - 1), dy = 2 * j - (RT_H - 1);              // half-steps from the middle
    int r = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy)) * 2;          // 0 … ~160
    rt_r[j * RT_W + i] = (uint8_t)v3_clamp(r, 0, 255);
    rt_a[j * RT_W + i] = (uint8_t)(v3_atan(dy, dx) >> 2);
  }
  memset(rt_src, 0, RT_W * RT_H);
}

// ─── The source pictures ──────────────────────────────────────────────
static void rt_blur() {                                               // a soft 3 × 3 blur, in place
  uint8_t prev[RT_W], cur[RT_W];
  memcpy(prev, rt_src, RT_W);
  for (int j = 0; j < RT_H; j++) {
    memcpy(cur, rt_src + j * RT_W, RT_W);
    const uint8_t* nx = j + 1 < RT_H ? rt_src + (j + 1) * RT_W : cur;
    for (int i = 0; i < RT_W; i++) {
      int l = i ? i - 1 : 0, r = i + 1 < RT_W ? i + 1 : i;
      int v = 4 * cur[i] + 2 * (cur[l] + cur[r] + prev[i] + nx[i]) + prev[l] + prev[r] + nx[l] + nx[r];
      rt_src[j * RT_W + i] = (uint8_t)(v >> 4);
    }
    memcpy(prev, cur, RT_W);
  }
}
static void rt_text(const char* s, float scroll) {
  int n = (int)strlen(s), tw = n * 6;                                  // font columns (a space after each letter)
  const int32_t step = 65536 * 7 / 34;                                 // font rows per sample: letters 34 samples tall
  int32_t ox = (int32_t)(scroll * 65536.0f);
  int y0 = (RT_H - 34) / 2;
  memset(rt_src, 0, RT_W * RT_H);
  for (int j = 0; j < 34; j++) {
    int fr = (int)(((int32_t)j * step) >> 16); if (fr > 6) fr = 6;
    for (int i = 0; i < RT_W; i++) {
      int32_t fx = ((int32_t)i * step * 5 / 4 + ox) >> 16;              // a little wider than tall
      fx %= (tw + 6); if (fx < 0) fx += tw + 6;
      int ch = fx / 6, b = fx % 6;
      if (ch >= n || b == 5) continue;
      if (v3_font[v3_glyph(s[ch])][fr] & (16 >> b)) rt_src[(y0 + j) * RT_W + i] = 255;
    }
  }
  rt_blur();
}
static inline int rt_vnoise(int32_t x, int32_t y, uint32_t seed) {      // smooth value noise, x y in 1/256 cells → 0 … 255
  int32_t xi = x >> 8, yi = y >> 8, fx = x & 255, fy = y & 255;
  uint32_t a = v3_hash((uint32_t)xi * 73856093u ^ (uint32_t)yi * 19349663u ^ seed) & 255;
  uint32_t b = v3_hash((uint32_t)(xi + 1) * 73856093u ^ (uint32_t)yi * 19349663u ^ seed) & 255;
  uint32_t c = v3_hash((uint32_t)xi * 73856093u ^ (uint32_t)(yi + 1) * 19349663u ^ seed) & 255;
  uint32_t d = v3_hash((uint32_t)(xi + 1) * 73856093u ^ (uint32_t)(yi + 1) * 19349663u ^ seed) & 255;
  fx = (fx * fx * (768 - 2 * fx)) >> 16; fy = (fy * fy * (768 - 2 * fy)) >> 16;
  int top = a + (((int)(b - a) * fx) >> 8), bot = c + (((int)(d - c) * fx) >> 8);
  return top + (((bot - top) * fy) >> 8);
}
static void rt_source(int src, int word, float t, float sdt) {
  int tt = (int)(t * 256);
  switch (src) {
    case RT_TEXT: rt_scroll += sdt * 4.0f; if (rt_scroll > 1000) rt_scroll -= 1000; rt_text(rt_words[word], rt_scroll); break;
    case RT_RIPPLE:
      for (int k = 0; k < RT_W * RT_H; k++) { int v = 128 + (v3_sin(rt_r[k] * 12 - tt * 2) >> 7); rt_src[k] = (uint8_t)v3_clamp(v * (255 - rt_r[k] / 2) >> 8, 0, 255); }
      break;
    case RT_PULSAR:                                                    // a new pulse profile at the back, the rest move forward
      rt_pulseT += sdt;
      while (rt_pulseT > 0.09f) {
        rt_pulseT -= 0.09f;
        memmove(rt_src + RT_W, rt_src, RT_W * (RT_H - 1));
        int peaks = 2 + v3_rnd(4);
        for (int i = 0; i < RT_W; i++) rt_src[i] = (uint8_t)v3_rnd(18);
        for (int p = 0; p < peaks; p++) {
          int c = RT_W / 2 + v3_rndr(-9, 9), a = 60 + v3_rnd(195), w = 1 + v3_rnd(3);
          for (int i = c - 3 * w; i <= c + 3 * w; i++) if (i >= 0 && i < RT_W) {
            int d = i - c, v = a * 9 / (9 + d * d * 9 / (w * w));
            rt_src[i] = (uint8_t)v3_clamp(rt_src[i] + v, 0, 255);
          }
        }
      }
      break;
    case RT_TERRAIN:
      rt_scroll += sdt * 0.6f; if (rt_scroll > 4000) rt_scroll -= 4000;
      for (int j = 0; j < RT_H; j++) for (int i = 0; i < RT_W; i++) {
        int32_t y = (int32_t)((j - rt_scroll * 8) * 256);
        int v = (rt_vnoise(i * 40, y / 6, 11) * 5 + rt_vnoise(i * 90, y / 3, 23) * 2 + rt_vnoise(i * 200, y * 2 / 3, 37)) >> 3;
        v = v - 70; if (v < 0) v = 0;
        rt_src[j * RT_W + i] = (uint8_t)v3_clamp(v * 2, 0, 255);
      }
      break;
    case RT_PLAID:
      for (int j = 0; j < RT_H; j++) for (int i = 0; i < RT_W; i++) {
        int v = (v3_sin(i * 48 + tt) * v3_sin(j * 56 - tt * 3 / 2)) >> 20;          // −256 … 256
        rt_src[j * RT_W + i] = (uint8_t)v3_clamp(128 + v / 2, 0, 255);
      }
      break;
    case RT_DOME: {
      int cx = (int)(RT_W / 2 + 14 * sinf(t * 0.7f)), cy = (int)(RT_H / 2 + 9 * sinf(t * 0.5f));
      for (int j = 0; j < RT_H; j++) for (int i = 0; i < RT_W; i++) {
        int dx = i - cx, dy = j - cy, d2 = dx * dx + dy * dy;
        int v = 255 - d2 * 255 / 340; rt_src[j * RT_W + i] = (uint8_t)(v > 0 ? (v * v) >> 8 : 0);
      }
    } break;
    case RT_SPIRAL:
      for (int k = 0; k < RT_W * RT_H; k++) {
        int v = 128 + (v3_sin(rt_a[k] * 12 + rt_r[k] * 8 - tt * 3) >> 7);
        rt_src[k] = (uint8_t)v3_clamp(v - rt_r[k] / 3, 0, 255);
      }
      break;
    case RT_STATIC:
      for (int k = 0; k < RT_W * RT_H; k++) { int v = (int)(v3_rand() & 255); rt_src[k] = (uint8_t)((v * v * v) >> 16); }
      break;
    default: {                                                          // a ball bouncing on a floor
      float bx = fmodf(t * 9.0f, 2.0f * (RT_W - 14)); if (bx > RT_W - 14) bx = 2 * (RT_W - 14) - bx;
      float by = fabsf(sinf(t * 2.2f));
      int cx = 7 + (int)bx, cy = 10 + (int)((1 - by) * (RT_H - 22));
      for (int j = 0; j < RT_H; j++) for (int i = 0; i < RT_W; i++) {
        int dx = i - cx, dy = j - cy, d2 = dx * dx + dy * dy, v = 0;
        if (d2 < 49) v = (int)v3_isqrt((uint32_t)(49 - d2) * 1300);
        if (((i >> 3) + (j >> 3)) & 1) v += 30;                         // the chequered floor
        rt_src[j * RT_W + i] = (uint8_t)v3_clamp(v, 0, 255);
      }
    }
  }
}

// a scan line with the lines behind cut away: x0..x1 in px, y in 1/8 px; top[] = highest line drawn so far
static int16_t* rt_top;
static void rt_hiddenSeg(int xa8, int ya8, int xb8, int yb8, uint8_t c, uint8_t edge) {
  if (xb8 < xa8) { int t = xa8; xa8 = xb8; xb8 = t; t = ya8; ya8 = yb8; yb8 = t; }
  int x0 = (xa8 + 4) >> 3, x1 = (xb8 + 4) >> 3;
  if (x1 < 0 || x0 >= W) return;
  int span = x1 - x0;
  int yp = ya8 >> 3;
  for (int x = x0; x <= x1; x++) {
    int y = span ? (ya8 + (yb8 - ya8) * (x - x0) / span) >> 3 : yb8 >> 3;
    if ((unsigned)x >= W) { yp = y; continue; }
    int lo = y < yp ? y : yp, hi = y < yp ? yp : y;                   // join steep pieces
    if (lo < rt_top[x]) {
      int e = v6_min(hi, rt_top[x] - 1);
      for (int yy = lo; yy <= e; yy++) if ((unsigned)yy < H) v3_buf[yy * W + x] = yy == lo ? edge : c;
      rt_top[x] = (int16_t)(lo < 0 ? 0 : lo);
    }
    yp = y;
  }
}

void prog_rutt_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  rt_mem();
  const RtLook& L = rt_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&rt_hgt, v7_kb(5, L.height / 100.0f), rate);
  v6_ease(&rt_lines, v7_kn(6, L.lines / 100.0f), rate);
  v6_ease(&rt_tilt, v7_kn(7, L.tilt / 100.0f), rate);
  v6_ease(&rt_turn, v7_kn(8, L.turn / 100.0f), rate);
  v6_ease(&rt_wob, v7_kn(10, L.wobble / 100.0f), rate);
  int src = v3_centre(9) ? L.src : v3_clamp(v3_k(9) * RT_NSRC / 1024, 0, RT_NSRC - 1);
  int style = L.style;
  if (!v3_centre(9) && style == RT_HIDDEN && src != RT_PULSAR && src != RT_TERRAIN) style = RT_LINES;
  if (src != rt_lastSrc) {
    rt_lastSrc = src; rt_scroll = 0;
    if (src == RT_PULSAR) { memset(rt_src, 0, RT_W * RT_H); rt_pulseT = 0; for (int k = 0; k < RT_H; k++) { rt_pulseT = 0.095f; rt_source(RT_PULSAR, 0, 0, 0); } }
  }
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 110);

  float sdt = v6_f.sdt, t = v6_t;
  rt_source(src, L.word, t, sdt);
  if (v6_k12hit) rt_drop = 0;
  if (rt_drop >= 0) {                                                  // the dropped ripple, running outwards
    rt_drop += sdt * 70.0f;
    int R = (int)rt_drop, amp = (int)(200 * (1.0f - rt_drop / 170.0f));
    if (rt_drop > 170) rt_drop = -1;
    else for (int k = 0; k < RT_W * RT_H; k++) {
      int d = rt_r[k] - R; if (d < -14 || d > 14) continue;
      rt_src[k] = (uint8_t)v3_clamp(rt_src[k] + ((amp * v3_cos(d * 36)) >> 14) * (14 - v3_abs(d)) / 14, 0, 255);
    }
  }

  // ── the picture fades like a phosphor, then the new lines go on top ──
  bool hidden = style == RT_HIDDEN;
  if (hidden) memset(v3_buf, 0, W * H);
  else v7_decay(16 + (int)((1.0f - L.glow / 100.0f) * 70));

  // ── 3-D: tilt (back), turn (round), perspective ──
  float z = v6_zoom(1.0f);
  float tilt = rt_tilt * 1.3f;                                         // 0 … ~75°
  float sway = hidden ? 0.25f : 1.0f;
  rt_yaw += sdt * (rt_turn - 0.25f) * 0.9f * sway;
  if (rt_yaw > 6.2832f) rt_yaw -= 6.2832f; if (rt_yaw < 0) rt_yaw += 6.2832f;
  float yaw = hidden ? 0.35f * sinf(rt_yaw) * rt_turn : (rt_turn < 0.3f ? 0.5f * sinf(rt_yaw * 2) * rt_turn : rt_yaw);
  int32_t ca = (int32_t)(cosf(tilt) * 4096), sa = (int32_t)(sinf(tilt) * 4096);
  int32_t cb = (int32_t)(cosf(yaw) * 4096), sb = (int32_t)(sinf(yaw) * 4096);
  int32_t sp16 = (int32_t)(4.9f * z * 16);                              // grid spacing, 1/16 px
  int32_t hk = (int32_t)(rt_hgt * 0.30f * z * 16 * 256);                 // height per luma step, ×256
  int32_t D = 420 * 16, F = 420;
  int cy16 = (int)((H / 2 + 22 * sinf(tilt)) * 16);
  int wobA = (int)(rt_wob * 30 * 16), wobT = (int)(t * 300);
  // project every point (x, y in 1/8 px)
  for (int j = 0; j < RT_H; j++) {
    int32_t Y = (int32_t)(2 * j - (RT_H - 1)) * sp16 / 2;
    int32_t Zp = -(Y * sa) >> 12, Yp = (Y * ca) >> 12;
    int32_t wx = wobA ? (v3_sin(j * 40 + wobT) * wobA) >> 14 : 0;
    for (int i = 0; i < RT_W; i++) {
      int k = j * RT_W + i;
      int32_t X = (int32_t)(2 * i - (RT_W - 1)) * sp16 / 2 + wx;
      int32_t Xr = (X * cb + Zp * sb) >> 12, Zr = (-X * sb + Zp * cb) >> 12;
      int32_t Yr = Yp - ((rt_src[k] * hk) >> 8);
      if (wobA) Yr += (v3_sin(i * 50 - wobT * 2) * (wobA / 3)) >> 14;
      int32_t den = D + Zr; if (den < D / 4) den = D / 4;
      int32_t s = (F << 16) / den;                                     // ×65536 / 16
      rt_pt[2 * k] = (int16_t)v3_clamp((int)(HALFW * 8 + ((Xr * s) >> 13)), -16000, 16000);
      rt_pt[2 * k + 1] = (int16_t)v3_clamp((int)(cy16 / 2 + ((Yr * s) >> 13)), -16000, 16000);
    }
  }
  // ── draw ──
  int nl = 8 + (int)(rt_lines * (RT_H - 8));                            // scan lines drawn
  int stepJ = (RT_H * 16) / nl;                                         // in 1/16 rows
  if (hidden) {
    int16_t top[W]; rt_top = top;                                        // per column: the highest line so far
    for (int x = 0; x < W; x++) top[x] = H;
    for (int jj = (RT_H - 1) * 16; jj >= 0; jj -= stepJ) {
      int j = jj >> 4;
      for (int i = 0; i + 1 < RT_W; i++) {
        int k = j * RT_W + i;
        int lum = (rt_src[k] + rt_src[k + 1]) >> 1;
        int hue = (j * 16 / RT_H) & 15;
        rt_hiddenSeg(rt_pt[2 * k], rt_pt[2 * k + 1], rt_pt[2 * k + 2], rt_pt[2 * k + 3], V6C(8 + lum * 7 / 255, hue), V6C(15, hue));
      }
    }
  } else {
    bool hl = style == RT_LINES || style == RT_MESH, vl = style == RT_VLINES || style == RT_MESH;
    int lev0 = style == RT_MESH ? 4 : 6;
    if (hl) for (int jj = 0; jj < RT_H * 16; jj += stepJ) {
      int j = jj >> 4;
      for (int i = 0; i + 1 < RT_W; i++) {
        int k = j * RT_W + i, lum = (rt_src[k] + rt_src[k + 1]) >> 1;
        v6_lineAdd(rt_pt[2 * k], rt_pt[2 * k + 1], rt_pt[2 * k + 2], rt_pt[2 * k + 3], lev0 + lum * (15 - lev0) / 255, (lum >> 4) & 15);
      }
    }
    if (vl) {
      int nv = 8 + (int)(rt_lines * (RT_W - 8)), stepI = (RT_W * 16) / nv;
      for (int ii = 0; ii < RT_W * 16; ii += stepI) {
        int i = ii >> 4;
        for (int j = 0; j + 1 < RT_H; j++) {
          int k = j * RT_W + i, k2 = k + RT_W, lum = (rt_src[k] + rt_src[k2]) >> 1;
          v6_lineAdd(rt_pt[2 * k], rt_pt[2 * k + 1], rt_pt[2 * k2], rt_pt[2 * k2 + 1], lev0 + lum * (15 - lev0) / 255, ((lum >> 4) + 8) & 15);
        }
      }
    }
    if (style == RT_DOTS) {
      int stepI = (RT_W * 16) / (8 + (int)(rt_lines * (RT_W - 8)));
      for (int jj = 0; jj < RT_H * 16; jj += stepJ) for (int ii = 0; ii < RT_W * 16; ii += stepI) {
        int k = (jj >> 4) * RT_W + (ii >> 4), lum = rt_src[k];
        v6_dot(rt_pt[2 * k], rt_pt[2 * k + 1], 1 + lum / 128, 6 + lum * 9 / 255, (lum >> 4) & 15);
      }
    }
  }
  v7_end(L.scan);
}

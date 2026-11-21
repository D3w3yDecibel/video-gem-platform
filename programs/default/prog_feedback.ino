// =====================================================================
// PROGRAM: FEEDBACK — a camera pointed at its own monitor (by Dewey) — Set 7, slot 2
//
// Point a video camera at the monitor it is feeding and the picture falls
// into itself: tunnels, spirals, nested screens, smoke that curls upward —
// the oldest trick in video art, from Nam June Paik and the Vasulkas to
// every 1970s TV studio. Zoom the camera in and the picture rushes out of
// the middle; zoom out and it shrinks into an endless corridor of screens;
// tilt the camera and it all spirals.
//
// How it works: PicoDVI keeps the frame on screen while the next is drawn,
// so each new frame is "filmed" from the one on screen — every pixel is
// fetched from a zoomed, turned and shifted spot in the last frame — a
// generation darker and a hue further round. A small shape (the "seed")
// is then drawn in the middle for the loop to multiply.
//
// Presets: k0 Tunnel · k1 Spiral · k2 Corridor · k3 Hall of Mirrors ·
//          k4 Smoke · k5 Kaleido · k6 Shimmer · k7 Galaxy · k8 Echo ·
//          k9 Strobe · k10 Vortex · k11 Tour
//   k12 — FLASH: a burst of light for the loop to carry away
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (seed size) · p3 Colour Flow · p4 Hue Spread
//   p5 Camera Zoom (◄ out | in ►) · p6 Rotate (◄ | ►) · p7 Drift (◄ down | up ►)
//   p8 Trails (short | long) · p9 Hue Step · p10 Seed · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { FB_CLIP, FB_WRAP, FB_MIRROR };
enum { FB_RING, FB_SQUARE, FB_PEN, FB_BARS, FB_DOTS, FB_STAR, FB_TRI, FB_NSEED };
struct FbLook { int8_t zoom, rot, drift; uint8_t trail, hue, mode, seed, flash, wave, frame, fill, rad, pal, flow, spread; };
// zoom: % per frame ×10 (35 = 1.035) · rot: degrees per frame ×10 · drift: px per frame ×10
static const FbLook fb_looks[11] = {
  // zoom rot drift trail hue mode       seed       flash wave frame fill rad pal              flow spr
  {  35,  12,   0,  70,  1, FB_CLIP,   FB_RING,   0,   0,   0,   0, 10, V7P_CHICAGO,      4,  90 },   // Tunnel
  {  20,  40,   0,  75,  1, FB_CLIP,   FB_SQUARE, 0,   0,   0,   0, 10, V7P_PAIK,         5, 100 },   // Spiral
  { -30,   4,   0,  85,  1, FB_CLIP,   FB_SQUARE, 0,   0,   1,   1, 10, V7P_VASULKA,      3,  70 },   // Corridor
  {  25,  10,   0,  80,  1, FB_WRAP,   FB_STAR,   0,   0,   0,   1, 14, V7P_SCANIMATE,    4,  90 },   // Hall of Mirrors
  {   4,   0,  15,  90,  0, FB_CLIP,   FB_DOTS,   0,  25,   0,   0,  6, V7P_LUMIA,        3,  60 },   // Smoke
  {  30,  20,   0,  75,  1, FB_MIRROR, FB_STAR,   0,   0,   0,   1,  8, V7P_BLACKLIGHT,   5,  90 },   // Kaleido
  {  12,  -6,   0,  75,  1, FB_CLIP,   FB_BARS,   0,  30,   0,   0, 10, V7P_SUNSET,       4,  80 },   // Shimmer
  { -12,  30,   0,  92,  0, FB_CLIP,   FB_DOTS,   0,   0,   0,   0, 45, V7P_LUMIA,        5,  90 },   // Galaxy
  {   5,   0,   0,  93,  1, FB_CLIP,   FB_PEN,    0,   0,   0,   0, 10, V7P_PHOSPHOR,     3,  60 },   // Echo
  {  50,   5,   0,  70,  1, FB_CLIP,   FB_RING,   1,   0,   0,   0, 10, V7P_STUDIO,       6, 100 },   // Strobe
  {  60,  60,   0,  65,  2, FB_CLIP,   FB_TRI,    0,   0,   0,   1, 10, V7P_KODACHROME,   5, 100 },   // Vortex
};
static const char* const fb_names[12] = { "Tunnel", "Spiral", "Corridor", "Hall of Mirrors", "Smoke", "Kaleido", "Shimmer", "Galaxy", "Echo", "Strobe", "Vortex", "Tour" };
static const char* const fb_lbl[6] = { "Camera Zoom", "Rotate", "Drift", "Trails", "Hue Step", "Seed" };
const char* prog_feedback_name() { return "FEEDBACK"; }
const char* prog_feedback_character() { return "A camera pointed at its own monitor: tunnels, spirals and endless screens"; }
const char* prog_feedback_presetName(int p) { return (p >= 0 && p < 12) ? fb_names[p] : NULL; }
const char* prog_feedback_potLabel(int p, int pot) { (void)p; return v7_label(pot, fb_lbl); }
uint8_t prog_feedback_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: two generation tables (a fraction of a level is spread over alternate rows)
static uint8_t *fb_genA, *fb_genB;
static void fb_mem() { fb_genA = v6_mem; fb_genB = v6_mem + 256; }

static float fb_zoom, fb_rot, fb_drift, fb_trail, fb_hue, fb_flash = 0, fb_strobe = 0, fb_huePh = 0, fb_ang = 0;

void prog_feedback_init() { fb_mem(); v6_init(); fb_flash = 0; }

// one row "filmed" from the last frame: source point (u, v) in 16.16, steps per pixel
__attribute__((optimize("O2"))) static void fb_rowClip(uint8_t* out, const uint8_t* src, int32_t u, int32_t v, int32_t du, int32_t dv, const uint8_t* gen) {
  for (int x = 0; x < W; x++, u += du, v += dv) {
    uint32_t sx = (uint32_t)(u >> 16), sy = (uint32_t)(v >> 16);
    out[x] = (sx < W && sy < H) ? gen[src[sy * W + sx]] : 0;
  }
}
__attribute__((optimize("O2"))) static void fb_rowWrap(uint8_t* out, const uint8_t* src, int32_t u, int32_t v, int32_t du, int32_t dv, const uint8_t* gen) {
  for (int x = 0; x < W; x++, u += du, v += dv) {
    int32_t sx = u >> 16, sy = v >> 16;
    if ((uint32_t)sx >= W) { sx %= W; if (sx < 0) sx += W; }
    if ((uint32_t)sy >= H) { sy %= H; if (sy < 0) sy += H; }
    out[x] = gen[src[sy * W + sx]];
  }
}
__attribute__((optimize("O2"))) static void fb_rowMirror(uint8_t* out, const uint8_t* src, int32_t u, int32_t v, int32_t du, int32_t dv, const uint8_t* gen) {
  for (int x = 0; x < W; x++, u += du, v += dv) {
    int32_t sx = u >> 16, sy = v >> 16;
    sx -= HALFW; if (sx < 0) sx = -1 - sx; sx += HALFW;                // folded about the middle
    sy -= HALFH; if (sy < 0) sy = -1 - sy; sy += HALFH;
    out[x] = ((uint32_t)sx < W && (uint32_t)sy < H) ? gen[src[sy * W + sx]] : 0;
  }
}

static void fb_seed(int seed, float t, float size, int hue, bool flash, bool fill, float rad) {
  int cx = HALFW * 8, cy = HALFH * 8;
  int R = (int)(size * 8);
  switch (seed) {
    case FB_RING: v6_ring(cx, cy, (int)(R * (0.8f + 0.2f * sinf(t * 3.1f))), 0, 1024, 15, hue, 1); break;
    case FB_SQUARE: case FB_TRI: case FB_STAR: {
      int n = seed == FB_SQUARE ? 4 : seed == FB_TRI ? 3 : 10;
      int a0 = (int)(t * 90);
      int32_t xs[11], ys[11];
      for (int i = 0; i < n; i++) {
        int a = a0 + i * 1024 / n, r = (seed == FB_STAR && (i & 1)) ? R * 2 / 5 : R;
        xs[i] = 2 * (cx + ((v3_cos(a) * r) >> 14)); ys[i] = 2 * (cy + ((v3_sin(a) * r) >> 14));   // 1/16 px
      }
      if (fill) v3_poly(xs, ys, n, V6C(15, hue));
      for (int i = 0; i < n; i++) { int j = (i + 1) % n; v6_lineAdd(xs[i] / 2, ys[i] / 2, xs[j] / 2, ys[j] / 2, 15, (hue + 5) & 15); }
    } break;
    case FB_PEN: {                                                     // a Lissajous pen
      int qx = cx + (int)(R * 3.0f * sinf(t * 1.7f)), qy = cy + (int)(R * 2.2f * sinf(t * 2.3f + 0.6f));
      v6_dot(qx, qy, 4, 15, hue);
    } break;
    case FB_BARS: {
      int y = cy / 8 + (int)(size * 2.4f * sinf(t * 1.3f));
      int w = (int)(size * 2.0f);
      for (int k = -2; k <= 2; k++) v3_hspan(HALFW - w, HALFW + w, y + k, V6C(15 - v3_abs(k) * 3, hue));
    } break;
    default:                                                            // sparks, near the middle or out on a ring
      for (int i = 0; i < 6; i++) {
        float a = v3_rndf() * 6.2832f, r = size * (rad > 1.5f ? rad * (0.8f + 0.4f * v3_rndf()) : v3_rndf() * rad);
        v6_dot(cx + (int)(cosf(a) * r * 8), cy + (int)(sinf(a) * r * 8), 1 + v3_rnd(2), 15, (hue + v3_rnd(3)) & 15);
      }
  }
  if (flash) v3_disc(HALFW, HALFH, (int)(size * 1.4f), V6C(15, (hue + 8) & 15));
}

void prog_feedback_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  fb_mem();
  const FbLook& L = fb_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&fb_zoom, v7_kb(5, L.zoom / 100.0f), rate);
  v6_ease(&fb_rot, v7_kb(6, L.rot / 60.0f), rate);
  v6_ease(&fb_drift, v7_kb(7, L.drift / 30.0f), rate);
  v6_ease(&fb_trail, v7_kn(8, L.trail / 100.0f), rate);
  v6_ease(&fb_hue, v3_centre(9) ? (float)L.hue : v3_u(9) * 4.0f, rate);
  int seed = v3_centre(10) ? L.seed : v3_clamp(v3_k(10) * FB_NSEED / 1024, 0, FB_NSEED - 1);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 120);

  float sdt = v6_f.sdt, k = sdt * 60.0f;                               // 1 = one 60 Hz frame of camera movement
  // ── the camera: zoom (per frame), rotation, drift ──
  float zf = powf(1.0f + fb_zoom * 0.10f, k);                          // ±10 % per frame at the ends
  float ang = fb_rot * 6.0f * 0.01745f * k;                            // up to 6° a frame
  float dy = -fb_drift * 3.0f * k;
  float inv = 1.0f / zf, c = cosf(ang) * inv, s = sinf(ang) * inv;
  int32_t du = (int32_t)(c * 65536), dv = (int32_t)(-s * 65536);       // source step per screen pixel →
  int32_t duy = (int32_t)(s * 65536), dvy = (int32_t)(c * 65536);       // … and per screen row ↓
  int32_t u = (int32_t)((HALFW - (HALFW * c + HALFH * s)) * 65536) + 32768;
  int32_t v = (int32_t)((HALFH - (-HALFW * s + HALFH * c) - dy) * 65536) + 32768;

  // ── the generation tables: a little darker, a hue further round ──
  int dec16 = k > 0 ? (int)((1.0f - fb_trail) * (1.0f - fb_trail) * 64 * k) + 1 : 0;   // sixteenths of a level per frame
  if (k > 0 && fb_trail > 0.97f) dec16 = 1;
  fb_huePh += fb_hue * k * 0.5f;
  int hstep = (int)fb_huePh; fb_huePh -= hstep;
  for (int g = 0; g < 2; g++) {
    uint8_t* gen = g ? fb_genB : fb_genA;
    int d = (dec16 >> 4) + g;
    for (int i = 0; i < 256; i++) {
      int l = (i >> 4) - d, h = (i + hstep) & 15;
      gen[i] = l > 0 ? (uint8_t)((l << 4) | h) : 0;
    }
  }
  const uint8_t* src = v7_front ? v7_front : v3_buf;
  float t0 = v6_t;
  int frac = dec16 & 15;
  uint8_t tmp[W];
  int wave = (int)(L.wave * 0.1f * 65536 * (k > 0 ? 1 : 0)), wt = (int)(t0 * 260);
  for (int y = 0; y < H; y++, u += duy, v += dvy) {
    int32_t uw = wave ? u + (int32_t)(((int64_t)v3_sin(y * 9 + wt) * wave) >> 14) : u;
    const uint8_t* gen = ((((y * 7) + v7_frameNo * 5) & 15) < frac) ? fb_genB : fb_genA;
    uint8_t* out = src == v3_buf ? tmp : v3_buf + y * W;
    if (L.mode == FB_WRAP) fb_rowWrap(out, src, uw, v, du, dv, gen);
    else if (L.mode == FB_MIRROR) fb_rowMirror(out, src, uw, v, du, dv, gen);
    else fb_rowClip(out, src, uw, v, du, dv, gen);
    if (out == tmp) memcpy(v3_buf + y * W, tmp, W);                     // (no last frame yet: work in place)
  }

  // ── the seed, and the flash ──
  float t = v6_t;
  fb_ang += sdt;
  float size = 26.0f * v6_zoom(1.0f);
  int hue = ((int)(t * 2.0f)) & 15;
  bool flash = false;
  if (v6_k12hit) fb_flash = 1.0f;
  if (fb_flash > 0) { flash = fb_flash > 0.8f; fb_flash -= v3_dt * 4.0f; }
  bool on = true;
  if (L.flash) { fb_strobe += sdt * 2.0f; on = (fb_strobe - floorf(fb_strobe)) < 0.25f; }
  if (L.frame) {                                                       // the monitor's own edge, filmed again and again
    uint8_t fc = V6C(15, (hue + 4) & 15);
    for (int k = 0; k < 3; k++) { v3_hspan(0, W - 1, k, fc); v3_hspan(0, W - 1, H - 1 - k, fc); v3_rect(k, 0, k, H - 1, fc); v3_rect(W - 1 - k, 0, W - 1 - k, H - 1, fc); }
  }
  if (on) fb_seed(seed, t, size, hue, flash || (L.flash && seed == FB_RING), L.fill, L.rad / 10.0f);
  else if (flash) fb_seed(FB_RING, t, size, hue, true, false, 1.0f);
  v7_end(0);
}

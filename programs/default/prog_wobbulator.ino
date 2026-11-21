// =====================================================================
// PROGRAM: WOBBULATOR — Paik and Abe's Wobbulator (by Dewey) — Set 7, slot 4
//
// Nam June Paik and Shuya Abe wound extra deflection coils onto the neck
// of a black-and-white TV and fed them from audio oscillators. Whatever
// was on screen — a face, a test card, nothing at all — got folded,
// twisted, wrung into spirals and wobbling ribbons, because the beam that
// draws the picture was being pushed about while it drew. The Wobbulator
// is still in use at the Experimental Television Center.
//
// How it works: the picture (a small 64 × 48 image) is drawn as scan lines
// — 84 lines of 136 points — and every point of the beam's path is pushed
// by two oscillators (across and down), turned (twist), and bent from the
// usual raster toward a circular one (each line a ring: the spiral raster).
// The lines glow and fade like a phosphor, brighter where the folds overlap and
// dimmer where the beam is flung across the screen fast.
//
// Presets: k0 Spiral · k1 Fold · k2 Twist · k3 Ribbon · k4 Lissajous ·
//          k5 Funnel · k6 Wave · k7 Bent Bars · k8 TV Twist · k9 Pinch ·
//          k10 Flutter · k11 Tour
//   k12 — KICK: a burst of deflection that rings away
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Across Deflect · p6 Down Deflect · p7 Frequency · p8 Spiral (◄ raster | rings ►)
//   p9 Picture · p10 Twist (◄ | ►) · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define WB_SW 64
#define WB_SH 48
#define WB_NL 84
#define WB_NS 136
enum { WB_BARS, WB_GLOW, WB_TEXT, WB_GRID, WB_STRIPES, WB_CHECK, WB_NSRC };
struct WbLook { uint8_t src, ax, ay, fx, fy, gx, gy, spiral; int8_t twist; uint8_t pinch, glow, pal, flow, spread, scan; };
// ax ay: deflection, % of half the screen · fx fy: cycles along a line ×10 · gx gy: cycles down the lines ×10
static const WbLook wb_looks[11] = {
  // src         ax  ay  fx  fy  gx  gy  spir twist pinch glow pal              flow spr scan
  { WB_GLOW,      8,  8, 30, 20,  0, 10, 100,  20,   0,  60, V7P_VASULKA,      4,  70, 1 },   // Spiral
  { WB_STRIPES,  55,  5, 25,  0, 15,  0,   0,   0,   0,  50, V7P_CHICAGO,      5,  90, 1 },   // Fold
  { WB_GRID,     10, 10, 10, 10,  0,  0,   0,  90,   0,  50, V7P_PHOSPHOR,     3,  40, 1 },   // Twist
  { WB_BARS,      0, 45,  0, 15, 10,  0,   0,   0,   0,  50, V7P_STUDIO,       0, 100, 1 },   // Ribbon
  { WB_GLOW,     40, 40, 20, 30,  5,  5,  20,  10,   0,  65, V7P_PAIK,         6,  90, 1 },   // Lissajous
  { WB_CHECK,    10, 25, 10, 20,  0, 15, 100, -40,  60,  60, V7P_SCANIMATE,    5,  90, 1 },   // Funnel
  { WB_GLOW,     15, 15,  5,  5, 10, 10,  10,   0,   0,  70, V7P_LUMIA,        3,  70, 1 },   // Wave
  { WB_BARS,     35, 15, 15, 10, 10,  0,   0,  15,   0,  50, V7P_STUDIO,       0, 100, 1 },   // Bent Bars
  { WB_TEXT,     15, 10, 10,  5,  0, 10,  30,  60,   0,  55, V7P_AMBER,        3,  50, 1 },   // TV Twist
  { WB_STRIPES,  10, 10, 10, 10,  0,  0,  50,   0,  90,  60, V7P_BLACKLIGHT,   5, 100, 1 },   // Pinch
  { WB_GRID,     12, 12, 90, 80, 40, 40,   0,   0,   0,  45, V7P_GHOST,        2,  50, 1 },   // Flutter
};
static const char* const wb_names[12] = { "Spiral", "Fold", "Twist", "Ribbon", "Lissajous", "Funnel", "Wave", "Bent Bars", "TV Twist", "Pinch", "Flutter", "Tour" };
static const char* const wb_lbl[6] = { "Across Deflect", "Down Deflect", "Frequency", "Spiral: Raster|Rings", "Picture", "Twist" };
const char* prog_wobbulator_name() { return "WOBBULATOR"; }
const char* prog_wobbulator_character() { return "Paik and Abe's Wobbulator: oscillators fold, twist and spiral the raster"; }
const char* prog_wobbulator_presetName(int p) { return (p >= 0 && p < 12) ? wb_names[p] : NULL; }
const char* prog_wobbulator_potLabel(int p, int pot) { (void)p; return v7_label(pot, wb_lbl); }
uint8_t prog_wobbulator_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static uint8_t* wb_pic;
static void wb_mem() { wb_pic = v6_mem; }
static int wb_madeSrc = -1;
static int32_t wb_rcp[14];                                                 // 8192 / n: one spot per pixel along a short stroke
static float wb_ax, wb_ay, wb_fq, wb_sp, wb_tw, wb_kick = 0, wb_phx = 0, wb_phy = 0, wb_pht = 0;

void prog_wobbulator_init() { wb_mem(); v6_init(); wb_madeSrc = -1; wb_kick = 0; for (int n = 1; n < 14; n++) wb_rcp[n] = 8192 / n; }

static void wb_make(int src) {
  for (int y = 0; y < WB_SH; y++) for (int x = 0; x < WB_SW; x++) {
    int dx = 2 * x - WB_SW + 1, dy = 2 * y - WB_SH + 1, c;
    switch (src) {
      case WB_BARS: c = y < WB_SH * 3 / 4 ? V6C(13, (x * 7 / WB_SW) * 2) : V6C(4 + (x * 10 / WB_SW), 15); break;
      case WB_GLOW: { int r = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy)); c = V6C(v3_clamp(15 - r / 6, 3, 15), (r / 5) & 15); } break;
      case WB_GRID: c = ((x & 7) == 0 || (y & 7) == 0) ? V6C(15, ((x >> 3) + (y >> 3)) & 15) : V6C(3, 8); break;
      case WB_STRIPES: c = V6C(((x + y) & 4) ? 14 : 4, ((x + y) >> 2) & 15); break;
      case WB_CHECK: c = (((x >> 3) + (y >> 3)) & 1) ? V6C(14, (x >> 2) & 15) : V6C(3, (y >> 2) & 15); break;
      default: c = V6C(3, 9);
    }
    wb_pic[y * WB_SW + x] = (uint8_t)c;
  }
  if (src == WB_TEXT) {                                                 // TV, big
    const char* s = "TV"; int sc = 4, x0 = (WB_SW - (12 * sc - sc)) / 2, y0 = (WB_SH - 7 * sc) / 2;
    for (int i = 0; s[i]; i++) {
      const uint8_t* g = v3_font[v3_glyph(s[i])];
      for (int r = 0; r < 7; r++) for (int b = 0; b < 5; b++) if (g[r] & (16 >> b))
        for (int yy = 0; yy < sc; yy++) for (int xx = 0; xx < sc; xx++)
          wb_pic[(y0 + r * sc + yy) * WB_SW + x0 + (i * 6 + b) * sc + xx] = V6C(15, i ? 6 : 1);
    }
  }
}

void prog_wobbulator_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  wb_mem();
  const WbLook& L = wb_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&wb_ax, v7_kn(5, L.ax / 100.0f), rate);
  v6_ease(&wb_ay, v7_kn(6, L.ay / 100.0f), rate);
  v6_ease(&wb_fq, v3_centre(7) ? 1.0f : powf(4.0f, v3_bi(7)), rate);
  v6_ease(&wb_sp, v7_kn(8, L.spiral / 100.0f), rate);
  v6_ease(&wb_tw, v7_kb(10, L.twist / 100.0f), rate);
  int src = v3_centre(9) ? (look == 8 ? WB_TEXT : L.src) : v3_clamp(v3_k(9) * WB_NSRC / 1024, 0, WB_NSRC - 1);
  if (src != wb_madeSrc) { wb_make(src); wb_madeSrc = src; }
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 110);

  float sdt = v6_f.sdt, t = v6_t;
  wb_phx += sdt * 0.37f; wb_phy += sdt * 0.29f; wb_pht += sdt * 0.21f;
  if (wb_phx > 1000) wb_phx -= 1000; if (wb_phy > 1000) wb_phy -= 1000; if (wb_pht > 1000) wb_pht -= 1000;
  if (v6_k12hit) wb_kick = 1.0f;
  float kick = wb_kick * wb_kick * sinf(t * 23.0f);
  if (wb_kick > 0) { wb_kick -= v3_dt * 0.8f; if (wb_kick < 0) wb_kick = 0; }

  v7_decay(10 + (int)((1.0f - L.glow / 100.0f) * 60));

  float z = v6_zoom(1.0f);
  int32_t hw8 = (int32_t)(150 * 8 * z), hh8 = (int32_t)(112 * 8 * z);   // raster half-size, 1/8 px
  int32_t rMax8 = (int32_t)(118 * 8 * z), rMin8 = (int32_t)(6 * 8 * z);
  int32_t AX = (int32_t)((wb_ax + fabsf(kick) * 0.5f) * 150 * 8 * z), AY = (int32_t)((wb_ay + kick * 0.3f) * 112 * 8 * z);
  float fq = wb_fq;
  int32_t fx = (int32_t)(L.fx * 0.1f * fq * 1024 / WB_NS * 64), fy = (int32_t)(L.fy * 0.1f * fq * 1024 / WB_NS * 64);   // per point, 1024/turn ×64
  int32_t gx = (int32_t)(L.gx * 0.1f * 1024 / WB_NL * 64), gy = (int32_t)(L.gy * 0.1f * 1024 / WB_NL * 64);           // per line
  int32_t phx = (int32_t)(wb_phx * 1024 * 64 * (1.0f + L.fx * 0.02f)), phy = (int32_t)(wb_phy * 1024 * 64 * (1.0f + L.fy * 0.02f));
  int32_t sp = (int32_t)(wb_sp * 256);
  int32_t twK = (int32_t)(wb_tw * 700);                                    // whole twist top to bottom, 1024/turn
  int32_t pinch = (int32_t)(L.pinch * 0.01f * 4096);
  int ptw = (int)(wb_pht * 1024 * 2);
  // per point, the same every line: the raster column, the ring direction, the picture column
  static int16_t xRt[WB_NS], cAt[WB_NS], sAt[WB_NS];
  for (int i = 0; i < WB_NS; i++) {
    xRt[i] = (int16_t)((int32_t)(2 * i - WB_NS + 1) * hw8 / WB_NS);
    int a = i * 1024 / WB_NS + 256; cAt[i] = (int16_t)(v3_cos(a) >> 1); sAt[i] = (int16_t)(v3_sin(a) >> 1);   // Q13
  }
  int32_t drStep = (rMax8 - rMin8) * 65536 / WB_NL / WB_NS;                // the spiral's growth per point, ×65536
  for (int j = 0; j < WB_NL; j++) {
    int sy = j * WB_SH / WB_NL;
    const uint8_t* srow = wb_pic + sy * WB_SW;
    int32_t vy = (int32_t)(2 * j - WB_NL + 1);                             // −95 … 95
    int32_t yR = vy * hh8 / WB_NL;                                         // raster: the line's height
    int32_t rr = rMin8 + (int32_t)j * (rMax8 - rMin8) / WB_NL;              // rings: the line's radius
    int ta = (int)((vy * twK) / WB_NL) + (int)((v3_sin(ptw >> 1) * twK) >> 15);
    int32_t tc = v3_cos(ta), ts = v3_sin(ta);
    int32_t psc = 4096;
    if (pinch) psc = 4096 - ((pinch * (16384 + v3_sin(j * 12 + ptw))) >> 15);  // the raster squeezed in and out
    int32_t pxA = phx + j * gx, pyA = phy + j * gy;
    int px = 0, py = 0, plev = 0, phue = 0;
    int32_t rq = rr * 65536;
    for (int i = 0; i < WB_NS; i++, pxA += fx, pyA += fy, rq += drStep) {
      int32_t X = xRt[i], Y = yR;
      if (sp) {                                                            // toward the ring raster: each ring flows into the next (a spiral)
        int32_t r = rq >> 16;
        int32_t xP = (r * cAt[i]) >> 13, yP = (r * sAt[i]) >> 13;
        X += ((xP - X) * sp) >> 8; Y += ((yP - Y) * sp) >> 8;
      }
      X += (AX * v3_sin((int)(pxA >> 6))) >> 14;
      Y += (AY * v3_sin((int)(pyA >> 6))) >> 14;
      if (pinch) { X = (X * psc) >> 12; Y = (Y * psc) >> 12; }
      int32_t Xr = (X * tc - Y * ts) >> 14, Yr = (X * ts + Y * tc) >> 14;
      int qx = HALFW * 8 + Xr, qy = HALFH * 8 + Yr;
      uint8_t c = srow[i * WB_SW / WB_NS];
      int lev = (c >> 4) * 7 / 15, hue = c & 15;
      if (i && (lev || plev)) {
        int dx = qx - px, dy = qy - py, l = (lev + plev + 1) >> 1, h = lev >= plev ? hue : phue;
        int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy, len = adx > ady ? adx : ady;
        if (len < 96) {                                                    // up to 12 px: the beam's spots, about a pixel apart
          int n = (len >> 3) + 1;
          int32_t sx = dx * wb_rcp[n], sy = dy * wb_rcp[n];                // per spot, ×65536 / 8
          int32_t ax = (px << 13) + sx, ay = (py << 13) + sy;
          int lf = len < 40 ? l : (l * 40) / len;
          for (int k = 0; k < n; k++, ax += sx, ay += sy) v6_add((ax + 32768) >> 16, (ay + 32768) >> 16, lf, h);
        } else {
          // the beam is dimmer where it moves fast (like a real tube): long fast strokes fade to a faint trace
          int lf = (l * 40) / len;
          if (lf >= 1) v6_lineAdd(px, py, qx, qy, lf, h);
          else if (len < 240) v6_add((px + qx + 8) >> 4, (py + qy + 8) >> 4, 1, h);
        }
      }
      px = qx; py = qy; plev = lev; phue = hue;
    }
  }
  v7_end(L.scan);
}

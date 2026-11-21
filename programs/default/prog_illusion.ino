// =====================================================================
// PROGRAM: ILLUSION — the classic optical illusions, moving (by Dewey) — Set 7, slot 7
//
// Pictures that the eye gets wrong: straight lines that look tilted (the
// café wall, Zöllner, Hering), dots that flash black where there are none
// (the scintillating grid, Hermann's grid), a disc that floats loose from
// its background (Ouchi), still patterns that seem to drift (Kitaoka),
// a flat chequerboard that bulges, feet that seem to step, a missing lilac
// disc that turns green, coloured blobs that fade away while you stare.
// Each is slowly animated so the effect keeps changing.
//
// How it works: each illusion is drawn row by row from small column and
// row tables (stripes, bricks, grids are all "which band am I in" sums),
// so every one of them costs little. The colour illusions (lilac chaser,
// fading blobs) set a few palette rows to run from grey to their colour.
//
// Presets: k0 Café Wall · k1 Scintillating Grid · k2 Ouchi · k3 Hermann Grid ·
//          k4 Zöllner · k5 Drift · k6 Lilac Chaser · k7 Hering · k8 Bulge ·
//          k9 Stepping Feet · k10 Fading · k11 Tour
//   k12 — STILL: hold to stop the motion (and watch the illusion keep moving)
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Size · p6 Motion · p7 Contrast · p8 Line Width · p9 Tilt · p10 Variation
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { IL_CAFE, IL_SCINT, IL_OUCHI, IL_HERMANN, IL_ZOLLNER, IL_DRIFT, IL_LILAC, IL_HERING, IL_BULGE, IL_FEET, IL_FADE };
struct IlLook { uint8_t size, motion, contrast, line, tilt, vary, pal, flow, spread, sat; };
static const IlLook il_looks[11] = {
  // size motion contr line tilt vary pal             flow spr sat
  { 32, 40, 100, 20, 50, 50, O7P_BW      , 0,  50,   0 },   // Café Wall
  { 40, 25, 100, 45,  0, 50, O7P_ANUSZ   , 3,  90, 100 },   // Scintillating Grid
  { 16, 35, 100,  0,  0, 50, O7P_AGAM    , 3,  90, 100 },   // Ouchi
  { 44, 20, 100, 25,  0, 50, O7P_STIJL   , 3,  90, 100 },   // Hermann Grid
  { 30, 30, 100, 15, 50, 50, O7P_RA      , 3,  90, 100 },   // Zöllner
  { 40, 30, 100,  0,  0, 50, O7P_KITAOKA , 2, 250, 100 },   // Drift
  { 40, 60, 100,  0,  0, 92, O7P_SPECTRUM, 2, 100, 100 },   // Lilac Chaser
  { 40, 20, 100, 15,  0, 50, O7P_STIJL   , 2,  50, 100 },   // Hering
  { 36, 25, 100, 20,  0, 50, O7P_VEGA    , 3,  90, 100 },   // Bulge
  { 10, 35, 100, 50,  0, 50, O7P_KITAOKA , 2, 250, 100 },   // Stepping Feet
  { 60, 10, 100,  0,  0, 50, O7P_SPECTRUM, 2,  90, 100 },   // Fading
};
static const char* const il_names[12] = { "Cafe Wall", "Scintillating Grid", "Ouchi", "Hermann Grid", "Zollner", "Drift", "Lilac Chaser", "Hering", "Bulge", "Stepping Feet", "Fading", "Tour" };
static const char* const il_lbl[6] = { "Size", "Motion", "Contrast", "Line Width", "Tilt", "Variation" };
const char* prog_illusion_name() { return "ILLUSION"; }
const char* prog_illusion_character() { return "Classic optical illusions, slowly moving: cafe wall, scintillating grid, Ouchi, drift"; }
const char* prog_illusion_presetName(int p) { return (p >= 0 && p < 12) ? il_names[p] : NULL; }
const char* prog_illusion_potLabel(int p, int pot) { (void)p; return o7_label(pot, il_lbl); }
uint8_t prog_illusion_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: a pattern row (2 × 640), column tables (320 × 2)
static uint8_t *il_pr, *il_pr2; static int16_t* il_col; static uint8_t* il_cb;
static void il_mem() { il_pr = v6_mem; il_pr2 = v6_mem + 1280; il_col = (int16_t*)(v6_mem + 2560); il_cb = v6_mem + 3200; }
static inline float il_kn(int pot, float own) { return v3_centre(pot) ? own : v3_u(pot); }
static inline float il_kb(int pot, float own) { return v3_centre(pot) ? own : v3_bi(pot); }
static float il_size, il_mot, il_con, il_line, il_tilt, il_vary, il_t = 0, il_t2 = 0;
void prog_illusion_init() { il_mem(); v6_init(); }

#define IL_W O7C(15, O7_GREY)
#define IL_K O7C(0, O7_GREY)
static inline uint8_t il_grey(int s) { return O7C(v3_clamp(s, 0, 15), O7_GREY); }
// a row filled from a repeating pattern buffer starting at `off` (the buffer holds 2 periods + W)
static inline void il_copy(uint8_t* out, const uint8_t* pat, int off) { memcpy(out, pat + off, W); }
// a soft disc of hue h onto the grey background (palette rows set grey → colour); the stronger colour wins
static void il_softDisc(int cx, int cy, int r, int h, int peak) {
  int r2 = r * r; if (r2 < 1) return;
  int32_t inv = (256 << 16) / r2;                                           // (no division per pixel)
  for (int dy = -r; dy <= r; dy++) {
    int y = cy + dy; if ((unsigned)y >= H) continue;
    int w2 = r2 - dy * dy; if (w2 <= 0) continue;
    int w = (int)v3_isqrt((uint32_t)w2);
    int x0 = cx - w < 0 ? 0 : cx - w, x1 = cx + w > W - 1 ? W - 1 : cx + w;
    uint8_t* row = v3_buf + y * W;
    int32_t dy2 = dy * dy;
    for (int x = x0; x <= x1; x++) {
      int dx = x - cx;
      int q = 256 - (int)(((dx * dx + dy2) * inv) >> 16); if (q <= 0) continue;
      int s = (peak * q * q) >> 16;
      if (s > (row[x] >> 4)) row[x] = O7C(s, h);
    }
  }
}
// a dark anti-aliased line on a light ground (ink: shade goes down), ends in 1/8 px
static void il_inkLine(int x0, int y0, int x1, int y1, int lev, int hue) {
  int dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  int n = (adx > ady ? adx : ady) >> 3; if (n < 1) n = 1; if (n > 800) n = 800;
  int32_t sx = (dx * 8192) / n, sy = (dy * 8192) / n, x = x0 * 8192, y = y0 * 8192;
  for (int i = 0; i <= n; i++, x += sx, y += sy) {
    int px = x >> 16, py = y >> 16;
    if ((unsigned)px >= W - 1 || (unsigned)py >= H - 1) continue;
    int fx = (x >> 13) & 7, fy = (y >> 13) & 7;
    uint8_t* p = v3_buf + py * W + px;
    int w[4] = { (8 - fx) * (8 - fy), fx * (8 - fy), (8 - fx) * fy, fx * fy };
    uint8_t* q[4] = { p, p + 1, p + W, p + W + 1 };
    for (int k = 0; k < 4; k++) { int a = (lev * w[k]) >> 6; if (!a) continue; int s = (*q[k] >> 4) - a; *q[k] = O7C(s < 0 ? 0 : s, hue); }
  }
}

void prog_illusion_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  il_mem();
  const IlLook& L = il_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&il_size, v3_centre(5) ? (float)L.size : 6.0f + v3_u(5) * 70.0f, rate);
  v6_ease(&il_mot, il_kn(6, L.motion / 100.0f), rate);
  v6_ease(&il_con, il_kn(7, L.contrast / 100.0f), rate);
  v6_ease(&il_line, il_kn(8, L.line / 100.0f), rate);
  v6_ease(&il_tilt, il_kb(9, (L.tilt - 50) / 50.0f), rate);
  v6_ease(&il_vary, il_kn(10, L.vary / 100.0f), rate);
  o7_liqK = (look == 6 || look == 7 || look == 10) ? 0.0f : 0.4f;         // illusions need their straight lines: little or no liquid
  o7_tint = 1.0f;                                                          // the greys take the palette's colours (B&W on p11 far left)
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float dt = v6_k12 ? 0.0f : v6_f.sdt;                                     // STILL: hold k12
  il_t += dt * (0.1f + il_mot * 2.0f); if (il_t > 10000.0f) il_t -= 10000.0f;
  il_t2 += dt; if (il_t2 > 10000.0f) il_t2 -= 10000.0f;
  float t = il_t;
  float z = v6_zoom(1.0f);
  int S = v3_clamp((int)(il_size * z), 4, 120);                            // the pattern's size
  int hi = 8 + (int)(7 * il_con), lo = 15 - hi;                             // white and black (contrast)
  uint8_t cW = il_grey(hi), cK = il_grey(lo);

  switch (look) {
    case IL_CAFE: {                                                        // rows of bricks, each row slid; grey mortar between
      int bh = S * 5 / 8, m = 1 + (int)(il_line * 5), per = bh + m;
      for (int x = 0; x < 2 * S + W; x++) il_pr[x] = ((x / S) & 1) ? cK : cW;
      uint8_t mort = il_grey(8);
      for (int y = 0; y < H; y++) {
        uint8_t* out = v3_buf + y * W;
        int r = (y + 1000 * per) / per, yy = (y + 1000 * per) % per;
        if (yy >= bh) { memset(out, mort, W); continue; }
        // each row slides by a quarter brick more than the last, rocking back and forth
        float sl = (r & 3) * 0.25f + 0.5f * o7_nsin(t * 0.6f + r * 0.4f) * il_vary;
        int off = ((int)(sl * S) % (2 * S) + 2 * S) % (2 * S);
        il_copy(out, il_pr, off);
      }
    } break;
    case IL_SCINT: case IL_HERMANN: {                                      // a grid of streets; at each crossing a white disc (scintillating)
      bool scint = look == IL_SCINT;
      int lw = 2 + (int)(il_line * S * 0.35f);
      int ox = (int)(t * 6.0f) % S, oy = (int)(t * 4.0f) % S;
      uint8_t street = scint ? il_grey(6 + (int)(2 * il_vary)) : cW, block = scint ? cK : cK;
      for (int x = 0; x < W; x++) { int g = (x + ox) % S; il_col[x] = (int16_t)(g < lw ? (2 * g - lw + 1) : 999); }   // distance to the street's middle (×2)
      int dr = lw * 4 / 5 + 1, discR2 = scint ? dr * dr : -1;              // (in half-pixels, like the distances)
      for (int y = 0; y < H; y++) {
        uint8_t* out = v3_buf + y * W;
        int g = (y + oy) % S;
        if (g >= lw) { for (int x = 0; x < W; x++) out[x] = il_col[x] != 999 ? street : block; continue; }
        int dy2 = (2 * g - lw + 1) * (2 * g - lw + 1);
        for (int x = 0; x < W; x++) {
          int c = il_col[x];
          out[x] = (c != 999 && c * c + dy2 <= discR2) ? cW : street;       // white disc where the streets cross
        }
      }
    } break;
    case IL_OUCHI: {                                                       // long bricks one way, and in a drifting disc the other way
      int bw = S * 2, bh = S / 2 > 2 ? S / 2 : 2;
      int ccx = HALFW + (int)(50 * o7_nsin(t * 0.7f)), ccy = HALFH + (int)(30 * o7_nsin(t * 0.9f)), R = 60 + (int)(25 * il_vary);
      int sx = (int)(t * 3.0f) % (2 * bw);
      for (int x = 0; x < W; x++) { il_cb[x] = (uint8_t)(((x + sx) / bw) & 1); il_col[x] = (int16_t)((x / bh) & 1); }
      for (int y = 0; y < H; y++) {
        uint8_t* out = v3_buf + y * W;
        int rowA = (y / bh) & 1, rowB = (y / bw) & 1, dy = y - ccy, dy2 = dy * dy;
        int w2 = R * R - dy2, xa = 9999, xb = -9999;
        if (w2 > 0) { int w = (int)v3_isqrt((uint32_t)w2); xa = ccx - w; xb = ccx + w; }
        for (int x = 0; x < W; x++) {
          int p = (x >= xa && x <= xb) ? (il_col[x] ^ rowB) : (il_cb[x] ^ rowA);
          out[x] = p ? cK : cW;
        }
      }
    } break;
    case IL_ZOLLNER: {                                                     // long lines crossed by short hatches leaning alternately
      int lw = 1 + (int)(il_line * 4), hh = S * 2 / 5, hp = 6 + S / 6;
      int sh = (int)(t * 8.0f) % hp;
      uint8_t* md = il_pr2;                                                // (v + 256) mod hp < lw → ink
      for (int v = 0; v < 1280; v++) md[v] = (uint8_t)(((v % hp) < lw) ? 1 : 0);
      for (int x = 0; x < W; x++) { int lx = x % S - S / 2; il_col[x] = (int16_t)lx; il_cb[x] = (uint8_t)((x / S) & 1); }
      for (int y = 0; y < H; y++) {
        uint8_t* out = v3_buf + y * W;
        int ya = y + sh + 256 - hp * 4, yb = y - sh + 256 + hp * 4;           // (offsets keep the index positive)
        for (int x = 0; x < W; x++) {
          int lx = il_col[x], alx = lx < 0 ? -lx : lx;
          uint8_t c = cW;
          if (alx <= lw / 2) c = cK;
          else if (alx <= hh && md[il_cb[x] ? lx + ya : yb - lx]) c = cK;   // hatches at ±45°, sliding opposite ways
          out[x] = c;
        }
      }
    } break;
    case IL_DRIFT: {                                                       // Kitaoka's drift: four-step light → dark bands, alternating direction by row
      static const uint8_t seqS[4] = { 0, 4, 15, 10 };                      // black, dark, white, light
      int bw = S / 4 > 1 ? S / 4 : 1, rh = S;
      uint8_t blue = O7C(15, 0), yellow = O7C(15, 1);
      for (int x = 0; x < 2 * S + W; x++) { int k = (x / bw) & 3; il_pr[x] = il_grey(seqS[k]); il_pr2[x] = il_grey(seqS[3 - k]); }
      if (L.sat > 0) for (int x = 0; x < 2 * S + W; x++) {                // colour: blue and yellow for the middle steps
        int k = (x / bw) & 3; if (k == 1) il_pr[x] = blue; if (k == 3) il_pr[x] = yellow;
        int k2 = 3 - k; if (k2 == 1) il_pr2[x] = blue; if (k2 == 3) il_pr2[x] = yellow;
      }
      int off = (int)(t * 2.0f * il_vary) % S;
      for (int y = 0; y < H; y++) {
        int r = y / rh, rr = y % rh;
        uint8_t* out = v3_buf + y * W;
        if (rr < 2) { memset(out, cK, W); continue; }                      // dark lines between the rows
        il_copy(out, (r & 1) ? il_pr2 : il_pr, (off + (r * S / 3)) % S + S);
      }
    } break;
    case IL_LILAC: case IL_FADE: {
      // palette: hues 0-13 run from the grey of the background up to their colour
      int g = look == IL_LILAC ? 190 : 150;
      for (int h = 0; h < 14; h++) {
        uint16_t c = display.getColor((uint8_t)((15 << 4) | h));
        int r = ((c >> 11) & 31) * 255 / 31, gg = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
        for (int s = 0; s < 16; s++) display.setColor((uint8_t)((s << 4) | h), (uint8_t)(g + (r - g) * s / 15), (uint8_t)(g + (gg - g) * s / 15), (uint8_t)(g + (b - g) * s / 15));
      }
      memset(v3_buf, O7C(0, 0), W * H);                                    // grey everywhere (shade 0 of a colour row = the grey)
      if (look == IL_LILAC) {
        int R = (int)(80 * z), r = v3_clamp((int)(S * 0.45f), 6, 40);
        int missing = (int)(il_t2 * (2.0f + il_mot * 10.0f)) % 12;
        int hue = (int)(il_vary * 13.0f);
        for (int k = 0; k < 12; k++) {
          if (k == missing) continue;
          int a = k * 1024 / 12;
          il_softDisc(HALFW + ((v3_cos(a) * R) >> 14), HALFH + ((v3_sin(a) * R) >> 14), r, hue, 12);
        }
      } else {
        for (int k = 0; k < 7; k++) {                                      // big soft blobs drifting very slowly
          float a = k * 0.9f + t * 0.05f;
          int bx = HALFW + (int)(110 * sinf(a * 1.3f + k)), by = HALFH + (int)(75 * cosf(a * 0.9f + k * 2));
          il_softDisc(bx, by, v3_clamp(S, 20, 70), (k * 2) % 14, 13);
        }
      }
      // the black cross to stare at
      for (int k = -6; k <= 6; k++) { v3_put(HALFW + k, HALFH, O7C(0, O7_GREY)); v3_put(HALFW, HALFH + k, O7C(0, O7_GREY)); v3_put(HALFW + k, HALFH + 1, O7C(0, O7_GREY)); v3_put(HALFW + 1, HALFH + k, O7C(0, O7_GREY)); }
    } break;
    case IL_HERING: {                                                      // rays from a point: two straight red lines look bowed
      memset(v3_buf, cW, W * H);
      int cx8 = (int)((HALFW + 30 * o7_nsin(t * 0.4f)) * 8), cy8 = HALFH * 8;
      int n = 18 + (int)(il_vary * 30);
      float spin = t * 0.1f;
      for (int k = 0; k < n; k++) {
        float a = spin + k * 3.14159f / n;
        int dx = (int)(cosf(a) * 400 * 8), dy = (int)(sinf(a) * 400 * 8);
        il_inkLine(cx8 - dx, cy8 - dy, cx8 + dx, cy8 + dy, 10 + (int)(il_line * 5), O7_GREY);
      }
      int sep = (int)(S * 1.6f * (1.0f + 0.15f * o7_nsin(t * 0.5f)));
      int lw = 2 + (int)(il_line * 4);
      for (int s = -1; s <= 1; s += 2) { int x = HALFW + s * sep; v3_rect(x - lw / 2, 0, x + (lw - 1) / 2, H - 1, O7C(15, 0)); }
    } break;
    case IL_BULGE: {                                                       // Kitaoka's bulge: chequers with small squares at the corners
      int d = 2 + (int)(il_line * S * 0.3f);
      int cx = HALFW + (int)(90 * o7_nsin(t * 0.4f)), cy = HALFH + (int)(60 * o7_nsin(t * 0.3f));
      int ox = (S * 1000 - (cx % S)) % S, oy = (S * 1000 - (cy % S)) % S;  // a grid corner sits at the bulge's middle
      // per column: chequer parity (bit 0), near a corner across (bit 1), which side (bit 2: 1 = right edge),
      // and whether the bulge's middle lies that way from that corner (bit 3)
      for (int x = 0; x < W; x++) {
        int gx = x + ox, cxi = gx / S, lx = gx % S, f = cxi & 1;
        int nxc = lx < d ? 0 : (lx >= S - d ? 1 : -1);
        if (nxc >= 0) {
          int Px = (cxi + nxc) * S - ox, sx = nxc == 0 ? 1 : -1, tx = cx > Px ? 1 : -1;
          f |= 2 | (sx == tx ? 8 : 0);
        }
        il_cb[x] = (uint8_t)f;
      }
      for (int y = 0; y < H; y++) {
        uint8_t* out = v3_buf + y * W;
        int gy = y + oy, cyi = gy / S, ly = gy % S, rp = cyi & 1;
        int nyc = ly < d ? 0 : (ly >= S - d ? 1 : -1);
        bool yok = false;
        if (nyc >= 0) { int Py = (cyi + nyc) * S - oy, sy = nyc == 0 ? 1 : -1, ty = cy > Py ? 1 : -1; yok = sy == ty; }
        for (int x = 0; x < W; x++) {
          int f = il_cb[x], par = (f ^ rp) & 1;
          bool dot = yok && (f & 10) == 10;                                 // near a corner both ways, pointing toward the middle
          out[x] = (par ^ dot) ? cK : cW;
        }
      }
    } break;
    default: {                                                             // Stepping feet: two bars glide over stripes and seem to walk
      int sw = v3_clamp(S, 3, 30);
      for (int x = 0; x < W; x++) il_cb[x] = (uint8_t)(((x / sw) & 1) ? cK : cW);
      for (int y = 0; y < H; y++) memcpy(v3_buf + y * W, il_cb, W);
      int bw = 60, bh = 18 + (int)(il_line * 20);
      int px = (int)(t * 25.0f) % (W + bw) - bw;
      uint8_t light = L.sat ? O7C(15, 1) : il_grey(12), dark = L.sat ? O7C(15, 0) : il_grey(3);
      v3_rect(px, 70, px + bw, 70 + bh, light);
      v3_rect(px, 150, px + bw, 150 + bh, dark);
    }
  }
  o7_end();
}

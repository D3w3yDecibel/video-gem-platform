// =====================================================================
// vg5a.h — the Set 5 "arcade" engine (by Dewey)
//
// Twelve classic-style arcade games that play themselves, like a cabinet
// in attract mode — until you touch a knob. Everything they share is here:
//
//   • TAKE-OVER: turn p0 or p1, or press k12, and you're playing ("1UP").
//     Leave the controls alone for 8 seconds and the computer takes back
//     over ("DEMO"). p0/p1 work as a stick, paddle, spinner or thrust,
//     depending on the game.
//   • COMMON KNOBS on every game:
//       p2 Speed (far left = freeze)   p3 Palette (centre = the preset's own)
//       p4 Trails (phosphor persistence; centre = preset)
//       p5 Difficulty (centre = preset)
//       p6 Camera: ◄ CRT scanlines | zoom in, following the action ►
//       p12-p15 the live FX from vg5fx.h (warp, mirror, colour, glitch)
//   • COLOURS: the vg3 "level × hue" layout — a pixel is (level << 4) | hue,
//     16 hues × 16 brightness levels — so trails fade every colour evenly
//     and scanlines are just "one level darker". Eleven palette LOOKS
//     (arcade, green phosphor, amber, pocket handheld, CGA, neon, pastel,
//     inverse, ice, thermal, sunset) re-colour any game.
//   • Pixel-art sprites from strings, a full 5×7 font, a HUD, banners,
//     explosion particles, screen shake, a fast polygon fill and lines.
//
// All the sprites, creatures and names in Set 5 are original: the games
// borrow the classic *gameplay* of the 1980s cabinets, not their characters.
// =====================================================================
#ifndef VG5A_H
#define VG5A_H
#include "vg3.h"
#include "vg5fx.h"

static inline int a5_min(int a, int b) { return a < b ? a : b; }
static inline int a5_max(int a, int b) { return a > b ? a : b; }
#define A5C(l, h) ((uint8_t)(((l) << 4) | (h)))

// ─── Palette looks ────────────────────────────────────────────────────
enum { A5_ARCADE, A5_PHOSPHOR, A5_AMBER, A5_POCKET, A5_CGA, A5_NEON, A5_PASTEL, A5_INVERSE, A5_ICE, A5_THERMAL, A5_SUNSET, A5_NLOOK };
static void a5_ramp(const uint8_t (*st)[3], int n, int v, uint8_t* o) {   // v 0 … 255 along n stops
  int p = v * (n - 1), i = p >> 8, f = p & 255;
  if (i >= n - 1) { i = n - 2; f = 255; }
  for (int c = 0; c < 3; c++) o[c] = v3_mix8(st[i][c], st[i + 1][c], f);
}
static const uint8_t a5_rIce[3][3] = {{0, 4, 24}, {40, 100, 210}, {215, 245, 255}};
static const uint8_t a5_rTherm[6][3] = {{0, 0, 0}, {20, 0, 140}, {170, 0, 160}, {255, 40, 0}, {255, 220, 0}, {255, 255, 255}};
static const uint8_t a5_rSun[4][3] = {{22, 0, 44}, {150, 30, 130}, {255, 105, 80}, {255, 230, 150}};
static const uint8_t a5_rPocket[4][3] = {{15, 56, 15}, {48, 98, 48}, {139, 172, 15}, {155, 188, 15}};
static void a5_filter(int look, uint8_t* c) {
  int r = c[0], g = c[1], b = c[2], l = (r * 77 + g * 150 + b * 29) >> 8;
  switch (look) {
    case A5_PHOSPHOR: { int bl = l > 200 ? (l - 200) * 3 : 0; r = l / 5 + bl; g = l; b = l / 3 + bl; break; }
    case A5_AMBER: r = l; g = l * 160 / 255; b = l / 9; break;
    case A5_POCKET: { int q = l < 40 ? 0 : (l < 110 ? 1 : (l < 180 ? 2 : 3)); r = a5_rPocket[q][0]; g = a5_rPocket[q][1]; b = a5_rPocket[q][2]; break; }
    case A5_CGA: {
      if (l < 36) { r = g = b = 0; break; }
      static const uint8_t cg[3][3] = {{85, 255, 255}, {255, 85, 255}, {255, 255, 255}};
      int best = 0, bd = 1 << 30;
      for (int i = 0; i < 3; i++) { int dr = r - cg[i][0], dg = g - cg[i][1], db = b - cg[i][2]; int d = dr * dr + dg * dg + db * db; if (i == 2) d += 30000; if (d < bd) { bd = d; best = i; } }
      if (l > 225) best = 2;
      r = cg[best][0]; g = cg[best][1]; b = cg[best][2]; break;
    }
    case A5_NEON: r = l + (r - l) * 2; g = l + (g - l) * 2; b = l + (b - l) * 2; r = r * 9 / 8; g = g * 9 / 8; b = b * 9 / 8; break;
    case A5_PASTEL: r = 70 + r * 185 / 255; g = 60 + g * 195 / 255; b = 95 + b * 160 / 255; r = (r * 3 + 255) / 4; g = (g * 3 + 255) / 4; b = (b * 3 + 255) / 4; break;
    case A5_INVERSE: r = 255 - r; g = 255 - g; b = 255 - b; break;
    case A5_ICE: { uint8_t o[3]; a5_ramp(a5_rIce, 3, l, o); r = o[0]; g = o[1]; b = o[2]; break; }
    case A5_THERMAL: { uint8_t o[3]; a5_ramp(a5_rTherm, 6, l, o); r = o[0]; g = o[1]; b = o[2]; break; }
    case A5_SUNSET: { uint8_t o[3]; a5_ramp(a5_rSun, 4, l, o); r = o[0]; g = o[1]; b = o[2]; break; }
    default: break;
  }
  c[0] = (uint8_t)v3_clamp(r, 0, 255); c[1] = (uint8_t)v3_clamp(g, 0, 255); c[2] = (uint8_t)v3_clamp(b, 0, 255);
}
// the 16 hues (fewer repeat), each at 16 levels, through the look. Call every frame (cheap).
static void a5_palette(const uint8_t (*cols)[3], int n, int look) {
  for (int h = 0; h < 16; h++) {
    const uint8_t* bc = cols[h % n];
    for (int l = 0; l < 16; l++) {
      uint8_t c[3] = { (uint8_t)(bc[0] * l / 15), (uint8_t)(bc[1] * l / 15), (uint8_t)(bc[2] * l / 15) };
      if (look) a5_filter(look, c);
      display.setColor((uint8_t)((l << 4) | h), c[0], c[1], c[2]);
    }
  }
}

// ─── Sprites from strings ─────────────────────────────────────────────
// px: w×h characters, row by row. '.' = see-through, '1'-'9' and 'a'-'f' =
// colour 1-15 from cmap. flags: 1 flip left-right, 2 flip up-down,
// 4 turn on its side (draws h wide, w tall), 8 double size.
// solid ≠ 0 draws every pixel in that one colour (hit flashes, shadows).
#define A5_FX 1
#define A5_FY 2
#define A5_ROT 4
#define A5_X2 8
static void a5_spr(const char* px, int w, int h, int x, int y, const uint8_t* cmap, int flags, uint8_t solid) {
  int sc = (flags & A5_X2) ? 2 : 1;
  int dw = (flags & A5_ROT) ? h : w, dh = (flags & A5_ROT) ? w : h;
  for (int dy = 0; dy < dh; dy++) {
    int yy = y + dy * sc; if (yy >= H || yy + sc <= 0) continue;
    for (int dx = 0; dx < dw; dx++) {
      int sx = dx, sy = dy;
      if (flags & A5_ROT) { sx = dy; sy = dx; }
      if (flags & A5_FX) { if (flags & A5_ROT) sy = h - 1 - sy; else sx = w - 1 - sx; }
      if (flags & A5_FY) { if (flags & A5_ROT) sx = w - 1 - sx; else sy = h - 1 - sy; }
      char ch = px[sy * w + sx];
      int k = (ch >= '1' && ch <= '9') ? ch - '0' : ((ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : 0);
      if (!k) continue;
      uint8_t c = solid ? solid : cmap[k];
      int xx = x + dx * sc;
      if (sc == 1) v3_put(xx, yy, c); else v3_rect(xx, yy, xx + 1, yy + 1, c);
    }
  }
}

// ─── Text: a full 5×7 font ────────────────────────────────────────────
static const char a5_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-.!?/x=+><* ";
static const uint8_t a5_font[][7] = {
  {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
  {31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},
  {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
  {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
  {31,1,2,4,8,16,31},
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{31,2,4,2,1,17,14},{2,6,10,18,31,2,2},
  {31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
  {0,12,12,0,12,12,0},{0,0,0,31,0,0,0},{0,0,0,0,0,12,12},{4,4,4,4,4,0,4},{14,17,1,2,4,0,4},{1,1,2,4,8,16,16},
  {0,0,17,10,4,10,17},{0,0,31,0,31,0,0},{0,4,4,31,4,4,0},{8,4,2,1,2,4,8},{2,4,8,16,8,4,2},{0,4,21,14,21,4,0},{0,0,0,0,0,0,0},
};
static int a5_glyph(char c) {
  if (c >= 'a' && c <= 'z' && c != 'x') c = (char)(c - 32);
  for (int i = 0; a5_chars[i]; i++) if (a5_chars[i] == c) return i;
  return (int)sizeof(a5_chars) - 2;                    // space
}
static int a5_textW(const char* s, int sc) { int n = (int)strlen(s); return n ? (n * 6 - 1) * sc : 0; }
static void a5_text(int x, int y, const char* s, int sc, uint8_t c) {
  for (int i = 0; s[i]; i++) {
    const uint8_t* g = a5_font[a5_glyph(s[i])];
    for (int r = 0; r < 7; r++) {
      uint8_t bits = g[r]; if (!bits) continue;
      for (int b = 0; b < 5; b++) if (bits & (16 >> b)) {
        int px = x + (i * 6 + b) * sc, py = y + r * sc;
        if (sc == 1) v3_put(px, py, c); else v3_rect(px, py, px + sc - 1, py + sc - 1, c);
      }
    }
  }
}
// text with a dark outline so it reads over anything
static void a5_textS(int x, int y, const char* s, int sc, uint8_t c) {
  a5_text(x + 1, y + 1, s, sc, 0); a5_text(x - 1, y, s, sc, 0); a5_text(x + 1, y, s, sc, 0); a5_text(x, y - 1, s, sc, 0);
  a5_text(x, y, s, sc, c);
}
static void a5_textC(int y, const char* s, int sc, uint8_t c) { a5_textS((W - a5_textW(s, sc)) / 2, y, s, sc, c); }
static void a5_num(char* o, uint32_t v, int digits) {           // zero-padded number
  o[digits] = 0;
  for (int i = digits - 1; i >= 0; i--) { o[i] = (char)('0' + v % 10); v /= 10; }
}

// ─── Take-over input ──────────────────────────────────────────────────
#define A5_IDLE 8.0f
static bool a5_player = false, a5_refOK = false, a5_k12was = false, a5_fire = false, a5_fireHeld = false, a5_touched = false;
static float a5_idle = 99.0f, a5_ref[2], a5_join = 0;
static float a5_camRef = 512; static bool a5_camArmed = false, a5_camRefOK = false;
static float a5_spRef = 512; static bool a5_spArmed = false, a5_spRefOK = false;
static void a5_reset() { a5_refOK = false; a5_player = false; a5_idle = 99.0f; a5_join = 0; a5_camArmed = false; a5_camRefOK = false; a5_spArmed = false; a5_spRefOK = false; fx5_disarm(); }
static inline float a5_stick(int i);
// p6 (camera) also wakes up only once it's turned
static float a5_cam() {
  if (!a5_camRefOK) { a5_camRef = v3_sm[6]; a5_camRefOK = true; }
  if (!a5_camArmed && fabsf(v3_sm[6] - a5_camRef) > 40.0f) a5_camArmed = true;
  return a5_camArmed ? a5_stick(6) : 0.0f;
}
static void a5_input() {
  a5_touched = false;
  if (!a5_refOK) { a5_ref[0] = v3_sm[0]; a5_ref[1] = v3_sm[1]; a5_refOK = true; }
  for (int i = 0; i < 2; i++) if (fabsf(v3_sm[i] - a5_ref[i]) > 28.0f) { a5_ref[i] = v3_sm[i]; a5_idle = 0; a5_touched = true; }
  bool k = keysPressed[KEY_MOD_A];
  a5_fire = k && !a5_k12was; a5_fireHeld = k; a5_k12was = k;
  if (k) { a5_idle = 0; a5_touched = true; }
  if (a5_touched && !a5_player) { a5_player = true; a5_join = 1.5f; }
  a5_idle += v3_dt; a5_join -= v3_dt;
  if (a5_player && a5_idle > A5_IDLE) a5_player = false;
}
// a knob as a stick: 0 in the centre band, −1 … 1 outside it
static inline float a5_stick(int i) {
  int k = v3_k(i);
  if (k >= 440 && k < 600) return 0.0f;
  return k < 440 ? -(440 - k) / 440.0f : (k - 600) / 423.0f;
}
// a 4-way stick from p0 (left/right) and p1 (left = down, right = up): 0 none, 1 right, 2 down, 3 left, 4 up
static int a5_dir4() {
  float x = a5_stick(0), y = a5_stick(1);
  if (x == 0.0f && y == 0.0f) return 0;
  if (fabsf(x) >= fabsf(y)) return x > 0 ? 1 : 3;
  return y > 0 ? 4 : 2;
}

// ─── Common knobs ─────────────────────────────────────────────────────
static float a5_speedK() {                         // like p6, p2 only takes effect once turned (a knob left far left would freeze the game)
  if (!a5_spRefOK) { a5_spRef = v3_sm[2]; a5_spRefOK = true; }
  if (!a5_spArmed && fabsf(v3_sm[2] - a5_spRef) > 40.0f) a5_spArmed = true;
  return a5_spArmed ? v3_speed(2, 3.0f) : 1.0f;
}
static int a5_lookK(int own) { return v3_centre(3) ? own : v3_clamp(v3_k(3) * A5_NLOOK / 1024, 0, A5_NLOOK - 1); }
static float a5_trailK(float own) { return v3_centre(4) ? own : v3_u(4); }
static float a5_diffK(float own) { return v3_centre(5) ? own : v3_u(5); }   // 0 easy … 1 hard
static const char* const a5_commonLbl[5] = { "Speed", "Palette", "Trails", "Difficulty", "Camera: CRT|Zoom" };
static const char* a5_label(int pot, const char* const* own) {
  if (pot >= 12 && pot < 16) return fx5_labels[pot - 12];
  if (pot >= 2 && pot <= 6) return a5_commonLbl[pot - 2];
  return (pot >= 0 && pot < 16) ? own[pot] : "";
}

// fixed-rate game logic: how many 1/60 s steps to run this frame
static float a5_acc = 0;
static int a5_steps(float speed) {
  a5_acc += v3_dt * speed;
  int n = (int)(a5_acc * 60.0f);
  if (n > 4) { n = 4; a5_acc = 0; } else a5_acc -= n / 60.0f;
  return n;
}
// clear for a new frame, or fade what's there (trails 0 … 1)
static void a5_clear(float trails) {
  if (trails < 0.02f) { memset(v3_buf, 0, W * H); return; }
  v3_fadeLev(v3_clamp(15 - (int)(trails * 14.5f), 1, 15));
}

// ─── Particles ────────────────────────────────────────────────────────
#define A5_NP 192
static int16_t a5_ptx[A5_NP], a5_pty[A5_NP], a5_ptvx[A5_NP], a5_ptvy[A5_NP];   // 1/16 px
static uint8_t a5_ptl[A5_NP], a5_pth[A5_NP];
static int a5_ptNext = 0, a5_ptGrav = 0;
static void a5_partClear() { memset(a5_ptl, 0, sizeof(a5_ptl)); }
static void a5_burst(int x, int y, int n, int hue, int spd, int life) {
  for (int i = 0; i < n; i++) {
    int k = a5_ptNext; a5_ptNext = (a5_ptNext + 1) % A5_NP;
    int a = v3_rnd(1024), s = v3_rndr(spd / 4, spd);
    a5_ptx[k] = (int16_t)(x * 16); a5_pty[k] = (int16_t)(y * 16);
    a5_ptvx[k] = (int16_t)((v3_cos(a) * s) >> 14); a5_ptvy[k] = (int16_t)((v3_sin(a) * s) >> 14);
    a5_ptl[k] = (uint8_t)v3_rndr(life / 2, life); a5_pth[k] = (uint8_t)hue;
  }
}
static void a5_partStep() {
  for (int k = 0; k < A5_NP; k++) if (a5_ptl[k]) {
    a5_ptl[k]--; a5_ptx[k] += a5_ptvx[k]; a5_pty[k] += a5_ptvy[k]; a5_ptvy[k] += a5_ptGrav;
    if (a5_ptx[k] < -320 || a5_ptx[k] > 5440 || a5_pty[k] < -320 || a5_pty[k] > 4160) a5_ptl[k] = 0;
  }
}
static void a5_partDraw() {
  for (int k = 0; k < A5_NP; k++) if (a5_ptl[k]) {
    int lev = a5_ptl[k] / 2 + 4; if (lev > 15) lev = 15;
    uint8_t c = A5C(lev, a5_pth[k]);
    int x = a5_ptx[k] >> 4, y = a5_pty[k] >> 4;
    v3_put(x, y, c); if (a5_ptl[k] > 12) { v3_put(x + 1, y, c); v3_put(x, y + 1, c); }
  }
}

// ─── Lines and filled polygons ────────────────────────────────────────
static int16_t a5_xl[H], a5_xr[H];
// a convex polygon, corners in 1/16 px, n ≤ 8 (one 32-bit divide per edge)
static void a5_fill(const int32_t* xs0, const int32_t* ys0, int n, uint8_t c) {
  if (n < 3 || n > 8) return;
  int32_t xs[8], ys[8], ymin = 1 << 30, ymax = -(1 << 30);
  for (int i = 0; i < n; i++) {
    xs[i] = v3_clamp(xs0[i], -8000, 13000); ys[i] = v3_clamp(ys0[i], -8000, 12000);
    if (ys[i] < ymin) ymin = ys[i]; if (ys[i] > ymax) ymax = ys[i];
  }
  int r0 = (int)((ymin + 7) >> 4), r1 = (int)((ymax - 8) >> 4);
  if (r0 < 0) r0 = 0; if (r1 > H - 1) r1 = H - 1;
  if (r0 > r1) return;
  for (int y = r0; y <= r1; y++) { a5_xl[y] = 32767; a5_xr[y] = -32768; }
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
      if (xl < a5_xl[y]) a5_xl[y] = (int16_t)xl;
      if (xr > a5_xr[y]) a5_xr[y] = (int16_t)xr;
    }
  }
  for (int y = r0; y <= r1; y++) {
    int a = a5_xl[y], b = a5_xr[y];
    if (a < 0) a = 0; if (b > W - 1) b = W - 1;
    if (a <= b) memset(v3_buf + y * W + a, c, b - a + 1);
  }
}
static inline void a5_quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint8_t c) {   // whole pixels
  int32_t xs[4] = { x0 * 16, x1 * 16, x2 * 16, x3 * 16 }, ys[4] = { y0 * 16, y1 * 16, y2 * 16, y3 * 16 };
  a5_fill(xs, ys, 4, c);
}
// a line, clipped to the screen first (no per-pixel checks)
static void a5_seg(int x0, int y0, int x1, int y1, uint8_t c) {
  for (int it = 0; it < 4; it++) {
    int o0 = (x0 < 0) | ((x0 > W - 1) << 1) | ((y0 < 0) << 2) | ((y0 > H - 1) << 3);
    int o1 = (x1 < 0) | ((x1 > W - 1) << 1) | ((y1 < 0) << 2) | ((y1 > H - 1) << 3);
    if (!(o0 | o1)) break;
    if (o0 & o1) return;
    int o = o0 ? o0 : o1, x, y, dx = x1 - x0, dy = y1 - y0;
    if (o & 4)      { y = 0;     x = x0 + (int)((int32_t)dx * (0 - y0) / dy); }
    else if (o & 8) { y = H - 1; x = x0 + (int)((int32_t)dx * (H - 1 - y0) / dy); }
    else if (o & 1) { x = 0;     y = y0 + (int)((int32_t)dy * (0 - x0) / dx); }
    else            { x = W - 1; y = y0 + (int)((int32_t)dy * (W - 1 - x0) / dx); }
    if (o == o0) { x0 = x; y0 = y; } else { x1 = x; y1 = y; }
  }
  if ((unsigned)x0 >= W || (unsigned)y0 >= H || (unsigned)x1 >= W || (unsigned)y1 >= H) return;
  int dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -W : W;
  uint8_t* p = v3_buf + y0 * W + x0;
  if (adx >= ady) { int e = adx >> 1; for (int i = 0; i <= adx; i++) { *p = c; p += sx; e -= ady; if (e < 0) { e += adx; p += sy; } } }
  else            { int e = ady >> 1; for (int i = 0; i <= ady; i++) { *p = c; p += sy; e -= adx; if (e < 0) { e += ady; p += sx; } } }
}
// a glowing vector line: a bright core with a dim halo either side (ends in 1/8 px)
static void a5_glow(int x0, int y0, int x1, int y1, int lev, int hue, bool halo) {
  if (halo) {
    int dx = x1 - x0, dy = y1 - y0;
    int ox = 0, oy = 0;
    if (v3_abs(dx) > v3_abs(dy)) oy = 8; else ox = 8;
    v3_lineAA(x0 + ox, y0 + oy, x1 + ox, y1 + oy, lev / 3, hue);
    v3_lineAA(x0 - ox, y0 - oy, x1 - ox, y1 - oy, lev / 3, hue);
  }
  v3_lineAA(x0, y0, x1, y1, lev, hue);
}
// a ring (whole pixels)
static void a5_circle(int cx, int cy, int r, uint8_t c) {
  int x = r, y = 0, e = 1 - r;
  while (x >= y) {
    v3_put(cx + x, cy + y, c); v3_put(cx - x, cy + y, c); v3_put(cx + x, cy - y, c); v3_put(cx - x, cy - y, c);
    v3_put(cx + y, cy + x, c); v3_put(cx - y, cy + x, c); v3_put(cx + y, cy - x, c); v3_put(cx - y, cy - x, c);
    y++; if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
  }
}

// ─── Camera: screen shake, zoom-follow, CRT scanlines ─────────────────
static float a5_shakeAmt = 0, a5_camX = W / 2, a5_camY = H / 2, a5_zoomNow = 1.0f;
static uint8_t a5_row[W];
static int16_t a5_xs[W];
static inline void a5_shake(float px) { if (px > a5_shakeAmt) a5_shakeAmt = px; }
static void a5_rotRow(uint8_t* r, int dx) {
  dx %= W; if (dx < 0) dx += W;
  if (!dx) return;
  memcpy(a5_row, r + W - dx, dx); memmove(r + dx, r, W - dx); memcpy(r, a5_row, dx);
}
// after the playfield is drawn and before the HUD: shake, and p6's zoom (following fx, fy)
static void a5_post(int fx, int fy) {
  if (a5_shakeAmt > 0.3f) {
    int sx = (int)((v3_rndf() * 2 - 1) * a5_shakeAmt), sy = (int)((v3_rndf() * 2 - 1) * a5_shakeAmt);
    if (sy > 0) { memmove(v3_buf + sy * W, v3_buf, (H - sy) * W); memset(v3_buf, 0, sy * W); }
    else if (sy < 0) { memmove(v3_buf, v3_buf - sy * W, (H + sy) * W); memset(v3_buf + (H + sy) * W, 0, -sy * W); }
    if (sx) for (int y = 0; y < H; y++) a5_rotRow(v3_buf + y * W, sx);
  }
  a5_shakeAmt *= 0.86f; if (a5_shakeAmt < 0.3f) a5_shakeAmt = 0;
  float c = a5_cam(), zt = c > 0 ? 1.0f + c * 1.6f : 1.0f;
  a5_zoomNow += (zt - a5_zoomNow) * v3_clampf(v3_dt * 4.0f, 0.0f, 1.0f);
  a5_camX += (fx - a5_camX) * v3_clampf(v3_dt * 3.0f, 0.0f, 1.0f);
  a5_camY += (fy - a5_camY) * v3_clampf(v3_dt * 3.0f, 0.0f, 1.0f);
  float z = a5_zoomNow;
  if (z < 1.02f) return;
  float vw = W / z, vh = H / z;
  float ox = v3_clampf(a5_camX - vw / 2, 0.0f, W - vw), oy = v3_clampf(a5_camY - vh / 2, 0.0f, H - vh);
  int32_t inv = (int32_t)(65536.0f / z), ox16 = (int32_t)(ox * 65536.0f), oy16 = (int32_t)(oy * 65536.0f);
  for (int x = 0; x < W; x++) a5_xs[x] = (int16_t)v3_clamp((ox16 + x * inv) >> 16, 0, W - 1);
  int pivot = (int)(oy / (1.0f - 1.0f / z)); if (pivot > H) pivot = H; if (pivot < 0) pivot = 0;
  for (int pass = 0; pass < 2; pass++) {
    int y0 = pass ? H - 1 : 0, y1 = pass ? pivot - 1 : pivot, st = pass ? -1 : 1;
    for (int y = y0; y != y1; y += st) {
      int sy = v3_clamp((oy16 + y * inv) >> 16, 0, H - 1);
      memcpy(a5_row, v3_buf + sy * W, W);
      uint8_t* d = v3_buf + y * W;
      for (int x = 0; x < W; x++) d[x] = a5_row[a5_xs[x]];
    }
  }
}
// at the very end: p6 turned left darkens every other row (CRT scanlines)
static void a5_crt() {
  float c = a5_cam();
  if (c >= 0) return;
  int amt = 1 + (int)(-c * 9.0f);
  uint32_t sub = (uint32_t)amt * 0x01010101u;
  for (int y = 1; y < H; y += 2) {
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int i = 0; i < W / 4; i++) {
      uint32_t v = p[i], lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
      uint32_t d = (lev | 0x10101010u) - sub, keep = ((d >> 4) & 0x01010101u) * 0x0Fu;
      p[i] = ((d & keep) << 4) | hue;
    }
  }
}

// ─── HUD and banners ──────────────────────────────────────────────────
// the top line: "1UP 001230" (or a blinking "DEMO") on the left, the high score in the middle
static void a5_hud(uint32_t score, uint32_t hi, uint8_t c1, uint8_t c2) {
  char n[8];
  bool blink = ((int)(globalTime * 2.5f)) & 1;
  if (a5_player) a5_textS(4, 2, "1UP", 1, c1); else if (blink) a5_textS(4, 2, "DEMO", 1, c1);
  a5_num(n, score % 1000000, 6); a5_textS(32, 2, n, 1, c2);
  a5_textS(W / 2 - 14, 2, "HI", 1, c1);
  a5_num(n, hi % 1000000, 6); a5_textS(W / 2 + 2, 2, n, 1, c2);
  if (a5_join > 0 && blink) a5_textC(H / 2 - 30, "PLAYER 1", 2, c1);
}
static void a5_banner(int y, const char* s, int sc, uint8_t c) {
  int w = a5_textW(s, sc), x = (W - w) / 2;
  v3_rect(x - 4 * sc, y - 3 * sc, x + w + 4 * sc, y + 7 * sc + 2 * sc, 0);
  a5_text(x, y, s, sc, c);
}
// a scrolling starfield (n stars, drifting at vx, vy px per second)
static void a5_stars(int n, float vx, float vy, int hue, uint32_t seed) {
  for (int i = 0; i < n; i++) {
    uint32_t h = v3_hash(i * 2654435761u + seed);
    int depth = 1 + (int)(h >> 29);
    float x = (h & 1023) + globalTime * vx * depth / 4.0f, y = ((h >> 10) & 1023) + globalTime * vy * depth / 4.0f;
    int ix = ((int)x % W + W) % W, iy = ((int)y % H + H) % H;
    int lev = 3 + depth * 2 + (((h >> 20) + (uint32_t)(globalTime * 6)) % 7 == 0 ? 4 : 0);
    v3_putMax(ix, iy, lev, hue);
  }
}
#endif

// =====================================================================
// PROGRAM: STEREO — hidden shapes in a Magic Eye (by Dewey) — Set 6, slot 8
//
// The reel that inspired Set 6 looks like a Magic Eye picture — and this
// one really is one. A strip of shimmering hexagonal pattern repeats across
// the screen, and each repeat is nudged just enough that, if you look
// "through" the screen (relax your eyes as if staring at something far
// behind it, until two neighbouring repeats overlap), a hidden shape floats
// up in 3-D: a turning Star of David, a breathing hexagon pyramid, a torus,
// rings of the Flower of Life, words … all moving.
// Can't see it? Hold k12 (or turn up Reveal) to light up the hidden shape.
//
// How it works: an autostereogram. Each pixel copies the colour from one
// pattern-width to its left, minus a little where the hidden shape is near.
//
// Presets: k0 Star of David · k1 Pyramid · k2 Torus · k3 Moon · k4 Flower ·
//          k5 Ripples · k6 Words · k7 Spiral · k8 Merkaba · k9 Floating Hexagons ·
//          k10 Siren's Song · k11 Tour
//   k12 — REVEAL (hold): shade the hidden shape so you can see what to look for
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Depth (◄ for crossed eyes | for relaxed eyes ►) · p6 Pattern Width
//   p7 Shape Size · p8 Motion · p9 Reveal · p10 Texture · p11 Shimmer
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

struct StLook { uint8_t shape, pal, depth, width, size, motion, tex, shim, flow, spread; };
static const StLook st_looks[11] = {
  // shape pal depth width size motion tex shim flow spread
  {  0,  0, 70, 50, 60, 40, 50, 30, 4, 80 },   // Star of David
  {  1,  9, 70, 50, 60, 40, 40, 20, 3, 60 },   // Pyramid
  {  2,  4, 75, 50, 60, 40, 60, 30, 5, 80 },   // Torus
  {  3,  7, 75, 50, 55, 40, 40, 30, 3, 70 },   // Moon
  {  4,  3, 65, 50, 60, 30, 60, 30, 4, 80 },   // Flower
  {  5,  2, 60, 50, 60, 50, 50, 40, 5, 90 },   // Ripples
  {  6, 10, 75, 50, 60, 30, 50, 20, 6, 80 },   // Words
  {  7,  8, 70, 50, 60, 50, 60, 30, 5, 90 },   // Spiral
  {  8,  5, 75, 50, 60, 50, 40, 30, 4, 60 },   // Merkaba
  {  9,  6, 70, 50, 50, 40, 50, 30, 4, 80 },   // Floating Hexagons
  { 10,  0, 70, 50, 60, 40, 70, 40, 5, 70 },   // Siren's Song
};
static const char* const st_names[12] = { "Star of David", "Pyramid", "Torus", "Moon", "Flower", "Ripples", "Words", "Spiral", "Merkaba", "Floating Hexagons", "Siren's Song", "Tour" };
static const char* const st_lbl[7] = { "Depth", "Pattern Width", "Shape Size", "Motion", "Reveal", "Texture", "Shimmer" };
const char* prog_stereo_name() { return "STEREO"; }
const char* prog_stereo_character() { return "A moving Magic Eye: look through the shimmering hex pattern and sacred shapes float up in 3-D"; }
const char* prog_stereo_presetName(int p) { return (p >= 0 && p < 12) ? st_names[p] : NULL; }
const char* prog_stereo_potLabel(int p, int pot) { (void)p; return v6_label(pot, st_lbl); }
uint8_t prog_stereo_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── Memory ───────────────────────────────────────────────────────────
#define ST_DW 160
#define ST_DH 120
#define ST_TP 64                                   // the pattern strip: up to 64 wide …
#define ST_TH 32                                   // … and 32 tall (it repeats down the screen)
static uint8_t* st_d;                              // the hidden shape's height, half resolution (0 = far … 255 = near)
static uint8_t* st_tile;                           // the pattern strip
static int16_t* st_sq;                             // square roots: st_sq[i] = √(i·16), for distances
static void st_mem() { st_d = v6_mem; st_tile = v6_mem + ST_DW * ST_DH; st_sq = (int16_t*)(st_tile + ST_TP * ST_TH); }   // 23.6 KB
#define ST_SQN 1200
static inline int st_sqrt(int32_t v) { if (v < 0) v = 0; int32_t i = v >> 4; return i >= ST_SQN - 1 ? st_sq[ST_SQN - 1] : st_sq[i]; }   // ≈ √v

// ─── The hidden shapes (half-resolution coordinates: centre (80, 60)) ───
static inline int st_clamp8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
// a triangle's bevelled height: inside it rises with the distance to the nearest edge.
// The edge normals (Q14) are worked out once a frame by st_triSet.
static int32_t st_nc[2][3], st_ns[2][3];
static void st_triSet(int t, int ang) { for (int k = 0; k < 3; k++) { int a = ang + k * 341 + 171; st_nc[t][k] = v3_cos(a); st_ns[t][k] = v3_sin(a); } }
static inline int st_tri(int t, int x, int y, int R) {
  int m = 1 << 30;
  for (int k = 0; k < 3; k++) {
    int d = R / 2 - ((x * st_nc[t][k] + y * st_ns[t][k]) >> 14);   // distance inside this edge
    if (d < m) m = d;
  }
  return m;
}
static uint8_t st_word[5][7];                      // the current word, as 5×7 letters (for Words)
static int st_wordN = 0, st_wordI = -1;
static const char* const st_words[6] = { "SIREN", "VIRGO", "LIGHT", "DREAM", "STAR", "HELLO" };
static void st_setWord(int i) {
  const char* w = st_words[i % 6]; st_wordN = (int)strlen(w); if (st_wordN > 5) st_wordN = 5;
  for (int c = 0; c < st_wordN; c++) { const uint8_t* g = v3_font[v3_glyph(w[c])]; for (int r = 0; r < 7; r++) st_word[c][r] = g[r]; }
  st_wordI = i;
}
// fill st_d for shape `sh` at time t (scale: shape size in half-res px).
// The smooth shapes are worked out on alternate rows each frame (ph = 0 or 1) — they move slowly, and it halves the work.
static void st_depth(int sh, float t, float size, int ph) {
  int R = (int)size;
  bool stamp = sh == 3 || sh == 6 || sh == 9;                       // shapes stamped on a cleared map
  if (stamp) { memset(st_d, 0, ST_DW * ST_DH); ph = 0; }
  int ys = stamp ? 1 : 2;                                           // row step
  if (!stamp) for (int y = ph; y < ST_DH; y += 2) memset(st_d + y * ST_DW, 0, ST_DW);
  int ang = (int)(t * 60.0f);
  switch (sh) {
    case 0: case 8: {                                              // Star of David / Merkaba (the two triangles turn apart)
      st_triSet(0, ang); st_triSet(1, sh == 8 ? -ang : ang + 512);
      int bev = R / 5 + 1;
      for (int y = ph; y < ST_DH; y += ys) for (int x = v6_max(0, 80 - R); x < v6_min(ST_DW, 80 + R); x++) {
        int px = x - 80, py = y - 60;
        if (py < -R || py > R) break;
        int d1 = st_tri(0, px, py, R), d2 = st_tri(1, px, py, R);
        int d = d1 > d2 ? d1 : d2;
        if (d > 0) st_d[y * ST_DW + x] = (uint8_t)(160 + v6_min(95, d * 95 / bev));   // bevelled edges, flat top
      }
      break;
    }
    case 1: {                                                      // Pyramid: a hexagon rising and falling
      int hR = (int)(R * (0.85f + 0.15f * sinf(t * 1.3f)));
      int top = (int)(170 + 80 * (0.5f + 0.5f * sinf(t * 0.9f)));
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - 80, py = y - 60, ax = v3_abs(px), ay = v3_abs(py);
        int hd = v6_max(ay, (ax * 887 + ay * 512) >> 10);          // a hexagon's "radius" (flat top)
        if (hd < hR) st_d[y * ST_DW + x] = (uint8_t)(top - (top - 40) * hd / hR);
      }
      break;
    }
    case 2: {                                                      // Torus: a ring tipping back and forth
      float tilt = 0.55f + 0.35f * sinf(t * 0.7f);
      int R1 = R * 7 / 10, R2 = R * 3 / 10;
      int ky = (int)(256.0f / tilt);
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - 80, py = ((y - 60) * ky) >> 8;
        int r = st_sqrt(px * px + py * py), e = r - R1;
        int h2 = R2 * R2 - e * e;
        if (h2 > 0) st_d[y * ST_DW + x] = (uint8_t)st_clamp8(110 + st_sqrt(h2) * 145 / (R2 + 1));
      }
      break;
    }
    case 3: {                                                      // Moon: a sphere drifting, with three little stars
      int cx = 80 + (int)(30 * sinf(t * 0.5f)), cy = 60 + (int)(14 * sinf(t * 0.8f));
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - cx, py = y - cy, h2 = R * R - px * px - py * py;
        if (h2 > 0) st_d[y * ST_DW + x] = (uint8_t)st_clamp8(90 + st_sqrt(h2) * 165 / (R + 1));
      }
      for (int k = 0; k < 3; k++) {
        int sx = 80 + (int)(60 * cosf(t * 0.3f + k * 2.1f)), sy = 60 + (int)(45 * sinf(t * 0.4f + k * 2.1f));
        for (int y = -6; y <= 6; y++) for (int x = -6; x <= 6; x++) {
          if (v3_abs(x) + v3_abs(y) * 2 > 7 && v3_abs(x) * 2 + v3_abs(y) > 7) continue;
          int X = sx + x, Y = sy + y; if ((unsigned)X < ST_DW && (unsigned)Y < ST_DH) st_d[Y * ST_DW + X] = 230;
        }
      }
      break;
    }
    case 4: {                                                      // Flower: the Seed of Life's seven rings, raised
      int r0 = R / 2, w = 3, lim = (r0 * 3 + w) * (r0 * 3 + w);
      int ccx[7], ccy[7];
      for (int c = 0; c < 7; c++) { ccx[c] = 0; ccy[c] = 0; if (c) { int a = (c - 1) * 171 + ang / 2; ccx[c] = (v3_cos(a) * r0) >> 14; ccy[c] = (v3_sin(a) * r0) >> 14; } }
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int qx = x - 80, qy = y - 60;
        if (qx * qx + qy * qy > lim) continue;
        int best = 0;
        for (int c = 0; c < 7; c++) {
          int px = qx - ccx[c], py = qy - ccy[c];
          int e = v3_abs(st_sqrt(px * px + py * py) - r0);
          if (e < w) { int h = 255 - e * 40; if (h > best) best = h; }
        }
        if (best) st_d[y * ST_DW + x] = (uint8_t)best;
        else { int px = x - 80, py = y - 60; if (px * px + py * py < r0 * r0 * 4) st_d[y * ST_DW + x] = 120; }
      }
      break;
    }
    case 5: {                                                      // Ripples: a pond's surface
      int k = 3000 / (R + 10);
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - 80, py = y - 60, r = st_sqrt(px * px + py * py);
        st_d[y * ST_DW + x] = (uint8_t)(128 + (v3_sin(r * k - (int)(t * 300)) >> 7) * (200 - v6_min(r, 150)) / 260);
      }
      break;
    }
    case 6: {                                                      // Words
      int wi = ((int)(t / 4.0f)) % 6;
      if (wi != st_wordI) st_setWord(wi);
      int sc = v3_clamp(R / 10, 2, 6), tw = st_wordN * 6 * sc - sc, x0 = 80 - tw / 2, y0 = 60 - 7 * sc / 2;
      for (int c = 0; c < st_wordN; c++) for (int r = 0; r < 7; r++) for (int b = 0; b < 5; b++) if (st_word[c][r] & (16 >> b)) {
        int X = x0 + (c * 6 + b) * sc, Y = y0 + r * sc;
        for (int yy = 0; yy < sc; yy++) for (int xx = 0; xx < sc; xx++) { int px = X + xx, py = Y + yy; if ((unsigned)px < ST_DW && (unsigned)py < ST_DH) st_d[py * ST_DW + px] = 220; }
      }
      break;
    }
    case 7: {                                                      // Spiral: a winding ramp, turning
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - 80, py = y - 60, r = st_sqrt(px * px + py * py);
        if (r > R * 3 / 2) continue;
        int a = v3_atan(py, px);
        int v = ((a * 3 + r * 1024 / (R / 2 + 1) - ang * 4) & 1023);
        st_d[y * ST_DW + x] = (uint8_t)(60 + (v < 512 ? v : 1023 - v) * 190 / 512);
      }
      break;
    }
    case 9: {                                                      // Floating Hexagons: seven small ones bobbing at different heights
      for (int k = 0; k < 7; k++) {
        float a = k * 0.8976f + t * 0.2f;
        int cx = k ? 80 + (int)(cosf(a) * R * 1.1f) : 80, cy = k ? 60 + (int)(sinf(a) * R * 0.8f) : 60;
        int hR = R / 3 + 2, h = (int)(140 + 100 * sinf(t * 1.1f + k * 1.7f));
        for (int y = -hR; y <= hR; y++) for (int x = -hR; x <= hR; x++) {
          int ax = v3_abs(x), ay = v3_abs(y);
          if (v6_max(ay, (ax * 887 + ay * 512) >> 10) >= hR) continue;
          int X = cx + x, Y = cy + y; if ((unsigned)X < ST_DW && (unsigned)Y < ST_DH && st_d[Y * ST_DW + X] < h) st_d[Y * ST_DW + X] = (uint8_t)h;
        }
      }
      break;
    }
    default: {                                                     // Siren's Song: rings rising from the middle, with a star inside
      st_triSet(0, ang); st_triSet(1, ang + 512);
      for (int y = ph; y < ST_DH; y += ys) for (int x = 0; x < ST_DW; x++) {
        int px = x - 80, py = y - 60, r = st_sqrt(px * px + py * py);
        int v = 120 + (v3_sin(r * 24 - (int)(t * 250)) >> 8) * v6_max(0, 120 - r) / 120;
        int d = v6_max(st_tri(0, px, py, R / 2), st_tri(1, px, py, R / 2));
        if (d > 0) v = 240;
        st_d[y * ST_DW + x] = (uint8_t)st_clamp8(v);
      }
      break;
    }
  }
}

// ─── The pattern strip: a dense, shimmering hexagonal motif with a little noise ───
static void st_makeTile(int P, float t, float tex, float shim) {
  int sp = v6_max(5, P / 5);                                       // hexagon spacing in px
  int noise = (int)(tex * 255);
  uint32_t ts = (uint32_t)(t * 20.0f * shim);
  for (int y = 0; y < ST_TH; y++) for (int x = 0; x < P; x++) {
    // a hexagonal lattice point and the distance to it
    int row = y / ((sp * 222) >> 8 > 0 ? (sp * 222) >> 8 : 1);
    int off = (row & 1) ? sp / 2 : 0;
    int cx = ((x + off) / sp) * sp - off + sp / 2, cy = row * ((sp * 222) >> 8) + ((sp * 222) >> 9);
    int dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
    int ring = (d2 * 16) / (sp * sp + 1);
    uint32_t h = v3_hash((uint32_t)(x * 73 + y * 9301) ^ (ts * 2654435761u));
    int lev = ring < 2 ? 15 : (ring < 4 ? 8 : (ring < 6 ? 12 : 4));
    int hue = (ring + row * 3 + (x * 16) / P) & 15;
    if ((int)(h & 255) < noise / 2) { lev = 6 + (h >> 8) % 10; hue = (h >> 12) & 15; }   // noise: it helps the eyes lock on
    st_tile[y * ST_TP + x] = (uint8_t)((lev << 4) | hue);
  }
}

// ─── The stereogram itself ────────────────────────────────────────────
__attribute__((optimize("O2"))) static void st_rows(int P, int maxSep, int reveal) {
  uint8_t sep[256];
  for (int i = 0; i < 256; i++) sep[i] = (uint8_t)(P - ((i * maxSep) >> 8));   // near = pattern repeats closer together
  for (int y = 0; y < H; y++) {
    uint8_t* row = v3_buf + y * W;
    const uint8_t* d = st_d + (y >> 1) * ST_DW;
    const uint8_t* tl = st_tile + (y & (ST_TH - 1)) * ST_TP;
    for (int x = 0; x < W; x++) {
      int s = sep[d[x >> 1]];
      row[x] = x < s ? tl[x % P] : row[x - s];
    }
    if (reveal) for (int x = 0; x < W; x++) {                      // reveal: light the hidden shape
      int h = d[x >> 1]; int lv = (row[x] >> 4) * (256 - reveal) + ((h >> 4) * reveal);
      row[x] = (uint8_t)(((lv >> 8) << 4) | (row[x] & 15));
    }
  }
}

static float st_depthK, st_width, st_size, st_motion, st_reveal, st_tex, st_shim, st_t = 0, st_rv = 0;
void prog_stereo_init() {
  st_mem(); v6_init(); st_wordI = -1;
  for (int i = 0; i < ST_SQN; i++) st_sq[i] = (int16_t)v3_isqrt((uint32_t)i * 16);
}

void prog_stereo_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  st_mem();
  const StLook& L = st_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  float tDepth = v3_centre(5) ? L.depth / 100.0f : v3_bi(5);
  float tWidth = v3_centre(6) ? L.width / 100.0f : v3_u(6);
  float tSize = v3_centre(7) ? L.size / 100.0f : v3_u(7);
  float tMotion = v3_centre(8) ? L.motion / 100.0f : v3_u(8);
  float tReveal = v3_centre(9) ? 0.0f : v3_u(9);
  float tTex = v3_centre(10) ? L.tex / 100.0f : v3_u(10);
  float tShim = v3_centre(11) ? L.shim / 100.0f : v3_u(11);
  v6_ease(&st_depthK, tDepth, 3.0f); v6_ease(&st_width, tWidth, 3.0f); v6_ease(&st_size, tSize, 3.0f); v6_ease(&st_motion, tMotion, 3.0f);
  v6_ease(&st_tex, tTex, 3.0f); v6_ease(&st_shim, tShim, 3.0f);
  v6_ease(&st_rv, v6_k12 ? 1.0f : tReveal, 6.0f);
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 110);
  st_t += sdt * (0.2f + st_motion * 1.6f);

  int P = v3_clamp(36 + (int)(st_width * 26), 36, ST_TP);          // pattern width in px (about the distance between your eyes, scaled)
  int maxSep = (int)(fabsf(st_depthK) * P * 0.28f);
  static int ph = 0; ph ^= 1;
  st_depth(L.shape, st_t, (12.0f + st_size * 40.0f) * v6_zoom(1.0f), v6_first ? 0 : ph);
  if (v6_first) st_depth(L.shape, st_t, (12.0f + st_size * 40.0f) * v6_zoom(1.0f), 1);
  if (st_depthK < 0) for (int i = 0; i < ST_DW * ST_DH; i++) st_d[i] = (uint8_t)(255 - st_d[i]);   // for crossed eyes: inside out
  st_makeTile(P, v6_t, st_tex, st_shim);
  st_rows(P, maxSep, (int)(st_rv * 200));
  v6_end();
}

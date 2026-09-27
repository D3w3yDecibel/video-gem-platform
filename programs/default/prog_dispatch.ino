// =====================================================================
// PROGRAM: DOT DISPATCH (by Dewey)
// Halftone dot fields in motion: 3D discs made of dots, a dot orb you
// fly through, grids that fold into fans, moiré, a sea and a tunnel of
// dots. Inspired by Max Drekker's "Dispatch" animations
// (https://www.instagram.com/p/DdZuOcZgnMd/). The code is original.
//
// Presets:
//   k0 Twin Discs  — two stacked discs of dots (ember + colour wheel) on a
//                    cream dot grid that dissolves into black
//   k1 Dot Orb     — a white dot sphere inside an RGB-split triangle; the
//                    camera can fly right through it
//   k2 Fold Fan    — a flat dot grid that folds into a mirrored fan
//   k3 Moiré       — two dot grids sliding over each other
//   k4 Dot Sea     — flying low over a rolling sea of dots
//   k5 Tunnel      — rings of dots rushing past, twisting
//   k6 Bumper      — a ball rolls through a dot picture, knocking dots aside
//   k7 Halftone    — a rotating halftone screen over moving plasma
//
//   k12 — DISPATCH: blast every dot outwards (they fall back into place)
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (centre = original colours)   p1 Speed (far left = freeze)
//   p2 Dot Size   p3 Density   p4 (per scene: Ripple/Wobble/Fold/Lens/
//   Waves/Twist/Bump/Plasma)   p5 Spin   p6 Zoom   p7 (per scene:
//   Dissolve/Fly-Thru/Stripes/Offset/Height/Curve/Scatter/Contrast)
//   p8 Colour Shift   p9 Fringe (RGB edges)   p10 Mirror (off/2/4)
//   p11 Background dots
//
// All per-dot maths is whole-number (no floats in the loops), and dots
// are drawn as filled spans, so it stays quick on the RP2040.
// =====================================================================

// ─── Colours ──────────────────────────────────────────────────────────
#define DD_BG      1      // cream
#define DD_BGDOT   2      // darker dot on cream
#define DD_GRID    3      // dim dot on black
#define DD_RED     4
#define DD_GRN     5
#define DD_BLU     6
#define DD_YEL     7
#define DD_WHITE   8
#define DD_TRI     9      // orange outline
#define DD_GREY    16     // 32 greys, dark → white
#define DD_EMB     64     // 64: black → red → orange → cream
#define DD_HUE     128    // 32 hues, full
#define DD_HUED    160    // 32 hues, dark
#define DD_ICE     192    // 56: navy → blue → purple → white → orange → red
#define DD_ICE_N   56

static int dd_scheme = 0;

static void dd_setc(int i, int r, int g, int b) {
  int R = r, G = g, B = b;
  switch (dd_scheme) {
    case 1: R = g; G = b; B = r; break;
    case 2: R = b; G = r; B = g; break;
    case 3: { int l = (r * 3 + g * 6 + b) / 10; R = l; G = l * 92 / 100; B = l * 78 / 100; } break;
    case 4: if (i != 0) { R = 255 - r; G = 255 - g; B = 255 - b; } break;
    default: break;
  }
  display.setColor(i, (uint8_t)R, (uint8_t)G, (uint8_t)B);
}

// piecewise-linear ramp: stops are {index, r, g, b}
static void dd_ramp(int base, const uint8_t (*st)[4], int ns) {
  for (int k = 0; k < ns - 1; k++) {
    int i0 = st[k][0], i1 = st[k + 1][0];
    for (int i = i0; i <= i1; i++) {
      int f = (i1 > i0) ? (i - i0) * 256 / (i1 - i0) : 0;
      dd_setc(base + i, st[k][1] + ((st[k + 1][1] - st[k][1]) * f >> 8),
                        st[k][2] + ((st[k + 1][2] - st[k][2]) * f >> 8),
                        st[k][3] + ((st[k + 1][3] - st[k][3]) * f >> 8));
    }
  }
}

static void dd_hsv(int i, int h, int v) {        // h 0..191 around the wheel, v 0..255
  int seg = h / 32, f = (h % 32) * 8;
  int q = v * (255 - f) / 255, t = v * f / 255;
  int r, g, b;
  switch (seg) {
    case 0: r = v; g = t; b = 0; break;
    case 1: r = q; g = v; b = 0; break;
    case 2: r = 0; g = v; b = t; break;
    case 3: r = 0; g = q; b = v; break;
    case 4: r = t; g = 0; b = v; break;
    default: r = v; g = 0; b = q; break;
  }
  dd_setc(i, r, g, b);
}

static int dd_palKey = -1;
static void dd_buildPalette(int scheme) {
  dd_palKey = scheme;
  dd_scheme = scheme;
  for (int i = 0; i < 256; i++) dd_setc(i, 0, 0, 0);
  dd_setc(DD_BG, 232, 200, 145);
  dd_setc(DD_BGDOT, 150, 110, 70);
  dd_setc(DD_GRID, 40, 40, 48);
  dd_setc(DD_RED, 255, 30, 40);
  dd_setc(DD_GRN, 40, 255, 90);
  dd_setc(DD_BLU, 50, 90, 255);
  dd_setc(DD_YEL, 255, 230, 60);
  dd_setc(DD_WHITE, 255, 255, 255);
  dd_setc(DD_TRI, 255, 150, 40);
  for (int i = 0; i < 32; i++) { int g = 10 + i * 245 / 31; dd_setc(DD_GREY + i, g, g, g + (31 - i) / 3); }
  static const uint8_t emb[][4] = {
    {0, 0, 0, 0}, {10, 60, 0, 10}, {24, 170, 10, 20}, {36, 230, 40, 20},
    {48, 245, 130, 40}, {58, 250, 200, 120}, {63, 245, 228, 180}
  };
  dd_ramp(DD_EMB, emb, 7);
  for (int i = 0; i < 32; i++) { dd_hsv(DD_HUE + i, i * 6, 255); dd_hsv(DD_HUED + i, i * 6, 110); }
  static const uint8_t ice[][4] = {
    {0, 5, 10, 50}, {8, 20, 50, 190}, {18, 110, 50, 150}, {28, 170, 190, 255},
    {34, 245, 245, 255}, {42, 250, 170, 90}, {49, 240, 90, 20}, {55, 200, 30, 10}
  };
  dd_ramp(DD_ICE, ice, 8);
  dd_setc(255, 255, 255, 255);
}

// ─── Maths ────────────────────────────────────────────────────────────
static int16_t dd_sinT[1024];                  // Q14 sine, 1024 steps per turn
static inline int dd_sin(int a) { return dd_sinT[a & 1023]; }
static inline int dd_cos(int a) { return dd_sinT[(a + 256) & 1023]; }

static int dd_isqrt(int v) {
  if (v <= 0) return 0;
  uint32_t op = (uint32_t)v, res = 0, one = 1u << 30;
  while (one > op) one >>= 2;
  while (one) {
    if (op >= res + one) { op -= res + one; res = (res >> 1) + one; }
    else res >>= 1;
    one >>= 2;
  }
  return (int)res;
}

// angle of (x, y), 0..1023 (rough but smooth)
static int dd_atan2(int y, int x) {
  if (x == 0 && y == 0) return 0;
  int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a;
  if (ax >= ay) a = ay * 128 / ax; else a = 256 - ax * 128 / ay;
  if (x < 0) a = 512 - a;
  if (y < 0) a = 1024 - a;
  return a & 1023;
}

static inline uint32_t dd_hash(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + (uint32_t)c * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

// bounce an index back and forth inside 0..n-1 (for shifting non-looping ramps)
static inline int dd_ping(int v, int n) {
  int p = 2 * (n - 1);
  v %= p; if (v < 0) v += p;
  return v < n ? v : p - v;
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* dd_buf;
static uint8_t dd_span[16][16];                // half-widths of small discs
static int dd_fringe = 0;                      // RGB edge offset (px)
static int dd_burst = 0;                       // k12 blast, Q8 (0 = none)

static void dd_disc(int cx, int cy, int r, uint8_t c) {
  if (r <= 0) {
    if ((unsigned)cx < (unsigned)W && (unsigned)cy < (unsigned)H) dd_buf[cy * W + cx] = c;
    return;
  }
  if (cx + r < 0 || cx - r >= W || cy + r < 0 || cy - r >= H) return;
  int y0 = cy - r < 0 ? -cy : -r, y1 = cy + r >= H ? H - 1 - cy : r;
  for (int dy = y0; dy <= y1; dy++) {
    int ady = dy < 0 ? -dy : dy;
    int hw = r < 16 ? dd_span[r][ady] : dd_isqrt(r * r + r - dy * dy);
    int x0 = cx - hw, x1 = cx + hw;
    if (x0 < 0) x0 = 0;
    if (x1 >= W) x1 = W - 1;
    if (x1 >= x0) memset(dd_buf + (cy + dy) * W + x0, c, x1 - x0 + 1);
  }
}

// a dot, with the optional RGB fringe and the k12 blast applied
static void dd_dot(int x, int y, int r, uint8_t c) {
  if (dd_burst) {
    uint32_t h = dd_hash(x, y, 7);
    x += ((x - W / 2) * dd_burst >> 8) + (int)((h & 63) - 32) * dd_burst / 256;
    y += ((y - H / 2) * dd_burst >> 8) + (int)(((h >> 6) & 63) - 32) * dd_burst / 256;
  }
  if (dd_fringe && r >= 1) {
    dd_disc(x - dd_fringe, y, r, DD_RED);
    dd_disc(x + dd_fringe, y, r, DD_BLU);
  }
  dd_disc(x, y, r, c);
}

// a shaded ball (light from the top left), lvl = brightest grey 0..31
static void dd_ball(int cx, int cy, int r, int lvl) {
  if (r < 1) { dd_disc(cx, cy, 0, (uint8_t)(DD_GREY + lvl)); return; }
  if (cx + r < 0 || cx - r >= W || cy + r < 0 || cy - r >= H) return;
  int hx = cx - r / 3, hy = cy - r / 3, rr = r * r, k = 4 * rr;
  for (int dy = -r; dy <= r; dy++) {
    int y = cy + dy;
    if ((unsigned)y >= (unsigned)H) continue;
    int hw = r < 16 ? dd_span[r][dy < 0 ? -dy : dy] : dd_isqrt(rr + r - dy * dy);
    int x0 = cx - hw < 0 ? 0 : cx - hw, x1 = cx + hw >= W ? W - 1 : cx + hw;
    uint8_t* row = dd_buf + y * W;
    int ddy = (y - hy) * (y - hy);
    for (int x = x0; x <= x1; x++) {
      int d2 = (x - hx) * (x - hx) + ddy;
      int s = lvl - d2 * 26 / k;
      row[x] = (uint8_t)(DD_GREY + (s < 1 ? 1 : s));
    }
  }
}

static void dd_line(int x0, int y0, int x1, int y1, uint8_t c, int th) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int n = 0; n < 4000; n++) {
    for (int a = 0; a < th; a++)
      for (int b = 0; b < th; b++)
        if ((unsigned)(x0 + a) < (unsigned)W && (unsigned)(y0 + b) < (unsigned)H) dd_buf[(y0 + b) * W + x0 + a] = c;
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float dd_sm[16];
static bool dd_smReady = false;
static void dd_smooth(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!dd_smReady) dd_sm[i] = pots[i];
    else dd_sm[i] += (pots[i] - dd_sm[i]) * a;
  }
  dd_smReady = true;
}
static inline int dd_k(int i) {                  // knob 0..1023 as an int
  int v = (int)dd_sm[i];
  return v < 0 ? 0 : (v > 1023 ? 1023 : v);
}

// per-frame settings
static int dd_t = 0;          // clock (ticks, ~60 per second at normal speed)
static int dd_sp = 6;         // dot spacing (px)
static int dd_size = 512;     // dot size knob
static int dd_p4 = 512, dd_p5 = 512, dd_p7 = 0;
static int dd_zoom = 256;     // Q8
static int dd_shift = 0;      // colour shift (ramp steps)
static int dd_bgDots = 512;

// dot radius for a given spacing (size knob: tiny … overlapping)
static inline int dd_rad(int sp) { return (sp * (64 + dd_size * 3 / 4)) >> 10; }

// ─── k0 Twin Discs ────────────────────────────────────────────────────
static void dd_sceneDiscs() {
  int t = dd_t, sp = dd_sp;
  // cream dot grid, eaten away into black from the middle outwards
  int diss = dd_p7;
  int cols = W / sp + 1, rows = H / sp + 1;
  int ccx = cols / 2, ccy = rows / 2, maxd = ccx * ccx + ccy * ccy;
  int dotR = dd_bgDots > 40 ? (sp * dd_bgDots >> 12) : -1;
  for (int j = 0; j < rows; j++)
    for (int i = 0; i < cols; i++) {
      int n = (dd_sin(i * 40 + t * 3) + dd_sin(j * 52 - t * 2) + dd_sin(i * 23 + j * 31 + t * 5)) / 3;   // ±16384
      int dx = i - ccx, dy = j - ccy;
      int v = ((n + 16384) >> 6) + ((dx * dx + dy * dy) * 767 / maxd);            // 0 … ~1023
      int x0 = i * sp, y0 = j * sp;
      if (v >= diss) {
        int x1 = x0 + sp > W ? W : x0 + sp;
        for (int y = y0; y < y0 + sp && y < H; y++) memset(dd_buf + y * W + x0, DD_BG, x1 - x0);
        if (dotR >= 0) dd_disc(x0 + sp / 2, y0 + sp / 2, dotR, DD_BGDOT);
      } else if ((dd_hash(i, j, 3) & 7) == 0) {                                    // loose dots drifting
        uint32_t h = dd_hash(i, j, 4);
        int ox = dd_sin(t * 2 + (int)(h & 1023)) * sp >> 13, oy = dd_cos(t * 3 + (int)(h >> 10)) * sp >> 13;
        dd_disc(x0 + sp / 2 + ox, y0 + sp / 2 + oy, dd_rad(sp) / 2 + 1, DD_BG);
      }
    }

  // the discs: one ember gradient on top, one colour wheel below
  int R = H * 33 / 100;
  int F = 500 * dd_zoom >> 8, D = 500;
  int amp = dd_p5 * 150 >> 10;                                   // how far they tilt
  int ax = dd_sin(t * 2) * amp >> 14, ay = dd_sin(t * 3 / 2 + 300) * amp >> 14;
  int sax = dd_sin(ax), cax = dd_cos(ax), say = dd_sin(ay), cay = dd_cos(ay);
  int ra = t * 2;                                                // colours turning
  int cra = dd_cos(ra), sra = dd_sin(ra);
  int warp = dd_p4 * 40 >> 10;
  int g = sp, rb = dd_rad(sp);
  for (int d = 0; d < 2; d++) {
    int cyd = d ? R * 55 / 100 : -R * 55 / 100, czd = d ? -12 : 12;
    for (int v = -R; v <= R; v += g)
      for (int u = -R; u <= R; u += g) {
        int rr2 = u * u + v * v;
        if (rr2 > R * R) continue;
        int w = warp * (dd_sin(u * 6 + t * 4) + dd_sin(v * 5 - t * 3)) >> 15;
        int du = (warp / 3) * dd_sin(v * 9 + t * 5) >> 14;
        int x = u + du, y = v + cyd, z = w + czd;
        int x1 = (x * cay + z * say) >> 14, z1 = (-x * say + z * cay) >> 14;
        int y2 = (y * cax - z1 * sax) >> 14, z2 = (y * sax + z1 * cax) >> 14;
        int zc = D + z2;
        if (zc < 50) continue;
        int sx = W / 2 + x1 * F / zc, sy = H / 2 + y2 * F / zc;
        int r = rb * F / zc;
        uint8_t c;
        if (d == 0) {
          int s = (u * cra + v * sra) >> 14;                                          // −R … R
          c = (uint8_t)(DD_EMB + dd_ping(18 + (s + R) * 42 / (2 * R) + dd_shift, 64));
        } else {
          int hue = ((dd_atan2(v, u) + ra) >> 5) + dd_shift;
          int fr = rr2 * 64 / (R * R);
          c = fr < 3 ? (uint8_t)(DD_EMB + 6) : (uint8_t)((fr < 22 ? DD_HUED : DD_HUE) + (hue & 31));
        }
        dd_dot(sx, sy, r, c);
      }
  }
}

// ─── k1 Dot Orb ───────────────────────────────────────────────────────
#define DD_ORB_N 640
static int16_t dd_orb[DD_ORB_N][3];            // unit sphere points (Q10)
static int16_t dd_ox[DD_ORB_N], dd_oy[DD_ORB_N], dd_or[DD_ORB_N];
static uint8_t dd_ol[DD_ORB_N], dd_ob[DD_ORB_N];

static void dd_sceneOrb() {
  int t = dd_t;
  // faint dot grid behind
  if (dd_bgDots > 40) {
    int s2 = dd_sp * 2;
    for (int y = s2 / 2; y < H; y += s2)
      for (int x = s2 / 2; x < W; x += s2) dd_disc(x, y, dd_bgDots > 700 ? 1 : 0, DD_GRID);
  }
  int F = 420 * dd_zoom >> 8;
  // fly-through: the camera eases in and passes right through the orb
  int Rs = H * 30 / 100;
  int Dc = 480;
  if (dd_p7 > 30) {
    int per = 900 - dd_p7 * 500 / 1023;
    int ph = t % per;                                           // 0 … per
    int e = ph * 1024 / per;                                    // 0 … 1023
    Dc = 480 - (e * e >> 10) * (480 + Rs + 60) / 1024;
  }
  // triangle with RGB-split edges
  {
    int Rt = H * 44 / 100 * dd_zoom >> 8;
    if (Dc < 480) Rt = Rt * 480 / (Dc + Rs > 60 ? Dc + Rs : 60);
    int rt = 768 + (dd_sin(t) * (dd_p5 >> 3) >> 14);
    int px[3], py[3];
    for (int k = 0; k < 3; k++) {
      px[k] = W / 2 + (dd_cos(rt + k * 341) * Rt >> 14);
      py[k] = H / 2 + 8 + (dd_sin(rt + k * 341) * Rt >> 14);
    }
    int o = 1 + dd_fringe + ((dd_sin(t * 5) + 16384) >> 13);
    for (int k = 0; k < 3; k++) {
      int k2 = (k + 1) % 3;
      dd_line(px[k] - o, py[k], px[k2] - o, py[k2], DD_RED, 1);
      dd_line(px[k] + o, py[k], px[k2] + o, py[k2], DD_BLU, 1);
      dd_line(px[k], py[k] + o, px[k2], py[k2] + o, DD_GRN, 1);
      dd_line(px[k], py[k], px[k2], py[k2], DD_TRI, 2);
    }
  }
  // the orb
  int ry = t * (1 + (dd_p5 >> 8)), rx = 90 + (dd_sin(t) >> 9);
  int sry = dd_sin(ry), cry = dd_cos(ry), srx = dd_sin(rx), crx = dd_cos(rx);
  int wob = dd_p4 * 90 >> 10;                                   // 0 … ~90/1024 of the radius
  int dotW = Rs * (5 + dd_size / 40) / 400;                    // world dot radius
  int n = 0;
  int cnt[8] = {0};
  for (int i = 0; i < DD_ORB_N; i++) {
    int ux = dd_orb[i][0], uy = dd_orb[i][1], uz = dd_orb[i][2];
    int rr = Rs * (1024 + (wob * dd_sin(ux * 3 + uy * 2 + t * 4) >> 14)) >> 10;
    int x = ux * rr >> 10, y = uy * rr >> 10, z = uz * rr >> 10;
    int x1 = (x * cry + z * sry) >> 14, z1 = (-x * sry + z * cry) >> 14;
    int y2 = (y * crx - z1 * srx) >> 14, z2 = (y * srx + z1 * crx) >> 14;
    int zc = Dc + z2;
    if (zc < 24) continue;
    dd_ox[n] = (int16_t)(W / 2 + x1 * F / zc);
    dd_oy[n] = (int16_t)(H / 2 + y2 * F / zc);
    int r = dotW * F / zc;
    dd_or[n] = (int16_t)(r > 400 ? 400 : r);
    int lvl = 16 - z2 * 15 / (rr > 0 ? rr : 1);                  // facing us = bright
    dd_ol[n] = (uint8_t)(i % 11 == 0 ? 200 + (i / 11) % 4 : (lvl < 2 ? 2 : lvl > 31 ? 31 : lvl));
    int b = zc * 8 / (Dc + Rs + 1);
    dd_ob[n] = (uint8_t)(b < 0 ? 0 : b > 7 ? 7 : b);
    cnt[dd_ob[n]]++;
    n++;
  }
  for (int b = 7; b >= 0; b--) {                                // far first
    if (!cnt[b]) continue;
    for (int i = 0; i < n; i++) {
      if (dd_ob[i] != b) continue;
      int r = dd_or[i];
      if (dd_ol[i] >= 200) {                                    // coloured sparkle
        static const uint8_t sc[4] = {DD_RED, DD_GRN, DD_BLU, DD_YEL};
        dd_dot(dd_ox[i], dd_oy[i], r / 3, sc[dd_ol[i] - 200]);
      } else if (r >= 4) dd_ball(dd_ox[i], dd_oy[i], r, r > 20 && dd_ol[i] < 18 ? 18 : dd_ol[i]);   // big ones flying past stay bright
      else dd_dot(dd_ox[i], dd_oy[i], r, (uint8_t)(DD_GREY + dd_ol[i]));
    }
  }
}

// ─── k2 Fold Fan ──────────────────────────────────────────────────────
static void dd_sceneFan() {
  int t = dd_t, sp = dd_sp;
  int m = ((16384 - dd_cos(t * 2)) >> 1) * (256 + dd_p4 * 3 / 4) >> 10;    // fold amount, Q14
  if (m > 16384) m = 16384;
  int spread = 170 + (dd_p5 >> 3);                               // fan width
  int apexY = -H / 6;
  int bands = 5 + (dd_p7 >> 7);
  int duty = (dd_p7 * m) >> 15;                                  // black stripes grow as it folds
  int cols = W / sp + 1, rows = H / sp + 1;
  int rb = dd_rad(sp);
  for (int j = 0; j < rows; j++) {
    int vq = j * 1023 / (rows - 1);                              // 0 top … 1023 bottom
    for (int i = 0; i < cols; i++) {
      int uq = (i * 2048 / (cols - 1)) - 1024;                   // −1024 … 1024
      int au = uq < 0 ? -uq : uq;
      // flat grid → fan
      int gx = i * sp + sp / 2, gy = j * sp + sp / 2;
      int th = uq * spread >> 10;
      int rho = (H * (280 + vq * 95 / 100) >> 10) * dd_zoom >> 8;
      int fx = W / 2 + (dd_sin(th) * rho >> 14), fy = apexY + (dd_cos(th) * rho >> 14) * 13 / 10;
      int x = gx + ((fx - gx) * m >> 14), y = gy + ((fy - gy) * m >> 14);
      // radial stripes, mirrored left/right
      int ph = (au * bands + t * 6) & 1023;
      if (ph < duty) continue;
      int ci = dd_ping(vq * (DD_ICE_N - 1) / 1023 + dd_shift, DD_ICE_N);
      int r = rb * (600 + vq / 2) >> 10;
      // orange X-bands low down once it has folded
      if (m > 5000 && vq > 480) {
        int xb = (au - (vq - 480) * 2 + t * 5) & 511;
        if (xb < 70) { ci = dd_ping(49 + dd_shift, DD_ICE_N); r++; }
      }
      dd_dot(x, y, r, (uint8_t)(DD_ICE + ci));
    }
  }
}

// ─── k3 Moiré ─────────────────────────────────────────────────────────
static void dd_sceneMoire() {
  int t = dd_t, sp = dd_sp + 1;
  int rb = dd_rad(sp);
  // grid A: still
  for (int y = sp / 2; y < H; y += sp)
    for (int x = sp / 2; x < W; x += sp)
      dd_dot(x, y, rb, (uint8_t)(DD_ICE + dd_ping(x * 30 / W + 4 + dd_shift, DD_ICE_N)));
  // grid B: turning, breathing and lensed over the top
  int a = (dd_sin(t) * (8 + (dd_p5 >> 4)) >> 14) + 3;
  int ca = dd_cos(a), sa = dd_sin(a);
  int sc = 1024 + (dd_p7 >> 3) + (dd_sin(t * 2) >> 9);           // scale Q10
  sc = sc * dd_zoom >> 8;
  int lens = dd_p4 * 14 >> 10;
  int half = 210 / sp + 2;
  for (int j = -half; j <= half; j++)
    for (int i = -half; i <= half; i++) {
      int x = i * sp * sc >> 10, y = j * sp * sc >> 10;
      int rx = (x * ca - y * sa) >> 14, ry = (x * sa + y * ca) >> 14;
      if (lens) {
        int d = dd_isqrt(rx * rx + ry * ry);
        int push = lens * dd_sin(d * 12 - t * 6) >> 14;
        if (d > 0) { rx += rx * push / d; ry += ry * push / d; }
      }
      int sx = W / 2 + rx, sy = H / 2 + ry;
      if (sx < -8 || sx > W + 8 || sy < -8 || sy > H + 8) continue;
      dd_dot(sx, sy, rb, (uint8_t)(DD_EMB + dd_ping(30 + (j + half) * 30 / (2 * half) + dd_shift, 64)));
    }
}

// ─── k4 Dot Sea ───────────────────────────────────────────────────────
static void dd_sceneSea() {
  int t = dd_t;
  int F = 300 * dd_zoom >> 8, hor = H * 30 / 100, camY = 90;
  int amp = 10 + (dd_p4 * 50 >> 10);
  int lift = dd_p7 * 80 >> 10;                                   // higher camera
  int dz = 24 + dd_sp * 2, NR = 44;
  int move = t * 3;
  int off = move % dz;
  int dx = 12 + dd_sp * 2;
  int roll = dd_sin(t) * (dd_p5 >> 5) >> 14;                     // camera banking
  for (int k = NR - 1; k >= 0; k--) {
    int z = 60 + k * dz - off;
    if (z < 30) continue;
    int zw = (z + move) / 2;
    int halfW = (W / 2 + 20) * z / F;
    int j0 = -(halfW / dx) - 1;
    int rw = (8 + dd_size / 60) * F / z;
    for (int j = j0; j <= -j0; j++) {
      int xw = j * dx;
      int h = amp * (dd_sin(xw * 3 + t * 3) + dd_sin(zw * 2 - t * 4) + dd_sin((xw + zw) * 3 / 2 + t)) / 49152;
      int sx = W / 2 + xw * F / z;
      int sy = hor + (camY + lift - h) * F / z + (roll * (sx - W / 2) >> 8);
      int ci = dd_ping((h + amp) * (DD_ICE_N - 1) / (2 * amp) + dd_shift, DD_ICE_N);
      if (z > dz * NR * 3 / 4 && (dd_hash(j, k, 1) & 1)) continue;   // thin out far off
      dd_dot(sx, sy, rw >> 2, (uint8_t)(DD_ICE + ci));
    }
  }
}

// ─── k5 Tunnel ────────────────────────────────────────────────────────
static void dd_sceneTunnel() {
  int t = dd_t;
  int F = 260 * dd_zoom >> 8;
  int NRg = 30, gap = 40, per = NRg * gap;
  int move = t * (4 + (dd_p5 >> 8));
  int nd = 20 + dd_sp * 3;                                        // dots per ring (density)
  int twist = dd_p4 >> 4;
  int curve = 20 + (dd_p7 >> 3);
  for (int k = NRg - 1; k >= 0; k--) {
    int z = ((k * gap - move) % per + per) % per + 30;
    int ringId = (k * gap - move) >= 0 ? (k * gap - move) / per : ((k * gap - move) - per + 1) / per;
    int ring = k + ringId * NRg;
    int cx = dd_sin(z / 2 + t * 2) * curve >> 12, cy = dd_cos(z / 3 + t) * curve >> 13;
    int rot = z * twist / 16 + t * 2;
    int rw = (6 + dd_size / 70) * F / z;
    uint8_t base = z > per / 2 ? DD_HUED : DD_HUE;
    int hue = (ring * 3 + dd_shift) & 31;
    for (int i = 0; i < nd; i++) {
      int a = i * 1024 / nd + rot;
      int x = cx + (dd_cos(a) * 150 >> 14), y = cy + (dd_sin(a) * 150 >> 14);
      int sx = W / 2 + x * F / z, sy = H / 2 + y * F / z;
      dd_dot(sx, sy, rw / 3, (uint8_t)(base + ((hue + (i & 1) * 2) & 31)));
    }
  }
}

// ─── k6 Bumper ────────────────────────────────────────────────────────
static void dd_sceneBumper() {
  int t = dd_t, sp = dd_sp;
  int bx = W / 2 + (dd_sin(t * 3) * (W * 35 / 100) >> 14);
  int by = H / 2 + (dd_sin(t * 2 + 200) * (H * 30 / 100) >> 14);
  int Rb = (H * (12 + (dd_p4 >> 5)) / 100) * dd_zoom >> 8;
  int Rb2 = Rb * Rb;
  int rb = dd_rad(sp);
  int R = H * 42 / 100, R2 = R * R;
  int scat = dd_p7 * sp * 3 >> 10;
  int ra = t * (1 + (dd_p5 >> 8));
  for (int y = sp / 2; y < H; y += sp)
    for (int x = sp / 2; x < W; x += sp) {
      int u = x - W / 2, v = y - H / 2, rr2 = u * u + v * v;
      uint8_t c;
      if (rr2 < R2) {
        int fr = rr2 * 64 / R2;
        int hue = ((dd_atan2(v, u) + ra) >> 5) + dd_shift;
        c = fr < 4 ? (uint8_t)(DD_EMB + 8) : (uint8_t)((fr < 24 ? DD_HUED : DD_HUE) + (hue & 31));
      } else c = (uint8_t)(DD_EMB + dd_ping(50 + dd_shift, 64));
      int px = x, py = y, r = rb;
      if (scat) {
        uint32_t h = dd_hash(x, y, 9);
        px += dd_sin(t * 2 + (int)(h & 1023)) * scat >> 14;
        py += dd_cos(t * 3 + (int)((h >> 10) & 1023)) * scat >> 14;
      }
      int ddx = px - bx, ddy = py - by, d2 = ddx * ddx + ddy * ddy;
      if (d2 < Rb2) {
        int d = dd_isqrt(d2);
        int push = (Rb2 - d2) / (Rb > 0 ? Rb : 1);               // 0 … Rb
        if (d > 0) { px += ddx * push / d; py += ddy * push / d; }
        r += push * rb / (Rb > 0 ? Rb : 1) + (push > Rb / 2 ? 1 : 0);
      }
      dd_dot(px, py, r, c);
    }
  dd_ball(bx, by, Rb * 55 / 100, 31);
}

// ─── k7 Halftone ──────────────────────────────────────────────────────
static void dd_sceneHalftone() {
  int t = dd_t, sp = dd_sp + 2;
  int a = t * (dd_p5 >> 7) / 4 + 128;                            // screen angle
  int ca = dd_cos(a), sa = dd_sin(a);
  int fq = 2 + (dd_p4 >> 7);
  int mx = W / 2 + (dd_sin(t * 2) * W / 3 >> 14), my = H / 2 + (dd_cos(t * 3) * H / 3 >> 14);
  int con = 256 + dd_p7 * 3 / 2;                                 // contrast Q8
  int maxr = (sp * (400 + dd_size) >> 11) * dd_zoom >> 8;
  int half = 210 / sp + 2;
  for (int j = -half; j <= half; j++)
    for (int i = -half; i <= half; i++) {
      int gx = i * sp, gy = j * sp;
      int x = W / 2 + ((gx * ca - gy * sa) >> 14), y = H / 2 + ((gx * sa + gy * ca) >> 14);
      if (x < -sp || x > W + sp || y < -sp || y > H + sp) continue;
      int d = dd_isqrt((x - mx) * (x - mx) + (y - my) * (y - my));
      int s = dd_sin(x * fq + t * 3) + dd_sin(y * fq * 5 / 4 - t * 2) + dd_sin((x + y) * fq / 2 + t) + dd_sin(d * fq * 2 - t * 4);
      int v = (s + 65536) >> 7;                                   // 0 … 1023
      v = 512 + ((v - 512) * con >> 8);
      if (v < 0) v = 0;
      if (v > 1023) v = 1023;
      int r = v * maxr >> 10;
      if (r <= 0 && v < 200) continue;
      dd_dot(x, y, r, (uint8_t)(DD_EMB + dd_ping(8 + v * 55 / 1023 + dd_shift, 64)));
    }
}

// ─── Mirror (post) ────────────────────────────────────────────────────
static void dd_mirror(int mode) {
  if (mode <= 0) return;
  for (int y = 0; y < H; y++) {
    uint8_t* row = dd_buf + y * W;
    for (int x = 0; x < W / 2; x++) row[W - 1 - x] = row[x];
  }
  if (mode >= 2)
    for (int y = 0; y < H / 2; y++) memcpy(dd_buf + (H - 1 - y) * W, dd_buf + y * W, W);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_dispatch_name() { return "DOT DISPATCH"; }
const char* prog_dispatch_character() { return "Halftone dot fields in 3D: discs, orbs, folds, moire, seas and tunnels of dots"; }

static const char* const dd_presetNames[] = {
  "Twin Discs", "Dot Orb", "Fold Fan", "Moire", "Dot Sea", "Tunnel", "Bumper", "Halftone"
};
#define DD_NUM_PRESETS 8

const char* prog_dispatch_presetName(int preset) {
  if (preset >= 0 && preset < DD_NUM_PRESETS) return dd_presetNames[preset];
  return NULL;
}

static const char* const dd_labels[12] = {
  "Palette", "Speed", "Dot Size", "Density", "", "Spin", "Zoom", "", "Color Shift", "Fringe", "Mirror", "BG Dots"
};
static const char* const dd_p4Labels[DD_NUM_PRESETS] = { "Ripple", "Wobble", "Fold", "Lens", "Waves", "Twist", "Bump", "Plasma" };
static const char* const dd_p7Labels[DD_NUM_PRESETS] = { "Dissolve", "Fly-Thru", "Stripes", "Offset", "Height", "Curve", "Scatter", "Contrast" };

const char* prog_dispatch_potLabel(int preset, int pot) {
  int p = (preset >= 0 && preset < DD_NUM_PRESETS) ? preset : 0;
  if (pot == 4) return dd_p4Labels[p];
  if (pot == 7) return dd_p7Labels[p];
  if (pot >= 0 && pot < 12) return dd_labels[pot];
  return "";
}

uint8_t prog_dispatch_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // every scene paints the whole screen
}

void prog_dispatch_init() {
  dd_palKey = -1;
  static bool ready = false;
  if (ready) return;
  ready = true;
  for (int i = 0; i < 1024; i++) dd_sinT[i] = (int16_t)lroundf(sinf(i * 6.2831853f / 1024.0f) * 16384.0f);
  for (int r = 0; r < 16; r++)
    for (int d = 0; d < 16; d++) {
      float q = (r + 0.5f) * (r + 0.5f) - (float)(d * d);
      dd_span[r][d] = (uint8_t)(q > 0 ? (int)sqrtf(q) : 0);
    }
  for (int i = 0; i < DD_ORB_N; i++) {                          // even points on a sphere
    float y = 1.0f - 2.0f * (i + 0.5f) / DD_ORB_N;
    float rr = sqrtf(1.0f - y * y), a = i * 2.39996323f;
    dd_orb[i][0] = (int16_t)(cosf(a) * rr * 1024.0f);
    dd_orb[i][1] = (int16_t)(y * 1024.0f);
    dd_orb[i][2] = (int16_t)(sinf(a) * rr * 1024.0f);
  }
}

void prog_dispatch_draw(int preset) {
  if (dd_sinT[256] == 0) prog_dispatch_init();                  // in case init wasn't called
  dd_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  dd_smooth(dt);

  static const uint8_t order[5] = {3, 1, 0, 2, 4};              // knob centre = original colours
  int sPos = dd_k(0) * 5 / 1024;
  dd_buildPalette(order[sPos > 4 ? 4 : sPos]);

  float sk = dd_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);
  static float tf = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e7f) tf = 0;
  dd_t = (int)tf;

  dd_size = dd_k(2);
  dd_sp = 10 - dd_k(3) * 7 / 1023;                               // 10 … 3 px
  dd_p4 = dd_k(4);
  dd_p5 = dd_k(5);
  int z = dd_k(6);
  dd_zoom = z < 512 ? 128 + z * 128 / 512 : 256 + (z - 512) * 256 / 511;
  dd_p7 = dd_k(7);
  dd_shift = (dd_k(8) - 512) * 32 / 512;                        // centre = no shift
  dd_fringe = dd_k(9) * 6 / 1023;
  int mir = dd_k(10) * 3 / 1024;
  dd_bgDots = dd_k(11);

  // k12: DISPATCH — every dot flies outwards and settles back
  static bool k12Was = false;
  static int burstF = 0;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) burstF = 256;
  k12Was = k12;
  if (burstF > 0) burstF = burstF * 15 / 16 - 1;
  if (burstF < 0) burstF = 0;
  dd_burst = burstF;

  memset(dd_buf, 0, W * H);
  switch (preset) {
    case 1:  dd_sceneOrb(); break;
    case 2:  dd_sceneFan(); break;
    case 3:  dd_sceneMoire(); break;
    case 4:  dd_sceneSea(); break;
    case 5:  dd_sceneTunnel(); break;
    case 6:  dd_sceneBumper(); break;
    case 7:  dd_sceneHalftone(); break;
    default: dd_sceneDiscs(); break;
  }
  dd_mirror(mir);
}

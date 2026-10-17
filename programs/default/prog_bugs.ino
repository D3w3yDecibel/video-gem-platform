// =====================================================================
// PROGRAM: BUGS — synthetic bugs (by Dewey)
// Wireframe arthropods scuttling across a black floor: insects, spiders,
// centipedes, crabs, scorpions, striders. Every leg is two thin segments
// solved like a real joint (the knee bends outward), each foot is planted
// and stays put while the body walks over it, and the steps ripple along
// the body with a phase delay — tripod gaits, wave gaits, rowing strokes.
// Tiny coloured markers light the knees and feet.
// Inspired by rybinfx's "Xenharmonikon synthetic bugs" (TouchDesigner,
// https://www.instagram.com/reels/DdmGyhRRQGi/). The code is original.
//
// Presets:
//   k0 Walkers     — six-legged insects, tripod gait
//   k1 Spiders     — eight long legs in a rippling wave gait
//   k2 Centipedes  — long bodies that follow their heads, legs in waves
//   k3 Mite Swarm  — a crowd of tiny bugs
//   k4 Striders    — long-legged water striders rowing in pulses
//   k5 Mantis      — slow stick-thin hunters, front legs folded
//   k6 Crabs       — scuttling sideways, claws snapping
//   k7 Scorpions   — claws forward, tail curled over the back
//   k8 Longlegs    — tiny bodies bobbing on very long legs
//   k9 Beetles     — shelled, short-legged, busy
//   k10 Ant Trail  — a column of ants following each other round a loop
//   k11 Menagerie  — one of everything
//
//   k12 — SCATTER: everyone panics and runs, then calms down
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8; centre = Blue Wire)   p1 Speed (far left = freeze)
//   p2 Count   p3 Leg Length   p4 Stride   p5 Wander (straight … twisty)
//   p6 Trails   p7 Joints (markers off … big)   p8 Size   p9 Antennae
//   p10 Body (outline only … filled-in detail)   p11 Glitch (off … heavy)
//   p12 Floor (none / dots / grid)   p13 Shadow (a ghost underneath)
//   p14 Twitch (calm … nervous legs)   p15 Glow (thin … bold lines)
//
// How it works: nothing about the legs is remembered. Each frame, every foot's
// place comes from how far its bug has walked (the gait phase), then a
// two-bone solve finds the knee. Pixels are (brightness << 4) | hue; lines are
// "brightest wins" and a quick fade leaves the trails.
// =====================================================================

#include "vg_sin.h"
#define BG_TWO_PI 6.28318531f
#define BG_PI 3.14159265f
static inline float bg_sin(float r) {             // radians → −1..1 from the shared table
  int a = (int)(r * (1024.0f / BG_TWO_PI));
  return vg_sin1024[a & 1023] * (1.0f / 16384.0f);
}
static inline float bg_cos(float r) { return bg_sin(r + BG_PI * 0.5f); }
static inline int bg_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static uint32_t bg_rng = 12345u;
static inline float bg_rand() { bg_rng = bg_rng * 1664525u + 1013904223u; return (bg_rng >> 8) * (1.0f / 16777216.0f); }
static inline uint32_t bg_hash(uint32_t a, uint32_t b) {
  uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
  h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
  return h;
}

// ─── Colours ──────────────────────────────────────────────────────────
// hues: 0 body lines, 1 legs, 2-5 joint markers, 6 floor, 7 shadow
#define BG_NSCHEME 8
static const uint8_t bg_cols[BG_NSCHEME][8][3] = {
  {{240,245,255},{70,120,255},{255,40,60},{40,255,120},{255,60,220},{40,230,255},{30,40,90},{20,30,70}},     // White Wire
  {{120,230,255},{255,90,200},{255,240,60},{60,255,140},{255,255,255},{255,120,40},{60,20,80},{40,20,60}},   // Neon
  {{255,190,80},{255,120,30},{255,255,220},{90,220,255},{255,60,40},{180,255,90},{70,40,10},{50,30,10}},     // Amber
  {{180,255,190},{40,220,90},{255,255,255},{255,80,80},{80,200,255},{255,230,60},{10,60,25},{8,40,20}},      // Green Phosphor
  {{110,170,255},{40,90,255},{255,40,70},{40,255,130},{255,70,230},{60,240,255},{20,30,80},{15,20,60}},      // Blue Wire (centre)
  {{255,230,230},{255,40,40},{255,200,40},{40,255,255},{255,255,255},{255,120,200},{70,10,10},{50,8,8}},     // Infrared
  {{200,140,255},{120,60,255},{60,255,230},{255,240,80},{255,80,180},{255,255,255},{40,20,80},{30,15,60}},   // Ultraviolet
  {{255,230,150},{230,170,60},{255,70,40},{110,255,160},{160,120,255},{255,255,255},{60,45,15},{45,35,10}},  // Gold
};
static int16_t bg_levK[16];
static void bg_buildPalette(int scheme) {
  const uint8_t (*c)[3] = bg_cols[scheme];
  for (int h = 0; h < 16; h++) {
    int r = 0, g = 0, b = 0;
    if (h < 8) { r = c[h][0]; g = c[h][1]; b = c[h][2]; }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = bg_levK[l];
      int rr = r * k >> 8, gg = g * k >> 8, bb = b * k >> 8;
      if (l >= 14 && h < 2) { rr += (255 - rr) >> 2; gg += (255 - gg) >> 2; bb += (255 - bb) >> 2; }
      display.setColor((l << 4) | h, rr, gg, bb);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* bg_buf;
static int bg_thick = 0;
static inline void bg_putP(uint8_t* p, int hue, int lev) { if ((*p >> 4) <= lev) *p = (uint8_t)((lev << 4) | hue); }
static inline void bg_put(int x, int y, int hue, int lev) {
  if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) bg_putP(bg_buf + y * W + x, hue, lev);
}
static void bg_line(float fx0, float fy0, float fx1, float fy1, int hue, int lev) {
  if (lev <= 0) return;
  int x0 = (int)fx0, y0 = (int)fy0, x1 = (int)fx1, y1 = (int)fy1;
  if ((x0 < 0 && x1 < 0) || (x0 >= W && x1 >= W) || (y0 < 0 && y1 < 0) || (y0 >= H && y1 >= H)) return;
  int dx = abs(x1 - x0), dy = abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
  int tx = dx >= dy ? 0 : 1, ty = 1 - tx, n = 0;
  bool inside = x0 >= 0 && x1 >= 0 && y0 >= 0 && y1 >= 0 && x0 < W - 1 && x1 < W - 1 && y0 < H - 1 && y1 < H - 1;
  for (;;) {
    if (inside) { uint8_t* p = bg_buf + y0 * W + x0; bg_putP(p, hue, lev); if (bg_thick) bg_putP(p + ty * W + tx, hue, lev - 3); }
    else { bg_put(x0, y0, hue, lev); if (bg_thick) bg_put(x0 + tx, y0 + ty, hue, lev - 3); }
    if ((x0 == x1 && y0 == y1) || ++n > 500) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}
static void bg_dot(float fx, float fy, int r, int hue, int lev) {   // a joint marker
  int x = (int)fx, y = (int)fy;
  if (r <= 0) { bg_put(x, y, hue, lev); return; }
  for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) if (i * i + j * j <= r * r + r) bg_put(x + i, y + j, hue, lev);
}
// an outlined ellipse (the body parts), rotated to angle a
static void bg_oval(float cx, float cy, float rx, float ry, float a, int hue, int lev, bool spine) {
  float ca = bg_cos(a), sa = bg_sin(a);
  float px = cx + rx * ca, py = cy + rx * sa;
  const int N = 12;
  for (int i = 1; i <= N; i++) {
    float t = i * (BG_TWO_PI / N);
    float ex = rx * bg_cos(t), ey = ry * bg_sin(t);
    float x = cx + ex * ca - ey * sa, y = cy + ex * sa + ey * ca;
    bg_line(px, py, x, y, hue, lev);
    px = x; py = y;
  }
  if (spine) bg_line(cx - rx * ca * 0.8f, cy - rx * sa * 0.8f, cx + rx * ca * 0.8f, cy + rx * sa * 0.8f, hue, lev - 3);
}
static inline uint32_t bg_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void bg_fade(int dec) {
  uint32_t* b = (uint32_t*)bg_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = bg_fade4(b[i], d4);
}

// ─── Species ──────────────────────────────────────────────────────────
#define BK_INSECT 0
#define BK_SPIDER 1
#define BK_CENTI 2
#define BK_MITE 3
#define BK_STRIDER 4
#define BK_MANTIS 5
#define BK_CRAB 6
#define BK_SCORP 7
#define BK_LONG 8
#define BK_BEETLE 9
#define BK_ANT 10
#define BG_NKIND 11
// per species: leg pairs, base size, speed (px/s at 1×), leg reach, stride, gait (0 tripod, 1 wave, 2 rowing)
static const uint8_t bg_kPairs[BG_NKIND] = {3, 4, 0, 4, 3, 3, 4, 4, 4, 3, 3};
static const uint8_t bg_kSize[BG_NKIND]  = {10, 9, 4, 4, 7, 9, 9, 9, 4, 9, 5};
static const uint8_t bg_kSpeed[BG_NKIND] = {46, 38, 40, 34, 60, 18, 34, 30, 28, 30, 34};
static const uint8_t bg_kReach[BG_NKIND] = {16, 26, 9, 9, 26, 18, 18, 18, 40, 11, 14};  // tenths of size
static const uint8_t bg_kGait[BG_NKIND]  = {0, 1, 1, 1, 2, 0, 1, 1, 1, 0, 0};

// ─── The bugs ─────────────────────────────────────────────────────────
#define BG_MAX 32
static float bg_x[BG_MAX], bg_y[BG_MAX], bg_h[BG_MAX], bg_d[BG_MAX], bg_turn[BG_MAX], bg_spd[BG_MAX];
static uint8_t bg_kind[BG_MAX], bg_scl[BG_MAX];   // species, size (×1/16)
static int bg_n = 0;
// centipede bodies follow their heads: a short trail of head positions (every 2 px)
#define BG_TRAIL 96
#define BG_NCENTI 3
static int16_t bg_tx[BG_NCENTI][BG_TRAIL], bg_ty[BG_NCENTI][BG_TRAIL];   // 1/4 px
static uint8_t bg_tHead[BG_NCENTI];
static float bg_tLast[BG_NCENTI][2];

// shared settings this frame
static float bg_legK = 1.0f, bg_strideK = 1.0f, bg_size = 1.0f, bg_antK = 1.0f, bg_twitch = 0.0f, bg_tsec = 0;
static int bg_joint = 1, bg_lineLev = 11, bg_body = 1;
static float bg_ox = 0, bg_oy = 0;                // drawing offset (shadow pass)
static int bg_shadowPass = 0;

static int bg_L(int lev) { return bg_shadowPass ? (lev > 6 ? 5 : lev - 1) : lev; }
static int bg_H(int hue) { return bg_shadowPass ? 7 : hue; }
static void bg_ln(float x0, float y0, float x1, float y1, int hue, int lev) { bg_line(x0 + bg_ox, y0 + bg_oy, x1 + bg_ox, y1 + bg_oy, bg_H(hue), bg_L(lev)); }
static void bg_mark(float x, float y, int r, int hue) { if (!bg_shadowPass && bg_joint > 0) bg_dot(x + bg_ox, y + bg_oy, r, hue, 15); }

// one leg: hip (hx,hy), foot (fx,fy), bone lengths, bend side; draws thigh + shin + markers
static void bg_leg(float hx, float hy, float fx, float fy, float L1, float L2, float bend, int mk, bool lifted) {
  float dx = fx - hx, dy = fy - hy;
  float D = sqrtf(dx * dx + dy * dy);
  float maxD = (L1 + L2) * 0.98f;
  if (D > maxD) { fx = hx + dx * maxD / D; fy = hy + dy * maxD / D; dx = fx - hx; dy = fy - hy; D = maxD; }
  if (D < 0.5f) D = 0.5f;
  float a = (L1 * L1 - L2 * L2 + D * D) / (2.0f * D);
  float h2 = L1 * L1 - a * a;
  float hh = h2 > 0 ? sqrtf(h2) : 0.0f;
  float ux = dx / D, uy = dy / D;
  float kx = hx + ux * a - uy * hh * bend, ky = hy + uy * a + ux * hh * bend;
  bg_ln(hx, hy, kx, ky, 1, bg_lineLev - 1);
  bg_ln(kx, ky, fx, fy, 1, bg_lineLev - (lifted ? 0 : 2));
  if (bg_joint > 0) {
    bg_mark(kx, ky, bg_joint > 1 ? 1 : 0, 2 + (mk & 3));
    if (!lifted || bg_joint > 2) bg_mark(fx, fy, 0, 2 + ((mk + 1) & 3));
  }
}

// a pair-row of legs at body point (cx,cy) facing a; i = pair index of n; phase from distance d
static void bg_legPair(float cx, float cy, float a, int i, int n, float d, float sz, int kind, uint32_t seed) {
  int gait = bg_kGait[kind];
  float reach = bg_kReach[kind] * 0.13f * sz * bg_legK;
  float L1 = reach * 0.6f, L2 = reach * 0.72f;
  if (kind == BK_LONG) { L1 = reach * 0.6f; L2 = reach * 0.6f; }
  float stride = sz * 1.4f * bg_strideK * (kind == BK_LONG ? 2.0f : 1.0f);
  float ca = bg_cos(a), sa = bg_sin(a);
  float spreadK = n > 1 ? (float)i / (n - 1) : 0.5f;              // 0 front … 1 back
  float fan = (0.55f - spreadK) * 1.5f;                            // front legs reach forward, back legs back
  if (kind == BK_STRIDER) fan = i == 0 ? 0.9f : (i == 1 ? 0.15f : -0.9f);
  if (kind == BK_MANTIS && i == 0) fan = 1.2f;
  for (int s = -1; s <= 1; s += 2) {
    float off;
    if (gait == 0) off = ((i + (s > 0 ? 0 : 1)) & 1) * 0.5f;      // tripod
    else if (gait == 1) off = i * 0.8f / (n > 0 ? n : 1) + (s > 0 ? 0.0f : 0.5f);   // wave
    else off = 0.0f;                                               // rowing: all together
    float ph = d / stride + off;
    ph -= floorf(ph);
    const float duty = 0.62f;
    float A = stride * duty, fxb, lift = 0;
    if (ph < duty) fxb = A * 0.5f - A * (ph / duty);
    else { float u = (ph - duty) / (1.0f - duty); fxb = -A * 0.5f + A * u; lift = bg_sin(BG_PI * u); }
    if (bg_twitch > 0) fxb += (bg_rand() - 0.5f) * bg_twitch * sz;
    float rx = reach * 0.62f, ry = reach * 0.7f;            // planted well inside full reach: the knees bend
    float lx = rx * fan + fxb, ly = s * (ry * (1.0f - 0.25f * lift) + sz * 0.3f);
    if (kind == BK_CRAB) { lx = rx * fan * 0.6f; ly = s * ry - fxb; }       // sideways walker: steps run along its side
    float hipx = cx + sa * s * sz * 0.25f, hipy = cy - ca * s * sz * 0.25f;
    float fxw = cx + lx * ca - ly * sa, fyw = cy + lx * sa + ly * ca;
    bg_leg(hipx, hipy, fxw, fyw, L1, L2, (float)s * (kind == BK_MANTIS && i == 0 ? -1.0f : 1.0f) * -1.0f, (int)(seed + i * 2 + (s > 0)), lift > 0.05f);
  }
}

static void bg_antennae(float x, float y, float a, float len, float wig) {
  for (int s = -1; s <= 1; s += 2) {
    float a1 = a + s * 0.45f + wig * s;
    float mx = x + bg_cos(a1) * len * 0.55f, my = y + bg_sin(a1) * len * 0.55f;
    float a2 = a1 - s * 0.35f + wig * 0.5f;
    float ex = mx + bg_cos(a2) * len * 0.5f, ey = my + bg_sin(a2) * len * 0.5f;
    bg_ln(x, y, mx, my, 0, bg_lineLev - 2);
    bg_ln(mx, my, ex, ey, 0, bg_lineLev - 3);
    bg_mark(ex, ey, 0, 5);
  }
}

static void bg_claw(float x, float y, float a, int s, float sz, float open) {
  float a1 = a + s * 0.7f, L = sz * 1.1f;
  float ex = x + bg_cos(a1) * L, ey = y + bg_sin(a1) * L;
  float a2 = a - s * 0.5f;
  float cx = ex + bg_cos(a2) * L, cy = ey + bg_sin(a2) * L;
  bg_ln(x, y, ex, ey, 1, bg_lineLev);
  bg_ln(ex, ey, cx, cy, 1, bg_lineLev);
  float pa = a2 + open * 0.5f, pb = a2 - open * 0.5f;
  bg_ln(cx, cy, cx + bg_cos(pa) * sz * 0.7f, cy + bg_sin(pa) * sz * 0.7f, 0, bg_lineLev);
  bg_ln(cx, cy, cx + bg_cos(pb) * sz * 0.7f, cy + bg_sin(pb) * sz * 0.7f, 0, bg_lineLev);
  bg_mark(ex, ey, 1, 2);
}

// draw one bug (not centipedes: they have their own)
static void bg_drawBug(int b) {
  int kind = bg_kind[b];
  float sz = bg_kSize[kind] * bg_scl[b] * (2.1f / 16.0f) * bg_size;
  float x = bg_x[b], y = bg_y[b], a = bg_h[b], d = bg_d[b];
  if (kind == BK_CRAB) a += BG_PI * 0.5f;                          // crabs face sideways to their walk
  if (kind == BK_LONG) y += bg_sin(d * 0.25f) * sz * 0.6f;          // bobbing
  float ca = bg_cos(a), sa = bg_sin(a);
  int pairs = bg_kPairs[kind];
  uint32_t seed = (uint32_t)b * 7u;
  int lev = bg_lineLev;
  // body parts along the axis
  float thL = sz * (kind == BK_MANTIS ? 1.8f : 1.0f);
  for (int i = 0; i < pairs; i++) {
    float t = pairs > 1 ? (float)i / (pairs - 1) : 0.5f;
    float off = thL * (0.45f - t * 0.9f);
    bg_legPair(x + ca * off, y + sa * off, a, i, pairs, d + i * 0.0f, sz, kind, seed);
  }
  float wig = bg_sin(bg_tsec * 9.0f + b) * 0.15f;
  switch (kind) {
    case BK_SPIDER:
      bg_oval(x + ca * sz * 0.3f, y + sa * sz * 0.3f, sz * 0.55f, sz * 0.45f, a, 0, lev, false);
      bg_oval(x - ca * sz * 1.1f, y - sa * sz * 1.1f, sz * 0.9f, sz * 0.75f, a, 0, lev, bg_body > 1);
      bg_mark(x + ca * sz * 0.75f, y + sa * sz * 0.75f, 0, 2);
      break;
    case BK_MITE: case BK_LONG:
      bg_oval(x, y, sz * 0.6f, sz * 0.5f, a, 0, lev, false);
      break;
    case BK_STRIDER:
      bg_oval(x, y, sz * 1.0f, sz * 0.25f, a, 0, lev, bg_body > 1);
      break;
    case BK_MANTIS:
      bg_ln(x - ca * sz * 2.2f, y - sa * sz * 2.2f, x + ca * sz * 1.4f, y + sa * sz * 1.4f, 0, lev);
      bg_oval(x - ca * sz * 2.6f, y - sa * sz * 2.6f, sz * 1.1f, sz * 0.35f, a, 0, lev, false);
      bg_oval(x + ca * sz * 1.8f, y + sa * sz * 1.8f, sz * 0.35f, sz * 0.45f, a, 0, lev, false);
      bg_antennae(x + ca * sz * 2.1f, y + sa * sz * 2.1f, a, sz * 2.0f * bg_antK, wig);
      break;
    case BK_CRAB: {
      bg_oval(x, y, sz * 0.75f, sz * 1.15f, a, 0, lev, bg_body > 1);
      float open = 0.4f + 0.4f * bg_sin(bg_tsec * 4.0f + b);
      float fa = a - BG_PI * 0.5f;                                 // claws point the way it faces (sideways)
      bg_claw(x + ca * sz * 0.0f + bg_cos(fa) * sz * 0.0f, y, fa, 1, sz, open);
      bg_claw(x, y, fa, -1, sz, open);
      bg_mark(x + bg_cos(fa) * sz * 0.6f + sa * 0.3f * sz, y + bg_sin(fa) * sz * 0.6f, 0, 4);
      break;
    }
    case BK_SCORP: {
      bg_oval(x, y, sz * 1.0f, sz * 0.55f, a, 0, lev, bg_body > 1);
      float open = 0.3f + 0.3f * bg_sin(bg_tsec * 3.0f + b);
      bg_claw(x + ca * sz * 0.8f, y + sa * sz * 0.8f, a, 1, sz * 0.9f, open);
      bg_claw(x + ca * sz * 0.8f, y + sa * sz * 0.8f, a, -1, sz * 0.9f, open);
      // the tail: segments curling up and over (forward in our top view)
      float px = x - ca * sz * 1.0f, py = y - sa * sz * 1.0f, ta = a + BG_PI;
      float curl = 0.42f + 0.08f * bg_sin(bg_tsec * 2.0f + b);
      for (int k = 0; k < 6; k++) {
        float L = sz * (0.62f - k * 0.04f);
        float nx = px + bg_cos(ta) * L, ny = py + bg_sin(ta) * L;
        bg_ln(px, py, nx, ny, 0, lev);
        bg_mark(nx, ny, 0, 2 + (k & 3));
        px = nx; py = ny; ta += curl;
      }
      bg_dot(px + bg_ox, py + bg_oy, 1, bg_H(2), bg_L(15));
      break;
    }
    case BK_BEETLE:
      bg_oval(x - ca * sz * 0.2f, y - sa * sz * 0.2f, sz * 1.1f, sz * 0.75f, a, 0, lev, true);
      bg_oval(x + ca * sz * 1.05f, y + sa * sz * 1.05f, sz * 0.3f, sz * 0.4f, a, 0, lev, false);
      bg_antennae(x + ca * sz * 1.3f, y + sa * sz * 1.3f, a, sz * 1.2f * bg_antK, wig);
      break;
    default:                                                       // insects and ants: head, thorax, abdomen
      bg_oval(x + ca * sz * 0.95f, y + sa * sz * 0.95f, sz * 0.32f, sz * 0.3f, a, 0, lev, false);
      bg_oval(x, y, sz * 0.5f, sz * 0.3f, a, 0, lev, false);
      bg_oval(x - ca * sz * 1.15f, y - sa * sz * 1.15f, sz * 0.7f, sz * 0.45f, a, 0, lev, bg_body > 1);
      bg_antennae(x + ca * sz * 1.2f, y + sa * sz * 1.2f, a, sz * 1.5f * bg_antK, wig);
      break;
  }
}

// a centipede: segments sampled back along its head's trail
static void bg_drawCenti(int b, int ci, int segs) {
  float sz = bg_kSize[BK_CENTI] * bg_scl[b] * (1.8f / 16.0f) * bg_size;
  float segLen = sz * 1.5f;
  int stepBack = (int)(segLen / 2.0f + 0.5f); if (stepBack < 1) stepBack = 1;   // trail points are 2 px apart
  float px = bg_x[b], py = bg_y[b], pa = bg_h[b];
  int head = bg_tHead[ci];
  float ca = bg_cos(pa), sa = bg_sin(pa);
  bg_antennae(px + ca * sz, py + sa * sz, pa, sz * 3.0f * bg_antK, bg_sin(bg_tsec * 7.0f + b) * 0.2f);
  bg_oval(px, py, sz * 0.8f, sz * 0.6f, pa, 0, bg_lineLev, false);
  for (int j = 1; j <= segs; j++) {
    int idx = (head - j * stepBack) % BG_TRAIL;
    if (idx < 0) idx += BG_TRAIL;
    float x = bg_tx[ci][idx] * 0.25f, y = bg_ty[ci][idx] * 0.25f;
    float dxs = px - x, dys = py - y;
    if (dxs * dxs + dys * dys > 400.0f) break;                    // trail broke (wrapped round the screen)
    float a = atan2f(dys, dxs);
    bg_ln(px, py, x, y, 0, bg_lineLev);
    float w = sz * (j == segs ? 0.4f : 0.7f);
    float c2 = bg_cos(a), s2 = bg_sin(a);
    bg_ln(x - s2 * w, y + c2 * w, x + s2 * w, y - c2 * w, 0, bg_lineLev - 1);   // segment plate
    // legs: one pair per segment, the wave running down the body
    float reach = 9 * 0.1f * sz * 2.2f * bg_legK;
    float L1 = reach * 0.55f, L2 = reach * 0.62f;
    float stride = sz * 2.0f * bg_strideK;
    for (int s = -1; s <= 1; s += 2) {
      float ph = (bg_d[b] - j * segLen * 0.0f) / stride + j * 0.14f + (s > 0 ? 0.0f : 0.5f);
      ph -= floorf(ph);
      const float duty = 0.62f;
      float A = stride * duty, fxb, lift = 0;
      if (ph < duty) fxb = A * 0.5f - A * (ph / duty);
      else { float u = (ph - duty) / (1.0f - duty); fxb = -A * 0.5f + A * u; lift = bg_sin(BG_PI * u); }
      float lx = fxb - reach * 0.15f, ly = s * reach * 0.85f * (1.0f - 0.2f * lift);
      float hx = x - s2 * s * w * 0.8f, hy = y + c2 * s * w * 0.8f;
      bg_leg(hx, hy, x + lx * c2 - ly * s2, y + lx * s2 + ly * c2, L1, L2, (float)-s, (int)(j + (s > 0)), lift > 0.05f);
    }
    px = x; py = y; pa = a;
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float bg_sm[16];
static bool bg_smReady = false;
static inline int bg_k(int i) { int v = (int)bg_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_bugs_name() { return "BUGS"; }
const char* prog_bugs_character() { return "Wireframe synthetic bugs: insects, spiders, centipedes, crabs and scorpions walking with planted feet"; }

static const char* const bg_presetNames[] = {
  "Walkers", "Spiders", "Centipedes", "Mite Swarm", "Striders", "Mantis",
  "Crabs", "Scorpions", "Longlegs", "Beetles", "Ant Trail", "Menagerie"
};
#define BG_NUM_PRESETS 12
const char* prog_bugs_presetName(int preset) {
  if (preset >= 0 && preset < BG_NUM_PRESETS) return bg_presetNames[preset];
  return NULL;
}
static const char* const bg_labels[16] = {
  "Palette", "Speed", "Count", "Leg Length", "Stride", "Wander", "Trails", "Joints",
  "Size", "Antennae", "Body", "Glitch", "Floor", "Shadow", "Twitch", "Glow"
};
const char* prog_bugs_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return bg_labels[pot];
  return "";
}
uint8_t prog_bugs_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we fade the last frame ourselves (trails)
}

static int bg_lastPreset = -1, bg_lastCount = -1;
void prog_bugs_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    bg_levK[0] = 0;
    for (int l = 1; l < 16; l++) bg_levK[l] = (int16_t)(256.0f * (0.14f + 0.86f * powf(l / 15.0f, 0.9f)));
  }
  bg_lastPreset = -1;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

// species for bug i in a preset
static int bg_kindFor(int preset, int i) {
  static const uint8_t mix[11] = {BK_INSECT, BK_SPIDER, BK_CENTI, BK_CRAB, BK_SCORP, BK_BEETLE, BK_LONG, BK_MANTIS, BK_STRIDER, BK_MITE, BK_ANT};
  switch (preset) {
    case 1: return BK_SPIDER;  case 2: return BK_CENTI;  case 3: return BK_MITE;   case 4: return BK_STRIDER;
    case 5: return BK_MANTIS;  case 6: return BK_CRAB;   case 7: return BK_SCORP;  case 8: return BK_LONG;
    case 9: return BK_BEETLE;  case 10: return BK_ANT;   case 11: return mix[i % 11];
    default: return BK_INSECT;
  }
}
static int bg_countFor(int preset, int k) {             // the Count knob around each preset's own number
  static const uint8_t base[BG_NUM_PRESETS] = {3, 2, 2, 18, 4, 2, 4, 2, 3, 5, 9, 11};
  float m = powf(2.0f, (k - 512) / 400.0f);
  int n = (int)(base[preset] * m + 0.5f);
  if (preset == 2 && n > BG_NCENTI) n = BG_NCENTI;
  return bg_clamp(n, 1, BG_MAX);
}

void prog_bugs_draw(int preset) {
  bg_buf = display.getBuffer();
  if (!bg_buf) return;
  if (preset < 0 || preset >= BG_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!bg_smReady) bg_sm[i] = pots[i];
      else bg_sm[i] += (pots[i] - bg_sm[i]) * a;
    }
    bg_smReady = true;
  }

  // ── knobs ──
  float sk = bg_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.6×
  int count = bg_countFor(preset, bg_k(2));
  bg_legK = 0.55f + bg_k(3) * 0.9f / 1023.0f;
  bg_strideK = 0.4f + bg_k(4) * 1.2f / 1023.0f;
  float wander = bg_k(5) / 1023.0f;
  int trails = bg_k(6);
  int jk = bg_k(7);
  bg_joint = jk < 60 ? 0 : (jk < 640 ? 1 : (jk < 900 ? 2 : 3));
  bg_size = powf(2.0f, (bg_k(8) - 512) / 512.0f);
  bg_antK = 0.3f + bg_k(9) * 1.4f / 1023.0f;
  bg_body = bg_k(10) < 512 ? 1 : 2;
  int glitch = bg_k(11) < 60 ? 0 : (bg_k(11) - 60) * 16 / 963;
  int floorMode = bg_k(12) < 300 ? 0 : (bg_k(12) < 720 ? 1 : 2);
  float shadow = bg_k(13) / 1023.0f;
  bg_twitch = bg_k(14) < 100 ? 0.0f : (bg_k(14) - 100) / 923.0f * 0.8f;
  int gk = bg_k(15);
  bg_lineLev = 8 + gk * 6 / 1024;
  bg_thick = gk > 800 ? 1 : 0;
  int sc = bg_k(0) * BG_NSCHEME / 1024; if (sc > BG_NSCHEME - 1) sc = BG_NSCHEME - 1;
  bg_buildPalette(sc);

  // ── SCATTER ──
  static bool k12Was = false;
  static float kickT = 9.0f;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) kickT = 0;
  k12Was = k12;
  kickT += dt;
  float panic = kickT < 0.1f ? kickT / 0.1f : expf(-(kickT - 0.1f) * 0.9f);

  // ── (re)populate ──
  if (preset != bg_lastPreset || count != bg_lastCount) {
    if (preset != bg_lastPreset) memset(bg_buf, 0, W * H);
    bg_rng = 777u + (uint32_t)preset * 31u;
    for (int i = 0; i < count; i++) {
      if (i >= bg_n || preset != bg_lastPreset) {
        bg_x[i] = 20 + bg_rand() * (W - 40); bg_y[i] = 20 + bg_rand() * (H - 40);
        bg_h[i] = bg_rand() * BG_TWO_PI; bg_d[i] = bg_rand() * 50.0f; bg_turn[i] = 0;
        bg_kind[i] = (uint8_t)bg_kindFor(preset, i);
        bg_scl[i] = (uint8_t)(preset == 3 ? 10 + (int)(bg_rand() * 10) : (preset == 11 ? 14 : 13 + (int)(bg_rand() * 7)));
        bg_spd[i] = 0.8f + bg_rand() * 0.4f;
      }
    }
    for (int c = 0; c < BG_NCENTI; c++) {                         // fresh trails
      bg_tHead[c] = 0;
      for (int k = 0; k < BG_TRAIL; k++) { bg_tx[c][k] = (int16_t)(bg_x[c] * 4); bg_ty[c][k] = (int16_t)(bg_y[c] * 4); }
      bg_tLast[c][0] = bg_x[c]; bg_tLast[c][1] = bg_y[c];
    }
    bg_n = count; bg_lastPreset = preset; bg_lastCount = count;
  }
  bg_tsec += dt * (speed > 0 ? 1.0f : 0.0f);
  if (bg_tsec > 1000.0f) bg_tsec -= 1000.0f;

  // ── move ──
  static float trailT = 0;
  trailT += dt * speed * 0.9f * (1.0f + 2.0f * panic);
  if (trailT > 10000.0f) trailT -= 10000.0f;
  for (int i = 0; i < bg_n; i++) {
    int kind = bg_kind[i];
    float v = bg_kSpeed[kind] * bg_spd[i] * speed * (1.0f + 2.5f * panic) * (0.6f + 0.4f * bg_strideK);
    if (kind == BK_STRIDER) v *= 0.6f + 0.8f * fmaxf(0.0f, bg_sin(bg_d[i] / (bg_kSize[kind] * 1.4f * bg_strideK) * BG_TWO_PI));   // rowing pulses
    if (preset == 10) {                                            // Ant Trail: a column round a loop
      float s = trailT * 0.07f - i * 0.24f;
      float nx = HALFW + 120.0f * bg_size * bg_sin(s), ny = HALFH + 70.0f * bg_size * bg_sin(2.0f * s);
      float ddx = nx - bg_x[i], ddy = ny - bg_y[i];
      float step = sqrtf(ddx * ddx + ddy * ddy);
      if (step > 0.01f && step < 40.0f) { bg_h[i] = atan2f(ddy, ddx); bg_d[i] += step; }
      bg_x[i] = nx; bg_y[i] = ny;
      continue;
    }
    float jitter = (bg_rand() - 0.5f) * (0.5f + wander * 6.0f);
    bg_turn[i] += jitter * dt * 6.0f;
    bg_turn[i] *= 1.0f - dt * 1.5f;
    float maxT = 0.3f + wander * 3.0f;
    if (bg_turn[i] > maxT) bg_turn[i] = maxT; else if (bg_turn[i] < -maxT) bg_turn[i] = -maxT;
    if (panic > 0.05f) {                                           // run away from the centre
      float ax = atan2f(bg_y[i] - HALFH, bg_x[i] - HALFW);
      float diff = ax - bg_h[i];
      while (diff > BG_PI) diff -= BG_TWO_PI;
      while (diff < -BG_PI) diff += BG_TWO_PI;
      bg_turn[i] += diff * panic * dt * 8.0f;
    }
    bg_h[i] += bg_turn[i] * dt * (speed > 0 ? 1.0f : 0.0f);
    float mv = v * dt;
    float dirA = bg_h[i];
    bg_x[i] += bg_cos(dirA) * mv; bg_y[i] += bg_sin(dirA) * mv;
    bg_d[i] += mv;
    if (bg_d[i] > 100000.0f) bg_d[i] -= 99840.0f;
    const float m = 30.0f;                                          // wrap round the screen edges
    if (bg_x[i] < -m) bg_x[i] += W + 2 * m; else if (bg_x[i] > W + m) bg_x[i] -= W + 2 * m;
    if (bg_y[i] < -m) bg_y[i] += H + 2 * m; else if (bg_y[i] > H + m) bg_y[i] -= H + 2 * m;
  }
  // centipede trails: a point every 2 px
  for (int i = 0; i < bg_n && i < BG_NCENTI; i++) {
    if (bg_kind[i] != BK_CENTI) continue;
    float dx = bg_x[i] - bg_tLast[i][0], dy = bg_y[i] - bg_tLast[i][1];
    float dd = dx * dx + dy * dy;
    if (dd > 900.0f) {                                             // wrapped: restart the trail here
      for (int k = 0; k < BG_TRAIL; k++) { bg_tx[i][k] = (int16_t)(bg_x[i] * 4); bg_ty[i][k] = (int16_t)(bg_y[i] * 4); }
      bg_tLast[i][0] = bg_x[i]; bg_tLast[i][1] = bg_y[i];
      continue;
    }
    while (dd >= 4.0f) {
      float l = sqrtf(dd);
      bg_tLast[i][0] += dx / l * 2.0f; bg_tLast[i][1] += dy / l * 2.0f;
      bg_tHead[i] = (uint8_t)((bg_tHead[i] + 1) % BG_TRAIL);
      bg_tx[i][bg_tHead[i]] = (int16_t)(bg_tLast[i][0] * 4); bg_ty[i][bg_tHead[i]] = (int16_t)(bg_tLast[i][1] * 4);
      dx = bg_x[i] - bg_tLast[i][0]; dy = bg_y[i] - bg_tLast[i][1]; dd = dx * dx + dy * dy;
    }
  }

  // ── trails: fade (real-time rate) ──
  {
    static float fadeAcc = 0;
    float tr = 1.0f - trails / 1023.0f;
    fadeAcc += dt * (4.0f + tr * tr * 600.0f);
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) bg_fade(dec);
  }

  // ── floor ──
  if (floorMode) {
    int lev = 3;
    uint8_t c = (uint8_t)((lev << 4) | 6);
    int stepF = floorMode == 1 ? 16 : 20;
    for (int y = 8; y < H; y += stepF)
      for (int x = 8; x < W; x += (floorMode == 1 ? stepF : 2)) { uint8_t* p = bg_buf + y * W + x; if ((*p >> 4) < lev) *p = c; }
    if (floorMode == 2)
      for (int x = 8; x < W; x += stepF) for (int y = 8; y < H; y += 2) { uint8_t* p = bg_buf + y * W + x; if ((*p >> 4) < lev) *p = c; }
  }

  // ── draw: a shadow pass (offset, dim) then the bugs ──
  int segs = 8 + bg_k(10) * 14 / 1023;
  for (int pass = shadow > 0.05f ? 0 : 1; pass < 2; pass++) {
    bg_shadowPass = pass == 0;
    bg_ox = pass == 0 ? 3.0f + shadow * 6.0f : 0.0f;
    bg_oy = pass == 0 ? 4.0f + shadow * 8.0f : 0.0f;
    for (int i = 0; i < bg_n; i++) {
      if (bg_kind[i] == BK_CENTI && i < BG_NCENTI) bg_drawCenti(i, i, segs);
      else bg_drawBug(i);
    }
  }
  bg_shadowPass = 0; bg_ox = bg_oy = 0;

  // ── glitch: shifted slices ──
  if (glitch > 0 && speed > 0) {
    int nSl = 1 + glitch / 4;
    for (int k = 0; k < nSl; k++) {
      if (bg_rand() > 0.15f + glitch * 0.04f) continue;
      int y0 = (int)(bg_rand() * H), hgt = 1 + (int)(bg_rand() * (2 + glitch));
      int sh = (int)((bg_rand() - 0.5f) * glitch * 4);
      for (int y = y0; y < y0 + hgt && y < H; y++) {
        uint8_t* row = bg_buf + y * W;
        if (sh > 0) memmove(row + sh, row, W - sh);
        else if (sh < 0) memmove(row, row - sh, W + sh);
      }
    }
  }
}

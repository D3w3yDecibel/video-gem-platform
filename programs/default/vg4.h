// =====================================================================
// vg4.h — the Set 4 landscape engine (by Dewey)
//
// All twelve Set 4 programs are flights over a landscape, each in the style
// of a decade (1920s … 2030s). This file is the shared part: HORIZON's
// voxel landscape from Set 3, grown with
//   • more kinds of ground (city blocks, pyramids, mesas, rolling hills,
//     flat floors, low-poly facets, terraces, dunes …)
//   • ground styles (grid lines, contour lines, inked edges, a point cloud,
//     lit dance-floor tiles)
//   • ways to EXPLODE the horizon, which each decade mixes its own way:
//       shards  — the land breaks into vertical shards that fly apart
//       slabs   — the land splits into layers by distance that lift apart
//       ring    — a shockwave ring rolls out through the ground
//       erupt   — the ground ahead heaves up into a giant mound
//       twist   — the land spirals round, like a cyclone
//       fold    — the far land curls up into the sky
//       scatter — the land dissolves into floating points
//   • particles (shards, sparks, pixels, petals …) and sky props (sun,
//     sunburst, searchlights, clouds, moons, stars, lasers, lens flares …)
//   • after-effects (scanlines, film grain, glitch, mirrors)
//
// Palette layout used by every Set 4 program:
//   0-39 sky gradient · 40-47 props (each program's own) · 48-63 accent ramp (sun, glow, props) ·
//   64 + band × 16 + fog × 4 + light: the ground (12 bands; band 0 = water,
//   band 11 = the line colour for grids, contours and edges)
// =====================================================================
#ifndef VG4_H
#define VG4_H
#include "vg3.h"
#include "hz_map.h"

#define H4_ACC 48
#define H4_PROP 40                                 // 40-47: eight colours each program sets for its own props (h4_prop)
static inline uint8_t h4_pix(int band, int fog, int light) { return (uint8_t)(64 + band * 16 + fog * 4 + light); }

// ─── Ground kinds, styles, explosion recipes ──────────────────────────
enum { T4_MAP, T4_TERRACE, T4_DUNES, T4_INVERT, T4_ROLLING, T4_CITY, T4_FLAT, T4_MESA, T4_PYRAMID, T4_FACETS, T4_SPIKES };
#define S4_GRID    1      // grid lines on the ground
#define S4_WIRE    2      // only the grid lines (everything else dark)
#define S4_CONTOUR 4      // contour lines every 16 height steps
#define S4_DOTS    8      // a point cloud: the top of each slice only
#define S4_EDGES   16     // an inked line along every ridge
#define S4_TILES   32     // a dance floor: tiles that light up on the beat
#define S4_ROAD    64     // a winding road (band 11)
#define S4_FLOWERS 128    // little dots of the line colour scattered on the ground (flowers)
#define X4_SHARDS  1
#define X4_SLABS   2
#define X4_RING    4
#define X4_ERUPT   8
#define X4_TWIST   16
#define X4_FOLD    32
#define X4_SCATTER 64
#define X4_BLOCKS  128    // shards cut into square blocks (with slabs): pixels flying apart

struct H4Cfg {
  uint8_t kind, style, xmode;
  int water;            // water / lava / floor level 0-255 (0 = none)
  float hsc;            // mountain height (1 = HORIZON's default scale)
  int terr;             // terrace step (0 = smooth)
  float fog;            // 0 … 1
  float dist;           // draw distance (cells)
  int pixW;             // column width 1, 2 or 4 (chunky eras)
  int lineW;            // grid / contour line width
  int beat;             // dance-floor beat counter
  bool glowWater;       // water glows (lava, neon)
  bool calmWater;       // water without waves
  int facet;            // low-poly block size for T4_FACETS (2, 4, 8)
};
static H4Cfg h4c;

// ─── Shared memory ────────────────────────────────────────────────────
#define H4_MAXSTEP 256
#define H4_MAXP 150
struct H4Part { float x, y, vx, vy, life, max; uint8_t col, shape, size, spin; };
static int16_t* h4_ybuf; static int32_t* h4_z; static int32_t* h4_iz; static uint8_t* h4_fogS; static int16_t* h4_dir;
static int16_t* h4_yoff; static int16_t* h4_yofT; static int16_t* h4_tw;
static H4Part* h4_p;
static void h4_mem() {
  h4_ybuf = (int16_t*)v3_scratch;                                   // 640
  h4_z = (int32_t*)(v3_scratch + 640);                              // 1024
  h4_iz = (int32_t*)(v3_scratch + 1664);                            // 1024
  h4_fogS = v3_scratch + 2688;                                      // 256
  h4_dir = (int16_t*)(v3_scratch + 2944);                           // 1280
  h4_yoff = (int16_t*)(v3_scratch + 4224);                          // 512
  h4_tw = (int16_t*)(v3_scratch + 4736);                            // 1024
  h4_p = (H4Part*)(v3_scratch + 5760);                              // 150 × 28 = 4200
  h4_yofT = (int16_t*)(v3_scratch + 10000);                         // 512
}

// ─── Camera and flight ────────────────────────────────────────────────
static float h4_x = 128, h4_y = 128, h4_head = 0, h4_alt = 60, h4_t = 0, h4_boost = 0, h4_roll = 0, h4_turnV = 0;
static float h4_ex = 0, h4_burst = 0, h4_ringT = 0;
static int h4_hzY = 70;
static uint8_t h4_band[256];

// tables so the board never computes a sine or a divide per ground sample
static int32_t h4_rlA[256], h4_rlB[256], h4_rlC[256];               // rolling hills: three sine waves
static uint8_t h4_terT[256]; static int h4_terSt = -1;               // terraces: height rounded down to a step
static bool h4_rlOk = false;
static void h4_tables() {
  if (!h4_rlOk) { for (int k = 0; k < 256; k++) { h4_rlA[k] = v3_sin(k * 4) * 45; h4_rlB[k] = v3_sin(k * 4 + 300) * 30; h4_rlC[k] = v3_sin(k * 12 + 100) * 12; } h4_rlOk = true; }
  int st = h4c.terr > 1 ? h4c.terr : 24;
  if (st != h4_terSt) { for (int h = 0; h < 256; h++) h4_terT[h] = (uint8_t)(h - h % st); h4_terSt = st; }
}
// the ground height 0-255 at a map cell
static inline int h4_height(int ix, int iy) {
  ix &= 255; iy &= 255;
  switch (h4c.kind) {
    case T4_TERRACE: return h4_terT[hz_map[(iy << 8) | ix]];
    case T4_DUNES: return 110 + ((v3_sin(ix * 12 + iy * 5) * 50 + v3_sin(iy * 9 - ix * 3 + 300) * 30) >> 14) + (hz_map[(iy << 8) | ix] >> 3);
    case T4_INVERT: { int h = 255 - hz_map[(iy << 8) | ix]; return h > 200 ? 200 + ((h - 200) >> 2) : h; }
    case T4_ROLLING:                                                                             // big soft hills (sines that repeat every 256 cells), a little texture
      return 100 + ((h4_rlA[(ix + iy) & 255] + h4_rlB[(2 * iy - ix) & 255] + h4_rlC[ix]) >> 14) + (hz_map[(iy << 8) | ix] >> 4);
    case T4_CITY: {                                                                              // blocks and streets
      int cx = ix >> 3, cy = iy >> 3;
      if ((ix & 7) < 2 || (iy & 7) < 2) return 8;
      uint32_t hh = v3_hash((uint32_t)(cx * 73 + cy * 9277));
      int base = hz_map[((cy << 3) << 8) | (cx << 3)];                                           // taller downtown where the map is high
      int h = 20 + (int)(((hh & 255) * 60) >> 8) + (base * 3 >> 2) + (((hh >> 8) & 7) == 0 ? 60 : 0);   // (no divides: the board's maths is slow at them)
      return h > 255 ? 255 : h;
    }
    case T4_FLAT: return 24;
    case T4_MESA: { int h = hz_map[(iy << 8) | ix]; return h > 150 ? 190 + (h >> 4) : (h > 95 ? 110 + (h >> 5) : 30 + (h >> 2)); }
    case T4_PYRAMID: {                                                                           // a field of pyramids
      int ax = (ix & 31) - 16, ay = (iy & 31) - 16; ax = ax < 0 ? -ax : ax; ay = ay < 0 ? -ay : ay;
      int m = ax > ay ? ax : ay;
      uint32_t hh = v3_hash((uint32_t)((ix >> 5) * 31 + (iy >> 5) * 977));
      int peak = 120 + (int)(((hh & 255) * 120) >> 8);
      int h = peak - ((m * peak * 17) >> 8); return h > 20 ? h : 20;
    }
    case T4_FACETS: { int m = ~((h4c.facet > 1 ? h4c.facet : 4) - 1); return hz_map[((iy & m) << 8) | (ix & m)]; }   // big flat blocks: early 3D
    case T4_SPIKES: { int h = hz_map[(iy << 8) | ix]; return (h * h) >> 8; }
    default: return hz_map[(iy << 8) | ix];
  }
}

static void h4_reset() { h4_x = 128; h4_y = 128; h4_head = 0; h4_alt = 60; h4_boost = 0; h4_turnV = 0; h4_burst = 0; h4_ex = 0; }

// fly: speed (cells/s), altitude above the ground, turn (−1 … 1), autopilot weave (0 … 1)
static void h4_fly(float speed, float alt, float turn, float autoK, bool boost, float dt) {
  if (boost) h4_boost = 1.0f;
  h4_boost *= expf(-dt * 0.8f);
  h4_t += dt;
  float weave = autoK * (0.55f * sinf(h4_t * 0.23f) + 0.35f * sinf(h4_t * 0.61f + 1.3f));
  float tv = (turn + weave) * 0.9f;
  h4_turnV += (tv - h4_turnV) * v3_clampf(dt * 1.5f, 0, 1);
  h4_head += h4_turnV * dt * (0.6f + speed / 20.0f);
  if (h4_head > 1000.0f || h4_head < -1000.0f) h4_head = fmodf(h4_head, 6.2831853f);
  h4_roll = h4_turnV * 0.35f;
  float v = speed * (1.0f + 2.5f * h4_boost);
  h4_x += cosf(h4_head) * v * dt; h4_y += sinf(h4_head) * v * dt;
  if (h4_x > 1e6f || h4_x < -1e6f) h4_x = fmodf(h4_x, 256.0f);
  if (h4_y > 1e6f || h4_y < -1e6f) h4_y = fmodf(h4_y, 256.0f);
  float gh = 0;
  for (int k = 0; k < 6; k++) {                                      // keep above the ground just ahead
    int h = h4_height((int)(h4_x + cosf(h4_head) * k * 2), (int)(h4_y + sinf(h4_head) * k * 2));
    if (h < h4c.water) h = h4c.water;
    float hw = h * h4c.hsc * 0.25f; if (hw > gh) gh = hw;
  }
  float want = gh + alt * (1.0f - 0.6f * h4_boost);
  h4_alt += (want - h4_alt) * v3_clampf(dt * 2.0f, 0, 1);
  if (h4_alt < gh + 2) h4_alt = gh + 2;
}

// the explosion amount: the knob, plus a one-shot burst (k12) that swells and settles
static void h4_explode(float knob, bool trigger, float dt) {
  if (trigger) h4_burst = 1.0f;
  h4_burst -= dt * 0.35f; if (h4_burst < 0) h4_burst = 0;
  float b = h4_burst > 0.7f ? (1.0f - h4_burst) / 0.3f : h4_burst / 0.7f;   // up fast, down slow
  float want = v3_clampf(knob + b * 0.9f, 0, 1);
  h4_ex += (want - h4_ex) * v3_clampf(dt * 3.0f, 0, 1);
  h4_ringT += dt * (0.25f + h4_ex * 0.35f);
  if (h4_ringT > 1000.0f) h4_ringT -= 1000.0f;
}

// ─── Palette ──────────────────────────────────────────────────────────
// sky: 4 stops top → horizon; acc: accent ramp dark → bright; band: 6 ground colours low → high (fills bands 1-10);
// water: band 0; line: band 11. shadeK: light contrast (1 = normal); fogMix: how strongly fog pulls to the horizon colour
static void h4_palette(const uint8_t (*sky)[3], const uint8_t (*acc)[3], const uint8_t (*band)[3], const uint8_t* water, const uint8_t* line,
                       float shadeK, float fogMix, float glowPulse) {
  v3_palGrad(sky, 4, 0, 40, false);
  for (int i = 0; i < 16; i++) display.setColor((uint8_t)(48 + i), v3_mix8(acc[0][0], acc[1][0], i * 17), v3_mix8(acc[0][1], acc[1][1], i * 17), v3_mix8(acc[0][2], acc[1][2], i * 17));
  uint8_t bc[12][3];
  for (int i = 1; i <= 10; i++) {                                    // 6 colours spread over bands 1-10
    int p = (i - 1) * 5 * 256 / 9, s0 = p >> 8, f = p & 255; if (s0 >= 5) { s0 = 4; f = 255; }
    for (int q = 0; q < 3; q++) bc[i][q] = v3_mix8(band[s0][q], band[s0 + 1][q], f);
  }
  for (int q = 0; q < 3; q++) { bc[0][q] = water[q]; bc[11][q] = line[q]; }
  int sk[4];
  for (int s = 0; s < 4; s++) sk[s] = (int)(256 * (1.0f + (s - 1.6f) * 0.28f * shadeK));
  static const int fogK[4] = {0, 70, 150, 215};
  for (int b = 0; b < 12; b++) for (int f = 0; f < 4; f++) for (int s = 0; s < 4; s++) {
    int k = sk[s];
    if (b == 11 || (b == 0 && h4c.glowWater)) k = (int)(256 * (0.85f + 0.2f * s / 3.0f) * glowPulse);   // glowing: no shading
    int r = v3_clamp(bc[b][0] * k >> 8, 0, 255), g = v3_clamp(bc[b][1] * k >> 8, 0, 255), bl = v3_clamp(bc[b][2] * k >> 8, 0, 255);
    int fk = (int)(fogK[f] * fogMix); if (b == 11) fk /= 2;
    r = v3_mix8(r, sky[3][0], fk); g = v3_mix8(g, sky[3][1], fk); bl = v3_mix8(bl, sky[3][2], fk);
    display.setColor((uint8_t)(64 + b * 16 + f * 4 + s), r, g, bl);
  }
}
// height → band: water, then 10 bands up to the peaks, with optional contour lines
static void h4_bands(int contourW) {
  int water = h4c.water;
  for (int h = 0; h < 256; h++) {
    int b;
    if (h <= water && water > 0) b = 0;
    else { int u = (h - water) * 10 / (256 - water + 1); b = 1 + v3_clamp(u, 0, 9); }
    if (contourW > 0 && b > 0 && (h & 15) < contourW) b = 11;
    h4_band[h] = (uint8_t)b;
  }
}

// ─── Sky ──────────────────────────────────────────────────────────────
static inline void h4_prop(int i, int r, int g, int b) { display.setColor((uint8_t)(H4_PROP + (i & 7)), r, g, b); }
static void h4_sky(int hzY, int span) {                              // the gradient: index 0 high up … 39 at the horizon
  if (span < 20) span = 20;
  for (int y = 0; y < H; y++) {
    int g = v3_clamp((y - (hzY - span)) * 39 / span, 0, 39), g2 = v3_clamp((y + 1 - (hzY - span)) * 39 / span, 0, 39);
    uint8_t a = (uint8_t)g, b = (uint8_t)g2;
    uint32_t w = (y & 1) ? (uint32_t)(a | (b << 8) | (a << 16) | (b << 24)) : (uint32_t)(b | (a << 8) | (b << 16) | (a << 24));
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int x = 0; x < W / 4; x++) p[x] = w;
  }
}
static void h4_stars(int n, int hzY, int seed) {
  for (int i = 0; i < n; i++) {
    uint32_t h = v3_hash(i * 7919u + seed);
    int x = (int)((h % 640u) - (uint32_t)(int)(h4_head * 60.0f + 64000) % 640u + 640) % 640 - 160;
    int y = (int)((h >> 10) % (uint32_t)(hzY > 10 ? hzY : 10));
    if ((unsigned)x < W && y < hzY - 3) v3_put(x, y, (uint8_t)(H4_ACC + 8 + (int)(v3_hash(i + (uint32_t)(h4_t * 3)) & 7)));
  }
}
// a disc from the accent ramp, brightest at the top; stripes: synthwave gaps widening downwards
static void h4_sun(int cx, int cy, int R, bool stripes) {
  if (R < 2) return;
  for (int y = cy - R; y <= cy + R; y++) {
    if ((unsigned)y >= H) continue;
    int dy = y - cy, hw = (int)v3_isqrt((uint32_t)(R * R - dy * dy));
    if (stripes && dy > -R / 5) { int band = (dy + R) * 8 / (2 * R); if (((dy + R) % 9) < band / 2 + 1) continue; }
    v3_hspan(cx - hw, cx + hw, y, (uint8_t)(H4_ACC + v3_clamp(15 - (dy + R) * 15 / (2 * R + 1), 0, 15)));
  }
}
// a soft halo (accent ramp), brightest in the middle; only brightens what's there
static void h4_glow(int cx, int cy, int R, int lev) {
  for (int y = cy - R; y <= cy + R; y++) {
    if ((unsigned)y >= H) continue;
    for (int x = cx - R; x <= cx + R; x++) {
      if ((unsigned)x >= W) continue;
      int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy); if (d2 > R * R) continue;
      int l = lev - d2 * lev / (R * R); if (l <= 0) continue;
      uint8_t* p = v3_buf + y * W + x; int v = H4_ACC + (l > 15 ? 15 : l);
      if (*p < H4_ACC || *p < v || *p >= 64) if (*p < 64 || l > 6) *p = (uint8_t)v;
    }
  }
}
// rays fanning out from a point (art-deco sunburst, rising-sun, laser fans)
static void h4_rays(int cx, int cy, int n, int len, float phase, float spread, int idxA, int idxB, float width) {
  for (int i = 0; i < n; i++) {
    float a = -3.14159f * 0.5f + (i - (n - 1) * 0.5f) * spread / n + phase;
    float w = width * spread / n;
    int32_t xs[3] = {cx * 16, (int32_t)((cx + cosf(a - w) * len) * 16), (int32_t)((cx + cosf(a + w) * len) * 16)};
    int32_t ys[3] = {cy * 16, (int32_t)((cy + sinf(a - w) * len) * 16), (int32_t)((cy + sinf(a + w) * len) * 16)};
    v3_poly(xs, ys, 3, (uint8_t)((i & 1) ? idxB : idxA));
  }
}
// a puffy cloud: overlapping discs, lit on top
static void h4_cloud(int cx, int cy, int sz, uint8_t lit, uint8_t shade) {
  static const int8_t ox[7] = {-12, -5, 4, 12, -8, 2, 9}, oy[7] = {2, -3, -5, 1, 4, 5, 4}, rr[7] = {7, 9, 10, 7, 6, 7, 6};
  for (int k = 0; k < 7; k++) v3_disc(cx + ox[k] * sz / 8, cy + oy[k] * sz / 8 + 1, rr[k] * sz / 8, shade);
  for (int k = 0; k < 4; k++) v3_disc(cx + ox[k] * sz / 8, cy + oy[k] * sz / 8 - 1, rr[k] * sz / 9, lit);
}

// ─── Particles ────────────────────────────────────────────────────────
enum { P4_DOT, P4_SQUARE, P4_SHARD, P4_STREAK, P4_STAR, P4_RING };
static int h4_np = 0;
static void h4_clearParts() { h4_np = 0; }
static void h4_emit(float x, float y, float vx, float vy, float life, uint8_t col, uint8_t shape, uint8_t size) {
  if (h4_np >= H4_MAXP) return;
  H4Part* p = &h4_p[h4_np++];
  p->x = x; p->y = y; p->vx = vx; p->vy = vy; p->life = life; p->max = life; p->col = col; p->shape = shape; p->size = size; p->spin = (uint8_t)v3_rnd(256);
}
// a burst from (x, y) outwards
static void h4_burstAt(float x, float y, int n, float speed, uint8_t col, uint8_t shape, uint8_t size, float up) {
  for (int i = 0; i < n; i++) {
    float a = v3_rndf() * 6.2831853f, s = speed * (0.3f + v3_rndf());
    h4_emit(x, y, cosf(a) * s, sinf(a) * s * 0.7f - up, 1.0f + v3_rndf() * 1.5f, col, shape, size);
  }
}
static void h4_parts(float dt, float grav, float drag) {
  for (int i = 0; i < h4_np; i++) {
    H4Part* p = &h4_p[i];
    p->life -= dt;
    if (p->life <= 0 || p->x < -40 || p->x > W + 40 || p->y < -60 || p->y > H + 40) { *p = h4_p[--h4_np]; i--; continue; }
    p->vy += grav * dt; p->vx *= 1.0f - drag * dt; p->vy *= 1.0f - drag * dt;
    p->x += p->vx * dt; p->y += p->vy * dt; p->spin += 3;
    int x = (int)p->x, y = (int)p->y, s = p->size;
    float k = p->life / p->max;
    uint8_t c = p->col;
    switch (p->shape) {
      case P4_SQUARE: v3_rect(x - s / 2, y - s / 2, x + s / 2, y + s / 2, c); break;
      case P4_SHARD: {
        int a = p->spin * 4;
        int32_t xs[3], ys[3];
        for (int j = 0; j < 3; j++) { int aa = a + j * 341 + (j == 2 ? 80 : 0); xs[j] = (x * 16) + ((v3_cos(aa) * s * 16) >> 14); ys[j] = (y * 16) + ((v3_sin(aa) * s * 12) >> 14); }
        v3_poly(xs, ys, 3, c); break;
      }
      case P4_STREAK: v3_line(x, y, x - (int)(p->vx * 0.05f), y - (int)(p->vy * 0.05f), c); break;
      case P4_STAR: { int r = (int)(s * (0.5f + 0.5f * k)) + 1; v3_line(x - r, y, x + r, y, c); v3_line(x, y - r, x, y + r, c); v3_put(x, y, c); break; }
      case P4_RING: { int r = (int)(s * (1.0f - k) * 3) + 1; for (int a = 0; a < 1024; a += 32) v3_put(x + ((v3_cos(a) * r) >> 14), y + ((v3_sin(a) * r) >> 14), c); break; }
      default: v3_put(x, y, c); if (s > 1) { v3_put(x + 1, y, c); v3_put(x, y + 1, c); v3_put(x + 1, y + 1, c); }
    }
  }
}

// ─── The ground ───────────────────────────────────────────────────────
static int16_t h4_shY[24]; static int16_t h4_shX[24];             // shard offsets per column group
static void h4_ground(int hzY, int rollPx) {
  const float E = h4_ex;
  h4_tables();
  const int xm = h4c.xmode;
  // steps out from the camera
  int nst = 0;
  {
    float z = 0.8f, dz = 0.14f, dist = h4c.dist;
    while (z < dist && nst < H4_MAXSTEP) {
      h4_z[nst] = (int32_t)(z * 256);
      h4_iz[nst] = (int32_t)(150.0f * 256.0f / z);
      float u = z / dist;
      h4_fogS[nst] = (uint8_t)v3_clamp((int)(u * u * 4.0f * h4c.fog * 1.15f), 0, 3);
      int yo = 0, yt = 0;
      if ((xm & X4_SLABS) && E > 0.01f) { int k = nst * 10 / 120; yt = (k & 1) ? (int)(E * k * 3) : -(int)(E * (k + 1) * 8); }   // layers crack apart (each moved whole)
      if ((xm & X4_FOLD) && E > 0.01f) yo -= (int)(E * 330.0f * u * u * u);                                               // the far land curls up (stretched)
      h4_yoff[nst] = (int16_t)yo; h4_yofT[nst] = (int16_t)yt;
      if ((xm & X4_TWIST) && E > 0.01f) { float a = E * 3.2f * u * (0.4f + u); h4_tw[nst * 2] = (int16_t)(cosf(a) * 4096); h4_tw[nst * 2 + 1] = (int16_t)(sinf(a) * 4096); }
      nst++; z += dz; dz *= 1.04f;
    }
  }
  float ch = cosf(h4_head), sh = sinf(h4_head);
  for (int x = 0; x < W; x++) {
    float a = (x - 160) / 160.0f;
    h4_dir[x * 2] = (int16_t)((ch - sh * a) * 4096); h4_dir[x * 2 + 1] = (int16_t)((sh + ch * a) * 4096);
    h4_ybuf[x] = H;
  }
  // shards: the screen splits into column groups that fly up and apart, with gaps between them
  int gw = (xm & X4_BLOCKS) ? 16 : 20, gap = 0;
  bool shards = (xm & (X4_SHARDS | X4_BLOCKS)) && E > 0.01f;
  if (shards) {
    gap = (int)(E * gw * 0.35f);
    for (int g = 0; g < W / gw + 1 && g < 24; g++) {
      uint32_t hh = v3_hash((uint32_t)g * 2654435761u + 17);
      float c = (g * gw + gw * 0.5f - 160) / 160.0f;
      h4_shX[g] = (int16_t)(E * c * 70);
      h4_shY[g] = (int16_t)(-E * (20 + (hh % 120)) * (0.5f + 0.5f * (1 - fabsf(c))));
      if ((xm & X4_BLOCKS) && (hh & 1)) h4_shY[g] = (int16_t)(E * (10 + (hh >> 8) % 50));
    }
  }
  // the centre of a shockwave or eruption: a point ahead on the horizon
  int32_t ecx = (int32_t)((h4_x + ch * 85.0f) * 256), ecy = (int32_t)((h4_y + sh * 85.0f) * 256);
  float ringR = (h4_ringT - floorf(h4_ringT)) * 120.0f;
  int ringR8 = (int)(ringR * 8), ringAmp = (int)(E * 90), eruptAmp = (int)(E * 160);
  bool ring = (xm & X4_RING) && E > 0.01f, erupt = (xm & X4_ERUPT) && E > 0.01f;
  bool twist = (xm & X4_TWIST) && E > 0.01f, scatter = (xm & X4_SCATTER) && E > 0.01f;
  int scatK = (int)(E * 256);

  int32_t camx = (int32_t)(h4_x * 256), camy = (int32_t)(h4_y * 256);
  int32_t camh = (int32_t)(h4_alt * 16);
  int hs = (int)(h4c.hsc * 64);
  int waveT = (int)(h4_t * 6);
  const int water = h4c.water, style = h4c.style, lineW = h4c.lineW;
  const uint8_t lineB = 11;
  const bool mapKind = h4c.kind == T4_MAP, city = h4c.kind == T4_CITY, gridOn = (style & (S4_GRID | S4_WIRE)) != 0;
  const bool fast = !twist && !ring && !erupt && !scatter && !(((xm & (X4_SLABS | X4_FOLD)) != 0) && E > 0.01f) && !(style & (S4_TILES | S4_ROAD | S4_FLOWERS));
  // speed guard: a deep view (or an exploding one) is drawn in double-width columns so a frame stays inside the budget
  const bool quant = h4c.pixW > 1;
  const int pw = quant ? h4c.pixW : ((nst * (fast ? 2 : 3) > 150 || h4c.kind != T4_MAP) ? 2 : 1);
  for (int x0 = 0; x0 < W; x0 += pw) {
    int rx = x0 + pw / 2;
    int extraY = 0;
    if (shards) {
      int g = x0 / gw; if (g > 23) g = 23;
      if ((x0 % gw) >= gw - gap) continue;                          // a crack: the sky shows through
      rx -= h4_shX[g]; extraY = h4_shY[g];
      rx = v3_clamp(rx, 0, W - 1);
    }
    int32_t dxr = h4_dir[rx * 2], dyr = h4_dir[rx * 2 + 1];
    int ybuf = h4_ybuf[x0];
    if (extraY < 0) ybuf += extraY;                                  // a lifted shard: its bottom edge lifts too, the glow shows beneath
    int roll = ((x0 - 160) * rollPx) / 160 + extraY;
    uint8_t* col = v3_buf + x0;
    int topY = H, prevH = 0;
    if (fast) {                                                      // the common case, kept tight: no explosion bending the land, plain styles
      for (int s = 0; s < nst; s++) {
        int32_t z = h4_z[s];
        int32_t wx = camx + ((dxr * z) >> 12), wy = camy + ((dyr * z) >> 12);
        int ix = (wx >> 8) & 255, iy = (wy >> 8) & 255;
        int h = mapKind ? hz_map[(iy << 8) | ix] : h4_height(ix, iy);
        int band, shade;
        if (h <= water && water > 0) {
          int wv = hz_map[(((iy + waveT) & 255) << 8) | ((ix + (waveT >> 1)) & 255)];
          h = water + (h4c.calmWater ? 0 : (wv >> 6));
          band = 0; shade = (wv & 15) == 0 ? 3 : ((wv & 15) == 1 ? 1 : 2);
        } else {
          if (city) shade = (ix & 7) == 2 ? 3 : ((iy & 7) == 2 ? 1 : 2);
          else {
            int hx = mapKind ? hz_map[(iy << 8) | ((ix + 1) & 255)] : h4_height(ix + 1, iy);
            int d = hx - h;
            shade = d > 5 ? 3 : (d > 0 ? 2 : (d > -5 ? 1 : 0));
          }
          band = h4_band[h];
        }
        int y = hzY + roll + (int)(((camh - ((h * hs) >> 4)) * h4_iz[s]) >> 12);
        if (quant) y &= ~(pw - 1);
        if (y < ybuf) {
          int f = h4_fogS[s];
          uint8_t pix;
          if (gridOn) {
            bool onLine = (wx & 0x7FF) < lineW * 8 + (s >> 1) * 6 || (wy & 0x7FF) < lineW * 8 + (s >> 1) * 6;
            pix = onLine ? h4_pix(lineB, f, 3) : ((style & S4_WIRE) ? h4_pix(0, 3, 0) : h4_pix(band, f, shade));
          } else pix = h4_pix(band, f, shade);
          int y0 = y < 0 ? 0 : y, y1 = ybuf;
          if (style & S4_DOTS) { int l = s < 30 ? 2 : 1; if (y0 + l < y1) y1 = y0 + l; }
          uint8_t* p = col + y0 * W;
          if (pw == 1) { for (int yy = y0; yy < y1; yy++, p += W) *p = pix; }
          else { for (int yy = y0; yy < y1; yy++, p += W) for (int k = 0; k < pw && x0 + k < W; k++) p[k] = pix; }
          if ((style & S4_EDGES) && band != 0 && y >= 0 && h - prevH > 5 && s > 2) { uint8_t e = h4_pix(lineB, f, 3); for (int k = 0; k < pw && x0 + k < W; k++) col[y * W + k] = e; }
          ybuf = y0; prevH = h;
          if (ybuf <= 0) break;
        }
      }
    } else
    for (int s = 0; s < nst; s++) {
      int32_t z = h4_z[s];
      int32_t dx = dxr, dy = dyr;
      if (twist) { int32_t c = h4_tw[s * 2], sn = h4_tw[s * 2 + 1]; dx = (dxr * c - dyr * sn) >> 12; dy = (dxr * sn + dyr * c) >> 12; }
      int32_t wx = camx + ((dx * z) >> 12), wy = camy + ((dy * z) >> 12);
      int ix = (wx >> 8) & 255, iy = (wy >> 8) & 255;
      int h = mapKind ? hz_map[(iy << 8) | ix] : h4_height(ix, iy);
      if (ring || erupt) {
        int32_t ex = (wx - ecx) >> 5, ey = (wy - ecy) >> 5;            // 1/8 cell
        int32_t d2 = ex * ex + ey * ey;
        if (erupt && d2 < 144 * 144) { int u = 256 - (int)((d2 << 8) / (144 * 144)); h += (eruptAmp * ((u * u) >> 8)) >> 8; }
        if (ring) { int d = (int)v3_isqrt((uint32_t)d2) - ringR8; if (d > -40 && d < 40) { int u = 40 - (d < 0 ? -d : d); h += ringAmp * u / 40; } }
        }
      if (h > 255) h = 255;
      int band, shade;
      if (h <= water && water > 0) {
        int wv = hz_map[(((iy + waveT) & 255) << 8) | ((ix + (waveT >> 1)) & 255)];
        h = water + (h4c.calmWater ? 0 : (wv >> 6));
        band = 0; shade = (wv & 15) == 0 ? 3 : ((wv & 15) == 1 ? 1 : 2);
      } else {
        if (h4c.kind == T4_CITY) shade = (ix & 7) == 2 ? 3 : ((iy & 7) == 2 ? 1 : 2);   // buildings: one lit face, one shaded
        else {
          int hx = mapKind ? hz_map[(iy << 8) | ((ix + 1) & 255)] : h4_height(ix + 1, iy);
          int d = hx - h;
          shade = d > 5 ? 3 : (d > 0 ? 2 : (d > -5 ? 1 : 0));
        }
        band = h4_band[h];
      }
      int32_t hw = (h * hs) >> 4;
      int y = hzY + roll + h4_yoff[s] + (int)(((camh - hw) * h4_iz[s]) >> 12);
      if (scatter) y += (((int)(v3_hash((uint32_t)(ix * 263 + iy * 71)) & 63) - 32) * scatK) >> 6;
      if (quant) y &= ~(pw - 1);
      if (y < ybuf) {
        int f = h4_fogS[s];
        if (y < -200) y = -200;
        uint8_t pix;
        bool onLine = false;
        if (gridOn) onLine = ((wx & 0x7FF) < lineW * 8 + (s >> 1) * 6 || (wy & 0x7FF) < lineW * 8 + (s >> 1) * 6);
        if (style & (S4_ROAD | S4_FLOWERS | S4_TILES)) {
        if ((style & S4_ROAD) && band > 0) { int ry = 128 + ((v3_sin(ix * 6) * 28) >> 14); int d = iy - ry; if (d < 0) d = -d; if (d < 3) { band = 11; shade = 3; } }
        if ((style & S4_FLOWERS) && band > 1 && band < 10 && (v3_hash((uint32_t)(ix * 31 + iy * 1013)) & 15) == 0) { band = 11; shade = 3; }
        if (style & S4_TILES) {
          int tx = ix >> 2, ty = iy >> 2;
          uint32_t hh = v3_hash((uint32_t)(tx * 977 + ty * 131 + h4c.beat * 7));
          if (((tx + ty) & 1) == 0) { band = (hh & 3) == 0 ? 11 : 1 + (int)((hh >> 4) % 10); shade = 3; } else { band = 1; shade = 0; }
          if ((ix & 3) == 0 || (iy & 3) == 0) { band = 0; shade = 0; }
        }
        }
        if (style & S4_WIRE) pix = onLine ? h4_pix(lineB, f, 3) : h4_pix(0, 3, 0);
        else if (onLine) pix = h4_pix(lineB, f, 3);
        else pix = h4_pix(band, f, shade);
        int yt = h4_yofT[s];                                         // a slab moved whole: draw its slice shifted, leaving a gap
        int y0 = y + yt < 0 ? 0 : y + yt;
        int y1 = ybuf + yt > H ? H : ybuf + yt;
        if (style & S4_DOTS) y1 = y0 + (s < 30 ? 2 : 1) < ybuf ? y0 + (s < 30 ? 2 : 1) : ybuf;   // a point cloud: just the top of each slice
        if (scatter && E > 0.3f) { int len = (int)((ybuf - y0) * (1.2f - E)); if (len < 1) len = 1; if (y0 + len < y1) y1 = y0 + len; }
        uint8_t* p = col + y0 * W;
        if (pw == 1) { for (int yy = y0; yy < y1; yy++, p += W) *p = pix; }
        else { for (int yy = y0; yy < y1; yy++, p += W) for (int k = 0; k < pw && x0 + k < W; k++) p[k] = pix; }
        if ((style & S4_EDGES) && band != 0 && y + yt >= 0 && y + yt < H && h - prevH > 5 && s > 2) { uint8_t e = h4_pix(lineB, f, 3); for (int k = 0; k < pw && x0 + k < W; k++) col[(y + yt) * W + k] = e; }   // ink the ridge
        ybuf = y < 0 ? 0 : y; prevH = h;
        if (y < topY) topY = y;
        if (ybuf <= 0) break;
      }
    }
    for (int k = 0; k < pw && x0 + k < W; k++) h4_ybuf[x0 + k] = (int16_t)ybuf;
  }
}

// ─── After-effects ────────────────────────────────────────────────────
static void h4_scanlines() {                                         // darker odd lines (ground: one step more fog; sky: higher up the gradient)
  for (int y = 1; y < H; y += 2) {
    uint8_t* p = v3_buf + y * W;
    for (int x = 0; x < W; x++) {
      uint8_t v = p[x];
      if (v >= 64) { if ((v & 12) != 12) p[x] = (uint8_t)(v + 4); else if (v & 3) p[x] = (uint8_t)(v - 1); }
      else if (v < 40) p[x] = (uint8_t)(v > 5 ? v - 5 : 0);
    }
  }
}
static void h4_grain(int n, uint8_t light, uint8_t dark, int scratches) {   // film grain and scratches
  for (int i = 0; i < n; i++) { uint32_t r = v3_rand(); v3_put((int)(r % W), (int)((r >> 9) % H), (r >> 20) & 1 ? light : dark); }
  for (int i = 0; i < scratches; i++) { int x = (int)(v3_hash((uint32_t)(h4_t * 24) + i * 77) % W); int y0 = v3_rnd(H / 2), y1 = y0 + 40 + v3_rnd(H); for (int y = y0; y < y1 && y < H; y++) v3_put(x, y, light); }
}
static void h4_shiftRow(int y, int dx) {
  if ((unsigned)y >= H || dx == 0) return;
  uint8_t* r = v3_buf + y * W;
  if (dx > 0) { if (dx >= W) return; memmove(r + dx, r, W - dx); } else { dx = -dx; if (dx >= W) return; memmove(r, r + dx, W - dx); }
}
static void h4_mirror(int mode) {                                    // 1: left → right, 2: also top → bottom, 3: four-way round the centre
  if (mode >= 1) for (int y = 0; y < H; y++) { uint8_t* r = v3_buf + y * W; for (int x = 0; x < 160; x++) r[319 - x] = r[x]; }
  if (mode >= 2) for (int y = 0; y < 120; y++) memcpy(v3_buf + (239 - y) * W, v3_buf + y * W, W);
}

// ─── Knobs every Set 4 program shares ─────────────────────────────────
//  p0 Palette  p1 Speed  p2 Altitude  p3 Turn  p4 Pitch  p5 Mountains  p6 Water / Floor  p7 (era)
//  p8 Fog  p9 Sun  p10 Draw Distance  p11 (era)  p12 Sky  p13 Autopilot  p14 Pulse  p15 EXPLODE
struct H4Knobs { float speed, alt, turn, pitch, hsc, fog, sunK, dist, skyK, autoK, pulseK, explode; int water; };
static H4Knobs h4k;
static void h4_readKnobs(float ownH, int ownWater, float ownFog, float ownSpeed) {
  h4k.speed = v3_k(1) < 30 ? 0.0f : ownSpeed * powf(3.0f, v3_bi(1));
  h4k.alt = 14.0f * powf(2.5f, v3_bi(2));
  h4k.turn = v3_centre(3) ? 0.0f : v3_bi(3);
  h4k.pitch = v3_bi(4);
  h4k.hsc = v3_centre(5) ? ownH : 0.15f + v3_u(5) * 1.6f;
  h4k.water = v3_centre(6) ? ownWater : v3_k(6) * 220 / 1023;
  h4k.fog = v3_centre(8) ? ownFog : v3_u(8);
  h4k.sunK = v3_centre(9) ? 1.0f : 0.3f + v3_u(9) * 1.6f;
  h4k.dist = 150.0f * powf(1.7f, v3_bi(10));
  h4k.skyK = v3_centre(12) ? 1.0f : v3_u(12) * 2.0f;
  h4k.autoK = v3_centre(13) ? 0.5f : v3_u(13);
  h4k.pulseK = v3_centre(14) ? 0.5f : v3_u(14);
  h4k.explode = v3_clampf((v3_u(15) - 0.45f) / 0.55f, 0, 1);       // the lower half keeps the land whole; the upper half blows it apart
}
static const char* const h4_labelsBase[16] = {
  "Palette", "Speed", "Altitude", "Turn", "Pitch", "Mountains", "Water Level", "",
  "Fog", "Sun", "Draw Distance", "", "Sky", "Autopilot", "Pulse", "EXPLODE"
};

// ─── Worlds and the shared frame routine ──────────────────────────────
// Each program has 11 worlds (its presets k0-k10; k11 tours them). A world is colours plus how the land looks.
#define W4_GLOW   1       // water glows (lava, neon)
#define W4_CALM   2       // water without waves
#define W4_STARS  4       // stars in the sky
#define W4_STRIPE 8       // striped sun
struct H4World {
  uint8_t sky[4][3], acc[2][3], band[6][3], water[3], line[3];
  uint8_t kind, style, xmode, waterLev, height, fog, terr, pixW, flags, deco, sunR, sunY;
};
static const H4World* h4_W = 0;
static int h4_nW = 11, h4_preset = -1, h4_world = 0, h4_prev = 0, h4_pal = 0;
static float h4_X = 1, h4_tourT = 0, h4_dt = 0.016f, h4_pulse = 0.5f, h4_bandShift = 0;   // bandShift: rotate the ground colours (0 … 6)
static bool h4_k12was = false, h4_trigger = false, h4_tour = false;
static int h4_rollPx = 0;

static void h4_init(const H4World* worlds, int n) {
  h4_mem(); v3_reset(); h4_W = worlds; h4_nW = n; h4_preset = -1; h4_clearParts(); h4_reset(); h4_bandShift = 0;
}
// colours of two worlds mixed (x = 0 … 1), then built into the palette
static void h4_worldPal(int a, int b, float x, float shadeK, float fogMix) {
  const H4World& A = h4_W[a]; const H4World& B = h4_W[b];
  int xf = (int)(x * 256);
  uint8_t sky[4][3], acc[2][3], band[6][3], water[3], line[3];
  for (int q = 0; q < 3; q++) {
    for (int i = 0; i < 4; i++) sky[i][q] = v3_mix8(A.sky[i][q], B.sky[i][q], xf);
    for (int i = 0; i < 2; i++) acc[i][q] = v3_mix8(A.acc[i][q], B.acc[i][q], xf);
    for (int i = 0; i < 6; i++) {
      float p = i + h4_bandShift; p -= floorf(p / 6.0f) * 6.0f;
      int i0 = (int)p, i1 = (i0 + 1) % 6, f = (int)((p - i0) * 256);
      int ca = v3_mix8(A.band[i0][q], B.band[i0][q], xf), cb = v3_mix8(A.band[i1][q], B.band[i1][q], xf);
      band[i][q] = v3_mix8(ca, cb, f);
    }
    water[q] = v3_mix8(A.water[q], B.water[q], xf); line[q] = v3_mix8(A.line[q], B.line[q], xf);
  }
  h4_palette(sky, acc, band, water, line, shadeK, fogMix, 0.75f + 0.45f * h4_pulse);
}
// everything up to the sky: timing, presets and the tour, knobs, flight, the explosion, colours
static bool h4_prepare(int preset, float ownSpeed, float shadeK, float fogMix) {
  v3_buf = display.getBuffer();
  if (!v3_buf || !h4_W) return false;
  if (preset < 0 || preset >= 12) preset = 0;
  h4_mem();
  v3_frame();
  float dt = v3_dt; h4_dt = dt;
  h4_tour = preset == 11;
  if (preset != h4_preset) {
    h4_preset = preset; h4_world = h4_tour ? v3_rnd(h4_nW) : (preset < h4_nW ? preset : 0);
    h4_prev = h4_world; h4_X = 1; h4_tourT = 0; h4_clearParts();
  }
  if (h4_tour) { h4_tourT += dt; if (h4_tourT > 25.0f) { h4_tourT = 0; h4_prev = h4_world; h4_world = (h4_world + 1 + v3_rnd(h4_nW - 1)) % h4_nW; h4_X = 0; } }
  if (h4_X < 1) { h4_X += dt / 2.5f; if (h4_X > 1) { h4_X = 1; h4_prev = h4_world; } }
  const H4World& w = h4_W[h4_world];
  h4_readKnobs(w.height / 100.0f, w.waterLev, w.fog / 100.0f, ownSpeed);
  h4_pal = v3_centre(0) ? h4_world : v3_clamp(v3_k(0) * h4_nW / 1024, 0, h4_nW - 1);
  h4c.kind = w.kind; h4c.style = w.style; h4c.xmode = w.xmode;
  h4c.water = h4k.water; h4c.hsc = h4k.hsc; h4c.terr = w.terr; h4c.fog = h4k.fog; h4c.dist = h4k.dist;
  h4c.pixW = w.pixW ? w.pixW : 1; h4c.lineW = 2; h4c.facet = 4;
  h4c.glowWater = w.flags & W4_GLOW; h4c.calmWater = w.flags & W4_CALM;
  bool k12 = keysPressed[KEY_MOD_A];
  h4_trigger = k12 && !h4_k12was;
  h4_k12was = k12;
  h4_fly(h4k.speed, h4k.alt, h4k.turn, h4k.autoK, false, dt);
  h4_explode(h4k.explode, h4_trigger, dt);
  h4_hzY = 70 + (int)(h4k.pitch * 60);
  h4_rollPx = (int)(h4_roll * 60);
  h4_pulse = 0.5f + 0.5f * sinf(h4_t * 6.2832f) * h4k.pulseK;
  h4c.beat = (int)(h4_t * 2.0f * (0.5f + h4k.pulseK));
  int palA = h4_X < 1 ? h4_prev : h4_pal, palB = h4_X < 1 ? h4_world : h4_pal;
  if (!v3_centre(0)) { palA = palB = h4_pal; }
  h4_worldPal(palA, palB, h4_X < 1 ? h4_X : 1.0f, shadeK, fogMix);
  h4_bands((h4c.style & S4_CONTOUR) ? 2 : 0);
  return true;
}
// the vanishing point on screen (where the explosions start)
static inline int h4_vx() { return 160; }
static inline int h4_vy() { return h4_hzY; }
#endif

// =====================================================================
// vg5s.h — the Set 5 strand engine (by Dewey)
//
// Ropes made of points, simulated the "Verlet" way: each point remembers
// where it was last frame, so its speed is just (now − before). Neighbouring
// points on a rope are pulled back to a fixed distance apart, which makes
// the rope bend but not stretch. Points can be pinned in place.
//
// Strands pile up instead of passing through each other: every frame the
// screen is split into 8×8-pixel cells, the points in each cell are
// counted, and points in crowded cells are pushed towards emptier ones.
// It's cheap, and the heaps it makes look like tangled spaghetti.
//
// Everything is in whole numbers: positions in 1/16 pixel. Used by
// CHIPLEADER, HAIR, SPAGHETTI and LOOM (Set 5, slots 0-3). The points live
// in vg3.h's shared working memory, so a program must call s5_setup() in
// its init and whenever it changes how many strands it has.
// =====================================================================
#ifndef VG5S_H
#define VG5S_H
#include "vg3.h"
#include "vg5fx.h"

#define S5_MAXP 1560                                 // points (8 bytes each)
#define S5_GW 40                                     // density grid: 40 × 30 cells of 8 px
#define S5_GH 30
static int16_t *s5_x, *s5_y, *s5_ox, *s5_oy;         // position and last position, 1/16 px
static uint8_t *s5_pin;                              // 1 bit per point: held in place
static uint8_t *s5_cnt;                              // density grid
static int s5_n = 0, s5_per = 8, s5_ns = 0;          // points, points per strand, strands
static int s5_rest = 96;                             // rest length between neighbours, 1/16 px

static void s5_setup(int strands, int per, int restPx16) {
  if (per < 2) per = 2;
  if (strands * per > S5_MAXP) strands = S5_MAXP / per;
  s5_ns = strands; s5_per = per; s5_n = strands * per; s5_rest = v3_clamp(restPx16, 8, 700);   // ≤ 700 keeps the rope maths in 32 bits
  uint8_t* m = v3_scratch;
  s5_x = (int16_t*)m;  m += S5_MAXP * 2;
  s5_y = (int16_t*)m;  m += S5_MAXP * 2;
  s5_ox = (int16_t*)m; m += S5_MAXP * 2;
  s5_oy = (int16_t*)m; m += S5_MAXP * 2;
  s5_pin = m;          m += (S5_MAXP + 7) / 8;
  s5_cnt = m;                                        // 1200 bytes (12480 + 195 + 1200 < 14336)
  memset(s5_pin, 0, (S5_MAXP + 7) / 8);
}
static inline bool s5_pinned(int i) { return (s5_pin[i >> 3] >> (i & 7)) & 1; }
static inline void s5_setPin(int i, bool on) { if (on) s5_pin[i >> 3] |= (uint8_t)(1 << (i & 7)); else s5_pin[i >> 3] &= (uint8_t)~(1 << (i & 7)); }
static inline void s5_place(int i, int x16, int y16) { s5_x[i] = s5_ox[i] = (int16_t)x16; s5_y[i] = s5_oy[i] = (int16_t)y16; }
static inline void s5_kick(int i, int vx16, int vy16) { s5_ox[i] = (int16_t)(s5_x[i] - vx16); s5_oy[i] = (int16_t)(s5_y[i] - vy16); }

// the world's edges, 1/16 px (the floor can be lowered off-screen to let everything fall away)
static int s5_left = 0, s5_right = (W - 1) * 16, s5_top = -2000, s5_floor = (H - 2) * 16;
static int s5_bounce = 0;                            // 0 = dead stop … 256 = full bounce off walls/floor
static int s5_friction = 160;                        // floor friction: how much sideways speed is kept (/256)

// move every free point: gravity (gx, gy in 1/16 px per frame²) and damping (/256 of speed kept)
static void s5_move(int gx, int gy, int keep) {
  for (int i = 0; i < s5_n; i++) {
    if (s5_pinned(i)) continue;
    int x = s5_x[i], y = s5_y[i];
    int vx = ((x - s5_ox[i]) * keep) / 256, vy = ((y - s5_oy[i]) * keep) / 256;   // (/, not >>: rounds towards zero, so no drift)
    if (vx > 300) vx = 300; else if (vx < -300) vx = -300;      // a speed limit keeps it all stable
    if (vy > 300) vy = 300; else if (vy < -300) vy = -300;
    s5_ox[i] = (int16_t)x; s5_oy[i] = (int16_t)y;
    x += vx + gx; y += vy + gy;
    s5_x[i] = (int16_t)v3_clamp(x, -1000, 6100); s5_y[i] = (int16_t)v3_clamp(y, -3000, 5000);
  }
}
// the walls and floor
static void s5_walls() {
  for (int i = 0; i < s5_n; i++) {
    if (s5_pinned(i)) continue;
    int y = s5_y[i], x = s5_x[i];
    if (y > s5_floor) {
      int vy = y - s5_oy[i], vx = x - s5_ox[i];
      s5_y[i] = (int16_t)s5_floor; s5_oy[i] = (int16_t)(s5_floor + (vy * s5_bounce) / 256);
      s5_ox[i] = (int16_t)(x - (vx * s5_friction) / 256);
    } else if (y < s5_top) { s5_y[i] = (int16_t)s5_top; }
    if (x < s5_left) { int vx = x - s5_ox[i]; s5_x[i] = (int16_t)s5_left; s5_ox[i] = (int16_t)(s5_left + (vx * s5_bounce) / 256); }
    else if (x > s5_right) { int vx = x - s5_ox[i]; s5_x[i] = (int16_t)s5_right; s5_ox[i] = (int16_t)(s5_right + (vx * s5_bounce) / 256); }
  }
}
// pull neighbours on each rope back to the rest length (no square roots: the Jakobsen approximation)
static void s5_ropes(int iters) {
  const int32_t L2 = (int32_t)s5_rest * s5_rest;
  for (int it = 0; it < iters; it++) {
    for (int s = 0; s < s5_ns; s++) {
      int a = s * s5_per, e = a + s5_per - 1;
      for (int i = a; i < e; i++) {
        int32_t dx = s5_x[i + 1] - s5_x[i], dy = s5_y[i + 1] - s5_y[i];
        int32_t d2 = dx * dx + dy * dy;
        if (d2 > 4000000) d2 = 4000000;
        int32_t k = (L2 << 12) / (d2 + L2) - 2048;                       // Q12: how far each end moves along (dx, dy)
        int mx = (int)((dx * k) / 4096), my = (int)((dy * k) / 4096);
        bool pa = s5_pinned(i), pb = s5_pinned(i + 1);
        if (pa && pb) continue;
        if (pa) { mx *= 2; my *= 2; }
        if (!pa && !pb) { s5_x[i] -= mx; s5_y[i] -= my; s5_x[i + 1] += mx; s5_y[i + 1] += my; }
        else if (pa) { s5_x[i + 1] += mx; s5_y[i + 1] += my; }
        else { s5_x[i] -= mx * 2; s5_y[i] -= my * 2; }
      }
    }
  }
}
static uint32_t s5_tick = 0;
// crowded cells push their points towards emptier ones: strands pile up instead of passing through
static void s5_crowd(int limit, int push) {
  if (push <= 0) return;
  s5_tick += 0x9E3779B9u;
  memset(s5_cnt, 0, S5_GW * S5_GH);
  for (int i = 0; i < s5_n; i++) {
    int cx = s5_x[i] >> 7, cy = s5_y[i] >> 7;
    if ((unsigned)cx < S5_GW && (unsigned)cy < S5_GH) { uint8_t* c = s5_cnt + cy * S5_GW + cx; if (*c < 255) (*c)++; }
  }
  for (int i = 0; i < s5_n; i++) {
    if (s5_pinned(i)) continue;
    int cx = s5_x[i] >> 7, cy = s5_y[i] >> 7;
    if ((unsigned)cx >= S5_GW || (unsigned)cy >= S5_GH) continue;
    const uint8_t* c = s5_cnt + cy * S5_GW + cx;
    int n = *c; if (n <= limit) continue;
    int l = cx > 0 ? c[-1] : 255, r = cx < S5_GW - 1 ? c[1] : 255;
    int u = cy > 0 ? c[-S5_GW] : n, d = cy < S5_GH - 1 ? c[S5_GW] : 255;   // the floor counts as full
    int over = n - limit; if (over > 12) over = 12;
    int gx = l - r, gy = u - d;
    gy -= 2;                                                       // a slight upward lean: piles grow up, not sideways only
    int mx = (gx * push * over) / 64, my = (gy * push * over) / 64;
    int j = (int)(v3_hash((uint32_t)i * 2654435761u + s5_tick) & 15) * 2 - 15;   // −15 … 15, evenly balanced: keeps a heap loose without drifting
    mx += j * over / 8;
    if (mx > 24) mx = 24; else if (mx < -24) mx = -24;
    if (my > 24) my = 24; else if (my < -24) my = -24;
    s5_x[i] += (int16_t)mx; s5_y[i] += (int16_t)my;
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static inline void s5_clear(uint8_t c) { memset(v3_buf, c, W * H); }
// one strand as a polyline; `under` > 0 adds a darker line under it (thicker, striped look)
static void s5_drawStrand(int s, uint8_t c, uint8_t under) {
  int a = s * s5_per;
  int px = s5_x[a] >> 4, py = s5_y[a] >> 4;
  for (int i = a + 1; i < a + s5_per; i++) {
    int x = s5_x[i] >> 4, y = s5_y[i] >> 4;
    if (under) v3_line(px, py + 1, x, y + 1, under);
    v3_line(px, py, x, y, c);
    px = x; py = y;
  }
}
// a strand as a 3-pixel band: bright top line, mid line, dark bottom line (a chip seen edge-on)
static void s5_drawBand(int s, uint8_t c0, uint8_t c1, uint8_t c2) {
  int a = s * s5_per;
  int px = s5_x[a] >> 4, py = s5_y[a] >> 4;
  for (int i = a + 1; i < a + s5_per; i++) {
    int x = s5_x[i] >> 4, y = s5_y[i] >> 4;
    v3_line(px, py + 2, x, y + 2, c2); v3_line(px, py + 1, x, y + 1, c1); v3_line(px, py, x, y, c0);
    px = x; py = y;
  }
}
// a strand drawn smooth (anti-aliased), brightest wins; slower, so used for thinner scenes
static void s5_drawStrandAA(int s, int lev, int hue) {
  int a = s * s5_per;
  for (int i = a; i < a + s5_per - 1; i++) v3_lineAA(s5_x[i] >> 1, s5_y[i] >> 1, s5_x[i + 1] >> 1, s5_y[i + 1] >> 1, lev, hue);
}
// a soft value noise over the screen for wind: x, y in pixels, t in 1/256 s steps; result −16384 … 16384
static inline int32_t s5_wind(int x, int y, uint32_t t, uint32_t seed) {
  return (v3_noise1((uint32_t)(x * 3 + y * 2) + t, seed) + v3_noise1((uint32_t)(y * 5 - x) + (t >> 1) + 50000, seed ^ 0x55u)) >> 1;
}
#endif

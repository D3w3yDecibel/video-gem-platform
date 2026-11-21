// =====================================================================
// PROGRAM: SUNSET — endless synthwave drive (by Dewey) — Set 8 slot 9
//
// Greatest hit from Set 3 (HORIZON: Synthwave) and Set 4 (1980s ARCADE,
// 2010s VAPOR) and the night city of Set 2 (PIXEL CITY): a slow cruise
// toward a striped sunset over a neon grid, a palm-lined ocean, a desert,
// or a city skyline, with mountains and stars. Pure late-night lounge.
//
// How it works: drawn straight onto the screen, row by row. The sky is a
// dithered gradient written four pixels at a time; the sun is a disc of
// spans with the classic gaps that widen toward its base; mountains are
// two layers of value-noise ridges; the floor is a perspective grid
// (depth = k / rows below the horizon), a shimmering ocean with the sun's
// broken reflection, or rolling dunes.
//
// Presets: k0 Outrun · k1 Ocean Drive · k2 Night Grid · k3 Desert · k4 Neon City ·
//          k5 Lagoon · k6 Retro · k7 Midnight · k8 Palm Beach · k9 Velvet Hills ·
//          k10 Lake · k11 Tour
//   k12 — BOOST: put your foot down for a few seconds
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (sun & scene size) · p3 Colour Flow · p4 Hue Spread
//   p5 Sun · p6 Horizon · p7 Mountains · p8 Cruise · p9 Stripes · p10 Haze
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8SU_GRID, H8SU_WATER, H8SU_DUNES };
enum { H8SU_F_SUN = 1, H8SU_F_MOON = 2, H8SU_F_MOUNT = 4, H8SU_F_CITY = 8, H8SU_F_PALMS = 16, H8SU_F_STARS = 32 };
struct H8SuLook { uint8_t floor, flags, sun, hz, mount, cruise, stripes, haze, pal, flow, spread; int8_t key; };
static const H8SuLook h8su_looks[11] = {
  // floor       flags                                                   sun hz mnt cru str haze pal          flow spr key
  { H8SU_GRID,  H8SU_F_SUN | H8SU_F_MOUNT,                                60, 55, 50, 50, 60, 60, V8P_SUNSET,   1,  60,  0 },   // Outrun
  { H8SU_WATER, H8SU_F_SUN | H8SU_F_PALMS,                                55, 58,  0, 40, 50, 55, V8P_DUSK,     1,  60,  0 },   // Ocean Drive
  { H8SU_GRID,  H8SU_F_MOON | H8SU_F_MOUNT | H8SU_F_STARS,                40, 55, 55, 45,  0, 45, V8P_INFRARED,   1,  60,  0 },   // Night Grid
  { H8SU_DUNES, H8SU_F_SUN,                                               80, 52,  0, 35, 40, 60, V8P_EMBER,    1,  50,  0 },   // Desert
  { H8SU_GRID,  H8SU_F_SUN | H8SU_F_CITY | H8SU_F_STARS,                  50, 58, 50, 50, 60, 55, V8P_VAPOR,    1,  70,  0 },   // Neon City
  { H8SU_WATER, H8SU_F_MOON | H8SU_F_PALMS | H8SU_F_STARS,                40, 56,  0, 35,  0, 45, V8P_MIAMI,   1,  60,  0 },   // Lagoon
  { H8SU_GRID,  H8SU_F_SUN,                                               85, 54,  0, 45, 75, 65, V8P_PEACH,    1,  60,  0 },   // Retro
  { H8SU_GRID,  H8SU_F_MOON | H8SU_F_MOUNT | H8SU_F_STARS,                35, 52, 65, 40,  0, 40, V8P_AURORA,   1,  70,  0 },   // Midnight
  { H8SU_WATER, H8SU_F_SUN | H8SU_F_PALMS,                                65, 55,  0, 40, 55, 60, V8P_RETROPOP,  1,  60,  0 },   // Palm Beach
  { H8SU_DUNES, H8SU_F_SUN | H8SU_F_MOUNT,                                60, 54, 60, 35, 45, 55, V8P_VELVET,   1,  60,  0 },   // Velvet Hills
  { H8SU_WATER, H8SU_F_MOON | H8SU_F_MOUNT | H8SU_F_STARS,                40, 55, 60, 30,  0, 45, V8P_FOREST,   1,  60,  0 },   // Lake
};
static const char* const h8su_names[12] = { "Outrun", "Ocean Drive", "Night Grid", "Desert", "Neon City", "Lagoon", "Retro", "Midnight", "Palm Beach", "Velvet Hills", "Lake", "Tour" };
static const char* const h8su_lbl[6] = { "Sun", "Horizon", "Mountains", "Cruise", "Stripes", "Haze" };
const char* prog_ghsunset_name() { return "SUNSET"; }
const char* prog_ghsunset_character() { return "Endless synthwave drive: striped sun, neon grid, palms, ocean and mountains"; }
const char* prog_ghsunset_presetName(int p) { return (p >= 0 && p < 12) ? h8su_names[p] : NULL; }
const char* prog_ghsunset_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8su_lbl); }
uint8_t prog_ghsunset_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static float h8su_sun = 0.6f, h8su_hz = 0.55f, h8su_mount = 0.5f, h8su_cruise = 0.5f, h8su_str = 0.5f, h8su_haze = 0.5f;
static float h8su_z = 0, h8su_t = 0, h8su_boost = 0, h8su_pan = 0;
static uint32_t h8su_seed = 11;
void prog_ghsunset_init() { v8_init(); h8su_boost = 0; h8su_seed = 1 + v3_rnd(1000); }

// a row of one level in 1/16 steps, dithered (Bayer), written 4 px at a time
static void h8su_row(int y, int lev16, int hue) {
  if ((unsigned)y >= H) return;
  const uint8_t* bay = v8_bayer + ((y & 3) << 2);
  uint32_t w = 0;
  for (int k = 0; k < 4; k++) { int l = v3_clamp((lev16 + bay[k]) >> 4, 0, 15); w |= (uint32_t)((l << 4) | hue) << (k * 8); }
  uint32_t* p = (uint32_t*)(v3_buf + y * W);
  for (int i = 0; i < W / 4; i++) p[i] = w;
}
static inline void h8su_max(int x, int y, int lev, int hue) { v3_putMax(x, y, lev, hue); }
// a palm tree: a curved trunk and drooping fronds, in near-black
static void h8su_palm(int bx, int by, int h, int lean, float sway, uint8_t c) {
  int tx = bx, ty = by;
  for (int s = 1; s <= 12; s++) {
    int nx = bx + (lean * s * s) / 144 + (int)(sway * s * 0.4f), ny = by - h * s / 12;
    v8_thick(tx, ty, nx, ny, 3 - s / 6, c); tx = nx; ty = ny;
  }
  for (int f = 0; f < 7; f++) {
    float a = -2.9f + f * 0.48f + sway * 0.05f;
    int px = tx, py = ty;
    for (int s = 1; s <= 8; s++) {
      float d = s * h * 0.075f;
      int nx = tx + (int)(cosf(a) * d), ny = ty + (int)(sinf(a) * d * 0.7f + s * s * h * 0.0035f);
      v8_thick(px, py, nx, ny, s < 5 ? 2 : 1, c); px = nx; py = ny;
    }
  }
}

void prog_ghsunset_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8SuLook& L = h8su_looks[look];
  float rate = v8_rate();
  v6_ease(&h8su_sun, v8_kn(5, L.sun / 100.0f), rate);
  v6_ease(&h8su_hz, v8_kn(6, L.hz / 100.0f), rate);
  v6_ease(&h8su_mount, v8_kn(7, L.mount / 100.0f), rate);
  v6_ease(&h8su_cruise, v8_kn(8, L.cruise / 100.0f), rate);
  v6_ease(&h8su_str, v8_kn(9, L.stripes / 100.0f), rate);
  v6_ease(&h8su_haze, v8_kn(10, L.haze / 100.0f), rate);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 140, L.key / 100.0f);
  if (v6_k12hit) h8su_boost = 1.0f;
  h8su_boost *= 1.0f - v3_clampf(v6_f.dt * 0.35f, 0.0f, 1.0f);
  float drive = (0.05f + h8su_cruise * h8su_cruise * 4.0f) * (1.0f + 3.0f * h8su_boost);
  h8su_z += v6_f.sdt * drive; if (h8su_z > 10000.0f) h8su_z -= 10000.0f;
  h8su_t += v6_f.sdt; if (h8su_t > 10000.0f) h8su_t -= 10000.0f;
  h8su_pan += v6_f.sdt * drive * 3.0f;
  float sc = v8_zoom(1.0f);
  int hz = (int)(H * (0.35f + 0.35f * h8su_hz) + 2.0f * sinf(h8su_t * 0.2f));
  hz = v3_clamp(hz, 40, H - 30);
  const int cx = W / 2;

  // ── sky: hue 0 at the top … 6 at the horizon, brighter toward it ──
  int haze = (int)(h8su_haze * 180);
  for (int y = 0; y < hz; y++) {
    int f = y * 256 / hz;                                            // 0 top … 256 horizon
    int hue = (f * 6) >> 8;
    int lev16 = 10 + ((f * f) >> 8) * (60 + haze) / 100;
    h8su_row(y, lev16, hue);
  }
  if (L.flags & H8SU_F_STARS) {
    for (int s = 0; s < 70; s++) {
      uint32_t h = v3_hash(h8su_seed + s * 7919u);
      int x = (int)(h % W), y = (int)((h >> 9) % (hz * 3 / 4 + 1));
      int tw = (v3_sin((int)(h >> 20) + v8_frameNo * (1 + (int)(h & 3))) + 16384) >> 13;
      h8su_max(x, y, 4 + tw + (int)((h >> 5) & 3), (int)((h >> 3) & 7) + 8);
    }
  }
  // ── sun or moon ──
  int R = (int)((10 + h8su_sun * 90) * sc), sy = hz - (int)(R * 0.45f);
  if (L.flags & H8SU_F_MOON) { R = (int)((12 + h8su_sun * 22) * sc); sy = hz - R - 30 - (int)(h8su_sun * 20); }
  if (L.flags & (H8SU_F_SUN | H8SU_F_MOON)) {
    bool moon = L.flags & H8SU_F_MOON;
    int gr = R * 2 + 8, gr2 = gr * gr, peak = 110 + haze;            // a soft dithered glow round it
    for (int dy = -gr; dy <= gr; dy++) {
      int y = sy + dy; if (y < 0 || y >= hz) continue;
      int w = (int)v3_isqrt((uint32_t)(gr2 - dy * dy));
      const uint8_t* bay = v8_bayer + ((y & 3) << 2);
      int x0 = v3_clamp(cx - w, 0, W - 1), x1 = v3_clamp(cx + w, 0, W - 1);
      for (int x = x0; x <= x1; x++) {
        int d2 = (x - cx) * (x - cx) + dy * dy;
        int q = 256 - d2 * 256 / gr2;                                 // 0 … 256
        int l16 = (q * q >> 8) * peak >> 8;
        h8su_max(x, y, (l16 + bay[x & 3]) >> 4, moon ? 12 : 8);
      }
    }
    int period = 12, scroll = (int)(h8su_t * 6) % period;
    for (int dy = -R; dy <= R; dy++) {
      int y = sy + dy; if (y < 0 || y >= hz) continue;
      int w = (int)v3_isqrt((uint32_t)(R * R - dy * dy));
      int f = (dy + R) * 256 / (2 * R + 1);                          // 0 top … 256 bottom
      int vis = hz - sy, dy0 = -R / 2;                               // the gaps widen toward the horizon
      if (!moon && dy > dy0 && h8su_str > 0.05f) {
        int gap = (int)(((dy - dy0) * period * h8su_str) / (vis - dy0 + 1)) + 1;
        if (((dy + scroll) % period + period) % period < gap) continue;
      }
      int hue = moon ? 12 + (f >> 7) : 10 - ((f * 5) >> 8);
      int lev = moon ? 12 : 15 - (f >> 7);
      v3_hspan(cx - w, cx + w, y, (uint8_t)((lev << 4) | (hue & 15)));
    }
    if (moon) {                                                       // a few soft craters
      v8_ellipse(cx - R / 3, sy - R / 4, R / 5, R / 6, (uint8_t)((10 << 4) | 12));
      v8_ellipse(cx + R / 4, sy + R / 5, R / 6, R / 7, (uint8_t)((10 << 4) | 12));
    }
  }
  // ── mountains: two ridges of value noise, or a city skyline ──
  if ((L.flags & H8SU_F_MOUNT) && h8su_mount > 0.02f) {
    for (int layer = 0; layer < 2; layer++) {
      int amp = (int)((layer ? 45 : 80) * h8su_mount * sc + 4);
      uint32_t sd = h8su_seed + layer * 777u;
      int pan = (int)(h8su_pan * (layer ? 2.5f : 1.0f) * 16);
      uint8_t body = (uint8_t)(((layer ? 1 : 2) << 4) | (layer ? 0 : 1)), rim = (uint8_t)(((layer ? 6 : 4) << 4) | 3);
      for (int x = 0; x < W; x++) {
        uint32_t u = (uint32_t)(x * 16 + pan + 100000);
        int32_t n1 = v3_noise1(u * 3 / 2, sd), n2 = v3_noise1(u * 5, sd ^ 0x55u);
        int hgt = (int)(((n1 + 16384) * amp >> 15) + ((n2 + 16384) * amp >> 17));
        int top = hz - hgt - (layer ? 0 : 6);
        if (top < hz) { for (int y = top + 1 < 0 ? 0 : top + 1; y < hz; y++) v3_buf[y * W + x] = body; if (top >= 0) v3_buf[top * W + x] = rim; }
      }
    }
  }
  if (L.flags & H8SU_F_CITY) {
    int pan = (int)(h8su_pan * 4);
    for (int b = -1; b < 24; b++) {
      int bi = b + pan / 18;
      uint32_t h = v3_hash((uint32_t)bi * 2654435761u + h8su_seed);
      int bx = b * 18 - (pan % 18), bw = 12 + (int)(h % 10), bh = (int)((20 + (h >> 8) % 60) * (0.5f + h8su_mount) * sc);
      for (int y = hz - bh; y < hz; y++) v3_hspan(bx, bx + bw, y, (uint8_t)((1 << 4) | 1));
      for (int wy = hz - bh + 4; wy < hz - 3; wy += 5) for (int wx = bx + 2; wx < bx + bw - 2; wx += 4) {
        uint32_t lit = v3_hash(h + wy * 31u + wx * 7u);
        if ((lit & 7) < 3) { int l = 6 + (int)((lit >> 4) & 3) + ((lit >> 8) % 97 == (uint32_t)(v8_frameNo / 30 % 97) ? -4 : 0); v3_put(wx, wy, (uint8_t)((l << 4) | (8 + (lit >> 12) % 4))); v3_put(wx + 1, wy, (uint8_t)((l << 4) | (8 + (lit >> 12) % 4))); }
      }
    }
  }
  // ── the floor ──
  for (int y = hz; y < H; y++) {
    int dz = y - hz + 1;
    int fog = 256 - v3_clamp(dz * 256 / 50, 0, 256);                  // near the horizon: hazy
    if (L.floor == H8SU_GRID) {
      int lev16 = 16 + (fog * (40 + haze)) / 256;
      h8su_row(y, lev16, 1);
      // depth in grid cells = 150 / dz (camera height ½ cell, focal 300 px): square cells
      int32_t zz = (150 << 8) / dz + (int32_t)(h8su_z * 256 * 1.5f);
      int fr = (int)(zz & 255), d = fr < 128 ? fr : 256 - fr;           // distance to the nearest cross line, 1/256 cell
      int dist16 = (int)((int32_t)d * dz * dz / 2400);                  // … in 1/16 rows (a cell is dz²/150 rows tall)
      int hl = dist16 < 20 ? 15 - dist16 * 10 / 20 : 0;
      if (hl > 0) { uint8_t* row = v3_buf + y * W; uint8_t c = (uint8_t)((v3_clamp(hl - (fog >> 6), 3, 15) << 4) | 2); for (int x = 0; x < W; x++) if ((row[x] >> 4) < hl) row[x] = c; }
      int sp = dz * 2 + 2;                                             // the lines running to the horizon
      int off = (int)(((int)(h8su_pan * 2) % 40) * sp / 40);
      for (int n = -(W / sp) - 2; n <= W / sp + 2; n++) {
        int x = cx + n * sp + off;
        int l = 11 - (fog >> 5);
        h8su_max(x, y, l, 2); h8su_max(x - 1, y, l / 2, 2); h8su_max(x + 1, y, l / 2, 2);
      }
    } else if (L.floor == H8SU_WATER) {
      int lev16 = 12 + (fog * (30 + haze)) / 256 + (dz > 60 ? 0 : 0);
      h8su_row(y, lev16, 4);
      // the sun's broken reflection: dashes of light that shimmer
      int rw = (int)(R * (0.6f + 0.6f * dz / 80.0f));
      uint32_t slot = (uint32_t)(h8su_t * 3);
      for (int sgm = 0; sgm < 5; sgm++) {
        uint32_t hh = v3_hash((uint32_t)y * 977u + slot * 131u + sgm * 7u);
        if ((hh & 3) == 0) continue;
        int w = 2 + (int)(hh >> 8) % (rw / 2 + 2), x0 = cx - rw + (int)((hh >> 16) % (2 * rw + 1));
        int wob = (v3_sin(y * 37 + (int)(h8su_t * 150)) * 4) >> 14;
        int lv = (L.flags & H8SU_F_MOON) ? 9 : 12 - (dz > 40 ? 2 : 0);
        for (int x = x0 + wob; x < x0 + wob + w; x++) h8su_max(x, y, lv, (L.flags & H8SU_F_MOON) ? 12 : 9);
      }
      if (((y + v8_frameNo / 8) % 9) == 0) { uint32_t hh = v3_hash(y * 31u + slot); int x0 = (int)(hh % W); for (int x = x0; x < x0 + 6 + (int)(hh >> 24) % 12; x++) h8su_max(x, y, 5, 3); }
    } else {
      int32_t Z = (150 << 8) / dz;
      int ph = (int)((Z * 4) + (int)(h8su_z * 1500));
      int band = (v3_sin(ph + ((v3_sin(y * 13) * 40) >> 14)) + 16384) >> 10;   // 0 … 32
      int lev16 = 24 + band * 2 + (fog * (40 + haze)) / 256;
      h8su_row(y, lev16, 5 + (band >> 4));
    }
  }
  if (L.flags & H8SU_F_PALMS) {
    float sway = 2.0f * sinf(h8su_t * 0.7f);
    uint8_t c = (uint8_t)((1 << 4) | 0);
    h8su_palm(30, H + 4, (int)(150 * sc), 30, sway, c);
    h8su_palm(W - 40, H + 4, (int)(120 * sc), -26, -sway * 0.8f, c);
    h8su_palm(W - 75, H + 8, (int)(95 * sc), -12, sway * 0.6f, c);
  }
  v8_end();
}

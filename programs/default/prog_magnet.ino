// =====================================================================
// PROGRAM: MAGNET — Nam June Paik's Magnet TV (by Dewey) — Set 7, slot 3
//
// In 1965 Nam June Paik put a big horseshoe magnet on top of a television.
// The magnet bent the electron beam, and the picture — just the empty
// raster of scan lines, or whatever was on — twisted into ribbons, loops
// and whirlpools that moved as the magnet moved. Anyone with an old TV
// and a magnet has seen it; Paik made it art.
//
// How it works: a picture is made once (scan lines, colour bars, a test
// card, a grid, rings, Paik's name, static) at half size. Each frame the
// magnets' pull and twist are worked out on a coarse grid (every 16 px),
// and every pixel fetches its colour from where the beam has been bent to,
// blending the bend smoothly between grid points.
//
// Presets: k0 Magnet TV · k1 Horseshoe · k2 Bars · k3 Test Card · k4 Grid ·
//          k5 Whirlpool · k6 Paik · k7 Static · k8 Ribbons · k9 Breath ·
//          k10 Stripes · k11 Tour
//   k12 — DEGAUSS: the picture shudders and swims back into shape, like an old set switched on
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Strength (◄ push | pull ►) · p6 Twist (◄ | ►) · p7 Reach · p8 Magnets (1-3)
//   p9 Wander · p10 Picture · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define MG_SW 160
#define MG_SH 120
#define MG_GX (W / 16 + 1)
#define MG_GY (H / 16 + 1)
enum { MG_RASTER, MG_BARS, MG_CARD, MG_GRID, MG_RINGS, MG_STRIPES, MG_TEXT, MG_STATIC, MG_NSRC };
struct MgLook { uint8_t src, n; int8_t str, twist; uint8_t reach, wander, breath, pal, flow, spread, scan; };
static const MgLook mg_looks[11] = {
  // src         n  str twist reach wander breath pal             flow spr scan
  { MG_RASTER,  1,  70,  30,  50,  30,   0, V7P_PHOSPHOR,     2,  30, 2 },   // Magnet TV
  { MG_RASTER,  2,  80,  10,  40,  25,   0, V7P_VASULKA,      3,  50, 2 },   // Horseshoe
  { MG_BARS,    1,  65,  25,  55,  35,   0, V7P_STUDIO,       0, 100, 2 },   // Bars
  { MG_CARD,    2,  55,  35,  45,  30,   0, V7P_GHOST,        2,  60, 2 },   // Test Card
  { MG_GRID,    3,  50,  20,  40,  40,   0, V7P_BLACKLIGHT,   4,  80, 1 },   // Grid
  { MG_RINGS,   1,  40,  90,  65,  20,   0, V7P_CHICAGO,      5,  90, 2 },   // Whirlpool
  { MG_TEXT,    1,  70,  40,  55,  30,   0, V7P_PAIK,         4,  90, 2 },   // Paik
  { MG_STATIC,  2,  70,  30,  50,  30,   0, V7P_GHOST,        1,  40, 2 },   // Static
  { MG_RASTER,  3,  90,  60,  45,  45,   0, V7P_SCANIMATE,    6, 100, 1 },   // Ribbons
  { MG_BARS,    1,  80,  20,  60,  15,  80, V7P_STUDIO,       0, 100, 2 },   // Breath
  { MG_STRIPES, 2,  60,  70,  50,  35,   0, V7P_SUNSET,       5,  90, 2 },   // Stripes
};
static const char* const mg_names[12] = { "Magnet TV", "Horseshoe", "Bars", "Test Card", "Grid", "Whirlpool", "Paik", "Static", "Ribbons", "Breath", "Stripes", "Tour" };
static const char* const mg_lbl[6] = { "Strength: Push|Pull", "Twist", "Reach", "Magnets", "Wander", "Picture" };
const char* prog_magnet_name() { return "MAGNET"; }
const char* prog_magnet_character() { return "Paik's Magnet TV: magnets bend the picture into ribbons and whirlpools"; }
const char* prog_magnet_presetName(int p) { return (p >= 0 && p < 12) ? mg_names[p] : NULL; }
const char* prog_magnet_potLabel(int p, int pot) { (void)p; return v7_label(pot, mg_lbl); }
uint8_t prog_magnet_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the picture (160 × 120) and the bend at each grid point (1/16 px)
static uint8_t* mg_pic; static int16_t* mg_du; static int16_t* mg_dv;
static void mg_mem() { mg_pic = v6_mem; mg_du = (int16_t*)(v6_mem + MG_SW * MG_SH); mg_dv = mg_du + MG_GX * MG_GY; }

static int mg_madeSrc = -1;
static float mg_str, mg_twist, mg_reach, mg_n, mg_wander, mg_ph = 0, mg_degauss = 0;

void prog_magnet_init() { mg_mem(); v6_init(); mg_madeSrc = -1; mg_degauss = 0; }

static void mg_make(int src) {
  uint8_t* p = mg_pic;
  for (int y = 0; y < MG_SH; y++) for (int x = 0; x < MG_SW; x++) {
    int dx = x - MG_SW / 2, dy = y - MG_SH / 2, c = 0;
    switch (src) {
      case MG_RASTER: c = (y & 1) ? 0 : V6C(13, (y * 16 / MG_SH) & 15); break;
      case MG_BARS: {
        int b = x * 7 / MG_SW;
        if (y < MG_SH * 2 / 3) c = V6C(14, b * 2);
        else if (y < MG_SH * 3 / 4) c = V6C(b & 1 ? 2 : 12, (12 - b * 2) & 15);
        else c = x < MG_SW * 5 / 7 ? V6C(x * 4 / MG_SW * 4 + 1, 15) : V6C(1, 0);
      } break;
      case MG_CARD: {
        int r = (int)v3_isqrt((uint32_t)(dx * dx * 4 + dy * dy * 4));
        bool grid = (x % 16) == 0 || (y % 16) == 0;
        if (r > 96 && r < 104) c = V6C(15, 4);
        else if (r < 96) c = (dy < -20) ? V6C(12, (x * 7 / MG_SW) * 2) : (dy > 20 ? V6C(2 + (x * 13 / MG_SW), 10) : V6C(9, 6));
        else c = grid ? V6C(14, 8) : V6C(3, 8);
        if (v3_abs(dx) < 1 || v3_abs(dy) < 1) c = V6C(15, 0);
      } break;
      case MG_GRID: c = ((x % 12) < 2 || (y % 12) < 2) ? V6C(14, ((x / 12) + (y / 12)) & 15) : 0; break;
      case MG_RINGS: { int r = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy)); c = ((r / 5) & 1) ? V6C(13, (r / 10) & 15) : V6C(2, (r / 10 + 8) & 15); } break;
      case MG_STRIPES: c = V6C(((x / 6) & 1) ? 14 : 5, (x * 16 / MG_SW) & 15); break;
      case MG_TEXT: c = (y & 1) ? 0 : V6C(4, 9); break;                 // a raster behind the letters
      default: { uint32_t r = v3_rand(); c = V6C((r >> 8) & 15, (r >> 16) & 1 ? 0 : 8); }
    }
    *p++ = (uint8_t)c;
  }
  if (src == MG_TEXT) {                                                 // TV in big letters
    const char* s = "TV"; int sc = 11, tw = 2 * 6 * sc - sc, x0 = (MG_SW - tw) / 2, y0 = (MG_SH - 7 * sc) / 2;
    for (int i = 0; s[i]; i++) {
      const uint8_t* g = v3_font[v3_glyph(s[i])];
      for (int r = 0; r < 7; r++) for (int b = 0; b < 5; b++) if (g[r] & (16 >> b))
        for (int yy = 0; yy < sc; yy++) for (int xx = 0; xx < sc; xx++)
          mg_pic[(y0 + r * sc + yy) * MG_SW + x0 + (i * 6 + b) * sc + xx] = V6C(15, (i * 6 + 2) & 15);
    }
  }
}

// one row: grid bends left/right of each 16-px cell are interpolated across it
__attribute__((optimize("O2"))) static void mg_row(uint8_t* out, int y, const int32_t* rdu, const int32_t* rdv, int32_t zoomK) {
  const uint8_t* pic = mg_pic;
  int32_t yc = (y - HALFH) * 16;
  for (int cx = 0; cx < W / 16; cx++) {
    int32_t du = rdu[cx], dv = rdv[cx];
    int32_t ddu = (rdu[cx + 1] - du) >> 4, ddv = (rdv[cx + 1] - dv) >> 4;
    int32_t xc = (cx * 16 - HALFW) * 16;
    uint8_t* o = out + cx * 16;
    for (int i = 0; i < 16; i++, du += ddu, dv += ddv, xc += 16) {
      // source in 1/16 screen px around the middle → half-size picture
      int32_t sx = ((((xc + du) * zoomK) >> 12) >> 5) + MG_SW / 2, sy = ((((yc + dv) * zoomK) >> 12) >> 5) + MG_SH / 2;
      o[i] = ((uint32_t)sx < MG_SW && (uint32_t)sy < MG_SH) ? pic[sy * MG_SW + sx] : 0;
    }
  }
}

void prog_magnet_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  mg_mem();
  const MgLook& L = mg_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&mg_str, v7_kb(5, L.str / 100.0f), rate);
  v6_ease(&mg_twist, v7_kb(6, L.twist / 100.0f), rate);
  v6_ease(&mg_reach, v7_kn(7, L.reach / 100.0f), rate);
  v6_ease(&mg_n, v3_centre(8) ? (float)L.n : 1.0f + v3_u(8) * 2.0f, rate);
  v6_ease(&mg_wander, v7_kn(9, L.wander / 100.0f), rate);
  int src = v3_centre(10) ? L.src : v3_clamp(v3_k(10) * MG_NSRC / 1024, 0, MG_NSRC - 1);
  if (src != mg_madeSrc || src == MG_STATIC) { mg_make(src); mg_madeSrc = src; }
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 80);

  float sdt = v6_f.sdt, t = v6_t;
  mg_ph += sdt * (0.15f + mg_wander * 1.2f);
  if (v6_k12hit) mg_degauss = 1.0f;
  float S = mg_str;
  if (L.breath) S *= 0.35f + 0.65f * (0.5f + 0.5f * sinf(t * 0.9f));
  float shake = 0;
  if (mg_degauss > 0) { shake = mg_degauss * mg_degauss * sinf(t * 31.0f) * 2.5f; mg_degauss -= v3_dt * 0.45f; if (mg_degauss < 0) mg_degauss = 0; }

  // ── the magnets: where they are, and how strong ──
  int nm = (int)(mg_n + 0.5f); if (nm < 1) nm = 1; if (nm > 3) nm = 3;
  int32_t mx[3], my[3], ms[3], mR2[3];
  float R = 18.0f + mg_reach * 90.0f;
  for (int m = 0; m < nm; m++) {
    float a = mg_ph * (1.0f + 0.31f * m) + m * 2.1f, b = mg_ph * (0.73f + 0.17f * m) + m * 1.3f;
    float spread = nm == 1 ? 0.55f : 0.75f;
    mx[m] = (int32_t)(sinf(a) * HALFW * spread);
    my[m] = (int32_t)(cosf(b) * HALFH * spread);
    if (L.n == 2 && nm == 2 && look == 1) { mx[m] = (int32_t)((m ? 40 : -40) + 30 * sinf(mg_ph)); my[m] = (int32_t)(30 * cosf(mg_ph * 0.8f)); }   // a horseshoe: two poles
    float sgn = (m & 1) && L.n >= 2 ? -1.0f : 1.0f;
    ms[m] = (int32_t)((S * sgn + shake) * 4000);                        // pull: the picture's scale at the magnet, ×4096 extra
    mR2[m] = (int32_t)(R * R);
  }
  int32_t tw = (int32_t)(mg_twist * 900);                               // whirl at the very middle, 1024 = a whole turn
  // ── the bend at every grid point (1/16 px): source = here + bend ──
  // near a magnet the beam is pulled (scaled about it) and whirled (turned about it), less and less further out
  for (int gy = 0; gy < MG_GY; gy++) for (int gx = 0; gx < MG_GX; gx++) {
    int32_t px = gx * 16 - HALFW, py = gy * 16 - HALFH, du = 0, dv = 0;
    for (int m = 0; m < nm; m++) {
      int32_t vx = px - mx[m], vy = py - my[m];
      int32_t r2 = vx * vx + vy * vy;
      int32_t near = (mR2[m] << 4) / ((r2 >> 8) + (mR2[m] >> 8) + 1);          // 4096 at the magnet → 0 far away
      near = (near * near) >> 12;                                            // (squared: the bend stays local)
      int32_t sc = 4096 + ((ms[m] * near) >> 12); if (sc < 600) sc = 600;
      int a = (int)((tw * near) >> 12);
      int32_t c = v3_cos(a), s2 = v3_sin(a);                                // Q14
      int32_t rx = (vx * c - vy * s2) >> 14, ry = (vx * s2 + vy * c) >> 14;
      du += ((rx * sc) >> 8) - vx * 16; dv += ((ry * sc) >> 8) - vy * 16;
    }
    mg_du[gy * MG_GX + gx] = (int16_t)v3_clamp(du, -30000, 30000);
    mg_dv[gy * MG_GX + gx] = (int16_t)v3_clamp(dv, -30000, 30000);
  }
  // ── every pixel ──
  int32_t zoomK = (int32_t)(4096 / v6_zoom(1.0f));
  int32_t rdu[MG_GX], rdv[MG_GX];
  for (int y = 0; y < H; y++) {
    int gy = y >> 4, fy = y & 15;
    const int16_t *u0 = mg_du + gy * MG_GX, *v0 = mg_dv + gy * MG_GX;
    const int16_t *u1 = gy + 1 < MG_GY ? u0 + MG_GX : u0, *v1 = gy + 1 < MG_GY ? v0 + MG_GX : v0;
    for (int gx = 0; gx < MG_GX; gx++) { rdu[gx] = u0[gx] + (((u1[gx] - u0[gx]) * fy) >> 4); rdv[gx] = v0[gx] + (((v1[gx] - v0[gx]) * fy) >> 4); }
    mg_row(v3_buf + y * W, y, rdu, rdv, zoomK);
  }
  v7_end(L.scan);
}

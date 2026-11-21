// =====================================================================
// PROGRAM: SLITSCAN — the Stargate corridor (by Dewey) — Set 7, slot 11
//
// For the Stargate in "2001: A Space Odyssey" (1968), Douglas Trumbull
// moved a camera toward a narrow slit, shutter open, while lit artwork slid
// past behind it — one frame took a minute — and the artwork smeared into
// two endless walls of light rushing past at impossible speed. Slit-scan
// went on to make Doctor Who's 1970s time tunnel and a hundred TV titles.
//
// How it works: the walls (or floor and ceiling, or a box of four) are
// flat planes in perspective. Down a wall, every column is one depth; on a
// floor, every row is: so each pixel is a texture lookup with one step
// along the row. The texture (streaks, a grid, lights, stripes, plasma,
// colour lanes, circuitry) streams toward you, fading into the distance.
//
// Presets: k0 Stargate · k1 Corridor · k2 Runway · k3 Hyperspace · k4 Lanes ·
//          k5 Op Tunnel · k6 Plasma · k7 Grid Floor · k8 Jupiter · k9 Circuit ·
//          k10 Spectral · k11 Tour
//   k12 — WARP: hold for a burst of speed
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Flight (◄ back | forward ►) · p6 Stretch · p7 Fog · p8 Layout
//   p9 Texture · p10 Swing · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { SX_WALLS, SX_FLOORCEIL, SX_FLOOR, SX_BOX, SX_NLAY };
enum { SX_STREAKS, SX_GRID, SX_LIGHTS, SX_STRIPES, SX_PLASMA, SX_LANES, SX_CIRCUIT, SX_NTEX };
struct SxLook { uint8_t lay, tex; int8_t flight; uint8_t stretch, fog, swing, pal, flow, spread, scan; };
static const SxLook sx_looks[11] = {
  // layout        texture      flight str fog swing pal              flow spr scan
  { SX_WALLS,     SX_STREAKS,   60,  70, 50,  30, V7P_CHICAGO,      5, 100, 2 },   // Stargate
  { SX_BOX,       SX_GRID,      40,  40, 55,  20, V7P_VASULKA,      3,  70, 2 },   // Corridor
  { SX_FLOOR,     SX_LIGHTS,    45,  50, 45,  15, V7P_SUNSET,       4,  90, 2 },   // Runway
  { SX_BOX,       SX_STREAKS,   95,  95, 40,  35, V7P_BLACKLIGHT,   6, 100, 2 },   // Hyperspace
  { SX_FLOORCEIL, SX_LANES,     60,  60, 50,  25, V7P_PAIK,         4, 100, 2 },   // Lanes
  { SX_BOX,       SX_STRIPES,   40,  30, 60,  25, V7P_GHOST,        0,  30, 1 },   // Op Tunnel
  { SX_WALLS,     SX_PLASMA,    35,  40, 45,  30, V7P_LUMIA,        5, 100, 2 },   // Plasma
  { SX_FLOOR,     SX_GRID,      50,  40, 40,  20, V7P_SCANIMATE,    4, 100, 2 },   // Grid Floor
  { SX_FLOORCEIL, SX_PLASMA,    55,  70, 45,  30, V7P_KODACHROME,   5, 100, 2 },   // Jupiter
  { SX_BOX,       SX_CIRCUIT,   45,  45, 55,  20, V7P_PHOSPHOR,     3,  60, 2 },   // Circuit
  { SX_WALLS,     SX_LANES,     70,  80, 45,  30, V7P_STUDIO,       5, 100, 2 },   // Spectral
};
static const char* const sx_names[12] = { "Stargate", "Corridor", "Runway", "Hyperspace", "Lanes", "Op Tunnel", "Plasma", "Grid Floor", "Jupiter", "Circuit", "Spectral", "Tour" };
static const char* const sx_lbl[6] = { "Flight: Back|Forward", "Stretch", "Fog", "Layout", "Texture", "Swing" };
const char* prog_slitscan_name() { return "SLITSCAN"; }
const char* prog_slitscan_character() { return "The 2001 Stargate: walls of light rushing past"; }
const char* prog_slitscan_presetName(int p) { return (p >= 0 && p < 12) ? sx_names[p] : NULL; }
const char* prog_slitscan_potLabel(int p, int pot) { (void)p; return v7_label(pot, sx_lbl); }
uint8_t prog_slitscan_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the texture (64 along × 64 across), per-column depth tables for the walls
#define SX_T 64
static uint8_t* sx_tex; static int32_t* sx_cu; static int32_t* sx_ck; static uint8_t* sx_cf;
static void sx_mem() { sx_tex = v6_mem; sx_cu = (int32_t*)(v6_mem + 4096); sx_ck = sx_cu + W; sx_cf = (uint8_t*)(sx_ck + W); }
static int sx_madeTex = -1;
static float sx_travel = 0, sx_flight, sx_stretch, sx_fog, sx_swing, sx_warp = 0, sx_sph = 0;

void prog_slitscan_init() { sx_mem(); v6_init(); sx_madeTex = -1; sx_warp = 0; }

// texture: u (0 … 63) runs along the flight, v (0 … 63) across it
static void sx_make(int tex) {
  for (int v = 0; v < SX_T; v++) {
    uint32_t hv = v3_hash(v * 2654435761u + tex * 97u);
    for (int u = 0; u < SX_T; u++) {
      int c = 0;
      switch (tex) {
        case SX_STREAKS: {                                                 // dashes of light, each lane its own colour and rhythm
          int len = 8 + (hv & 31), on = 3 + ((hv >> 5) & 15), off = (hv >> 10) & 63;
          bool lit = ((u + off) % len) < on && ((hv >> 16) & 3);
          c = lit ? V6C(9 + ((u + off) % len) * 6 / on, (hv >> 20) & 15) : V6C(1, (hv >> 20) & 15);
        } break;
        case SX_GRID: c = ((u & 15) == 0 || (v & 15) == 0) ? V6C(15, ((u >> 4) + (v >> 4) * 3) & 15) : V6C(1, 8); break;
        case SX_LIGHTS: c = ((u & 7) < 2 && (v & 15) < 2) ? V6C(15, (v >> 4) * 4 + 2) : ((v & 15) == 8 ? V6C(4, 9) : 0); break;
        case SX_STRIPES: c = (u & 16) ? V6C(15, 0) : 0; break;
        case SX_PLASMA: { int s = v3_sin(u * 32 + v3_sin(v * 48) / 64) + v3_sin(v * 40 - u * 16); c = V6C(v3_clamp(8 + (s >> 12), 1, 15), ((s >> 11) + 16) & 15); } break;
        case SX_LANES: c = (v & 7) == 0 ? V6C(2, 0) : V6C(11 + ((u & 32) ? 2 : 0), (v >> 3) * 2 + 1); break;
        default: {                                                         // circuitry: traces and pads
          uint32_t hu = v3_hash(u * 7919u + (v >> 3) * 104729u);
          bool trace = (v & 7) == 3 && (hu & 3) != 0, via = (u & 7) == 3 && (v & 7) == 3 && (hu & 8);
          c = via ? V6C(15, 4) : trace ? V6C(10, 2) : (((u ^ v) & 31) == 0 ? V6C(6, 6) : V6C(1, 10));
        }
      }
      sx_tex[u * SX_T + v] = (uint8_t)c;
    }
  }
}
static inline uint8_t sx_fogged(uint8_t c, int f) { int l = (c >> 4) - f; return l > 0 ? (uint8_t)((l << 4) | (c & 15)) : 0; }

// a floor/ceiling row: depth fixed (texture row tu), across steps by dv per pixel (16.16 texels)
// (texture positions wrap: they are worked out in unsigned arithmetic, only the low bits matter)
__attribute__((optimize("O2"))) static void sx_floorRow(uint8_t* out, int x0, int x1, int tu, uint32_t v, uint32_t dv, int fog) {
  const uint8_t* row = sx_tex + (tu & (SX_T - 1)) * SX_T;
  for (int x = x0; x < x1; x++, v += dv) out[x] = sx_fogged(row[(v >> 16) & (SX_T - 1)], fog);
}

void prog_slitscan_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  sx_mem();
  const SxLook& L = sx_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&sx_flight, v7_kb(5, L.flight / 100.0f), rate);
  v6_ease(&sx_stretch, v7_kn(6, L.stretch / 100.0f), rate);
  v6_ease(&sx_fog, v7_kn(7, L.fog / 100.0f), rate);
  v6_ease(&sx_swing, v7_kn(10, L.swing / 100.0f), rate);
  int lay = v3_centre(8) ? L.lay : v3_clamp(v3_k(8) * SX_NLAY / 1024, 0, SX_NLAY - 1);
  int tex = v3_centre(9) ? L.tex : v3_clamp(v3_k(9) * SX_NTEX / 1024, 0, SX_NTEX - 1);
  if (tex != sx_madeTex) { sx_make(tex); sx_madeTex = tex; }
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 120);

  float sdt = v6_f.sdt;
  float w = v6_k12 ? 1.0f : 0.0f;
  sx_warp += (w - sx_warp) * v3_clampf(v3_dt * (v6_k12 ? 2.0f : 1.0f), 0, 1);
  sx_travel += sdt * sx_flight * 90.0f * (1.0f + sx_warp * 5.0f);          // texels per second
  if (sx_travel > 65536.0f) sx_travel -= 65536.0f; if (sx_travel < 0) sx_travel += 65536.0f;
  sx_sph += sdt * 0.35f;
  float z = v6_zoom(1.0f);
  // the vanishing point, swinging gently
  int vx = HALFW + (int)(sx_swing * 70 * sinf(sx_sph)), vy = HALFH + (int)(sx_swing * 45 * sinf(sx_sph * 0.77f + 1.0f));
  // A plane at distance h: a pixel d px from the vanishing line sees depth ∝ 1/d.
  //   along the flight: u = Lb / d + travel   ·   across: v = (other offset) · Ha / d   ·   fog = Fg / d
  int32_t Lb = (int32_t)(2048 * (0.25f + sx_stretch * 2.0f) * z);
  int32_t Ha = (int32_t)(48 * z * 65536);
  int32_t Fg = (int32_t)(sx_fog * 260);
  int trav = (int)sx_travel;
  bool box = lay == SX_BOX;
  if (lay == SX_WALLS || box) for (int x = 0; x < W; x++) {                // walls: everything per column
    int d = x - vx, ad = d < 0 ? -d : d; if (ad < 1) ad = 1;
    int32_t m = box ? 4 : 3;                                                   // (a box's walls stand a third further out)
    sx_cu[x] = Lb * m / 3 / ad + trav + (d < 0 ? 32 : 0);
    sx_ck[x] = Ha / 3 * m / ad;
    int f = Fg * m / 3 / ad; sx_cf[x] = (uint8_t)(f > 15 ? 15 : f);
  }
  for (int y = 0; y < H; y++) {
    uint8_t* out = v3_buf + y * W;
    int dy = y - vy, ady = dy < 0 ? -dy : dy; if (ady < 1) ady = 1;
    if (lay == SX_FLOOR && dy <= 0) {                                        // the sky above a lone floor: a glow on the horizon
      int g = 15 - ady / 5; memset(out, g > 0 ? V6C(g / 2, 12) : 0, W);
      continue;
    }
    int tu = 0, fog = 15; uint32_t v0 = 0, dv = 0;
    if (lay != SX_WALLS) {                                                     // floor or ceiling, for this row
      tu = Lb / ady + trav + (dy < 0 ? 32 : 0);
      dv = (uint32_t)(Ha / ady);
      v0 = (uint32_t)0 - dv * (uint32_t)vx;
      fog = Fg / ady; if (fog > 15) fog = 15;
    }
    if (lay == SX_FLOORCEIL || lay == SX_FLOOR) { sx_floorRow(out, 0, W, tu, v0, dv, fog); continue; }
    const uint8_t* frow = sx_tex + (tu & (SX_T - 1)) * SX_T;
    for (int x = 0; x < W; x++) {
      int dx = x - vx, adx = dx < 0 ? -dx : dx;
      if (box && ady * 4 > adx * 3) out[x] = sx_fogged(frow[((v0 + dv * (uint32_t)x) >> 16) & (SX_T - 1)], fog);
      else out[x] = sx_fogged(sx_tex[(sx_cu[x] & (SX_T - 1)) * SX_T + ((((uint32_t)dy * (uint32_t)sx_ck[x]) >> 16) & (SX_T - 1))], sx_cf[x]);
    }
  }
  v7_end(L.scan);
}

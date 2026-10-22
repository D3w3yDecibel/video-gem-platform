// =====================================================================
// PROGRAM: PORTAL — a flight down a hexagonal tunnel (by Dewey) — Set 6, slot 11
//
// The walls of a six-sided tunnel are tiled with SIREN's star lattice, and
// we fly down it forever toward a light at the end. The tunnel sways, turns
// and twists into a spiral; its tiles ripple and glow in bands of colour
// that rush past; it can be round or square instead, and lined with
// strapwork, honeycomb or petals.
//
// How it works: a table (borrowed from Set 3's WORMHOLE, in flash) gives
// the angle and depth for every 2 × 2 block of the screen; the angle wraps
// round the tunnel and the depth runs along it, and together they look up
// the lattice — the same per-frame colour table trick as SIREN.
//
// Presets: k0 Siren Gate · k1 Honeycomb · k2 Temple Corridor · k3 Star Well ·
//          k4 Lotus Tunnel · k5 Ember Core · k6 Ice Shaft · k7 Spiral Portal ·
//          k8 Nebula Drift · k9 Gilded Hall · k10 Spectrum Rush · k11 Tour
//   k12 — WARP: a burst of speed, the walls streaking past
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Shape (round · hexagon · square; centre = preset) · p6 Flight (◄ back | forward ►)
//   p7 Twist · p8 Sway · p9 Tiles Around · p10 Fog · p11 Tile (stars · straps · honeycomb · petals)
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"
#include "wh_lut.h"

enum { PO_ROUND, PO_HEX, PO_SQUARE };
enum { PO_STARS, PO_STRAPS, PO_COMB, PO_PETALS, PO_NTILE };
struct PoLook { uint8_t shape, tile, pal, fly, twist, sway, around, fog, len, flow, spread; };
static const PoLook po_looks[11] = {
  // shape   tile       pal fly twist sway around fog len flow spread
  { PO_HEX,   PO_STARS,  0, 50, 10, 40, 12, 50, 50, 5, 70 },   // Siren Gate
  { PO_HEX,   PO_COMB,   9, 45,  0, 30, 12, 50, 50, 4, 50 },   // Honeycomb
  { PO_SQUARE,PO_STRAPS, 1, 40,  0, 20,  8, 60, 60, 3, 50 },   // Temple Corridor
  { PO_ROUND, PO_STARS,  4, 55, 20, 30, 10, 40, 40, 6, 80 },   // Star Well
  { PO_ROUND, PO_PETALS, 3, 40, 10, 40, 12, 50, 50, 4, 70 },   // Lotus Tunnel
  { PO_HEX,   PO_STARS,  5, 70, 15, 50, 12, 40, 40, 7, 60 },   // Ember Core
  { PO_HEX,   PO_STRAPS, 4, 45,  5, 30, 12, 60, 50, 4, 50 },   // Ice Shaft
  { PO_ROUND, PO_STARS,  8, 50, 60, 30, 10, 50, 50, 6, 90 },   // Spiral Portal
  { PO_ROUND, PO_COMB,   8, 40, 20, 50, 14, 40, 60, 5, 90 },   // Nebula Drift
  { PO_SQUARE,PO_STARS,  9, 40,  0, 20,  8, 60, 60, 3, 50 },   // Gilded Hall
  { PO_HEX,   PO_PETALS,11, 75, 25, 40, 12, 40, 40, 9,100 },   // Spectrum Rush
};
static const char* const po_names[12] = { "Siren Gate", "Honeycomb", "Temple Corridor", "Star Well", "Lotus Tunnel", "Ember Core", "Ice Shaft", "Spiral Portal", "Nebula Drift", "Gilded Hall", "Spectrum Rush", "Tour" };
static const char* const po_lbl[7] = { "Shape", "Flight", "Twist", "Sway", "Tiles Around", "Fog", "Tile" };
const char* prog_portal_name() { return "PORTAL"; }
const char* prog_portal_character() { return "A flight down a hexagonal tunnel tiled with glowing star lattice, toward a light at the end"; }
const char* prog_portal_presetName(int p) { return (p >= 0 && p < 12) ? po_names[p] : NULL; }
const char* prog_portal_potLabel(int p, int pot) { (void)p; return v6_label(pot, po_lbl); }
uint8_t prog_portal_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── Memory ───────────────────────────────────────────────────────────
static uint8_t* po_rt;                 // this frame's colour for every spot in a hexagon tile
static uint8_t* po_fog;                // depth → how bright (and a hue shift)
static uint8_t* po_hexF;               // angle → how much further a hexagonal wall is than a round one (×256)
static void po_mem() { po_rt = v6_mem; po_fog = v6_mem + V6_QN + 3; po_hexF = po_fog + 512; }

// the tile's colour for every spot (rebuilt each frame: it ripples)
static void po_tile(int tile, float ph, float star) {
  const uint16_t* geo = tile == PO_STRAPS ? v6_geoWedge : v6_geoPolar;
  int phi = (int)(ph * 64), amp = (int)(star * 150);
  for (int i = 0; i < V6_QN; i++) {
    int e = geo[i], lo = e & 255, hi = (e >> 8) & 127, B = (e >> 15) & 1, lev, hue;
    switch (tile) {
      case PO_STARS: {                                            // rings scaled by a six-pointed star
        int c = v3_cos((hi * 512) >> 7) >> 6; int sh = c > 0 ? c : -((c * c) >> 8);
        int p = ((lo * (256 + ((sh * amp) >> 8))) >> 8) + phi;          // about four rings per tile
        int t = p & 63, d = t < 32 ? t : 63 - t;
        lev = v3_clamp((d * 15) / 20, 0, 15); if (d < 4) lev = lev * d / 4;
        hue = ((p >> 6) * 5) & 15; break;
      }
      case PO_STRAPS: {                                           // a Hankin strap from each edge's middle
        int X = lo, Y = (hi * 297) >> 8;
        int d1 = v3_abs(((X - 255) * 181 + Y * 181) >> 8);       // 45° contact angle
        if (d1 < 26) { lev = 15 - d1 / 3; hue = 0; }
        else if (d1 < 34) { lev = 2; hue = 0; }
        else { lev = 6 + ((X + Y) >> 6); hue = (d1 < 90 ? 6 : 11) + B * 3; }
        break;
      }
      case PO_COMB: {                                             // honeycomb: bright edges, glowing middles
        lev = lo > 205 ? 15 : (lo < 60 ? 10 - lo / 10 : 3 + ((lo + phi) & 31) / 8);
        hue = (lo > 205 ? 2 : 8) + B * 4; break;
      }
      default: {                                                  // petals: six round petals round each centre
        int c = v3_cos((hi * 512) >> 7);                          // 1 at the edges' middles, −1 at the corners
        int pr = 128 + ((c * 60) >> 14);                          // the petal's outline radius
        int d = v3_abs(lo - pr - ((phi & 63) - 32) / 4);
        lev = d < 14 ? 15 - d : (lo < pr ? 7 : 2);
        hue = (lo < pr ? 4 : 10) + B * 3; break;
      }
    }
    po_rt[i] = (uint8_t)((v3_clamp(lev, 0, 15) << 4) | (hue & 15));
  }
}

// ─── The hot loop: one row of 2 × 2 blocks ────────────────────────────
// cx, cy: the vanishing point in blocks. Each block: angle round the tunnel → U, depth → V.
__attribute__((optimize("O2"))) static void po_row(uint8_t* row, int by, int cx, int cy, int shape, int32_t uK, int32_t vK, uint32_t vOff, uint32_t uOff, int32_t twist, int hueOff) {
  const uint8_t* rt = po_rt; const uint8_t* fog = po_fog; const uint8_t* hf = po_hexF;
  int dy = by - cy, ady = dy < 0 ? -dy : dy; if (ady >= WH_QH) ady = WH_QH - 1;
  const uint32_t* lrow = wh_lut + ady * WH_QW;
  for (int bx = 0; bx < W / 2; bx++) {
    int dx = bx - cx, adx = dx < 0 ? -dx : dx; if (adx >= WH_QW) adx = WH_QW - 1;
    uint32_t e = lrow[adx];
    int q = (int)(e & 255);
    int A = dx >= 0 ? (dy >= 0 ? q : 1024 - q) : (dy >= 0 ? 512 - q : 512 + q);   // 0 … 1023 round the tunnel
    int dep = shape == PO_SQUARE ? (int)(e >> 20) : (int)((e >> 8) & 4095);
    if (shape == PO_HEX) dep = (dep * hf[A & 1023]) >> 8;
    uint32_t U = (uint32_t)(A & 1023) * (uint32_t)uK + (uint32_t)(dep * twist) + uOff;
    uint32_t V = (uint32_t)(dep * vK) + vOff;
    int c = rt[V6_QIDX(U, V)];
    int f = fog[dep > 4095 ? 255 : dep >> 4];                     // bits 0-3: how much tile detail survives the distance; bits 4-7: hue shift
    int lev = 7 + ((((c >> 4) - 7) * (f & 15)) >> 4);             // far away the tiles blur into a glow
    if (dep > 3300) lev = 15;                                     // the light at the end
    int hue = (f & 15) < 7 ? (f >> 4) + hueOff : c + (f >> 4) + hueOff;   // far away, the tile's own colours blur out too
    uint8_t px = (uint8_t)((lev << 4) | (hue & 15));
    row[2 * bx] = px; row[2 * bx + 1] = px;
  }
  memcpy(row + W, row, W);
}

static float po_fly, po_twist, po_sway, po_around, po_fogK, po_len;
static float po_z = 0, po_warp = 0, po_cxF = 80, po_cyF = 60, po_rip = 0, po_tw = 0;
static int po_shapeNow = PO_HEX, po_tileNow = PO_STARS;

void prog_portal_init() {
  po_mem(); v6_init(); po_z = 0; po_warp = 0;
  for (int a = 0; a < 1024; a++) {                                 // hexagon: the wall is cos30 / cos(θ' − 30°) as far as a round one's
    int t = a % 171; float th = (t / 171.0f) * 1.0471976f - 0.5235988f;
    po_hexF[a] = (uint8_t)v3_clamp((int)(256.0f * 0.8660254f / cosf(th) - 0.5f), 0, 255);
  }
}

void prog_portal_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  po_mem();
  const PoLook& L = po_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  int shape = v3_centre(5) ? L.shape : v3_clamp(v3_k(5) * 3 / 1024, 0, 2);
  float tFly = v3_centre(6) ? L.fly / 100.0f : v3_bi(6) * 1.5f;
  float tTwist = v3_centre(7) ? L.twist / 100.0f : v3_bi(7);
  float tSway = v3_centre(8) ? L.sway / 100.0f : v3_u(8);
  float tAround = v3_centre(9) ? L.around : 6.0f + v3_u(9) * 18.0f;
  float tFog = v3_centre(10) ? L.fog / 100.0f : v3_u(10);
  int tile = v3_centre(11) ? L.tile : v3_clamp(v3_k(11) * PO_NTILE / 1024, 0, PO_NTILE - 1);
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&po_fly, tFly, rate); v6_ease(&po_twist, tTwist, rate); v6_ease(&po_sway, tSway, rate);
  v6_ease(&po_around, tAround, rate); v6_ease(&po_fogK, tFog, rate); v6_ease(&po_len, L.len / 100.0f, rate);
  if (v6_first || v6_fade > 0.5f || !v3_centre(5)) po_shapeNow = shape;
  if (v6_first || v6_fade > 0.5f || !v3_centre(11)) po_tileNow = tile;
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 130);

  // ── flight ──
  if (v6_k12hit) po_warp = 1.0f;
  po_warp -= dt * 0.5f; if (po_warp < 0) po_warp = 0;
  float wv = po_warp * po_warp;
  po_z += sdt * (po_fly * 0.9f + wv * 6.0f);
  po_rip += sdt * 1.3f;
  po_tw += sdt * po_twist * 0.25f;
  // the vanishing point sways (in blocks)
  float sw = po_sway;
  float tcx = 80 + sw * 34.0f * sinf(v6_t * 0.37f) + sw * 10.0f * sinf(v6_t * 0.91f);
  float tcy = 60 + sw * 22.0f * sinf(v6_t * 0.29f + 1.0f);
  v6_ease(&po_cxF, tcx, 2.0f); v6_ease(&po_cyF, tcy, 2.0f);

  // ── this frame's tile, and the fog ──
  po_tile(po_tileNow, -po_rip, 0.55f + 0.25f * sinf(v6_t * 0.5f));
  int fogN = (int)(14 + (1.0f - po_fogK) * 70);                    // how deep (in depth/16) before the tiles blur away
  for (int d = 0; d < 256; d++) {
    int w = 16 - (d * 16) / fogN;
    int hs = (d >> 3) & 15;                                       // colour bands receding into the distance
    po_fog[d] = (uint8_t)((v3_clamp(w, 0, 15)) | (hs << 4));
  }

  // ── draw ──
  float z = v6_zoom(1.0f);
  // tiles round the tunnel: a whole number, or there'd be a seam
  int32_t uK = (int32_t)((int)(po_around + 0.5f)) * 64;           // 1024 angle steps → tiles × 65536
  int32_t vK = (int32_t)(65536.0f / (40.0f + po_len * 120.0f) * z);
  uint32_t vOff = (uint32_t)(int32_t)(po_z * 65536.0f);
  uint32_t uOff = (uint32_t)(int32_t)(po_tw * 65536.0f);           // the tunnel slowly turning
  int32_t twist = (int32_t)(po_twist * 40.0f);                     // … and twisting into a spiral
  int hueOff = (int)(po_z * 0.5f) & 15;
  int cx = (int)po_cxF, cy = (int)po_cyF;
  for (int by = 0; by < H / 2; by++) po_row(v3_buf + by * 2 * W, by, cx, cy, po_shapeNow, uK, vK, vOff, uOff, twist, hueOff);
  v6_end();
}

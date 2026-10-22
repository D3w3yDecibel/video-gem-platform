// =====================================================================
// PROGRAM: SIREN — a living hexagonal star lattice (by Dewey) — Set 6, slot 0
//
// The program closest to the reel that inspired Set 6 (Burt Vera Cruz,
// "Call of the Virgo Siren"): a dense quilt of tiny six-fold stars and
// rosettes, laid over a second, larger lattice of mandalas, washed in
// slow horizontal bands of colour that drift from violet through gold to
// teal. The rings in every hexagon ripple outward, the stars sharpen and
// soften, and each hexagon runs a little ahead of or behind its
// neighbours, so the whole surface shimmers and morphs.
//
// How it works: every pixel finds its hexagon (vg6.h's lattice), looks up
// its distance and angle from the hexagon's centre in a table, and turns
// that into a ring of a star-shaped ripple through three tiny tables that
// are rebuilt every frame. A second lattice, three times bigger, does the
// same at half resolution and decides the colours (or shades the stars).
//
// Presets: k0 Siren · k1 Mosaic · k2 Peacock · k3 Lotus · k4 Oracle ·
//          k5 Ember · k6 Jade Floor · k7 Opal · k8 Nebula · k9 Gilded ·
//          k10 Spectrum · k11 Tour
//   k12 — BLOOM: a pulse of light bursts out of every star
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Star (how pointed) · p6 Rings (how many) · p7 Mandalas (the big lattice)
//   p8 Perspective (a floor running to the horizon) · p9 Spin
//   p10 Variety (hexagons out of step) · p11 Bands (colour bands down the screen)
//   p12-p15 live FX (warp, mirror, colour, glitch)
// =====================================================================
#include "vg6.h"

// ─── Looks ────────────────────────────────────────────────────────────
// sp spacing px · big: big lattice ratio ×10 · tw big lattice turned 30°? · pts 1 = 6 points, 2 = 12
// star, ring ×100 · mand: big lattice 0-100 · mode 0 colours, 1 shades, 2 both · persp, spin, var, band ×100
// pal, flow ×100, spread ×100, ripple speed ×10, sheen
struct SrLook { uint8_t sp, big, tw, pts, pts2, star, ring, mand, mode, persp, spin, var, band, pal, flow, spread, rip, sheen; };
static const SrLook sn_looks[11] = {
  // sp big tw pts pts2 star ring mand mode persp spin var band pal flow spread rip sheen
  { 15, 30, 1, 1, 2,  55, 45, 75, 0, 45, 10, 35, 70,  0, 4, 60, 12, 120 },   // Siren
  { 26, 30, 0, 1, 1,  80, 30, 40, 1,  0,  6, 10, 30,  1, 3, 45,  8,  90 },   // Mosaic
  { 22, 40, 1, 2, 1,  40, 55, 80, 2, 15,  8, 25, 45,  2, 5, 60, 10, 140 },   // Peacock
  { 30, 30, 0, 2, 2,  65, 35, 50, 0,  0,  4, 15, 55,  3, 3, 40,  6, 200 },   // Lotus
  { 14, 30, 1, 1, 2,  45, 40, 60, 1, 30, 12, 45, 60,  4, 6, 70, 16, 160 },   // Oracle
  { 20, 30, 0, 1, 1,  70, 60, 60, 0, 20, 14, 30, 80,  5, 8, 50, 24,  60 },   // Ember
  { 16, 30, 1, 1, 2,  55, 45, 50, 2, 90,  0, 30, 50,  6, 4, 55, 12, 110 },   // Jade Floor
  { 24, 30, 0, 2, 1,  35, 30, 60, 0, 10,  6, 20, 40,  7, 3, 35,  6, 120 },   // Opal
  { 19, 40, 1, 1, 2,  60, 50, 90, 2, 25, 16, 70, 60,  8, 6, 80, 14, 150 },   // Nebula
  { 24, 30, 1, 2, 2,  60, 40, 45, 1,  0,  5, 15, 30,  9, 2, 40, 10, 170 },   // Gilded
  { 18, 30, 1, 1, 1,  55, 45, 70, 0, 35, 10, 40, 90, 11, 8,100, 12, 100 },   // Spectrum
};
static const char* const sn_names[12] = { "Siren", "Mosaic", "Peacock", "Lotus", "Oracle", "Ember", "Jade Floor", "Opal", "Nebula", "Gilded", "Spectrum", "Tour" };
static const char* const sn_lbl[7] = { "Star", "Rings", "Mandalas", "Perspective", "Spin", "Variety", "Bands" };
const char* prog_siren_name() { return "SIREN"; }
const char* prog_siren_character() { return "A living hexagonal star lattice: rippling six-fold stars over mandalas, in drifting bands of colour"; }
const char* prog_siren_presetName(int p) { return (p >= 0 && p < 12) ? sn_names[p] : NULL; }
const char* prog_siren_potLabel(int p, int pot) { (void)p; return v6_label(pot, sn_lbl); }
uint8_t prog_siren_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── Memory (shared scratch) ──────────────────────────────────────────
static uint8_t* sn_rt;                   // 4225: colour (level | ring hue << 4) of every spot in a hexagon, this frame
static uint8_t* sn_comb;                 // 4096: [star colour | mandala level << 8] → final level and hue
static uint16_t* sn_rf;  static uint16_t* sn_st;  static uint8_t* sn_ring;   // small lattice: radius → phase, angle → star, phase → colour
static uint16_t* sn_rf2; static uint16_t* sn_st2; static uint8_t* sn_ring2;  // big lattice
static uint16_t* sn_bigB;                // per 4 columns: the mandala level << 8
static uint8_t* sn_bigH;                 // … and the hue shift there (colour bands + each mandala its own tint)
static uint8_t* sn_varT;                 // per-mandala tints
static uint8_t* sn_rt2;                  // 4225: the big lattice's colours this frame
static void sn_mem() {
  uint8_t* p = v6_mem;
  sn_comb = p; p += 4096;
  sn_rf = (uint16_t*)p; sn_st = sn_rf + 256; sn_rf2 = sn_st + 128; sn_st2 = sn_rf2 + 256; p += 1536;
  sn_bigB = (uint16_t*)p; p += 320;
  sn_rt = p; p += V6_QN + 3;
  sn_rt2 = p; p += V6_QN + 3;
  sn_ring = p; sn_ring2 = p + 256; sn_bigH = p + 512; sn_varT = p + 672; p += 800;   // total ≈ 15.2 KB
}

// ─── Smoothed parameters (presets fade into each other) ───────────────
static float sn_sp, sn_big, sn_star, sn_ring_, sn_mand, sn_persp, sn_spin, sn_var, sn_band, sn_rip, sn_sheen;
static float sn_ang = 0, sn_ph = 0, sn_ph2 = 0, sn_bandPh = 0, sn_bloom = 0, sn_morph = 0, sn_pan = 0;
static bool sn_twNow = true; static int sn_ptsNow = 1, sn_pts2Now = 2, sn_modeNow = 0;

void prog_siren_init() {
  sn_mem(); v6_init();
  const SrLook& L = sn_looks[0];
  sn_sp = L.sp; sn_big = L.big / 10.0f; sn_star = L.star / 100.0f; sn_ring_ = L.ring / 100.0f; sn_mand = L.mand / 100.0f;
  sn_persp = L.persp / 100.0f; sn_spin = L.spin / 100.0f; sn_var = L.var / 100.0f; sn_band = L.band / 100.0f; sn_rip = L.rip / 10.0f; sn_sheen = L.sheen;
}

// the three tables of one lattice. Phase = rf[radius] × st[angle] / 256: the
// rings are scaled by the star, so they become nested six- (or twelve-) pointed
// stars. period: ring spacing in px; hexR: 0.6 × the hexagon spacing in px.
// pts: 1 = six points, 2 = twelve. star: 0 … 1 how pointed.
static void sn_tables(uint16_t* rf, uint16_t* st, uint8_t* ring, float period, float hexR, float star, int pts, float ph, int hueStep) {
  float kk = 64.0f * hexR / (255.0f * period);                      // phase per radius step (64 = one ring)
  int k = (int)(kk * 256), phi = (int)(ph * 64);
  for (int r = 0; r < 256; r++) rf[r] = (uint16_t)(((r * k) >> 8) & 0xFFFF);
  int amp = (int)(star * 150);
  for (int a = 0; a < 128; a++) {
    int c = v3_cos((a * pts * 512) >> 7) >> 6;                      // 0° … 30° → cos(6θ) or cos(12θ), −256 … 256
    int sh = c > 0 ? c : -((c * c) >> 8);                           // sharper points than a plain cosine
    st[a] = (uint16_t)(256 + ((sh * amp) >> 8));                    // ×1 ± star
  }
  for (int i = 0; i < 256; i++) {
    int t = (i + phi) & 63, d = t < 32 ? t : 63 - t;                // a ring: 0 at its edges, 31 in the middle
    int lev = v3_clamp((d * 15) / 22, 0, 15);
    if (d < 5) lev = (lev * d) / 5;
    int hue = ((((i + phi) & 255) >> 6) * hueStep) & 15;           // each ring a step round the palette
    ring[i] = (uint8_t)(lev | (hue << 4));
  }
}

// ─── The hot loops (compiled for speed: they run for every pixel) ──────
// the big lattice, at quarter resolution across (half down): its level and a hue shift for every 4 columns
__attribute__((optimize("O2"))) static void sn_bigRowDraw(uint32_t U, uint32_t V, int32_t du, int32_t dv, int hb, int tint) {
  const uint8_t* rt = sn_rt2; const uint8_t* vt = sn_varT;
  uint16_t* ob = sn_bigB; uint8_t* oh = sn_bigH;
  if (!tint) {
    for (int x = 0; x < W / 4; x++, U += du, V += dv) { int r = rt[V6_QIDX(U, V)]; ob[x] = (uint16_t)((r & 15) << 8); oh[x] = (uint8_t)(hb + (r >> 4)); }
    return;
  }
  for (int x = 0; x < W / 4; x++, U += du, V += dv) {
    int i = V6_QIDX(U, V), r = rt[i];
    uint32_t off = (uint32_t)(v6_geoPolar[i] & 0x8000);              // which mandala: each gets its own tint
    int id = (int)((((U + off) >> 16) ^ (((V + off) >> 16) << 2) ^ (off >> 12)) & 127);
    ob[x] = (uint16_t)((r & 15) << 8); oh[x] = (uint8_t)(hb + (r >> 4) + vt[id]);
  }
}
// the small lattice: four pixels share one big-lattice value
__attribute__((optimize("O2"))) static void sn_rowDraw(uint8_t* row, uint32_t U, uint32_t V, int32_t du, int32_t dv) {
  const uint8_t* rt = sn_rt; const uint8_t* comb = sn_comb; const uint16_t* bb = sn_bigB; const uint8_t* bh = sn_bigH;
  for (int k = 0; k < W / 4; k++) {
    const uint8_t* cb = comb + bb[k]; int h = bh[k], c;
    c = cb[rt[V6_QIDX(U, V)]]; U += du; V += dv; row[0] = (uint8_t)((c & 0xF0) | ((c + h) & 15));
    c = cb[rt[V6_QIDX(U, V)]]; U += du; V += dv; row[1] = (uint8_t)((c & 0xF0) | ((c + h) & 15));
    c = cb[rt[V6_QIDX(U, V)]]; U += du; V += dv; row[2] = (uint8_t)((c & 0xF0) | ((c + h) & 15));
    c = cb[rt[V6_QIDX(U, V)]]; U += du; V += dv; row[3] = (uint8_t)((c & 0xF0) | ((c + h) & 15));
    row += 4;
  }
}

void prog_siren_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  sn_mem();
  const SrLook& L = sn_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;

  // ── knobs (centre = the look's own) and smoothing ──
  float tStar = v3_centre(5) ? L.star / 100.0f : v3_u(5) * 1.3f;
  float tRing = v3_centre(6) ? L.ring / 100.0f : 0.15f + v3_u(6) * 1.2f;
  float tMand = v3_centre(7) ? L.mand / 100.0f : v3_u(7);
  float tPersp = v3_centre(8) ? L.persp / 100.0f : v3_u(8);
  float tSpin = v3_centre(9) ? L.spin / 100.0f : v3_bi(9) * 0.6f;
  float tVar = v3_centre(10) ? L.var / 100.0f : v3_u(10);
  float tBand = v3_centre(11) ? L.band / 100.0f : v3_u(11);
  float rate = v6_fade < 1.0f ? 1.6f : 4.0f;
  v6_ease(&sn_sp, L.sp, rate); v6_ease(&sn_big, L.big / 10.0f, rate);
  v6_ease(&sn_star, tStar, rate); v6_ease(&sn_ring_, tRing, rate); v6_ease(&sn_mand, tMand, rate);
  v6_ease(&sn_persp, tPersp, rate); v6_ease(&sn_spin, tSpin, rate); v6_ease(&sn_var, tVar, rate);
  v6_ease(&sn_band, tBand, rate); v6_ease(&sn_rip, L.rip / 10.0f, rate); v6_ease(&sn_sheen, L.sheen, rate);
  if (v6_fade > 0.5f) { sn_twNow = L.tw; sn_ptsNow = L.pts; sn_pts2Now = L.pts2; sn_modeNow = L.mode; }   // switch shapes halfway through a fade
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, (int)sn_sheen);

  // ── time ──
  if (v6_k12hit) sn_bloom = 1.0f;
  sn_bloom -= dt * 0.9f; if (sn_bloom < 0) sn_bloom = 0;
  float bl = sn_bloom * sn_bloom;
  sn_ang += sdt * sn_spin * 0.25f;
  sn_ph -= sdt * sn_rip * 0.25f;                                    // rings travel outward
  sn_ph2 += sdt * sn_rip * 0.07f;
  sn_bandPh += sdt * 0.9f;
  sn_morph += sdt * 0.35f;
  sn_pan += sdt * (sn_persp > 0.5f ? 0.6f * (sn_persp - 0.5f) * 2 : 0.0f);   // a floor glides toward you
  float zoom = v6_zoom(1.0f) * (1.0f + 0.06f * sinf(v6_t * 0.21f));
  float star = v3_clampf(sn_star * (0.75f + 0.35f * sinf(sn_morph)) + bl * 0.5f, 0.0f, 1.2f);
  float star2 = v3_clampf(sn_star * 0.8f * (0.8f + 0.3f * sinf(sn_morph * 0.7f + 1.3f)), 0.0f, 1.2f);
  float sp = sn_sp * zoom, sp2 = sp * sn_big;
  float period = (3.5f + (1.0f - v3_clampf(sn_ring_, 0, 1.3f)) * 7.0f) * (0.6f + 0.4f * zoom) * (1.0f - bl * 0.4f);   // ring spacing, px
  sn_tables(sn_rf, sn_st, sn_ring, period, sp * 0.6f, star, sn_ptsNow, sn_ph + bl * 2.0f, 5);
  sn_tables(sn_rf2, sn_st2, sn_ring2, period * 2.2f, sp2 * 0.6f, star2, sn_pts2Now, sn_ph2, 3);

  // ── the two lattices ──
  V6View a, b;
  v6_view(&a, sp, sn_ang, 0, sn_pan, 0, 0);          // origin at the screen centre
  v6_view(&b, sp2, sn_ang + (sn_twNow ? 0.5235988f : 0.0f), 0.31f, 0.17f + sn_pan / sn_big, 0, 0);
  int mand = (int)(sn_mand * 16);
  // this frame's colour for every spot in a hexagon
  {
    const uint16_t* rf = sn_rf; const uint16_t* st = sn_st; const uint8_t* ring = sn_ring;
    for (int i = 0; i < V6_QN; i++) { int e = v6_geoPolar[i]; sn_rt[i] = ring[((rf[e & 255] * st[(e >> 8) & 127]) >> 8) & 255]; }
    rf = sn_rf2; st = sn_st2; ring = sn_ring2;
    for (int i = 0; i < V6_QN; i++) { int e = v6_geoPolar[i]; sn_rt2[i] = ring[((rf[e & 255] * st[(e >> 8) & 127]) >> 8) & 255]; }
  }
  // how the mandalas (big lattice) and the stars combine: [star colour | mandala level << 8] → level, hue
  for (int bi = 0; bi < 16; bi++) {
    int hs = sn_modeNow == 1 ? 0 : (bi * mand) >> 5;
    for (int r = 0; r < 256; r++) {
      int li = r & 15, lev;
      if (sn_modeNow == 0) lev = (li * (14 + ((bi * mand) >> 6))) >> 4;                 // the mandalas colour the stars
      else if (sn_modeNow == 1) lev = (li * (16 - mand + ((bi * mand) >> 4))) >> 4;   // … or shade them
      else lev = v6_max(li, (bi * mand) >> 5);                                         // … or both, glowing through
      sn_comb[r | (bi << 8)] = (uint8_t)((v3_clamp(lev, 0, 15) << 4) | (((r >> 4) + hs) & 15));
    }
  }
  int tint = (int)(sn_var * 8);
  for (int i = 0; i < 128; i++) sn_varT[i] = (uint8_t)((v3_hash(i * 7919u + 17u) & 15) * tint >> 4);
  int bandK = (int)(sn_band * 4096 / H);                            // hue bands: up to 16 hues top to bottom
  int bandOff = (int)(sn_bandPh * 256);
  for (int y = 0; y < H; y++) {
    // perspective: rows nearer the top are further away (smaller hexagons). ps in 1/4096.
    int32_t ps = 4096;
    if (sn_persp > 0.01f) ps = (int32_t)(4096.0f / (0.35f + 0.65f * (1.0f - sn_persp) + sn_persp * 0.845f * y / H));
    int32_t sy = (y - 120) * ps;                                    // 1/4096 px
    if (!(y & 1)) {                                                 // the big lattice at half resolution, on even rows
      int hb = (((y * bandK) >> 8) + (bandOff >> 8)) & 15;
      int32_t bux = ((b.du_x * ps) >> 12) * 4, bvx = ((b.dv_x * ps) >> 12) * 4;
      sn_bigRowDraw(b.u0 + (uint32_t)((sy >> 6) * (b.du_y >> 2) >> 4) - (uint32_t)(40 * bux),
                    b.v0 + (uint32_t)((sy >> 6) * (b.dv_y >> 2) >> 4) - (uint32_t)(40 * bvx), bux, bvx, hb, tint);
    }
    int32_t dux = (a.du_x * ps) >> 12, dvx = (a.dv_x * ps) >> 12;
    // the view's origin is the screen centre: step back to column 0 at this row's scale
    sn_rowDraw(v3_buf + y * W, a.u0 + (uint32_t)((sy >> 6) * (a.du_y >> 2) >> 4) - (uint32_t)(160 * dux),
               a.v0 + (uint32_t)((sy >> 6) * (a.dv_y >> 2) >> 4) - (uint32_t)(160 * dvx), dux, dvx);
  }
  v6_end();
}

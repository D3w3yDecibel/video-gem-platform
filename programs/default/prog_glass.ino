// =====================================================================
// PROGRAM: GLASS — living stained glass (by Dewey) — Set 3, slot 8
//
// A window of jewel-coloured glass held in dark lead. The panes are alive:
// they drift, grow, split and merge, so the window is always re-leading
// itself. Light shines through — brighter in the middle of each pane, a
// bright bevel at its edge — and a beam of sunlight sweeps across. Rose
// windows turn with perfect symmetry; k12 shatters the window and lets it
// fly back together.
//
// How it works: every pane belongs to a seed point (a Voronoi diagram).
// The screen is split into 16×16 tiles, each with a short list of the seeds
// that can reach it, so each 2×2 block only checks a few seeds; blocks near
// a lead line are redone pixel by pixel so the lines stay crisp.
//
// Presets:
//   k0 Cathedral   — an eight-fold rose window
//   k1 Tiffany     — soft organic panes in copper foil
//   k2 Crystal     — crushed glass: many small shimmering pieces
//   k3 Honeycomb   — amber hexagons, breathing
//   k4 Cracked Ice — pale ice crazed with white cracks
//   k5 Jewel       — a few big faceted jewels with bright bevels
//   k6 Kaleido     — a six-fold kaleidoscope of neon glass
//   k7 Neon Glass  — dark glass, bright neon leading
//   k8 Cells       — living cells that divide, with glowing nuclei
//   k9 Shatter     — breaks apart and flies back together, again and again
//   k10 Sun Window — big panes with a strong sweeping sunbeam
//   k11 Gallery    — moves through all the others every ~20 s
//
//   k12 — SHATTER
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Panes (how many)
//   p3 Lead Width   p4 Order (left: random, right: a regular lattice)   p5 Sunbeam
//   p6 Beam Speed   p7 Bevel   p8 Glow (light through the middle of each pane)
//   p9 Shimmer   p10 Layout (centre = preset: random, hexagons, rings, spiral, grid)
//   p11 Symmetry (centre = preset; 1-12 fold)   p12 Nuclei   p13 Colours (how many)
//   p14 Shatter (automatic)   p15 Lead Colour (dark → copper → gold → white)
// =====================================================================

#include "vg3.h"

// ─── Shared memory ────────────────────────────────────────────────────
#define GL_MAXS 64                                 // seeds actually drawn (after symmetry)
#define GL_MAXM 48                                 // master seeds
#define GL_TW 20                                   // tiles 16 × 16 px
#define GL_TH 15
#define GL_TC 16                                   // candidates per tile
struct GlSeed { float x, y, hx, hy, vx, vy, ph; uint8_t hue, live; uint16_t pad; };
static GlSeed* gl_m;                               // master seeds
static int16_t* gl_sx; static int16_t* gl_sy;      // drawn seeds, px
static uint8_t* gl_sh;                             // drawn seeds' hues
static uint8_t* gl_cand;                           // [tile][GL_TC]
static uint8_t* gl_nc;                             // candidates per tile
static void gl_mem() {
  gl_m = (GlSeed*)v3_scratch;                                       // 48 × 32 B
  gl_sx = (int16_t*)(v3_scratch + 1536); gl_sy = (int16_t*)(v3_scratch + 1664);
  gl_sh = v3_scratch + 1792;
  gl_cand = v3_scratch + 1856;                                      // 300 × 12
  gl_nc = v3_scratch + 1856 + GL_TW * GL_TH * GL_TC;                // 300
}

// ─── Palettes: hues 1-12 glass, 13 bevel, 14 nucleus; hue 0 lead ──────
#define GL_NPAL 8
static const uint8_t gl_pals[GL_NPAL][8][3] = {
  {{200,10,30},{20,50,200},{0,150,70},{255,190,0},{120,20,170},{0,120,200},{255,90,0},{255,240,200}},     // 0 Cathedral
  {{230,150,40},{140,160,40},{220,110,120},{60,150,140},{240,210,120},{170,80,40},{110,170,90},{255,235,190}},  // 1 Tiffany
  {{150,230,255},{80,140,255},{230,250,255},{120,90,255},{0,200,230},{200,170,255},{60,90,200},{255,255,255}},  // 2 Ice
  {{255,170,0},{255,120,0},{230,200,40},{200,90,10},{255,220,100},{170,110,0},{255,150,40},{255,250,200}},  // 3 Amber
  {{255,0,170},{0,240,255},{255,240,0},{120,60,255},{0,255,120},{255,90,0},{200,0,255},{255,255,255}},     // 4 Neon
  {{170,0,40},{0,30,120},{0,90,50},{120,0,110},{200,140,0},{0,80,120},{90,0,0},{255,230,180}},            // 5 Jewel
  {{255,90,60},{255,170,40},{255,60,130},{150,40,160},{255,210,90},{220,40,60},{120,60,200},{255,240,220}},  // 6 Sunset
  {{90,200,90},{200,240,120},{40,150,120},{230,120,170},{120,220,200},{60,120,60},{250,200,120},{255,255,220}},  // 7 Garden
};
static const uint8_t gl_leads[4][3] = {{18,18,24},{150,80,40},{230,180,60},{235,235,240}};

// ─── Presets ──────────────────────────────────────────────────────────
enum { GLY_RANDOM, GLY_HEX, GLY_RINGS, GLY_SPIRAL, GLY_GRID };
struct GlLook { uint8_t pal, n, lead, order, beam, beamSp, bevel, glow, shimmer, layout, sym, mirror, nuclei, colours, shatter, leadCol, speed, life, dark; };
static const GlLook gl_looks[11] = {
  // pal  n  lead order beam bsp bevel glow shim layout   sym mir nuc col shat lc spd life dark
  {  0,  64,  3,  80,  40, 30,  50,  60,  30, GLY_RINGS,  8, 0,  0,  7,  0, 0, 30,  0, 0},   // Cathedral
  {  1,  34,  2,  10,  25, 25,  35,  55,  20, GLY_RANDOM, 1, 0,  0,  7,  0, 1, 40, 20, 0},   // Tiffany
  {  2,  64,  1,   0,  50, 50,  40,  40,  80, GLY_RANDOM, 1, 0,  0,  6,  0, 3, 50, 50, 0},   // Crystal
  {  3,  46,  2, 100,  35, 35,  45,  60,  30, GLY_HEX,    1, 0,  0,  5,  0, 0, 30,  0, 0},   // Honeycomb
  {  2,  56,  1,  15,  30, 30,  10,  35,  40, GLY_RANDOM, 1, 0,  0,  4,  0, 3, 25, 30, 0},   // Cracked Ice
  {  5,  12,  3,  20,  45, 30,  90,  75,  30, GLY_RANDOM, 1, 0,  0,  7,  0, 2, 25, 10, 0},   // Jewel
  {  4,  36,  2,  30,  40, 45,  50,  50,  50, GLY_RANDOM, 6, 1,  0,  7,  0, 3, 60,  0, 0},   // Kaleido
  {  4,  30,  2,  20,  20, 40,  60,  20,  60, GLY_RANDOM, 1, 0,  0,  7,  0, 9, 40, 30, 1},   // Neon Glass
  {  7,  28,  2,  20,  15, 20,  40,  70,  20, GLY_RANDOM, 1, 0, 80,  6,  0, 1, 35, 80, 0},   // Cells
  {  6,  40,  2,  40,  40, 40,  50,  55,  30, GLY_SPIRAL, 1, 0,  0,  7, 70, 0, 40,  0, 0},   // Shatter
  {  1,  16,  3,  60,  95, 25,  45,  50,  15, GLY_GRID,   1, 0,  0,  7,  0, 0, 20,  0, 0},   // Sun Window
};
static const char* const gl_presetNames[12] = {
  "Cathedral", "Tiffany", "Crystal", "Honeycomb", "Cracked Ice", "Jewel",
  "Kaleido", "Neon Glass", "Cells", "Shatter", "Sun Window", "Gallery"
};
static const char* const gl_labels[16] = {
  "Palette", "Speed", "Panes", "Lead Width", "Order", "Sunbeam", "Beam Speed", "Bevel",
  "Glow", "Shimmer", "Layout", "Symmetry", "Nuclei", "Colours", "Shatter", "Lead Colour"
};
const char* prog_glass_name() { return "GLASS"; }
const char* prog_glass_character() { return "Living stained glass: jewel panes drift, split and merge in their leading, lit by a sweeping sunbeam"; }
const char* prog_glass_presetName(int preset) { return (preset >= 0 && preset < 12) ? gl_presetNames[preset] : NULL; }
const char* prog_glass_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? gl_labels[pot] : ""; }
uint8_t prog_glass_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int gl_preset = -1, gl_look = 0, gl_nm = 0, gl_ns = 0, gl_palNow = 0, gl_palPrev = 0, gl_layoutNow = -1;
static float gl_palX = 1, gl_t = 0, gl_beamT = 0, gl_shatter = 0, gl_shatT = 0, gl_showT = 0, gl_symA = 0;
static bool gl_k12was = false;
static int gl_sym = 1; static bool gl_mirror = false;
static volatile int gl_bevHi = 0;
static int gl_field = 0; static bool gl_full = true;

// lay out the master seeds' home positions
static void gl_layout(int layout, int n, int sym, float order) {
  float cx = 160, cy = 120;
  for (int i = 0; i < GL_MAXM; i++) {
    GlSeed* s = &gl_m[i];
    float hx, hy;
    if (sym > 1) {                                                  // seeds in one wedge; the rest are rotated copies
      float wedge = 6.2831853f / sym;
      float r = 12 + 150 * sqrtf((i + 0.5f) / n), a = v3_rndf() * wedge * (gl_mirror ? 0.5f : 1.0f);
      if (layout == GLY_RINGS) {                                      // a rose window: rings of panes, alternate rings offset
        int per = n >= 6 ? 2 : 1, ring = i / per, rings = (n + per - 1) / per;
        r = 18 + ring * 150.0f / rings;
        a = ((i % per) + 0.5f + (ring & 1) * 0.5f) / per * wedge * (gl_mirror ? 0.5f : 1.0f);
      }
      hx = cx + r * cosf(a); hy = cy + r * sinf(a);
    } else {
      switch (layout) {
        case GLY_HEX: { int cols = (int)sqrtf(n * 1.35f) + 1; float sp = (float)W / cols; int row = i / cols, col = i % cols;
          hx = (col + 0.5f + (row & 1) * 0.5f) * sp; hy = (row + 0.5f) * sp * 0.866f; break; }
        case GLY_GRID: { int cols = (int)sqrtf(n * 1.33f) + 1; int rows = (n + cols - 1) / cols; hx = ((i % cols) + 0.5f) * W / cols; hy = ((i / cols) + 0.5f) * H / rows; break; }
        case GLY_RINGS: { int ring = 0, k = i, cap = 1; while (k >= cap) { k -= cap; ring++; cap = ring * 6; }
          float a = ring ? k * 6.2831853f / cap + ring * 0.3f : 0; float r = ring * 26.0f; hx = cx + r * cosf(a) * 1.15f; hy = cy + r * sinf(a); break; }
        case GLY_SPIRAL: { float a = i * 2.3999632f, r = 9.5f * sqrtf((float)i + 0.5f) * sqrtf(40.0f / n); hx = cx + r * cosf(a) * 1.3f; hy = cy + r * sinf(a); break; }
        default: hx = 10 + v3_rnd(W - 20); hy = 10 + v3_rnd(H - 20);
      }
      // Order: left = jitter the lattice into randomness
      float j = (1.0f - order) * 26.0f;
      if (layout != GLY_RANDOM) { hx += (v3_rndf() - 0.5f) * j; hy += (v3_rndf() - 0.5f) * j; }
    }
    s->hx = hx; s->hy = hy;
  }
}

static void gl_setLook(int look, int n, int layout, int sym, bool mirror, float order, bool fresh) {
  gl_look = look; gl_sym = sym; gl_mirror = mirror;
  int nm = sym > 1 ? (n + sym - 1) / sym : n;
  if (nm > GL_MAXM) nm = GL_MAXM;
  if (sym > 1 && nm * sym * (mirror ? 2 : 1) > GL_MAXS) nm = GL_MAXS / (sym * (mirror ? 2 : 1));
  if (nm < 1) nm = 1;
  gl_layout(layout, nm, sym, order);
  gl_layoutNow = layout;
  for (int i = 0; i < GL_MAXM; i++) {
    GlSeed* s = &gl_m[i];
    if (fresh || i >= gl_nm) { s->x = s->hx; s->y = s->hy; s->vx = s->vy = 0; }
    s->ph = v3_rndf() * 100;
    s->hue = (uint8_t)(1 + v3_rnd(12));
    s->live = 1;
  }
  gl_nm = nm;
}

void prog_glass_init() {
  gl_mem();
  gl_full = true;
  v3_reset();
  gl_preset = -1;
}

static void gl_palette(int colours, float shimmer, int leadCol, bool dark) {
  int xf = (int)(gl_palX * 256);
  for (int h = 1; h <= 12; h++) {
    int k = (h - 1) % (colours < 1 ? 1 : colours);
    int c[3];
    for (int q = 0; q < 3; q++) c[q] = v3_mix8(gl_pals[gl_palPrev][k % 7][q], gl_pals[gl_palNow][k % 7][q], xf);
    // a second shade for hues beyond the palette's seven: a little lighter
    if (h > 7) for (int q = 0; q < 3; q++) c[q] = v3_clamp(c[q] + (255 - c[q]) / 4, 0, 255);
    float sh = 1.0f - shimmer * 0.4f * (0.5f + 0.5f * sinf(gl_t * (0.9f + h * 0.13f) + h * 1.7f));
    v3_setLH(h, (int)(c[0] * sh), (int)(c[1] * sh), (int)(c[2] * sh), 1.1f, 0);
  }
  int bc[3];
  for (int q = 0; q < 3; q++) bc[q] = v3_mix8(gl_pals[gl_palPrev][7][q], gl_pals[gl_palNow][7][q], xf);
  v3_setLH(13, bc[0], bc[1], bc[2], 1.0f, 0);                        // bevel
  v3_setLH(14, 255, 255, 255, 1.0f, 0);                              // nuclei
  int lc = leadCol > 3 ? 3 : leadCol;
  if (leadCol >= 9) v3_setLH(0, 255, 255, 255, 1.0f, 0);             // neon leading: hue 0 used as a bright line
  else v3_setLH(0, gl_leads[lc][0], gl_leads[lc][1], gl_leads[lc][2], 1.0f, 0);
}

void prog_glass_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  gl_mem();
  v3_frame();
  float dt = v3_dt;
  bool gallery = preset == 11;

  int look = gl_look;
  if (preset != gl_preset) look = gallery ? v3_rnd(11) : preset;
  if (gallery && preset == gl_preset) { gl_showT += dt; if (gl_showT > 20.0f) { gl_showT = 0; look = (gl_look + 1 + v3_rnd(10)) % 11; } }
  const GlLook& L = gl_looks[look];

  // ── knobs ──
  float speed = v3_speed(1, 3.0f) * L.speed / 40.0f;
  int n = v3_centre(2) ? L.n : 6 + v3_k(2) * 58 / 1023;
  float leadW = v3_centre(3) ? L.lead : 0.5f + v3_u(3) * 5.0f;
  float order = v3_centre(4) ? L.order / 100.0f : v3_u(4);
  float beam = v3_centre(5) ? L.beam / 100.0f : v3_u(5);
  float beamSp = v3_centre(6) ? L.beamSp / 100.0f : v3_u(6);
  float bevel = v3_centre(7) ? L.bevel / 100.0f : v3_u(7);
  float glow = v3_centre(8) ? L.glow / 100.0f : v3_u(8);
  float shimmer = v3_centre(9) ? L.shimmer / 100.0f : v3_u(9);
  int layout = v3_pick(10, L.layout, 5);
  int sym = v3_centre(11) ? L.sym : 1 + v3_k(11) * 12 / 1024;
  bool mirror = v3_centre(11) ? L.mirror : (sym > 1 && (sym & 1) == 0);
  float nuclei = v3_centre(12) ? L.nuclei / 100.0f : v3_u(12);
  int colours = v3_centre(13) ? L.colours : 1 + v3_k(13) * 7 / 1024;
  float shatter = v3_centre(14) ? L.shatter / 100.0f : v3_u(14);
  int leadCol = v3_centre(15) ? L.leadCol : v3_k(15) * 4 / 1024;
  if (sym > 1 && layout == GLY_RANDOM && L.layout == GLY_RINGS && v3_centre(10)) layout = GLY_RINGS;
  int pal = v3_centre(0) ? L.pal : v3_clamp(v3_k(0) * GL_NPAL / 1024, 0, GL_NPAL - 1);
  if (pal != gl_palNow && gl_palX >= 1) { gl_palPrev = gl_palNow; gl_palNow = pal; gl_palX = 0; }
  if (gl_palX < 1) { gl_palX += dt / 2.0f; if (gl_palX > 1) gl_palX = 1; }

  // ── (re)build the window ──
  static int lastN = -1, lastSym = -1, lastLayout = -1; static bool lastMir = false; static float lastOrder = -1;
  if (preset != gl_preset || look != gl_look || n != lastN || sym != lastSym || mirror != lastMir || layout != lastLayout || fabsf(order - lastOrder) > 0.08f) {
    bool fresh = preset != gl_preset;
    if (preset != gl_preset) { gl_palNow = gl_palPrev = pal; gl_palX = 1; }
    gl_preset = preset;
    gl_setLook(look, n, layout, sym, mirror, order, fresh);
    gl_full = true;
    lastN = n; lastSym = sym; lastMir = mirror; lastLayout = layout; lastOrder = order;
  }

  // ── shatter ──
  bool k12 = keysPressed[KEY_MOD_A];
  bool doShatter = k12 && !gl_k12was;
  gl_k12was = k12;
  if (shatter > 0.02f) { gl_shatT += dt; if (gl_shatT > 2.5f + (1.0f - shatter) * 14.0f) { gl_shatT = 0; doShatter = true; } }
  if (doShatter) {
    float ox = 60 + v3_rnd(200), oy = 40 + v3_rnd(160);
    for (int i = 0; i < gl_nm; i++) { GlSeed* s = &gl_m[i]; float dx = s->x - ox, dy = s->y - oy, d = sqrtf(dx * dx + dy * dy) + 4; s->vx += dx / d * (180 + v3_rnd(120)); s->vy += dy / d * (180 + v3_rnd(120)); }
  }

  // ── move: seeds wander round their homes; Cells divide and die ──
  float sdt = dt * speed;
  gl_t += dt;
  gl_beamT += dt * (0.05f + beamSp * 0.4f);
  gl_symA += sdt * 0.04f * (gl_sym > 1 ? 1.0f : 0.0f);
  float wander = 6 + (1.0f - order) * 22;
  for (int i = 0; i < gl_nm; i++) {
    GlSeed* s = &gl_m[i];
    s->ph += sdt;
    float tx = s->hx + wander * sinf(s->ph * 0.37f + i * 1.3f), ty = s->hy + wander * cosf(s->ph * 0.29f + i * 2.1f);
    if (L.layout == GLY_HEX && v3_centre(10)) { float b = 1.0f + 0.04f * sinf(gl_t * 0.6f); tx = 160 + (tx - 160) * b; ty = 120 + (ty - 120) * b; }   // honeycomb breathes
    s->vx += ((tx - s->x) * 3.0f - s->vx * 1.6f) * dt;
    s->vy += ((ty - s->y) * 3.0f - s->vy * 1.6f) * dt;
    s->x += s->vx * dt * (speed > 0 ? 1.0f : 0.0f); s->y += s->vy * dt * (speed > 0 ? 1.0f : 0.0f);
  }
  if (L.life && speed > 0 && gl_sym <= 1) {                          // a pane splits off a neighbour, another melts away
    static float lifeT = 0; lifeT += sdt * L.life / 50.0f;
    if (lifeT > 1.0f) {
      lifeT = 0;
      int a = v3_rnd(gl_nm), b = v3_rnd(gl_nm);
      GlSeed* sa = &gl_m[a]; GlSeed* sb = &gl_m[b];
      sb->x = sa->x + (v3_rndf() - 0.5f) * 6; sb->y = sa->y + (v3_rndf() - 0.5f) * 6;   // b reappears inside a: a divides
      sb->hx = sa->hx + (v3_rndf() - 0.5f) * 50; sb->hy = sa->hy + (v3_rndf() - 0.5f) * 50;
      sb->hx = v3_clampf(sb->hx, 8, W - 8); sb->hy = v3_clampf(sb->hy, 8, H - 8);
      sb->hue = (uint8_t)(1 + v3_rnd(12));
    }
  }

  // ── drawn seeds (with symmetry) ──
  int ns = 0;
  float ca = cosf(gl_symA), sa = sinf(gl_symA);
  for (int i = 0; i < gl_nm && ns < GL_MAXS; i++) {
    GlSeed* s = &gl_m[i];
    if (gl_sym <= 1) { gl_sx[ns] = (int16_t)s->x; gl_sy[ns] = (int16_t)s->y; gl_sh[ns] = s->hue; ns++; continue; }
    float dx = s->x - 160, dy = s->y - 120;
    float rx = dx * ca - dy * sa, ry = dx * sa + dy * ca;
    for (int k = 0; k < gl_sym && ns < GL_MAXS; k++) {
      float a = k * 6.2831853f / gl_sym, c = cosf(a), sn = sinf(a);
      gl_sx[ns] = (int16_t)(160 + rx * c - ry * sn); gl_sy[ns] = (int16_t)(120 + rx * sn + ry * c); gl_sh[ns] = s->hue; ns++;
      if (gl_mirror && ns < GL_MAXS) {                                // the mirror image within each wedge
        float my = -ry;
        float b = a + 0.0f;
        float c2 = cosf(b), s2 = sinf(b);
        gl_sx[ns] = (int16_t)(160 + rx * c2 - my * s2); gl_sy[ns] = (int16_t)(120 + rx * s2 + my * c2); gl_sh[ns] = s->hue; ns++;
      }
    }
  }
  gl_ns = ns;

  // ── which seeds can reach each tile ──
  for (int t = 0; t < GL_TW * GL_TH; t++) {
    int cx = (t % GL_TW) * 16 + 8, cy = (t / GL_TW) * 16 + 8;
    int32_t d1 = 0x7fffffff, d2 = 0x7fffffff;
    for (int i = 0; i < ns; i++) {
      int32_t dx = gl_sx[i] - cx, dy = gl_sy[i] - cy, d = dx * dx + dy * dy;
      if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
    }
    // any seed farther than (second-nearest + 2 × half-diagonal) can't be nearest or second-nearest anywhere in the tile
    int32_t r2 = (int32_t)v3_isqrt((uint32_t)(d2 > 0 ? d2 : 0)) + 23; r2 *= r2;
    uint8_t* c = gl_cand + t * GL_TC; int m = 0;
    for (int i = 0; i < ns && m < GL_TC; i++) {
      int32_t dx = gl_sx[i] - cx, dy = gl_sy[i] - cy;
      if (dx * dx + dy * dy <= r2) c[m++] = (uint8_t)i;
    }
    gl_nc[t] = (uint8_t)m;
  }

  // ── colours ──
  bool dark = L.dark;
  gl_palette(colours, shimmer, leadCol >= 9 ? 9 : leadCol, dark);
  uint8_t leadPix = leadCol >= 9 ? (uint8_t)0xF0 : (uint8_t)0x80;   // lead: hue 0 (neon: full bright)
  if (leadCol < 9) leadPix = (uint8_t)((leadCol == 0 ? 15 : 12) << 4);
  // interior light: brightest in the middle of each pane
  uint8_t shade[64];                                               // in 1/16 levels, dithered below so there are no rings
  for (int i = 0; i < 64; i++) { float u = i / 63.0f; float lv = 15.0f - glow * 9.0f * u - (1 - glow) * 3.0f * u; if (dark) lv = 4.5f - 2.5f * u; shade[i] = (uint8_t)v3_clamp((int)(lv * 16), 48, 240); }
  // the sunbeam: a soft diagonal band
  float ba = 0.6f + 0.3f * sinf(gl_beamT * 0.37f);
  int bc = (int)(cosf(ba) * 256), bs = (int)(sinf(ba) * 256);
  int bpos = (int)(fmodf(gl_beamT * 160.0f, 760.0f)) - 220;
  int8_t beamT[128];
  for (int i = 0; i < 128; i++) { int d = v3_abs(i - 64); beamT[i] = (int8_t)(d < 40 ? (int)(beam * 6 * (40 - d) / 40) : 0); }
  int lw = (int)(leadW * 1.0f + 0.5f); if (lw < 1) lw = 1;
  int bw = (int)(bevel * 3.0f + 0.5f);
  int32_t lw2 = lw * lw, bw2 = (lw + bw) * (lw + bw);

  gl_field ^= 1;
  // ── pixels, in 2×2 blocks ──
  // The nearest two panes are found once per block. The distance to the line between them changes linearly across the
  // block (d2 − d1 = 2 p·(s1 − s2) + const), so each of the four pixels is classed exactly with a few additions.
  int16_t cxs[GL_TC], cys[GL_TC]; uint8_t chs[GL_TC];
  gl_bevHi = bevel > 0.6f ? 1 : 0;                                  // (a stored int: keeps float maths out of the pixel loop)
  const int bevHi = gl_bevHi;
  const uint32_t nearK = (uint32_t)(bw2 + 2 * (lw + bw) + 2);
  for (int t = 0; t < GL_TW * GL_TH; t++) {
    int tx = (t % GL_TW) * 16, ty = (t / GL_TW) * 16;
    const uint8_t* cand = gl_cand + t * GL_TC; int m = gl_nc[t];
    if (m == 0) { for (int y = ty; y < ty + 16; y++) memset(v3_buf + y * W + tx, leadPix, 16); continue; }
    for (int k = 0; k < m; k++) { cxs[k] = gl_sx[cand[k]]; cys[k] = gl_sy[cand[k]]; chs[k] = gl_sh[cand[k]]; }
    for (int by = ty + (gl_full ? 0 : gl_field * 2); by < ty + 16; by += gl_full ? 2 : 4) {   // interlaced: alternate block rows each frame
      uint8_t* row0 = v3_buf + by * W;
      int8_t bl = beamT[((((tx * bc + by * bs) >> 8) - bpos) & 127)];
      for (int bx = tx; bx < tx + 16; bx += 2) {
        int32_t d1 = 0x7fffffff, d2 = 0x7fffffff; int i1 = 0, i2 = 0;
        for (int k = 0; k < m; k++) {
          int32_t dx = cxs[k] - bx, dy = cys[k] - by, d = dx * dx + dy * dy;
          if (d < d2) { if (d < d1) { d2 = d1; i2 = i1; d1 = d; i1 = k; } else { d2 = d; i2 = k; } }
        }
        int si = d1 >> 6; if (si > 63) si = 63;
        int sh0 = shade[si];
        uint8_t h1 = chs[i1];
        if (m > 1) {
          int32_t ex = cxs[i1] - cxs[i2], ey = cys[i1] - cys[i2];
          uint32_t L2 = (uint32_t)(4 * (ex * ex + ey * ey));
          int32_t diff = d2 - d1;
          if (diff < 0x7fff && (uint32_t)diff * (uint32_t)diff < nearK * L2) {   // a line is near: class each pixel
            uint8_t h2 = chs[i2];
            for (int q = 0; q < 4; q++) {
              int ox = q & 1, oy = q >> 1;
              int32_t dq = diff + 2 * (ox * ex + oy * ey);
              uint8_t hq = h1;
              if (dq < 0) { dq = -dq; hq = h2; }                  // this pixel is actually in the other pane
              uint32_t dd = dq > 46000 ? 0xFFFFFFFFu : (uint32_t)dq * (uint32_t)dq;
              uint8_t pix;
              if (dd < (uint32_t)lw2 * L2) pix = dark ? (uint8_t)(0xF0 | hq) : leadPix;   // (neon glass: the leading glows in the pane's colour)
              else if (dd < (uint32_t)bw2 * L2) pix = (uint8_t)(0xF0 | (bevHi ? 13 : hq));
              else { int lev = ((sh0 + ((ox ^ oy) & 1) * 8) >> 4) + bl; pix = (uint8_t)((lev > 15 ? 15 : lev) << 4 | hq); }
              row0[oy * W + bx + ox] = pix;
            }
            continue;
          }
        }
        // inside a pane: the glow, dithered across the block
        int l0 = (sh0 >> 4) + bl, l1 = ((sh0 + 8) >> 4) + bl;
        uint8_t p0 = (uint8_t)((l0 > 15 ? 15 : l0) << 4 | h1), p1 = (uint8_t)((l1 > 15 ? 15 : l1) << 4 | h1);
        row0[bx] = p0; row0[bx + 1] = p1; row0[W + bx] = p1; row0[W + bx + 1] = p0;
      }
    }
  }
  gl_full = false;
  // nuclei
  if (nuclei > 0.02f) {
    int r = 1 + (int)(nuclei * 4);
    for (int i = 0; i < ns; i++) {
      v3_disc(gl_sx[i], gl_sy[i], r + 1, (uint8_t)(0x30 | gl_sh[i]));
      v3_disc(gl_sx[i], gl_sy[i], r, (uint8_t)(0xF0 | 14));
    }
  }
}

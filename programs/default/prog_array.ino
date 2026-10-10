// =====================================================================
// PROGRAM: ILLUMINATED ARRAY (by Dewey)
// A dusty white circuit map: a maze of straight wires, T-junctions and
// 45-degree short-cuts drawn as grainy point-cloud light, slowly
// rewiring itself (new wires draw themselves in, old ones dissolve),
// with glowing stair-stepped outlines (red, yellow, blue, white) that
// bloom, merge, split and drift across it like heat on a thermal map.
// Inspired by polyhop's "Illuminated Array VIII"
// (https://www.instagram.com/p/DeSYC8PkkIi/). The code is original.
//
// Presets:
//   k0 Array      — the classic look, slowly evolving between moods (p14)
//   k1 Wires      — just the circuit: swinging wires and racing packets
//   k2 Swarm      — lots of small, quick blobs
//   k3 Bloom      — one big hotspot: nested red/yellow/blue/white rings
//   k4 Topo       — the whole screen as a stair-stepped contour map
//   k5 Pixel      — chunky low-res outlines and solid wires
//   k6 Fly-over   — the camera glides and zooms across the circuit
//   k7 Overload   — everything at once
//   k8 Branches   — jointed arms pivoting between angles on the grid
//   k9 Routes     — red and yellow wiring paths stepping across
//   k10 Blueprint — big sparse cells, blue tile grids, red frames with rays
//   k11 Morph     — drifts through every mood (Maze, Blueprint, Circuit, Heat)
//
//   k12 — SURGE: the whole circuit rewires and swings back into place,
//         with a flash and a rush of packets
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8 schemes, centre = Original)  p1 Speed (far left = freeze)
//   p2 Grid (wire spacing, right = denser)      p3 Grain (left = solid, right = dusty)
//   p4 Blobs (how many)   p5 Blob Size   p6 Rewire (far left = frozen network)
//   p7 Diagonals   p8 Pan (centre = still)   p9 Zoom   p10 Glow   p11 Trails
//   p12 Pivot (swinging wires + branch arms)   p13 Sparkle
//   p14 Evolve (Array / Fly-over / Overload / Morph: how fast the mood drifts;
//               far left = hold)   p15 Color Cycle
//
// How it works:
//  - Wires sit on irregular columns and rows (each one nudged by a hash),
//    so every line is perfectly straight and meets others in T-junctions.
//    Whether a wire is lit comes from a hash of (wire, time-slot): no
//    memory needed, and the circuit is endless, so the camera can pan.
//  - Blobs are "metaballs" round a few wandering hotspots. The field is
//    sampled on a coarse grid (red 7 px, yellow 5, blue 3, white 2) and
//    the outline is drawn along the cell edges, which gives the
//    stair-stepped look. Only two rows of cells are kept at a time.
//  - Swinging wires: a run of wire is hinged at one end and rotates in
//    from 90 degrees (or out to 45 and back) using the sine table.
//  - Branches, routes and tiles are rebuilt from hashes every frame too.
//  - Every pixel stores (brightness << 4) | colour layer, and brighter
//    always wins, so the trails fade with one quick pass.
//  All per-pixel work is whole-number maths (the RP2040 has no FPU).
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
static int16_t ia_sinT[256];                    // Q14 sine, 256 steps per turn
static inline int ia_sin(int a) { return ia_sinT[a & 255]; }
static inline int ia_cos(int a) { return ia_sinT[(a + 64) & 255]; }
static inline uint32_t ia_hash(int a, int b) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static inline uint32_t ia_hash3(int a, int b, int c) { return ia_hash((int)ia_hash(a, b), c); }
static inline int ia_floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static inline int ia_clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ─── Colour layers ────────────────────────────────────────────────────
// palette index = (brightness << 4) | layer
#define IA_GRID 0
#define IA_RED  1
#define IA_YEL  2
#define IA_BLU  3
#define IA_WHT  4
#define IA_PKT  5
#define IA_SPK  6
#define IA_SWG  7     // swinging wires
#define IA_ARMA 8     // branch arms (two contrasting colours)
#define IA_ARMB 9
#define IA_AC1  10    // extra accents for routes and tiles
#define IA_AC2  11

#define IA_NSCHEME 8
static const char* const ia_schemeNames[IA_NSCHEME] = {
  "Mono", "Ice", "Matrix", "Amber", "Original", "Neon", "Thermal", "Vapor"
};
// per scheme: grid, red layer, yellow layer, blue layer, white layer, packets, sparkle
// accents per scheme: swinging wires, arm A, arm B, accent 1, accent 2
// (chosen to contrast with each other and with the scheme's wire colour)
static const uint8_t ia_acc[IA_NSCHEME][5][3] = {
  {{150,190,255},{255,200,120},{255,255,255},{190,190,200},{130,140,160}},   // Mono (faint tints)
  {{255,110,220},{255,200,60},{110,255,190},{0,255,255},{180,110,255}},      // Ice
  {{255,50,200},{255,225,0},{0,190,255},{130,255,0},{0,255,170}},            // Matrix
  {{0,200,255},{255,50,120},{255,255,210},{255,140,0},{190,80,255}},         // Amber
  {{0,230,255},{255,0,200},{130,255,50},{255,130,0},{170,80,255}},           // Original
  {{255,230,0},{0,255,120},{255,90,0},{90,150,255},{255,255,255}},           // Neon
  {{0,170,255},{190,0,255},{255,255,255},{255,60,0},{255,205,0}},            // Thermal
  {{255,255,110},{0,255,200},{255,120,80},{140,120,255},{255,170,255}},      // Vapor
};
static const uint8_t ia_cols[IA_NSCHEME][7][3] = {
  {{225,225,232},{255,255,255},{205,205,210},{150,150,165},{255,255,255},{255,255,255},{255,255,255}}, // Mono
  {{200,225,255},{0,200,255},{150,240,255},{40,90,255},{255,255,255},{120,255,255},{220,240,255}},     // Ice
  {{120,220,140},{0,255,90},{180,255,60},{0,200,180},{230,255,230},{200,255,120},{200,255,200}},       // Matrix
  {{255,210,150},{255,70,0},{255,190,0},{255,120,40},{255,240,210},{255,220,120},{255,230,190}},       // Amber
  {{235,235,242},{255,45,45},{255,205,30},{70,120,255},{255,255,255},{255,255,255},{255,255,255}},     // Original
  {{225,215,255},{255,0,170},{0,255,230},{150,60,255},{255,255,255},{255,80,255},{230,220,255}},       // Neon
  {{170,170,190},{255,0,60},{255,140,0},{255,240,80},{255,255,255},{255,200,80},{255,250,220}},        // Thermal
  {{255,215,240},{255,90,190},{80,240,255},{170,90,255},{255,240,140},{120,255,240},{255,230,250}},    // Vapor
};

// cyc: colour-cycle position (0-1023 = once round the four blob colours)
static void ia_buildPalette(int scheme, int cyc, int flash) {
  static const uint8_t gain[12] = {0, 34, 58, 80, 102, 124, 146, 168, 192, 214, 236, 255};
  int step = (cyc >> 8) & 3, f = cyc & 255;
  for (int L = 0; L < 16; L++) {
    int r = 0, g = 0, b = 0;
    bool used = true;
    if (L >= IA_RED && L <= IA_WHT) {          // blob colours rotate with the colour cycle
      int a = IA_RED + ((L - IA_RED + step) & 3), c = IA_RED + ((L - IA_RED + step + 1) & 3);
      r = (ia_cols[scheme][a][0] * (256 - f) + ia_cols[scheme][c][0] * f) >> 8;
      g = (ia_cols[scheme][a][1] * (256 - f) + ia_cols[scheme][c][1] * f) >> 8;
      b = (ia_cols[scheme][a][2] * (256 - f) + ia_cols[scheme][c][2] * f) >> 8;
    } else if (L <= IA_SPK) {
      r = ia_cols[scheme][L][0]; g = ia_cols[scheme][L][1]; b = ia_cols[scheme][L][2];
    } else if (L <= IA_AC2) {                   // accents rotate among themselves too
      int k0 = (L - IA_SWG + step) % 5, k1 = (k0 + 1) % 5;
      r = (ia_acc[scheme][k0][0] * (256 - f) + ia_acc[scheme][k1][0] * f) >> 8;
      g = (ia_acc[scheme][k0][1] * (256 - f) + ia_acc[scheme][k1][1] * f) >> 8;
      b = (ia_acc[scheme][k0][2] * (256 - f) + ia_acc[scheme][k1][2] * f) >> 8;
    } else {
      used = false;
    }
    display.setColor(L, 0, 0, 0);               // brightness 0 = black
    for (int l = 1; l < 16; l++) {
      int rr = 0, gg = 0, bb = 0;
      if (used) {
        if (l <= 11) { int m = gain[l]; rr = r * m >> 8; gg = g * m >> 8; bb = b * m >> 8; }
        else {                                   // hot core: toward white
          int wv = (l - 11) * 40;
          rr = r + ((255 - r) * wv >> 8); gg = g + ((255 - g) * wv >> 8); bb = b + ((255 - b) * wv >> 8);
        }
        if (flash) { rr = (rr + 255 * flash) / (flash + 1); gg = (gg + 255 * flash) / (flash + 1); bb = (bb + 255 * flash) / (flash + 1); }
      }
      display.setColor((l << 4) | L, rr, gg, bb);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* ia_buf;

static inline void ia_putP(uint8_t* p, int layer, int l) {
  if ((*p >> 4) < l) *p = (uint8_t)((l << 4) | layer);
}
static inline void ia_put(int x, int y, int layer, int l) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || l <= 0) return;
  ia_putP(ia_buf + y * W + x, layer, l > 15 ? 15 : l);
}

// Fade every pixel's brightness (high nibble) by dec, four pixels at a time.
static inline uint32_t ia_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;                          // borrow can't cross bytes
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;                // bytes that stayed >= 0
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void ia_fade(int dec) {
  uint32_t* b = (uint32_t*)ia_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = ia_fade4(b[i], d4);
}

// ─── Settings (set each frame from the knobs) ─────────────────────────
static int ia_t = 0;                 // animation clock (ticks, 60 per second at speed 1)
static int ia_rt = 0;                // rewire clock (1/1024 time-slots)
static int ia_frame = 0;
static int ia_seed = 1;
static int ia_G = 45;                // wire spacing (world px)
static int ia_zq = 256;              // zoom (Q8)
static int ia_camX = 0, ia_camY = 0; // camera centre (world px)
static int ia_keep = 170, ia_dust = 20;
static int ia_density = 576, ia_diag = 200, ia_pkt = 170;
static int ia_glow = 3;
static int ia_regrow = 1024;         // k12 surge: 0 → 1024 as the circuit redraws itself
static int ia_shim = 0;              // grain shimmer seed

static inline int ia_sx(int wx) { return (((wx - ia_camX) * ia_zq) >> 8) + HALFW; }
static inline int ia_sy(int wy) { return (((wy - ia_camY) * ia_zq) >> 8) + HALFH; }
static inline int ia_colX(int i) { return i * ia_G + (int)(ia_hash(i * 3 + ia_seed, 11) % (uint32_t)(ia_G * 11 / 20)); }
static inline int ia_rowY(int j) { return j * ia_G + (int)(ia_hash(j * 3 + ia_seed, 12) % (uint32_t)(ia_G * 11 / 20)); }

// ─── Wires ────────────────────────────────────────────────────────────
// Is wire group `grp` of line `id` lit in time-slot e?
static inline bool ia_on(int id, int grp, int e, int thr) {
  return (int)(ia_hash3(id, grp, e * 977 + ia_seed) & 1023) < thr;
}

static uint32_t ia_eHash;            // hash of the last edge looked at (for its packet etc.)
static int ia_eLvl;                  // its brightness level (10..13)
static int ia_ePiv;                  // its swing angle (256 = full turn, 0 = lying on the grid)
static int ia_eAnc;                  // the run end it swings about (column or row index)
static int ia_pivot = 400;           // how many wire runs swing (0..1023)

// dir 0 = horizontal edge on row `line`, between columns pos and pos+1
// dir 1 = vertical edge on column `line`, between rows pos and pos+1
// dir 2 = diagonal in cell (column line, row pos)
// Returns how lit it is (0..256); *g0..*g1 = the part drawn so far (0..256).
static int ia_edge(int dir, int line, int pos, int* g0, int* g1) {
  *g0 = 0; *g1 = 256;
  int L = 1, k = 0, grp = pos;
  if (dir < 2) {                                         // wires come in straight runs of 1-4 edges
    L = 1 + (int)(ia_hash(line * 2 + dir, 21 + ia_seed) & 3);
    int q = pos + (int)(ia_hash(line * 2 + dir, 23) % (uint32_t)L);
    grp = ia_floorDiv(q, L);
    k = q - grp * L;
  }
  int id = line * 3 + dir;
  uint32_t hp = ia_hash3(id, grp, 31 + ia_seed);
  ia_eHash = hp;
  ia_eLvl = 10 + (int)((hp >> 12) % 4u);
  int thr = dir < 2 ? ia_density : ia_diag;
  int v = ia_rt + (int)(hp & 1023);
  int ep = v >> 10, f = v & 1023;
  if (!ia_on(id, grp, ep, thr)) return 0;
  int b = 256;
  if (f > 820 && !ia_on(id, grp, ep + 1, thr)) b = (1023 - f) * 256 / 203;   // dissolving
  int kk = (hp & 2048) ? L - 1 - k : k;                  // which way the run draws itself in
  ia_ePiv = 0;
  if (dir < 2 && (int)((hp >> 22) & 1023) < ia_pivot) {  // a SWINGING run: hinged at one end
    int runStart = grp * L - (int)(ia_hash(line * 2 + dir, 23) % (uint32_t)L);
    ia_eAnc = (hp & 2048) ? runStart + L : runStart;
    int ang = 0;
    if (f < 300 && !ia_on(id, grp, ep - 1, thr)) {        // swings in from 90 degrees
      int u = 300 - f; ang = 64 * u / 300; ang = ang * (u + 300) / 600;   // eases to rest
    }
    if (f > 820 && !ia_on(id, grp, ep + 1, thr)) ang = 64 * (f - 820) / 203;   // swings away
    if (((hp >> 14) & 3) == 0 && f >= 380 && f < 780)     // mid-life: swings out to 45 and back
      ang = 32 * ia_sin((f - 380) * 128 / 400) >> 14;
    if (ia_regrow < 1024) { int a2 = 64 * (1024 - ia_regrow) >> 10; if (a2 > ang) ang = a2; }
    ia_ePiv = (hp & 0x20000000u) ? -ang : ang;
    return b;
  }
  int prog = 1 << 20;
  if (f < 300 && !ia_on(id, grp, ep - 1, thr)) prog = f * L * 256 / 300 - kk * 256;
  if (ia_regrow < 1024) { int p2 = ia_regrow * L / 4 - kk * 256; if (p2 < prog) prog = p2; }
  if (prog <= 0) return 0;
  if (prog < 256) { if (hp & 2048) *g0 = 256 - prog; else *g1 = prog; }
  return b;
}

// one grainy straight line (horizontal or vertical); a = along, c = across
static void ia_wireSpan(bool vert, int a0, int a1, int c, int b, int lvl) {
  if (a0 > a1) { int t = a0; a0 = a1; a1 = t; }
  int lim = vert ? H - 1 : W - 1, climit = vert ? W : H;
  if (a1 < 0 || a0 > lim) return;
  if (c < -4 || c >= climit + 4) return;
  if (a0 < 0) a0 = 0;
  if (a1 > lim) a1 = lim;
  int L = lvl * b >> 8;
  if (L < 1) return;
  int kp = ia_keep * (128 + (b >> 1)) >> 8;              // dissolving wires get dustier
  bool onRow = (unsigned)c < (unsigned)climit;
  for (int a = a0; a <= a1; a++) {
    int x = vert ? c : a, y = vert ? a : c;
    uint32_t r = ia_hash(x * 263 + y * 7919, ia_shim);
    if (onRow && (int)(r & 255) < kp) {
      int l = L - (int)((r >> 8) & 1) - (((r >> 9) & 7) == 0 ? 3 : 0);
      if (l < 1) l = 1;
      ia_putP(ia_buf + y * W + x, IA_GRID, l);
    }
    if (ia_glow >= 2 && onRow && (r & 0x40000000u)) {    // soft halo either side
      if (vert) { ia_put(x - 1, y, IA_GRID, ia_glow - 1); ia_put(x + 1, y, IA_GRID, ia_glow - 1); }
      else      { ia_put(x, y - 1, IA_GRID, ia_glow - 1); ia_put(x, y + 1, IA_GRID, ia_glow - 1); }
    }
    if ((int)((r >> 10) & 255) < ia_dust) {              // point-cloud dust beside the wire
      int o = (int)((r >> 18) & 7) - 3;
      if (o == 0) o = 4;
      int dl = 2 + (int)((r >> 21) & 3);
      if (vert) ia_put(x + o, y, IA_GRID, dl); else ia_put(x, y + o, IA_GRID, dl);
    }
  }
}

// a grainy line at any angle (Bresenham), used by swinging wires and branches
static void ia_gline(int x0, int y0, int x1, int y1, int layer, int L, int kp, int dust) {
  if ((x0 < 0 && x1 < 0) || (x0 >= W && x1 >= W) || (y0 < 0 && y1 < 0) || (y0 >= H && y1 >= H)) return;
  if (L < 1 && dust <= 0) return;
  if (L > 15) L = 15;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, n = 0;
  for (;;) {
    if ((unsigned)x0 < (unsigned)W && (unsigned)y0 < (unsigned)H) {
      uint32_t r = ia_hash(x0 * 263 + y0 * 7919, ia_shim);
      if (L > 0 && (int)(r & 255) < kp) {
        int l = L - (int)((r >> 8) & 1);
        ia_putP(ia_buf + y0 * W + x0, layer, l < 1 ? 1 : l);
      }
      if ((int)((r >> 10) & 255) < dust) ia_put(x0 + (int)((r >> 18) & 7) - 3, y0 + (int)((r >> 21) & 7) - 3, layer, 2 + (int)((r >> 24) & 3));
    }
    if ((x0 == x1 && y0 == y1) || ++n > 1200) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// a solid coloured line at any angle with an optional glow (routes, rays)
static void ia_sline(int x0, int y0, int x1, int y1, int layer, int L, int glow) {
  if ((x0 < -2 && x1 < -2) || (x0 >= W + 2 && x1 >= W + 2) || (y0 < -2 && y1 < -2) || (y0 >= H + 2 && y1 >= H + 2)) return;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, n = 0;
  bool steep = -dy > dx;
  for (;;) {
    ia_put(x0, y0, layer, L);
    if (glow) {
      if (steep) { ia_put(x0 - 1, y0, layer, glow); ia_put(x0 + 1, y0, layer, glow); }
      else       { ia_put(x0, y0 - 1, layer, glow); ia_put(x0, y0 + 1, layer, glow); }
    }
    if ((x0 == x1 && y0 == y1) || ++n > 1200) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// a little hollow square marker (joints, hinges)
static void ia_box(int x, int y, int r, int layer, int l) {
  for (int k = -r; k <= r; k++) {
    ia_put(x + k, y - r, layer, l); ia_put(x + k, y + r, layer, l);
    ia_put(x - r, y + k, layer, l); ia_put(x + r, y + k, layer, l);
  }
}

// a 2-px-thick solid line (the second pass is offset across the line)
static void ia_tline(int x0, int y0, int x1, int y1, int layer, int L, int glow) {
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  ia_sline(x0, y0, x1, y1, layer, L, 0);
  if (steep) ia_sline(x0 + 1, y0, x1 + 1, y1, layer, L, 0);
  else       ia_sline(x0, y0 + 1, x1, y1 + 1, layer, L, 0);
  if (glow) {                                                  // soft halo outside both passes
    if (steep) { ia_sline(x0 - 1, y0, x1 - 1, y1, layer, glow, 0); ia_sline(x0 + 2, y0, x1 + 2, y1, layer, glow, 0); }
    else       { ia_sline(x0, y0 - 1, x1, y1 - 1, layer, glow, 0); ia_sline(x0, y0 + 2, x1, y1 + 2, layer, glow, 0); }
  }
}

// a swinging wire: crisp line in its own colour, a hinge box where it pivots
static void ia_swing(int x0, int y0, int x1, int y1, int ax, int ay, int b) {
  int L = 14 * b >> 8;
  if (L < 1) return;
  int gl = ia_glow > 2 ? ia_glow - 1 : 2;
  ia_sline(x0, y0, x1, y1, IA_SWG, L, L > gl + 2 ? gl : 0);
  ia_box(ax, ay, 1, IA_SWG, L);
  ia_put(ax, ay, IA_SWG, 15 * b >> 8);
}

// rotate (x, y) about (ax, ay) by angle a (256 = full turn)
static inline void ia_rot(int ax, int ay, int a, int* x, int* y) {
  int dx = *x - ax, dy = *y - ay, c = ia_cos(a), sn = ia_sin(a);
  *x = ax + ((dx * c - dy * sn) >> 14);
  *y = ay + ((dx * sn + dy * c) >> 14);
}

// a packet: a bright dash racing along an edge of screen length len
static void ia_packet(bool vert, int a0, int a1, int c, uint32_t hp, int boost) {
  int len = a1 - a0;
  if (len < 4) return;
  if ((int)((hp >> 20) & 1023) >= ia_pkt + boost) return;
  int spd = 1 + (int)((hp >> 7) & 1);
  int cyc = len * 2 + (int)((hp >> 3) % 40u);
  int p = (ia_t * spd + (int)(hp % 997u)) % cyc;
  if (p >= len) return;
  bool rev = (hp >> 30) & 1;
  for (int tl = 0; tl < 6; tl++) {
    int q = rev ? p + tl : p - tl;
    if (q < 0 || q > len) continue;
    int a = rev ? a1 - q : a0 + q;
    int l = 15 - tl * 2;
    if (vert) { ia_put(c, a, IA_PKT, l); if (tl == 0) { ia_put(c - 1, a, IA_PKT, 7); ia_put(c + 1, a, IA_PKT, 7); } }
    else      { ia_put(a, c, IA_PKT, l); if (tl == 0) { ia_put(a, c - 1, IA_PKT, 7); ia_put(a, c + 1, IA_PKT, 7); } }
  }
}

#define IA_MAXGRID 52
static int16_t ia_colS[IA_MAXGRID], ia_rowS[IA_MAXGRID];
// one bit per edge: lit, settled and fully drawn (for the junction nodes)
static uint8_t ia_hFull[(IA_MAXGRID * IA_MAXGRID + 7) / 8], ia_vFull[(IA_MAXGRID * IA_MAXGRID + 7) / 8];
static inline void ia_bitSet(uint8_t* a, int i) { a[i >> 3] |= (uint8_t)(1 << (i & 7)); }
static inline bool ia_bitGet(const uint8_t* a, int i) { return (a[i >> 3] >> (i & 7)) & 1; }

static void ia_drawGrid(int dimQ, int pktBoost, bool nodes) {
  int hwx = (HALFW << 8) / ia_zq + 4, hwy = (HALFH << 8) / ia_zq + 4;
  int i0 = ia_floorDiv(ia_camX - hwx, ia_G) - 1, i1 = ia_floorDiv(ia_camX + hwx, ia_G) + 1;
  int j0 = ia_floorDiv(ia_camY - hwy, ia_G) - 1, j1 = ia_floorDiv(ia_camY + hwy, ia_G) + 1;
  if (i1 - i0 >= IA_MAXGRID) i1 = i0 + IA_MAXGRID - 1;
  if (j1 - j0 >= IA_MAXGRID) j1 = j0 + IA_MAXGRID - 1;
  int ni = i1 - i0 + 1, nj = j1 - j0 + 1;
  for (int i = 0; i < ni; i++) ia_colS[i] = (int16_t)ia_sx(ia_colX(i0 + i));
  for (int j = 0; j < nj; j++) ia_rowS[j] = (int16_t)ia_sy(ia_rowY(j0 + j));
  int g0, g1;
  memset(ia_hFull, 0, sizeof(ia_hFull));
  memset(ia_vFull, 0, sizeof(ia_vFull));

  // horizontal wires (some rows are doubled: a fainter twin 3 px below)
  for (int j = 0; j < nj; j++) {
    int y = ia_rowS[j];
    if (y < -4 || y >= H + 4) continue;
    bool twin = (ia_hash(j0 + j, 51 + ia_seed) % 100u) < 12;
    for (int i = 0; i < ni - 1; i++) {
      int b = ia_edge(0, j0 + j, i0 + i, &g0, &g1);
      if (!b) continue;
      b = b * dimQ >> 8;
      int xa = ia_colS[i], xb = ia_colS[i + 1];
      int s0 = xa + ((xb - xa) * g0 >> 8), s1 = xa + ((xb - xa) * g1 >> 8);
      uint32_t hp = ia_eHash;
      if (ia_ePiv) {                                       // swinging: rotate about the run's end
        int ax = ia_sx(ia_colX(ia_eAnc)), x0 = xa, y0 = y, x1 = xb, y1 = y;
        ia_rot(ax, y, ia_ePiv, &x0, &y0); ia_rot(ax, y, ia_ePiv, &x1, &y1);
        ia_swing(x0, y0, x1, y1, ax, y, b);
        continue;
      }
      if (g0 == 0 && g1 == 256 && b >= 200) ia_bitSet(ia_hFull, j * IA_MAXGRID + i);
      ia_wireSpan(false, s0, s1, y, b, ia_eLvl);
      if (twin) ia_wireSpan(false, s0, s1, y + 3, b * 5 >> 3, ia_eLvl);
      if (g0 == 0 && g1 == 256) ia_packet(false, xa, xb, y, hp, pktBoost);
    }
  }
  // vertical wires
  for (int i = 0; i < ni; i++) {
    int x = ia_colS[i];
    if (x < -4 || x >= W + 4) continue;
    bool twin = (ia_hash(i0 + i, 52 + ia_seed) % 100u) < 10;
    for (int j = 0; j < nj - 1; j++) {
      int b = ia_edge(1, i0 + i, j0 + j, &g0, &g1);
      if (!b) continue;
      b = b * dimQ >> 8;
      int ya = ia_rowS[j], yb = ia_rowS[j + 1];
      int s0 = ya + ((yb - ya) * g0 >> 8), s1 = ya + ((yb - ya) * g1 >> 8);
      uint32_t hp = ia_eHash;
      if (ia_ePiv) {
        int ay = ia_sy(ia_rowY(ia_eAnc)), x0 = x, y0 = ya, x1 = x, y1 = yb;
        ia_rot(x, ay, ia_ePiv, &x0, &y0); ia_rot(x, ay, ia_ePiv, &x1, &y1);
        ia_swing(x0, y0, x1, y1, x, ay, b);
        continue;
      }
      if (g0 == 0 && g1 == 256 && b >= 200) ia_bitSet(ia_vFull, i * IA_MAXGRID + j);
      ia_wireSpan(true, s0, s1, x, b, ia_eLvl);
      if (twin) ia_wireSpan(true, s0, s1, x + 3, b * 5 >> 3, ia_eLvl);
      if (g0 == 0 && g1 == 256) ia_packet(true, ya, yb, x, hp, pktBoost);
    }
  }
  // 45-degree short-cuts across some cells
  if (ia_diag > 0) {
    for (int j = 0; j < nj - 1; j++) {
      for (int i = 0; i < ni - 1; i++) {
        int b = ia_edge(2, i0 + i, j0 + j, &g0, &g1);
        if (!b) continue;
        b = b * dimQ >> 8;
        int cw = ia_colS[i + 1] - ia_colS[i], ch = ia_rowS[j + 1] - ia_rowS[j];
        int n = cw < ch ? cw : ch;
        uint32_t hp = ia_eHash;
        if (((hp >> 14) & 3) == 0) n /= 3;                // a small chamfered corner
        int corner = (int)((hp >> 16) & 3);
        int x0 = (corner & 1) ? ia_colS[i + 1] : ia_colS[i];
        int y0 = (corner & 2) ? ia_rowS[j + 1] : ia_rowS[j];
        int sx = (corner & 1) ? -1 : 1, sy = (corner & 2) ? -1 : 1;
        if (x0 + sx * n < -2 && x0 < -2) continue;
        if (x0 + sx * n > W + 2 && x0 > W + 2) continue;
        int L = ia_eLvl * b >> 8;
        if (L < 1) continue;
        int kp = ia_keep * (128 + (b >> 1)) >> 8;
        int k0 = n * g0 >> 8, k1 = n * g1 >> 8;
        for (int k = k0; k <= k1; k++) {
          int x = x0 + sx * k, y = y0 + sy * k;
          if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) continue;
          uint32_t r = ia_hash(x * 263 + y * 7919, ia_shim);
          if ((int)(r & 255) < kp) ia_putP(ia_buf + y * W + x, IA_GRID, L - (int)((r >> 8) & 3) > 0 ? L - (int)((r >> 8) & 3) : 1);
          if ((int)((r >> 10) & 255) < ia_dust) ia_put(x + (int)((r >> 18) & 7) - 3, y, IA_GRID, 2 + (int)((r >> 21) & 3));
        }
      }
    }
  }
  // junctions: where three or four wires meet, a bright node
  if (nodes) {
    for (int j = 1; j < nj - 1; j++) {
      int y = ia_rowS[j];
      if (y < 0 || y >= H) continue;
      for (int i = 1; i < ni - 1; i++) {
        int x = ia_colS[i];
        if (x < 0 || x >= W) continue;
        int cnt = ia_bitGet(ia_hFull, j * IA_MAXGRID + i - 1) + ia_bitGet(ia_hFull, j * IA_MAXGRID + i)
                + ia_bitGet(ia_vFull, i * IA_MAXGRID + j - 1) + ia_bitGet(ia_vFull, i * IA_MAXGRID + j);
        if (cnt < 3) continue;
        int bm = dimQ;
        int c = 15 * bm >> 8, a = 10 * bm >> 8;
        ia_put(x, y, IA_GRID, c);
        ia_put(x - 1, y, IA_GRID, a); ia_put(x + 1, y, IA_GRID, a);
        ia_put(x, y - 1, IA_GRID, a); ia_put(x, y + 1, IA_GRID, a);
        if (cnt == 4 || (ia_hash(i0 + i, j0 + j) & 7) == 0) {           // a few flare bigger
          int s = 4 * bm >> 8;
          ia_put(x - 1, y - 1, IA_GRID, s); ia_put(x + 1, y - 1, IA_GRID, s);
          ia_put(x - 1, y + 1, IA_GRID, s); ia_put(x + 1, y + 1, IA_GRID, s);
          ia_put(x - 2, y, IA_GRID, s); ia_put(x + 2, y, IA_GRID, s);
          ia_put(x, y - 2, IA_GRID, s); ia_put(x, y + 2, IA_GRID, s);
        }
      }
    }
  }
}

// ─── Blobs ────────────────────────────────────────────────────────────
#define IA_MAXHS 12
#define IA_NOEP (-2000000000)
static int32_t ia_hx[IA_MAXHS], ia_hy[IA_MAXHS], ia_hEp[IA_MAXHS];   // hotspot home (world px), time-slot
static int16_t ia_hsX[IA_MAXHS], ia_hsY[IA_MAXHS], ia_hsE[IA_MAXHS]; // this frame: screen pos, size envelope 0..256
static int ia_nHS = 0;

// move the hotspots: each lives for one time-slot (period ticks), swelling
// up then shrinking away, and is reborn somewhere else in view
static void ia_hotspots(int n, int period, int spreadQ) {
  ia_nHS = n;
  for (int k = 0; k < n; k++) {
    int P = period + (int)(ia_hash(k, 71) % (uint32_t)(period / 2 + 1));
    int v = ia_t + (int)(ia_hash(k, 72) % (uint32_t)P);
    int ep = v / P, ft = v % P;
    int f = ft * 256 / P;                                  // 0..255 through its life
    if (ep != ia_hEp[k]) {
      ia_hEp[k] = ep;
      uint32_t h = ia_hash3(k, ep, 73 + ia_seed);
      int rx = ((HALFW << 8) / ia_zq) * spreadQ >> 8, ry = ((HALFH << 8) / ia_zq) * spreadQ >> 8;
      ia_hx[k] = ia_camX + (int)(h % (uint32_t)(2 * rx + 1)) - rx;
      ia_hy[k] = ia_camY + (int)((h >> 11) % (uint32_t)(2 * ry + 1)) - ry;
    }
    uint32_t h2 = ia_hash3(k, ia_hEp[k], 74 + ia_seed);
    int e = ia_sin(f >> 1) >> 6;                           // 0..256, half a sine
    e = e * 3 >> 1; if (e > 256) e = 256;                  // flat top
    int dvx = (int)(h2 & 63) - 32, dvy = (int)((h2 >> 6) & 63) - 32;   // drifts this far over its life
    int wx = ia_hx[k] + (dvx * f >> 7) + (10 * ia_sin(ia_t / 3 + (int)(h2 >> 12)) >> 14);
    int wy = ia_hy[k] + (dvy * f >> 7) + (10 * ia_cos(ia_t / 4 + (int)(h2 >> 20)) >> 14);
    ia_hsX[k] = (int16_t)ia_clampi(ia_sx(wx), -2000, 2000);
    ia_hsY[k] = (int16_t)ia_clampi(ia_sy(wy), -2000, 2000);
    ia_hsE[k] = (int16_t)e;
  }
}

#define IA_MAXB 72
static int16_t ia_bx[IA_MAXB], ia_by[IA_MAXB], ia_bR[IA_MAXB];
static int32_t ia_bR2[IA_MAXB], ia_bInv[IA_MAXB];
static int ia_nb = 0;

static void ia_addBall(int x, int y, int R) {
  if (R < 2 || ia_nb >= IA_MAXB) return;
  if (x + R < 0 || x - R >= W || y + R < 0 || y - R >= H) return;
  ia_bx[ia_nb] = (int16_t)x; ia_by[ia_nb] = (int16_t)y; ia_bR[ia_nb] = (int16_t)R;
  ia_bR2[ia_nb] = R * R; ia_bInv[ia_nb] = (1 << 24) / (R * R);
  ia_nb++;
}

// balls of one colour layer round every hotspot
// per layer:              -   red  yel  blu  wht
static const uint8_t ia_lN[5]    = {0, 4,   3,   7,   3};
static const uint8_t ia_lRmin[5] = {0, 20,  14,  6,   3};
static const uint8_t ia_lRrng[5] = {0, 22,  14,  6,   3};
static const uint8_t ia_lOff[5]  = {0, 40,  44,  34,  40};

static void ia_layerBalls(int L, int sizeQ) {
  ia_nb = 0;
  for (int k = 0; k < ia_nHS; k++) {
    if (ia_hsE[k] <= 8) continue;
    int ep = (int)ia_hEp[k];
    int cx = ia_hsX[k], cy = ia_hsY[k];
    if (L == IA_BLU) {                                     // blue: a tight cluster of little cells
      uint32_t hc = ia_hash3(k * 8 + L, ep, 81 + ia_seed);
      int a = (int)(hc & 255) + ia_t / 5, d = ia_lOff[L] * (int)((hc >> 8) & 255) >> 8;
      cx += (d * ia_cos(a) >> 14) * ia_zq >> 8;
      cy += (d * ia_sin(a) >> 14) * ia_zq >> 8;
    }
    for (int s = 0; s < ia_lN[L]; s++) {
      uint32_t h = ia_hash3(k * 8 + L, s, ep * 31 + ia_seed);
      int w = (int)((h >> 8) & 7) - 3;
      int a = (int)(h & 255) + (ia_t * w >> 3);
      int d = (L == IA_BLU ? 17 : ia_lOff[L]) * (int)((h >> 12) & 255) >> 8;
      d = d * (200 + (56 * ia_sin(ia_t / 2 + (int)(h >> 20)) >> 14)) >> 8;     // breathing
      int R = ia_lRmin[L] + (int)((h >> 24) % ia_lRrng[L]);
      R = R * (224 + (32 * ia_sin(ia_t * 2 / 3 + (int)(h >> 4)) >> 14)) >> 8;   // pulsing
      R = R * sizeQ >> 8;
      R = R * ia_hsE[k] >> 8;
      R = R * ia_zq >> 8;
      int x = cx + ((d * ia_cos(a) >> 14) * ia_zq >> 8);
      int y = cy + ((d * ia_sin(a) >> 14) * ia_zq >> 8);
      ia_addBall(x, y, R);
    }
  }
}

// big soft balls for the Bloom and Topo presets (one field, many rings)
static void ia_bigBalls(int sizeQ, int nPer, int rMin, int rRng, int off) {
  ia_nb = 0;
  for (int k = 0; k < ia_nHS; k++) {
    if (ia_hsE[k] <= 8) continue;
    for (int s = 0; s < nPer; s++) {
      uint32_t h = ia_hash3(k * 8 + 7, s, (int)ia_hEp[k] * 31 + ia_seed);
      int w = (int)((h >> 8) & 7) - 3;
      int a = (int)(h & 255) + (ia_t * w >> 3);
      int d = (s == 0 ? 0 : off) * (int)((h >> 12) & 255) >> 8;
      d = d * (190 + (66 * ia_sin(ia_t / 2 + (int)(h >> 20)) >> 14)) >> 8;
      int R = rMin + (int)((h >> 24) % (uint32_t)rRng);
      R = R * (220 + (36 * ia_sin(ia_t * 2 / 3 + (int)(h >> 4)) >> 14)) >> 8;
      R = R * sizeQ >> 8;
      R = R * ia_hsE[k] >> 8;
      R = R * ia_zq >> 8;
      ia_addBall(ia_hsX[k] + ((d * ia_cos(a) >> 14) * ia_zq >> 8),
                 ia_hsY[k] + ((d * ia_sin(a) >> 14) * ia_zq >> 8), R);
    }
  }
}

// outline segment along a cell edge (with optional glow either side)
static void ia_cseg(bool vert, int a0, int c, int len, int layer, int lvl, int glow) {
  for (int a = a0; a <= a0 + len; a++) {
    if (vert) {
      ia_put(c, a, layer, lvl);
      if (glow) { ia_put(c - 1, a, layer, glow); ia_put(c + 1, a, layer, glow); }
    } else {
      ia_put(a, c, layer, lvl);
      if (glow) { ia_put(a, c - 1, layer, glow); ia_put(a, c + 1, layer, glow); }
    }
  }
}

// Sample the field of the current balls on a grid of c-px cells and draw
// the boundary of the "inside" cells: a stair-stepped contour.
#define IA_ROWMAX 172
static uint8_t ia_rowA[IA_ROWMAX], ia_rowB[IA_ROWMAX];

static void ia_contour(int c, int thr, int layer, int lvl, int glow) {
  if (ia_nb <= 0) return;
  int minx = W, maxx = 0, miny = H, maxy = 0;
  for (int b = 0; b < ia_nb; b++) {
    if (ia_bx[b] - ia_bR[b] < minx) minx = ia_bx[b] - ia_bR[b];
    if (ia_bx[b] + ia_bR[b] > maxx) maxx = ia_bx[b] + ia_bR[b];
    if (ia_by[b] - ia_bR[b] < miny) miny = ia_by[b] - ia_bR[b];
    if (ia_by[b] + ia_bR[b] > maxy) maxy = ia_by[b] + ia_bR[b];
  }
  minx = ia_clampi(minx, -c, W + c); maxx = ia_clampi(maxx, -c, W + c);
  miny = ia_clampi(miny, -c, H + c); maxy = ia_clampi(maxy, -c, H + c);
  // each layer's cell grid is shifted a little so their stair-steps don't all line up
  int ox = (layer * 3 + 1) % c, oy = (layer * 5 + 2) % c;
  minx -= ox; maxx -= ox; miny -= oy; maxy -= oy;
  int i0 = ia_floorDiv(minx, c) - 1, i1 = maxx / c + 1, j0 = ia_floorDiv(miny, c) - 1, j1 = maxy / c + 1;
  int ni = i1 - i0 + 1;
  if (ni > IA_ROWMAX) { ni = IA_ROWMAX; i1 = i0 + ni - 1; }
  uint8_t* prev = ia_rowA;
  uint8_t* cur = ia_rowB;
  memset(prev, 0, ni);
  uint8_t list[IA_MAXB];
  for (int j = j0; j <= j1 + 1; j++) {
    int yc = j * c + (c >> 1) + oy;
    int nl = 0;
    if (j > j0 && j < j1) {
      for (int b = 0; b < ia_nb; b++) {
        int dy = yc - ia_by[b];
        if (dy < ia_bR[b] && dy > -ia_bR[b]) list[nl++] = (uint8_t)b;
      }
    }
    cur[0] = 0; cur[ni - 1] = 0;
    for (int ii = 1; ii < ni - 1; ii++) {
      uint8_t in = 0;
      if (nl) {
        int xc = (i0 + ii) * c + (c >> 1) + ox;
        int sum = 0;
        for (int q = 0; q < nl; q++) {
          int b = list[q];
          int dx = xc - ia_bx[b];
          int R = ia_bR[b];
          if (dx >= R || dx <= -R) continue;
          int dy = yc - ia_by[b];
          int t = ia_bR2[b] - (dx * dx + dy * dy);
          if (t <= 0) continue;
          int v = (int)(((uint32_t)t * (uint32_t)ia_bInv[b]) >> 16);   // 0..256
          sum += v * v >> 8;
          if (sum >= thr) { in = 1; break; }
        }
      }
      cur[ii] = in;
    }
    int y = j * c + oy;
    for (int ii = 0; ii < ni; ii++) {
      int x = (i0 + ii) * c + ox;
      if (cur[ii] != prev[ii]) ia_cseg(false, x, y, c, layer, lvl, glow);
      if (ii > 0 && cur[ii] != cur[ii - 1]) ia_cseg(true, y, x, c, layer, lvl, glow);
    }
    uint8_t* tmp = prev; prev = cur; cur = tmp;
  }
}

// ─── Branches: jointed arms that pivot between angles ─────────────────
// Each arm is rooted on a wire junction and is a chain of 2-4 segments.
// Every joint swings to a new multiple of 45 degrees now and then and
// eases to rest there, so the whole branch folds and unfolds like a
// robot arm. Some arms fork. Nothing is stored: it all comes from hashes.
static int ia_shortA(int d) { return ((d + 128) & 255) - 128; }    // shortest turn

static void ia_arms(int n) {
  int Gs = ia_G * ia_zq >> 8;
  if (Gs < 6) Gs = 6;
  for (int a = 0; a < n; a++) {
    int P = 420 + (int)(ia_hash(a, 101) % 240u);
    int v = ia_t + (int)(ia_hash(a, 102) % (uint32_t)P);
    int ep = v / P, f = (v % P) * 256 / P;                       // life 0..255
    uint32_t h = ia_hash3(a, ep, 103 + ia_seed);
    int ci = ia_floorDiv(ia_camX, ia_G) + (int)(h % 9u) - 4;
    int rj = ia_floorDiv(ia_camY, ia_G) + (int)((h >> 4) % 7u) - 3;
    int x = ia_sx(ia_colX(ci)), y = ia_sy(ia_rowY(rj));
    int env = f < 40 ? f * 256 / 40 : (f > 215 ? (255 - f) * 256 / 40 : 256);
    int layer = (h >> 28) & 1 ? IA_ARMB : IA_ARMA;              // two contrasting arm colours
    int lvl = 13 * env >> 8;
    if (lvl < 2) continue;
    int gl = ia_glow > 0 ? (ia_glow > 4 ? 4 : ia_glow) : 0;
    int ns = 2 + (int)((h >> 8) % 3u);
    int grow = f < 60 ? f * ns * 256 / 60 : ns * 256;          // segments unfold one by one
    int ang = 0, forkX = 0, forkY = 0, forkA = 0;
    bool fork = ((h >> 12) & 1) != 0;
    for (int q = -1; q <= 1; q++) { ia_put(x + q, y - 1, layer, lvl + 2); ia_put(x + q, y, layer, lvl + 2); ia_put(x + q, y + 1, layer, lvl + 2); }   // root
    for (int s = 0; s < ns; s++) {
      uint32_t hs = ia_hash3(a * 8 + s, ep, 104 + ia_seed);
      int len = Gs * (110 + (int)(hs % 150u)) >> 8;
      int Tj = 60 + (int)((hs >> 8) % 70u);                      // ticks between moves
      int u = ia_t + (int)(hs % (uint32_t)Tj);
      int k = u / Tj, fr = (u % Tj) * 256 / Tj;
      int t0 = (int)(ia_hash3(a * 8 + s, k, ep) % 8u) * 32;
      int t1 = (int)(ia_hash3(a * 8 + s, k + 1, ep) % 8u) * 32;
      if (s > 0) { t0 = (t0 % 160) - 64; t1 = (t1 % 160) - 64; }  // joints bend at most 90
      int e = fr < 110 ? fr * 256 / 110 : 256;
      e = e * e * (768 - 2 * e) >> 16;                             // smoothstep: eases in and out
      ang += t0 + (ia_shortA(t1 - t0) * e >> 8);
      int gs = grow - s * 256;
      if (gs <= 0) break;
      if (gs < 256) len = len * gs >> 8;
      int ex = x + (len * ia_cos(ang) >> 14), ey = y + (len * ia_sin(ang) >> 14);
      ia_tline(x, y, ex, ey, layer, lvl, gl);                      // 2 px thick, crisp
      if (s == 0 && fork) { forkX = ex; forkY = ey; forkA = ang + ((hs & 1) ? 64 : -64) + ia_shortA(t1 - t0) / 2; }
      x = ex; y = ey;
      ia_box(x, y, 2, layer, lvl + 2);                              // joint: a hollow box with a bright core
      ia_put(x, y, layer, 15);
    }
    if (fork && grow > 512) {
      int len = Gs * 3 / 4;
      int ex = forkX + (len * ia_cos(forkA) >> 14), ey = forkY + (len * ia_sin(forkA) >> 14);
      ia_sline(forkX, forkY, ex, ey, layer, lvl - 1, gl > 1 ? gl - 1 : 0);
      ia_box(ex, ey, 1, layer, lvl);
    }
  }
}

// ─── Routes: open red / yellow wiring paths that step across the screen ─
// A route is a walk on a small grid (mostly straight, sometimes turning,
// now and then a 45-degree jog). Its head grows forward and its tail
// follows, like a snake being drawn.
static const int8_t ia_d8x[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const int8_t ia_d8y[8] = {0, 1, 1, 1, 0, -1, -1, -1};

static void ia_routes(int n) {
  for (int r = 0; r < n; r++) {
    int P = 360 + (int)(ia_hash(r, 111) % 300u);
    int v = ia_t + (int)(ia_hash(r, 112) % (uint32_t)P);
    int ep = v / P, ft = v % P;
    uint32_t h = ia_hash3(r, ep, 113 + ia_seed);
    int x, y;
    if (ia_nHS > 0 && (h & 1)) { int k = (int)((h >> 1) % (uint32_t)ia_nHS); x = ia_hsX[k]; y = ia_hsY[k]; }
    else { x = 20 + (int)((h >> 3) % (uint32_t)(W - 40)); y = 20 + (int)((h >> 12) % (uint32_t)(H - 40)); }
    int c = 6 + (int)((h >> 20) % 6u);                             // grid step (px)
    int steps = 10 + (int)((h >> 24) % 14u);
    int d = (int)((h >> 5) & 3) * 2;                              // start axis-aligned
    static const uint8_t rl[4] = {IA_RED, IA_YEL, IA_AC1, IA_AC2};
    int layer = rl[r & 3];
    int head = ft * (steps + steps / 2) * 2 / P;                  // grows over the first ~2/3 of its life
    int tail = head - steps;
    if (head > steps) head = steps;
    for (int s = 0; s < steps; s++) {
      uint32_t hs = ia_hash3(r * 64 + s, ep, 114);
      int len = 1 + (int)(hs % 4u);
      int turn = (int)((hs >> 4) & 7);
      if (d & 1) d = (d + ((hs >> 8) & 1 ? 1 : 7)) & 7;           // after a jog, back onto the grid
      else if (turn == 4) d = (d + 2) & 7;
      else if (turn == 5) d = (d + 6) & 7;
      else if (turn == 6) { d = (d + 1) & 7; len = 1 + (len >> 1); }
      else if (turn == 7) { d = (d + 7) & 7; len = 1 + (len >> 1); }
      int ex = x + ia_d8x[d] * len * c, ey = y + ia_d8y[d] * len * c;
      if (ex < -40 || ex > W + 40 || ey < -40 || ey > H + 40) {   // keep near the screen: turn back
        d = (d + 4) & 7; ex = x + ia_d8x[d] * len * c; ey = y + ia_d8y[d] * len * c;
      }
      if (s >= tail && s < head) ia_sline(x, y, ex, ey, layer, 13, ia_glow > 0 ? (ia_glow > 3 ? 3 : ia_glow) : 0);
      x = ex; y = ey;
    }
  }
}

// ─── Tiles: blue cell grids, little white squares, red frames with rays ─
static void ia_rect(int x, int y, int w, int h, int layer, int l, int glow) {
  ia_cseg(false, x, y, w, layer, l, glow);
  ia_cseg(false, x, y + h, w, layer, l, glow);
  ia_cseg(true, y, x, h, layer, l, glow);
  ia_cseg(true, y, x + w, h, layer, l, glow);
}

static void ia_tiles(int n) {
  int gl = ia_glow > 2 ? 2 : ia_glow;
  for (int p = 0; p < n; p++) {
    int P = 300 + (int)(ia_hash(p, 121) % 260u);
    int v = ia_t + (int)(ia_hash(p, 122) % (uint32_t)P);
    int ep = v / P, f = (v % P) * 256 / P;
    int env = ia_sin(f >> 1) >> 6;                                 // 0..256..0 over its life
    uint32_t h = ia_hash3(p, ep, 123 + ia_seed);
    int x0, y0;
    if (ia_nHS > 0 && (h & 1)) { int k = (int)((h >> 1) % (uint32_t)ia_nHS); x0 = ia_hsX[k] - 20; y0 = ia_hsY[k] - 15; }
    else { x0 = 10 + (int)((h >> 3) % (uint32_t)(W - 60)); y0 = 10 + (int)((h >> 12) % (uint32_t)(H - 50)); }
    int kind = (int)((h >> 20) & 3);
    int tl = (h >> 30) & 1 ? IA_AC2 : IA_BLU;                     // tile colour
    int fl = (h >> 2) & 1 ? IA_AC1 : IA_RED;                      // frame colour
    if (kind <= 1) {                                               // blue cell grid
      int cols = 3 + (int)((h >> 22) % 4u), rows = 2 + (int)((h >> 25) % 4u);
      int s = 8 + (int)((h >> 28) % 5u);
      for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
          uint32_t hc = ia_hash3(p * 64 + r * 8 + c, ep, 124);
          if ((hc & 255) > 190) continue;                          // some cells missing
          if ((int)((hc >> 8) & 255) > env) continue;              // cells appear and vanish one by one
          ia_rect(x0 + c * s, y0 + r * s, s, s, tl, 12, gl);
          if (((hc >> 16) & 15) == 0) ia_rect(x0 + c * s + 2, y0 + r * s + 2, s - 4, s - 4, IA_WHT, 14, 0);
        }
    } else if (kind == 2) {                                        // cluster of little white squares
      int m = 4 + (int)((h >> 22) % 6u);
      for (int q = 0; q < m; q++) {
        uint32_t hq = ia_hash3(p * 64 + q, ep, 125);
        if ((int)((hq >> 8) & 255) > env) continue;
        int s = 4 + (int)(hq % 5u);
        ia_rect(x0 + (int)((hq >> 16) % 34u), y0 + (int)((hq >> 24) % 26u), s, s, IA_WHT, 14, 0);
      }
    } else {                                                       // red frame with a turning ray
      int w = 40 + (int)((h >> 22) % 70u), hh = 26 + (int)((h >> 26) % 50u);
      int l = 12 * env >> 8;
      if (l < 1) continue;
      ia_rect(x0, y0, w, hh, fl, l, gl);
      int cx = (h & 0x40000000u) ? x0 + w : x0, cy = (h & 0x80000000u) ? y0 + hh : y0;
      int base = (int)((h >> 9) & 3) * 64;
      int a = base + (24 * ia_sin(ia_t + (int)(h >> 14)) >> 14);  // the ray pivots back and forth
      int len = 60 + (int)((h >> 13) % 140u);
      ia_sline(cx, cy, cx + (len * ia_cos(a) >> 14), cy + (len * ia_sin(a) >> 14), fl, l, 0);
    }
  }
}

// ─── Moods: what Evolve drifts between ────────────────────────────────
// (a plain table, not a struct: the Arduino builder's auto-prototypes can't see struct types)
enum { IA_MZOOM, IA_MDIAG, IA_MDENS, IA_MPIVOT, IA_MBLOBS, IA_MARMS, IA_MROUTES, IA_MTILES, IA_MN };
static const int16_t ia_moods[4][IA_MN] = {
  //  zoom diag dens pivot blobs arms routes tiles      (zoom, blobs: 256 = 1x)
  {   190, 380, 660, 700, 256,  5,   2,     1 },        // Maze: dense, many diagonals, swinging
  {   420,  60, 520, 260,  90,  2,   3,     4 },        // Blueprint: big sparse cells, tiles, frames
  {   260, 200, 600, 450, 150,  3,   7,     2 },        // Circuit: wiring routes everywhere
  {   240, 260, 560, 560, 340,  6,   2,     2 },        // Heat: big blobs and branches
};

// ─── Sparkle: glinting dots and little dot-grid patches ───────────────
static void ia_sparkles(int amt) {
  if (amt <= 0) return;
  int n = amt * 70 >> 10;
  for (int s = 0; s < n; s++) {
    int v = ia_t + s * 37;
    uint32_t h = ia_hash3(s, v / 40, 91 + ia_seed);
    int f = v % 40;
    int l = 15 - (f > 20 ? f - 20 : 20 - f) * 3 / 4;
    int x = (int)(h % (uint32_t)W), y = (int)((h >> 9) % (uint32_t)H);
    ia_put(x, y, IA_SPK, l);
    if (l > 12 && (h & 0x80000000u)) {
      ia_put(x - 1, y, IA_SPK, l - 7); ia_put(x + 1, y, IA_SPK, l - 7);
      ia_put(x, y - 1, IA_SPK, l - 7); ia_put(x, y + 1, IA_SPK, l - 7);
    }
  }
  // dot grids
  int patches = 1 + (amt >> 9);
  for (int p = 0; p < patches; p++) {
    int P = 240, v = ia_t + p * 97;
    uint32_t h = ia_hash3(p, v / P, 92 + ia_seed);
    if ((h & 3) == 0) continue;
    int f = (v % P) * 256 / P;
    int e = ia_sin(f >> 1) >> 10;                            // 0..16
    int cols = 4 + (int)((h >> 2) % 5u), rows = 3 + (int)((h >> 5) % 4u);
    int px = (int)((h >> 8) % (uint32_t)(W - 40)) + 8, py = (int)((h >> 17) % (uint32_t)(H - 40)) + 8;
    for (int r = 0; r < rows; r++)
      for (int c = 0; c < cols; c++) {
        uint32_t hd = ia_hash(p * 64 + r * 8 + c, ia_t / 6);
        int l = e * 9 / 16 - (int)(hd & 3);
        ia_put(px + c * 4, py + r * 4, IA_SPK, l);
      }
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float ia_sm[16];
static bool ia_smReady = false;
static inline int ia_k(int i) { int v = (int)ia_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_array_name() { return "ILLUMINATED ARRAY"; }
const char* prog_array_character() { return "Grainy circuit-map wires that swing and rewire, pivoting branches, wiring routes and stair-step colour blobs"; }

static const char* const ia_presetNames[] = {
  "Array", "Wires", "Swarm", "Bloom", "Topo", "Pixel", "Fly-over", "Overload",
  "Branches", "Routes", "Blueprint", "Morph"
};
#define IA_NUM_PRESETS 12

const char* prog_array_presetName(int preset) {
  if (preset >= 0 && preset < IA_NUM_PRESETS) return ia_presetNames[preset];
  return NULL;
}

static const char* const ia_labels[16] = {
  "Palette", "Speed", "Grid", "Grain", "Blobs", "Blob Size", "Rewire", "Diagonals",
  "Pan", "Zoom", "Glow", "Trails", "Pivot", "Sparkle", "Evolve", "Color Cycle"
};
const char* prog_array_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return ia_labels[pot];
  return "";
}

uint8_t prog_array_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we clear or fade the last frame ourselves (trails)
}

void prog_array_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    for (int i = 0; i < 256; i++) ia_sinT[i] = (int16_t)lroundf(sinf(i * 6.2831853f / 256.0f) * 16384.0f);
  }
  for (int k = 0; k < IA_MAXHS; k++) ia_hEp[k] = IA_NOEP;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);   // start from black
}

void prog_array_draw(int preset) {
  if (ia_sinT[64] == 0) prog_array_init();
  ia_buf = display.getBuffer();
  if (!ia_buf) return;
  if (preset < 0 || preset >= IA_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!ia_smReady) ia_sm[i] = pots[i];
      else ia_sm[i] += (pots[i] - ia_sm[i]) * a;
    }
    ia_smReady = true;
  }
  ia_frame++;

  // clocks: p1 speed (far left = freeze), p6 rewire rate
  float sk = ia_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.25f * powf(12.0f, (sk - 0.03f) / 0.97f);
  static float tf = 0, rtf = 0, camXf = 0, camYf = 0, zoomPh = 0, moodF = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e8f) tf = 0;
  ia_t = (int)tf;
  float rk = ia_sm[6] / 1023.0f;
  float rate = (rk < 0.03f) ? 0.0f : 0.03f * powf(30.0f, (rk - 0.03f) / 0.97f);   // time-slots per second
  rtf += dt * rate * (speed > 0 ? (0.5f + 0.5f * speed) : 0.0f);
  if (rtf > 1.0e6f) rtf = 0;
  ia_rt = (int)(rtf * 1024.0f);
  ia_shim = ia_frame / 3;

  // mood: Evolve (p14) drifts through the four moods; far left = stay put.
  // Each preset starts from its own mood.
  static const uint8_t baseMood[IA_NUM_PRESETS] = {0, 0, 3, 3, 3, 2, 2, 0, 0, 2, 1, 0};
  float ek = ia_sm[14] / 1023.0f;
  bool evolves = (preset == 0 || preset == 6 || preset == 7 || preset == 11);
  if (preset == 11 && ek < 0.25f) ek = 0.25f;                   // Morph always morphs
  if (evolves && ek > 0.03f) moodF += dt * speed * (0.025f + ek * ek * 0.2f);   // moods per second
  if (moodF > 4000.0f) moodF -= 4000.0f;
  int mood[IA_MN];
  {
    int mi = evolves ? (int)moodF : 0;
    int mfr = evolves ? (int)((moodF - (int)moodF) * 256.0f) : 0;
    int w = mfr < 150 ? 0 : (mfr - 150) * 256 / 106;            // hold, then cross-fade
    w = w * w * (768 - 2 * w) >> 16;
    int m0 = (baseMood[preset] + mi) & 3, m1 = (m0 + 1) & 3;
    for (int q = 0; q < IA_MN; q++) mood[q] = ia_moods[m0][q] + (((ia_moods[m1][q] - ia_moods[m0][q]) * w + 128) >> 8);
  }

  // knobs
  ia_G = 70 - ia_k(2) * 50 / 1023;                                // 70 … 20 world px
  int gk = ia_k(3);
  ia_keep = 256 - gk * 120 / 1023;                                // solid … very dusty
  ia_dust = gk * 48 / 1023;
  ia_density = mood[IA_MDENS];
  ia_diag = ia_k(7) * mood[IA_MDIAG] / 512;                            // knob centre = the mood's amount
  ia_glow = ia_k(10) * 7 / 1024;
  int pivK = ia_k(12);
  ia_pivot = pivK * mood[IA_MPIVOT] / 1024;                      // fewer swing at once: cleaner
  if (ia_pivot > 1000) ia_pivot = 1000;
  ia_pkt = 0;
  int sizeQ = 96 + ia_k(5) * 320 / 1023;                          // 0.4 … 1.6
  int blobsK = ia_k(4);
  int sparkle = ia_k(13);
  int trails = ia_k(11);

  // zoom (centre ≈ 1×, times the mood's zoom) and pan (centre = still)
  float zk = (ia_sm[9] - 512.0f) / 512.0f;
  float zoom = powf(2.0f, zk * 1.3f) * (evolves ? mood[IA_MZOOM] / 256.0f : 1.0f);
  float pk = (ia_sm[8] - 512.0f) / 512.0f;
  if (pk > -0.06f && pk < 0.06f) pk = 0;
  float pv = pk * pk * pk * 140.0f;                               // world px per second
  if (preset == 6) {                                              // Fly-over: always gliding
    pv += 45.0f;
    zoomPh += dt * speed * 0.35f;
    zoom *= 1.0f + 0.3f * sinf(zoomPh);
  }
  float dirA = 0.45f + 0.5f * sinf(tf / 2400.0f);
  camXf += dt * speed * pv * cosf(dirA);
  camYf += dt * speed * pv * sinf(dirA) * 0.6f;
  if (camXf > 1.0e6f || camXf < -1.0e6f) camXf = 0;
  if (camYf > 1.0e6f || camYf < -1.0e6f) camYf = 0;
  ia_camX = (int)floorf(camXf);
  ia_camY = (int)floorf(camYf);
  ia_zq = (int)(zoom * 256.0f);
  if (ia_zq < 64) ia_zq = 64;
  if (ia_zq > 900) ia_zq = 900;

  // per-preset character
  int nHS = 5 * mood[IA_MBLOBS] >> 8, period = 420, spread = 220;
  int arms = mood[IA_MARMS], routes = mood[IA_MROUTES], tiles = mood[IA_MTILES];
  int cellR = 7, cellY = 5, cellB = 3, cellW = 2;
  int gridDim = 256, pktBoost = 0;
  bool nested = false;
  switch (preset) {
    case 1: nHS = 0; pktBoost = 300; arms = 3; routes = 0; tiles = 0; break;           // Wires
    case 2: nHS = 10; period = 220; sizeQ = sizeQ * 3 / 5; arms = 1; routes = 3; tiles = 2; break; // Swarm
    case 3: nHS = 1; period = 900; spread = 40; nested = true; gridDim = 200; arms = 2; routes = 0; tiles = 0; break; // Bloom
    case 4: nHS = 8; period = 1100; spread = 300; nested = true; gridDim = 170; arms = 0; routes = 0; tiles = 0; break; // Topo
    case 5: cellR = 12; cellY = 9; cellB = 6; cellW = 4; ia_keep = 256; ia_dust = 0; arms = 2; routes = 3; tiles = 2; break; // Pixel
    case 6: nHS = 4 * mood[IA_MBLOBS] >> 8; break;                                          // Fly-over
    case 7: nHS = 11; ia_G = ia_G * 7 / 10; pktBoost = 250; if (sparkle < 600) sparkle = 600;
            if (ia_density < 700) ia_density = 700; arms += 4; routes += 4; tiles += 2; break; // Overload
    case 8: nHS = 2; ia_density = 430; gridDim = 190; arms = 14; routes = 0; tiles = 0; // Branches
            ia_pivot = ia_pivot * 3 / 2 + 150; if (ia_pivot > 1000) ia_pivot = 1000; break;
    case 9: nHS = 4; sizeQ = sizeQ * 3 / 4; arms = 1; routes = 12; tiles = 2; gridDim = 210; break; // Routes
    case 10: nHS = 2; ia_G = ia_G * 3 / 2; arms = 2; routes = 3; tiles = 6; break;     // Blueprint
    default: break;
  }
  if (ia_G < 14) ia_G = 14;
  nHS = (nHS * blobsK + 256) >> 9;                                 // knob centre = preset's own number
  if (preset == 3 && blobsK > 600) nHS = 1 + (blobsK - 600) / 220; // Bloom: up to 2-3 blooms
  if (nHS > IA_MAXHS) nHS = IA_MAXHS;
  if (preset == 1) nHS = 0;
  arms = arms * pivK / 512;                                        // Pivot knob scales the branches too
  if (arms > 20) arms = 20;

  // k12: SURGE — rewire everything, swing it back in, flash, packets everywhere
  static bool k12Was = false;
  static int flash = 0, surgeTicks = 0;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) {
    ia_seed = (int)(ia_hash((int)now, ia_seed) & 0xFFFF) + 1;
    ia_regrow = 0;
    flash = 6;
    surgeTicks = 150;
    for (int k = 0; k < IA_MAXHS; k++) ia_hEp[k] = IA_NOEP;
  }
  k12Was = k12;
  if (ia_regrow < 1024) { ia_regrow += (int)(dt * 900.0f) + 1; if (ia_regrow > 1024) ia_regrow = 1024; }
  if (surgeTicks > 0) { surgeTicks--; pktBoost += 700; }

  // colours: p0 scheme (centre = Original), p15 colour cycle (real time)
  int scheme = ia_k(0) * IA_NSCHEME / 1024;
  if (scheme > IA_NSCHEME - 1) scheme = IA_NSCHEME - 1;
  static float cycF = 0;
  float ck = ia_sm[15] / 1023.0f;
  if (ck > 0.03f) cycF += dt * ck * ck * 600.0f;
  if (cycF > 1.0e6f) cycF = 0;
  ia_buildPalette(scheme, ((int)cycF) & 1023, flash > 0 ? (flash + 1) / 2 : 0);
  if (flash > 0) flash--;

  // clear, or fade for trails (real-time rate)
  if (trails < 24) {
    memset(ia_buf, 0, W * H);
  } else {
    static float fadeAcc = 0;
    float tk = 1.0f - trails / 1023.0f;
    fadeAcc += dt * (8.0f + tk * tk * 500.0f);
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) ia_fade(dec);
  }

  // the circuit
  ia_drawGrid(gridDim, pktBoost, true);

  // the blobs
  ia_hotspots(nHS, period, spread);
  int lv = 13, gl = ia_glow;
  if (nHS > 0) {
    if (!nested) {
      ia_layerBalls(IA_RED, sizeQ); ia_contour(cellR, 64, IA_RED, lv, gl);
      ia_layerBalls(IA_YEL, sizeQ); ia_contour(cellY, 64, IA_YEL, lv, gl);
      ia_layerBalls(IA_BLU, sizeQ); ia_contour(cellB, 64, IA_BLU, lv, gl);
      ia_layerBalls(IA_WHT, sizeQ); ia_contour(cellW, 64, IA_WHT, 14, gl > 2 ? 2 : gl);
    } else {
      if (preset == 3) ia_bigBalls(sizeQ, 5, 60, 30, 40);   // Bloom: one big swelling body
      else             ia_bigBalls(sizeQ, 5, 14, 22, 44);   // Topo: hills all over
      ia_contour(8, 20, IA_RED, lv, gl);
      ia_contour(6, 60, IA_YEL, lv, gl);
      ia_contour(4, 120, IA_BLU, lv, gl);
      ia_contour(3, 190, IA_WHT, 14, gl > 2 ? 2 : gl);
    }
  }

  // branches, routes, tiles
  ia_tiles(tiles);
  ia_routes(routes);
  ia_arms(arms);

  ia_sparkles(sparkle);
}

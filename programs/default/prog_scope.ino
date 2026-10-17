// =====================================================================
// PROGRAM: SCOPE (by Dewey)
// Oscilloscope music: a single glowing beam drawing Lissajous figures,
// harmonograph spirals, spirograph roses, spinning wireframes, torus
// knots, vortex tunnels, synth waveforms, rose curves, supershapes and
// strange attractors on a glowing tube. Like a real XY scope, the beam is
// brighter where it moves slowly and fainter where it whips past, and the
// phosphor glows on after it has gone. Turn up Mutate and the figures
// keep evolving on their own: ratios glide, shapes swell, new forms grow.
//
// Presets:
//   k0 Lissajous    — two sine waves against each other, gliding between ratios
//   k1 Harmonograph — decaying pendulum spirals
//   k2 Spirograph   — rolling-circle roses
//   k3 Wireframe    — a spinning 3D cube / octahedron / pyramid / globe
//   k4 Torus Knot   — a knotted ring turning in 3D
//   k5 Vortex       — rings rushing out of a spinning tunnel
//   k6 Waveform     — the classic time trace: two synth voices
//   k7 Morph Show   — glides from one figure to the next on its own
//   k8 Rose         — polar rose curves (petals = the ratio of two numbers)
//   k9 Supershape   — Gielis "superformula" forms: stars, flowers, cushions
//   k10 Attractor   — Lorenz butterfly / Rössler band / Thomas knot, drawn as they fly
//   k11 Mutant      — a random figure that never stops mutating
//
//   k12 — KICK: the figure swells outward, glows and glides into something new (no strobe)
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Tube (8 colour schemes, centre = classic green)   p1 Speed (far left = freeze)
//   p2 / p3 the figure's two main numbers (ratios, wheels, shape, knot…; see the info screen)
//   p4 Phase   p5 Mod (wobble, FM, complexity)   p6 Persistence (afterglow)
//   p7 Mutate (far left = off; right = faster, wilder evolution)   p8 Size   p9 Rotate (3D spin)
//   p10 Brightness   p11 Graticule (off / grid / grid + scanlines)
//   p12 Noise (beam jitter + hum; off left of ~40 %)   p13 Copies (1 … 6 round the centre)
//   p14 Colour Mode: Phosphor / Duo (two contrasting colours by beam direction) /
//                    Depth (near vs far) / Speed / Rainbow along the path
//   p15 Glow (halo; past ~60 % the beam also gets wider)
//
// How it works: each frame the figure is sampled at ~1500 points from
// sine oscillators (whole-number phase counters and a sine table) and
// joined with straight beam segments; each segment adds light in
// proportion to how slowly the beam crossed it. Pixel = (brightness << 4)
// | hue; the brightness fades every frame (persistence), four at a time.
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
#include "vg_sin.h"
#define os_sinT vg_sin512                        // shared sine table (in flash: no RAM)
static inline int os_sin(uint32_t a) { return os_sinT[(a >> 7) & 511]; }
static inline int os_cos(uint32_t a) { return os_sinT[((a >> 7) + 128) & 511]; }
static inline uint32_t os_hash(int a, int b) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static inline int os_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ─── Colours ──────────────────────────────────────────────────────────
// palette index = (brightness << 4) | hue
// hue 0 = the tube's phosphor; hues 1-14 = the tube's contrasting colour
// wheel (4 stops) for the colour modes; hue 15 = the graticule.
#define OS_NSCHEME 8
static const char* const os_schemeNames[OS_NSCHEME] = {
  "Amber", "White", "Blue P7", "Ice", "Green P1", "Red", "Vapor", "Rainbow"
};
// per tube: afterglow, fresh trace, graticule, then 4 contrasting wheel stops
static const uint8_t os_cols[OS_NSCHEME][7][3] = {
  {{120,40,0},{255,190,60},{70,40,10},   {255,140,0},{0,200,255},{255,40,60},{150,90,255}},     // Amber: orange / cyan / red / violet
  {{70,80,110},{240,245,255},{50,55,70}, {230,240,255},{255,40,170},{0,230,255},{255,220,0}},   // White: white / pink / cyan / yellow
  {{110,160,20},{140,190,255},{20,40,70},{60,120,255},{200,255,40},{255,40,200},{0,240,255}},   // Blue P7: blue / lime / magenta / cyan
  {{0,60,120},{180,240,255},{10,40,60},  {0,220,255},{255,60,200},{40,80,255},{255,255,255}},   // Ice: cyan / magenta / blue / white
  {{0,90,20},{120,255,120},{10,60,25},   {40,255,90},{255,40,220},{0,230,255},{255,240,0}},     // Green: green / magenta / cyan / yellow
  {{110,0,10},{255,90,80},{70,10,15},    {255,30,30},{0,240,255},{255,160,0},{70,90,255}},      // Red: red / cyan / orange / blue
  {{80,0,120},{255,120,230},{50,20,80},  {255,70,200},{0,240,255},{170,80,255},{255,190,120}},  // Vapor: pink / cyan / violet / peach
  {{60,0,110},{255,255,255},{40,40,60},  {255,30,30},{0,230,255},{255,230,0},{60,80,255}},      // Rainbow: red / cyan / yellow / blue
};
static int os_flash = 0;

static void os_buildPalette(int scheme, int cyc) {
  const uint8_t (*c)[3] = os_cols[scheme];
  for (int h = 0; h < 16; h++) {
    int ar, ag, ab, fr, fg, fb;
    if (h == 0) { ar = c[0][0]; ag = c[0][1]; ab = c[0][2]; fr = c[1][0]; fg = c[1][1]; fb = c[1][2]; }
    else if (h == 15) { ar = c[2][0] / 2; ag = c[2][1] / 2; ab = c[2][2] / 2; fr = c[2][0]; fg = c[2][1]; fb = c[2][2]; }
    else {
      int pos = (((h - 1) * 256 / 14) + cyc) & 255;               // round the 4-stop wheel
      int p4 = pos * 4, s0 = p4 >> 8, f = p4 & 255, s1 = (s0 + 1) & 3;
      f = f * f * (768 - 2 * f) >> 16;                            // smoothstep: stay near the stops (purer colours)
      fr = (c[3 + s0][0] * (256 - f) + c[3 + s1][0] * f) >> 8;
      fg = (c[3 + s0][1] * (256 - f) + c[3 + s1][1] * f) >> 8;
      fb = (c[3 + s0][2] * (256 - f) + c[3 + s1][2] * f) >> 8;
      ar = fr * 2 / 5; ag = fg * 2 / 5; ab = fb * 2 / 5;
    }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int r, g, b;
      if (l <= 6) { r = ar * l / 6; g = ag * l / 6; b = ab * l / 6; }                // afterglow
      else if (l <= 12) { int k = (l - 6) * 256 / 6; r = ar + ((fr - ar) * k >> 8); g = ag + ((fg - ag) * k >> 8); b = ab + ((fb - ab) * k >> 8); }
      else { int k = (l - 12) * 28; r = fr + ((255 - fr) * k >> 8); g = fg + ((255 - fg) * k >> 8); b = fb + ((255 - fb) * k >> 8); }
      if (os_flash) { r = (r + 255) >> 1; g = (g + 255) >> 1; b = (b + 255) >> 1; }
      display.setColor((l << 4) | h, os_clamp(r, 0, 255), os_clamp(g, 0, 255), os_clamp(b, 0, 255));
    }
  }
}

// ─── Drawing the beam ─────────────────────────────────────────────────
#define OS_PHOS 0
#define OS_DUO 1
#define OS_DEPTH 2
#define OS_SPEED 3
#define OS_RAINBOW 4
static uint8_t* os_buf;
static int os_bright = 6;                        // light per segment
static int os_glow = 0;                          // halo light
static int os_width = 1;                         // 1 or 2 px
static int os_colMode = 0;
static int os_alpha = 256;                       // Q8 weight for cross-fades and the KICK bloom (dithered)
static uint32_t os_dith = 0;

// add light to a pixel (keeps the newer hue); unchecked + checked versions
static inline void os_addP(uint8_t* p, int hue, int a) {
  int l = (*p >> 4) + a;
  *p = (uint8_t)(((l > 15 ? 15 : l) << 4) | hue);
}
static inline void os_add(int x, int y, int hue, int a) {
  if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) os_addP(os_buf + y * W + x, hue, a);
}

// one beam segment; brighter when short (the beam moved slowly)
static void os_seg(int x0, int y0, int x1, int y1, int hue) {
  if ((x0 < -2 && x1 < -2) || (x0 > W + 1 && x1 > W + 1) || (y0 < -2 && y1 < -2) || (y0 > H + 1 && y1 > H + 1)) return;
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int len = dx > dy ? dx : dy;
  int a = os_bright * 3 / (len + 3);
  if (a < 1) a = 1;
  if (os_alpha != 256) {                                          // fractional light: dither the remainder
    int aq = a * os_alpha;
    a = aq >> 8;
    os_dith = os_dith * 1664525u + 1013904223u;
    if ((int)((os_dith >> 24) & 255) < (aq & 255)) a++;
    if (a <= 0) return;
  }
  if (a > 9) a = 9;
  int g = os_glow ? (a * os_glow) >> 3 : 0;
  if (os_colMode == OS_SPEED) hue = 1 + os_clamp(len * 2, 0, 13);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  bool inside = x0 > 1 && x1 > 1 && y0 > 1 && y1 > 1 && x0 < W - 2 && x1 < W - 2 && y0 < H - 2 && y1 < H - 2;
  int err = dx - dy, n = 0;
  int x = x0, y = y0;
  for (;;) {
    if (inside) {                                                 // fast path: no bounds checks
      uint8_t* p = os_buf + y * W + x;
      os_addP(p, hue, a);
      if (os_width > 1) { os_addP(p + 1, hue, a >> 1); os_addP(p + W, hue, a >> 1); }
      if (g) { os_addP(p - 1, hue, g); os_addP(p + 1, hue, g); os_addP(p - W, hue, g); os_addP(p + W, hue, g); }
    } else {
      os_add(x, y, hue, a);
      if (os_width > 1) { os_add(x + 1, y, hue, a >> 1); os_add(x, y + 1, hue, a >> 1); }
      if (g) { os_add(x - 1, y, hue, g); os_add(x + 1, y, hue, g); os_add(x, y - 1, hue, g); os_add(x, y + 1, hue, g); }
    }
    if ((x == x1 && y == y1) || ++n > 800) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x += sx; }
    if (e2 < dx) { err += dx; y += sy; }
  }
}

// ─── The pen: figures hand their points here ──────────────────────────
// Points are Q14 (±16384 = edge of the figure); pos = where along the
// figure (0..255, for Rainbow); depth = 0 near … 255 far (for Depth).
#define OS_MAXCOPY 6
static int os_size = 100;                        // px for 1.0
static int os_copies = 1;
static int os_ccos[OS_MAXCOPY], os_csin[OS_MAXCOPY];   // copy turns, worked out once a frame
static int os_noise = 0;
static int os_punch = 256;                       // KICK scale (Q8)
static int os_px[OS_MAXCOPY], os_py[OS_MAXCOPY];
static bool os_down = false;                     // pen is down (continue from the last point)
static int os_t = 0;
static int os_cx = HALFW, os_cy = HALFH;

static void os_up() { os_down = false; }
static void os_point(int x, int y, int pos, int depth, int bright) {
  if (os_noise) {                                                 // jitter + mains hum
    uint32_t h = os_hash(x ^ (os_t << 3), y + os_t);
    x += (((int)(h & 255) - 128) * os_noise) >> 8;
    y += (((int)((h >> 8) & 255) - 128) * os_noise) >> 8;
    y += (os_sin((uint32_t)(x * 3 + os_t * 900)) * os_noise) >> 14;
  }
  int sz = os_size * os_punch >> 8;
  int ob = os_bright;
  if (bright) os_bright = os_bright * bright >> 8;
  if (depth < 0) {                                                // 2D figures: depth from the radius
    int ax = abs(x), ay = abs(y);
    depth = os_clamp(((ax > ay ? ax + (ay >> 1) : ay + (ax >> 1)) >> 6), 0, 255);
  }
  for (int c = 0; c < os_copies; c++) {
    int xx = x, yy = y;
    if (c > 0) { xx = (x * os_ccos[c] - y * os_csin[c]) >> 14; yy = (x * os_csin[c] + y * os_ccos[c]) >> 14; }
    int sx = os_cx + ((xx * sz) >> 14), sy = os_cy - ((yy * sz) >> 14);
    if (os_down) {
      int hue = 0;
      switch (os_colMode) {
        case OS_DUO: hue = ((sx >= os_px[c]) != (c & 1)) ? 1 : 5; break;   // two contrasting colours by direction
        case OS_DEPTH: hue = 1 + (depth * 8 >> 8); break;                    // near → far across the wheel
        case OS_RAINBOW: hue = 1 + (pos * 14 >> 8); break;
        default: break;
      }
      os_seg(os_px[c], os_py[c], sx, sy, hue);
    }
    os_px[c] = sx; os_py[c] = sy;
  }
  os_bright = ob;
  os_down = true;
}

// 3D: rotate a point (Q14) and project it, brighter when near
static int os_rcx = 16384, os_rsx = 0, os_rcy = 16384, os_rsy = 0, os_rcz = 16384, os_rsz = 0;
static void os_point3(int x, int y, int z, int pos) {
  int y1 = (y * os_rcx - z * os_rsx) >> 14, z1 = (y * os_rsx + z * os_rcx) >> 14;      // about X
  int x2 = (x * os_rcy + z1 * os_rsy) >> 14, z2 = (-x * os_rsy + z1 * os_rcy) >> 14;  // about Y
  int x3 = (x2 * os_rcz - y1 * os_rsz) >> 14, y3 = (x2 * os_rsz + y1 * os_rcz) >> 14; // about Z
  int d = 49152 - z2;                                                                  // perspective (camera at 3)
  int px = (int)((int64_t)x3 * 40000 / d), py = (int)((int64_t)y3 * 40000 / d);
  int br = 256 - (z2 >> 7);                                                            // near = brighter
  os_point(px, py, pos, os_clamp(128 + (z2 >> 7), 0, 255), os_clamp(br, 120, 400));
}

// ─── Figures ──────────────────────────────────────────────────────────
#define OS_N 1500                                // samples per figure
static int os_k2 = 512, os_k3 = 512, os_k4 = 512, os_k5 = 512;   // (mutated) figure knobs
static uint32_t os_phase = 0;                    // slow drift (0..65535 per turn)
static uint32_t os_rot = 0;                      // 3D spin
static int os_speedQ = 256;                      // animation speed (Q8), for the attractors

static inline int os_pos(int i, int n) { return i * 255 / (n > 0 ? n : 1); }

static const uint8_t os_ratios[11][2] = {{1,1},{1,2},{1,3},{2,3},{3,4},{3,5},{4,5},{5,6},{2,5},{3,7},{5,7}};

// Lissajous: the ratio glides smoothly between the table's entries as the
// knob (or Mutate) moves, so figures melt from one to the next
static void os_lissajous(int n) {
  int rq = os_k2 * 10 * 256 / 1023;                               // position in the table (Q8)
  int ri = rq >> 8, rf = rq & 255;
  if (ri >= 10) { ri = 10; rf = 0; }
  int ri2 = ri < 10 ? ri + 1 : 10;
  int aq = os_ratios[ri][0] * (256 - rf) + os_ratios[ri2][0] * rf;   // a, b in Q8
  int bq = os_ratios[ri][1] * (256 - rf) + os_ratios[ri2][1] * rf;
  int fm = os_k5 < 400 ? 0 : (os_k5 - 400) * 192 / 624;         // Mod: a third wobble (off at the left), smooth
  uint32_t ph = os_phase + (uint32_t)(os_k4 * 64);
  int amp = 14800;
  os_up();
  for (int i = 0; i <= n; i++) {
    uint32_t u = (uint32_t)((int64_t)i * 65536 / n);
    int w = fm ? os_sin(u * (uint32_t)((aq + bq) >> 7) + os_phase * 3) * fm >> 10 : 0;
    int x = (os_sin((uint32_t)(((uint64_t)u * aq) >> 8) + ph) + w) * amp >> 14;
    int y = (os_sin((uint32_t)(((uint64_t)u * bq) >> 8)) - w) * amp >> 14;
    os_point(x, y, os_pos(i, n), -1, 0);
  }
}

// harmonograph: two decaying pendulums per axis
static void os_harmonograph(int n) {
  int cyc = 8 + os_k5 * 24 / 1024;                                // how many swings it draws
  int f1 = 2 + os_k2 * 4 / 1024, f2 = 2 + os_k3 * 4 / 1024;
  int det = 140 + (os_k4 >> 2);                                   // slight detune (×1/4096)
  uint32_t p1 = os_phase, p2 = os_phase * 2 + 9000, p3 = os_phase / 2 + 30000, p4 = 51000 - os_phase;
  int decay = 16384;
  int dstep = 16384 - (16384 * 14 / 10) / n;                     // decays to ~25 %
  os_up();
  for (int i = 0; i <= n; i++) {
    uint32_t u = (uint32_t)((int64_t)i * 65536 * cyc / n);
    int x = (os_sin(u * f1 + p1) + os_sin((uint32_t)((uint64_t)u * (f2 * 4096 + det) >> 12) + p2)) >> 1;
    int y = (os_sin((uint32_t)((uint64_t)u * (f1 * 4096 + det / 2) >> 12) + p3) + os_sin(u * f2 + p4)) >> 1;
    x = (x * decay >> 14) * 15500 >> 14; y = (y * decay >> 14) * 15500 >> 14;
    decay = decay * dstep >> 14;
    os_point(x, y, os_pos(i, n), 255 - (decay >> 6), 150);
  }
}

// spirograph (hypotrochoid): a circle of radius r rolling inside one of radius R
static void os_spiro(int n) {
  int R = 5 + os_k2 * 6 / 1024, r = 1 + os_k3 * 4 / 1024;
  if (r >= R) r = R - 1;
  int d = 30 + os_k5 * 80 / 1024;                                 // pen distance (%)
  int g = R, h = r; while (h) { int t = g % h; g = h; h = t; }    // turns needed to close: r / gcd
  int turns = r / g;
  uint32_t ph = os_phase;
  os_up();
  for (int i = 0; i <= n; i++) {
    uint32_t u = (uint32_t)((int64_t)i * 65536 * turns / n) + ph;
    uint32_t v = (uint32_t)((int64_t)u * (R - r) / r);
    int x = (os_cos(u) * (R - r) + os_cos(v) * r * d / 100) / R;
    int y = (os_sin(u) * (R - r) - os_sin(v) * r * d / 100) / R;
    os_point(x * 15000 >> 14, y * 15000 >> 14, os_pos(i, n), -1, 0);
  }
}

// wireframes: edges traced by the beam
static const int8_t os_cubeV[8][3] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
static const uint8_t os_cubeE[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
static const int8_t os_octV[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
static const uint8_t os_octE[12][2] = {{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{4,3},{3,5},{5,2}};
static const int8_t os_tetV[5][3] = {{0,1,0},{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}};
static const uint8_t os_tetE[8][2] = {{0,1},{0,2},{0,3},{0,4},{1,2},{2,3},{3,4},{4,1}};

static void os_edges(const int8_t (*v)[3], const uint8_t (*e)[2], int ne, int scale, int perEdge) {
  for (int k = 0; k < ne; k++) {
    const int8_t* a = v[e[k][0]];
    const int8_t* b = v[e[k][1]];
    os_up();
    for (int s = 0; s <= perEdge; s++) {
      int x = (a[0] * (perEdge - s) + b[0] * s) * scale / perEdge;
      int y = (a[1] * (perEdge - s) + b[1] * s) * scale / perEdge;
      int z = (a[2] * (perEdge - s) + b[2] * s) * scale / perEdge;
      os_point3(x, y, z, k * 255 / ne);
    }
  }
}

static void os_wire(int n) {
  int shape = os_k2 * 4 / 1024; if (shape > 3) shape = 3;
  int sc = 8000 + (os_k5 >> 2);
  if (shape == 2) os_edges(os_cubeV, os_cubeE, 12, sc, 12);              // centre of the knob = cube
  else if (shape == 0) os_edges(os_octV, os_octE, 12, sc * 13 / 10, 12);
  else if (shape == 1) os_edges(os_tetV, os_tetE, 8, sc * 12 / 10, 14);
  else {                                                          // globe: a spiral from pole to pole
    int turns = 8 + os_k3 * 16 / 1024;
    os_up();
    for (int i = 0; i <= n; i++) {
      uint32_t lat = (uint32_t)(i * 32768 / n) - 16384, lon = (uint32_t)((int64_t)i * 65536 * turns / n);
      int cl = os_cos(lat);
      os_point3((cl * os_cos(lon) >> 14) * 12000 >> 14, os_sin(lat) * 12000 >> 14, (cl * os_sin(lon) >> 14) * 12000 >> 14, os_pos(i, n));
    }
  }
}

// torus knot (p, q)
static void os_knot(int n) {
  int p = 2 + os_k2 * 4 / 1024, q = 3 + os_k3 * 5 / 1024;
  if (p == q) q++;
  int tube = 4000 + (os_k5 * 5) ;                                 // how fat the ring is
  os_up();
  for (int i = 0; i <= n; i++) {
    uint32_t u = (uint32_t)((int64_t)i * 65536 / n);
    int r = 8000 + (os_cos(u * (uint32_t)q) * tube >> 14);
    int x = r * os_cos(u * (uint32_t)p) >> 14;
    int y = r * os_sin(u * (uint32_t)p) >> 14;
    int z = os_sin(u * (uint32_t)q) * tube >> 14;
    os_point3(x, y, z, os_pos(i, n));
  }
}

// vortex: rings rushing out of a spinning tunnel
static void os_vortex(int n) {
  int rings = 4 + os_k2 * 8 / 1024;
  int lobes = os_k3 * 7 / 1024;                                   // 0 = round, else petal rings
  int per = n / rings;
  for (int k = 0; k < rings; k++) {
    int z = ((k * 65536 / rings) + (int)(os_phase * 3)) & 65535;  // 0..65535 out from the centre
    int rad = (z >> 2) * (z >> 2) >> 14;                          // grows faster as it nears
    rad = rad * 16000 >> 14;
    int twist = (int)(os_rot >> 1) + k * (os_k5 * 20);
    os_up();
    for (int i = 0; i <= per; i++) {
      uint32_t u = (uint32_t)(i * 65536 / per) + (uint32_t)twist;
      int rr = rad;
      if (lobes) rr = rad * (12000 + (os_cos(u * (uint32_t)lobes) >> 2)) >> 14;
      os_point(rr * os_cos(u) >> 14, rr * os_sin(u) >> 14, (k * 255 / rings), 255 - (z >> 8), 120 + (z >> 8));
    }
  }
}

// waveform (time mode): two voices swept left to right
static void os_wave(int n) {
  int f1 = 1 + os_k2 * 6 / 1024, f2 = 1 + os_k3 * 6 / 1024;
  int fm = (os_k5 * os_k5) >> 16;                                // FM depth (gentle at the centre)
  for (int v = 0; v < 2; v++) {
    os_up();
    int yoff = v == 0 ? 6500 : -6500;
    for (int i = 0; i <= n / 2; i++) {
      int x = -16000 + (int)((int64_t)i * 32000 / (n / 2));
      uint32_t u = (uint32_t)((int64_t)i * 65536 * 2 / (n / 2));
      int y;
      if (v == 0) {
        uint32_t mod = (uint32_t)((os_sin(u * (uint32_t)(f1 * 2) + os_phase * 2) * fm) >> 2);
        y = os_sin(u * (uint32_t)f1 + mod + os_phase * 4);
      } else {
        int s = 0;                                                // a few harmonics: buzzy
        for (int hmn = 1; hmn <= 4; hmn++) s += os_sin(u * (uint32_t)(f2 * hmn) - os_phase * 5) / hmn;
        y = s * 9 / 16;
      }
      os_point(x, yoff + (y * 5200 >> 14), os_pos(i, n / 2), v ? 220 : 30, 0);   // the two voices sit at opposite "depths"
    }
  }
}

// rose curve r = cos(k·θ), k = petals / sweep
static void os_rose(int n) {
  int pn = 1 + os_k2 * 7 / 1024, pd = 1 + os_k3 * 5 / 1024;
  int off = os_k5 * 9000 / 1023;                                  // Mod: pulls toward a limaçon (inner loops)
  int turns = ((pn * pd) & 1) ? pd : 2 * pd;                      // half-turns needed to close
  uint32_t rotA = os_phase;
  os_up();
  for (int i = 0; i <= n; i++) {
    uint32_t th = (uint32_t)((int64_t)i * 32768 * turns / n);
    int r = os_cos((uint32_t)((uint64_t)th * pn / pd) + (uint32_t)(os_k4 * 32));
    r = (r * (16384 - off) >> 14) + off;
    r = r * 15000 >> 14;
    os_point(r * os_cos(th + rotA) >> 14, r * os_sin(th + rotA) >> 14, os_pos(i, n), -1, 0);
  }
}

// supershape (Gielis superformula): r(θ) = (|cos(mθ/4)|^n2 + |sin(mθ/4)|^n3)^(−1/n1)
// The radius table is rebuilt only when its numbers change.
static int16_t os_ssR[256];
static float os_ssKey = -1;
static void os_superShape(int n) {
  float m = 2.0f + os_k2 * 10.0f / 1023.0f;                       // symmetry (glides between whole numbers)
  float n1 = 0.15f + (float)os_k3 * os_k3 * 3.5f / 1046529.0f;     // pinch (spiky at the centre, round to the right)
  float n2 = 0.4f + (float)os_k5 * os_k5 * 5.0f / 1046529.0f;      // puff
  float key = m * 1000.0f + n1 * 37.0f + n2 * 1.3f;
  if (key != os_ssKey) {
    os_ssKey = key;
    float mx = 0.001f;
    float tmp[256];
    for (int i = 0; i < 256; i++) {
      float th = i * (6.2831853f / 256.0f);
      float a = fabsf(cosf(m * th * 0.25f)), b = fabsf(sinf(m * th * 0.25f));
      float s = (a > 1e-4f ? powf(a, n2) : 0) + (b > 1e-4f ? powf(b, n2) : 0);
      float r = s > 1e-6f ? powf(s, -1.0f / n1) : 0;
      if (r > 50.0f) r = 50.0f;
      tmp[i] = r;
      if (r > mx) mx = r;
    }
    for (int i = 0; i < 256; i++) os_ssR[i] = (int16_t)(tmp[i] / mx * 15000.0f);
  }
  int pulse = 16384 + (os_sin(os_phase * 4) >> 3);                // breathes
  uint32_t rotA = os_phase;
  os_up();
  for (int i = 0; i <= n; i++) {
    int idx = (i * 256 / n) & 255, fr = (i * 256 % n) * 256 / n;
    int r = (os_ssR[idx] * (256 - fr) + os_ssR[(idx + 1) & 255] * fr) >> 8;
    r = r * pulse >> 14;
    uint32_t th = (uint32_t)(i * 65536 / n);
    os_point(r * os_cos(th + rotA) >> 14, r * os_sin(th + rotA) >> 14, os_pos(i, n), -1, 0);
  }
}

// strange attractors, integrated as they are drawn (the trail is the picture)
static float os_ax = 0.1f, os_ay = 0.0f, os_az = 0.0f;
static int os_attrKind = -1;
static void os_attractor(int n) {
  (void)n;
  int kind = os_k2 * 3 / 1024; if (kind > 2) kind = 2;            // Lorenz / Rössler / Thomas
  if (kind != os_attrKind) { os_attrKind = kind; os_ax = 0.1f; os_ay = 0.0f; os_az = kind == 2 ? 0.3f : 0.0f; }
  int steps = 40 + (os_speedQ * 260 >> 8);
  if (steps > 500) steps = 500;
  float x = os_ax, y = os_ay, z = os_az;
  float pk = os_k3 / 1023.0f, mk = os_k5 / 1023.0f;
  float dt, sc, cz;
  if (kind == 0) { dt = 0.004f; sc = 16384.0f / 26.0f; cz = 25.0f; }
  else if (kind == 1) { dt = 0.02f; sc = 16384.0f / 15.0f; cz = 8.0f; }
  else { dt = 0.06f; sc = 16384.0f / 4.8f; cz = 0.0f; }
  for (int s = 0; s < steps; s++) {
    float dx, dy, dz;
    if (kind == 0) {                                              // Lorenz: rho on p3
      float rho = 20.0f + pk * 25.0f, sig = 8.0f + mk * 6.0f;
      dx = sig * (y - x); dy = x * (rho - z) - y; dz = x * y - 2.6667f * z;
    } else if (kind == 1) {                                       // Rössler: c on p3
      float c = 4.0f + pk * 8.0f, a = 0.1f + mk * 0.2f;
      dx = -y - z; dy = x + a * y; dz = 0.2f + z * (x - c);
    } else {                                                      // Thomas: b on p3
      float b = 0.13f + pk * 0.1f;
      dx = sinf(y) - b * x; dy = sinf(z) - b * y; dz = sinf(x) - b * z;
    }
    x += dx * dt; y += dy * dt; z += dz * dt;
    if (!(x == x) || fabsf(x) > 1000.0f || fabsf(y) > 1000.0f || fabsf(z) > 1000.0f) { x = 0.1f; y = 0; z = kind == 2 ? 0.3f : 0; os_up(); }
    int px = (int)(x * sc), py = (int)(y * sc), pz = (int)((z - cz) * sc);
    if (kind == 0) { int t = py; py = pz; pz = t; }               // Lorenz: wings side by side
    os_point3(os_clamp(px, -30000, 30000), os_clamp(py, -30000, 30000), os_clamp(pz, -30000, 30000), (s * 255 / steps));
  }
  os_ax = x; os_ay = y; os_az = z;
}

#define OS_NFIG 11
static void os_figure(int which, int n) {
  switch (which) {
    case 1: os_harmonograph(n); break;
    case 2: os_spiro(n); break;
    case 3: os_wire(n); break;
    case 4: os_knot(n); break;
    case 5: os_vortex(n); break;
    case 6: os_wave(n); break;
    case 8: os_rose(n); break;
    case 9: os_superShape(n); break;
    case 10: os_attractor(n); break;
    default: os_lissajous(n); break;
  }
}

// The whole-number part of a figure's settings (petals, gear sizes, knot P/Q …).
// When it changes, the old and new shapes cross-fade instead of snapping.
static int os_sig(int which) {
  switch (which) {
    case 1: return (os_k2 * 4 >> 10) | (os_k3 * 4 >> 10) << 4 | (os_k5 * 24 >> 10) << 8;
    case 2: return (os_k2 * 6 >> 10) | (os_k3 * 4 >> 10) << 4;
    case 3: { int sh = os_k2 * 4 >> 10; if (sh > 3) sh = 3; return sh | (sh == 3 ? (os_k3 * 16 >> 10) << 4 : 0); }
    case 4: return (os_k2 * 4 >> 10) | (os_k3 * 5 >> 10) << 4;
    case 5: return (os_k2 * 8 >> 10) | (os_k3 * 7 >> 10) << 4;
    case 6: return (os_k2 * 6 >> 10) | (os_k3 * 6 >> 10) << 4;
    case 8: return (os_k2 * 7 >> 10) | (os_k3 * 5 >> 10) << 4;
    case 10: { int k = os_k2 * 3 >> 10; return k > 2 ? 2 : k; }
    default: return 0;                                            // Lissajous, Supershape: already smooth
  }
}
static int os_pA[4], os_pB[4];                   // settled figure knobs: fading from A to B
static float os_xf = 1.0f;                       // cross-fade progress (1 = done)
static inline void os_setK(const int* k) { os_k2 = k[0]; os_k3 = k[1]; os_k4 = k[2]; os_k5 = k[3]; }
static int os_sigWith(int which, const int* k) { os_setK(k); return os_sig(which); }
static void os_drawFig(int which, int n, int weight, int boost) {   // weight, boost Q8
  if (weight < 6) return;
  if (os_xf < 1.0f && which != 10 && os_sigWith(which, os_pA) != os_sigWith(which, os_pB)) {
    float x = os_xf * os_xf * (3.0f - 2.0f * os_xf);
    int wa = (int)(weight * (1.0f - x)), wb = (int)(weight * x);
    if (wa >= 6) { os_setK(os_pA); os_alpha = wa * boost >> 8; os_figure(which, n); }
    if (wb >= 6) { os_setK(os_pB); os_alpha = wb * boost >> 8; os_figure(which, n); }
  } else {
    os_setK(os_pB); os_alpha = weight * boost >> 8; os_figure(which, n);
  }
  os_alpha = 256;
}

// ─── Graticule, scanlines, fade ───────────────────────────────────────
static void os_graticule(int mode) {
  if (mode <= 0) return;
  uint8_t g = (uint8_t)((3 << 4) | 15), gt = (uint8_t)((5 << 4) | 15);
  int x0 = 20, x1 = 300, y0 = 8, y1 = 232;
  for (int k = 0; k <= 10; k++) {                                 // 10 × 8 divisions
    int x = x0 + k * 28;
    for (int y = y0; y <= y1; y += 2) { uint8_t* p = os_buf + y * W + x; if ((*p >> 4) < 3) *p = g; }
  }
  for (int k = 0; k <= 8; k++) {
    int y = y0 + k * 28;
    for (int x = x0; x <= x1; x += 2) { uint8_t* p = os_buf + y * W + x; if ((*p >> 4) < 3) *p = g; }
  }
  for (int x = x0; x <= x1; x += 6) for (int d = -2; d <= 2; d++) { uint8_t* p = os_buf + (HALFH + d) * W + x; if ((*p >> 4) < 5) *p = gt; }
  for (int y = y0; y <= y1; y += 6) for (int d = -2; d <= 2; d++) { uint8_t* p = os_buf + y * W + HALFW + d; if ((*p >> 4) < 5) *p = gt; }
}
static void os_scanlines() {
  for (int y = 1; y < H; y += 2) {
    uint8_t* p = os_buf + y * W;
    for (int x = 0; x < W; x++) { int l = p[x] >> 4; if (l > 1) p[x] = (uint8_t)(((l * 3 >> 2) << 4) | (p[x] & 15)); }
  }
}
static inline uint32_t os_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void os_fade(int dec) {
  uint32_t* b = (uint32_t*)os_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = os_fade4(b[i], d4);
}

// ─── Mutation: the figure's numbers wander to new values by themselves ─
static float os_mut[4], os_mutTgt[4];            // offsets for p2, p3, p4, p5 (knob units)
static float os_mutClock = 0;
static float os_mutHold = 0;                     // a KICK with Mutate off: wander for a few seconds, then drift home
static void os_mutate(float amount, float dt, bool jump, int seed) {
  if (os_mutHold > 0) { os_mutHold -= dt; if (amount < 0.45f) amount = 0.45f; }
  os_mutClock += dt * (0.12f + amount * 0.35f);                    // new targets every ~8 s … ~2 s
  if (os_mutClock >= 1.0f || jump) {
    os_mutClock = 0;
    for (int i = 0; i < 4; i++) {
      uint32_t h = os_hash(seed + i * 77, (int)(os_mut[i] * 13.0f) + i);
      os_mutTgt[i] = ((int)(h & 1023) - 512) * amount * (i == 0 ? 1.2f : 0.9f);
    }
  }
  float glide = dt * (0.35f + amount * 0.9f);                     // slow, liquid glides
  if (glide > 1.0f) glide = 1.0f;
  for (int i = 0; i < 4; i++) {
    float tgt = amount > 0.01f ? os_mutTgt[i] : 0.0f;
    os_mut[i] += (tgt - os_mut[i]) * glide;
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float os_sm[16];
static bool os_smReady = false;
static inline int os_k(int i) { int v = (int)os_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_scope_name() { return "SCOPE"; }
const char* prog_scope_character() { return "Oscilloscope music: a glowing beam drawing figures, wireframes, roses, supershapes and attractors that mutate"; }

static const char* const os_presetNames[] = {
  "Lissajous", "Harmonograph", "Spirograph", "Wireframe", "Torus Knot", "Vortex",
  "Waveform", "Morph Show", "Rose", "Supershape", "Attractor", "Mutant"
};
#define OS_NUM_PRESETS 12

const char* prog_scope_presetName(int preset) {
  if (preset >= 0 && preset < OS_NUM_PRESETS) return os_presetNames[preset];
  return NULL;
}

static const char* const os_labels[16] = {
  "Tube", "Speed", "X Ratio", "Y Ratio", "Phase", "Mod", "Persistence", "Mutate",
  "Size", "Rotate", "Brightness", "Graticule", "Noise", "Copies", "Colour Mode", "Glow"
};
const char* prog_scope_potLabel(int preset, int pot) {
  // a few knobs mean something different in some presets
  if (pot == 2) {
    switch (preset) {
      case 1: return "Swing X";      case 2: return "Big Wheel";   case 3: return "Shape";
      case 4: return "Knot P";       case 5: return "Rings";       case 6: return "Voice 1 Pitch";
      case 8: return "Petals";       case 9: return "Symmetry";    case 10: return "Lorenz/Rossler/Thomas";
      default: break;
    }
  }
  if (pot == 3) {
    switch (preset) {
      case 1: return "Swing Y";      case 2: return "Small Wheel"; case 3: return "Globe Turns";
      case 4: return "Knot Q";       case 5: return "Petals";      case 6: return "Voice 2 Pitch";
      case 8: return "Sweep";        case 9: return "Pinch";       case 10: return "Shape";
      default: break;
    }
  }
  if (pot == 5) {
    switch (preset) {
      case 1: return "Length";       case 2: return "Pen Reach";   case 3: case 4: return "Size / Fatness";
      case 5: return "Twist";        case 6: return "FM Depth";    case 8: return "Inner Loops";
      case 9: return "Puff";         case 10: return "Flow";
      default: break;
    }
  }
  if (pot >= 0 && pot < 16) return os_labels[pot];
  return "";
}

uint8_t prog_scope_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we fade the tube ourselves (persistence)
}

void prog_scope_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
  }
  os_attrKind = -1;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_scope_draw(int preset) {
  os_buf = display.getBuffer();
  if (!os_buf) return;
  if (preset < 0 || preset >= OS_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!os_smReady) os_sm[i] = pots[i];
      else os_sm[i] += (pots[i] - os_sm[i]) * a;
    }
    os_smReady = true;
  }

  // ── clocks ── (wrapped at 60 turns so every multiple / third / fifth of them stays seamless,
  //               and small enough that float steps stay smooth for hours)
  const float WRAP = 65536.0f * 60.0f;
  float sk = os_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(16.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.6×
  static float tf = 0, phF = 0, rotF = 0, morphF = 0;
  tf += dt * 60.0f * speed;
  if (tf > 1.0e6f) tf -= 1.0e6f;
  os_t = (int)tf;
  os_speedQ = (int)(speed * 256.0f);

  // ── KICK: a smooth swell (no strobe): size punch, extra light, and a phase rush ──
  static bool k12Was = false;
  static float kickT = 9.0f;
  bool k12 = keysPressed[KEY_MOD_A];
  bool kick = k12 && !k12Was;
  k12Was = k12;
  if (kick) kickT = 0;
  kickT += dt;
  float env = kickT < 0.12f ? kickT / 0.12f : expf(-(kickT - 0.12f) * 2.5f);
  env = env * env * (3.0f - 2.0f * env);
  os_punch = (int)(256.0f * (1.0f + 0.22f * env));
  int boost = 256 + (int)(env * 230.0f);
  os_flash = 0;

  phF += dt * speed * 2600.0f + dt * env * 30000.0f;              // phase drift (1/65536 turn) + KICK rush
  float rk = (os_sm[9] - 512.0f) / 512.0f;
  rotF += dt * speed * (3000.0f + rk * 9000.0f) + dt * env * 9000.0f;
  if (phF >= WRAP) phF -= WRAP;
  if (rotF >= WRAP) rotF -= WRAP; else if (rotF < 0) rotF += WRAP;
  os_phase = (uint32_t)phF;
  os_rot = (uint32_t)rotF;

  // ── mutation ──
  float mutAmt = os_k(7) < 40 ? 0.0f : (os_k(7) - 40) / 983.0f;
  if (preset == 11 && mutAmt < 0.55f) mutAmt = 0.55f;             // the Mutant always mutates
  if (kick && mutAmt < 0.05f) os_mutHold = 4.0f;
  os_mutate(mutAmt * (speed > 0 ? 1.0f : 0.0f), dt * (speed > 0 ? 0.5f + speed : 0.0f), kick, (int)now);
  int tgt[4] = {os_clamp(os_k(2) + (int)os_mut[0], 0, 1023), os_clamp(os_k(3) + (int)os_mut[1], 0, 1023),
                os_clamp(os_k(4) + (int)os_mut[2], 0, 1023), os_clamp(os_k(5) + (int)os_mut[3], 0, 1023)};

  // ── knobs ──
  int persist = os_k(6);
  os_size = (int)(105.0f * powf(2.0f, (os_k(8) - 512) / 512.0f));
  os_bright = 3 + os_k(10) * 9 / 1023;
  int grat = os_k(11) < 300 ? 0 : (os_k(11) < 720 ? 1 : 2);
  { int nk = os_k(12) - 400; os_noise = nk > 0 ? nk * nk / 600 : 0; }
  os_copies = 1 + os_k(13) * OS_MAXCOPY / 1024; if (os_copies > OS_MAXCOPY) os_copies = OS_MAXCOPY;
  for (int c = 0; c < os_copies; c++) { uint32_t a = (uint32_t)(c * 65536 / os_copies); os_ccos[c] = os_cos(a); os_csin[c] = os_sin(a); }
  os_colMode = os_k(14) * 5 / 1024; if (os_colMode > 4) os_colMode = 4;
  os_glow = os_k(15) * 8 / 1024;
  os_width = os_k(15) > 614 ? 2 : 1;
  os_cx = HALFW; os_cy = HALFH;

  // 3D spin: three angles turning at different rates
  uint32_t r = os_rot;
  os_rcx = os_cos(r / 3); os_rsx = os_sin(r / 3);
  os_rcy = os_cos(r);     os_rsy = os_sin(r);
  os_rcz = os_cos(r / 5); os_rsz = os_sin(r / 5);

  // ── colours ──
  int sc = os_k(0) * OS_NSCHEME / 1024;
  if (sc > OS_NSCHEME - 1) sc = OS_NSCHEME - 1;
  static float cycF = 0;
  if (os_colMode > 0) cycF += dt * speed * (30.0f + mutAmt * 120.0f);
  if (cycF > 1.0e6f) cycF = 0;
  os_buildPalette(sc, ((int)cycF) & 255);
  if (os_flash > 0) os_flash--;

  // ── persistence: fade the tube (real-time rate) ──
  {
    static float fadeAcc = 0;
    float pk = 1.0f - persist / 1023.0f;
    float rate = 10.0f + pk * pk * 600.0f;
    if (preset == 10) rate *= 0.35f;                               // attractors draw a little each frame: keep their trails
    fadeAcc += dt * rate;
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) os_fade(dec);
  }

  // ── which figures are on screen ──
  static int curA = 0, curB = 1, lastSlot = -1;
  float mix = 0;
  bool show = preset == 7 || preset == 11;
  if (show) {
    // Morph Show: every figure in turn. Mutant: a random figure, always mutating.
    float rate = preset == 7 ? 0.12f : 0.08f;
    morphF += dt * speed * rate;
    if (kick && preset == 11 && morphF - (int)morphF < 0.7f) morphF = (int)morphF + 0.7f;   // KICK: start the hand-over now
    if (morphF > 1.0e5f) morphF -= 9.0e4f;
    int slot = (int)morphF;
    if (slot != lastSlot) {
      bool first = lastSlot < 0;
      lastSlot = slot;
      static const uint8_t order[9] = {0, 8, 1, 9, 2, 3, 4, 5, 10};
      if (preset == 7) { curA = order[slot % 9]; curB = order[(slot + 1) % 9]; }
      else {
        curA = first ? order[os_hash(slot, 11) % 9u] : curB;
        curB = order[os_hash(slot, (int)now) % 9u];
        if (curB == curA) curB = order[(slot + 3) % 9];
      }
    }
    float fr = morphF - (int)morphF;
    mix = fr < 0.7f ? 0.0f : (fr - 0.7f) / 0.3f;                  // hold, then a long hand-over
    mix = mix * mix * (3.0f - 2.0f * mix);
  }

  // ── settle the figure knobs: whole-number changes cross-fade (~1 s) instead of snapping ──
  {
    static bool primed = false;
    if (!primed) { primed = true; for (int i = 0; i < 4; i++) os_pA[i] = os_pB[i] = tgt[i]; }
    int figs[2] = {show ? curA : preset, show ? curB : -1};
    bool differs = false;
    for (int f = 0; f < 2; f++) {
      if (figs[f] < 0) continue;
      if (os_sigWith(figs[f], tgt) != os_sigWith(figs[f], os_pB)) differs = true;
    }
    if (os_xf < 1.0f) {
      os_xf += dt / 1.1f;
      if (os_xf > 1.0f) os_xf = 1.0f;
      if (!differs) for (int i = 0; i < 4; i++) os_pB[i] = tgt[i];   // keep following smooth moves
    } else if (differs) {
      for (int i = 0; i < 4; i++) { os_pA[i] = os_pB[i]; os_pB[i] = tgt[i]; }
      os_xf = 0.0f;
    } else {
      for (int i = 0; i < 4; i++) os_pA[i] = os_pB[i] = tgt[i];
    }
  }

  // ── draw ──
  int n = OS_N / os_copies;
  if (n < 400) n = 400;
  if (show) {
    os_drawFig(curA, n, (int)(256.0f * (1.0f - mix)), boost);
    os_drawFig(curB, n, (int)(256.0f * mix), boost);
  } else {
    os_drawFig(preset, n, 256, boost);
  }
  os_graticule(grat);
  if (grat == 2) os_scanlines();
}

// =====================================================================
// PROGRAM: PIPEWORKS — pipes growing across a plotter sheet (by Dewey) — Set 5, slot 5
//
// Seen from above: pipes grow across the paper one step at a time, turning
// at random, bolted with flanges at the bends, passing over and under each
// other. Each pipe is shaded like a cylinder with pen-drawn rings and
// casts an airbrushed shadow, longer the higher it runs, so the sheet
// slowly becomes a dense plant room like di_diff_world's plots. When the
// sheet is full the paper feeds through and a fresh one begins.
//
// The pipes are kept as a list of straight runs and the whole sheet is
// redrawn every frame (so the info overlay never leaves marks on it); each
// run is a handful of fast horizontal fills.
//
// Presets:
//   k0 Plant Room  — grey pen plot on white paper
//   k1 Blueprint   — pale pipes on blue
//   k2 Copper      — copper pipes on a dark floor
//   k3 Brass & Steel — two metals side by side
//   k4 Neon Tubes  — glowing tubes in the dark
//   k5 Sewer       — fat dark pipes, long shadows
//   k6 Capillaries — many thin pipes, fast
//   k7 Chrome      — hard black and white
//   k8 Red Valves  — steel pipes, some lit red
//   k9 Sepia Plan  — brown ink on old paper
//   k10 Circuit    — thin green traces running long and straight
//   k11 Tour       — a new preset with every new sheet
//
//   k12 — NEW SHEET: feed the paper now
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Pipes (how many grow at once)
//   p3 Thickness   p4 Height (shadow length)   p5 Turns (how often they bend)   p6 Rings (spacing)
//   p7 Flanges   p8 Light Angle   p9 Airbrush (shadow strength)   p10 Line Darkness   p11 Paper Grid
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5p.h"

struct Pp5Preset { P5Look look; uint8_t heads, radius, height, turns, rings, flanges, mixB, grid, cell, speed; int8_t ink; uint8_t paperGrid, floor; };
static const Pp5Preset pp5_pre[11] = {
  {{{12,12,14},{120,120,122},{246,245,240},{12,12,14},{120,120,122},{246,245,240}}, 3, 6, 3, 90, 5, 1, 0, 1, 20, 60, 50, 1, 127},
  {{{6,24,80},{60,110,190},{200,225,250},{6,24,80},{60,110,190},{200,225,250}}, 3, 6, 3, 90, 5, 1, 0, 1, 20, 60, 50, 1, 70},
  {{{20,8,4},{150,70,30},{255,200,150},{20,8,4},{150,70,30},{255,200,150}}, 3, 7, 4, 80, 6, 1, 0, 0, 22, 60, 40, 0, 16},
  {{{10,12,16},{110,120,135},{235,240,250},{20,12,0},{170,120,40},{255,235,160}}, 4, 6, 3, 90, 5, 1, 128, 1, 20, 60, 46, 1, 118},
  {{{0,0,4},{20,40,90},{120,255,250},{4,0,4},{100,20,90},{255,140,240}}, 3, 5, 2, 90, 4, 0, 128, 0, 20, 70, -40, 1, 6},
  {{{6,10,6},{60,70,58},{170,180,160},{6,10,6},{60,70,58},{170,180,160}}, 2, 11, 6, 60, 8, 1, 0, 0, 30, 45, 34, 0, 34},
  {{{12,12,14},{120,120,122},{246,245,240},{12,12,14},{120,120,122},{246,245,240}}, 7, 3, 2, 120, 3, 0, 0, 1, 12, 110, 44, 1, 127},
  {{{0,0,0},{90,90,90},{255,255,255},{0,0,0},{90,90,90},{255,255,255}}, 3, 7, 4, 90, 5, 1, 0, 0, 22, 60, 70, 0, 127},
  {{{10,10,12},{110,110,115},{235,235,238},{20,0,0},{170,30,20},{255,170,150}}, 3, 6, 3, 90, 5, 1, 70, 1, 20, 60, 46, 1, 122},
  {{{30,18,8},{140,100,60},{240,225,190},{30,18,8},{140,100,60},{240,225,190}}, 3, 6, 3, 90, 5, 1, 0, 1, 20, 60, 46, 1, 120},
  {{{0,20,8},{20,110,50},{170,255,170},{0,20,8},{20,110,50},{170,255,170}}, 5, 2, 1, 40, 0, 0, 0, 0, 10, 120, -40, 1, 14},
};
static const char* const pp5_names[12] = {
  "Plant Room", "Blueprint", "Copper", "Brass & Steel", "Neon Tubes", "Sewer", "Capillaries", "Chrome", "Red Valves", "Sepia Plan", "Circuit", "Tour"
};
static const char* const pp5_labels[16] = {
  "Palette", "Speed", "Pipes", "Thickness", "Height", "Turns", "Rings", "Flanges",
  "Light Angle", "Airbrush", "Line Darkness", "Paper Grid", "Fullness", "Feed Speed", "Colour Mix", "Grid Size"
};
const char* prog_pipes_name() { return "PIPEWORKS"; }
const char* prog_pipes_character() { return "Pipes growing and bending across a plotter sheet, mesh-ringed and airbrush-shadowed, then the paper feeds"; }
const char* prog_pipes_presetName(int preset) { return (preset >= 0 && preset < 12) ? pp5_names[preset] : NULL; }
const char* prog_pipes_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? pp5_labels[pot] : ""; }
uint8_t prog_pipes_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// a straight run of pipe: from (x, y), `len` pixels in direction (dx, dy)
struct Pp5Seg { int16_t x, y, len; int8_t dx, dy; uint8_t level, ramp, r, flags; };
#define PP5_FLANGE0 1                                  // a flange where it starts (a bend)
#define PP5_CAP 2                                      // a flange (or cap) where it ends
#define PP5_MAXSEG (V3_SCRATCH / sizeof(Pp5Seg))
static Pp5Seg* pp5_seg;
static int pp5_nseg = 0;
#define PP5_HEADS 8
struct Pp5Head { int16_t x, y; int8_t dx, dy; uint8_t level, ramp, alive; int16_t len, seg; };
static Pp5Head pp5_h[PP5_HEADS];
static int pp5_preset = -1, pp5_look = 0, pp5_r = 6, pp5_cell = 20, pp5_feed = 0, pp5_scroll = 0, pp5_drawn = 0, pp5_shx = 1, pp5_shy = 1;
static float pp5_acc = 0;
static bool pp5_k12 = false, pp5_fresh = true;
static int pp5_ring = 5, pp5_air = 40, pp5_height = 3, pp5_turns = 90, pp5_flanges = 1, pp5_mixB = 0, pp5_paperGrid = 1, pp5_full = 2200, pp5_floor = 127;
static uint8_t pp5_prof[32];                           // tone across a pipe (/255), from the radius and the light

static int pp5_newSeg(int hi, uint8_t flags) {
  Pp5Head& h = pp5_h[hi];
  if (pp5_nseg >= (int)PP5_MAXSEG) return -1;
  Pp5Seg& g = pp5_seg[pp5_nseg];
  g.x = h.x; g.y = (int16_t)(h.y); g.len = 0; g.dx = h.dx; g.dy = h.dy; g.level = h.level; g.ramp = h.ramp; g.r = (uint8_t)pp5_r; g.flags = flags;
  return pp5_nseg++;
}
static void pp5_newHead(int hi) {
  Pp5Head& h = pp5_h[hi];
  h.x = (int16_t)(pp5_cell / 2 + v3_rnd(W / pp5_cell) * pp5_cell); h.y = (int16_t)(pp5_cell / 2 + v3_rnd(H / pp5_cell) * pp5_cell);
  int d = v3_rnd(4); h.dx = (int8_t)(d == 0 ? 1 : (d == 1 ? -1 : 0)); h.dy = (int8_t)(d == 2 ? 1 : (d == 3 ? -1 : 0));
  h.level = (uint8_t)(1 + v3_rnd(pp5_height > 0 ? pp5_height : 1)); h.ramp = (uint8_t)(v3_rnd(256) < pp5_mixB ? 1 : 0);
  h.alive = 1; h.len = (int16_t)v3_rndr(20, 90);
  h.seg = (int16_t)pp5_newSeg(hi, PP5_FLANGE0);
  if (h.seg < 0) h.alive = 0;
}
static void pp5_drawFlange(int x, int y, int r, int level, int ramp) {
  int s = r + 3, sl = level * 3 + 2;
  for (int yy = -s; yy <= s; yy++) p5_sprayRow(y + yy + pp5_shy * sl, x - s + pp5_shx * sl, x + s + pp5_shx * sl, pp5_air, pp5_air, 5);
  int32_t xs[4] = {(x - s) * 16, (x + s + 1) * 16, (x + s + 1) * 16, (x - s) * 16}, ys[4] = {(y - s) * 16, (y - s) * 16, (y + s + 1) * 16, (y + s + 1) * 16};
  p5_quad(xs, ys, p5_tone(110, ramp));
  p5_mesh(xs, ys, s / 2 + 1, s / 2 + 1, p5_line(110, ramp));
  p5_outline(xs, ys, p5_tone(6, ramp));
  v3_disc(x, y, s / 3, p5_tone(40, ramp));                          // the bolt
}
// a run of pipe: shadow, body rows, rings, outline
static void pp5_drawSeg(int gi) {
  const Pp5Seg& g = pp5_seg[gi];
  if (g.len <= 0 && !(g.flags & PP5_FLANGE0)) return;
  int r = g.r, sl = g.level * 3 + 2, y0 = g.y - pp5_scroll;
  int ax = g.x, ay = y0, bx = g.x + g.dx * g.len, by = y0 + g.dy * g.len;
  if (ax > bx) { int t = ax; ax = bx; bx = t; } if (ay > by) { int t = ay; ay = by; by = t; }
  bool horiz = g.dx != 0;
  if (g.len > 0) {
    // the shadow: everything under it darkened, softer at the edges
    if (horiz) for (int k = -r; k <= r; k++) p5_sprayRow(ay + k + pp5_shy * sl, ax + pp5_shx * sl, bx + pp5_shx * sl, r - v3_abs(k) < 2 ? pp5_air / 2 : pp5_air, r - v3_abs(k) < 2 ? pp5_air / 2 : pp5_air, 9);
    else for (int y = ay; y <= by; y++) p5_sprayRow(y + pp5_shy * sl, ax - r + pp5_shx * sl, ax + r + pp5_shx * sl, pp5_air, pp5_air, 9);
    // the body
    for (int k = -r; k <= r; k++) {
      int t = (118 * pp5_prof[k + r]) >> 8;
      if (k == -r || k == r) t = 6;
      else if (k == -(r / 2) || (r >= 5 && k == r / 2)) t -= p5_ink;
      uint8_t c = p5_tone(t, g.ramp);
      if (horiz) v3_hspan(ax, bx, ay + k, c);
      else for (int y = ay; y <= by; y++) if ((unsigned)y < H && (unsigned)(ax + k) < W) v3_buf[y * W + ax + k] = c;
    }
    // rings across it, at fixed places on the sheet so they don't crawl
    if (pp5_ring > 0) {
      int sp = pp5_ring + 2;
      if (horiz) { for (int x = ax + ((sp - ((ax % sp) + sp) % sp) % sp); x <= bx; x += sp) for (int k = -r + 1; k < r; k++) { int t = ((118 * pp5_prof[k + r]) >> 8) - p5_ink; v3_put(x, ay + k, p5_tone(t, g.ramp)); } }
      else { int gy = ay + pp5_scroll; for (int y = ay + ((sp - ((gy % sp) + sp) % sp) % sp); y <= by; y += sp) for (int k = -r + 1; k < r; k++) { int t = ((118 * pp5_prof[k + r]) >> 8) - p5_ink; v3_put(ax + k, y, p5_tone(t, g.ramp)); } }
    }
  }
  if ((g.flags & PP5_FLANGE0) && pp5_flanges) pp5_drawFlange(g.x, y0, r, g.level, g.ramp);
  if (g.flags & PP5_CAP) { if (pp5_flanges) pp5_drawFlange(g.x + g.dx * g.len, y0 + g.dy * g.len, r, g.level, g.ramp); else v3_disc(g.x + g.dx * g.len, y0 + g.dy * g.len, r, p5_tone(30, g.ramp)); }
}
static void pp5_paper() {                              // fresh paper with a faint plotted grid (it scrolls with the sheet)
  uint8_t paper = p5_tone(pp5_floor, 0), grid = p5_tone(pp5_floor > 60 ? pp5_floor - 16 : pp5_floor + 12, 0);
  memset(v3_buf, paper, W * H);
  if (!pp5_paperGrid) return;
  int c = pp5_cell, off = ((c / 2 - pp5_scroll) % c + c) % c;
  for (int y = off; y < H; y += c) memset(v3_buf + y * W, grid, W);
  for (int y = 0; y < H; y++) for (int x = c / 2; x < W; x += c) v3_buf[y * W + x] = grid;
}
void prog_pipes_init() { v3_reset(); p5_paletteDirty(); pp5_preset = -1; pp5_fresh = true; pp5_seg = (Pp5Seg*)v3_scratch; pp5_nseg = 0; }

static void pipes5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  pp5_seg = (Pp5Seg*)v3_scratch;
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != pp5_preset) { pp5_preset = preset; pp5_look = preset == 11 ? 0 : preset; pp5_fresh = true; }
  const Pp5Preset& P = pp5_pre[pp5_look];
  const Pp5Preset& Q = pp5_pre[v3_centre(0) ? pp5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 4.0f) * P.speed;
  int heads = v3_centre(2) ? P.heads : 1 + v3_clamp(v3_k(2) * PP5_HEADS / 1024, 0, PP5_HEADS - 1);
  int r = v3_centre(3) ? P.radius : 2 + (int)(v3_u(3) * 12);
  pp5_height = v3_centre(4) ? P.height : (int)(v3_u(4) * 8);
  pp5_turns = v3_centre(5) ? P.turns : (int)(v3_u(5) * 255);
  pp5_ring = v3_centre(6) ? P.rings : (int)(v3_u(6) * 12);
  pp5_flanges = v3_centre(7) ? P.flanges : v3_k(7) > 512;
  int la = v3_centre(8) ? 0 : v3_clamp(v3_k(8) * 4 / 1024, 0, 3);
  pp5_shx = (la == 0 || la == 3) ? 1 : -1; pp5_shy = (la < 2) ? 1 : -1;
  pp5_air = (int)v3_rel(9, 34, 2.5f);
  p5_ink = v3_centre(10) ? Q.ink : (int)(v3_bi(10) * 90);
  pp5_paperGrid = v3_centre(11) ? Q.paperGrid : v3_k(11) > 512;
  pp5_floor = Q.floor;
  pp5_full = (int)v3_rel(12, 2600, 3.0f);
  int feedSp = (int)v3_rel(13, 3, 3.0f); if (feedSp < 1) feedSp = 1;
  pp5_mixB = v3_centre(14) ? Q.mixB : (int)(v3_u(14) * 255);
  int cell = v3_centre(15) ? P.cell : 8 + (int)(v3_u(15) * 32);
  if (r * 2 + 2 > cell) r = (cell - 2) / 2;
  if (r > 15) r = 15;
  if (r != pp5_r || cell != pp5_cell) { pp5_r = r; pp5_cell = cell; pp5_fresh = true; }
  for (int k = 0; k <= 2 * pp5_r && k < 32; k++) {                  // cylinder shading, lit a little off-centre towards the light
    float u = (float)(k - pp5_r) / (pp5_r + 0.5f);
    float l = sqrtf(1.0f - u * u) * 0.85f + 0.15f - 0.35f * u * (float)pp5_shy;
    pp5_prof[k] = (uint8_t)v3_clamp((int)(l * 255), 16, 255);
  }
  p5_palette(Q.look, 0.9f);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !pp5_k12 && pp5_feed == 0) pp5_feed = H; pp5_k12 = k12;
  if (pp5_fresh) {                                                  // a blank sheet
    pp5_fresh = false; pp5_feed = 0; pp5_drawn = 0; pp5_nseg = 0; pp5_scroll = 0;
    for (int i = 0; i < PP5_HEADS; i++) { if (i < heads) pp5_newHead(i); else pp5_h[i].alive = 0; }
  }
  // ── the paper feed: the sheet scrolls up and a fresh one comes in below ──
  if (pp5_feed > 0) {
    int n = feedSp * 2; if (n > pp5_feed) n = pp5_feed;
    pp5_scroll += n; pp5_feed -= n;
    if (pp5_feed <= 0) {
      if (pp5_preset == 11) pp5_look = (pp5_look + 1) % 11;
      pp5_fresh = true;
    }
  } else {
    // ── grow ──
    pp5_acc += speed * v3_dt;
    int steps = (int)pp5_acc; pp5_acc -= steps;
    if (steps > 40) steps = 40;
    for (int st = 0; st < steps; st++) {
      for (int i = 0; i < PP5_HEADS; i++) {
        Pp5Head& h = pp5_h[i];
        if (i >= heads) { h.alive = 0; continue; }
        if (!h.alive) { pp5_newHead(i); continue; }
        int x = h.x + h.dx, y = h.y + h.dy;
        h.x = (int16_t)x; h.y = (int16_t)y; pp5_drawn++;
        if (h.seg >= 0) pp5_seg[h.seg].len++;
        if (((x - pp5_cell / 2) % pp5_cell) == 0 && ((y - pp5_cell / 2) % pp5_cell) == 0) {   // at a cell centre: maybe turn, maybe end
          if (--h.len <= 0) { if (h.seg >= 0) pp5_seg[h.seg].flags |= PP5_CAP; h.alive = 0; continue; }
          bool out = x < pp5_cell || x > W - pp5_cell || y < pp5_cell || y > H - pp5_cell;
          if (out || v3_rnd(256) < pp5_turns / 3) {
            int8_t ndx = h.dy, ndy = h.dx;                         // a right-angle turn, either way
            if (v3_rnd(2)) { ndx = (int8_t)-ndx; ndy = (int8_t)-ndy; }
            if (out) {                                             // at the edge: turn back inwards
              if (x + ndx * pp5_cell < pp5_cell / 2 || x + ndx * pp5_cell > W - pp5_cell / 2 || y + ndy * pp5_cell < pp5_cell / 2 || y + ndy * pp5_cell > H - pp5_cell / 2) { ndx = (int8_t)-ndx; ndy = (int8_t)-ndy; }
              if (x + ndx * pp5_cell < 0 || x + ndx * pp5_cell > W || y + ndy * pp5_cell < 0 || y + ndy * pp5_cell > H) { if (h.seg >= 0) pp5_seg[h.seg].flags |= PP5_CAP; h.alive = 0; continue; }
            }
            h.dx = ndx; h.dy = ndy;
            h.seg = (int16_t)pp5_newSeg(i, PP5_FLANGE0);
            if (h.seg < 0) { h.alive = 0; pp5_drawn = pp5_full + 1; }
          }
        }
      }
    }
    if (pp5_drawn > pp5_full) pp5_feed = H;
  }
  // ── draw the whole sheet ──
  pp5_paper();
  for (int i = 0; i < pp5_nseg; i++) pp5_drawSeg(i);
}
void prog_pipes_draw(int preset) { pipes5_draw(preset); fx5_apply(); }

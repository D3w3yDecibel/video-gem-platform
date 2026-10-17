// =====================================================================
// PROGRAM: SIGNAL — an analog TV with a mind of its own (by Dewey) — Set 3, slot 7
//
// Colour bars, test cards, a blue "NO SIGNAL" screen, a film countdown and
// a bouncing logo — seen through everything that goes wrong with analog
// video: the picture rolls, the tracking slips, colours bleed sideways,
// snow drifts in, hum bars crawl up the screen, lines tear, ghosts echo,
// and the old CRT curves at the edges and switches off to a dot.
//
// How it works: the picture is built one line at a time. For every screen
// line the program works out which line of the source it shows (rolling,
// collapsing), makes that source line, then copies it across with the
// line's own wobble, tear, colour bleed, ghost, snow and scanline.
//
// Presets:
//   k0 Colour Bars   — SMPTE bars on a slightly worn tape
//   k1 Test Card     — a test card with its own moving clock
//   k2 No Signal     — the blue screen, with the box bouncing round
//   k3 VHS Tracking  — a tape with bad tracking: noise bands, bleed, PLAY
//   k4 Vertical Roll — the picture won't hold still
//   k5 Channel Surf  — flicking through the channels
//   k6 Hum Bars      — mains hum crawling through a ghosting picture
//   k7 Glitch        — a corrupted signal: torn lines and displaced blocks
//   k8 CRT Off       — the set switches off to a dot … and back on
//   k9 Dead Channel  — snow, with a station faintly underneath
//   k10 Countdown    — a film leader counting down
//   k11 Broadcast    — a bit of everything, changing every few seconds
//
//   k12 — CHANGE CHANNEL
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Picture (centre = true colour; then sepia, green, amber, cyan, hot, mono, night)
//   p1 Speed (far left = freeze)   p2 Snow   p3 Tracking   p4 Roll   p5 Wobble
//   p6 Colour Bleed   p7 Scanlines   p8 Tear   p9 Hum Bars   p10 Channel (centre = preset)
//   p11 Glitch   p12 Ghost   p13 Channel Time (how often Channel Surf flicks)
//   p14 Colour (left = black and white, right = oversaturated)   p15 CRT (curve and dark corners)
// =====================================================================

#include "vg3.h"
#include "wh_lut.h"                                // (borrowed from WORMHOLE: angles for the countdown sweep)

// ─── Shared memory ────────────────────────────────────────────────────
#define SG_PAD 48
static uint8_t* sg_line;                           // one source line, with padding either side
static uint8_t* sg_colAdd;                         // per-column brightness change (CRT corners)
static void sg_mem() { sg_line = v3_scratch; sg_colAdd = v3_scratch + 512; }

// ─── Colours: (level << 4) | hue ─────────────────────────────────────
// hues: 0 grey, 1 yellow, 2 cyan, 3 green, 4 magenta, 5 red, 6 blue, 7 orange,
//       8 -I navy, 9 +Q purple, 10 skin, 11 OSD green, 12 sky blue, 13 pink, 14 lime, 15 violet
static const uint8_t sg_hues[16][3] = {
  {255,255,255},{255,255,0},{0,255,255},{0,255,0},{255,0,255},{255,0,0},{0,0,255},{255,140,0},
  {0,60,110},{80,0,140},{240,180,140},{60,255,90},{60,140,255},{255,110,190},{180,255,40},{170,90,255},
};
#define SG_NTINT 8
static const uint8_t sg_tints[SG_NTINT][3] = {      // multiplies the picture (and sets how much colour survives)
  {255,255,255},{255,215,160},{120,255,140},{255,190,80},{140,230,255},{255,240,240},{255,255,255},{110,255,110},
};
static const uint8_t sg_tintSat[SG_NTINT] = {255, 70, 0, 0, 90, 255, 0, 40};
static inline uint8_t sg_px(int lev, int hue) { return (uint8_t)((v3_clamp(lev, 0, 15) << 4) | hue); }

// draw the part of a text that falls on source line y into the line buffer
static void sg_textRow(uint8_t* L, int y, const char* s, int tx, int ty, int sc, uint8_t pix, int box, uint8_t boxPix) {
  int n = (int)strlen(s);
  int w = n * 6 * sc - sc, h = 7 * sc;
  if (box) {
    if (y < ty - box || y >= ty + h + box) return;
    for (int x = tx - box; x < tx + w + box; x++) if (x >= -SG_PAD && x < W + SG_PAD) L[SG_PAD + x] = boxPix;
  }
  if (y < ty || y >= ty + h) return;
  int r = (y - ty) / sc;
  for (int i = 0; i < n; i++) {
    uint8_t bits = v3_font[v3_glyph(s[i])][r];
    for (int c = 0; c < 5; c++) if (bits & (16 >> c)) {
      int x0 = tx + (i * 6 + c) * sc;
      for (int k = 0; k < sc; k++) { int x = x0 + k; if (x >= -SG_PAD && x < W + SG_PAD) L[SG_PAD + x] = pix; }
    }
  }
}

// ─── Sources ──────────────────────────────────────────────────────────
enum { SC_BARS, SC_CARD, SC_BLUE, SC_LOGO, SC_STATIC, SC_COUNT, SC_N };
static float sg_t = 0;
static float sg_bx = 60, sg_by = 50, sg_bvx = 55, sg_bvy = 38;   // bouncing box
static int sg_logoHue = 1;
static int sg_count = 8;
static uint32_t sg_rng = 1;
static inline uint32_t sg_r() { uint32_t x = sg_rng; x ^= x << 13; x ^= x >> 17; x ^= x << 5; sg_rng = x; return x; }

static void sg_fill(uint8_t* L, int x0, int x1, uint8_t p) {
  if (x0 < -SG_PAD) x0 = -SG_PAD; if (x1 > W + SG_PAD) x1 = W + SG_PAD;
  if (x1 > x0) memset(L + SG_PAD + x0, p, x1 - x0);
}
static void sg_source(int src, uint8_t* L, int y) {
  switch (src) {
    case SC_BARS: {
      static const uint8_t top[7] = {0, 1, 2, 3, 4, 5, 6};
      if (y < 160) {
        sg_fill(L, -SG_PAD, 0, sg_px(12, 0));
        for (int i = 0; i < 7; i++) sg_fill(L, i * W / 7, (i + 1) * W / 7, sg_px(12, top[i]));
        sg_fill(L, W, W + SG_PAD, sg_px(12, 6));
      } else if (y < 180) {
        static const uint8_t rev[7] = {6, 0, 4, 0, 2, 0, 0};
        for (int i = 0; i < 7; i++) sg_fill(L, i * W / 7, (i + 1) * W / 7, rev[i] == 0 && (i & 1) ? sg_px(1, 0) : sg_px(12, rev[i]));
        sg_fill(L, -SG_PAD, 0, sg_px(12, 6)); sg_fill(L, W, W + SG_PAD, sg_px(12, 0));
      } else {
        sg_fill(L, -SG_PAD, W + SG_PAD, sg_px(1, 0));
        sg_fill(L, 0, 57, sg_px(10, 8)); sg_fill(L, 57, 114, sg_px(15, 0)); sg_fill(L, 114, 171, sg_px(10, 9));
        sg_fill(L, 228, 243, sg_px(0, 0)); sg_fill(L, 243, 258, sg_px(2, 0)); sg_fill(L, 258, 273, sg_px(3, 0));
      }
      break;
    }
    case SC_CARD: {
      // grey background, white grid, a big circle with bars, steps, a sweep clock and gratings — drawn in spans
      int dy = y - 120;
      bool gridRow = (y % 24) == 0;
      sg_fill(L, -SG_PAD, W + SG_PAD, gridRow ? sg_px(14, 0) : sg_px(5, 0));
      if (!gridRow) for (int x = 8 - 24; x < W + SG_PAD; x += 24) if (x >= -SG_PAD) L[SG_PAD + x] = sg_px(14, 0);
      if (y > 20 && y < 44) { sg_fill(L, 17, 64, sg_px(12, 13)); sg_fill(L, 257, 304, sg_px(12, 14)); }
      if (y > 196 && y < 220) { sg_fill(L, 17, 64, sg_px(12, 12)); sg_fill(L, 257, 304, sg_px(12, 7)); }
      int ry = 104 * 104 - dy * dy;
      if (ry > 0) {
        int wo = (int)v3_isqrt((uint32_t)ry);                                   // the circle's white rim
        int wi2 = 100 * 100 - dy * dy, wi = wi2 > 0 ? (int)v3_isqrt((uint32_t)wi2) : 0;
        sg_fill(L, 160 - wo, 160 + wo + 1, sg_px(15, 0));
        if (wi > 0) {
          int xa = 160 - wi, xb = 160 + wi + 1;
          if (dy < -60) { for (int k = 0; k < 7; k++) { int a = 160 - 161 + k * 46; sg_fill(L, a < xa ? xa : a, a + 46 > xb ? xb : a + 46, sg_px(12, 1 + k % 6)); } }
          else if (dy < -26) { for (int k = 0; k < 8; k++) { int a = 60 + k * 25; sg_fill(L, a < xa ? xa : a, a + 25 > xb ? xb : a + 25, sg_px(3 + k * 12 / 7, 0)); } }
          else if (dy < 26) {
            sg_fill(L, xa, xb, sg_px(2, 0));
            if (v3_abs(dy) < 2) sg_fill(L, xa, xb, sg_px(15, 0));
            sg_fill(L, 159, 161, sg_px(15, 0));
            if (v3_abs(dy) < 24) {                                              // the sweep hand
              int hand = ((int)(sg_t * 64)) & 1023;
              int hw = (int)v3_isqrt((uint32_t)(24 * 24 - dy * dy));
              for (int x = 160 - hw; x <= 160 + hw; x++) {
                int a = v3_atan(dy, x - 160), da = (hand - a) & 1023;
                L[SG_PAD + x] = (da < 12 || da > 1012) ? sg_px(15, 5) : sg_px(4 + (da < 200 ? (200 - da) / 25 : 0), 12);
              }
            }
          }
          else if (dy < 60) {                                                   // multiburst gratings
            for (int band = 0; band < 6; band++) {
              int a = 60 + band * 34, b = a + 34, f = 2 + band * 2;
              if (a < xa) a = xa; if (b > xb) b = xb;
              for (int x = a; x < b; x++) L[SG_PAD + x] = ((x / f) & 1) ? sg_px(14, 0) : sg_px(2, 0);
            }
          }
          else sg_fill(L, xa, xb, sg_px(10, 10));
        }
      }
      sg_textRow(L, y, "VIDEO GEM", 160 - 26, 194, 1, sg_px(15, 0), 3, sg_px(0, 0));
      break;
    }
    case SC_BLUE:
      sg_fill(L, -SG_PAD, W + SG_PAD, sg_px(9, 6));
      sg_textRow(L, y, "NO SIGNAL", (int)sg_bx, (int)sg_by, 2, sg_px(15, 0), 6, sg_px(1, 0));
      break;
    case SC_LOGO:
      sg_fill(L, -SG_PAD, W + SG_PAD, sg_px(0, 0));
      sg_textRow(L, y, "VIDEO", (int)sg_bx, (int)sg_by, 4, sg_px(15, sg_logoHue), 0, 0);
      sg_textRow(L, y, "GEM", (int)sg_bx + 24, (int)sg_by + 34, 4, sg_px(13, (sg_logoHue + 3) % 7 + 1), 0, 0);
      break;
    case SC_STATIC:                                               // snow, with the test card faintly underneath
      sg_source(SC_CARD, L, y);
      for (int x = -SG_PAD; x < W + SG_PAD; x += 4) {
        uint32_t r = sg_r();
        for (int k = 0; k < 4; k++) { uint8_t c = L[SG_PAD + x + k]; L[SG_PAD + x + k] = sg_px(((c >> 4) * 5 >> 4) + ((r >> (k * 8)) & 11), (r >> (k * 8 + 6)) & 1 ? (c & 15) : 0); }
      }
      break;
    default: {                                                    // film countdown
      int dy = y - 120;
      int frac = (int)((sg_t - floorf(sg_t)) * 1024);
      sg_fill(L, -SG_PAD, W + SG_PAD, sg_px(10, 0));
      int ry = 93 * 93 - dy * dy;
      if (ry > 0) {
        int w = (int)v3_isqrt((uint32_t)ry);
        for (int x = 160 - w; x <= 160 + w; x++) {
          int dx = x - 160, r2 = dx * dx + dy * dy;
          uint8_t p = sg_px(10, 0);
          int qx = v3_abs(dx) >> 1, qy = v3_abs(dy) >> 1;
          int q = (int)(wh_lut[qy * WH_QW + qx] & 255);
          int a = dx >= 0 ? (dy >= 0 ? q : 1024 - q) : (dy >= 0 ? 512 - q : 512 + q);
          if (((a + 256) & 1023) < frac) p = sg_px(6, 0);           // the sweep, clockwise from the top
          if ((r2 > 88 * 88) || (r2 > 70 * 70 && r2 < 73 * 73)) p = sg_px(15, 0);
          L[SG_PAD + x] = p;
        }
      }
      sg_fill(L, 159, 161, sg_px(2, 0));
      if (v3_abs(dy) < 2) sg_fill(L, -SG_PAD, W + SG_PAD, sg_px(2, 0));
      char d[2] = {(char)('0' + sg_count), 0};
      sg_textRow(L, y, d, 160 - 22, 120 - 38, 11, sg_px(1, 0), 0, 0);
      break;
    }
  }
}

// ─── Presets ──────────────────────────────────────────────────────────
struct SgLook { uint8_t src, snow, track, roll, wob, bleed, scan, tear, hum, glitch, ghost, crt, surf, power, osd; };
static const SgLook sg_looks[11] = {
  //  src        snow trk roll wob bleed scan tear hum glt ghost crt surf power osd
  {SC_BARS,     6,  10,  0, 15, 25, 40,  5, 10,  0,  0, 40, 0, 0, 0},   // Colour Bars
  {SC_CARD,     8,   5,  0, 10, 15, 45,  0, 10,  0,  0, 50, 0, 0, 0},   // Test Card
  {SC_BLUE,     4,   0,  0,  8, 10, 50,  0,  0,  0,  0, 55, 0, 0, 0},   // No Signal
  {SC_BARS,    15,  85,  0, 35, 60, 40, 15, 10,  5,  10, 40, 0, 0, 1},  // VHS Tracking
  {SC_CARD,     8,  10, 60, 10, 15, 40,  0, 10,  0,  0, 45, 0, 0, 0},   // Vertical Roll
  {SC_BARS,    10,  15,  0, 15, 25, 45,  5, 10,  0,  0, 50, 1, 0, 2},   // Channel Surf
  {SC_CARD,     8,  10,  0, 15, 20, 40,  5, 85,  0, 60, 45, 0, 0, 0},   // Hum Bars
  {SC_LOGO,    10,  30,  0, 25, 70, 30, 60, 10, 85, 20, 40, 0, 0, 0},   // Glitch
  {SC_CARD,     5,   5,  0,  8, 15, 45,  0,  5,  0,  0, 60, 0, 1, 0},   // CRT Off
  {SC_STATIC,  35,  20, 10, 20, 10, 40,  5, 40,  0, 35, 50, 0, 0, 0},   // Dead Channel
  {SC_COUNT,   10,   5,  0, 10, 10, 30,  0, 15,  0,  0, 55, 0, 0, 0},   // Countdown
};
static const char* const sg_presetNames[12] = {
  "Colour Bars", "Test Card", "No Signal", "VHS Tracking", "Vertical Roll", "Channel Surf",
  "Hum Bars", "Glitch", "CRT Off", "Dead Channel", "Countdown", "Broadcast"
};
static const char* const sg_labels[16] = {
  "Picture", "Speed", "Snow", "Tracking", "Roll", "Wobble", "Colour Bleed", "Scanlines",
  "Tear", "Hum Bars", "Channel", "Glitch", "Ghost", "Channel Time", "Colour", "CRT"
};
const char* prog_signal_name() { return "SIGNAL"; }
const char* prog_signal_character() { return "An analog TV: colour bars and test cards rolling, tearing, bleeding and snowing"; }
const char* prog_signal_presetName(int preset) { return (preset >= 0 && preset < 12) ? sg_presetNames[preset] : NULL; }
const char* prog_signal_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? sg_labels[pot] : ""; }
uint8_t prog_signal_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int sg_preset = -1, sg_look = 0, sg_src = 0, sg_chan = 3;
static float sg_roll = 0, sg_rollV = 0, sg_burst = 0, sg_osdT = 0, sg_surfT = 0, sg_powerT = 0, sg_trackY = 0, sg_humY = 0, sg_bcastT = 0;
static bool sg_k12was = false;
static uint8_t sg_glitchY[6], sg_glitchH[6]; static int8_t sg_glitchX[6]; static uint8_t sg_glitchHue[6];
static float sg_glitchT = 0;
static uint8_t sg_clampLev[112];                   // level −48 … 63 → clamped level << 4

static void sg_palette(int tint, float sat) {
  int ts = sg_tintSat[tint];
  float s = sat * ts / 255.0f;
  for (int h = 0; h < 16; h++) {
    int r = sg_hues[h][0], g = sg_hues[h][1], b = sg_hues[h][2];
    int y = (r * 77 + g * 150 + b * 29) >> 8;
    r = v3_clamp((int)(y + (r - y) * s), 0, 255); g = v3_clamp((int)(y + (g - y) * s), 0, 255); b = v3_clamp((int)(y + (b - y) * s), 0, 255);
    r = r * sg_tints[tint][0] / 255; g = g * sg_tints[tint][1] / 255; b = b * sg_tints[tint][2] / 255;
    v3_setLH(h, r, g, b, 1.15f, 0);
  }
}

void prog_signal_init() {
  sg_mem();
  for (int i = 0; i < 112; i++) sg_clampLev[i] = (uint8_t)(v3_clamp(i - 48, 0, 15) << 4);
  v3_reset();
  sg_preset = -1;
}

static void sg_changeChannel(bool surf) {
  sg_burst = 1.0f; sg_osdT = 2.2f; sg_rollV = 300 + v3_rnd(300);
  sg_chan = 2 + v3_rnd(60);
  if (surf) { int ns; do { ns = v3_rnd(SC_N); } while (ns == sg_src); sg_src = ns; }
}

void prog_signal_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  sg_mem();
  v3_frame();
  float dt = v3_dt;
  bool bcast = preset == 11;
  if (preset != sg_preset) {
    sg_preset = preset;
    sg_look = bcast ? v3_rnd(11) : preset;
    sg_src = sg_looks[sg_look].src;
    sg_roll = 0; sg_rollV = 0; sg_burst = 0; sg_powerT = 0; sg_bcastT = 0;
    if (sg_looks[sg_look].osd) sg_osdT = 2.2f;
  }
  if (bcast) {
    sg_bcastT += dt;
    if (sg_bcastT > 9.0f) { sg_bcastT = 0; sg_look = (sg_look + 1 + v3_rnd(10)) % 11; sg_changeChannel(false); sg_src = sg_looks[sg_look].src; }
  }
  const SgLook& L = sg_looks[sg_look];

  // ── knobs ──
  float speed = v3_speed(1, 3.0f);
  #define SG_K(i, own) (v3_centre(i) ? (own) / 100.0f : v3_u(i))
  float snow = SG_K(2, L.snow), track = SG_K(3, L.track), roll = SG_K(4, L.roll), wob = SG_K(5, L.wob);
  float bleed = SG_K(6, L.bleed), scan = SG_K(7, L.scan), tear = SG_K(8, L.tear), hum = SG_K(9, L.hum);
  float glitch = SG_K(11, L.glitch), ghost = SG_K(12, L.ghost), crt = SG_K(15, L.crt);
  #undef SG_K
  int chanK = v3_pick(10, -1, SC_N);
  float surfEvery = v3_centre(13) ? 4.0f : 1.5f + v3_u(13) * 10.0f;
  float sat = v3_centre(14) ? 1.0f : v3_u(14) * 1.6f;
  int tint = v3_centre(0) ? 0 : v3_clamp(v3_k(0) * SG_NTINT / 1024, 0, SG_NTINT - 1);
  if (chanK >= 0 && chanK != sg_src && !L.surf) { sg_src = chanK; sg_changeChannel(false); }
  float sdt = dt * speed;
  sg_t += sdt;

  // ── channel changes ──
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !sg_k12was) sg_changeChannel(chanK < 0);
  sg_k12was = k12;
  if (L.surf) { sg_surfT += dt; if (sg_surfT > surfEvery) { sg_surfT = 0; sg_changeChannel(true); } }
  sg_burst -= dt / 0.4f; if (sg_burst < 0) sg_burst = 0;
  sg_osdT -= dt;

  // ── moving parts ──
  sg_bx += sg_bvx * sdt; sg_by += sg_bvy * sdt;
  int bw = sg_src == SC_LOGO ? 116 : 106, bh = sg_src == SC_LOGO ? 62 : 14;
  if (sg_bx < 8) { sg_bx = 8; sg_bvx = fabsf(sg_bvx); sg_logoHue = sg_logoHue % 7 + 1; }
  if (sg_bx > W - 8 - bw) { sg_bx = (float)(W - 8 - bw); sg_bvx = -fabsf(sg_bvx); sg_logoHue = sg_logoHue % 7 + 1; }
  if (sg_by < 8) { sg_by = 8; sg_bvy = fabsf(sg_bvy); sg_logoHue = sg_logoHue % 7 + 1; }
  if (sg_by > H - 8 - bh) { sg_by = (float)(H - 8 - bh); sg_bvy = -fabsf(sg_bvy); sg_logoHue = sg_logoHue % 7 + 1; }
  sg_count = 8 - ((int)sg_t % 7);
  // vertical roll: a steady roll, plus the slip after a channel change settling down
  sg_rollV *= expf(-dt * 2.5f);
  float rv = roll * roll * 260.0f * speed + sg_rollV;
  if (roll < 0.05f && sg_roll > 0.5f && rv < 150.0f) rv = 150.0f;   // finish the roll: the picture locks back in at the top
  sg_roll += rv * dt;
  if (roll < 0.05f && sg_roll >= 260.0f) sg_roll = 0; else sg_roll = fmodf(sg_roll, 260.0f);
  int rollOff = (int)sg_roll;
  sg_trackY += sdt * 37.0f; if (sg_trackY > H + 60) sg_trackY = -60;
  sg_humY -= sdt * 18.0f; if (sg_humY < 0) sg_humY += H;
  // glitch blocks: re-chosen a few times a second
  sg_glitchT -= sdt;
  if (sg_glitchT <= 0) {
    sg_glitchT = 0.15f + v3_rndf() * 0.5f;
    for (int i = 0; i < 6; i++) { sg_glitchY[i] = (uint8_t)v3_rnd(H); sg_glitchH[i] = (uint8_t)(2 + v3_rnd(18)); sg_glitchX[i] = (int8_t)(v3_rnd(80) - 40); sg_glitchHue[i] = (uint8_t)v3_rnd(16); }
  }
  // power cycle (CRT Off): on 5 s → collapse 0.6 s → dot 0.8 s → dark 1 s → back on 0.5 s
  float vScale = 1.0f, hScale = 1.0f, powerGlow = 0;
  if (L.power) {
    sg_powerT += dt; if (sg_powerT > 7.8f) sg_powerT = 0;
    float t = sg_powerT;
    if (t < 5.5f) {}
    else if (t < 6.0f) { float u = (t - 5.5f) / 0.5f; vScale = 1.0f - u * 0.995f; powerGlow = u; }
    else if (t < 6.7f) { float u = (t - 6.0f) / 0.7f; vScale = 0.005f; hScale = 1.0f - u * 0.98f; powerGlow = 1.0f - u * 0.3f; }
    else if (t < 7.3f) { float u = (t - 6.7f) / 0.6f; vScale = 0.005f; hScale = 0.02f; powerGlow = 0.7f - u * 0.4f; }   // the afterglow dot
    else { float u = (t - 7.3f) / 0.5f; vScale = u; hScale = 0.3f + 0.7f * u; powerGlow = 1.0f - u; }
  }

  // ── colours ──
  sg_palette(tint, sat);

  // ── per-column brightness: the CRT's dark corners ──
  for (int x = 0; x < W; x++) { int d = v3_abs(x - 160) - 110; sg_colAdd[x] = (uint8_t)(d > 0 ? (int)(d * crt * 5 / 50) : 0); }

  // ── lines ──
  int bleedPx = (int)(bleed * 9);
  int snowP = (int)(snow * snow * 255 + sg_burst * 200); if (snowP > 255) snowP = 255;
  int ghostOff = 12 + (int)(ghost * 10), ghostK = (int)(ghost * 11);
  int wobAmp = (int)(wob * 6 * 16), trackAmp = (int)(track * 30);
  uint8_t* Lb = sg_line;
  sg_rng ^= (uint32_t)(sg_t * 1000) * 2654435761u + 1;
  int ty = (int)sg_trackY, hy = (int)sg_humY;
  // snow is heavy work: on the dead channel only every other line is redrawn each frame (like interlaced video)
  static int field = 0; field ^= 1;
  bool il = sg_src == SC_STATIC;
  for (int y = il ? field : 0; y < H; y += il ? 2 : 1) {
    uint8_t* out = v3_buf + y * W;
    // which source line? (rolling, and the picture collapsing when the set switches off)
    int dy = y - 120;
    if (vScale < 1.0f) {
      if (vScale <= 0.0f || v3_abs(dy) > (int)(120 * vScale) + (vScale < 0.02f ? 1 : 0)) { memset(out, 0, W); continue; }
    }
    int sy = vScale < 1.0f && vScale > 0.0f ? 120 + (int)(dy / (vScale < 0.008f ? 0.008f : vScale)) : y;
    sy = sy + rollOff;
    while (sy >= 260) sy -= 260;
    if (sy >= 240) {                                               // the blanking bar between frames
      memset(out, 0, W);
      if (sy >= 248 && sy < 252) memset(out + 20, sg_px(3, 0), 60);
      continue;
    }
    if (sy < 0) sy = 0;
    sg_source(sg_src, Lb, sy);
    // OSD over the source
    if (sg_osdT > 0 || L.osd == 1) {
      char buf[8];
      if (L.osd == 1) { if ((int)(sg_t * 1.2f) & 1) sg_textRow(Lb, sy, "PLAY >", 20, 16, 2, sg_px(15, 11), 0, 0); }
      if (sg_osdT > 0) { buf[0] = 'C'; buf[1] = 'H'; buf[2] = ' '; buf[3] = (char)('0' + sg_chan / 10); buf[4] = (char)('0' + sg_chan % 10); buf[5] = 0; sg_textRow(Lb, sy, buf, 236, 16, 3, sg_px(15, 11), 0, 0); }
    }
    // horizontal: wobble, tracking band, tear, glitch blocks, CRT curve
    int xs = (wobAmp * v3_sin(y * 9 + (int)(sg_t * 300))) >> 18;
    xs += (wobAmp * v3_sin(y * 31 + (int)(sg_t * 170))) >> 19;
    int tdist = v3_abs(y - ty);
    bool inTrack = trackAmp > 0 && tdist < 6 + trackAmp / 3;
    if (inTrack) xs += (int)(sg_r() % (uint32_t)(trackAmp + 1)) - trackAmp / 3;
    if (track > 0.3f && y > H - 6) xs += (int)(sg_r() % 24) - 4;     // head-switching noise at the bottom
    if (tear > 0.05f && (sg_r() & 1023) < (uint32_t)(tear * tear * 40)) xs += (int)(sg_r() % 40) - 20;
    int hueSwap = -1;
    if (glitch > 0.05f) for (int i = 0; i < (int)(glitch * 6 + 0.5f); i++) if (y >= sg_glitchY[i] && y < sg_glitchY[i] + sg_glitchH[i]) { xs += sg_glitchX[i]; if (glitch > 0.5f) hueSwap = sg_glitchHue[i]; }
    if (xs > SG_PAD - 12) xs = SG_PAD - 12; if (xs < -(SG_PAD - 12)) xs = -(SG_PAD - 12);
    // brightness for the whole line: scanlines, hum bar, power glow
    int rowAdd = 0;
    if (y & 1) rowAdd -= (int)(scan * 5 + 0.5f);
    if (hum > 0.02f) { int d = ((y - hy + H) % H); rowAdd += (int)(hum * 4 * v3_sin(d * 1024 / H) / 16384.0f) - (int)(hum * 2); }
    if (powerGlow > 0) rowAdd += (int)(powerGlow * 8);
    if (inTrack) rowAdd += 2;
    int vig = (int)(crt * 3 * dy * dy / (120 * 120));
    rowAdd -= vig;
    // CRT curve: the line is a little wider towards the top and bottom (stepped in 1/256 px)
    int32_t step = (int32_t)(256 * (1.0f - crt * 0.07f * dy * dy / (120.0f * 120.0f)));
    if (hScale < 1.0f) step = hScale > 0.01f ? (int32_t)(256 / hScale) : 256 * 100;
    int32_t pos0 = (160 << 8) - 160 * step + (-xs * 256) + (SG_PAD << 8);
    const int32_t lo = 24 << 8, hi = (W + 2 * SG_PAD - 3) << 8;     // (room on the left for the bleed and the ghost)
    int xa = 0, xb = W - 1;                                        // the columns that land on the picture
    if (pos0 < lo) xa = (int)((lo - pos0 + step - 1) / step);
    if (pos0 + (int32_t)(W - 1) * step > hi) xb = (int)((hi - pos0) / step);
    if (xa > W) xa = W; if (xb < -1) xb = -1;
    if (xa > 0) memset(out, 0, xa);
    if (xb < W - 1) memset(out + xb + 1, 0, W - 1 - xb);
    const uint8_t* lv = sg_clampLev + 48 + rowAdd;                  // level → clamped level, with this line's brightness
    const uint8_t* ca = sg_colAdd;
    int32_t pos = pos0 + xa * step;
    int bh = bleedPx >> 1;
    if (!ghostK && hueSwap < 0) {
      for (int x = xa; x <= xb; x++, pos += step) {
        int i = pos >> 8;
        out[x] = (uint8_t)(lv[(Lb[i] >> 4) - ca[x]] | (Lb[i - bh] & 15));   // colour arrives late: it bleeds to the right
      }
    } else {
      for (int x = xa; x <= xb; x++, pos += step) {
        int i = pos >> 8;
        int lev = (Lb[i] >> 4) - ca[x], hue = Lb[i - bh] & 15;
        if (ghostK) { int g = ((Lb[i - ghostOff] >> 4) * ghostK) >> 4; if (g > lev) { lev = g; hue = Lb[i - ghostOff] & 15; } }
        if (hueSwap >= 0) hue = (hue + hueSwap) & 15;
        out[x] = (uint8_t)(lv[lev] | hue);
      }
    }
    // snow: scattered grey specks (more in the tracking band)
    int snowLocal = inTrack ? (snowP + 120 > 255 ? 255 : snowP + 120) : snowP;
    if (snowLocal > 0) {
      int nsn = (W * snowLocal) >> 8;
      if ((int)(sg_r() & 255) < ((W * snowLocal) & 255)) nsn++;
      for (int k = 0; k < nsn; k++) {
        uint32_t r = sg_r();
        int x = (int)((r & 0xFFFF) * W >> 16);
        out[x] = (uint8_t)(lv[(int)((r >> 16) & 15) - 1] & 0xF0);
      }
    }
  }
}

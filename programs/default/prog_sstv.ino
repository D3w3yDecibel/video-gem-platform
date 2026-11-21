// =====================================================================
// PROGRAM: SSTV — slow-scan television (by Dewey) — Set 7, slot 10
//
// Radio amateurs send pictures as sound: slow-scan TV, one line at a time,
// over the shortwave bands and down from the International Space Station.
// A picture takes seconds to arrive, painted from the top by the scanning
// line, through static and fading signals; when the timing drifts the
// picture slants, and the colours can slip sideways out of register.
// The old picture stays below until the new one covers it.
//
// How it works: the picture lives at half size (160 × 120). A few lines of
// the new picture are worked out each frame — Earth from orbit, a test
// card, mountains, the Moon, a QSL card, a mandala, weather radar, a colour
// grid, a night city, Mars — and the reception spoils them as they land:
// static, fading, slant, colour slip and dropped lines. The scanning line
// glows where the picture is being written.
//
// Presets: k0 ISS · k1 Test Card · k2 Mountains · k3 Moon · k4 QSL · k5 Mandala ·
//          k6 Radar · k7 Colour Grid · k8 Night City · k9 Mars · k10 Storm · k11 Tour
//   k12 — NEXT: start receiving a new picture now
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Scan Rate · p6 Static · p7 Slant · p8 Fading · p9 Hold · p10 Picture
//   p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

#define SV_W 160
#define SV_H 120
enum { SV_ISS, SV_CARD, SV_MOUNTAINS, SV_MOON, SV_QSL, SV_MANDALA, SV_RADAR, SV_GRID, SV_CITY, SV_MARS, SV_NPIC };
struct SvLook { uint8_t pic, rate, noise, slant, fading, hold, mode, pal, flow, spread, scan; };
static const SvLook sv_looks[11] = {
  // pic           rate noise slant fade hold mode pal              flow spr scan
  { SV_ISS,         40,  20,  10,  30,  40,  0, V7P_VASULKA,      0,  90, 2 },   // ISS
  { SV_CARD,        45,  10,   0,  10,  35,  1, V7P_STUDIO,       0, 100, 2 },   // Test Card
  { SV_MOUNTAINS,   40,  15,   5,  20,  40,  2, V7P_SUNSET,       0,  80, 2 },   // Mountains
  { SV_MOON,        35,  20,   0,  25,  40,  0, V7P_GHOST,        0,  40, 2 },   // Moon
  { SV_QSL,         50,  15,  15,  20,  35,  1, V7P_SCANIMATE,    0, 100, 2 },   // QSL
  { SV_MANDALA,     45,  10,   0,  15,  35,  2, V7P_CHICAGO,      2, 100, 2 },   // Mandala
  { SV_RADAR,       50,  15,   0,  15,  35,  0, V7P_PHOSPHOR,     0,  70, 2 },   // Radar
  { SV_GRID,        55,  10,  10,  10,  30,  1, V7P_PAIK,         0, 100, 2 },   // Colour Grid
  { SV_CITY,        40,  20,   5,  25,  40,  2, V7P_BLACKLIGHT,   0,  80, 2 },   // Night City
  { SV_MARS,        40,  20,   0,  25,  40,  0, V7P_KODACHROME,   0,  70, 2 },   // Mars
  { SV_ISS,         45,  75,  30,  90,  25,  1, V7P_GHOST,        0,  60, 2 },   // Storm
};
static const char* const sv_names[12] = { "ISS", "Test Card", "Mountains", "Moon", "QSL", "Mandala", "Radar", "Colour Grid", "Night City", "Mars", "Storm", "Tour" };
static const char* const sv_lbl[6] = { "Scan Rate", "Static", "Slant", "Fading", "Hold", "Picture" };
static const char* const sv_modes[3] = { "ROBOT 36", "MARTIN 1", "SCOTTIE 2" };
const char* prog_sstv_name() { return "SSTV"; }
const char* prog_sstv_character() { return "Slow-scan TV: pictures arriving line by line through static and fading"; }
const char* prog_sstv_presetName(int p) { return (p >= 0 && p < 12) ? sv_names[p] : NULL; }
const char* prog_sstv_potLabel(int p, int pot) { (void)p; return v7_label(pot, sv_lbl); }
uint8_t prog_sstv_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static uint8_t* sv_pic;
static void sv_mem() { sv_pic = v6_mem; }
static float sv_line = 0, sv_holdT = 0, sv_slip = 0, sv_qsb = 0;
static int sv_state = 0, sv_curPic = 0, sv_lastLook = -1;                 // state: 0 header, 1 receiving, 2 holding
static uint32_t sv_seed = 7;
static float sv_rate, sv_noise, sv_slant, sv_fading, sv_hold;

static void sv_newPicture(int pic) { sv_curPic = pic; sv_seed = v3_rand() | 1; sv_line = 0; sv_state = 0; sv_holdT = 0; sv_slip = 0; }
void prog_sstv_init() { sv_mem(); v6_init(); sv_lastLook = -1; memset(sv_pic, 0, SV_W * SV_H); }

// ─── The pictures: one colour per (x, y), worked out a line at a time ───
static inline uint32_t sv_h(int a, int b) { return v3_hash((uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ sv_seed); }
static int sv_fbm(int x, int y, int sc) {                                // soft noise 0 … 255
  int v = 0, amp = 128, tot = 0;
  for (int o = 0; o < 3; o++) {
    int s = sc >> o; if (s < 2) s = 2;
    int xi = x / s, yi = y / s, fx = (x % s) * 256 / s, fy = (y % s) * 256 / s;
    int a = sv_h(xi + o * 999, yi) & 255, b = sv_h(xi + 1 + o * 999, yi) & 255, c = sv_h(xi + o * 999, yi + 1) & 255, d = sv_h(xi + 1 + o * 999, yi + 1) & 255;
    int t = a + (((b - a) * fx) >> 8), u = c + (((d - c) * fx) >> 8);
    v += (t + (((u - t) * fy) >> 8)) * amp; tot += amp * 1; amp >>= 1;
  }
  return v / tot;
}
static bool sv_textAt(const char* s, int x, int y, int x0, int y0, int sc) {
  int cx = (x - x0), cy = (y - y0);
  if (cx < 0 || cy < 0 || cy >= 7 * sc) return false;
  int i = cx / (6 * sc), b = (cx / sc) % 6;
  if (i >= (int)strlen(s) || b == 5) return false;
  return (v3_font[v3_glyph(s[i])][cy / sc] & (16 >> b)) != 0;
}
static uint8_t sv_pixel(int pic, int x, int y) {
  int dx = x - SV_W / 2, dy = y - SV_H / 2;
  uint32_t r = sv_h(x, y);
  switch (pic) {
    case SV_ISS: {                                                       // Earth's limb from orbit, stars, ARISS
      if (sv_textAt("ARISS", x, y, 6, 6, 1)) return V6C(15, 15);
      int ex = x - 80, ey = y - 330, d2 = ex * ex + ey * ey;
      if (d2 < 245 * 245) {
        int cl = sv_fbm(x + 40, y * 2, 24);
        int edge = 245 * 245 - d2;
        if (edge < 3000) return V6C(9 + edge / 600, 3);                  // the thin blue air at the edge
        if (cl > 150) return V6C(12 + (cl - 150) / 40, 12);              // clouds
        return V6C(5 + (cl >> 5), (cl >> 6) + 2 + ((sv_fbm(x, y, 40) > 140) ? 6 : 0));   // sea and land
      }
      if ((r & 255) < 3) return V6C(8 + (r >> 8 & 7), 14);               // stars
      return V6C(0, 0);
    }
    case SV_CARD: {
      if (y < 40) return V6C(13, (x * 8 / SV_W) * 2);
      if (y < 50) return V6C(x * 16 / SV_W, 0);
      int rr = dx * dx + (y - 85) * (y - 85);
      if (sv_textAt("N6VG", x, y, 56, 102, 2)) return V6C(15, 14);
      if (rr < 30 * 30 && rr > 26 * 26) return V6C(15, 4);
      if (rr < 26 * 26) return V6C(6 + ((x / 6 + y / 6) & 1) * 6, 8);
      if (sv_textAt("TEST", x, y, 4, 55, 1)) return V6C(14, 2);
      return ((x % 10) == 0 || (y % 10) == 0) ? V6C(10, 6) : V6C(3, 6);
    }
    case SV_MOUNTAINS: {
      int sun = (x - 112) * (x - 112) + (y - 38) * (y - 38);
      for (int layer = 0; layer < 3; layer++) {
        int ridge = 50 + layer * 16 + ((sv_fbm(x * (2 + layer), layer * 300, 64) - 128) * (40 - layer * 10)) / 128;
        if (y > ridge && y < 92) return V6C(2 + layer * 2 + (sv_fbm(x * 3, y * 3, 8) >> 6), 10 + layer);
      }
      if (y >= 92) {                                                       // the lake: the sky upside down, rippling
        int ry = 184 - y + ((sv_h(y, 3) & 3) - 1);
        int s2 = (x - 112) * (x - 112) + (ry - 38) * (ry - 38);
        if (s2 < 120) return V6C(10, 4);
        return V6C(2 + (ry * 6 / 92), 7 + ((x + y) & 1));
      }
      if (sun < 110) return V6C(15, 4);
      if (sun < 250) return V6C(12, 5);
      return V6C(4 + y * 8 / 90, (y * 6 / 90) & 15);
    }
    case SV_MOON: {
      int d2 = dx * dx * 4 / 3 + dy * dy;
      if (d2 < 48 * 48) {
        int v = 10 + ((sv_fbm(x * 2, y * 2, 20) - 128) >> 5);
        for (int c = 0; c < 9; c++) {                                     // craters
          uint32_t hc = v3_hash(sv_seed + c * 31u);
          int cx = SV_W / 2 - 40 + (int)(hc % 80), cy = SV_H / 2 - 35 + (int)((hc >> 8) % 70), cr = 3 + (int)((hc >> 16) % 9);
          int e2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
          if (e2 < cr * cr) { v -= 3; if (e2 > (cr - 1) * (cr - 1) && x < cx) v += 5; }
        }
        if (dx > 20 + dy * dy / 120) v -= 4;                               // the terminator
        return V6C(v3_clamp(v, 1, 15), 8);
      }
      return (r & 255) < 2 ? V6C(10, 8) : V6C(0, 0);
    }
    case SV_QSL: {
      if (x < 4 || x >= SV_W - 4 || y < 4 || y >= SV_H - 4) return V6C(13, 4);
      if (sv_textAt("73", x, y, 34, 20, 7)) return V6C(15, 10);
      if (sv_textAt("DE N6VG", x, y, 38, 88, 2)) return V6C(14, 2);
      return V6C(2 + ((x + y) >> 4 & 1), 6 + (y >> 5));
    }
    case SV_MANDALA: {
      int a = v3_atan(dy, dx), rr = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy));
      int petals = 8, w = v3_abs(((a * petals) & 1023) - 512);
      int v = 8 + ((v3_sin(rr * 40 - w) + v3_cos(a * 12 + rr * 20)) >> 12);
      return V6C(v3_clamp(v, 0, 15), ((rr >> 3) + (w >> 7)) & 15);
    }
    case SV_RADAR: {
      int rr = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy));
      if (rr > 58) return V6C(0, 0);
      if (rr % 15 == 0 || dx == 0 || dy == 0) return V6C(6, 0);
      int n = sv_fbm(x * 2, y * 2, 30);
      if (n > 190) return V6C(15, 4);                                      // heavy rain
      if (n > 165) return V6C(12, 2);
      if (n > 140) return V6C(9, 0);
      return V6C(1 + ((sv_fbm(x * 4, y * 4, 12) > 180) ? 2 : 0), 8);       // land
    }
    case SV_GRID: { int gx = x * 8 / SV_W, gy = y * 6 / SV_H; return ((x % 20) < 2 || (y % 20) < 2) ? V6C(2, 0) : V6C(8 + gy, (gx * 2 + gy * 5) & 15); }
    case SV_CITY: {
      int bw = 6 + (int)(sv_h(x / 10, 7) % 10), bx = x / bw;
      int top = 40 + (int)(sv_h(bx, 11) % 60);
      if (y > top) {
        bool win = ((x % 4) == 1 || (x % 4) == 2) && (y % 5) < 2 && (sv_h(x / 4, y / 5) & 7) < 3;
        return win ? V6C(12 + (sv_h(x, y / 5) & 3), 4) : V6C(1, 10);
      }
      if ((x - 30) * (x - 30) + (y - 22) * (y - 22) < 64) return V6C(14, 8);   // the moon
      return (r & 511) < 3 ? V6C(9, 8) : V6C(1 + y / 30, 12);
    }
    default: {                                                             // Mars: dunes under a dusty sky
      int hz = 52 + ((sv_fbm(x, 0, 50) - 128) >> 4);
      if (y < hz) return V6C(6 + y * 4 / hz, 4);
      int dune = (v3_sin(x * 9 + y * 25 + (sv_fbm(x, y, 30) >> 1)) >> 12) + (sv_fbm(x * 2, y * 2, 16) >> 6);
      return V6C(v3_clamp(6 + dune, 2, 14), 2 + ((y - hz) > 40 ? 1 : 0));
    }
  }
}

void prog_sstv_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  sv_mem();
  const SvLook& L = sv_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&sv_rate, v3_centre(5) ? L.rate / 100.0f : v3_u(5), rate);
  v6_ease(&sv_noise, v7_kn(6, L.noise / 100.0f), rate);
  v6_ease(&sv_slant, v7_kb(7, L.slant / 100.0f), rate);
  v6_ease(&sv_fading, v7_kn(8, L.fading / 100.0f), rate);
  v6_ease(&sv_hold, v7_kn(9, L.hold / 100.0f), rate);
  int pic = v3_centre(10) ? L.pic : v3_clamp(v3_k(10) * SV_NPIC / 1024, 0, SV_NPIC - 1);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 60);

  float sdt = v6_f.sdt, t = v6_t;
  if (look != sv_lastLook || v6_k12hit || (pic != sv_curPic && sv_state == 2)) { sv_newPicture(pic); sv_lastLook = look; }
  // the signal: fading (QSB) is a slow swell plus a little flutter
  sv_qsb = 0.5f + 0.35f * sinf(t * 0.7f) + 0.15f * sinf(t * 3.1f);
  float strength = 1.0f - sv_fading * (1.0f - sv_qsb);
  float noise = v3_clampf(sv_noise * (1.6f - strength), 0, 1);

  if (sv_state == 0) { sv_holdT += sdt; if (sv_holdT > 0.8f) { sv_state = 1; sv_holdT = 0; } }   // the header tone
  else if (sv_state == 1) {
    float lps = 6.0f + sv_rate * sv_rate * 70.0f;                          // lines per second
    float target = sv_line + lps * sdt;
    int from = (int)sv_line, to = (int)target; if (to > SV_H) to = SV_H;
    uint8_t row[SV_W];
    for (int y = from; y < to; y++) {
      sv_slip += sv_slant * 0.35f;
      int slip = (int)sv_slip;
      int mis = (int)(noise * 3 + (L.mode == 1 ? 2 : 0));                  // colour out of register
      bool drop = (int)(v3_rand() % 1000) < (int)(noise * noise * 60);
      for (int x = 0; x < SV_W; x++) {
        int sx = ((x - slip) % SV_W + SV_W) % SV_W;
        row[x] = sv_pixel(pic, sx, y);
      }
      uint8_t* dst = sv_pic + y * SV_W;
      for (int x = 0; x < SV_W; x++) {
        int l = row[x] >> 4, h = row[(x + SV_W - mis) % SV_W] & 15;
        uint32_t rnd = v3_rand();
        if (drop || (int)(rnd & 1023) < (int)(noise * noise * 500)) { l = (rnd >> 10) & 15; h = (rnd >> 14) & 15; }
        else if (noise > 0.05f) l += (int)(((rnd >> 10) & 7) * noise) - (int)(3 * noise);
        dst[x] = V6C(v3_clamp(l, 0, 15), h);
      }
    }
    sv_line = target;
    if (sv_line >= SV_H) { sv_state = 2; sv_holdT = 0; }
  } else {
    sv_holdT += sdt;
    if (sv_holdT > 1.0f + sv_hold * 12.0f) sv_newPicture(pic);
  }

  // ── on screen: the picture at double size (zoomed about the middle), the new part bright, the old part dimmer ──
  float z = v6_zoom(1.0f);
  int32_t step = (int32_t)(32768 / z);                                    // picture px per screen px, ×65536 (½ at zoom 1)
  int32_t x0 = (int32_t)(SV_W / 2 * 65536) - step * HALFW, y0 = (int32_t)(SV_H / 2 * 65536) - step * HALFH;
  int scanY = sv_state == 1 ? (int)sv_line : -10;
  int16_t xs[W];
  for (int x = 0; x < W; x++) { int32_t sx = (x0 + step * x) >> 16; xs[x] = (int16_t)(sx < 0 ? -1 : (sx >= SV_W ? -1 : sx)); }
  for (int y = 0; y < H; y++) {
    int32_t sy = (y0 + step * y) >> 16;
    uint8_t* out = v3_buf + y * W;
    if (sy < 0 || sy >= SV_H) { memset(out, 0, W); continue; }
    const uint8_t* src = sv_pic + sy * SV_W;
    bool old = sv_state != 2 && sy > scanY;
    for (int x = 0; x < W; x++) {
      int sx = xs[x];
      uint8_t c = sx < 0 ? 0 : src[sx];
      if (old) { int l = (c >> 4) - 3; c = l > 0 ? (uint8_t)((l << 4) | (c & 15)) : 0; }
      out[x] = c;
    }
    if (sy == scanY) for (int x = 0; x < W; x++) out[x] = (uint8_t)((15 << 4) | (out[x] & 15));   // the scanning line
  }
  // ── the receiver's readout ──
  if (sv_state != 2 && ((v7_frameNo >> 4) & 1)) { v3_text(6, H - 12, "RCV", 1, V6C(15, 4)); v3_text(30, H - 12, sv_modes[L.mode], 1, V6C(12, 4)); }
  if (sv_state == 0) {                                                     // the header: a tone across the top
    int w = (int)(sv_holdT * 400) % W;
    v3_rect(0, 0, w, 2, V6C(15, 10));
  }
  v7_end(L.scan);
}

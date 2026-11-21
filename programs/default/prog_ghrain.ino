// =====================================================================
// PROGRAM: RAIN — slow digital rain (by Dewey) — Set 8 slot 7
//
// Greatest hit from Set 1 (DIGITAL RAIN) and Set 2 (ASCII WORLDS): streams
// of little glyphs sliding down the screen, the leading glyph bright, the
// tail fading out — slowed right down, softly coloured, in two layers for
// depth. Also: streams that spell out chill words, glyph snow that drifts
// and sways, and streams that rise like bubbles.
//
// How it works: the screen is a grid of character cells (6 × 8 px, or
// 12 × 16 for the near layer). Each column has a stream with a head
// position, speed and length; the cells behind the head light up with
// glyphs that change now and then. The glyphs are 64 made-up symbols,
// generated (mirrored random 5 × 7 bitmaps) — no real alphabet — or, in
// Words mode, letters from the font.
//
// Presets: k0 Digital Rain · k1 Lofi Words · k2 Glyph Snow · k3 Neon Rain ·
//          k4 Deep Rain · k5 Rising · k6 Ember Code · k7 Vapor Words ·
//          k8 Drizzle · k9 Starfall · k10 Green Screen · k11 Tour
//   k12 — CURTAIN: every stream starts again from the top, together
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (glyph size: small / mixed / big) ·
//   p3 Colour Flow · p4 Hue Spread · p5 Density · p6 Length · p7 Flicker ·
//   p8 Sway · p9 Glow · p10 Layers · p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8RA_STREAM, H8RA_WORDS, H8RA_SNOW, H8RA_UP };
struct H8RaLook { uint8_t mode, density, len, flick, sway, glow, layers, speed, pal, flow, spread; int8_t key; };
static const H8RaLook h8ra_looks[11] = {
  // mode         dens len flick sway glow lay spd pal          flow spr key
  { H8RA_STREAM,   60,  50,  40,   0,  60,  2, 50, V8P_FOREST,   1,  40,  0 },   // Digital Rain
  { H8RA_WORDS,    45,  60,  10,   0,  60,  2, 40, V8P_DUSK,     2,  70,  0 },   // Lofi Words
  { H8RA_SNOW,     50,   0,  25,  60,  55,  2, 35, V8P_CANDY,  3, 100,  0 },   // Glyph Snow
  { H8RA_STREAM,   55,  55,  40,   0,  65,  2, 50, V8P_LOUNGE,   3,  90,  0 },   // Neon Rain
  { H8RA_STREAM,   85,  45,  45,   0,  55,  2, 55, V8P_LAGOON,   2,  60,  0 },   // Deep Rain
  { H8RA_UP,       50,  40,  35,  30,  60,  2, 40, V8P_TOXIC,   3,  80,  0 },   // Rising
  { H8RA_STREAM,   55,  50,  40,   0,  65,  1, 45, V8P_EMBER,    2,  50,  0 },   // Ember Code
  { H8RA_WORDS,    50,  60,  10,   0,  60,  2, 45, V8P_VAPOR,    3,  90,  0 },   // Vapor Words
  { H8RA_STREAM,   30,  65,  30,   0,  55,  2, 30, V8P_RETROPOP,    2,  60,  0 },   // Drizzle
  { H8RA_SNOW,     70,   0,  20,  40,  60,  1, 30, V8P_VELVET,   2,  80,  0 },   // Starfall
  { H8RA_STREAM,   65,  50,  45,   0,  60,  1, 55, V8P_FOREST,   1,  30, 30 },   // Green Screen
};
static const char* const h8ra_names[12] = { "Digital Rain", "Lofi Words", "Glyph Snow", "Neon Rain", "Deep Rain", "Rising", "Ember Code", "Vapor Words", "Drizzle", "Starfall", "Green Screen", "Tour" };
static const char* const h8ra_lbl[6] = { "Density", "Length", "Flicker", "Sway", "Glow", "Layers" };
const char* prog_ghrain_name() { return "RAIN"; }
const char* prog_ghrain_character() { return "Slow digital rain: soft glyph streams, falling words and drifting glyph snow"; }
const char* prog_ghrain_presetName(int p) { return (p >= 0 && p < 12) ? h8ra_names[p] : NULL; }
const char* prog_ghrain_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8ra_lbl); }
uint8_t prog_ghrain_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

static const char* const h8ra_words[16] = { "CHILL", "VIBES", "GROOVY", "LOFI", "RELAX", "FLOW", "DREAM", "SLOW DOWN", "CALM", "FAR OUT", "PEACE", "GLOW", "DRIFT", "EASY", "BREATHE", "NO RUSH" };
#define H8RA_NG 64
#define H8RA_MAXC 54                                   // columns per layer (320 / 6)
// working memory (v6_mem): glyph bitmaps, then per layer and column the stream
#define H8RA_GLY (v6_mem)                               // 64 × 7 bytes
#define H8RA_HEAD ((int32_t*)(v6_mem + 512))            // [2][MAXC] head row, 1/256 cell
#define H8RA_SPD ((int16_t*)(v6_mem + 512 + 2 * H8RA_MAXC * 4))
#define H8RA_LEN (v6_mem + 512 + 2 * H8RA_MAXC * 6)
#define H8RA_SEED ((uint32_t*)(v6_mem + 1280))
static float h8ra_dens = 0.5f, h8ra_len = 0.5f, h8ra_flick = 0.4f, h8ra_sway = 0, h8ra_glow = 0.6f;
static float h8ra_t = 0;
static bool h8ra_ready = false;

static void h8ra_glyphs() {
  uint8_t* G = H8RA_GLY;
  for (int g = 0; g < H8RA_NG; g++) {
    uint32_t h = v3_hash(g * 2654435761u + 99);
    int bits = 0;
    for (int r = 0; r < 7; r++) {
      uint32_t hr = v3_hash(h + r * 977u);
      int left = (int)(hr & 7), mid = (int)((hr >> 3) & 1);           // columns 0-2, mirrored to 4-2
      int row = ((left & 1) << 4) | ((left & 2) << 2) | (mid << 2) | ((left & 2)) | ((left & 1));
      if (r == 0 || r == 6) row &= (hr & 32) ? 31 : 14;
      if ((hr >> 6) & 1) row = (g & 1) ? (row | 4) : row;              // some get a spine
      G[g * 7 + r] = (uint8_t)row; bits += row ? 1 : 0;
    }
    if (bits < 3) { G[g * 7 + 1] = 14; G[g * 7 + 3] = 17; G[g * 7 + 5] = 14; }
  }
}
static void h8ra_spawn(int L, int c, int rows, bool top) {
  uint32_t s = v3_rand();
  H8RA_SEED[L * H8RA_MAXC + c] = s;
  H8RA_SPD[L * H8RA_MAXC + c] = (int16_t)(480 + (s & 1023) + (L ? 0 : 200));   // 1/256 cells per second: 2 … 6 cells/s
  H8RA_LEN[L * H8RA_MAXC + c] = (uint8_t)(4 + ((s >> 8) % 18));
  H8RA_HEAD[L * H8RA_MAXC + c] = top ? -(int32_t)((s >> 16) % 6) * 256 : (int32_t)((s >> 12) % (rows + 20)) * 256;
}
void prog_ghrain_init() { v8_init(); h8ra_ready = false; }

// one glyph cell: top-left (x, y), scale sc, colour c; word mode draws a letter instead
static void h8ra_cell(int x, int y, int sc, uint8_t c, int gi, char letter) {
  const uint8_t* g = letter ? v8_font[v8_glyph(letter)] : H8RA_GLY + (gi & (H8RA_NG - 1)) * 7;
  if (sc == 1) {
    if (x < 0 || x > W - 5 || y < 0 || y > H - 7) return;
    uint8_t* p = v3_buf + y * W + x;
    for (int r = 0; r < 7; r++, p += W) { int b = g[r]; if (!b) continue; if (b & 16) p[0] = c; if (b & 8) p[1] = c; if (b & 4) p[2] = c; if (b & 2) p[3] = c; if (b & 1) p[4] = c; }
  } else for (int r = 0; r < 7; r++) { int b = g[r]; if (!b) continue; for (int k = 0; k < 5; k++) if (b & (16 >> k)) v3_rect(x + k * sc, y + r * sc, x + k * sc + sc - 1, y + r * sc + sc - 1, c); }
}

void prog_ghrain_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  if (!h8ra_ready) {
    h8ra_glyphs();
    for (int L = 0; L < 2; L++) for (int c = 0; c < H8RA_MAXC; c++) h8ra_spawn(L, c, L ? 30 : 15, false);   // spread down the screen at the start
    h8ra_ready = true;
  }
  const H8RaLook& Lk = h8ra_looks[look];
  float rate = v8_rate();
  v6_ease(&h8ra_dens, v3_centre(5) ? Lk.density / 100.0f : 0.1f + 0.9f * v3_u(5), rate);   // never empty
  v6_ease(&h8ra_len, v8_kn(6, Lk.len / 100.0f), rate);
  v6_ease(&h8ra_flick, v8_kn(7, Lk.flick / 100.0f), rate);
  v6_ease(&h8ra_sway, v8_kn(8, Lk.sway / 100.0f), rate);
  v6_ease(&h8ra_glow, v8_kn(9, Lk.glow / 100.0f), rate);
  int layers = v3_centre(10) ? Lk.layers : 1 + (v3_k(10) >= 512);
  v8_colours(Lk.pal, Lk.flow / 100.0f, Lk.spread / 100.0f, 160, Lk.key / 100.0f);
  // a short glow behind everything (soft, so slow glyphs don't strobe)
  if (v6_first || v6_f.fresh) memset(v3_buf, 0, W * H); else v8_decay(v8_fade < 0.5f ? 64 : 16 + (int)((1.0f - h8ra_glow) * 100));   // Glow right = long glowing smears
  h8ra_t += v6_f.sdt;
  if (h8ra_t > 100000.0f) h8ra_t -= 100000.0f;
  float z = v8_zoom(1.0f);
  int sizeMode = z < 0.8f ? 0 : (z > 1.25f ? 2 : 1);                 // small only / mixed / big only
  bool up = Lk.mode == H8RA_UP, snow = Lk.mode == H8RA_SNOW, words = Lk.mode == H8RA_WORDS;
  float spd = v6_f.sdt * (0.4f + Lk.speed / 60.0f);
  int lenMax = 2 + (int)(h8ra_len * 30);
  if (v6_k12hit) for (int L = 0; L < 2; L++) for (int c = 0; c < H8RA_MAXC; c++) H8RA_HEAD[L * H8RA_MAXC + c] = -(int32_t)(c % 3) * 128;
  for (int L = layers - 1; L >= 0; L--) {                             // far layer first
    int sc = sizeMode == 0 ? 1 : (sizeMode == 2 ? 2 : (L == 0 ? 2 : 1));
    if (layers == 1 && sizeMode == 1) sc = 1;
    int cw = 6 * sc, ch = 8 * sc, cols = W / cw, rows = H / ch;
    if (cols > H8RA_MAXC) cols = H8RA_MAXC;
    int far = (L == 1);
    for (int c = 0; c < cols; c++) {
      int id = L * H8RA_MAXC + c;
      uint32_t s = H8RA_SEED[id];
      if ((int)((s >> 24) * 100 >> 8) >= (int)(h8ra_dens * 100)) {      // this column sits out (density)
        H8RA_HEAD[id] += (int32_t)(H8RA_SPD[id] * spd); if (H8RA_HEAD[id] > (rows + 30) * 256) h8ra_spawn(L, c, rows, true);
        continue;
      }
      int len = snow ? 1 : v3_clamp(H8RA_LEN[id] * lenMax / 21, 2, 30);
      H8RA_HEAD[id] += (int32_t)(H8RA_SPD[id] * spd * (far ? 0.6f : 1.0f) * (snow ? 0.5f : 1.0f));
      int32_t head = H8RA_HEAD[id];
      if ((head >> 8) - len > rows + 1) { h8ra_spawn(L, c, rows, true); continue; }
      int hue = (int)((c * 3 + (s >> 4) % 5 + L * 8) & 15);
      const char* word = words ? h8ra_words[(s >> 9) & 15] : 0;
      int wl = word ? (int)strlen(word) + 1 : 1;
      int sway = snow || up ? (((int)(h8ra_sway * 30 * sc) * v3_sin((int)(h8ra_t * 80) + (int)(s & 1023))) >> 14) : 0;
      int hr = head >> 8, frac = head & 255;
      for (int k = 0; k < len; k++) {
        int r = hr - k;
        if (r < 0 || r >= rows + 1) continue;
        int b = 256 - (k * 256 + frac) / len;                         // 256 at the head … 0 at the tail
        if (b <= 0) continue;
        int lev = 2 + ((b * 11) >> 8) + (k == 0 ? 2 + (int)(h8ra_glow * 2) : 0);
        if (far) lev = lev * 2 / 3;
        lev = v3_clamp(lev, 1, 15);
        int y = up ? H - (r + 1) * ch + ((frac * ch) >> 8) : r * ch - ch + ((frac * ch) >> 8);
        if (snow) y = r * ch + ((frac * ch) >> 8) - ch;
        int gi = (int)(v3_hash(id * 131u + k * 977u + (s & 4095) + (uint32_t)((h8ra_t * (0.3f + h8ra_flick * 6.0f)) + k * 0.37f)) & 63);   // glyphs travel with the stream
        char letter = word ? word[(len - 1 - k) % wl] : 0;
        if (word && letter == 0) continue;                            // the gap between words
        if (letter == ' ') continue;
        h8ra_cell(c * cw + (cw - 5 * sc) / 2 + sway, y, sc, (uint8_t)((lev << 4) | hue), gi, letter);
      }
    }
  }
  v8_end();
}

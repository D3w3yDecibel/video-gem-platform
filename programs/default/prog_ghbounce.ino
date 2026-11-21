// =====================================================================
// PROGRAM: BOUNCE — the screensaver corner meme (by Dewey) — Set 8 slot 10
//
// Everyone has stared at a bouncing screensaver logo waiting for it to hit
// the corner EXACTLY. This program makes it happen (eventually): the logo
// glides about, changing colour at every wall, and when it lands perfectly
// in a corner — confetti, a big "CORNER!!!" and a running count.
// The logo is our own: a chill word over a flat disc that says VIDEO GEM.
//
// How it works: plain straight-line bouncing. To guarantee corner hits,
// the program "unfolds" the bounces (a reflected path is a straight line
// through mirrored copies of the screen) and picks a direction that ends in
// a corner of one of those copies 15-120 s away — so the corner hit is
// real geometry, not a cheat. "So Close" aims 6 px off, every time.
//
// Presets: k0 Classic · k1 Destiny · k2 So Close · k3 Crowd · k4 Echo ·
//          k5 Loading · k6 Error · k7 Big · k8 Tiny · k9 Snooze · k10 Party · k11 Tour
//   k12 — "COME ON…": steer the logo into a corner within a few seconds
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (logo size) · p3 Colour Flow · p4 Hue Spread
//   p5 Word · p6 Logos · p7 Glide · p8 Trails · p9 Corner Luck · p10 Confetti
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8BO_BOUNCE, H8BO_LOADING, H8BO_ERROR, H8BO_SNOOZE };
struct H8BoLook { uint8_t mode, n, word, scale, glide, trails, luck, aim, confetti, pal; };
// aim: 0 random, 1 destiny (corner), 2 so close (6 px off)
static const H8BoLook h8bo_looks[11] = {
  // mode           n word sc glide trail luck aim conf pal
  { H8BO_BOUNCE,    1, 0,  3,  50,   0,  35,  1,  60, V8P_VAPOR },     // Classic   (corner every ~1½-2 min)
  { H8BO_BOUNCE,    1, 1,  3,  55,   0,  90,  1,  70, V8P_LOUNGE },    // Destiny   (corner every ~20 s)
  { H8BO_BOUNCE,    1, 2,  3,  55,   0,  95,  2,  60, V8P_RETROPOP },    // So Close
  { H8BO_BOUNCE,    6, 9,  2,  45,   0,  30,  1,  60, V8P_SHERBET },   // Crowd
  { H8BO_BOUNCE,    1, 3,  3,  50,  70,  50,  1,  60, V8P_AURORA },    // Echo
  { H8BO_LOADING,   0, 0,  3,  50,   0,   0,  0,  50, V8P_ELECTRIC },      // Loading
  { H8BO_ERROR,     0, 0,  2,  50,   0,   0,  0,  50, V8P_LAGOON },    // Error
  { H8BO_BOUNCE,    1, 4,  6,  35,   0,  40,  1,  80, V8P_PEACH },     // Big
  { H8BO_BOUNCE,   16, 5,  1,  55,   0,  20,  0,  40, V8P_RAINBOW },     // Tiny
  { H8BO_SNOOZE,    1, 6,  3,  50,   0,  30,  1,  60, V8P_VELVET },    // Snooze
  { H8BO_BOUNCE,    8, 9,  2,  60,  30,  90,  1, 100, V8P_LOUNGE },    // Party
};
static const char* const h8bo_names[12] = { "Classic", "Destiny", "So Close", "Crowd", "Echo", "Loading", "Error", "Big", "Tiny", "Snooze", "Party", "Tour" };
static const char* const h8bo_lbl[6] = { "Word", "Logos", "Glide", "Trails", "Corner Luck", "Confetti" };
static const char* const h8bo_wordList[10] = { "VIBES", "CHILL", "GROOVY", "LOFI", "FAR OUT", "VIBE", "ZZZ", "CALM", "ZEN", "" };
static const char* const h8bo_crowd[8] = { "CHILL", "VIBES", "GROOVY", "LOFI", "ZEN", "CALM", "FLOW", "DREAM" };
const char* prog_ghbounce_name() { return "BOUNCE"; }
const char* prog_ghbounce_character() { return "The bouncing screensaver logo, and the glorious moment it hits the corner"; }
const char* prog_ghbounce_presetName(int p) { return (p >= 0 && p < 12) ? h8bo_names[p] : NULL; }
const char* prog_ghbounce_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8bo_lbl); }
uint8_t prog_ghbounce_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8BO_MAXL 16
#define H8BO_MAXP 90
// logos and confetti live in the shared memory
struct H8BoLogo { float x, y, vx, vy; int8_t hue, word, lastX, lastY; int16_t w, h; };
struct H8BoBit { float x, y, vx, vy, life; int8_t hue; };
#define H8BO_LOGO ((H8BoLogo*)v6_mem)
#define H8BO_BIT ((H8BoBit*)(v6_mem + 1024))
static int h8bo_nl = 0, h8bo_corners = 0, h8bo_frame = 0, h8bo_sc = 3, h8bo_hues[8] = { 0, 5, 10, 3, 8, 13, 6, 11 };
static float h8bo_celebrate = -1, h8bo_close = -1, h8bo_t = 0, h8bo_luck = 0.5f, h8bo_glide = 0.5f, h8bo_trails = 0, h8bo_conf = 0.6f, h8bo_snooze = 0;
static int h8bo_cx = 0, h8bo_cy = 0, h8bo_look = -1;
void prog_ghbounce_init() { v8_init(); h8bo_nl = 0; h8bo_corners = 0; h8bo_celebrate = -1; h8bo_close = -1; h8bo_look = -1; memset(v6_mem, 0, 1024 + H8BO_MAXP * sizeof(H8BoBit)); }

static int h8bo_logoW(const char* w, int sc) { int tw = v8_textW(w, sc); return tw < 9 * 6 ? 9 * 6 : tw; }
// aim logo i at a corner of a mirrored copy of the screen, tmin … tmax seconds away (off = px short of it)
static void h8bo_aim(int i, float speed, float tmin, float tmax, float off) {
  H8BoLogo& g = H8BO_LOGO[i];
  float sx = (float)(W - g.w), sy = (float)(H - g.h);
  float best = 1e9f, bdx = 0, bdy = 0, target = (tmin + tmax) * 0.5f;
  float lo2 = tmin * speed * tmin * speed, hi2 = tmax * speed * tmax * speed;
  for (int k = -20; k <= 20; k++) for (int l = -20; l <= 20; l++) {   // corners of mirrored copies of the screen
    float dx = k * sx - g.x + (k & 1 ? -off : off), dy = l * sy - g.y + (l & 1 ? -off * 0.0f : 0.0f);
    float d2 = dx * dx + dy * dy;
    if (d2 < lo2 || d2 > hi2) continue;                                // out of the time window (no square root needed)
    float t = sqrtf(d2) / speed;
    float ax = fabsf(dx), ay = fabsf(dy);
    if (ax < ay * 0.5f || ay < ax * 0.3f) continue;                   // not too steep, not too flat
    float score = fabsf(t - target) + v3_rndf() * 8.0f;
    if (score < best) { best = score; bdx = dx; bdy = dy; }
  }
  if (best > 1e8f) { float a = 0.6f + v3_rndf() * 0.5f; bdx = cosf(a) * (v3_rnd(2) ? 1 : -1); bdy = sinf(a) * (v3_rnd(2) ? 1 : -1); }
  float d = sqrtf(bdx * bdx + bdy * bdy); if (d < 1e-3f) d = 1;
  g.vx = bdx / d * speed; g.vy = bdy / d * speed;
}
static void h8bo_burst(int x, int y, int n) {
  for (int k = 0; k < H8BO_MAXP && n > 0; k++) {
    H8BoBit& b = H8BO_BIT[k];
    if (b.life > 0) continue;
    float a = v3_rndf() * 6.2832f, s = 40 + v3_rndf() * 160;
    b.x = (float)x; b.y = (float)y; b.vx = cosf(a) * s; b.vy = sinf(a) * s - 60; b.life = 1.5f + v3_rndf() * 2.0f; b.hue = (int8_t)v3_rnd(16);
    n--;
  }
}
static void h8bo_drawLogo(int i, uint8_t lev) {
  H8BoLogo& g = H8BO_LOGO[i];
  const char* w = g.word >= 0 && g.word < 10 ? h8bo_wordList[(int)g.word] : "VIBES";
  if (g.word >= 10) w = h8bo_crowd[(g.word - 10) & 7];
  int x = (int)g.x, y = (int)g.y, sc = h8bo_sc, tw = v8_textW(w, sc);
  uint8_t c = (uint8_t)((lev << 4) | (g.hue & 15));
  v8_text(x + (g.w - tw) / 2, y, w, sc, c);
  int dy = y + 7 * sc + sc + 2, rx = g.w / 2, ry = 3 + sc * 2;       // the flat disc underneath
  v8_ellipse(x + g.w / 2, dy + ry, rx, ry, (uint8_t)(((lev > 2 ? lev - 2 : lev) << 4) | (g.hue & 15)));
  if (sc >= 2) v8_text(x + g.w / 2 - v8_textW("VIDEO GEM", 1) / 2, dy + ry - 3, "VIDEO GEM", 1, (uint8_t)(g.hue & 15));
}
static void h8bo_setup(int look, int nl) {                          // (an index, not the struct: Arduino's prototypes)
  const H8BoLook& L = h8bo_looks[look];
  h8bo_nl = nl; h8bo_frame = 0;
  for (int i = 0; i < nl; i++) {
    H8BoLogo& g = H8BO_LOGO[i];
    int wsel = v3_centre(5) ? L.word : v3_k(5) * 9 / 1024;
    g.word = (int8_t)(L.word == 9 ? 10 + (i % 8) : wsel);
    const char* w = g.word >= 10 ? h8bo_crowd[(g.word - 10) & 7] : h8bo_wordList[(int)g.word];
    g.w = (int16_t)h8bo_logoW(w, h8bo_sc); g.h = (int16_t)(7 * h8bo_sc + h8bo_sc + 2 + 2 * (3 + h8bo_sc * 2) + 1);
    if (g.w > W - 4) g.w = W - 4;
    g.x = (float)v3_rnd(v6_max(1, W - g.w)); g.y = (float)v3_rnd(v6_max(1, H - g.h));
    g.hue = (int8_t)h8bo_hues[i & 7]; g.lastX = g.lastY = 100;
    float a = 0.5f + v3_rndf() * 0.6f; g.vx = cosf(a) * (v3_rnd(2) ? 1 : -1); g.vy = sinf(a) * (v3_rnd(2) ? 1 : -1);
  }
}

void prog_ghbounce_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8BoLook& L = h8bo_looks[look];
  float rate = v8_rate();
  v6_ease(&h8bo_glide, v8_kn(7, L.glide / 100.0f), rate);
  v6_ease(&h8bo_trails, v8_kn(8, L.trails / 100.0f), rate);
  v6_ease(&h8bo_luck, v8_kn(9, L.luck / 100.0f), rate);
  v6_ease(&h8bo_conf, v8_kn(10, L.confetti / 100.0f), rate);
  v8_colours(L.pal, 0.02f, 1.0f, 120, 0);
  int nl = v3_centre(6) ? L.n : 1 + v3_k(6) * H8BO_MAXL / 1024;
  if (L.mode != H8BO_BOUNCE && L.mode != H8BO_SNOOZE) nl = 0;
  int sc = v3_clamp((int)(L.scale * v8_zoom(1.0f) + 0.5f), 1, 7);
  if (v6_f.fresh || look != h8bo_look || nl != h8bo_nl || sc != h8bo_sc) { h8bo_sc = sc; h8bo_look = look; h8bo_setup(look, nl); h8bo_celebrate = -1; h8bo_t = 0; h8bo_snooze = 0; }
  float dt = v6_f.sdt;
  h8bo_t += dt; h8bo_frame++;
  // the background: black, or fading trails
  if (h8bo_trails > 0.03f && !v6_f.fresh) v8_decay(2 + (int)((1.0f - h8bo_trails) * 60)); else memset(v3_buf, 0, W * H);
  float speed = 15.0f + h8bo_glide * h8bo_glide * 240.0f;
  float tmin = 4 + (1.0f - h8bo_luck) * 70, tmax = tmin + 8 + (1.0f - h8bo_luck) * 30;   // seconds of (chill) travel to the next corner
  if (L.mode == H8BO_SNOOZE) {                                        // slows to a nap, Zzz, wakes up
    float cyc = fmodf(h8bo_t, 30.0f);
    h8bo_snooze = cyc < 14 ? 1.0f - cyc / 14 : (cyc < 22 ? 0.0f : (cyc - 22) / 8);
    speed *= 0.05f + 0.95f * h8bo_snooze;
  }

  // ── logos ──
  for (int i = 0; i < h8bo_nl; i++) {
    H8BoLogo& g = H8BO_LOGO[i];
    if (h8bo_frame == 1 || v6_k12hit) {
      int aim = L.aim;
      if (aim == 0) { float a = atan2f(g.vy, g.vx); g.vx = cosf(a) * speed; g.vy = sinf(a) * speed; }
      else if (v6_k12hit) h8bo_aim(i, speed, 3, 7, aim == 2 ? 6 : 0);
      else h8bo_aim(i, speed, tmin, tmax, aim == 2 ? 6 : 0);
    }
    float v = sqrtf(g.vx * g.vx + g.vy * g.vy);
    if (v > 0.01f && fabsf(v - speed) > 0.5f) { float k = speed / v; g.vx *= k; g.vy *= k; }   // follow the Glide knob
    g.x += g.vx * dt; g.y += g.vy * dt;
    bool hx = false, hy = false;
    float mx = (float)(W - g.w), my = (float)(H - g.h);
    if (g.x <= 0) { g.x = -g.x; g.vx = fabsf(g.vx); hx = true; }
    if (g.x >= mx) { g.x = 2 * mx - g.x; g.vx = -fabsf(g.vx); hx = true; }
    if (g.y <= 0) { g.y = -g.y; g.vy = fabsf(g.vy); hy = true; }
    if (g.y >= my) { g.y = 2 * my - g.y; g.vy = -fabsf(g.vy); hy = true; }
    if (hx) g.lastX = 0; else if (g.lastX < 100) g.lastX++;
    if (hy) g.lastY = 0; else if (g.lastY < 100) g.lastY++;
    if (hx || hy) {
      g.hue = (int8_t)((g.hue + 5) & 15);
      bool corner = g.lastX <= 3 && g.lastY <= 3;                      // both walls within a few frames: CORNER
      bool near = !corner && ((hx && (g.y < 8 || g.y > my - 8)) || (hy && (g.x < 8 || g.x > mx - 8)));
      if (corner) {
        h8bo_corners++; h8bo_celebrate = 0; g.lastX = g.lastY = 100;
        h8bo_cx = (int)(g.x + g.w / 2); h8bo_cy = (int)(g.y + g.h / 2);
        h8bo_burst(g.x < W / 2 ? 0 : W - 1, g.y < H / 2 ? 0 : H - 1, 20 + (int)(h8bo_conf * 70));
        if (L.aim) h8bo_aim(i, speed, tmin, tmax, L.aim == 2 ? 6 : 0);
      } else if (near && (L.aim == 2 || h8bo_nl <= 2)) { h8bo_close = 0; if (L.aim) h8bo_aim(i, speed, tmin, tmax, L.aim == 2 ? 6 : 0); }
    }
    int lev = 14;
    if (L.mode == H8BO_SNOOZE) lev = 8 + (int)(6 * h8bo_snooze);
    if (h8bo_celebrate >= 0) g.hue = (int8_t)((g.hue + (h8bo_frame & 3 ? 0 : 1)) & 15);
    h8bo_drawLogo(i, (uint8_t)lev);
  }
  if (L.mode == H8BO_SNOOZE && h8bo_nl) {                             // Zzz while it naps
    H8BoLogo& g = H8BO_LOGO[0];
    if (h8bo_snooze < 0.3f) for (int z = 0; z < 3; z++) {
      float ph = fmodf(h8bo_t * 0.5f + z * 0.33f, 1.0f);
      int zx = (int)(g.x + g.w) + (int)(ph * 30) + (int)(6 * sinf(h8bo_t * 2 + z)), zy = (int)g.y - (int)(ph * 50);
      v8_text(zx, zy, "Z", 1 + z % 2, (uint8_t)((v3_clamp((int)(14 * (1 - ph)), 2, 14) << 4) | 9));
    }
    float cyc = fmodf(h8bo_t, 30.0f);
    if (cyc > 22 && cyc < 24) v8_textC((int)(g.x + g.w / 2), (int)g.y - 22, "!", 3, (uint8_t)((15 << 4) | 3));
  }

  // ── Loading: the bar that never quite gets there ──
  if (L.mode == H8BO_LOADING) {
    float cyc = fmodf(h8bo_t, 34.0f), pct;
    if (cyc < 14) pct = 90.0f * (1 - (1 - cyc / 14) * (1 - cyc / 14));
    else if (cyc < 20) pct = 90 + 9 * (cyc - 14) / 6;
    else pct = 99;
    int bx = 40, by = H / 2, bw = W - 80, bh = 16;
    v8_textC(W / 2, by - 30, cyc < 27 ? "LOADING VIBES" : (cyc < 30 ? "JUST KIDDING" : "VIBES LOADED @"), 2, (uint8_t)((14 << 4) | 2));
    v3_rect(bx - 2, by - 2, bx + bw + 1, by + bh + 1, (uint8_t)((6 << 4) | 4));
    v3_rect(bx, by, bx + bw - 1, by + bh - 1, 0);
    int fill = (int)(bw * (cyc >= 27 ? 1.0f : pct / 100.0f));
    for (int x = 0; x < fill; x++) v3_rect(bx + x, by + 2, bx + x, by + bh - 3, (uint8_t)((12 << 4) | ((x / 12 + (int)(h8bo_t * 4)) & 15)));
    char s[16]; int p = cyc >= 27 ? 100 : (int)pct; snprintf(s, sizeof s, "%d%%", p);
    v8_textC(W / 2, by + bh + 10, s, 2, (uint8_t)((15 << 4) | 5));
    if (cyc >= 20 && cyc < 27) v8_textC(W / 2, by + bh + 34, (int)(h8bo_t * 2) & 1 ? "ANY SECOND NOW" : "ANY SECOND NOW.", 1, (uint8_t)((9 << 4) | 7));
    if (cyc >= 30 && cyc < 30.1f && h8bo_conf > 0.05f) h8bo_burst(W / 2, by, 30 + (int)(h8bo_conf * 40));
  }
  // ── Error: a cascade of very relaxed error boxes ──
  if (L.mode == H8BO_ERROR) {
    static const char* const msg[4] = { "ERROR: TOO CHILL", "WARNING: VIBES HIGH", "NOTHING IS WRONG", "TAKE A BREAK?" };
    int nb = (int)fmodf(h8bo_t / 1.6f, 16.0f);
    for (int b = 0; b <= nb && b < 12; b++) {
      int x = 20 + b * 14, y = 14 + b * 12, w = 170, hh = 64;
      v3_rect(x + 3, y + 3, x + w + 3, y + hh + 3, 0);
      v3_rect(x, y, x + w, y + hh, (uint8_t)((11 << 4) | 5));
      v3_rect(x, y, x + w, y + 11, (uint8_t)((6 << 4) | 2));
      v8_text(x + 3, y + 2, "VIBE.EXE", 1, (uint8_t)((15 << 4) | 5));
      v3_rect(x + w - 10, y + 2, x + w - 3, y + 9, (uint8_t)((12 << 4) | 3));
      v8_text(x + 8, y + 20, msg[b & 3], 1, (uint8_t)((1 << 4) | 4));
      v3_rect(x + 20, y + 42, x + 64, y + 56, (uint8_t)((14 << 4) | 5)); v8_text(x + 36, y + 46, "OK", 1, (uint8_t)((1 << 4) | 4));
      v3_rect(x + 90, y + 42, x + 160, y + 56, (uint8_t)((14 << 4) | 5)); v8_text(x + 98, y + 46, "ALSO OK", 1, (uint8_t)((1 << 4) | 4));
    }
  }

  // ── confetti and the corner celebration ──
  for (int k = 0; k < H8BO_MAXP; k++) {
    H8BoBit& b = H8BO_BIT[k];
    if (b.life <= 0) continue;
    b.life -= v6_f.dt; b.vy += 120 * v6_f.dt; b.vx *= 1.0f - v6_f.dt * 0.6f;
    b.x += b.vx * v6_f.dt; b.y += b.vy * v6_f.dt;
    int x = (int)b.x, y = (int)b.y;
    uint8_t c = (uint8_t)((v3_clamp((int)(b.life * 10), 3, 15) << 4) | (b.hue & 15));
    int sz = ((k + h8bo_frame / 6) & 1) ? 2 : 1;                     // they flutter
    v3_rect(x, y, x + sz, y + 3 - sz, c);
  }
  if (h8bo_celebrate >= 0) {
    h8bo_celebrate += v6_f.dt;
    if (h8bo_celebrate > 4.5f) h8bo_celebrate = -1;
    else {
      int bob = (int)(4 * sinf(h8bo_celebrate * 8));
      int lev = h8bo_celebrate < 0.3f ? (int)(h8bo_celebrate * 50) : 15;
      v8_textC(W / 2, H / 2 - 30 + bob, "CORNER!!!", 4, (uint8_t)((v3_clamp(lev, 2, 15) << 4) | ((h8bo_frame / 8) & 15)));
      char s[24]; snprintf(s, sizeof s, "CORNERS: %d", h8bo_corners);
      v8_textC(W / 2, H / 2 + 6, s, 2, (uint8_t)((14 << 4) | 5));
      if (h8bo_celebrate > 0.5f && h8bo_celebrate < 0.55f) h8bo_burst(W / 2, H / 2 - 20, 20 + (int)(h8bo_conf * 30));
    }
  }
  if (h8bo_close >= 0) {
    h8bo_close += v6_f.dt;
    if (h8bo_close > 2.5f) h8bo_close = -1;
    else v8_textC(W / 2, H - 40, h8bo_close < 1.2f ? "SO CLOSE!" : "SO CLOSE. :(", 2, (uint8_t)((13 << 4) | 3));
  }
  v8_end();
}

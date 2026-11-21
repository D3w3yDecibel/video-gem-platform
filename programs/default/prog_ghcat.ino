// =====================================================================
// PROGRAM: VIBE CAT — a cat that vibes (by Dewey) — Set 8 slot 11
//
// The internet's favourite activity: a cat nodding along to the music.
// Our own cartoon cat (drawn from ellipses and triangles, no borrowed
// characters) bops its head on the beat, closes its eyes in bliss, swishes
// its tail, and music notes float away. Sunglasses descend for the vibe
// check; a choir of cats does the wave; a loaf floats through space; DJ
// Whiskers scratches; a tower of cats wobbles in time.
//
// The beat: p5 Tempo sets the BPM (centre = 92). Or TAP k12 a few times in
// time with the music you're mixing to — the cats lock to your tempo and
// to where you tapped (the downbeat).
//
// Presets: k0 Vibe · k1 Shades · k2 Choir · k3 Loaf · k4 DJ · k5 Disco ·
//          k6 Stack · k7 Zoomies · k8 Nap · k9 Rave · k10 Duet · k11 Tour
//   k12 — TAP TEMPO
//
// Knobs: p0 Fur (centre = preset's own: orange, grey, black, white, cream, tabby)
//   p1 Speed (beat follows Tempo; Speed scales everything else) · p2 Zoom (cat size)
//   p3 Colour Flow · p4 Background colours · p5 Tempo · p6 Cats · p7 Bop
//   p8 Notes · p9 Captions · p10 Glow · p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8CA_VIBE, H8CA_SHADES, H8CA_CHOIR, H8CA_LOAF, H8CA_DJ, H8CA_DISCO, H8CA_STACK, H8CA_ZOOM, H8CA_NAP, H8CA_RAVE, H8CA_DUET };
struct H8CaLook { uint8_t fur, n, bop, notes, glow, pal; };
static const H8CaLook h8ca_looks[11] = {
  // fur n bop notes glow pal
  { 0, 1, 60, 50, 50, V8P_DUSK },      // Vibe
  { 1, 1, 60, 40, 50, V8P_VAPOR },     // Shades
  { 0, 5, 55, 60, 45, V8P_MIAMI },    // Choir
  { 4, 1, 30, 20, 40, V8P_LOUNGE },    // Loaf
  { 2, 1, 65, 40, 55, V8P_LOUNGE },    // DJ
  { 3, 1, 60, 40, 60, V8P_GALAXY },    // Disco
  { 5, 4, 55, 30, 45, V8P_SHERBET },   // Stack
  { 0, 1, 60, 20, 45, V8P_PEACH },     // Zoomies
  { 1, 1, 10, 10, 30, V8P_LAGOON },    // Nap
  { 0, 9, 60, 20, 60, V8P_NEON },    // Rave
  { 4, 2, 55, 30, 50, V8P_PEACH },     // Duet
};
static const char* const h8ca_names[12] = { "Vibe", "Shades", "Choir", "Loaf", "DJ", "Disco", "Stack", "Zoomies", "Nap", "Rave", "Duet", "Tour" };
static const char* const h8ca_lbl[6] = { "Tempo", "Cats", "Bop", "Notes", "Captions", "Glow" };
static const char* const h8ca_caps[11] = { "NO THOUGHTS. JUST VIBES.", "VIBE CHECK: PASSED", "THE CHOIR", "LOAF MODE", "DJ WHISKERS", "SATURDAY NIGHT PURR", "TOTEM OF VIBES", "ZOOMIES!", "DO NOT DISTURB", "CAT RAVE", "PURRFECT HARMONY" };
const char* prog_ghcat_name() { return "VIBE CAT"; }
const char* prog_ghcat_character() { return "A cartoon cat bopping its head to the beat (tap the tempo on k12)"; }
const char* prog_ghcat_presetName(int p) { return (p >= 0 && p < 12) ? h8ca_names[p] : NULL; }
const char* prog_ghcat_potLabel(int p, int pot) { (void)p; if (pot == 0) return "Fur"; if (pot == 4) return "Background"; return v8_label(pot, h8ca_lbl); }
uint8_t prog_ghcat_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── colours: hues 0-7 the cat and props, 8-15 the background (from the chill palettes) ───
enum { H8K_FUR = 0, H8K_DARK = 1, H8K_BELLY = 2, H8K_PINK = 3, H8K_LINE = 4, H8K_WHITE = 5, H8K_GOLD = 6, H8K_SHADES = 7 };
#define H8C(l, h) ((uint8_t)(((l) << 4) | (h)))
static const uint8_t h8ca_furs[6][3][3] = {
  { {250, 150,  60}, {190,  90,  30}, {255, 228, 190} },   // orange
  { {150, 158, 175}, { 90,  96, 115}, {228, 230, 238} },   // grey
  { { 62,  58,  76}, { 32,  30,  44}, {205, 200, 215} },   // black (with a pale chest)
  { {242, 242, 248}, {196, 200, 216}, {255, 255, 255} },   // white
  { {240, 215, 175}, {120,  86,  60}, {255, 242, 220} },   // cream / pointed
  { {170, 120,  70}, {105,  68,  40}, {232, 205, 165} },   // brown tabby
};
static void h8ca_palette(int fur, int pal, float key) {
  const uint8_t (*f)[3] = h8ca_furs[fur];
  v3_setLH(H8K_FUR, f[0][0], f[0][1], f[0][2], 1.1f, 0);
  v3_setLH(H8K_DARK, f[1][0], f[1][1], f[1][2], 1.1f, 0);
  v3_setLH(H8K_BELLY, f[2][0], f[2][1], f[2][2], 1.1f, 0);
  v3_setLH(H8K_PINK, 255, 150, 175, 1.1f, 0);
  v3_setLH(H8K_LINE, 28, 16, 34, 1.1f, 0);
  v3_setLH(H8K_WHITE, 250, 250, 255, 1.1f, 0);
  v3_setLH(H8K_GOLD, 255, 205, 90, 1.1f, 0);
  v3_setLH(H8K_SHADES, 26, 26, 44, 1.1f, 0);
  float dim = key > 0 ? 1.0f - key : 1.0f;                          // Key right: the background goes to black
  int lift = key < 0 ? (int)(-key * 40) : 0;
  for (int h = 8; h < 16; h++) {
    int c[3]; int tri = (h - 8) <= 4 ? h - 8 : 8 - (h - 8);
    v8_grad(pal, (int)(v6_flow * 1536) + tri * 160 * (16 + v6_f.spread / 16) / 32, c);
    for (int l = 0; l < 16; l++) {
      int k = (int)((l * 17) * dim) + lift;
      display.setColor((uint8_t)((l << 4) | h), (uint8_t)v3_clamp(c[0] * k >> 8, 0, 255), (uint8_t)v3_clamp(c[1] * k >> 8, 0, 255), (uint8_t)v3_clamp(c[2] * k >> 8, 0, 255));
    }
  }
}

// per-frame body language, set by the draw from the beat (v3: more dynamic)
static int h8ca_pawUp[2] = { 0, 0 }, h8ca_earW = 0, h8ca_glance = 0; static float h8ca_squash = 0;

// ─── the cat ───
// (cx, cy) = centre of the head at rest, s = head radius; nod 0 … 256 (head dips forward),
// lean −256 … 256 (head tilts side to side), eyes 0 open / 1 blissed-out ^ ^ / 2 asleep,
// shadeY: sunglasses' height above the eyes (px, 0 = on; ≥ 999 = none), tail phase 0 … 1023, body: 0 sitting / 1 loaf
static void h8ca_cat(int cx, int cy, int s, int nod, int lean, int eyes, int shadeY, int tail, int body) {
  if (s < 4) return;
  int bx = cx, by = cy + s * 5 / 4;
  // tail (behind the body): a chain of discs curling up from the right
  if (!body) {
    int px = bx + s * 7 / 10, py = by + s * 7 / 10;
    for (int k = 1; k <= 12; k++) {
      int sw = (v3_sin(tail + k * 40) * s) >> 15;
      int nx = bx + s * 7 / 10 + k * s / 10 + sw * k / 12, ny = by + s * 7 / 10 - k * k * s / 90;
      v8_thick(px, py, nx, ny, s / 7 + 1, H8C(3, H8K_LINE));
      px = nx; py = ny;
    }
    px = bx + s * 7 / 10; py = by + s * 7 / 10;
    for (int k = 1; k <= 12; k++) {
      int sw = (v3_sin(tail + k * 40) * s) >> 15;
      int nx = bx + s * 7 / 10 + k * s / 10 + sw * k / 12, ny = by + s * 7 / 10 - k * k * s / 90;
      v8_thick(px, py, nx, ny, s / 9 + 1, H8C(12, k > 9 ? H8K_DARK : H8K_FUR));
      px = nx; py = ny;
    }
  }
  // body
  if (body) {                                                         // a loaf: wide and flat, paws tucked
    bx = cx + s * 3 / 5;                                              // the head sits at the front of the loaf
    for (int k = 1; k <= 6; k++) v8_thick(bx + s * 7 / 5 + k * s / 12, by + s / 3 - k * s / 20, bx + s * 7 / 5 + (k + 1) * s / 12, by + s / 3 - (k + 1) * s / 20, s / 9 + 1, H8C(11, k > 4 ? H8K_DARK : H8K_FUR));   // a tucked tail
    v8_ellipse(bx, by + s / 6, s * 3 / 2 + 2, s * 3 / 4 + 2, H8C(3, H8K_LINE));
    v8_ellipse(bx, by + s / 6, s * 3 / 2, s * 3 / 4, H8C(12, H8K_FUR));
    for (int k = 0; k <= 2; k++) v8_thick(bx + s / 4 + k * s / 3, by - s / 2, bx + s / 3 + k * s / 3, by - s / 4, s / 14 + 1, H8C(10, H8K_DARK));   // tabby stripes on the back
  } else {
    int brx = (int)(s * 19 / 20 * (1.0f + 0.10f * h8ca_squash)), bry = (int)(s * 17 / 20 * (1.0f - 0.14f * h8ca_squash));   // squashes on the beat
    by += (s * 17 / 20) - bry;
    v8_ellipse(bx, by, brx + 2, bry + 2, H8C(3, H8K_LINE));
    v8_ellipse(bx, by, brx, bry, H8C(12, H8K_FUR));
    v8_ellipse(bx, by + s / 8, s * 11 / 20, s * 12 / 20, H8C(13, H8K_BELLY));
    int pyL = by + s * 4 / 5 - h8ca_pawUp[0], pyR = by + s * 4 / 5 - h8ca_pawUp[1];   // paws tap along
    v8_ellipse(bx - s * 7 / 20, pyL, s / 4 + 1, s / 7 + 1, H8C(3, H8K_LINE));
    v8_ellipse(bx + s * 7 / 20, pyR, s / 4 + 1, s / 7 + 1, H8C(3, H8K_LINE));
    v8_ellipse(bx - s * 7 / 20, pyL, s / 4, s / 7, H8C(14, H8K_BELLY));
    v8_ellipse(bx + s * 7 / 20, pyR, s / 4, s / 7, H8C(14, H8K_BELLY));
  }
  // head
  int hx = cx + ((lean * s) >> 10), hy = cy + ((nod * s) >> 10);
  int el = ((lean * s) >> 11) + h8ca_earW;                            // ears follow the tilt (and flick)
  int ex1 = hx - s * 3 / 4, ex2 = hx + s * 3 / 4;
  v8_tri(ex1 - 2, hy - s / 3, hx - s * 11 / 20 + el, hy - s * 21 / 20 - 3, hx - s / 6, hy - s * 3 / 5 - 2, H8C(3, H8K_LINE));
  v8_tri(ex2 + 2, hy - s / 3, hx + s * 11 / 20 + el, hy - s * 21 / 20 - 3, hx + s / 6, hy - s * 3 / 5 - 2, H8C(3, H8K_LINE));
  v8_tri(ex1, hy - s / 3, hx - s * 11 / 20 + el, hy - s * 21 / 20, hx - s / 6, hy - s * 3 / 5, H8C(12, H8K_FUR));
  v8_tri(ex2, hy - s / 3, hx + s * 11 / 20 + el, hy - s * 21 / 20, hx + s / 6, hy - s * 3 / 5, H8C(12, H8K_FUR));
  v8_tri(ex1 + s / 8, hy - s / 3, hx - s * 11 / 20 + el, hy - s * 17 / 20, hx - s / 4, hy - s / 2, H8C(12, H8K_PINK));
  v8_tri(ex2 - s / 8, hy - s / 3, hx + s * 11 / 20 + el, hy - s * 17 / 20, hx + s / 4, hy - s / 2, H8C(12, H8K_PINK));
  v8_ellipse(hx, hy, s * 17 / 20 + 2, s * 18 / 25 + 2, H8C(3, H8K_LINE));
  v8_ellipse(hx, hy, s * 17 / 20, s * 18 / 25, H8C(13, H8K_FUR));
  v8_ellipse(hx, hy + s / 4, s / 2, s / 3, H8C(13, H8K_BELLY));                // muzzle
  for (int k = -1; k <= 1; k++) v8_thick(hx + k * s / 5, hy - s * 2 / 3 + 2, hx + k * s / 6, hy - s * 2 / 5, s / 14 + 1, H8C(10, H8K_DARK));   // forehead stripes
  // eyes
  int eyY = hy - s / 12, eL = hx - s / 3, eR = hx + s / 3, ew = s / 7 + 1;
  if (eyes == 0) {
    v8_ellipse(eL, eyY, ew, ew + s / 20, H8C(2, H8K_LINE)); v8_ellipse(eR, eyY, ew, ew + s / 20, H8C(2, H8K_LINE));
    int gl = (h8ca_glance * ew) / 2;                                  // looking around
    v3_disc(eL + ew / 3 + gl, eyY - ew / 3, s / 20 + 1, H8C(15, H8K_WHITE)); v3_disc(eR + ew / 3 + gl, eyY - ew / 3, s / 20 + 1, H8C(15, H8K_WHITE));
  } else if (eyes == 1) {                                             // blissed out: ^ ^
    int r = s / 22 + 1;
    v8_thick(eL - ew, eyY + ew / 2, eL, eyY - ew / 2, r, H8C(2, H8K_LINE)); v8_thick(eL, eyY - ew / 2, eL + ew, eyY + ew / 2, r, H8C(2, H8K_LINE));
    v8_thick(eR - ew, eyY + ew / 2, eR, eyY - ew / 2, r, H8C(2, H8K_LINE)); v8_thick(eR, eyY - ew / 2, eR + ew, eyY + ew / 2, r, H8C(2, H8K_LINE));
  } else {                                                            // asleep: — —
    int r = s / 24 + 1;
    v8_thick(eL - ew, eyY, eL + ew, eyY + 1, r, H8C(2, H8K_LINE)); v8_thick(eR - ew, eyY, eR + ew, eyY + 1, r, H8C(2, H8K_LINE));
  }
  // nose, mouth, whiskers, cheeks
  int ny = hy + s / 6;
  v8_tri(hx - s / 10, ny - s / 20, hx + s / 10, ny - s / 20, hx, ny + s / 14, H8C(13, H8K_PINK));
  int r = s / 30 + 1;
  v8_thick(hx, ny + s / 14, hx - s / 8, ny + s / 6, r, H8C(3, H8K_LINE)); v8_thick(hx, ny + s / 14, hx + s / 8, ny + s / 6, r, H8C(3, H8K_LINE));
  v8_thick(hx - s / 8, ny + s / 6, hx - s / 5, ny + s / 9, r, H8C(3, H8K_LINE)); v8_thick(hx + s / 8, ny + s / 6, hx + s / 5, ny + s / 9, r, H8C(3, H8K_LINE));
  for (int k = -1; k <= 1; k++) {
    v3_line(hx - s * 2 / 5, ny + k * s / 14, hx - s * 6 / 5, ny + k * s / 6 - s / 20, H8C(11, H8K_WHITE));
    v3_line(hx + s * 2 / 5, ny + k * s / 14, hx + s * 6 / 5, ny + k * s / 6 - s / 20, H8C(11, H8K_WHITE));
  }
  if (eyes) { v8_ellipse(eL - s / 12, ny, s / 9, s / 16 + 1, H8C(8, H8K_PINK)); v8_ellipse(eR + s / 12, ny, s / 9, s / 16 + 1, H8C(8, H8K_PINK)); }
  // sunglasses
  if (shadeY < 999) {
    int gy = eyY - shadeY, gw = s * 9 / 25, gh = s / 4;
    v3_rect(hx - s * 3 / 4, gy - gh / 2 - 1, hx + s * 3 / 4, gy - gh / 2 + s / 14, H8C(3, H8K_SHADES));
    v3_rect(eL - gw, gy - gh / 2, eL + gw, gy + gh / 2, H8C(2, H8K_SHADES));
    v3_rect(eR - gw, gy - gh / 2, eR + gw, gy + gh / 2, H8C(2, H8K_SHADES));
    v8_thick(eL - gw + 3, gy + gh / 2 - 3, eL - gw + gh / 2 + 3, gy - gh / 2 + 2, 1, H8C(12, H8K_WHITE));   // the shine
    v8_thick(eR - gw + 3, gy + gh / 2 - 3, eR - gw + gh / 2 + 3, gy - gh / 2 + 2, 1, H8C(12, H8K_WHITE));
  }
}

// ─── notes, hearts and Zs that float up ───
#define H8CA_NN 16
static float h8ca_nx[H8CA_NN], h8ca_ny[H8CA_NN], h8ca_nl[H8CA_NN]; static int8_t h8ca_nk[H8CA_NN], h8ca_nh[H8CA_NN];
static void h8ca_emit(int x, int y, int kind) {
  for (int k = 0; k < H8CA_NN; k++) if (h8ca_nl[k] <= 0) { h8ca_nx[k] = (float)x; h8ca_ny[k] = (float)y; h8ca_nl[k] = 1.0f; h8ca_nk[k] = (int8_t)kind; h8ca_nh[k] = (int8_t)(8 + v3_rnd(8)); return; }
}
static void h8ca_floaters(float dt) {
  for (int k = 0; k < H8CA_NN; k++) {
    if (h8ca_nl[k] <= 0) continue;
    h8ca_nl[k] -= dt * 0.33f; h8ca_ny[k] -= dt * 28; h8ca_nx[k] += sinf(h8ca_nl[k] * 9 + k) * dt * 14;
    int x = (int)h8ca_nx[k], y = (int)h8ca_ny[k], l = v3_clamp((int)(h8ca_nl[k] * 20), 1, 15);
    uint8_t c = h8ca_nk[k] == 0 ? H8C(l, H8K_GOLD) : (h8ca_nk[k] == 1 ? H8C(l, H8K_PINK) : H8C(l, H8K_WHITE));
    if (h8ca_nk[k] == 0) { v8_ellipse(x, y, 4, 3, c); v8_thick(x + 3, y, x + 3, y - 12, 0, c); v8_thick(x + 3, y - 12, x + 8, y - 8, 1, c); }   // ♪
    else if (h8ca_nk[k] == 1) v8_text(x - 6, y - 7, "@", 2, c);                                                                                // heart
    else v8_text(x, y, "Z", 2, c);
  }
}

static float h8ca_beat = 0, h8ca_bpm = 92, h8ca_tapLast = -10, h8ca_now = 0, h8ca_t = 0;
static int h8ca_taps = 0; static float h8ca_tapSum = 0;
static int h8ca_lastBeat = -1;
void prog_ghcat_init() { v8_init(); for (int k = 0; k < H8CA_NN; k++) h8ca_nl[k] = 0; h8ca_taps = 0; }

// a soft background: a dark vertical gradient in the palette, a little brighter in the middle
// a ring of light (centre, radius in px)
static void h8ca_ring(int cx, int cy, int r, int lev, int hue) {
  if (r < 4 || lev <= 0) return;
  int px = cx + r, py = cy;
  for (int a = 16; a <= 1024; a += 16) {
    int nx = cx + ((r * v3_cos(a)) >> 14), ny = cy + ((r * v3_sin(a) * 3 / 4) >> 14);
    v8_thick(px, py, nx, ny, 1, H8C(v3_clamp(lev, 1, 15), hue)); px = nx; py = ny;
  }
}
static void h8ca_bg(int glow) {
  for (int y = 0; y < H; y++) {
    int f = v3_abs(y - H / 2) * 256 / (H / 2);
    int l16 = (256 - f) * glow / 256;
    const uint8_t* bay = v8_bayer + ((y & 3) << 2);
    uint32_t w = 0;
    for (int k = 0; k < 4; k++) w |= (uint32_t)(((v3_clamp((l16 + bay[k]) >> 4, 0, 15)) << 4) | (8 + ((y * 8) / H))) << (k * 8);
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int i = 0; i < W / 4; i++) p[i] = w;
  }
}

void prog_ghcat_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8CaLook& L = h8ca_looks[look];
  float dt = v6_f.dt;
  h8ca_now += dt; h8ca_t += v6_f.sdt;
  // tempo: the knob, or taps on k12
  if (v6_k12hit) {
    float gap = h8ca_now - h8ca_tapLast;
    if (gap > 0.25f && gap < 2.0f) { h8ca_tapSum += gap; h8ca_taps++; h8ca_bpm = v3_clampf(60.0f * h8ca_taps / h8ca_tapSum, 40, 200); }
    else { h8ca_taps = 0; h8ca_tapSum = 0; }
    h8ca_tapLast = h8ca_now; h8ca_beat = floorf(h8ca_beat + 0.5f);   // the tap is the downbeat
  }
  if (!v3_centre(5)) { h8ca_bpm = 50.0f + v3_u(5) * 130.0f; h8ca_taps = 0; h8ca_tapSum = 0; }
  else if (h8ca_taps == 0 && h8ca_now - h8ca_tapLast > 2.0f && h8ca_tapSum == 0) h8ca_bpm += (92.0f - h8ca_bpm) * v3_clampf(dt, 0, 1);
  h8ca_beat += dt * h8ca_bpm / 60.0f;
  if (h8ca_beat > 100000.0f) h8ca_beat -= 99968.0f;
  int beatN = (int)h8ca_beat; float ph = h8ca_beat - beatN;
  bool onBeat = beatN != h8ca_lastBeat; h8ca_lastBeat = beatN;

  int fur = v3_centre(0) ? L.fur : v3_clamp(v3_k(0) * 6 / 1024, 0, 5);
  float flow = v3_centre(3) ? 0.02f : v3_bi(3) * 0.2f;
  v6_flow += flow * dt; v6_flow -= floorf(v6_flow);
  v6_f.spread = v3_centre(4) ? 160 : (int)(v3_u(4) * 400);
  float key = v3_centre(11) ? 0.0f : v3_bi(11);
  h8ca_palette(fur, L.pal, key);
  float bop = v3_centre(7) ? L.bop / 100.0f : v3_u(7) * 1.7f;      // past ~60 % it really headbangs
  float notes = v8_kn(8, L.notes / 100.0f);
  bool caps = v3_centre(9) ? true : v3_k(9) > 512;
  int glow = (int)(v8_kn(10, L.glow / 100.0f) * 130);              // (v2: a more colourful backdrop)
  float z = v8_zoom(1.0f);

  // the nod: dips on the beat and eases back up; the tilt swaps side every beat
  float dip = ph < 0.25f ? ph / 0.25f : 1.0f - (ph - 0.25f) / 0.75f;
  dip = dip * dip * (3 - 2 * dip);
  int nod = (int)(dip * 200 * bop), lean = (int)(((beatN & 1) ? 1 : -1) * sinf(ph * 3.14159f) * 160 * bop);
  int tail = (int)(h8ca_t * 300);
  // ── the beat in the body and the room ──
  float kick = (1.0f - ph) * (1.0f - ph);                              // 1 on the beat, easing off
  h8ca_squash = dip * bop;
  h8ca_pawUp[beatN & 1] = (int)(kick * 9 * bop * z); h8ca_pawUp[(beatN + 1) & 1] = 0;
  h8ca_earW = ((beatN & 3) == 2) ? (int)(sinf(ph * 25.0f) * 4 * (1.0f - ph)) : 0;
  int b8 = beatN & 7;
  h8ca_glance = b8 == 5 ? -1 : (b8 == 6 ? 1 : 0);
  h8ca_bg((int)(glow * (1.0f + 0.9f * kick * kick)));                 // the backdrop pumps
  if (look == H8CA_LOAF || look == H8CA_NAP) v8_stars(80, 6, 4321);
  int s = (int)(46 * z * (1.0f + ((beatN & 3) == 0 ? 0.07f * kick : 0.0f))), cx = W / 2, cy = H / 2 - 18;   // a swell on every bar
  cx += (int)(sinf(h8ca_t * 0.35f) * 28 * bop);                       // sways about
  cy += (int)(dip * s * 0.06f) - (b8 == 7 ? (int)(sinf(ph * 3.14159f) * s * 0.7f * v3_clampf(bop, 0.3f, 1.2f)) : 0);   // a hop every 8 beats
  if (look != H8CA_LOAF && look != H8CA_NAP && look != H8CA_ZOOM)      // rings of light pulse out on every beat
    for (int r = 0; r < 3; r++) { float rp = ph + r; h8ca_ring(cx, cy + s, (int)(s * 1.4f + rp * 70), (int)(9 * (1.0f - rp / 3.0f)), 8 + ((beatN + r) & 7)); }
  if (onBeat && notes > 0.05f && v3_rnd(100) < (int)(notes * 100) && look != H8CA_NAP) h8ca_emit(cx + s + v3_rnd(40), cy, look == H8CA_DUET ? 1 : 0);

  switch (look) {
    case H8CA_SHADES: {                                               // 16-beat cycle: vibe, shades come down, hold, lift
      int b16 = beatN & 15; float pos = b16 + ph;
      int shade = 999;
      if (pos >= 8 && pos < 10) { float e = (pos - 8) / 2; e = e * e * (3 - 2 * e); shade = (int)((1 - e) * (cy + s)); }
      else if (pos >= 10 && pos < 15) shade = 0;
      else if (pos >= 15) shade = (int)((pos - 15) * (cy + s));
      h8ca_cat(cx, cy, s, nod, lean, shade == 0 ? 0 : 1, shade, tail, 0);
      if (caps && pos >= 10 && pos < 15) v8_textC(W / 2, H - 26, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break; }
    case H8CA_CHOIR: case H8CA_STACK: case H8CA_RAVE: case H8CA_DUET: {
      int n = v3_centre(6) ? L.n : 1 + v3_k(6) * 9 / 1024;
      if (look == H8CA_DUET) n = 2;
      for (int i = 0; i < n; i++) {
        float p2 = ph - i * 0.15f; p2 -= floorf(p2);
        float d2 = p2 < 0.25f ? p2 / 0.25f : 1.0f - (p2 - 0.25f) / 0.75f; d2 = d2 * d2 * (3 - 2 * d2);
        int nd = (int)(d2 * 200 * bop), ln = (int)((((beatN + i) & 1) ? 1 : -1) * sinf(p2 * 3.14159f) * 160 * bop);
        int x, y, ss;
        if (look == H8CA_STACK) { ss = (int)(30 * z) - i * 2; x = cx + (int)(sinf(h8ca_beat * 3.14159f * 0.5f + i * 0.8f) * i * 4 * bop); y = H - 40 - i * ss * 2 - ss; }
        else if (look == H8CA_RAVE) { int cols = n <= 4 ? 2 : 3; ss = (int)((n <= 4 ? 26 : 19) * z); x = W / (cols * 2) * (2 * (i % cols) + 1); y = (H / ((n + cols - 1) / cols + 1)) * (i / cols + 1) - ss / 2; }
        else if (look == H8CA_DUET) { ss = (int)(36 * z); x = W / 2 + (i ? 70 : -70); y = cy + 6; ln = (i ? -100 : 100) + ln / 2; }
        else { ss = (int)((n > 5 ? 18 : 24) * z); x = W / (n + 1) * (i + 1); y = cy + 10; }
        h8ca_cat(x, y, ss, nd, ln, 1, 999, tail + i * 200, 0);
        if (look == H8CA_DUET && onBeat && i == 0 && notes > 0.05f) h8ca_emit(W / 2, cy - 10, 1);
      }
      if (caps) v8_textC(W / 2, H - 22, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break; }
    case H8CA_LOAF: case H8CA_NAP: {
      bool nap = look == H8CA_NAP;
      float br = nap ? 1.0f + 0.03f * sinf(h8ca_t * 1.6f) : 1.0f;
      int lx = nap ? cx : (int)(cx + 90 * sinf(h8ca_t * 0.21f)), ly = nap ? cy + 20 : (int)(cy + 10 + 30 * sinf(h8ca_t * 0.33f));
      h8ca_cat(lx, ly - (int)(s * (br - 1) * 4), (int)(s * br), nap ? 40 : nod / 3, nap ? 0 : lean / 3, nap ? 2 : 1, 999, tail, 1);
      if (nap && (beatN & 1) && onBeat) h8ca_emit(lx + s, ly - s, 2);
      if (caps) v8_textC(W / 2, H - 22, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break; }
    case H8CA_DJ: {
      int dy = cy + s * 2 + 4;
      h8ca_cat(cx, cy, s, nod, lean, 1, ((beatN >> 4) & 1) ? 0 : 999, tail, 0);
      for (int b = 0; b < 16; b++) {                                   // the levels behind the decks
        int hgt = (int)((10 + 30 * fabsf(sinf(h8ca_beat * 3.14159f + b * 0.7f))) * (0.6f + 0.4f * dip));
        v3_rect(8 + b * 19, dy - hgt, 8 + b * 19 + 12, dy - 1, H8C(6, 8 + (b & 7)));
      }
      v3_rect(0, dy, W - 1, H - 1, H8C(2, H8K_LINE));
      for (int d = 0; d < 2; d++) {
        int tx = d ? W - 80 : 80, ty = dy + 22;
        v8_ellipse(tx, ty, 46, 16, H8C(6, H8K_WHITE)); v8_ellipse(tx, ty, 44, 15, H8C(3, H8K_SHADES)); v8_ellipse(tx, ty, 6, 2, H8C(12, H8K_PINK));
        int a = (int)(h8ca_t * 700) + d * 300;
        v8_thick(tx + ((v3_cos(a) * 34) >> 14), ty + ((v3_sin(a) * 11) >> 14), tx + ((v3_cos(a) * 40) >> 14), ty + ((v3_sin(a) * 13) >> 14), 1, H8C(13, H8K_WHITE));
      }
      int px = W - 80 + (int)(18 * sinf(h8ca_beat * 6.2832f));         // a paw scratching
      v8_ellipse(px, dy + 14, 12, 8, H8C(3, H8K_LINE)); v8_ellipse(px, dy + 14, 10, 6, H8C(13, H8K_BELLY));
      if (caps) v8_textC(W / 2, 10, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break; }
    case H8CA_DISCO: {
      for (int sp = 0; sp < 6; sp++) {                                  // light spots sweeping round
        int sx = (int)(W / 2 + 140 * sinf(h8ca_t * 0.5f + sp * 1.05f)), sy = (int)(H / 2 + 90 * cosf(h8ca_t * 0.37f + sp * 1.7f));
        v8_ellipse(sx, sy, 18, 12, H8C(5, 8 + sp));
      }
      int bx = W / 2, by = 26, br = 22;
      for (int yy = -br; yy <= br; yy += 4) for (int xx = -br; xx <= br; xx += 4) {
        if (xx * xx + yy * yy > br * br) continue;
        uint32_t hh = v3_hash((uint32_t)((xx + (int)(h8ca_t * 20)) * 31 + yy * 977));
        int l = 5 + (int)(hh & 7) + (((hh >> 3) & 15) == (uint32_t)(beatN & 15) ? 4 : 0);
        v3_rect(bx + xx, by + yy, bx + xx + 2, by + yy + 2, H8C(v3_clamp(l, 1, 15), (hh >> 8) & 1 ? H8K_WHITE : 8 + ((hh >> 9) & 7)));
      }
      v3_line(bx, 0, bx, by - br, H8C(6, H8K_WHITE));
      h8ca_cat(cx, cy + 22, (int)(s * 0.85f), nod, lean, 1, 999, tail, 0);
      if (caps) v8_textC(W / 2, H - 22, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break; }
    case H8CA_ZOOM: {                                                   // 12 s: run, run, run … then sit and vibe
      float c12 = fmodf(h8ca_t, 12.0f);
      if (c12 < 4.0f) {
        float run = fmodf(c12, 1.33f) / 1.33f; int dir = ((int)(c12 / 1.33f)) & 1;
        int x = dir ? (int)(W + 60 - run * (W + 120)) : (int)(-60 + run * (W + 120));
        int hop = (int)(fabsf(sinf(c12 * 18)) * 10);
        for (int k = 1; k < 6; k++) v8_thick(x - (dir ? -1 : 1) * (s + k * 12), cy + s / 2 + k * 5, x - (dir ? -1 : 1) * (s + k * 12 + 20), cy + s / 2 + k * 5, 1, H8C(7, H8K_WHITE));   // speed lines
        h8ca_cat(x, cy - hop, (int)(s * 0.8f), 60, dir ? -200 : 200, 0, 999, tail * 4, 0);
        if (caps) v8_textC(W / 2, 14, h8ca_caps[look], 3, H8C(15, H8K_GOLD));
      } else h8ca_cat(cx, cy, s, nod, lean, 1, 999, tail, 0);
      break; }
    default:
      h8ca_cat(cx, cy, s, nod, lean, (b8 == 5 || b8 == 6) ? 0 : 1, 999, tail * 2, 0);
      if (caps && (beatN & 31) < 12) v8_textC(W / 2, H - 22, h8ca_caps[look], 2, H8C(15, H8K_GOLD));
      break;
  }
  h8ca_floaters(dt);
  v8_end();
}

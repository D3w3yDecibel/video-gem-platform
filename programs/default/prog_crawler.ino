// =====================================================================
// PROGRAM: CRAWLER — a segmented bug winds down a mushroom field
// (by Dewey) — Set 5 "Arcade", slot 5
//
// In the spirit of the 1981 bug-shooter: a long CRAWLER snakes across the
// screen, dropping a row every time it hits a mushroom or the edge. Shoot a
// segment and it turns into a mushroom — and the crawler splits in two,
// each half with its own head. A SPIDER zig-zags round your patch at the
// bottom eating mushrooms, a FLEA drops straight down sowing new ones, and
// a SCORPION poisons the mushrooms it passes, so a head that touches one
// dives straight for you. Clear the crawler and the whole field changes
// colour for the next wave.
//
// It plays itself; touch p0/p1 or k12 and you're the shooter.
//
// Presets:
//   k0 Classic   k1 Garden   k2 Psychedelic (colours never stop)   k3 Long Crawler
//   k4 Many Heads   k5 Fast   k6 Mushroom Forest   k7 Night Glow   k8 Neon
//   k9 Phosphor   k10 Pastel   k11 Tour — a new preset every wave
//   k12 — FIRE (hold for rapid fire). Joins the game.
//
// Knobs:
//   p0 Shooter ◄ left | right ►   p1 Shooter ◄ low | high ►   (touch to play)
//   p2 Speed  p3 Palette  p4 Trails  p5 Difficulty  p6 Camera (◄ CRT | zoom ►)
//   p7 Crawler Length (centre = preset)   p8 Mushrooms (sparse … dense)
//   p9 Critters (spider, flea, scorpion: none … lots)   p10 Crawler Speed
//   p11 Background (centre = preset: black, grass, starfield, stripes)
//   p12-p15 live FX: Warp, Mirror, Colour, Glitch (centre = off)
// =====================================================================

#include "vg5a.h"

// hues: 0 white, 1 mushroom cap, 2 mushroom stem, 3 body, 4 head, 5 shooter, 6 shot, 7 spider, 8 flea,
//       9 scorpion, 10 poisoned, 11 HUD, 12 background, 13 bang, 14 legs
struct Cr6Pre { uint8_t look, len, heads, speed, density, critters, bg, cycle, trails; };
static const Cr6Pre cr6_pre[11] = {
  { A5_ARCADE, 12, 0, 1, 45, 50, 0, 0, 0 }, { A5_ARCADE, 12, 0, 1, 55, 50, 1, 0, 0 }, { A5_ARCADE, 12, 0, 1, 45, 50, 0, 1, 60 },
  { A5_ARCADE, 28, 0, 1, 45, 40, 0, 0, 0 }, { A5_ARCADE, 10, 5, 1, 45, 50, 0, 0, 0 }, { A5_ARCADE, 12, 1, 2, 45, 60, 0, 0, 0 },
  { A5_ARCADE, 12, 0, 1, 90, 50, 0, 0, 0 }, { A5_ARCADE, 14, 1, 1, 50, 50, 2, 0, 140 }, { A5_NEON, 12, 1, 1, 45, 60, 3, 0, 100 },
  { A5_PHOSPHOR, 12, 1, 1, 45, 50, 0, 0, 160 }, { A5_PASTEL, 12, 1, 1, 50, 50, 0, 0, 0 },
};
static const char* const cr6_names[12] = { "Classic", "Garden", "Psychedelic", "Long Crawler", "Many Heads", "Fast", "Mushroom Forest", "Night Glow", "Neon", "Phosphor", "Pastel", "Tour" };
static const char* const cr6_lbl[16] = { "Shooter: Left|Right", "Shooter: Low|High", "", "", "", "", "", "Crawler Length", "Mushrooms", "Critters", "Crawler Speed", "Background", "", "", "", "" };
const char* prog_crawler_name() { return "CRAWLER"; }
const char* prog_crawler_character() { return "A segmented crawler winds through a mushroom field - plays itself until you touch p0/p1/k12"; }
const char* prog_crawler_presetName(int preset) { return (preset >= 0 && preset < 12) ? cr6_names[preset] : NULL; }
const char* prog_crawler_potLabel(int preset, int pot) { (void)preset; return a5_label(pot, cr6_lbl); }
uint8_t prog_crawler_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// colours for each wave: mushroom cap, stem, crawler body, head
static const uint8_t cr6_waveCols[8][4][3] = {
  {{255,40,40},{60,200,255},{60,220,60},{255,60,200}}, {{255,220,0},{255,60,60},{60,120,255},{255,255,255}},
  {{80,255,80},{255,140,0},{255,60,200},{255,255,0}}, {{60,140,255},{255,240,0},{255,100,0},{60,255,200}},
  {{255,100,255},{100,255,255},{255,220,60},{255,60,60}}, {{255,140,40},{120,255,80},{120,120,255},{255,180,220}},
  {{220,220,220},{255,60,60},{60,255,120},{255,220,0}}, {{0,220,180},{255,80,160},{255,255,120},{120,180,255}},
};

#define CR6_C 40
#define CR6_R 28
#define CR6_Y0 16
#define CR6_ZONE 22                     // rows 22-27: the shooter's patch
#define CR6_NS 40
static uint8_t cr6_m[CR6_R][CR6_C];     // mushrooms: health 1-4, 0x80 = poisoned
static int16_t cr6_sx[CR6_NS], cr6_sy[CR6_NS];  // segments: x px, y px
static int8_t cr6_sd[CR6_NS], cr6_sv[CR6_NS];   // dir ±1, vertical heading ±1 (down / up in the patch)
static uint8_t cr6_son[CR6_NS], cr6_shead[CR6_NS], cr6_sdrop[CR6_NS], cr6_sdive[CR6_NS];
static float cr6_px = 160, cr6_py = 228, cr6_bx = -1, cr6_by = 0, cr6_dead = 0, cr6_banner = 0, cr6_over = 0;
static float cr6_spx = -20, cr6_spy = 200, cr6_spvx = 0, cr6_spvy = 0; static int cr6_spOn = 0;
static float cr6_fx = 0, cr6_fy = -10; static int cr6_fOn = 0, cr6_fHp = 0;
static float cr6_scx = -20, cr6_scy = 60, cr6_scv = 0; static int cr6_scOn = 0;
static int cr6_wave = 1, cr6_lives = 3, cr6_preset = -1, cr6_look = 0, cr6_speed = 1, cr6_frame = 0;
static uint32_t cr6_score = 0, cr6_hi = 16543;
static float cr6_diff = 0.4f, cr6_critK = 0.5f, cr6_aiX = 160, cr6_aiY = 228, cr6_aiT = 0;
static int cr6_popX[4], cr6_popY[4], cr6_popV[4], cr6_popT[4];

static const char cr6_sMush[65] = ".111111." "11211211" "11111111" "12111121" ".111111." "...22..." "...22..." "..2222..";
static const char cr6_sSeg[2][65] = { "..3333.." ".333333." "33333333" "33333333" "33333333" ".333333." "e.3333.e" ".e....e.",
                                      "..3333.." ".333333." "33333333" "33333333" "33333333" ".333333." ".e3333e." "e......e" };
static const char cr6_sHead[65] = "..4444.." ".444444." "41144114" "41f441f4" "44444444" ".444444." "e.4444.e" ".e....e.";
static const char cr6_sShooter[65] = "...55..." "...55..." "..5005.." ".555555." "55555555" "55.55.55" "5..55..5" "........";
static const char cr6_sSpider[129] = "e..e....e....e.." ".e.e....e...e.e." "..e77777777777e." "...7f77777777f7." "...7777777777777" "..e77777777777e." ".e.e....e....e.e" "e..e....e.....e.";
static const char cr6_sFlea[65] = "..8888.." ".888888." "8f8888f8" "88888888" ".888888." "..e..e.." ".e....e." "e......e";
static const char cr6_sScorp[129] = "..........999..." "...........9.9.." "............99.." ".9999999999999.." "9f99999999999999" ".9999999999999.." "..e.e.e.e.e.e..." ".e.e.e.e.e.e...." ;
static uint8_t cr6_cmap[16];

static inline bool cr6_mush(int c, int r) { return c >= 0 && c < CR6_C && r >= 0 && r < CR6_R && (cr6_m[r][c] & 0x7F); }
static void cr6_addPop(int x, int y, int v) { int k = 0; for (int i = 1; i < 4; i++) if (cr6_popT[i] < cr6_popT[k]) k = i; cr6_popX[k] = x; cr6_popY[k] = y; cr6_popV[k] = v; cr6_popT[k] = 50; cr6_score += v; }
static void cr6_sow(int density) {
  memset(cr6_m, 0, sizeof(cr6_m));
  int n = density * 2;
  for (int k = 0; k < n; k++) { int c = v3_rnd(CR6_C), r = 1 + v3_rnd(CR6_R - 3); cr6_m[r][c] = 4; }
}
static void cr6_newCrawler(int len, int heads) {
  memset(cr6_son, 0, sizeof(cr6_son));
  len = v3_clamp(len, 1, CR6_NS);
  int body = len - heads; if (body < 1) body = 1;
  int k = 0;
  int dir = v3_rnd(2) ? 1 : -1, x0 = dir > 0 ? 8 * 16 : W - 8 * 17;
  for (int i = 0; i < body && k < CR6_NS; i++, k++) {             // the main crawler: a head and a long tail behind it
    cr6_son[k] = 1; cr6_shead[k] = i == 0; cr6_sd[k] = (int8_t)dir; cr6_sv[k] = 1; cr6_sdrop[k] = 0; cr6_sdive[k] = 0;
    cr6_sx[k] = (int16_t)(x0 - dir * i * 8); cr6_sy[k] = CR6_Y0;
  }
  for (int h = 0; h < heads && k < CR6_NS; h++, k++) {             // loose heads
    cr6_son[k] = 1; cr6_shead[k] = 1; cr6_sd[k] = v3_rnd(2) ? 1 : -1; cr6_sv[k] = 1; cr6_sdrop[k] = 0; cr6_sdive[k] = 0;
    cr6_sx[k] = (int16_t)(8 * v3_rnd(CR6_C)); cr6_sy[k] = CR6_Y0;
  }
}
static void cr6_startWave() {
  const Cr6Pre& P = cr6_pre[cr6_look];
  int len = v3_centre(7) ? P.len : 4 + (int)(v3_u(7) * 36);
  cr6_newCrawler(len, P.heads + (cr6_wave - 1) / 2);
  cr6_banner = 1.4f;
}
static void cr6_newGame() {
  const Cr6Pre& P = cr6_pre[cr6_look];
  cr6_score = 0; cr6_lives = 3; cr6_wave = 1; cr6_over = 0; cr6_dead = 0; cr6_spOn = cr6_fOn = cr6_scOn = 0;
  cr6_sow(v3_centre(8) ? P.density : (int)(v3_u(8) * 140));
  cr6_px = 160; cr6_py = CR6_Y0 + 27 * 8;
  cr6_startWave();
}
static void cr6_die() {
  if (cr6_dead > 0) return;
  cr6_dead = 2.4f; a5_shake(5); a5_burst((int)cr6_px + 4, (int)cr6_py + 4, 50, 5, 50, 50);
}

// the crawler moves: across, and down a row (or up in the patch) whenever blocked
static void cr6_moveSegs(int spd) {
  for (int i = 0; i < CR6_NS; i++) if (cr6_son[i]) {
    for (int s = 0; s < spd; s++) {
      if (cr6_sdrop[i] > 0) { cr6_sy[i] += cr6_sv[i]; cr6_sdrop[i]--; if (cr6_sdrop[i] == 0 && cr6_sdive[i] && cr6_sy[i] < CR6_Y0 + (CR6_R - 1) * 8) cr6_sdrop[i] = 8; continue; }
      if (cr6_sdive[i] && cr6_sy[i] >= CR6_Y0 + (CR6_R - 1) * 8) cr6_sdive[i] = 0;
      if ((cr6_sx[i] & 7) == 0) {
        int c = cr6_sx[i] / 8, r = (cr6_sy[i] - CR6_Y0) / 8, nc = c + cr6_sd[i];
        bool blocked = nc < 0 || nc >= CR6_C || cr6_mush(nc, r);
        if (nc >= 0 && nc < CR6_C && r >= 0 && r < CR6_R && (cr6_m[r][nc] & 0x80)) cr6_sdive[i] = 1;   // poisoned: dive
        if (blocked || cr6_sdive[i]) {
          // turn: drop a row (or rise, down in the patch)
          if (r >= CR6_R - 1) cr6_sv[i] = -1;
          if (r <= CR6_ZONE && cr6_sv[i] < 0) cr6_sv[i] = 1;
          cr6_sdrop[i] = 8; cr6_sd[i] = (int8_t)-cr6_sd[i];
          if (cr6_sdive[i]) cr6_sv[i] = 1;
          continue;
        }
      }
      cr6_sx[i] += cr6_sd[i];
    }
  }
}
static void cr6_hitSeg(int i) {
  int c = (cr6_sx[i] + 4) / 8, r = (cr6_sy[i] - CR6_Y0 + 4) / 8;
  if (c >= 0 && c < CR6_C && r >= 0 && r < CR6_R) cr6_m[r][c] = 4;
  cr6_addPop(cr6_sx[i], cr6_sy[i], cr6_shead[i] ? 100 : 10);
  a5_burst(cr6_sx[i] + 4, cr6_sy[i] + 4, 14, cr6_shead[i] ? 4 : 3, 30, 22);
  cr6_son[i] = 0;
  // the segment that was following it becomes a head: the next one in the chain, right behind
  for (int k = 0; k < CR6_NS; k++) if (cr6_son[k] && !cr6_shead[k] && cr6_sy[k] == cr6_sy[i] && v3_abs(cr6_sx[k] - (cr6_sx[i] - cr6_sd[i] * 8)) <= 2) { cr6_shead[k] = 1; break; }
  for (int k = 0; k < CR6_NS; k++) if (cr6_son[k] && !cr6_shead[k]) {    // anything with no segment ahead of it is a head
    bool led = false; int ax = cr6_sx[k] + cr6_sd[k] * 8;
    for (int j = 0; j < CR6_NS && !led; j++) if (j != k && cr6_son[j] && v3_abs(cr6_sx[j] - ax) <= 3 && v3_abs(cr6_sy[j] - cr6_sy[k]) <= 8) led = true;
    if (!led) for (int j = 0; j < CR6_NS && !led; j++) if (j != k && cr6_son[j] && v3_abs(cr6_sx[j] - cr6_sx[k]) <= 8 && v3_abs(cr6_sy[j] - cr6_sy[k]) <= 9 && cr6_sy[j] != cr6_sy[k]) led = true;
    if (!led) cr6_shead[k] = 1;
  }
}

// the computer: get under the lowest head, keep firing, dodge the spider
static void cr6_ai() {
  int best = -1, by = -1;
  for (int i = 0; i < CR6_NS; i++) if (cr6_son[i]) { int s = cr6_sy[i] * 2 + (cr6_shead[i] ? 30 : 0); if (s > by) { by = s; best = i; } }
  float tx = cr6_px, ty = CR6_Y0 + 27 * 8;
  if (best >= 0) tx = cr6_sx[best] + cr6_sd[best] * 10;
  if (cr6_fOn) tx = cr6_fx;
  if (cr6_spOn) {
    float dx = cr6_spx - cr6_px, dy = cr6_spy - cr6_py;
    if (fabsf(dx) < 34 && fabsf(dy) < 30) { tx = cr6_px - (dx > 0 ? 40 : -40); ty = cr6_spy < CR6_Y0 + 25 * 8 ? CR6_Y0 + 27 * 8 : CR6_Y0 + 23 * 8; }
    else if (fabsf(dx) < 60 && cr6_spy > cr6_py - 40) tx = cr6_spx;
  }
  // don't stand under a segment that's in the patch and coming down on us
  for (int i = 0; i < CR6_NS; i++) if (cr6_son[i] && cr6_sy[i] > CR6_Y0 + (CR6_ZONE - 1) * 8 && fabsf(cr6_sx[i] - tx) < 10 && fabsf(cr6_sy[i] - ty) < 18) tx += cr6_sd[i] > 0 ? -18 : 18;
  cr6_aiX = v3_clampf(tx, 0, W - 8); cr6_aiY = ty;
}

void prog_crawler_init() { v3_reset(); a5_reset(); cr6_preset = -1; a5_partClear(); }

static void crawler5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame(); fx5_take(); a5_input();
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != cr6_preset) { cr6_preset = preset; cr6_look = preset == 11 ? 0 : preset; cr6_newGame(); }
  const Cr6Pre& P = cr6_pre[cr6_look];
  cr6_diff = a5_diffK(0.35f);
  cr6_critK = v3_centre(9) ? P.critters / 100.0f : v3_u(9);
  cr6_speed = v3_centre(10) ? P.speed : 1 + (int)(v3_u(10) * 2.99f);
  int bg = v3_pick(11, P.bg, 4);

  int wc = (cr6_wave - 1) & 7;
  uint8_t hues[16][3] = { {255,255,255},{0},{0},{0},{0},{255,255,255},{255,255,120},{220,80,255},{255,255,255},{80,255,120},{170,0,220},{255,230,60},{20,60,20},{255,160,40},{200,180,160},{0} };
  if (P.cycle) wc = (int)(globalTime * 1.5f) & 7;
  memcpy(hues[1], cr6_waveCols[wc][0], 3); memcpy(hues[2], cr6_waveCols[wc][1], 3); memcpy(hues[3], cr6_waveCols[wc][2], 3); memcpy(hues[4], cr6_waveCols[wc][3], 3);
  if (bg == 2) { hues[12][0] = 10; hues[12][1] = 10; hues[12][2] = 40; } else if (bg == 3) { hues[12][0] = 60; hues[12][1] = 0; hues[12][2] = 70; }
  a5_palette(hues, 16, a5_lookK(P.look));
  static const uint8_t cmBase[16] = { 0, A5C(15,1), A5C(13,2), A5C(15,3), A5C(15,4), A5C(15,5), 0, A5C(15,7), A5C(15,8), A5C(15,9), 0, 0, 0, 0, A5C(13,14), A5C(15,0) };
  memcpy(cr6_cmap, cmBase, 16); cr6_cmap[15] = A5C(2, 0);

  float dt = 1.0f / 60.0f;
  int steps = a5_steps(a5_speedK());
  for (int st = 0; st < steps; st++) {
    a5_partStep(); cr6_frame++;
    for (int k = 0; k < 4; k++) if (cr6_popT[k] > 0) cr6_popT[k]--;
    if (cr6_banner > 0) cr6_banner -= dt;
    if (cr6_over > 0) { cr6_over -= dt; if (cr6_over <= 0) { if (cr6_preset == 11) cr6_look = (cr6_look + 1) % 11; cr6_newGame(); } continue; }
    if (cr6_dead > 0) {
      cr6_dead -= dt;
      if (cr6_dead < 1.6f && (cr6_frame & 3) == 0) {                   // mend damaged mushrooms, one by one, 5 points each
        for (int r = 0; r < CR6_R; r++) for (int c = 0; c < CR6_C; c++) if ((cr6_m[r][c] & 0x7F) && ((cr6_m[r][c] & 0x7F) < 4 || (cr6_m[r][c] & 0x80))) { cr6_m[r][c] = 4; cr6_score += 5; r = CR6_R; break; }
      }
      if (cr6_dead <= 0) {
        if (--cr6_lives < 0) { cr6_over = 4; if (cr6_score > cr6_hi) cr6_hi = cr6_score; }
        else { int n = 0; for (int i = 0; i < CR6_NS; i++) n += cr6_son[i]; cr6_newCrawler(n, P.heads); cr6_spOn = cr6_fOn = 0; cr6_px = 160; cr6_py = CR6_Y0 + 27 * 8; }
      }
      continue;
    }
    // the shooter
    float tx, ty; bool fire;
    if (a5_player) { tx = v3_u(0) * (W - 8); ty = CR6_Y0 + (CR6_R - 1) * 8 - v3_u(1) * (CR6_R - CR6_ZONE - 1) * 8; fire = a5_fireHeld || a5_fire; }
    else { cr6_aiT -= dt; if (cr6_aiT <= 0) { cr6_ai(); cr6_aiT = 0.1f; } tx = cr6_aiX; ty = cr6_aiY; fire = true; }
    float mv = 2.4f, dx = v3_clampf(tx - cr6_px, -mv, mv), dy = v3_clampf(ty - cr6_py, -mv, mv);
    float nx = cr6_px + dx, ny = cr6_py + dy;
    // mushrooms block the shooter
    if (!cr6_mush((int)(nx + 4) / 8, (int)(cr6_py + 4 - CR6_Y0) / 8)) cr6_px = nx;
    if (!cr6_mush((int)(cr6_px + 4) / 8, (int)(ny + 4 - CR6_Y0) / 8)) cr6_py = ny;
    cr6_px = v3_clampf(cr6_px, 0, W - 8); cr6_py = v3_clampf(cr6_py, CR6_Y0 + CR6_ZONE * 8, CR6_Y0 + (CR6_R - 1) * 8);
    // the shot: one at a time, very fast
    if (cr6_bx < 0 && fire) { cr6_bx = cr6_px + 3; cr6_by = cr6_py - 2; }
    if (cr6_bx >= 0) {
      for (int sub = 0; sub < 3 && cr6_bx >= 0; sub++) {
        cr6_by -= 3;
        if (cr6_by < CR6_Y0) { cr6_bx = -1; break; }
        int c = (int)cr6_bx / 8, r = (int)(cr6_by - CR6_Y0) / 8;
        if (cr6_mush(c, r)) {
          int h = (cr6_m[r][c] & 0x7F) - 1; cr6_m[r][c] = h ? (uint8_t)((cr6_m[r][c] & 0x80) | h) : 0;
          if (!h) { cr6_score += 1; a5_burst(c * 8 + 4, CR6_Y0 + r * 8 + 4, 6, 1, 20, 14); }
          cr6_bx = -1; break;
        }
        for (int i = 0; i < CR6_NS; i++) if (cr6_son[i] && cr6_bx >= cr6_sx[i] - 1 && cr6_bx <= cr6_sx[i] + 8 && cr6_by >= cr6_sy[i] && cr6_by <= cr6_sy[i] + 8) { cr6_hitSeg(i); cr6_bx = -1; break; }
        if (cr6_bx < 0) break;
        if (cr6_spOn && fabsf(cr6_bx - cr6_spx - 8) < 9 && fabsf(cr6_by - cr6_spy - 4) < 5) {
          int d = (int)(cr6_py - cr6_spy), v = d < 16 ? 900 : (d < 40 ? 600 : 300);
          cr6_addPop((int)cr6_spx, (int)cr6_spy, v); a5_burst((int)cr6_spx + 8, (int)cr6_spy + 4, 24, 7, 40, 30); cr6_spOn = 0; cr6_bx = -1; break;
        }
        if (cr6_fOn && fabsf(cr6_bx - cr6_fx - 4) < 5 && fabsf(cr6_by - cr6_fy - 4) < 6) { if (--cr6_fHp <= 0) { cr6_addPop((int)cr6_fx, (int)cr6_fy, 200); a5_burst((int)cr6_fx + 4, (int)cr6_fy + 4, 16, 8, 30, 24); cr6_fOn = 0; } cr6_bx = -1; break; }
        if (cr6_scOn && fabsf(cr6_bx - cr6_scx - 8) < 9 && fabsf(cr6_by - cr6_scy - 4) < 5) { cr6_addPop((int)cr6_scx, (int)cr6_scy, 1000); a5_burst((int)cr6_scx + 8, (int)cr6_scy + 4, 24, 9, 40, 30); cr6_scOn = 0; cr6_bx = -1; break; }
      }
    }
    // the crawler
    int spd = cr6_speed + (cr6_wave > 4 ? 1 : 0);
    cr6_moveSegs(spd);
    for (int i = 0; i < CR6_NS; i++) if (cr6_son[i] && fabsf(cr6_sx[i] - cr6_px) < 7 && fabsf(cr6_sy[i] - cr6_py) < 7) cr6_die();
    // spider
    if (!cr6_spOn) { if (v3_rnd(1000) < (int)(cr6_critK * 8)) { cr6_spOn = 1; cr6_spx = v3_rnd(2) ? -16 : W; cr6_spy = CR6_Y0 + CR6_ZONE * 8; cr6_spvx = (cr6_spx < 0 ? 1 : -1) * (0.8f + cr6_diff); cr6_spvy = 1.2f; } }
    else {
      cr6_spx += cr6_spvx; cr6_spy += cr6_spvy;
      if (cr6_spy > CR6_Y0 + (CR6_R - 1) * 8) cr6_spvy = -(0.8f + v3_rndf() * 1.2f);
      if (cr6_spy < CR6_Y0 + (CR6_ZONE - 2) * 8) cr6_spvy = 0.8f + v3_rndf() * 1.2f;
      if (v3_rnd(60) == 0) cr6_spvy = -cr6_spvy;
      int c = (int)(cr6_spx + 8) / 8, r = (int)(cr6_spy + 4 - CR6_Y0) / 8;
      if (c >= 0 && c < CR6_C && r >= 0 && r < CR6_R && cr6_m[r][c] && v3_rnd(6) == 0) cr6_m[r][c] = 0;   // it eats mushrooms
      if (cr6_spx < -20 || cr6_spx > W + 4) cr6_spOn = 0;
      if (fabsf(cr6_spx + 8 - cr6_px - 4) < 9 && fabsf(cr6_spy + 4 - cr6_py - 4) < 7) cr6_die();
    }
    // flea: drops when your patch is bare
    if (!cr6_fOn) {
      int n = 0; for (int r = CR6_ZONE; r < CR6_R; r++) for (int c = 0; c < CR6_C; c++) if (cr6_m[r][c]) n++;
      if (n < 5 && cr6_wave >= 2 && v3_rnd(1000) < (int)(cr6_critK * 10)) { cr6_fOn = 1; cr6_fHp = 2; cr6_fx = 8.0f * v3_rnd(CR6_C); cr6_fy = CR6_Y0; }
    } else {
      cr6_fy += 1.6f + cr6_diff + (cr6_fHp == 1 ? 1.5f : 0);
      int c = (int)cr6_fx / 8, r = (int)(cr6_fy - CR6_Y0) / 8;
      if (r >= 0 && r < CR6_R - 1 && !cr6_m[r][c] && v3_rnd(30) == 0) cr6_m[r][c] = 4;
      if (cr6_fy > H) cr6_fOn = 0;
      if (fabsf(cr6_fx - cr6_px) < 7 && fabsf(cr6_fy - cr6_py) < 7) cr6_die();
    }
    // scorpion: crosses high up, poisoning mushrooms
    if (!cr6_scOn) { if (cr6_wave >= 3 && v3_rnd(2000) < (int)(cr6_critK * 6)) { cr6_scOn = 1; cr6_scx = v3_rnd(2) ? -16 : W; cr6_scy = CR6_Y0 + 8.0f * (2 + v3_rnd(12)); cr6_scv = cr6_scx < 0 ? 0.7f : -0.7f; } }
    else {
      cr6_scx += cr6_scv;
      int c = (int)(cr6_scx + 8) / 8, r = (int)(cr6_scy - CR6_Y0) / 8;
      if (c >= 0 && c < CR6_C && cr6_m[r][c]) cr6_m[r][c] |= 0x80;
      if (cr6_scx < -20 || cr6_scx > W + 4) cr6_scOn = 0;
    }
    // wave cleared
    bool any = false; for (int i = 0; i < CR6_NS; i++) if (cr6_son[i]) any = true;
    if (!any) { cr6_wave++; if (cr6_preset == 11) cr6_look = (cr6_look + 1) % 11; cr6_startWave(); }
  }

  // ── draw ──
  a5_clear(a5_trailK(P.trails / 255.0f));
  if (bg == 1) for (int y = CR6_Y0; y < H; y++) for (int x = (y * 7) & 7; x < W; x += 8) v3_put(x, y, A5C(3 + ((x * 13 + y * 7) & 1), 12));
  if (bg == 2) a5_stars(80, 0, 2, 0, 21);
  if (bg == 3) for (int y = CR6_Y0; y < H; y += 8) { uint8_t c = A5C(((y / 8) & 1) ? 3 : 2, 12); for (int k = 0; k < 8 && y + k < H; k++) memset(v3_buf + (y + k) * W, c, W); }
  for (int x = 0; x < W; x += 4) v3_put(x, CR6_Y0 + CR6_ZONE * 8 - 1, A5C(4, 11));
  // mushrooms
  for (int r = 0; r < CR6_R; r++) for (int c = 0; c < CR6_C; c++) if (cr6_m[r][c]) {
    int h = cr6_m[r][c] & 0x7F, x = c * 8, y = CR6_Y0 + r * 8;
    uint8_t cm[16]; memcpy(cm, cr6_cmap, 16);
    if (cr6_m[r][c] & 0x80) { cm[1] = A5C(15, 10); cm[2] = A5C(11, 10); }
    if (bg == 2 || cr6_look == 7) { cm[1] = A5C(15, 1); }
    // damaged mushrooms lose rows from the bottom
    char sp[65]; memcpy(sp, cr6_sMush, 65);
    for (int k = 0; k < (4 - h) * 2; k++) memset(sp + (7 - k) * 8, '.', 8);
    a5_spr(sp, 8, 8, x, y, cm, 0, 0);
  }
  // crawler
  int fr = (cr6_frame >> 3) & 1;
  for (int i = 0; i < CR6_NS; i++) if (cr6_son[i]) a5_spr(cr6_shead[i] ? cr6_sHead : cr6_sSeg[(fr + i) & 1], 8, 8, cr6_sx[i], cr6_sy[i], cr6_cmap, cr6_sd[i] < 0 ? A5_FX : 0, 0);
  if (cr6_spOn) a5_spr(cr6_sSpider, 16, 8, (int)cr6_spx, (int)cr6_spy, cr6_cmap, fr ? A5_FX : 0, 0);
  if (cr6_fOn) a5_spr(cr6_sFlea, 8, 8, (int)cr6_fx, (int)cr6_fy, cr6_cmap, 0, cr6_fHp == 1 ? A5C(15, 0) : 0);
  if (cr6_scOn) a5_spr(cr6_sScorp, 16, 8, (int)cr6_scx, (int)cr6_scy, cr6_cmap, cr6_scv < 0 ? A5_FX : 0, 0);
  if (cr6_bx >= 0) v3_rect((int)cr6_bx, (int)cr6_by, (int)cr6_bx, (int)cr6_by + 5, A5C(15, 6));
  if (cr6_dead <= 0) a5_spr(cr6_sShooter, 8, 8, (int)cr6_px, (int)cr6_py, cr6_cmap, 0, 0);
  a5_partDraw();
  for (int k = 0; k < 4; k++) if (cr6_popT[k] > 0 && cr6_popV[k] >= 100) { char s[8]; snprintf(s, 8, "%d", cr6_popV[k]); a5_textS(v3_clamp(cr6_popX[k], 2, W - 26), cr6_popY[k] - 6, s, 1, A5C(15, 0)); }

  a5_post((int)cr6_px + 4, (int)cr6_py);
  a5_hud(cr6_score, cr6_hi, A5C(15, 11), A5C(15, 0));
  for (int i = 0; i < v3_clamp(cr6_lives, 0, 6); i++) a5_spr(cr6_sShooter, 8, 8, W - 10 - i * 10, 2, cr6_cmap, 0, 0);
  char s[16]; snprintf(s, 16, "WAVE %d", cr6_wave);
  if (cr6_banner > 0) a5_banner(H / 2 - 20, s, 2, A5C(15, 11));
  if (cr6_over > 0) a5_banner(H / 2 - 8, "GAME OVER", 2, A5C(15, 4));
  a5_crt();
}
void prog_crawler_draw(int preset) { crawler5_draw(preset); fx5_apply(); }

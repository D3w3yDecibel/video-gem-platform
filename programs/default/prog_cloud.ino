// =====================================================================
// PROGRAM: DATA CLOUD (by Dewey)
// A 3D cloud of hundreds of glowing nodes in contrasting colour groups —
// a dense bright core thinning out into space — wired to their hubs and
// to each other, with data packets racing along the wires, orbit rings,
// and tiny labels of strange made-up words. It turns, breathes and flies.
// Approximate take on dreokt's network-visualisation post (the labels
// are invented nonsense words, not anybody's name).
//
// All the maths is whole numbers (the RP2040 has no decimal hardware),
// and nothing is stored per node: every node's position, colour and
// label are re-made each frame from its number, so it uses almost no
// memory. Each node is worked out once per frame.
//
// Presets:
//   k0 Cloud         — the turning cloud, drifting in and out
//   k1 Fly-through   — endless dive through the cloud
//   k2 Clusters      — separate coloured groups, each wired to its own hub
//   k3 Globe         — nodes on a sphere, wired to the centre
//   k4 Burst         — the cloud explodes outward and pulls back in
//   k5 Galaxy        — a flat spiral with arms
//   k6 Constellation — few, bright, heavily labelled nodes
//
//   k12 — press for a new cloud
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Colors (duotone, heat, neon [centre], ice, candy)
//   p1 Speed (far left = still)   p2 Lines (left = none)
//   p3 Labels (how many)
//   p4 Nodes (how many)   p5 Zoom   p6 Spread   p7 Tilt
//   p8 Fly (how far it swoops in and out)   p9 Glow (how many bright nodes)
//   p10 Beam — turn it up for more:
//        a light axis through the cloud with pulses racing along it,
//        then (⅓ up) a scanner ring sweeping up and down that lights up
//        and names the nodes it passes, then (⅔ up) radar arms spinning
//        round the scanner
//   p11 Web (links between neighbours)
//   p12 Packets (data racing along the wires)   p13 Rings (orbits)
//   p14 / p15 do something different in each preset:
//     Cloud          p14 Core (how many crowd the centre)   p15 Shimmer
//     Fly-through    p14 Tunnel (hollow middle)             p15 Warp (streaks)
//     Clusters       p14 Groups (1–5)                       p15 Hub Links
//     Globe          p14 Bands (rings of latitude)          p15 Wobble
//     Burst          p14 Rhythm (how fast it pulses)        p15 Scatter
//     Galaxy         p14 Arms (2–6)                         p15 Twist
//     Constellation  p14 Star Size                          p15 Word Length
// =====================================================================

#define CL_MAXN   1000
#define CL_GROUPS 5

// Palette (ordered so that brighter things always win when drawn over)
#define CL_LINE0   1       // 1–30   lines: group × 6 levels
#define CL_LINEN   6
#define CL_DOT0    32      // 32–71  nodes: group × 8 levels (far → near)
#define CL_DOTN    8
#define CL_STAR    72      // bright node core
#define CL_BEAM    73
#define CL_PACKET  74
#define CL_LAB0    80      // 80–89  label colours: group × (text, box)

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float cl_sm[16];
static bool  cl_smReady = false;

static void cl_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!cl_smReady) cl_sm[i] = pots[i];
    else cl_sm[i] += (pots[i] - cl_sm[i]) * a;
  }
  cl_smReady = true;
}
static inline float cl_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (cl_sm[idx] / 1023.0f); }
static inline int cl_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (cl_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Whole-number sine (256 steps per turn, ×16384) ───────────────────
static const int16_t cl_sinT[256] = {
  0, 402, 804, 1205, 1606, 2006, 2404, 2801, 3196, 3590, 3981, 4370, 4756, 5139, 5520, 5897,
  6270, 6639, 7005, 7366, 7723, 8076, 8423, 8765, 9102, 9434, 9760, 10080, 10394, 10702, 11003, 11297,
  11585, 11866, 12140, 12406, 12665, 12916, 13160, 13395, 13623, 13842, 14053, 14256, 14449, 14635, 14811, 14978,
  15137, 15286, 15426, 15557, 15679, 15791, 15893, 15986, 16069, 16143, 16207, 16261, 16305, 16340, 16364, 16379,
  16383, 16379, 16364, 16340, 16305, 16261, 16207, 16143, 16069, 15986, 15893, 15791, 15679, 15557, 15426, 15286,
  15137, 14978, 14811, 14635, 14449, 14256, 14053, 13842, 13623, 13395, 13160, 12916, 12665, 12406, 12140, 11866,
  11585, 11297, 11003, 10702, 10394, 10080, 9760, 9434, 9102, 8765, 8423, 8076, 7723, 7366, 7005, 6639,
  6270, 5897, 5520, 5139, 4756, 4370, 3981, 3590, 3196, 2801, 2404, 2006, 1606, 1205, 804, 402,
  0, -402, -804, -1205, -1606, -2006, -2404, -2801, -3196, -3590, -3981, -4370, -4756, -5139, -5520, -5897,
  -6270, -6639, -7005, -7366, -7723, -8076, -8423, -8765, -9102, -9434, -9760, -10080, -10394, -10702, -11003, -11297,
  -11585, -11866, -12140, -12406, -12665, -12916, -13160, -13395, -13623, -13842, -14053, -14256, -14449, -14635, -14811, -14978,
  -15137, -15286, -15426, -15557, -15679, -15791, -15893, -15986, -16069, -16143, -16207, -16261, -16305, -16340, -16364, -16379,
  -16383, -16379, -16364, -16340, -16305, -16261, -16207, -16143, -16069, -15986, -15893, -15791, -15679, -15557, -15426, -15286,
  -15137, -14978, -14811, -14635, -14449, -14256, -14053, -13842, -13623, -13395, -13160, -12916, -12665, -12406, -12140, -11866,
  -11585, -11297, -11003, -10702, -10394, -10080, -9760, -9434, -9102, -8765, -8423, -8076, -7723, -7366, -7005, -6639,
  -6270, -5897, -5520, -5139, -4756, -4370, -3981, -3590, -3196, -2801, -2404, -2006, -1606, -1205, -804, -402,
};
static inline int cl_sin(int a) { return cl_sinT[a & 255]; }
static inline int cl_cos(int a) { return cl_sinT[(a + 64) & 255]; }

// ─── Colours ──────────────────────────────────────────────────────────
static int cl_palKey = -1;

static void cl_hue(float h, float* c) {
  h -= floorf(h);
  float x = h * 6.0f; int k = (int)x; float q = x - k;
  switch (k) {
    case 0:  c[0] = 1; c[1] = q; c[2] = 0; break;
    case 1:  c[0] = 1 - q; c[1] = 1; c[2] = 0; break;
    case 2:  c[0] = 0; c[1] = 1; c[2] = q; break;
    case 3:  c[0] = 0; c[1] = 1 - q; c[2] = 1; break;
    case 4:  c[0] = q; c[1] = 0; c[2] = 1; break;
    default: c[0] = 1; c[1] = 0; c[2] = 1 - q; break;
  }
}

// Five strongly contrasting colours per scheme (r, g, b)
static const uint8_t cl_schemes[5][CL_GROUPS][3] = {
  {{0, 230, 255}, {255, 120, 0}, {0, 150, 255}, {255, 200, 40}, {240, 240, 255}},      // duotone: cyan vs orange
  {{255, 40, 20}, {255, 170, 0}, {255, 255, 90}, {255, 0, 120}, {255, 255, 255}},      // heat
  {{0, 255, 255}, {255, 0, 200}, {255, 240, 0}, {60, 255, 60}, {255, 110, 0}},         // neon
  {{120, 220, 255}, {0, 120, 255}, {200, 255, 255}, {100, 90, 255}, {255, 255, 255}},  // ice
  {{255, 80, 180}, {80, 255, 200}, {180, 120, 255}, {255, 230, 80}, {120, 200, 255}}   // candy
};

static void cl_buildPalette(int scheme, int lineLvl, int shift) {
  int key = (scheme * 64 + lineLvl / 4) * 8 + shift;
  if (key == cl_palKey) return;
  cl_palKey = key;
  for (int i = 0; i < 256; i++) display.setColor(i, 0, 0, 0);
  for (int g = 0; g < CL_GROUPS; g++) {
    const uint8_t* c = cl_schemes[scheme][(g + shift) % CL_GROUPS];
    for (int l = 0; l < CL_LINEN; l++) {                     // lines: faint, in the group colour
      float v = lineLvl / 255.0f * (0.35f + 0.65f * l / (CL_LINEN - 1));
      display.setColor(CL_LINE0 + g * CL_LINEN + l, (uint8_t)(c[0] * v), (uint8_t)(c[1] * v), (uint8_t)(c[2] * v));
    }
    for (int l = 0; l < CL_DOTN; l++) {                      // nodes: far = dim colour, near = hot / whitish
      float t = l / (float)(CL_DOTN - 1);
      float v = 0.35f + 0.65f * t, w = t * t * 0.25f;
      display.setColor(CL_DOT0 + g * CL_DOTN + l, (uint8_t)((c[0] * (1 - w) + 255 * w) * v),
                       (uint8_t)((c[1] * (1 - w) + 255 * w) * v), (uint8_t)((c[2] * (1 - w) + 255 * w) * v));
    }
    // labels: text in the group colour, box in the "opposite" group's colour, dark
    const uint8_t* o = cl_schemes[scheme][(g + shift + 2) % CL_GROUPS];
    display.setColor(CL_LAB0 + g * 2, c[0] / 2 + 127, c[1] / 2 + 127, c[2] / 2 + 127);
    display.setColor(CL_LAB0 + g * 2 + 1, o[0] / 4, o[1] / 4, o[2] / 4);
  }
  display.setColor(CL_STAR, 255, 255, 255);
  display.setColor(CL_BEAM, 170, 170, 200);
  display.setColor(CL_PACKET, 255, 255, 230);
}

// ─── Tiny 3×5 font (A–Z, 0–9, _ .) ───────────────────────────────────
static const uint16_t cl_font[38] = {
  0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A,
  0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B73, 0x6BAD, 0x388E, 0x7492,
  0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x7B6F, 0x2C97, 0x62A7, 0x628E,
  0x5BC9, 0x798E, 0x39EF, 0x7292, 0x7BEF, 0x7BCE, 0x0007, 0x0002,
};

static uint8_t* cl_buf;

static inline void cl_put(int x, int y, uint8_t c) {
  if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) cl_buf[y * W + x] = c;
}
// Only paint if brighter (keeps dots over lines and labels over dots)
static inline void cl_max(int x, int y, uint8_t c) {
  if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) {
    uint8_t& p = cl_buf[y * W + x];
    if (p < c) p = c;
  }
}

// ─── Nonsense words ───────────────────────────────────────────────────
// Built from syllables: a starting sound, a vowel, sometimes an ending.
static const char* const cl_onset[] = {
  "B", "BR", "CH", "D", "DR", "F", "FL", "G", "GR", "GL", "K", "KR", "L", "M", "N", "P", "PL",
  "QU", "R", "S", "SH", "SK", "SL", "ST", "T", "TH", "TR", "V", "VR", "W", "X", "Z", "ZH", "", ""
};
static const char* const cl_vowel[] = {
  "A", "E", "I", "O", "U", "AI", "OU", "EE", "OO", "YA", "AE", "IO", "Y", "UA"
};
static const char* const cl_coda[] = {
  "", "", "", "", "N", "R", "X", "SK", "LT", "M", "TH", "NK", "Z", "B", "RN", "SH"
};
#define CL_NONSET (sizeof(cl_onset) / sizeof(cl_onset[0]))
#define CL_NVOWEL (sizeof(cl_vowel) / sizeof(cl_vowel[0]))
#define CL_NCODA  (sizeof(cl_coda) / sizeof(cl_coda[0]))

// Make a word (2–4 syllables) from the number h; returns its length
static int cl_sylMin = 2, cl_sylSpan = 3;          // syllables: min … min + span − 1

static int cl_word(uint32_t h, char* out, int maxLen) {
  int n = 0;
  int syl = cl_sylMin + (int)(h % cl_sylSpan);
  uint32_t g = h;
  for (int s = 0; s < syl; s++) {
    g = g * 1103515245u + 12345u;
    const char* on = cl_onset[(g >> 8) % CL_NONSET];
    const char* vo = cl_vowel[(g >> 16) % CL_NVOWEL];
    const char* co = (s == syl - 1 || ((g >> 24) & 3) == 0) ? cl_coda[(g >> 26) % CL_NCODA] : "";
    for (const char* p = on; *p && n < maxLen; p++) out[n++] = *p;
    for (const char* p = vo; *p && n < maxLen; p++) out[n++] = *p;
    for (const char* p = co; *p && n < maxLen; p++) out[n++] = *p;
  }
  // now and then a number tag, like a version or a node id
  if (((h >> 29) & 3) == 0 && n + 3 <= maxLen) {
    out[n++] = ((h >> 5) & 1) ? '_' : '.';
    out[n++] = (char)('0' + (h >> 6) % 10);
    out[n++] = (char)('0' + (h >> 10) % 10);
  }
  return n;
}

static inline int cl_glyph(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '0' && c <= '9') return 26 + (c - '0');
  return c == '_' ? 36 : 37;
}

static void cl_label(int x, int y, uint32_t h, int group, bool box) {
  char w[16];
  int len = cl_word(h, w, 14);
  if (x + len * 4 < 0 || x >= W || y + 6 < 0 || y - 1 >= H) return;
  if (box) {                                           // coloured highlight bar behind the text
    uint8_t bc = (uint8_t)(CL_LAB0 + group * 2 + 1);
    for (int yy = y - 1; yy <= y + 5; yy++)
      for (int xx = x - 1; xx < x + len * 4; xx++) cl_put(xx, yy, bc);
  }
  uint8_t tc = (uint8_t)(CL_LAB0 + group * 2);
  for (int i = 0; i < len; i++) {
    uint16_t bits = cl_font[cl_glyph(w[i])];
    for (int r = 0; r < 5; r++) {
      int row = (bits >> (12 - r * 3)) & 7;
      if (!row) continue;
      if (row & 4) cl_put(x + i * 4, y + r, tc);
      if (row & 2) cl_put(x + i * 4 + 1, y + r, tc);
      if (row & 1) cl_put(x + i * 4 + 2, y + r, tc);
    }
  }
}

// Faint line, max-blended so it never covers dots, packets or labels.
static void cl_line(int x0, int y0, int x1, int y1, uint8_t c) {
  if ((x0 < 0 && x1 < 0) || (x0 >= W && x1 >= W) || (y0 < 0 && y1 < 0) || (y0 >= H && y1 >= H)) return;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int g = 0; g < 700; g++) {
    if ((unsigned)x0 < (unsigned)W && (unsigned)y0 < (unsigned)H) {
      uint8_t& p = cl_buf[y0 * W + x0];
      if (p < c) p = c;
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// ─── Nodes (made fresh every frame from their number) ─────────────────
static uint16_t cl_seed = 1;

static inline uint32_t cl_hash(uint32_t a, uint32_t b) {
  uint32_t h = a * 2654435761u ^ (b + 0x9E3779B9u) * 2246822519u ^ (uint32_t)cl_seed * 3266489917u;
  h ^= h >> 15; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
  return h;
}
static inline uint32_t cl_mix(uint32_t h) { h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; return h; }
// Roughly bell-shaped number, −1535 … 1535 (sum of three random numbers)
static inline int cl_bell(uint32_t h) {
  return (int)((h & 1023) + ((h >> 10) & 1023) + ((h >> 20) & 1023)) - 1535;
}
static inline int32_t cl_isqrt(int32_t n) {
  if (n <= 0) return 0;
  int32_t x = (int32_t)sqrtf((float)n);
  while (x * x > n) x--;
  while ((x + 1) * (x + 1) <= n) x++;
  return x;
}

// The camera, worked out once per frame
static int cl_cA, cl_sA, cl_cB, cl_sB, cl_D, cl_F;
#define CL_Q 14

// World point → screen. Returns false if it's behind the camera.
static inline bool cl_project(int x, int y, int z, int& sx, int& sy, int& zc) {
  int x1 = (x * cl_cA - z * cl_sA) >> CL_Q;
  int z1 = (x * cl_sA + z * cl_cA) >> CL_Q;
  int y2 = (y * cl_cB - z1 * cl_sB) >> CL_Q;
  int z2 = (y * cl_sB + z1 * cl_cB) >> CL_Q;
  zc = z2 + cl_D;
  if (zc < 40) return false;
  sx = HALFW + x1 * cl_F / zc; sy = HALFH + y2 * cl_F / zc;
  return true;
}

static void cl_hubPos(int k, int spread, int& x, int& y, int& z) {
  uint32_t h = cl_hash(k + 5000, 7);
  x = (cl_bell(h) * spread) >> 10;
  y = (cl_bell(cl_mix(h)) * spread) >> 11;
  z = (cl_bell(cl_mix(h + 1)) * spread) >> 10;
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_cloud_name() { return "DATA CLOUD"; }
const char* prog_cloud_character() { return "A turning 3D cloud of glowing, labelled network nodes"; }

static const char* const cl_presetNames[] = {
  "Cloud", "Fly-through", "Clusters", "Globe", "Burst", "Galaxy", "Constellation"
};
#define CL_NUM_PRESETS 7

const char* prog_cloud_presetName(int preset) {
  if (preset >= 0 && preset < CL_NUM_PRESETS) return cl_presetNames[preset];
  return NULL;
}

static const char* const cl_labels[14] = {
  "Colors", "Speed", "Lines", "Labels", "Nodes", "Zoom", "Spread", "Tilt", "Fly", "Glow",
  "Beam", "Web", "Packets", "Rings"
};
static const char* const cl_extra[CL_NUM_PRESETS][2] = {
  {"Core", "Shimmer"}, {"Tunnel", "Warp"}, {"Groups", "Hub Links"}, {"Bands", "Wobble"},
  {"Rhythm", "Scatter"}, {"Arms", "Twist"}, {"Star Size", "Word Length"}
};
const char* prog_cloud_potLabel(int preset, int pot) {
  if (pot >= 0 && pot < 14) return cl_labels[pot];
  if ((pot == 14 || pot == 15) && preset >= 0 && preset < CL_NUM_PRESETS) return cl_extra[preset][pot - 14];
  return "";
}

uint8_t prog_cloud_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;
}

void prog_cloud_init() {
  cl_palKey = -1;
}

void prog_cloud_draw(int preset) {
  cl_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  cl_smoothKnobs(dt);

  // ── Knobs
  static const uint8_t order[5] = {0, 1, 2, 3, 4};
  int scheme = order[cl_pot(0, 0, 4)];
  float sk = cl_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.05f * powf(20.0f, (sk - 0.03f) / 0.97f);
  int lineLvl = (int)cl_potf(2, 0, 200);
  int labels = cl_pot(3, 0, 50);
  int n = cl_pot(4, 100, CL_MAXN);
  float kz = cl_sm[5] / 1023.0f;
  float zoom = (kz < 0.5f) ? powf(0.5f, (0.5f - kz) * 2.0f) : powf(3.0f, (kz - 0.5f) * 2.0f);
  int spread = (int)cl_potf(6, 350, 900);
  float tilt = cl_potf(7, -0.9f, 0.9f);
  float fly = cl_potf(8, 0.0f, 1.0f);
  int glowPct = cl_pot(9, 0, 40);
  float bk = cl_sm[10] / 1023.0f;
  bool beamOn = bk > 0.05f, scanOn = bk > 0.35f, radarOn = bk > 0.7f;
  int webPx = (int)cl_potf(11, 0, 55);                       // link neighbours closer than this (px)
  int packets = cl_pot(12, 0, 3);                            // 0 = none … 3 = lots
  int rings = cl_pot(13, 0, 3);
  float e1 = cl_sm[14] / 1023.0f, e2 = cl_sm[15] / 1023.0f;  // the two per-preset knobs, 0 … 1
  int e1Q = (int)(e1 * 256), e2Q = (int)(e2 * 256);
  cl_sylMin = 2; cl_sylSpan = 3;
  if (preset == 6) { cl_sylMin = 1 + (int)(e2 * 3.99f); cl_sylSpan = 2; }   // Word Length
  int groups = (preset == 2) ? 1 + (int)(e1 * 4.99f) : CL_GROUPS;           // Clusters: Groups
  int arms = 2 + (int)(e1 * 4.99f);                                          // Galaxy: Arms

  static float t = 0, yaw = 0, flyT = 0, flyZ = 0, burstT = 0;
  t += dt;
  yaw += dt * speed * 3.0f;
  flyT += dt * (0.15f + speed * 0.5f);
  flyZ += dt * (60 + 500 * speed + 300 * fly);
  burstT += dt * (0.5f + speed * 2.0f) * (preset == 4 ? 0.3f + e1 * 3.0f : 1.0f);   // Burst: Rhythm
  int shift = (scheme == 4) ? ((int)(t * 0.2f)) % CL_GROUPS : 0; // candy: colours rotate between groups
  cl_buildPalette(scheme, lineLvl, shift);

  // k12: new cloud
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) cl_seed++;
  k12Was = k12;

  if (preset == 6) { if (n > 200) n = 200; labels = labels * 2 + 10; }
  if (labels > n) labels = n;

  // ── Camera: turn (yaw), tilt, distance (swoops in and out)
  float pitch = tilt + (preset == 5 ? 0.55f : 0.0f);
  cl_cA = (int)(cosf(yaw) * (1 << CL_Q)); cl_sA = (int)(sinf(yaw) * (1 << CL_Q));
  cl_cB = (int)(cosf(pitch) * (1 << CL_Q)); cl_sB = (int)(sinf(pitch) * (1 << CL_Q));
  cl_D = (int)(1500 - fly * 1100 * (0.5f + 0.5f * sinf(flyT * 0.6f)));
  cl_F = (int)(270 * zoom);
  int camZ = (int)flyZ;
  int burstQ = (int)((0.25f + 0.75f * (0.5f - 0.5f * cosf(burstT))) * 256);
  int galA = (int)(yaw * 0.3f * 40.74f);                     // galaxy turn, in 1/256ths of a turn
  int pkT = (int)(t * (40 + speed * 200));                   // packet clock
  int tQ = (int)(t * 60);                                    // shimmer / wobble clock

  // ── Beam: a light axis through the cloud, pulses racing along it
  int yScan = (int)(sinf(t * (0.35f + speed * 1.5f)) * spread * 1.05f);   // scanner height (world)
  int scanBand = spread / 9;
  if (beamOn) {
    int ax0 = HALFW, ay0 = 0, ax1 = HALFW, ay1 = H - 1, zc;
    if (preset != 1) {                                       // the axis turns and tilts with the cloud
      int R = spread * 13 / 10, bx0, by0, bx1, by1;
      if (cl_project(0, -R, 0, bx0, by0, zc) && cl_project(0, R, 0, bx1, by1, zc)) {
        ax0 = bx0; ay0 = by0; ax1 = bx1; ay1 = by1;
      }
    }
    cl_line(ax0, ay0, ax1, ay1, CL_BEAM);
    // shimmer: a flickering second strand beside it
    int wob = (cl_sin(tQ * 5) >> 13);                        // −2 … 2
    if (wob != 0) cl_line(ax0 + wob, ay0, ax1 + wob, ay1, (uint8_t)(CL_LINE0 + 4 * CL_LINEN + 4));
    // pulses: bright blobs racing up and down, with short tails
    int nP = 1 + (int)(bk * 6);
    for (int k = 0; k < nP; k++) {
      int f = (pkT * 2 + k * 256 / nP) & 255;
      if (k & 1) f = 255 - f;                                // every other one goes the other way
      for (int tail = 0; tail < 4; tail++) {
        int ff = f + ((k & 1) ? tail * 3 : -tail * 3);
        if (ff < 0 || ff > 255) continue;
        int px = ax0 + (((ax1 - ax0) * ff) >> 8), py = ay0 + (((ay1 - ay0) * ff) >> 8);
        uint8_t c = tail == 0 ? (uint8_t)CL_PACKET : (uint8_t)CL_BEAM;
        cl_max(px, py, c);
        if (tail == 0) { cl_max(px + 1, py, c); cl_max(px - 1, py, c); cl_max(px, py + 1, c); cl_max(px, py - 1, c); }
      }
    }
  }
  // ── Scanner: a ring sweeping up and down the axis (fly-through: rings
  //    rushing outward from the middle), with radar arms spinning round it
  if (scanOn) {
    if (preset == 1) {
      for (int k = 0; k < 3; k++) {
        int R = ((pkT * 3 + k * 85) & 255) * 3 / 4 + 4;
        int px = 0, py = 0;
        for (int a = 0; a <= 48; a++) {
          int x = HALFW + ((cl_cos(a * 16 / 3) * R) >> CL_Q), y = HALFH + ((cl_sin(a * 16 / 3) * R * 3 / 4) >> CL_Q);
          if (a) cl_line(px, py, x, y, (uint8_t)(CL_LINE0 + ((k + 1) % CL_GROUPS) * CL_LINEN + 5));
          px = x; py = y;
        }
      }
    } else {
      int Rs = spread * 9 / 10;
      int px = 0, py = 0; bool have = false;
      for (int a = 0; a <= 48; a++) {
        int ang = a * 16 / 3;
        int x = (cl_cos(ang) * Rs) >> CL_Q, z = (cl_sin(ang) * Rs) >> CL_Q, sx, sy, zc;
        if (!cl_project(x, yScan, z, sx, sy, zc)) { have = false; continue; }
        if (have) cl_line(px, py, sx, sy, CL_BEAM);
        px = sx; py = sy; have = true;
      }
      if (radarOn) {                                         // radar arms
        int cx, cy, zc;
        if (cl_project(0, yScan, 0, cx, cy, zc)) {
          for (int k = 0; k < 3; k++) {
            int ang = tQ * 3 + k * 85;
            for (int s2 = 0; s2 < 3; s2++) {                 // each arm fans out a little
              int a2 = ang - s2 * 3;
              int x = (cl_cos(a2) * Rs) >> CL_Q, z = (cl_sin(a2) * Rs) >> CL_Q, sx, sy;
              if (cl_project(x, yScan, z, sx, sy, zc))
                cl_line(cx, cy, sx, sy, s2 == 0 ? (uint8_t)CL_BEAM : (uint8_t)(CL_LINE0 + ((k * 2) % CL_GROUPS) * CL_LINEN + 5 - s2));
            }
          }
        }
      }
    }
  }

  // ── Orbit rings round the cloud (group colours), tilted differently
  if (rings > 0 && preset != 1) {
    for (int r = 0; r < rings; r++) {
      int R = spread * (2 + r) / 3;
      int tiltA = 20 + r * 45;                               // ring tilt (1/256 turn)
      int px = 0, py = 0; bool have = false;
      for (int k = 0; k <= 64; k++) {
        int a = k * 4 + (int)(t * (8 + r * 5));
        int x = (cl_cos(a) * R) >> CL_Q, z = (cl_sin(a) * R) >> CL_Q;
        int y = (z * cl_sin(tiltA)) >> CL_Q; z = (z * cl_cos(tiltA)) >> CL_Q;
        int sx, sy, zc;
        if (!cl_project(x, y, z, sx, sy, zc)) { have = false; continue; }
        if (have) cl_line(px, py, sx, sy, (uint8_t)(CL_LINE0 + ((r * 2) % CL_GROUPS) * CL_LINEN + 2));   // dim, behind the nodes
        px = sx; py = sy; have = true;
      }
    }
  }

  // Hubs on screen (Clusters: each group's centre)
  int hubSX[CL_GROUPS], hubSY[CL_GROUPS];
  bool hubOK[CL_GROUPS];
  for (int k = 0; k < CL_GROUPS; k++) {
    int hx, hy, hz, zc;
    cl_hubPos(k, spread, hx, hy, hz);
    hubOK[k] = cl_project(hx, hy, hz, hubSX[k], hubSY[k], zc);
  }

  // Clusters: Hub Links — the hubs wired into a ring, with a halo round each
  if (preset == 2 && e2 > 0.05f) {
    int hl = (int)(e2 * 5.99f);
    for (int k = 0; k < groups; k++) {
      int k2 = (k + 1) % groups;
      uint8_t c = (uint8_t)(CL_LINE0 + k * CL_LINEN + CL_LINEN - 1);
      if (groups > 1 && hubOK[k] && hubOK[k2]) cl_line(hubSX[k], hubSY[k], hubSX[k2], hubSY[k2], c);
      if (hubOK[k] && hl > 1) {
        int R = 3 + hl * 3, px = 0, py = 0;
        for (int a = 0; a <= 32; a++) {
          int x = hubSX[k] + ((cl_cos(a * 8 + tQ) * R) >> CL_Q), y = hubSY[k] + ((cl_sin(a * 8 + tQ) * R) >> CL_Q);
          if (a) cl_line(px, py, x, y, c);
          px = x; py = y;
        }
      }
    }
  }

  // Last node drawn in each group (for the web of neighbour links)
  int lastX[CL_GROUPS], lastY[CL_GROUPS];
  for (int g = 0; g < CL_GROUPS; g++) lastX[g] = -9999;
  int drawnLines = 0;
  int web2 = webPx * webPx;

  // ── Every node, once: work out where it is, then wire it, dot it, label it
  for (int i = 0; i < n; i++) {
    uint32_t h = cl_hash(i, 1);
    uint32_t h2 = cl_mix(h ^ 0xA5A5A5A5u);
    int group = (int)(h2 % groups);
    int x, y, z;
    switch (preset) {
      case 1: {                                              // fly-through box
        x = (int)(h & 2047) - 1024;
        y = (((int)((h >> 11) & 2047) - 1024) * 3) >> 2;
        z = (int)((((h2 >> 4) & 4095) - camZ) & 4095) - 200; // wraps round: always ahead of you
        if (e1 > 0.03f) {                                    // Tunnel: push nodes out of the middle
          int Rt = (int)(e1 * 700);
          int r = (int)cl_isqrt(x * x + y * y);
          if (r < Rt) {
            if (r < 1) { x = Rt; y = 0; }
            else { x = x * Rt / r; y = y * Rt / r; }
          }
        }
        break;
      }
      case 2: {                                              // clusters round hubs
        int hx, hy, hz;
        cl_hubPos(group, spread, hx, hy, hz);
        x = hx + (cl_bell(h) * spread >> 12);
        y = hy + (cl_bell(cl_mix(h)) * spread >> 12);
        z = hz + (cl_bell(cl_mix(h + 7)) * spread >> 12);
        break;
      }
      case 3: {                                              // globe: point on a sphere
        int vx = cl_bell(h), vy = cl_bell(cl_mix(h)), vz = cl_bell(cl_mix(h + 7));
        int len = (int)cl_isqrt((int32_t)vx * vx + (int32_t)vy * vy + (int32_t)vz * vz);
        if (len < 1) len = 1;
        int R = spread * 3 / 4;
        x = vx * R / len; y = vy * R / len; z = vz * R / len;
        if (e2 > 0.03f) {                                    // Wobble: the sphere ripples
          int w = (e2Q * cl_sin(y * 384 / (R + 1) + tQ * 2)) >> 12;   // slow bulges, up to ±1/4 of the radius
          x += (x * w) >> 12; y += (y * w) >> 12; z += (z * w) >> 12;
        }
        if (e1 > 0.03f) {                                    // Bands: snap to rings of latitude
          int bands = 2 + (int)(e1 * 12);
          int lat = (y * bands + (y >= 0 ? R : -R)) / (2 * R);    // nearest band
          int ny = lat * 2 * R / bands;
          if (ny > R) ny = R; else if (ny < -R) ny = -R;
          int want = (int)cl_isqrt(R * R - ny * ny), have = (int)cl_isqrt(x * x + z * z);
          if (have > 0) { x = x * want / have; z = z * want / have; }
          y = ny;
        }
        break;
      }
      case 5: {                                              // galaxy: spiral arms in a flat disc
        int r = (int)((h & 1023) * (h & 1023) >> 10);         // more near the middle
        int arm = (int)((h2 >> 3) % arms);
        int twist = 32 + e2Q;                               // Twist: how tightly the arms curl
        int a = arm * 256 / arms + ((r * twist) >> 9) + (int)((h >> 12) & 31) - 16 + galA;
        int rr = r * spread >> 10;
        x = (cl_cos(a) * rr) >> CL_Q;
        z = (cl_sin(a) * rr) >> CL_Q;
        y = cl_bell(cl_mix(h)) * spread >> 14;
        break;
      }
      default: {                                             // cloud: bell-shaped, dense core
        x = cl_bell(h) * spread >> 10;
        y = cl_bell(cl_mix(h)) * spread >> 10;
        z = cl_bell(cl_mix(h + 7)) * spread >> 10;
        if ((int)((h2 >> 8) & 255) < 20 + (e1Q * 3 >> 2)) { x >>= 2; y >>= 2; z >>= 2; }   // Core: pulled into the centre
        if (e2 > 0.03f) {                                    // Shimmer: every node wobbles on its own
          int amp = e2Q >> 2;
          x += (cl_sin(tQ * 3 + (int)(h & 255)) * amp) >> CL_Q;
          y += (cl_sin(tQ * 4 + (int)((h >> 8) & 255)) * amp) >> CL_Q;
          z += (cl_sin(tQ * 5 + (int)((h >> 16) & 255)) * amp) >> CL_Q;
        }
        break;
      }
    }
    if (preset == 4) {                                       // burst: breathe in and out
      int sc = 16 + (e2Q >> 1);                              // Scatter: nodes out of step
      int nb = burstQ + (int)(((h2 >> 20) & 255) * sc >> 8) - (sc >> 1);
      if (nb < 16) nb = 16;
      x = x * nb >> 8; y = y * nb >> 8; z = z * nb >> 8;
    }
    // project
    int sx, sy, zc;
    if (preset == 1) {
      zc = z;
      if (zc < 40) continue;
      sx = HALFW + x * cl_F / zc; sy = HALFH + y * cl_F / zc;
    } else if (!cl_project(x, y, z, sx, sy, zc)) continue;
    bool onScreen = (unsigned)sx < (unsigned)W && (unsigned)sy < (unsigned)H;
    // how near: 0 (far) … CL_DOTN-1 (close)
    int near = (preset == 1) ? (CL_DOTN - 1) - zc * CL_DOTN / 3900 : (CL_DOTN - 1) - (zc - cl_D + 1200) * CL_DOTN / 2600;
    if (near < 0) near = 0; else if (near > CL_DOTN - 1) near = CL_DOTN - 1;
    uint8_t lineC = (uint8_t)(CL_LINE0 + group * CL_LINEN + (near * CL_LINEN) / CL_DOTN);

    // wires: to its hub (or the centre)
    if (lineLvl > 3 && onScreen && drawnLines < 450 && (int)(h2 & 255) < 60 + lineLvl) {
      int tx = HALFW, ty = HALFH;
      if (preset == 2 && hubOK[group]) { tx = hubSX[group]; ty = hubSY[group]; }
      if (preset == 1) { int wq = 8 + e2Q / 6; tx = sx + (((sx - HALFW) * wq) >> 6); ty = sy + (((sy - HALFH) * wq) >> 6); }   // Warp: streak length
      cl_line(sx, sy, tx, ty, lineC);
      drawnLines++;
      // a data packet racing along the wire
      if (packets > 0 && preset != 1 && (int)((h2 >> 16) & 7) < packets * 2) {
        int f = (int)((pkT + (h >> 3)) & 255);               // 0 … 255 along the wire
        int px = tx + (((sx - tx) * f) >> 8), py = ty + (((sy - ty) * f) >> 8);
        cl_max(px, py, CL_PACKET); cl_max(px + 1, py, CL_PACKET);
        cl_max(px, py + 1, CL_PACKET); cl_max(px + 1, py + 1, CL_PACKET);
      }
    }
    // web: link to the previous node of the same group if it's close on screen
    if (webPx > 2 && onScreen) {
      int lx = lastX[group];
      if (lx > -9999) {
        int dx = sx - lx, dy = sy - lastY[group];
        if (dx * dx + dy * dy < web2) cl_line(lx, lastY[group], sx, sy, lineC);
      }
      lastX[group] = sx; lastY[group] = sy;
    }
    if (!onScreen) continue;
    // the node (the scanner lights up the nodes it's passing through)
    bool scanned = scanOn && preset != 1 && abs(y - yScan) < scanBand;
    bool bright = scanned || (int)((h >> 24) % 100) < glowPct;
    uint8_t c = (uint8_t)(CL_DOT0 + group * CL_DOTN + near);
    cl_max(sx, sy, bright ? (uint8_t)CL_STAR : c);
    if (bright || near >= CL_DOTN - 2) {                     // a little cross for bright / close nodes
      cl_max(sx + 1, sy, c); cl_max(sx - 1, sy, c); cl_max(sx, sy + 1, c); cl_max(sx, sy - 1, c);
      if (bright && near > CL_DOTN / 2) {
        uint8_t d = (uint8_t)(CL_DOT0 + group * CL_DOTN + near / 2);
        int rays = (preset == 6) ? 2 + (int)(e1 * 6) : 2;    // Star Size: longer rays
        for (int k = 2; k <= rays; k++) {
          cl_max(sx + k, sy, d); cl_max(sx - k, sy, d); cl_max(sx, sy + k, d); cl_max(sx, sy - k, d);
        }
        if (preset == 6 && rays > 3) {                        // and diagonal sparkle
          cl_max(sx + 1, sy + 1, d); cl_max(sx - 1, sy - 1, d); cl_max(sx + 1, sy - 1, d); cl_max(sx - 1, sy + 1, d);
        }
      }
    }
    if ((i < labels && near > 1) || (scanned && (h & 7) == 0))
      cl_label(sx + 3, sy - 2, cl_hash(i, 99), group, scanned || ((h2 >> 24) & 3) == 0);
  }
}

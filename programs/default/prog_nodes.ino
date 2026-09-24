// =====================================================================
// PROGRAM: NODES (by Dewey)
// Soft white nodes drifting over a dotted grid, linked to their
// neighbours by thin grey lines, with a smooth rainbow curve threading
// through all of them. Approximate take on polyhop's "Nested" series.
//
// Presets:
//   k0 Constellation — nodes wander; the curve threads through them
//   k1 Loop          — the curve closes into a loop
//   k2 Nested        — three copies inside each other, turning
//   k3 Orbits        — nodes circle the centre at different speeds
//   k4 Web           — more nodes, every nearby pair linked
//   k5 Breakdown     — calm at first, then it gets wilder and wilder
//                      until it "breaks" … and starts again
//   k6 Ribbon        — three curves side by side, like a ribbon
//
//   k12 — press for a new arrangement
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Colors (neon, fire, rainbow [centre], ocean, white)
//   p1 Speed (far left = stop)   p2 Links (grey line brightness)
//   p3 Grid (dot brightness)
//   p4 Nodes (how many)   p5 Node Size   p6 Curvy (curve tension)
//   p7 Neighbours (links per node)   p8 Color Flow (colours travel
//   along the curve)   p9 Zoom   p10 Thickness   p11 Wander
// =====================================================================

#define ND_MAXN   16
#define ND_GRID   1          // palette: grid dots
#define ND_LINK   2          // grey links
#define ND_NODE   3          // node white
#define ND_NODE2  4          // node shade
#define ND_NODE3  5          // node edge
#define ND_HUE0   16         // 16–255: the curve's colours
#define ND_HUEN   240

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float nd_sm[16];
static bool  nd_smReady = false;

static void nd_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!nd_smReady) nd_sm[i] = pots[i];
    else nd_sm[i] += (pots[i] - nd_sm[i]) * a;
  }
  nd_smReady = true;
}
static inline float nd_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (nd_sm[idx] / 1023.0f); }
static inline int nd_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (nd_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colours ──────────────────────────────────────────────────────────
static int nd_palKey = -1;

static void nd_hue(float h, float* rgb) {
  h -= floorf(h);
  float x = h * 6.0f; int k = (int)x; float q = x - k;
  switch (k) {
    case 0:  rgb[0] = 1; rgb[1] = q; rgb[2] = 0; break;
    case 1:  rgb[0] = 1 - q; rgb[1] = 1; rgb[2] = 0; break;
    case 2:  rgb[0] = 0; rgb[1] = 1; rgb[2] = q; break;
    case 3:  rgb[0] = 0; rgb[1] = 1 - q; rgb[2] = 1; break;
    case 4:  rgb[0] = q; rgb[1] = 0; rgb[2] = 1; break;
    default: rgb[0] = 1; rgb[1] = 0; rgb[2] = 1 - q; break;
  }
}

static void nd_buildPalette(int scheme, int linkLvl, int gridLvl, int flow) {
  int key = ((scheme * 64 + linkLvl / 4) * 64 + gridLvl / 4) * 256 + flow;
  if (key == nd_palKey) return;
  nd_palKey = key;
  display.setColor(0, 0, 0, 0);
  display.setColor(ND_GRID, gridLvl, gridLvl, gridLvl);
  display.setColor(ND_LINK, linkLvl, linkLvl, linkLvl);
  display.setColor(ND_NODE, 245, 245, 245);
  display.setColor(ND_NODE2, 200, 200, 205);
  display.setColor(ND_NODE3, 120, 120, 125);
  for (int i = 6; i < ND_HUE0; i++) display.setColor(i, 0, 0, 0);
  float off = flow / 256.0f;
  for (int i = 0; i < ND_HUEN; i++) {
    float t = i / (float)ND_HUEN + off;        // position along the curve (wraps)
    t -= floorf(t);
    float c[3];
    switch (scheme == 0 ? 3 : scheme == 2 ? 0 : scheme) {                  // centre of the knob = rainbow
      case 0: nd_hue(t, c); break;                                          // rainbow
      case 1: { float u = 0.5f + 0.5f * sinf(t * TWO_PI);                    // fire
                c[0] = 1; c[1] = 0.15f + 0.7f * u; c[2] = 0.1f * u; break; }
      case 2: { float u = 0.5f + 0.5f * sinf(t * TWO_PI);                    // ocean
                c[0] = 0.1f * u; c[1] = 0.4f + 0.5f * u; c[2] = 1; break; }
      case 3: { float u = 0.5f + 0.5f * sinf(t * TWO_PI);                    // neon pink ↔ cyan
                c[0] = 1 - 0.8f * u; c[1] = 0.2f + 0.8f * u; c[2] = 1; break; }
      default: { float u = 0.75f + 0.25f * sinf(t * TWO_PI);                 // white
                 c[0] = c[1] = c[2] = u; break; }
    }
    display.setColor(ND_HUE0 + i, (uint8_t)(c[0] * 255), (uint8_t)(c[1] * 255), (uint8_t)(c[2] * 255));
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* nd_buf;

static inline void nd_px(int x, int y, uint8_t c) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  nd_buf[y * W + x] = c;
}

// Thin grey link, lightly dotted for texture
static void nd_link(int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int g = 0; g < 900; g++) {
    if ((g % 5) != 4) nd_px(x0, y0, ND_LINK);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// Curve segment with a colour and thickness 1–3
static void nd_seg(int x0, int y0, int x1, int y1, uint8_t c, int th) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  bool steep = -dy > dx;
  for (int g = 0; g < 900; g++) {
    nd_px(x0, y0, c);
    if (th >= 2) { if (steep) nd_px(x0 + 1, y0, c); else nd_px(x0, y0 + 1, c); }
    if (th >= 3) { if (steep) nd_px(x0 - 1, y0, c); else nd_px(x0, y0 - 1, c); }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// A soft, slightly grainy white node
static void nd_node(float fx, float fy, float r, int seed) {
  int cx = (int)fx, cy = (int)fy, R = (int)(r + 1);
  float r2 = r * r;
  for (int y = -R; y <= R; y++)
    for (int x = -R; x <= R; x++) {
      float d2 = (float)(x * x + y * y);
      if (d2 > r2) continue;
      uint32_t h = (uint32_t)((x + 17) * 73 + (y + 31) * 151 + seed * 97);
      h ^= h >> 3; h *= 0x9E3779B1u; h >>= 28;
      uint8_t c = ND_NODE;
      if (d2 > r2 * 0.72f) c = (h & 1) ? ND_NODE3 : ND_NODE2;        // soft edge
      else if ((h & 7) == 0) c = ND_NODE2;                            // grain
      nd_px(cx + x, cy + y, c);
    }
}

// ─── Nodes ────────────────────────────────────────────────────────────
static float nd_hx[ND_MAXN], nd_hy[ND_MAXN];    // home positions (−1 … 1)
static float nd_x[ND_MAXN], nd_y[ND_MAXN];      // current screen positions
static int   nd_n = 9;
static uint8_t nd_ord[ND_MAXN];                 // path order for the curve
static int   nd_ordN = -1;                      // (worked out for this many nodes)
static uint16_t nd_seed = 1;

static float nd_rnd(int i, int k) {             // repeatable 0–1 from the seed
  uint32_t h = (uint32_t)(i * 2654435761u) ^ (uint32_t)(k * 40503u) ^ ((uint32_t)nd_seed * 2246822519u);
  h ^= h >> 15; h *= 0x85EBCA6Bu; h ^= h >> 13;
  return (h & 0xFFFF) / 65535.0f;
}

static void nd_makeHomes() {
  for (int i = 0; i < ND_MAXN; i++) {
    // spread nodes out: try a few spots, keep the one farthest from the others
    float bx = 0, by = 0, best = -1;
    for (int tr = 0; tr < 8; tr++) {
      float x = nd_rnd(i, tr * 2) * 2 - 1, y = nd_rnd(i, tr * 2 + 1) * 2 - 1;
      float dmin = 9;
      for (int j = 0; j < i; j++) {
        float dx = x - nd_hx[j], dy = y - nd_hy[j];
        float d = dx * dx + dy * dy;
        if (d < dmin) dmin = d;
      }
      if (dmin > best) { best = dmin; bx = x; by = y; }
    }
    nd_hx[i] = bx; nd_hy[i] = by;
  }
  nd_ordN = -1;
}

// Visit the nodes in a sensible order (start at the leftmost, then always
// hop to the nearest unvisited one) so the curve flows instead of zig-zagging.
static void nd_makeOrder(int n) {
  if (n == nd_ordN) return;
  nd_ordN = n;
  bool used[ND_MAXN] = {false};
  int cur = 0;
  for (int i = 1; i < n; i++) if (nd_hx[i] < nd_hx[cur]) cur = i;
  for (int k = 0; k < n; k++) {
    nd_ord[k] = (uint8_t)cur; used[cur] = true;
    float bd = 1e9f; int bi = -1;
    for (int j = 0; j < n; j++) {
      if (used[j]) continue;
      float dx = nd_hx[j] - nd_hx[cur], dy = nd_hy[j] - nd_hy[cur], d = dx * dx + dy * dy;
      if (d < bd) { bd = d; bi = j; }
    }
    if (bi < 0) break;
    cur = bi;
  }
}

// Cardinal spline point between p1 and p2 (p0, p3 are neighbours)
static inline float nd_card(float p0, float p1, float p2, float p3, float t, float k) {
  float m1 = k * (p2 - p0), m2 = k * (p3 - p1);
  float t2 = t * t, t3 = t2 * t;
  return (2 * t3 - 3 * t2 + 1) * p1 + (t3 - 2 * t2 + t) * m1 + (-2 * t3 + 3 * t2) * p2 + (t3 - t2) * m2;
}

// Draw the curve through the given points (indices into xs/ys)
static void nd_curve(const float* xs, const float* ys, int n, bool closed, float k, int th,
                     float offX, float offY, int hueShift) {
  if (n < 2) return;
  int spans = closed ? n : n - 1;
  const int STEPS = 14;
  int total = spans * STEPS, step = 0;
  int px = 0, py = 0;
  for (int s = 0; s < spans; s++) {
    int i0 = s - 1, i1 = s, i2 = s + 1, i3 = s + 2;
    if (closed) { i0 = (i0 + n) % n; i2 %= n; i3 %= n; }
    else { if (i0 < 0) i0 = 0; if (i3 > n - 1) i3 = n - 1; }
    for (int q = (s == 0 ? 0 : 1); q <= STEPS; q++) {
      float t = q / (float)STEPS;
      int x = (int)(nd_card(xs[i0], xs[i1], xs[i2], xs[i3], t, k) + offX);
      int y = (int)(nd_card(ys[i0], ys[i1], ys[i2], ys[i3], t, k) + offY);
      int ci = (step * ND_HUEN / (total + 1) + hueShift) % ND_HUEN;
      if (step > 0) nd_seg(px, py, x, y, (uint8_t)(ND_HUE0 + ci), th);
      px = x; py = y; step++;
    }
  }
}

// Link each node to its nearest `links` neighbours
static void nd_links(const float* xs, const float* ys, int n, int links, float maxD) {
  if (links <= 0) return;
  for (int i = 0; i < n; i++) {
    int best[4] = {-1, -1, -1, -1};
    float bd[4] = {1e9f, 1e9f, 1e9f, 1e9f};
    for (int j = 0; j < n; j++) {
      if (j == i) continue;
      float dx = xs[j] - xs[i], dy = ys[j] - ys[i], d = dx * dx + dy * dy;
      for (int m = 0; m < links; m++)
        if (d < bd[m]) {
          for (int q = links - 1; q > m; q--) { bd[q] = bd[q - 1]; best[q] = best[q - 1]; }
          bd[m] = d; best[m] = j; break;
        }
    }
    for (int m = 0; m < links; m++) {
      int j = best[m];
      if (j < 0) continue;
      if (bd[m] > maxD * maxD) continue;
      nd_link((int)xs[i], (int)ys[i], (int)xs[j], (int)ys[j]);
    }
  }
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_nodes_name() { return "NODES"; }
const char* prog_nodes_character() { return "Drifting nodes, grey links and a rainbow curve through them all"; }

static const char* const nd_presetNames[] = {
  "Constellation", "Loop", "Nested", "Orbits", "Web", "Breakdown", "Ribbon"
};
#define ND_NUM_PRESETS 7

const char* prog_nodes_presetName(int preset) {
  if (preset >= 0 && preset < ND_NUM_PRESETS) return nd_presetNames[preset];
  return NULL;
}

static const char* const nd_labels[12] = {
  "Colors", "Speed", "Links", "Grid", "Nodes", "Node Size", "Curvy", "Neighbours",
  "Color Flow", "Zoom", "Thickness", "Wander"
};
const char* prog_nodes_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 12) return nd_labels[pot];
  return "";
}

uint8_t prog_nodes_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;
}

void prog_nodes_init() {
  nd_palKey = -1;
  nd_makeHomes();
}

void prog_nodes_draw(int preset) {
  nd_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  nd_smoothKnobs(dt);

  // Knobs
  int scheme = nd_pot(0, 0, 4);
  float sk = nd_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.1f * powf(30.0f, (sk - 0.03f) / 0.97f);
  int linkLvl = (int)nd_potf(2, 0, 200);
  int gridLvl = (int)nd_potf(3, 0, 110);
  int count = nd_pot(4, 4, ND_MAXN);
  float nodeR = nd_potf(5, 2.0f, 11.0f);
  float curvy = nd_potf(6, 0.0f, 1.0f);          // 0 = straight lines, 0.5 = smooth, 1 = loopy
  int links = nd_pot(7, 0, 4);
  float flowRate = nd_potf(8, 0.0f, 0.4f);
  float kz = nd_sm[9] / 1023.0f;
  float zoom = (kz < 0.5f) ? powf(0.5f, (0.5f - kz) * 2.0f) : powf(2.5f, (kz - 0.5f) * 2.0f);
  int thick = nd_pot(10, 1, 3);
  float wander = nd_potf(11, 0.0f, 0.45f);

  static float t = 0, flow = 0;
  t += dt * speed;
  flow += dt * flowRate;
  flow -= floorf(flow);
  nd_buildPalette(scheme, linkLvl, gridLvl, (int)(flow * 256) & 255);

  // k12: new arrangement
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) { nd_seed++; nd_makeHomes(); }
  k12Was = k12;

  // Breakdown: a slowly rising "chaos" level that resets
  static float chaos = 0;
  if (preset == 5) {
    chaos += dt * speed * 0.06f;
    if (chaos > 1.25f) { chaos = 0; nd_seed++; nd_makeHomes(); }
  } else chaos = 0;

  // Grid of faint dots
  if (gridLvl > 2) {
    int gs = (int)(10 * zoom); if (gs < 4) gs = 4;
    int ox = (int)(HALFW) % gs, oy = (int)(HALFH) % gs;
    for (int y = oy; y < H; y += gs)
      for (int x = ox; x < W; x += gs) nd_buf[y * W + x] = ND_GRID;
  }

  // Where is everything this frame?
  float sx = HALFW * 0.85f * zoom, sy = HALFH * 0.85f * zoom;
  int n = (preset == 4) ? ND_MAXN : count;
  float k = curvy;
  for (int i = 0; i < n; i++) {
    float fx = 0.3f + 0.7f * nd_rnd(i, 50), fy = 0.3f + 0.7f * nd_rnd(i, 51);
    float px = nd_rnd(i, 52) * TWO_PI, py = nd_rnd(i, 53) * TWO_PI;
    float x, y;
    if (preset == 1) {                                     // Loop: around a ring
      float a = i * TWO_PI / n + t * 0.15f;
      float r = 0.65f + 0.2f * sinf(t * fx + px);
      x = cosf(a) * r; y = sinf(a) * r;
    } else if (preset == 3) {                              // Orbits
      float r = 0.2f + 0.8f * (i + 1) / n;
      float a = t * (0.9f / (0.4f + r)) * (i & 1 ? -1.0f : 1.0f) + px;
      x = cosf(a) * r; y = sinf(a) * r * 0.9f;
    } else {
      x = nd_hx[i]; y = nd_hy[i];
    }
    float w = wander * (1.0f + chaos * 3.0f);
    x += w * sinf(t * fx + px) + chaos * chaos * 0.3f * sinf(t * 7.0f * fy + py);
    y += w * sinf(t * fy + py) + chaos * chaos * 0.3f * cosf(t * 6.0f * fx + px);
    nd_x[i] = HALFW + x * sx; nd_y[i] = HALFH + y * sy;
  }
  if (preset == 5) k = curvy + chaos * chaos * 2.5f;       // the curve goes wild
  if (preset != 1 && preset != 3) {                         // put the nodes in path order
    nd_makeOrder(n);
    static float tx[ND_MAXN], ty[ND_MAXN];
    for (int i = 0; i < n; i++) { tx[i] = nd_x[nd_ord[i]]; ty[i] = nd_y[nd_ord[i]]; }
    for (int i = 0; i < n; i++) { nd_x[i] = tx[i]; nd_y[i] = ty[i]; }
  }

  int hueShift = 0;
  float nodeScale = zoom < 1.0f ? zoom : 1.0f;

  if (preset == 2) {
    // Nested: three copies around the centre, each smaller and turning
    static float nx[ND_MAXN], ny[ND_MAXN];
    for (int c = 0; c < 3; c++) {
      float s = 1.0f - c * 0.3f, a = t * 0.1f * (c & 1 ? -1.0f : 1.0f) * (c + 1);
      float ca = cosf(a) * s, sa = sinf(a) * s;
      for (int i = 0; i < n; i++) {
        float dx = nd_x[i] - HALFW, dy = nd_y[i] - HALFH;
        nx[i] = HALFW + dx * ca - dy * sa; ny[i] = HALFH + dx * sa + dy * ca;
      }
      nd_links(nx, ny, n, links, 1e9f);
      nd_curve(nx, ny, n, false, k, thick, 0, 0, c * ND_HUEN / 3);
      for (int i = 0; i < n; i++) nd_node(nx[i], ny[i], nodeR * s * nodeScale, i + c * 20);
    }
    return;
  }

  if (preset == 4) nd_links(nd_x, nd_y, n, links > 0 ? 4 : 0, W * 0.3f * zoom);
  else nd_links(nd_x, nd_y, n, links, 1e9f);

  if (preset == 6) {                                        // Ribbon: three side-by-side curves
    float off = 4.0f + nodeR * 0.6f;
    nd_curve(nd_x, nd_y, n, false, k, thick, -off, -off, 0);
    nd_curve(nd_x, nd_y, n, false, k, thick, 0, 0, ND_HUEN / 3);
    nd_curve(nd_x, nd_y, n, false, k, thick, off, off, 2 * ND_HUEN / 3);
  } else {
    nd_curve(nd_x, nd_y, n, preset == 1, k, thick, 0, 0, hueShift);
  }
  for (int i = 0; i < n; i++) nd_node(nd_x[i], nd_y[i], nodeR * nodeScale, i);
}

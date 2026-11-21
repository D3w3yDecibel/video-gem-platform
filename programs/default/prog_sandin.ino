// =====================================================================
// PROGRAM: SANDIN — the Image Processor (by Dewey) — Set 7, slot 0
//
// In 1973 Dan Sandin built the Image Processor in Chicago: a patchable
// analog computer for video, "the Moog of video". Oscillators make ramps
// and waves across the screen and down it; they are added, subtracted,
// multiplied and compared, and a colourizer turns the result into bands
// of colour. Every picture here is just two oscillators — one across, one
// down — through that colourizer, the way the IP's patterns were made.
//
// How it works: for each screen line, the 256 possible values of the
// across-oscillator are combined with that line's down-oscillator value
// and run through the colourizer, giving a 256-entry colour table; each
// pixel is then one table lookup at its oscillator phase. Bending shifts
// each line's phase (the bands curve); Fan changes each line's frequency.
//
// Presets: k0 Chicago · k1 Plaid · k2 Circuit · k3 Diamonds · k4 Venetian ·
//          k5 Bend · k6 Moiré · k7 Comparator · k8 Lattice · k9 Ring Mod ·
//          k10 Spectre · k11 Tour
//   k12 — PATCH: re-patch the oscillators (the next pair of waveforms)
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Across Freq · p6 Down Freq · p7 Bend · p8 Bands · p9 Edge (◄ soft | hard ►)
//   p10 Mix (Add, Difference, Ring, XOR, Max) · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { SD_ADD, SD_DIFF, SD_RING, SD_XOR, SD_MAX, SD_NOPS };
struct SdLook { uint8_t hw, vw, op; uint8_t hf, vf; int8_t hdrift, vdrift; uint8_t bend, fan, bands, edge, cdrift, pal, flow, spread, scan; };
static const SdLook sd_looks[11] = {
  // hw         vw          op       hf  vf  hdr vdr bend fan bands edge cdr pal              flow spr scan
  { V7W_SAW,    V7W_SAW,    SD_ADD,  20, 14,  6, -4,  10,  0,  60,  20, 30, V7P_CHICAGO,     6,  70, 3 },  // Chicago
  { V7W_TRI,    V7W_TRI,    SD_RING, 55, 40,  5,  3,   0,  0,  40,  50, 20, V7P_PAIK,        4,  60, 3 },  // Plaid
  { V7W_SAW,    V7W_SAW,    SD_XOR,  30, 22,  8, -6,   0,  0,  70,  90, 10, V7P_BLACKLIGHT,  5,  80, 2 },  // Circuit
  { V7W_TRI,    V7W_TRI,    SD_ADD,  40, 30, -5,  4,   0,  0,  50,  30, 40, V7P_SUNSET,      5,  70, 3 },  // Diamonds
  { V7W_SQUARE, V7W_SINE,   SD_MAX,  90, 20,  3,  6,  30,  0,  40,  70, 20, V7P_STUDIO,      3,  50, 4 },  // Venetian
  { V7W_SINE,   V7W_SINE,   SD_ADD,  25, 18,  4, -3,  70, 35,  50,  25, 30, V7P_VASULKA,     5,  60, 3 },  // Bend
  { V7W_SINE,   V7W_SINE,   SD_DIFF, 60, 46,  6, -5,  10,  0,  40,  80, 10, V7P_GHOST,       3,  40, 2 },  // Moiré
  { V7W_SINE,   V7W_TRI,    SD_ADD,  22, 16,  5,  5,  20,  0,  25, 100, 25, V7P_AMBER,       4,  60, 4 },  // Comparator
  { V7W_TRI,    V7W_TRI,    SD_MAX,  50, 38, -4,  6,   0, 20,  45,  95, 30, V7P_SCANIMATE,   6,  70, 3 },  // Lattice
  { V7W_SINE,   V7W_SINE,   SD_RING, 48, 34,  7, -4,  35,  0,  45,  40, 20, V7P_CHICAGO,     6,  90, 3 },  // Ring Mod
  { V7W_TRI,    V7W_SINE,   SD_XOR,  24, 16, -6,  3,  25, 25,  36,  60, 35, V7P_KODACHROME,  5,  80, 3 },  // Spectre
};
static const char* const sd_names[12] = { "Chicago", "Plaid", "Circuit", "Diamonds", "Venetian", "Bend", "Moire", "Comparator", "Lattice", "Ring Mod", "Spectre", "Tour" };
static const char* const sd_lbl[6] = { "Across Freq", "Down Freq", "Bend", "Bands", "Edge: Soft|Hard", "Mix: Add|Diff|Ring|XOR|Max" };
const char* prog_sandin_name() { return "SANDIN"; }
const char* prog_sandin_character() { return "Dan Sandin's Image Processor: two oscillators through a colourizer"; }
const char* prog_sandin_presetName(int p) { return (p >= 0 && p < 12) ? sd_names[p] : NULL; }
const char* prog_sandin_potLabel(int p, int pot) { (void)p; return v7_label(pot, sd_lbl); }
uint8_t prog_sandin_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the colourizer (512), the across-wave (256), one line's colour table (256)
static uint8_t* sd_col; static int8_t* sd_hw; static uint8_t* sd_row;
static void sd_mem() { sd_col = v6_mem; sd_hw = (int8_t*)(v6_mem + 512); sd_row = v6_mem + 768; }

static float sd_hph = 0, sd_vph = 0, sd_bph = 0, sd_cph = 0;
static float sd_hf, sd_vf, sd_bend, sd_bands, sd_edge, sd_fan;
static int sd_patch = 0, sd_lastLook = -1;
static const uint8_t sd_patches[8][2] = { {0,0}, {1,1}, {2,2}, {3,0}, {1,2}, {0,3}, {2,1}, {4,1} };

void prog_sandin_init() { sd_mem(); v6_init(); sd_patch = 0; sd_lastLook = -1; }

// one line: the 256 across-values combined with this line's down-value, through the colourizer
__attribute__((optimize("O2"))) static void sd_rowTable(int op, int vv) {
  const int8_t* hw = sd_hw; const uint8_t* col = sd_col + 256; uint8_t* row = sd_row;
  switch (op) {
    case SD_ADD:  for (int i = 0; i < 256; i++) row[i] = col[hw[i] + vv]; break;
    case SD_DIFF: for (int i = 0; i < 256; i++) { int d = hw[i] - vv; if (d < 0) d = -d; row[i] = col[2 * d - 255]; } break;
    case SD_RING: for (int i = 0; i < 256; i++) row[i] = col[(hw[i] * vv) >> 6]; break;
    case SD_XOR:  for (int i = 0; i < 256; i++) row[i] = col[(((hw[i] + 128) ^ (vv + 128)) << 1) - 255]; break;
    default:      for (int i = 0; i < 256; i++) { int m = hw[i] > vv ? hw[i] : vv; row[i] = col[2 * m]; } break;
  }
}
// one line of pixels: phase p (8.8 table steps … in 1/256), step dp per pixel
__attribute__((optimize("O2"))) static void sd_line(uint8_t* out, uint32_t p, uint32_t dp) {
  const uint8_t* row = sd_row;
  for (int x = 0; x < W; x += 4) {
    uint32_t a = row[(p >> 8) & 255]; p += dp;
    uint32_t b = row[(p >> 8) & 255]; p += dp;
    uint32_t c = row[(p >> 8) & 255]; p += dp;
    uint32_t d = row[(p >> 8) & 255]; p += dp;
    *(uint32_t*)(out + x) = a | (b << 8) | (c << 16) | (d << 24);
  }
}

void prog_sandin_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  sd_mem();
  const SdLook& L = sd_looks[look];
  if (look != sd_lastLook) { sd_lastLook = look; sd_patch = 0; }
  if (v6_k12hit) sd_patch = (sd_patch + 1) % 8;
  int hwShape = sd_patch ? sd_patches[sd_patch][0] : L.hw, vwShape = sd_patch ? sd_patches[sd_patch][1] : L.vw;

  float rate = v6_fade < 1.0f ? 1.5f : 5.0f;
  float z = v6_zoom(1.0f);
  v6_ease(&sd_hf, (v3_centre(5) ? L.hf / 10.0f : 0.5f * powf(48.0f, v3_u(5))) / z, rate);
  v6_ease(&sd_vf, (v3_centre(6) ? L.vf / 10.0f : 0.5f * powf(36.0f, v3_u(6))) / z, rate);
  v6_ease(&sd_bend, v7_kn(7, L.bend / 100.0f), rate);
  v6_ease(&sd_bands, v3_centre(8) ? L.bands / 10.0f : 1.0f + v3_u(8) * 15.0f, rate);
  v6_ease(&sd_edge, v7_kn(9, L.edge / 100.0f), rate);
  v6_ease(&sd_fan, L.fan / 100.0f, rate);
  int op = v3_centre(10) ? L.op : v3_clamp(v3_k(10) * SD_NOPS / 1024, 0, SD_NOPS - 1);
  v7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 90);

  float sdt = v6_f.sdt;
  sd_hph += sdt * L.hdrift * 0.05f; sd_vph += sdt * L.vdrift * 0.05f;
  sd_bph += sdt * 0.23f; sd_cph += sdt * L.cdrift * 0.01f;
  sd_hph -= floorf(sd_hph); sd_vph -= floorf(sd_vph); sd_bph -= floorf(sd_bph);
  if (sd_cph >= 64.0f) sd_cph -= 64.0f;                                 // the band colours repeat every 64 bands

  // ── the colourizer: value −256 … 255 → bands of colour ──
  {
    int bandsK = (int)(sd_bands * 256);                              // bands across the whole range, ×256
    int gain = 256 + (int)(sd_edge * sd_edge * 3800);                 // soft → hard edges
    int32_t off = (int32_t)(sd_cph * 65536.0f);                       // the colourizer turning, in bands ×65536
    for (int i = 0; i < 512; i++) {
      int32_t t = (int32_t)i * bandsK / 2 + off;                       // position in bands, ×65536
      int b = (t >> 16) & 63, f = (t >> 8) & 255;                     // band number, fraction 0 … 255
      int tri = f < 128 ? f * 2 : (255 - f) * 2;                      // 0 at the band edges, 255 in the middle
      int lev = 8 + (((tri - 128) * 7 * gain) >> 15);
      lev = v3_clamp(lev, 0, 15);
      if (sd_edge > 0.6f && lev < 3) lev = 0;                        // hard key: black between bands
      sd_col[i] = V6C(lev, (b * 5 + (b >> 2)) & 15);
    }
  }
  // ── the across-oscillator, once per frame as a 256-step table ──
  for (int i = 0; i < 256; i++) sd_hw[i] = (int8_t)v7_wave(hwShape, (uint32_t)i << 8);
  // ── each line ──
  float hcyc = sd_hf;                                                  // cycles across the screen
  int32_t dp0 = (int32_t)(hcyc * 65536.0f / W);                        // phase per pixel (65536 = one cycle)
  uint32_t hp0 = (uint32_t)(sd_hph * 65536.0f) - (uint32_t)(dp0 * (W / 2));
  uint32_t vstep = (uint32_t)(sd_vf * 65536.0f / H), vp = (uint32_t)(sd_vph * 65536.0f) - vstep * (H / 2);
  int bendAmp = (int)(sd_bend * 65536.0f * 0.5f);                       // up to half a cycle of bending
  int fanK = (int)(sd_fan * 16384);
  uint32_t bp = (uint32_t)(sd_bph * 65536.0f);
  for (int y = 0; y < H; y++, vp += vstep) {
    int vv = v7_wave(vwShape, vp >> 0);
    sd_rowTable(op, vv);
    int32_t shift = (int32_t)(((int64_t)v3_sin((int)((bp >> 6) + y * 3)) * bendAmp) >> 14);
    int32_t dp = dp0;
    if (fanK) dp = dp0 + (int32_t)(((int64_t)dp0 * ((v3_sin(y * 2 + (int)(bp >> 7)) * fanK) >> 14)) >> 14);
    uint32_t p = hp0 + (uint32_t)shift - (uint32_t)((dp - dp0) * (W / 2));
    sd_line(v3_buf + y * W, p, (uint32_t)dp);
  }
  v7_end(L.scan);
}

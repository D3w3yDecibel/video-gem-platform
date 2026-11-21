// =====================================================================
// PROGRAM REGISTRY — SET 8 "GREATEST HITS: CHILL EDITION" (Dewey)
//
// The favourite looks of Sets 1-7, rebuilt slow, smooth and laid back for
// mixing (made to run into a 2-channel mixer next to a Recursion Studio 4K),
// plus a meme program and two new ones:
//
//   slot 0  LAVA      — lava lamp & liquid light            (Set 1 Liquid Light, Set 3 LIQUID)
//   slot 1  VEILS     — aurora curtains & Lumia veils       (Set 7 LUMIA, Set 2/4 auroras)
//   slot 2  QUASI     — quasicrystal waves                  (Set 6 QUASI, Set 3 MOIRE)
//   slot 3  BLACK HOLE — gravitational lens, accretion disk, jets, wormhole, binary merger (new in v4;
//                        replaced KALEIDO — prog_ghkaleido.ino is still in the folder, unlisted)
//   slot 4  HYPNO     — spirals, tunnels, Julia dream       (Set 1 Hypnotic, Set 3 WORMHOLE)
//   slot 5  ASCII     — a rocket launch to the Milky Way, a jellyfish, neon glyphs — in letters (new in v6;
//                       replaced RIBBONS — prog_ghribbons.ino is still in the folder, unlisted)
//   slot 6  WHITNEY   — differential motion, harmonograph   (Set 7 WHITNEY, Set 1 Hypnotic)
//   slot 7  RAIN      — slow digital rain, falling words    (Set 1 DIGITAL RAIN, Set 2 ASCII)
//   slot 8  MELT      — melting cellular automata           (Set 1 MIND MELT, Set 3 AUTOMATA)
//   slot 9  SUNSET    — endless synthwave drive             (Set 3 HORIZON, Set 4 VAPOR, Set 2 PIXEL CITY)
//   slot 10 WOBBLERS  — soft-body jelly creatures with googly eyes (new in v5; replaced BOUNCE —
//                       prog_ghbounce.ino is still in the folder, unlisted)
//   slot 11 VIBE CAT  — MEME: a cat that bops its head to the beat (tap tempo on k12)
//
// Every program: presets k0-k10, k11 = Tour (a new look every 40 s, 4 s fades),
// k12 = its own action. Knobs: p0 Palette, p1 Speed, p2 Zoom, p3 Colour Flow,
// p4 Hue Spread, p5-p10 the program's own, p11 KEY (◄ glow | black ►, for the
// mixer's luma key), p12-p15 live FX (vg5fx.h). p1 and p12-p15 do nothing until turned.
//
// Shared engine: vg8.h, on vg6.h (Set 6), vg5fx.h and vg3.h — it does NOT need
// vg7.h or vg7op.h. Switch program: hold k14 (Control) + k15 (Shift) and press
// k0-k11. The screen shows "PROGRAM n" with n = slot + 1.
//
// Older sets may still be in this folder but are not listed here, so the
// linker leaves them out: they cost no RAM.
// RAM budget: keep the compile's "Global variables" under ~88 KB (Set 8 ≈ 29 KB of its own).
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_ghlava, prog_ghlava_init, PROG_FLAG_OWNS_GLOBALS),          // LAVA — slot 0
  PROG_ENTRY_EX(1, prog_ghveils, prog_ghveils_init, PROG_FLAG_OWNS_GLOBALS),        // VEILS — slot 1
  PROG_ENTRY_EX(2, prog_ghquasi, prog_ghquasi_init, PROG_FLAG_OWNS_GLOBALS),        // QUASI — slot 2
  PROG_ENTRY_EX(3, prog_ghhole, prog_ghhole_init, PROG_FLAG_OWNS_GLOBALS),          // BLACK HOLE — slot 3
  PROG_ENTRY_EX(4, prog_ghhypno, prog_ghhypno_init, PROG_FLAG_OWNS_GLOBALS),        // HYPNO — slot 4
  PROG_ENTRY_EX(5, prog_ghascii, prog_ghascii_init, PROG_FLAG_OWNS_GLOBALS),       // ASCII — slot 5
  PROG_ENTRY_EX(6, prog_ghwhitney, prog_ghwhitney_init, PROG_FLAG_OWNS_GLOBALS),    // WHITNEY — slot 6
  PROG_ENTRY_EX(7, prog_ghrain, prog_ghrain_init, PROG_FLAG_OWNS_GLOBALS),          // RAIN — slot 7
  PROG_ENTRY_EX(8, prog_ghmelt, prog_ghmelt_init, PROG_FLAG_OWNS_GLOBALS),          // MELT — slot 8
  PROG_ENTRY_EX(9, prog_ghsunset, prog_ghsunset_init, PROG_FLAG_OWNS_GLOBALS),      // SUNSET — slot 9
  PROG_ENTRY_EX(10, prog_ghjelly, prog_ghjelly_init, PROG_FLAG_OWNS_GLOBALS),      // WOBBLERS — slot 10
  PROG_ENTRY_EX(11, prog_ghcat, prog_ghcat_init, PROG_FLAG_OWNS_GLOBALS),           // VIBE CAT — slot 11
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);

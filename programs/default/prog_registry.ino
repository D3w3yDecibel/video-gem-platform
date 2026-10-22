// =====================================================================
// PROGRAM REGISTRY — SET 6 "SIREN" (Dewey)
//
// Set 6: twelve programs on one theme — sacred geometry, hexagons and
// six-fold symmetry in shifting bands of colour — after Burt Vera Cruz's
// reel "Call of the Virgo Siren". Each one is a different way of making
// that kind of picture:
//
//   slot 0  SIREN     — a living hexagonal star lattice (closest to the reel)
//   slot 1  FLOWER    — compass drawings: Seed and Flower of Life, Torus, Lotus
//   slot 2  METATRON  — sacred solids in light: Metatron's Cube, Merkaba, tesseract
//   slot 3  GIRIH     — Islamic star patterns that breathe (Hankin's method)
//   slot 4  KALEIDO   — a liquid-light kaleidoscope, 4 to 12 mirrors
//   slot 5  FROST     — snowflakes growing live (Reiter's model)
//   slot 6  PRISMS    — a field of hexagonal pillars rising and falling
//   slot 7  TRIAD     — endless Sierpinski and bitwise triangle fractals
//   slot 8  STEREO    — a moving Magic Eye with hidden sacred shapes
//   slot 9  QUASI     — quasicrystals: 3 to 13 crossing waves
//   slot 10 CYMATICS  — sand singing into Chladni figures
//   slot 11 PORTAL    — a flight down a hexagonal tunnel
//
// Every program: presets k0-k10, k11 = Tour (changes look every ~20 s), k12 = its
// own action. Common knobs: p0 Palette, p1 Speed, p2 Zoom, p3 Colour Flow,
// p4 Hue Spread; p5-p11 are the program's own; p12-p15 live FX (vg5fx.h).
// p1 and p12-p15 do nothing until you turn them.
//
// Shared engine: vg6.h (+ vg6_geo.h tables), on vg3.h and vg5fx.h.
// Switch program: hold k14 (Control) + k15 (Shift) and press k0-k11.
// The screen shows "PROGRAM n" with n = slot + 1.
//
// Older sets may still be in this folder but are not listed here, so the
// linker leaves them out: they cost no RAM.
//
// RAM budget: keep the compile's "Global variables" under ~88 KB.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_siren, prog_siren_init, PROG_FLAG_OWNS_GLOBALS),        // SIREN — slot 0
  PROG_ENTRY_EX(1, prog_flower, prog_flower_init, PROG_FLAG_OWNS_GLOBALS),      // FLOWER — slot 1
  PROG_ENTRY_EX(2, prog_metatron, prog_metatron_init, PROG_FLAG_OWNS_GLOBALS),  // METATRON — slot 2
  PROG_ENTRY_EX(3, prog_girih, prog_girih_init, PROG_FLAG_OWNS_GLOBALS),        // GIRIH — slot 3
  PROG_ENTRY_EX(4, prog_kaleido, prog_kaleido_init, PROG_FLAG_OWNS_GLOBALS),    // KALEIDO — slot 4
  PROG_ENTRY_EX(5, prog_frost, prog_frost_init, PROG_FLAG_OWNS_GLOBALS),        // FROST — slot 5
  PROG_ENTRY_EX(6, prog_prisms, prog_prisms_init, PROG_FLAG_OWNS_GLOBALS),      // PRISMS — slot 6
  PROG_ENTRY_EX(7, prog_triad, prog_triad_init, PROG_FLAG_OWNS_GLOBALS),        // TRIAD — slot 7
  PROG_ENTRY_EX(8, prog_stereo, prog_stereo_init, PROG_FLAG_OWNS_GLOBALS),      // STEREO — slot 8
  PROG_ENTRY_EX(9, prog_quasi, prog_quasi_init, PROG_FLAG_OWNS_GLOBALS),        // QUASI — slot 9
  PROG_ENTRY_EX(10, prog_cymatics, prog_cymatics_init, PROG_FLAG_OWNS_GLOBALS), // CYMATICS — slot 10
  PROG_ENTRY_EX(11, prog_portal, prog_portal_init, PROG_FLAG_OWNS_GLOBALS),     // PORTAL — slot 11
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);

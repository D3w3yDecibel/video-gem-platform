// =====================================================================
// PROGRAM REGISTRY — SET 3 (Dewey)
//
// Set 3: all-new programs on the same Video Gem platform.
// Switch program: hold k14 (Control) + k15 (Shift) and press k0-k11.
// The screen shows "PROGRAM n" with n = slot + 1.
//
// Slots 3-11 share helpers and one block of working memory in vg3.h
// (only one program runs at a time), which is how they fit in RAM.
// WORMHOLE, SIGNAL and MOIRE also need wh_lut.h (a table of angles, in flash);
// HORIZON needs hz_map.h (its landscape, in flash).
//
// The Set 2 programs (prog_pixcity … prog_bugs) and the RMNA demos are
// still in this folder but not listed here, so the linker leaves them out:
// they cost no RAM. Set 2 itself is safe on the set-2 branch.
//
// RAM budget: keep the compile's "Global variables" under ~88 KB.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_flora, prog_flora_init, PROG_FLAG_OWNS_GLOBALS),          // FLORA — Set 3 slot 0
  PROG_ENTRY_EX(1, prog_spacetime, prog_spacetime_init, PROG_FLAG_OWNS_GLOBALS),  // SPACETIME — slot 1
  PROG_ENTRY_EX(2, prog_sprout, prog_sprout_init, PROG_FLAG_OWNS_GLOBALS),        // SPROUT — slot 2
  PROG_ENTRY_EX(3, prog_liquid, prog_liquid_init, PROG_FLAG_OWNS_GLOBALS),        // LIQUID — slot 3
  PROG_ENTRY_EX(4, prog_wormhole, prog_wormhole_init, PROG_FLAG_OWNS_GLOBALS),    // WORMHOLE — slot 4
  PROG_ENTRY_EX(5, prog_automata, prog_automata_init, PROG_FLAG_OWNS_GLOBALS),    // AUTOMATA — slot 5
  PROG_ENTRY_EX(6, prog_murmur, prog_murmur_init, PROG_FLAG_OWNS_GLOBALS),        // MURMUR — slot 6
  PROG_ENTRY_EX(7, prog_signal, prog_signal_init, PROG_FLAG_OWNS_GLOBALS),        // SIGNAL — slot 7
  PROG_ENTRY_EX(8, prog_glass, prog_glass_init, PROG_FLAG_OWNS_GLOBALS),          // GLASS — slot 8
  PROG_ENTRY_EX(9, prog_mapper, prog_mapper_init, PROG_FLAG_OWNS_GLOBALS),        // MAPPER — slot 9
  PROG_ENTRY_EX(10, prog_moire, prog_moire_init, PROG_FLAG_OWNS_GLOBALS),         // MOIRE — slot 10
  PROG_ENTRY_EX(11, prog_horizon, prog_horizon_init, PROG_FLAG_OWNS_GLOBALS),     // HORIZON — slot 11
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);

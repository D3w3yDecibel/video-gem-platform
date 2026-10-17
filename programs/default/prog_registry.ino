// =====================================================================
// PROGRAM REGISTRY — SET 4 "DECADES" (Dewey)
//
// Set 4: twelve variations on HORIZON (the landscape flight from Set 3),
// one per decade from the 1920s to the 2030s. Each has its own worlds,
// colours, sky props and its own way of EXPLODING the horizon on p15
// (or press k12 for a single blast).
// Switch program: hold k14 (Control) + k15 (Shift) and press k0-k11.
// The screen shows "PROGRAM n" with n = slot + 1.
//
// All twelve share the landscape engine in vg4.h, which needs vg3.h
// (shared helpers and working memory) and hz_map.h (the terrain, in flash).
//
// The Set 3 programs (prog_flora … prog_horizon) and older sets are still
// in this folder but not listed here, so the linker leaves them out: they
// cost no RAM. Set 3 itself is safe on the Set-3 branch.
//
// RAM budget: keep the compile's "Global variables" under ~88 KB.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_deco, prog_deco_init, PROG_FLAG_OWNS_GLOBALS),        // 1920s DECO — slot 0
  PROG_ENTRY_EX(1, prog_noir, prog_noir_init, PROG_FLAG_OWNS_GLOBALS),        // 1930s NOIR — slot 1
  PROG_ENTRY_EX(2, prog_techni, prog_techni_init, PROG_FLAG_OWNS_GLOBALS),    // 1940s TECHNICOLOR — slot 2
  PROG_ENTRY_EX(3, prog_atomic, prog_atomic_init, PROG_FLAG_OWNS_GLOBALS),    // 1950s ATOMIC — slot 3
  PROG_ENTRY_EX(4, prog_psyche, prog_psyche_init, PROG_FLAG_OWNS_GLOBALS),    // 1960s PSYCHEDELIC — slot 4
  PROG_ENTRY_EX(5, prog_disco, prog_disco_init, PROG_FLAG_OWNS_GLOBALS),      // 1970s DISCO — slot 5
  PROG_ENTRY_EX(6, prog_arcade, prog_arcade_init, PROG_FLAG_OWNS_GLOBALS),    // 1980s ARCADE — slot 6
  PROG_ENTRY_EX(7, prog_cgi, prog_cgi_init, PROG_FLAG_OWNS_GLOBALS),          // 1990s CGI — slot 7
  PROG_ENTRY_EX(8, prog_y2k, prog_y2k_init, PROG_FLAG_OWNS_GLOBALS),          // 2000s Y2K — slot 8
  PROG_ENTRY_EX(9, prog_vapor, prog_vapor_init, PROG_FLAG_OWNS_GLOBALS),      // 2010s VAPOR — slot 9
  PROG_ENTRY_EX(10, prog_neural, prog_neural_init, PROG_FLAG_OWNS_GLOBALS),   // 2020s NEURAL — slot 10
  PROG_ENTRY_EX(11, prog_future, prog_future_init, PROG_FLAG_OWNS_GLOBALS),   // 2030s FUTURE — slot 11
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);

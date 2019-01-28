/*
  Tests with no branches, but with protected variables

  After enforce:
   always_ok(1) => 3
   also_always_ok(1) => 3
   never_ok(1) => crash
   also_never_ok(1) => crash
*/

#include "it.h"

int other_v0 = 0;
int other_v1 = 1;

int protected_v2 = 2;
int protected_v3 = 3;

int always_ok(int sel) {
  open_it();
  sel += protected_v2;
  close_it();
  return sel;
}

int also_always_ok(int sel) {
  sel += other_v0;
  open_it();
  sel += protected_v2;
  close_it();
  return sel;
}

int never_ok(int sel) {
  return protected_v2; /* oops! - outside open & close */
}

int also_never_ok(int sel) {
  open_it();
  sel += protected_v2;
  close_it();
  sel += protected_v3;  /* oops! - outside open & close */
  return sel;
}

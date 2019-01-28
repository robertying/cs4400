/*
  Tests with no branches or protected variables.

  After enforce:
   always_ok(1) => 1
   also_always_ok(1) => 2
   never_ok(1) => crash
   also_never_ok(1) => crash
   still_never_ok(1) => crash
*/

#include "it.h"

int other_v0 = 0;
int other_v1 = 1;

int always_ok(int sel) {
  open_it();
  close_it();
  return 1;
}

int also_always_ok(int sel) {
  sel += other_v0;
  open_it();
  close_it();
  open_it();
  sel += other_v1;
  close_it();
  open_it();
  close_it();
  return sel;
}

int never_ok(int sel) {
  open_it();
  return 3; /* oops! - return without a balancing close */
}

int also_never_ok(int sel) {
  open_it();
  open_it(); /* oops! - already open */
  close_it(); /* doesn't matter whether this is here */
  close_it(); /* doesn't matter whether this is here */
  return 4;
}

int still_never_ok(int sel) {
  open_it();
  close_it();
  close_it(); /* oops! - not currently open */
  return 5;
}

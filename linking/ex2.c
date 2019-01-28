/*
  Tests that involve branches, but no variables.

  After enforce:
   always_ok(1) => 1
   always_ok(0) => 0
   sometimes_ok(1) => 1
   sometimes_ok(0) => crash
   never_ok(1) => crash
   never_ok(0) => crash
*/

#include "it.h"

int always_ok(int sel) {
  if (sel) {
    open_it();
    close_it();
    return 1;
  } else {
    return 0;
  }
}

int sometimes_ok(int sel) {
  open_it();
  if (sel) {
    close_it();
    return 1;
  } else {
    return 0; /* oops! - didn't close before return */
  }
}

int never_ok(int sel) {
  if (sel) {
    open_it();
    close_it();
    close_it(); /* oops! */
    return 1;
  } else {
    open_it();
    open_it(); /* oops! */
    return 0;
  }
}

/*
  Test that involve global variables.

  After enforce:
   always_ok(1) => -3
   always_ok(0) => -3
   also_always_ok(1) => 3
   also_always_ok(0) => 3
   still_always_ok(1) => 3
   still_always_ok(0) => -4
   sometimes_ok(1) => 4
   sometimes_ok(0) => crash
   never_ok(1) => crash
   never_ok(0) => crash
   also_never_ok(1) => crash
   also_never_ok(0) => crash
*/

#include "it.h"

int protected_a = 3;
int protected_b = 4;

int unprotected_a = -3;
int other_b = -4;

int always_ok(int sel) {
  return unprotected_a;
}

int also_always_ok(int sel) {
  int v;
  open_it();
  v = protected_a;
  close_it();
  return v;
}

int still_always_ok(int sel) {
  if (sel) {
    int v;
    open_it();
    v = protected_a;
    close_it();
    return v;
  } else {
    return other_b;
  }
}

int sometimes_ok(int sel) {
  open_it();
  if (sel) {
    int v = protected_b;
    close_it();
    return v;
  } else {
    return protected_b; /* oops! - not open */
  }
}

int never_ok(int sel) {
  if (sel) {
    open_it();
    close_it();
    return protected_a; /* oops! - not open anymore */
  } else {
    return protected_a; /* oops! - never was open */
  }
}

int also_never_ok(int sel) {
  int v = protected_a;  /* oops! - not open */
  if (sel) {
    open_it();
    v = protected_a; /* ok, but too late */
    close_it();
    return v;
  } else {
    return v;
  }
}

/*
  Tests with more branching.

  After enforce:
   function_0(-1) => crash
   function_0(0) => 2
   function_0(1) => crash
   function_1(-1) => crash
   function_1(-2) => 4
   function_1(0) => 3
   function_2(-1) => 5
   function_2(0) => 1
*/

#include "it.h"

int protected_v1 = 1;
int protected_v2 = 2;
int protected_v3 = 3;
int other_v1 = 4;
int other_v2 = 5;

int function_0(int sel) {
  if (sel < 0)
    return protected_v1;
  else if (sel == 0) {
    int v;
    open_it();
    v = protected_v2;
    close_it();
    return v;
  } else 
    return protected_v3;
}

int function_1(int sel) {
  if (sel < 0) {
    open_it();
    if (sel < -1) {
      close_it();
      return other_v1;
    } else
      return other_v2;
  } else {
    int v;
    open_it();
    v = protected_v3;
    close_it();
    return v;
  }
}

int function_2(int sel) {
  open_it();
  close_it();
  open_it();
  if (sel < 0) {
    close_it();
    return other_v2;
  } else {
    int v = protected_v1;
    close_it();
    return v;
  }
}

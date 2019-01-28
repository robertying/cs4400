/*
 * driver.h - Various definitions for the Performance Lab.
 * 
 * DO NOT MODIFY ANYTHING IN THIS FILE
 */
#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdlib.h>

#define RIDX(i,j,n) (1+(i)*(n)+(j))

typedef union {
  struct {
    unsigned short red;
    unsigned short green;
    unsigned short blue;
  };
  int dim;
} pixel;

typedef void (*pinwheel_test_func) (pixel*, pixel*);
typedef void (*glow_test_func) (pixel*, pixel*);

void pinwheel(pixel *, pixel *);
void glow(pixel *, pixel *);

void register_pinwheel_functions(void);
void register_glow_functions(void);
void add_pinwheel_function(pinwheel_test_func, char*);
void add_glow_function(glow_test_func, char*);

#endif /* _DEFS_H_ */


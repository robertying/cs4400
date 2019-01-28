#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

/******************************************************
 * PINWHEEL KERNEL
 *
 * Your different versions of the pinwheel kernel go here
 ******************************************************/

/*
  * naive_pinwheel - The naive baseline version of pinwheel
  */
char naive_pinwheel_descr[] = "naive_pinwheel: baseline implementation";
void naive_pinwheel(pixel *src, pixel *dest)
{
	int i, j;

	for (i = 0; i < src->dim; i++)
		for (j = 0; j < src->dim; j++)
		{
			/* Check whether we're in the diamond region */
			if ((fabs(i + 0.5 - src->dim / 2) + fabs(j + 0.5 - src->dim / 2)) < src->dim / 2)
			{
				/* In diamond region, so rotate and grayscale */
				int s_idx = RIDX(i, j, src->dim);
				int d_idx = RIDX(src->dim - j - 1, i, src->dim);
				dest[d_idx].red = ((int)src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3;
				dest[d_idx].green = ((int)src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3;
				dest[d_idx].blue = ((int)src[s_idx].red + src[s_idx].green + src[s_idx].blue) / 3;
			}
			else
			{
				/* Not in diamond region, so keep the same */
				int s_idx = RIDX(i, j, src->dim);
				int d_idx = RIDX(i, j, src->dim);
				dest[d_idx] = src[s_idx];
			}
		}
}

/*
 * pinwheel - Your current working version of pinwheel
 * IMPORTANT: This is the version you will be graded on
 */
char pinwheel_descr[] = "pinwheel: Current working version";
void pinwheel(pixel *src, pixel *dest)
{
	int i, j, k, m;
	int dim = src->dim;
	int dim_2 = dim >> 1;
	int block = 8;

	for (k = 0; k < dim; k += block)
	{
		for (m = 0; m < dim; m += block)
		{
			for (i = k; i < k + block; i++)
			{
				int dimi1 = i + 1 - dim_2;
				int begin = i < dim_2 ? dim_2 - i : dimi1;
				int end = i < dim_2 ? dim_2 + i : dim - dimi1;
				for (j = m; j < m + block; j += 2)
				{
					int jj = j + 1;

					/* Check whether we're in the diamond region */
					if (j >= begin && j < end)
					{
						/* In diamond region, so rotate and grayscale */
						int d_idx = RIDX(dim - j - 1, i, dim);
						pixel src_idx = src[RIDX(i, j, dim)];
						unsigned short src_color = ((int)src_idx.red + src_idx.green + src_idx.blue) / 3;

						dest[d_idx].red = src_color;
						dest[d_idx].green = src_color;
						dest[d_idx].blue = src_color;
					}
					else
					{
						int idx = RIDX(i, j, dim);
						dest[idx] = src[idx];
					}

					if (jj >= begin && jj < end)
					{
						/* In diamond region, so rotate and grayscale */
						int d_idx = RIDX(dim - jj - 1, i, dim);
						pixel src_idx = src[RIDX(i, jj, dim)];
						unsigned short src_color = ((int)src_idx.red + src_idx.green + src_idx.blue) / 3;

						dest[d_idx].red = src_color;
						dest[d_idx].green = src_color;
						dest[d_idx].blue = src_color;
					}
					else
					{
						int idx = RIDX(i, jj, dim);
						dest[idx] = src[idx];
					}
				}
			}
		}
	}
}

/*********************************************************************
 * register_pinwheel_functions - Register all of your different versions
 *     of the pinwheel kernel with the driver by calling the
 *     add_pinwheel_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_pinwheel_functions()
{
	add_pinwheel_function(&pinwheel, pinwheel_descr);
	add_pinwheel_function(&naive_pinwheel, naive_pinwheel_descr);
}

/***************************************************************
 * GLOW KERNEL
 *
 * Starts with various typedefs and helper functions for the glow
 * function, and you may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct
{
	int red;
	int green;
	int blue;
} pixel_sum;

/*
 * initialize_pixel_sum - Initializes all fields of sum to 0
 */
inline static void initialize_pixel_sum(pixel_sum *sum)
{
	sum->red = sum->green = sum->blue = 0;
}

/*
 * accumulate_sum - Accumulates field values of p in corresponding
 * fields of sum
 */
inline static void accumulate_weighted_sum(pixel_sum *sum, pixel p, double weight)
{
	sum->red += (int)p.red * weight;
	sum->green += (int)p.green * weight;
	sum->blue += (int)p.blue * weight;
}

/*
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel
 */
inline static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum)
{
	current_pixel->red = (unsigned short)sum.red;
	current_pixel->green = (unsigned short)sum.green;
	current_pixel->blue = (unsigned short)sum.blue;
}

/*
 * weighted_combo - Returns new pixel value at (i,j)
 */
inline static pixel weighted_combo(int dim, int i, int j, pixel *src)
{
	pixel current_pixel;

	int red = 0;
	int green = 0;
	int blue = 0;

	int iii = -1 + i;

	int jjj = -1 + j;
	if ((iii >= 0) && (iii < dim) && (jjj >= 0) && (jjj < dim))
	{
		pixel p = src[RIDX(iii, jjj, dim)];
		red += ((int)p.red << 2) / 25;
		green += ((int)p.green << 2) / 25;
		blue += ((int)p.blue << 2) / 25;
	}
	jjj += 2;
	if ((iii >= 0) && (iii < dim) && (jjj >= 0) && (jjj < dim))
	{
		pixel p = src[RIDX(iii, jjj, dim)];
		red += ((int)p.red << 2) / 25;
		green += ((int)p.green << 2) / 25;
		blue += ((int)p.blue << 2) / 25;
	}

	iii++;

	jjj = j;
	if ((iii >= 0) && (iii < dim) && (jjj >= 0) && (jjj < dim))
	{
		pixel p = src[RIDX(iii, jjj, dim)];
		red += (int)p.red * 3 / 10;
		green += (int)p.green * 3 / 10;
		blue += (int)p.blue * 3 / 10;
	}

	iii++;

	jjj = -1 + j;
	if ((iii >= 0) && (iii < dim) && (jjj >= 0) && (jjj < dim))
	{
		pixel p = src[RIDX(iii, jjj, dim)];
		red += ((int)p.red << 2) / 25;
		green += ((int)p.green << 2) / 25;
		blue += ((int)p.blue << 2) / 25;
	}
	jjj += 2;
	if ((iii >= 0) && (iii < dim) && (jjj >= 0) && (jjj < dim))
	{
		pixel p = src[RIDX(iii, jjj, dim)];
		red += ((int)p.red << 2) / 25;
		green += ((int)p.green << 2) / 25;
		blue += ((int)p.blue << 2) / 25;
	}

	current_pixel.red = (unsigned short)red;
	current_pixel.green = (unsigned short)green;
	current_pixel.blue = (unsigned short)blue;

	return current_pixel;
}

/******************************************************
 * Your different versions of the glow kernel go here
 ******************************************************/

/*
  * naive_glow - The naive baseline version of glow
  */
char naive_glow_descr[] = "naive_glow: baseline implementation";
void naive_glow(pixel *src, pixel *dst)
{
	int i, j;

	for (i = 0; i < src->dim; i++)
		for (j = 0; j < src->dim; j++)
			dst[RIDX(i, j, src->dim)] = weighted_combo(src->dim, i, j, src);
}

/*
 * glow - Your current working version of glow.
 * IMPORTANT: This is the version you will be graded on
 */
char glow_descr[] = "glow: Current working version";
void glow(pixel *src, pixel *dst)
{
	int i, j;
	int dim = src->dim;
	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			dst[RIDX(i, j, dim)] = weighted_combo(dim, i, j, src);
}

/*********************************************************************
 * register_glow_functions - Register all of your different versions
 *     of the glow kernel with the driver by calling the
 *     add_glow_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_glow_functions()
{
	add_glow_function(&glow, glow_descr);
	add_glow_function(&naive_glow, naive_glow_descr);
}

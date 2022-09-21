#ifndef __MY_MATH_H__
#define __MY_MATH_H__

#include "IO_In_Out.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define MA_pi				3.14159265359f
#define MA_pi_x2			6.28318530717f

#define MA_MAX(x, y)        ((x) > (y) ? (x) : (y))
#define MA_MIN(x, y)        ((x) < (y) ? (x) : (y))
#define MA_CLAMP(a, b, c)   MY_MIN(MY_MAX((a), (b)), (c))

#define MA_MIN_2(x,y)		( y ^ ((x ^ y) & -(x < y)) )
#define MA_MAX_2(x,y)		( x ^ ((x ^ y) & -(x < y)) )

#define MA_FP18_BITSHIFT			18
#define MA_FP18_DEC					262144.0f
#define MA_FP18_FRACTION_MASK		((1 << MA_FP18_BITSHIFT) - 1)
#define MA_FP18_INTEGER_MASK		(~MA_FP18_FRACTION_MASK)
#define MA_FP18_05					131072							// ((int32)(0.5f * 262144.0f))
#define MA_FP18_0999				261882							// ((int32)(0.999f * 262144.0f))

#define MA_FP18_FLOOR(x)			(x & MA_FP18_INTEGER_MASK)
#define MA_FP18_FRACTION(x)			(x & MA_FP18_FRACTION_MASK)

// ----------------------------------------------------------------------------------------------------
// --- FOR ENDIANNES SWAPPING - used for files created on different platforms like level maps, etc. ---
// ----------------------------------------------------------------------------------------------------
u_int16		MA_swap_uint16(u_int16);
int16		MA_swap_int16(int16);
u_int32		MA_swap_uint32(u_int32);
int32		MA_swap_int32(int32);

void        MA_Add_Number_Spaces(int32, char*);

#endif

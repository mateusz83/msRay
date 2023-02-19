#ifndef __GR_DISTANCE_SHADING_H__
#define __GR_DISTANCE_SHADING_H__

#include "IO_In_Out.h"

#define GR_DISTANCE_INTENSITY_LENGTH             128   
#define GR_DISTANCE_INTENSITY_MIN                100.0f 
#define GR_DISTANCE_INTENSITY_SCALE              20.0f 

// Structures for precalculated distance intensity tables.
int32** GR_distance_shading_walls__LUT;
int32** GR_distance_shading_flats__LUT;

int32	GR_Distance_Shading_Init_Once(void);
void	GR_Distance_Shading_Cleanup(void);

#endif

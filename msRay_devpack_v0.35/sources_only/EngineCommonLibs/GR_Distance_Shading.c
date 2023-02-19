#include "GR_Distance_Shading.h"

#include <stdlib.h>

int32	GR_Distance_Shading_Init_Once(void)
{
	// Setup distance shading intensity LUT.

	int32 memory_allocated = 0;

	// -- MALLOC WALLS --
	int32 mem_size = GR_DISTANCE_INTENSITY_LENGTH * sizeof(int32*);
	GR_distance_shading_walls__LUT = (int32**)malloc(mem_size);
	if (GR_distance_shading_walls__LUT == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;


	// -- MALLOC FLATS --
	mem_size = GR_DISTANCE_INTENSITY_LENGTH * sizeof(int32*);
	GR_distance_shading_flats__LUT = (int32**)malloc(mem_size);
	if (GR_distance_shading_flats__LUT == NULL) return 0;

	// counting memory...
	memory_allocated += mem_size;


	float32 step = (1.0f - (GR_DISTANCE_INTENSITY_MIN / IO_TEXTURE_MAX_SHADES)) / GR_DISTANCE_INTENSITY_LENGTH;
	float32 factor = 1.0f;

	for (int32 i = 0; i < GR_DISTANCE_INTENSITY_LENGTH; i++)
	{
		// -- MALLOC WALLS --
		mem_size = IO_TEXTURE_MAX_SHADES * sizeof(int32);
		GR_distance_shading_walls__LUT[i] = (int32*)malloc(mem_size);
		if (GR_distance_shading_walls__LUT[i] == NULL) return 0;

		// counting memory...
		memory_allocated += mem_size;


		// -- MALLOC FLATS --
		mem_size = IO_TEXTURE_MAX_SHADES * sizeof(int32);
		GR_distance_shading_flats__LUT[i] = (int32*)malloc(mem_size);
		if (GR_distance_shading_flats__LUT[i] == NULL) return 0;

		// counting memory...
		memory_allocated += mem_size;

		for (int32 j = 0; j < IO_TEXTURE_MAX_SHADES; j++)
		{
			int32 tmp = (int32)(j * factor) * IO_TEXTURE_MAX_COLORS;

			GR_distance_shading_walls__LUT[i][j] = tmp;
			GR_distance_shading_flats__LUT[i][j] = tmp;
		}

		factor -= step;
	}

	return memory_allocated;
}
void	GR_Distance_Shading_Cleanup(void)
{
	for (int32 i = 0; i < GR_DISTANCE_INTENSITY_LENGTH; i++)
		free(GR_distance_shading_walls__LUT[i]);

	free(GR_distance_shading_walls__LUT);
}
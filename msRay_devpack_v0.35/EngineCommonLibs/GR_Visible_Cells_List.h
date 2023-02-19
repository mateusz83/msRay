#ifndef __GR_VISIBLE_CELLS_LIST_H__
#define __GR_VISIBLE_CELLS_LIST_H__

#include "LV_Level.h"

// NOTE!
// If player can see lots of cells in big room, more than GR_VCL_MAX_VISIBLE_CELLS,
// the game can crash or weird glitches can occure. So we can increase this value.
// But 512 should be enough.

#define		GR_VCL_MAX_VISIBLE_CELLS		512

int8		GR_vcl_found[LV_MAP_CELLS_COUNT];
int16		GR_vcl_list[GR_VCL_MAX_VISIBLE_CELLS];
int16		GR_vcl_size;

void		GR_VCL_Reset(void);
void		GR_VCL_Add_Cell(int16);


#endif
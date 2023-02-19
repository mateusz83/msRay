#ifndef __GR_WALL_RENDER_H__
#define __GR_WALL_RENDER_H__

#include "LV_Level.h"
#include "MA_Math.h"

#include <stdlib.h>

// ------------------------------------------------------------------------
// --- GAME RENDER - WALLS / DOORS - PUBLIC globals, constants, variables ---
// ------------------------------------------------------------------------
int16	GR_wall_end_min, GR_wall_start_max;

// ----------------------------------------------------------------
// --- GAME RENDER - WALLS / DOORS - PUBLIC functions definitions ---
// ----------------------------------------------------------------
int32	GR_Walls_Init_Once(void);
void	GR_Walls_Cleanup(void);

void	GR_Doors_Open(void);
void	GR_Doors_Update(void);

void	GR_Walls_Raycast(void);
void	GR_Walls_Render(void);

#endif
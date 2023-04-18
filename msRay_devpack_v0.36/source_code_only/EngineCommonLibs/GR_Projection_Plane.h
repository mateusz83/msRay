#ifndef __GR_PROJECTION_PLANE_H__
#define __GR_PROJECTION_PLANE_H__

#include "IO_In_Out.h"

// -----------------------------------------------------------------------------
// --- GAME RENDER - PROJECTION PLANE - PUBLIC globals, constants, variables ---
// -----------------------------------------------------------------------------
int16		GR_horizont;

int16		GR_render_width, GR_render_height;
int16		GR_render_width__sub1, GR_render_width__div2;
int16		GR_render_height__sub1, GR_render_height__div2;
float32		GR_render_width__1div;

float32		GR_pp_dx, GR_pp_dy;
float32		GR_pp_nsize_x__div2, GR_pp_nsize_y__div2;
float32*	GR_pp_coord_x__LUT;

// ---------------------------------------------------------------------
// --- GAME RENDER - PROJECTION PLANE - PUBLIC functions definitions ---
// ---------------------------------------------------------------------
void	GR_Projection_Plane_Init_Once(void);
void	GR_Projection_Plane_Reset(void);

#endif
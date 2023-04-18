#ifndef __GR_FLATS_H__
#define __GR_FLATS_H__

#include "IO_In_Out.h"
#include <stdlib.h>

// ----------------------------------------------------------
// --- GAME RENDER - FLATS - PUBLIC functions definitions ---
// ----------------------------------------------------------
int32	GR_Flats_Init_Once(void);
void	GR_Flats_Cleanup(void);

void	GR_Flats_Cache_Calculations(void);
void	GR_Flats_Render(void);

#endif
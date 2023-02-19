#ifndef __GR_GAME_RENDER_H__
#define __GR_GAME_RENDER_H__

#include "IO_In_Out.h"

// ---------------------------------------------------
// --- GAME RENDER - PUBLIC functions declarations ---
// ---------------------------------------------------
int32	GR_Init_Once(void);
void	GR_Cleanup(void);

void	GR_Reset(void);
void	GR_Run(void);

#endif
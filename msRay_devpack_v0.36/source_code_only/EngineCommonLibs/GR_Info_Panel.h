#ifndef __GR_INFO_PANEL_H__
#define __GR_INFO_PANEL_H__

#include "IO_In_Out.h"

// -----------------------------------------------------------------------------
// --- GAME RENDER - INFO PANEL - PUBLIC globals, constants, variables ---
// -----------------------------------------------------------------------------
int16		GR_info_panel_height;

// ---------------------------------------------------------------------
// --- GAME RENDER - INFO PANEL - PUBLIC functions definitions ---
// ---------------------------------------------------------------------
int32	GR_Info_Panel_Init_Once(void);
void	GR_Info_Panel_Cleanup(void);

void	GR_Info_Panel_Render(void);

#endif
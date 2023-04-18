#ifndef __EN_ENGINE_MAIN_H__
#define __EN_ENGINE_MAIN_H__

#include "IO_In_Out.h"
#include "MA_Math.h"

// -----------------------------------------------------
// --- ENGINE - PUBLIC globals, constants, variables ---
// -----------------------------------------------------
sIO_Input IO_input;
sIO_Prefs IO_prefs;

// ----------------------------------------------
// --- ENGINE - PUBLIC functions declarations ---
// ----------------------------------------------
int32		EN_Init(int8, int16, int16, int32);
void		EN_Run(int8*);
void		EN_Cleanup();

#endif
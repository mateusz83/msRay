#ifndef __TM_TIMER_H__
#define __TM_TIMER_H__

#include "IO_In_Out.h"

// ------------------------------------------------------------------------------------------------------------------
// NOTE! Timer functions depends on target Operating System. Use timer with microseconds precision.
// the return value from 'TM_Get_Delta()' should be float containing time in microseconds precision ex. == 0.002380s
// ------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------
// --- TIMER - PUBLIC functions declarations ---
// ---------------------------------------------
int8	TM_Init(void);
void	TM_Cleanup(void);
float32 TM_Get_Delta_Time(void);

#endif

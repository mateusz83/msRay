#ifndef __PL_PLAYER_H__
#define __PL_PLAYER_H__

#include "IO_In_Out.h"

// -----------------------------------------------------------
// --- GAME OBJECTS - PUBLIC globals, constants, variables ---
// -----------------------------------------------------------

// Player attributes.
#define PL_MOVE_SPEED        2.0f     
#define PL_Z_SPEED           10   
#define PL_Z_MAX             10.0f
#define PL_Z_LUT_SIZE        256
#define PL_Z_LUT_SIZE__SUB1  255

// Player structure and extern declaration.
typedef struct
{
	// Precalculated z position array, used while moving.
	float32 z__LUT[PL_Z_LUT_SIZE];

	// Player orientation parameters.
	float32 x, y, z;
	float32 angle, pitch;
	float32 z_accumulation;
	float32 pitch_speed;

	float32 pitch_max;

}_IO_BYTE_ALIGN_ sPL_Player;

sPL_Player PL_player;

// ----------------------------------------------------
// --- GAME OBJECTS - PUBLIC functions declarations ---
// ----------------------------------------------------
void PL_Player_Init(void);
void PL_Player_Reset(void);

#endif
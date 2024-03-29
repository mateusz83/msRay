#ifndef __PL_PLAYER_H__
#define __PL_PLAYER_H__

#include "IO_In_Out.h"

// -----------------------------------------------------------
// --- GAME OBJECTS - PUBLIC globals, constants, variables ---
// -----------------------------------------------------------

// Player attributes.
#define PL_MOVE_SPEED			2.0f     
#define PL_Z_SPEED				15   
#define PL_Z_MAX				7.0f
#define PL_Z_LUT_SIZE			256
#define PL_Z_LUT_SIZE__SUB1		255
#define	PL_BB_SIZE				0.25f

// Player structure and extern declaration.
typedef struct
{
	// Precalculated z position array, used while moving.
	float32 z__LUT[PL_Z_LUT_SIZE];

	// Player orientation parameters.
	float32 x, y, z;
	float32 starting_angle_radians, pitch;
	float32 z_accumulation;
	float32 pitch_speed;

	float32 pitch_max;

	int32	curr_cell_id;
	int8	curr_cell_x, curr_cell_y;

}_IO_BYTE_ALIGN_ sPL_Player;

sPL_Player PL_player;

// ----------------------------------------------------
// --- GAME OBJECTS - PUBLIC functions declarations ---
// ----------------------------------------------------
void PL_Player_Init_Once(void);
void PL_Player_Reset(void);

void PL_Player_Movement_Blocking(float32 _new_pos_x, float32 _new_pos_y, int8* _block_x, int8* _block_y);

#endif
#include "GR_Game_Render.h"
#include "GR_Projection_Plane.h"
#include "GR_Distance_Shading.h"
#include "GR_Visible_Cells_List.h"
#include "GR_Walls.h"
#include "GR_Flats.h"

#include "PL_Player.h"
#include "LV_Level.h"
#include "MA_Math.h"

#include <stdlib.h>

// -----------------------------------------------------------
// --- GAME RENDER - PRIVATE globals, constants, variables ---
// -----------------------------------------------------------
static int8	GR_start_lock;

// ---------------------------------------------------
// --- GAME RENDER - PRIVATE functions definitions ---
// ---------------------------------------------------
static void	GR_Update_Input(void)
{
	if (GR_start_lock)
	{
		IO_input.mouse_dx = 0;
		IO_input.mouse_dy = 0;

		GR_start_lock = 0;
	}
	
	// ESC key.
	if (IO_input.keys[IO_KEYCODE_ESC])
	{
		IO_prefs.engine_state = EN_STATE_GAMEPLAY_CLEANUP;
	}

	// SPACE key - try open door.
	if (IO_input.keys[IO_KEYCODE_SPACE])
	{
		GR_Doors_Open();
	}

	// Update mouse movement X - rotate direction of player and the projection plane.
	float player_angle_speed;

	#if defined _WIN32
		player_angle_speed = (float32)IO_input.mouse_dx * IO_prefs.delta_time;
	#elif defined AMIGA
		player_angle_speed = ((float32)IO_input.mouse_dx * IO_prefs.delta_time) * 0.2f;
	#endif

	// Rotate camera direction and camera projection plane.
	float32 tmp_sinf = sinf(player_angle_speed);
	float32 tmp_cosf = cosf(player_angle_speed);

	float32 old_pp_dx = GR_pp_dx;
	GR_pp_dx = GR_pp_dx * tmp_cosf - GR_pp_dy * tmp_sinf;
	GR_pp_dy = old_pp_dx * tmp_sinf + GR_pp_dy * tmp_cosf;

	float32 old_pp_nsize_x = GR_pp_nsize_x__div2;
	GR_pp_nsize_x__div2 = GR_pp_nsize_x__div2 * tmp_cosf - GR_pp_nsize_y__div2 * tmp_sinf;
	GR_pp_nsize_y__div2 = old_pp_nsize_x * tmp_sinf + GR_pp_nsize_y__div2 * tmp_cosf;

	// Update mouse movement Y - pitch.
	PL_player.pitch -= (float32)IO_input.mouse_dy * PL_player.pitch_speed * IO_prefs.delta_time;

	if (PL_player.pitch > PL_player.pitch_max) PL_player.pitch = PL_player.pitch_max;
	if (PL_player.pitch < -PL_player.pitch_max) PL_player.pitch = -PL_player.pitch_max;

	// Calculate player movement speed.
	float32 speed_dt = PL_MOVE_SPEED * IO_prefs.delta_time;

	u_int8 player_moved = 0;

	// Moving foreward and backward are self-excluding so we can type as..
	if (IO_input.keys[IO_KEYCODE_W])
	{
		player_moved = 1;
	
		float32 new_pos_x = PL_player.x + GR_pp_dx * speed_dt;
		float32 new_pos_y = PL_player.y + GR_pp_dy * speed_dt;

		int8 block_x = 0;
		int8 block_y = 0;

		// Check player movement blocking.
		PL_Player_Movement_Blocking(new_pos_x, new_pos_y, &block_x, &block_y);

		if (block_x == 0)	PL_player.x = new_pos_x;
		if (block_y == 0)	PL_player.y = new_pos_y;
	}
	else if (IO_input.keys[IO_KEYCODE_S])
	{
		player_moved = 1;

		float32 new_pos_x = PL_player.x - GR_pp_dx * speed_dt;
		float32 new_pos_y = PL_player.y - GR_pp_dy * speed_dt;

		int8 block_x = 0;
		int8 block_y = 0;

		// Check player movement blocking.
		PL_Player_Movement_Blocking(new_pos_x, new_pos_y, &block_x, &block_y);

		if (block_x == 0)	PL_player.x = new_pos_x;
		if (block_y == 0)	PL_player.y = new_pos_y;
	}

	// moving left and right are self-excluding so we can type as..
	if (IO_input.keys[IO_KEYCODE_A])
	{
		player_moved = 1;

		float32 new_pos_x = PL_player.x + GR_pp_dy * speed_dt;
		float32 new_pos_y = PL_player.y - GR_pp_dx * speed_dt;

		int8 block_x = 0;
		int8 block_y = 0;

		// Check player movement blocking.
		PL_Player_Movement_Blocking(new_pos_x, new_pos_y, &block_x, &block_y);

		if (block_x == 0)	PL_player.x = new_pos_x;
		if (block_y == 0)	PL_player.y = new_pos_y;
	}
	else if (IO_input.keys[IO_KEYCODE_D])
	{
		player_moved = 1;

		float32 new_pos_x = PL_player.x - GR_pp_dy * speed_dt;
		float32 new_pos_y = PL_player.y + GR_pp_dx * speed_dt;

		int8 block_x = 0;
		int8 block_y = 0;

		// Check player movement blocking.
		PL_Player_Movement_Blocking(new_pos_x, new_pos_y, &block_x, &block_y);

		if (block_x == 0)	PL_player.x = new_pos_x;
		if (block_y == 0)	PL_player.y = new_pos_y;
	}

	if (player_moved)
	{
		PL_player.z_accumulation += 500.0f * IO_prefs.delta_time;

		if (PL_player.z_accumulation >= PL_Z_LUT_SIZE__SUB1)
			PL_player.z_accumulation = 0.0f;

		int32 index = (int32)PL_player.z_accumulation;

		PL_player.z = PL_player.z__LUT[index];
	}

	PL_player.curr_cell_x = (int8)PL_player.x;
	PL_player.curr_cell_y = (int8)PL_player.y;
	PL_player.curr_cell_id = PL_player.curr_cell_x + (PL_player.curr_cell_y << LV_MAP_LENGTH_BITSHIFT);
}

// --------------------------------------------------
// --- GAME RENDER - PUBLIC functions definitions ---
// --------------------------------------------------
int32	GR_Init_Once()
{
	int32 memory_allocated = 0;
	int32 mem_size;

	// Init projection plane variables.
	GR_Projection_Plane_Init_Once();

	// Init Player settings once.
	PL_Player_Init_Once();

	// Init precalculated LUT that contains distance shading values.
	mem_size = GR_Distance_Shading_Init_Once();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;

	// Init all Walls helpers.
	mem_size = GR_Walls_Init_Once();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;
	
	// Init all Flats helpers.
	mem_size = GR_Flats_Init_Once();
	if (mem_size == 0) return 0;
	memory_allocated += mem_size;

	return memory_allocated;
}
void	GR_Cleanup(void)
{
	GR_Flats_Cleanup();
	GR_Walls_Cleanup();
	GR_Distance_Shading_Cleanup();
}

void	GR_Reset(void)
{
	GR_start_lock = 1;

	// Reset the rest of player startup settings. 
	// The starting x,y position and angle is set during level loading.
	PL_Player_Reset();

	// Reset Projection Plane.
	GR_Projection_Plane_Reset();
}
void	GR_Run()
{
	// Get keys and mouse parameters and update.
	GR_Update_Input();

	// Update the parameters of active doors.
	GR_Doors_Update();

	//memset(IO_prefs.output_buffer_32, 0x88, IO_prefs.screen_frame_buffer_size);

	// Raycast Walls, gather info about walls.
	GR_Walls_Raycast();

	// Cache some per-line calculations to avoid duplications.
	GR_Flats_Cache_Calculations();

	// Render Flats first.
    GR_Flats_Render();

	// Render Walls second.
	GR_Walls_Render();
}


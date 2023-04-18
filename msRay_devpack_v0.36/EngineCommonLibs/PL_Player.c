#include "PL_Player.h"
#include "LV_Level.h"
#include "MA_Math.h"

// There is an issue when Player collides with narrow BOX WALLS with sizes: 0.1f, 0.2f, 0.3f,
// because the gaps between Players Bounding Box are too wide.
// So when checking collisions with BOX WALLS/DOORS lets add to narrow walls additional values,
// so they will have an additional larger bounding box around them.
// Its only needed for 0.1, 0.2, 0.3 sizes as 0.4 seems to be fine.
// Lets use a wall side size multiplied by 10 as index to the table. So for the size 0.1 it will be index number 1.
static float32 PL_narrow_wall_helper_bb[11] = { 0.0f, 0.2f, 0.15f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

// ----------------------------------------------
// --- PLAYER - PRIVATE functions definitions ---
// ----------------------------------------------
static void PL_Player_Wall_Test(float32 _new_pos_x, float32 _new_pos_y, sLV_Cell* _test_cell, float32 _bb_x, float32 _bb_y, int8* _block_axis)
{
	// Perform the test only if the map cell is not empty.
	switch (_test_cell->cell_type)
	{
		case LV_C_WALL_STANDARD:
		case LV_C_WALL_FOURSIDE:
			// If the cell is regular we cant move it there there for sure. Block the axis and break.
			*_block_axis = 1;
			break;

		case LV_C_WALL_THIN_HORIZONTAL:
		case LV_C_WALL_THIN_VERTICAL:
		case LV_C_WALL_THIN_OBLIQUE:
		case LV_C_DOOR_THICK_HORIZONTAL:
		case LV_C_DOOR_THICK_VERTICAL:
		case LV_C_DOOR_THIN_OBLIQUE:
		{
			// When the walls are not regular we need additional calculation against wall vertices.

			// Lets use this equation to new player position and current BB point - against the line segment of the cell.
			// If (Ax1 + By1 + C)(Ax2 + By2 + C) > 0 the two points are on the same side, when < 0 they are on the opposite sides.
			// If it equals 0 at leat one of the point is on the line.

			float32 a = _test_cell->wall_vertex[0][1] - _test_cell->wall_vertex[1][1];
			float32 b = _test_cell->wall_vertex[1][0] - _test_cell->wall_vertex[0][0];
			float32 c = _test_cell->wall_vertex[0][0] * _test_cell->wall_vertex[1][1] - _test_cell->wall_vertex[0][1] * _test_cell->wall_vertex[1][0];

			float32 test_equation = (a * _new_pos_x + b * _new_pos_y + c) * (a * _bb_x + b * _bb_y + c);

			if (test_equation <= 0 && (_test_cell->height > 0)) *_block_axis = 1;
		}
		break;

		case LV_C_WALL_BOX:
		case LV_C_WALL_BOX_FOURSIDE:
		case LV_C_WALL_BOX_SHORT:
		case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_LEFT:
		case LV_C_DOOR_BOX_FOURSIDE_HORIZONTAL_RIGHT:
		case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_LEFT:
		case LV_C_DOOR_BOX_FOURSIDE_VERTICAL_RIGHT:
		{
			// For non regular box walls/doors we can simplify additional the calculations.

			// For narrow sides like: 0.1, 0.2, 0.3 lets add additional values so the bounding boxex around them will be bit bigger.
			float32 side_x_addition = PL_narrow_wall_helper_bb[(int32)((_test_cell->wall_vertex[1][0] - _test_cell->wall_vertex[0][0]) * 10.0f)];
			float32 side_y_addition = PL_narrow_wall_helper_bb[(int32)((_test_cell->wall_vertex[1][1] - _test_cell->wall_vertex[0][1]) * 10.0f)];

			if (_bb_x >= _test_cell->wall_vertex[0][0] - side_x_addition &&
				_bb_x <= _test_cell->wall_vertex[1][0] + side_x_addition &&
				_bb_y >= _test_cell->wall_vertex[0][1] - side_y_addition && 
				_bb_y <= _test_cell->wall_vertex[1][1] + side_y_addition)
				*_block_axis = 1;
		}
		break;
	}
}

// ---------------------------------------------------
// --- PLAYER - PUBLIC functions definitions ---
// ---------------------------------------------------
void PL_Player_Init_Once(void)
{
    // Init precalculated z position array.
    float32 tmp_step = 360.0f / PL_Z_LUT_SIZE;
    int32 tmp_index = 0;

    for (float32 f = 0.0f; f < 360.0f; f += tmp_step)
    {
        PL_player.z__LUT[tmp_index] = sinf(f * MA_pi / 180.0f) * PL_Z_MAX * ((float32)IO_prefs.screen_height / 320.0f);
        tmp_index++;
    }

    // Set pitch speed.
    #if defined _WIN32
        PL_player.pitch_speed = ((float32)IO_prefs.screen_height * 1.5f);
    #elif defined AMIGA
        PL_player.pitch_speed = ((float32)IO_prefs.screen_height * 1.5f) / 10.0f;
    #endif

    // Set max pitch.
    PL_player.pitch_max = roundf(IO_prefs.screen_height / 3.0f);
}
void PL_Player_Reset(void)
{
	PL_player.pitch = 0.0f;

	PL_player.z_accumulation = 0.0f;
	PL_player.z = PL_player.z__LUT[(int32)PL_player.z_accumulation];
}

void PL_Player_Movement_Blocking(float32 _new_pos_x, float32 _new_pos_y, int8* _block_x, int8* _block_y)
{
	// BB (x0,y0) (x1,y1) is the Bounding Box over the Player. It is always world axis aligned.

	// Old position of BB (before Player new movement)
	float32 old_bb_x0 = PL_player.x - PL_BB_SIZE;
	float32 old_bb_y0 = PL_player.y - PL_BB_SIZE;
	float32 old_bb_x1 = PL_player.x + PL_BB_SIZE;
	float32 old_bb_y1 = PL_player.y + PL_BB_SIZE;

	// New position of BB (if the Player would move in the new direction)
	float32 new_bb_x0 = (_new_pos_x - PL_BB_SIZE);
	float32 new_bb_y0 = (_new_pos_y - PL_BB_SIZE);
	float32 new_bb_x1 = (_new_pos_x + PL_BB_SIZE);
	float32 new_bb_y1 = (_new_pos_y + PL_BB_SIZE);

	// We will now examine position of each of the four corners of the BB.
	// To allow Player 'sliding' over the walls, we will split the examine processs into two conditions.
	// First condition with NEW X and OLD Y, and the second with OLD X and NEW Y.


	// ------------------
	
	
	// BB (x0,y0). TOP-LEFT corner of BB. ** X-axis.
	sLV_Cell* test_cell = &LV_map[(int32)new_bb_x0 + ((int32)old_bb_y0 << 6)];

	if (test_cell->cell_type != 0)
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, new_bb_x0, old_bb_y0, _block_x);


	// BB (x0,y0). TOP-LEFT corner of BB. ** Y-axis.
	test_cell = &LV_map[(int32)old_bb_x0 + ((int32)new_bb_y0 << 6)];

	if (test_cell->cell_type != 0)
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, old_bb_x0, new_bb_y0, _block_y);


	// -------------------


	// BB (x1,y0). TOP-RIGHT corner of BB. ** X-axis.
	test_cell = &LV_map[(int32)new_bb_x1 + ((int32)old_bb_y0 << 6)];

	// If the x-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_x == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, new_bb_x1, old_bb_y0, _block_x);


	// BB (x1,y0). TOP-RIGHT corner of BB. ** Y-axis.
	test_cell = &LV_map[(int32)old_bb_x1 + ((int32)new_bb_y0 << 6)];

	// If the y-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_y == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, old_bb_x1, new_bb_y0, _block_y);


	// -------------------


	// BB (x1,y1). BOTTOM-RIGHT corner of BB. ** X-axis.
	test_cell = &LV_map[(int32)new_bb_x1 + ((int32)old_bb_y1 << 6)];

	// If the x-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_x == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, new_bb_x1, old_bb_y1, _block_x);


	// BB (x1,y1). BOTTOM-RIGHT corner of BB. ** Y-axis.
	test_cell = &LV_map[(int32)old_bb_x1 + ((int32)new_bb_y1 << 6)];

	// If the y-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_y == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, old_bb_x1, new_bb_y1, _block_y);
	

	// -------------------


	// BB (x0,y1). BOTTOM-LEFT corner of BB. ** X-axis.
	test_cell = &LV_map[(int32)new_bb_x0 + ((int32)old_bb_y1 << 6)];

	// If the x-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_x == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, new_bb_x0, old_bb_y1, _block_x);

	// BB (x0,y1). BOTTOM-LEFT corner of BB. ** Y-axis.
	test_cell = &LV_map[(int32)old_bb_x0 + ((int32)new_bb_y1 << 6)];

	// If the y-axis is already blocked skip the test.
	if ( (test_cell->cell_type != 0) && (*_block_y == 0) )
		PL_Player_Wall_Test(_new_pos_x, _new_pos_y, test_cell, old_bb_x0, new_bb_y1, _block_y);
		
		
	// -------------------
}

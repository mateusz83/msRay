#include "EN_Engine_Main.h"
#include "TM_Timer.h"
#include "LV_Level.h"
#include "GR_Game_Render.h"

// ---------------------------------------------
// --- ENGINE - PUBLIC functions definitions ---
// ---------------------------------------------
int32	EN_Init(int16 _width, int16 _height, int32 _pxf)
{
	// return at the end, how much memory was allocated
	int32 allocated_memory_size = 0;

	// prefs init
	IO_prefs.screen_width = _width;
	IO_prefs.screen_height = _height;
	IO_prefs.screen_width_center = _width >> 1;
	IO_prefs.screen_height_center = _height >> 1;
	IO_prefs.screen_pixel_format = _pxf;

	// Setup according to selected depth mode and pixel format.
	switch (IO_prefs.screen_pixel_format)
	{
		// 32 bit format
		case IO_PIXFMT_ARGB32:
		case IO_PIXFMT_BGRA32:
		case IO_PIXFMT_RGBA32:

			IO_prefs.screen_bytes_per_pixel = 4;
			IO_prefs.screen_frame_buffer_size = _width * _height * IO_prefs.screen_bytes_per_pixel;

			switch (IO_prefs.screen_pixel_format)
			{
				case IO_PIXFMT_ARGB32:
					IO_prefs.ch1 = 16;
					IO_prefs.ch2 = 8;
					IO_prefs.ch3 = 0;
					break;

				case IO_PIXFMT_BGRA32:
					IO_prefs.ch1 = 8;
					IO_prefs.ch2 = 16;
					IO_prefs.ch3 = 24;
					break;

				case IO_PIXFMT_RGBA32:
					IO_prefs.ch1 = 24;
					IO_prefs.ch2 = 16;
					IO_prefs.ch3 = 8;
					break;
			}
			break;
	}

	// Timer init - only once
	if (!TM_Init()) 
		return 0;

	// Raycaster init - only once
	int32 gr_mem_size = GR_Init_Once();
	if (!gr_mem_size)
		return 0;

	allocated_memory_size += gr_mem_size;

	// Set engine starting state...
	IO_prefs.engine_state = EN_STATE_GAMEPLAY_BEGIN;

	return allocated_memory_size;
}
void	EN_Run(int8* _main_loop)
{
	// get delta time between frames
	IO_prefs.delta_time = TM_Get_Delta_Time();

	// Raycaster loop need to be executed as fast as possible.
	// So let EN_STATE_GAMEPLAY_RUN = 0, so we can do a fast test against 0.
	if (IO_prefs.engine_state == EN_STATE_GAMEPLAY_RUN)
	{
		GR_Run();
	}
	else
	{
		switch (IO_prefs.engine_state)
		{
			case EN_STATE_GAMEPLAY_BEGIN:
				IO_prefs.engine_state = LV_Prepare_Level(0);
				GR_Reset();
				break;

			case EN_STATE_GAMEPLAY_CLEANUP:
				LV_Free_Level_Resources();
				IO_prefs.engine_state = EN_STATE_END;
				break;

			case EN_STATE_END:
				*_main_loop = 0;
				break;
		}
	}
}
void	EN_Cleanup()
{
	// free Game Render resources
	GR_Cleanup();

	// free Timer resources
	TM_Cleanup();
}

#include "GR_Info_Panel.h"
#include "BM_Bitmap.h"

#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------------------
// --- GAME RENDER - INFO PANEL - PRIVATE globals, constants, variables ---
// ------------------------------------------------------------------------
#define GR_INFO_PANEL_RELATIVE_HEIGHT		40

static int32		GR_info_panel_buffer_y_pos;
static int32		GR_info_panel_byte_size;
static int8			GR_info_panel_buffers_to_refresh;

// The info panel pixels as 32 bits.
static u_int32*		GR_info_panel__PTR;

// ---------------------------------------------------------------------
// --- GAME RENDER - INFO PANEL - PUBLIC functions definitions ---
// ---------------------------------------------------------------------
int32	GR_Info_Panel_Init_Once(void)
{
	// The info panel doesnt need to be copied on screen every frame. 
	// We can do this only ONE TIME at the beggining.
	// Because later on, we gonne be upadting only fragment of the info panels, like health or ammo etc.
	// But because the number of screen buffers can be set at starting options
	// we need to copy the info panel image at the begining to everyu screen buffer - to avoid flickering.
	// We will use helping variable that will keep informations about this.
	GR_info_panel_buffers_to_refresh = IO_prefs.screen_buffers_number;

	// Step 1.
	// Lets calculate info panel HEIGHT. The info panel WIDTH == screen_width.
	// As the reference lets take a resolutions that is 320 px width, for example: 320x180 (16:9), 320x200 (16:10), 320x240 (4:3), 320x256 (5:4).
	// For that screen width we designing a nominal, static height: GR_INFO_PANEL_RELATIVE_HEIGHT.
	// For all 320px width the info panel height will be the same. The info panel height will only change when the screen width is different.
	// The info panel aspect will always remains the same.
	GR_info_panel_height = (int16)(((float32)IO_prefs.screen_width / 320.0f) * (float32)GR_INFO_PANEL_RELATIVE_HEIGHT);


	// Step 2.
	// Lets calculate info panel starting y position on screen.
	GR_info_panel_buffer_y_pos = 0 + (IO_prefs.screen_height - GR_info_panel_height) * IO_prefs.screen_width;


	// Step 3.
	// Lets calculate the info panel size in bytes. It will be used to memcpy the image to screen.
	// We are using 32bit color.
	GR_info_panel_byte_size = IO_prefs.screen_width * GR_info_panel_height * sizeof(u_int32);


	// Step 4.
	// Check if there is a file called: (%screen_width%__info_panel__32bit.tga) in IO_GRAPHICS_DIRECTORY.
	char info_panel_filename[IO_MAX_STRING_PATH_FILENAME];
	memset(info_panel_filename, 0, IO_MAX_STRING_PATH_FILENAME);

	sprintf(info_panel_filename, "%s%d%s", IO_GRAPHICS_DIRECTORY, IO_prefs.screen_width, "__info_panel__32bit.tga");

	FILE* file_in;
	file_in = fopen(info_panel_filename, "rb");


	// Step 4a.
	// If the specified info panel for this (non common) width not exist - let create a new panel info by scaling down the big one.

	// NOTE.
	// For common and most used resolutions, like for: 320x.. 640x.. 800x.. the panels should be created and fine tuned manually.
	// Also there should be original panel in 1920x.. and smaller 1280x..

	if (file_in == NULL)
	{
		char info_panel_source_filename[IO_MAX_STRING_PATH_FILENAME];
		memset(info_panel_source_filename, 0, IO_MAX_STRING_PATH_FILENAME);

		// For he widths smaller than 1280, use 1280_panel, for bigger use 1920x..
		if (IO_prefs.screen_width < 1280)
			sprintf(info_panel_source_filename, "%s%s", IO_GRAPHICS_DIRECTORY, "1280__info_panel__32bit.tga");
		else
			sprintf(info_panel_source_filename, "%s%s", IO_GRAPHICS_DIRECTORY, "1920__info_panel__32bit.tga");

		int32 result = BM_Resize_And_Save_TGA_Bilinear_Interpolation(info_panel_source_filename, info_panel_filename, IO_prefs.screen_width, GR_info_panel_height);
		if (result <= 0) return 0;
	}
	else
		fclose(file_in);


	// Step 5.
	// Lets allocate memory for info panel image and read it.
	GR_info_panel__PTR = (u_int32*)malloc(GR_info_panel_byte_size);
	if (GR_info_panel__PTR == NULL) return 0;

	// Read TGA as BGRA
	int32 tga_result = BM_Read_TGA_32bit(info_panel_filename, GR_info_panel__PTR, GR_info_panel_byte_size, &IO_prefs);
	if (tga_result <= 0) return 0;

	return GR_info_panel_byte_size;
}
void	GR_Info_Panel_Cleanup(void)
{
	free(GR_info_panel__PTR);
}

void	GR_Info_Panel_Render(void)
{
	// The info panel need to be copied on the screen only once at the begining.
	// Later on only fragment will be updated.
	// But to avoid flickering we need to put it in every screen buffer.
	if (GR_info_panel_buffers_to_refresh > 0)
	{
		memcpy(IO_prefs.output_buffer_32 + GR_info_panel_buffer_y_pos, GR_info_panel__PTR, GR_info_panel_byte_size);
		GR_info_panel_buffers_to_refresh--;
	}
}
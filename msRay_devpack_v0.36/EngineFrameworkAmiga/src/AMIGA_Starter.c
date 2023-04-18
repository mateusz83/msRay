// AmigaOS includes.
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>

// For ReAction gadgets library.
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <clib/reaction_lib_protos.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/chooser.h>
#include <images/label.h>

#include <proto/listbrowser.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/window.h>
#include <proto/button.h>

// Standard C includes.
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ----------------------------------------
// --- AMIGA Starter - global variables ---
// ----------------------------------------
static const char 	*STARTER_version =	"$VER: Version 0.36";

#define STARTER_WINDOW_NAME         	"msRay (v0.36), Amiga Starter."

struct Window* 		STARTER_window;
struct Screen* 		STARTER_pubscreen;  
APTR   				STARTER_visual_info;	

// Values to be exported as input arguments for Framework/Engine.
ULONG	EXPORT_display_mode, EXPORT_mode_id, EXPORT_width, EXPORT_height, EXPORT_buffers;

UBYTE 	is_RTG_lib, is_RTG_32bit, is_HAM, is_Reaction;
UWORD	count_ARGB32, count_BGRA32, count_RGBA32;
 
 // For CGX/RTG graphics.
struct Library* CyberGfxBase = NULL;

// All GUI gadgets numerations ids.
enum { GID_GADGETS_LIST, GID_START_BUTTON, GID_HELP_BUTTON, GID_SCREEN_LIST, GID_MODE_CHOOSER, GID_ASPECT_CHOOSER, GID_PIXEL_CHOOSER, GID_BUFFER_CHOOSER, GID_LAST };

// All GUI gadgets structure holder.
struct Gadget *G_gadgets[GID_LAST];

// All GUI gadgets strings for cycle/chooser.
const char* G_mode_strings[] 				= { "Fullscreen (32bit)", "Fullscreen (HAM8)", NULL };
const char* G_aspect_fullscreen_strings[] 	= { "Same as System", "16:9", "16:10", "4:3", "5:4", "Show all", NULL };
const char* G_aspect_window_strings[] 		= { "Same as System", "16:9", "16:10", "4:3", "5:4", NULL };
const char* G_pixel_strings[] 				= { "ARGB", "BGRA", "RGBA", NULL };
const char* G_buffering_strings_3[] 		= { "Single", "Double (vsync)", "Triple", NULL };
const char* G_buffering_strings_2[] 		= { "Single", "Double (vsync)", NULL };


// All GUI gadgets helper variables, to keep states for chooser/cycle gadgets.
enum { G_MODE_FULLSCREEN_32BIT, G_MODE_HAM };
enum { G_ASPECT_SYSTEM, G_ASPECT_16x9, G_ASPECT_16x10, G_ASPECT_4x3, G_ASPECT_5x4, G_ASPECT_ALL };
enum { G_PIXEL_ARGB, G_PIXEL_BGRA, G_PIXEL_RGBA };
enum { G_BUFFER_SINGLE, G_BUFFER_DOUBLE, G_BUFFER_TRIPLE };

ULONG		G_mode_selection, G_aspect_selection, G_pixel_selection, G_buffer_selection, G_start_selection;
struct List G_screen_list; 	

// Only for ReAction gadgets.
struct Library*     WindowBase 			= NULL;
struct Library*     LayoutBase 			= NULL;
struct Library*     LabelBase 			= NULL;
struct Library*		ButtonBase 			= NULL;
struct Library*		ListBrowserBase 	= NULL;
struct Library*		ChooserBase 		= NULL;

Object* 			REACTION_window_object;
struct List			REACTION_list_browser_labels, REACTION_mode_labels, REACTION_aspect_fullscreen_labels, 
					REACTION_aspect_window_labels, REACTION_pixel_labels, REACTION_buffering_labels_3, REACTION_buffering_labels_2;

// Stucture describing columns for ReAction list view gadget.
struct ColumnInfo 	REACTION_lb_column_info[] =
{
	{  38, "Driver", 0  },
	{  62, "Resolution", 0 },
	{ -1, (STRPTR)~0, -1 }
};

// A helper custom Node structure that will hold informations for single screen mode.
typedef struct
{
    struct Node node; 

	// Keep mode_id, screen width and height as numbers (for Export).
	ULONG mode_id, width, height;

	// Keep separete strings for driver name and width x height (for ReAction).
	BYTE driver_string[16];
	BYTE resolution_string[16];

	// Also keep combined driver name and width x height as one string (for Intuition).
	BYTE full_info_string[32];    

} sMY_Data_Node;

// ----------------------------------------------
// --- AMIGA Starter - functions DECLARATIONS ---
// ----------------------------------------------
UBYTE 	STARTER_Init();
void 	STARTER_Cleanup();
void 	STARTER_Message_Box(const char* _title, const char* _info, const char* _options);

UBYTE 	Test_RTG(void);
UBYTE	Test_32bit_Modes(void);
UBYTE	Test_HAM_Modes(void);

ULONG 	Get_Fullscreen_32bit_Modes(void);
ULONG 	Get_HAM_Modes(void);

void	Add_To_Screen_List(ULONG _mode_id, ULONG _width, ULONG _height, const char* _driver);
void 	Add_To_Screen_List_Empty(void);
void	Clear_Screen_List(void);

void	Get_Export_Values();

void	GUI_Update(void);
void	GUI_Show_Help(void);

UBYTE 	GUI_REACTION_Init(void);
UBYTE 	GUI_REACTION_Create(void);
void 	GUI_REACTION_Message_Loop(void);
void	GUI_REACTION_Convert_List(void);
void	GUI_REACTION_Strings_To_Chooser_Labels(const char* _strings[], struct List* _list);
void	GUI_REACTION_Clear_Chooser_Labels(struct List* _list);

UBYTE 	GUI_INTUITION_Create(void);
void 	GUI_INTUITION_Message_Loop(void);

// --------------------------------------------
// --- AMIGA Starter - main() - entry point ---
// --------------------------------------------
int main(void)
{
	// Test what elements are available on this system, and try to init them.
	if (!STARTER_Init())
	{
		// If initialisation went wrong, clean up the resources and end application.
		STARTER_Cleanup();
		return 0;
	}

	// Run Message Loop.
    if (is_Reaction)	GUI_REACTION_Message_Loop();
    else		        GUI_INTUITION_Message_Loop();

	// After ending Message Loop, cleanup the resources.
	STARTER_Cleanup();

	// -----------------------------------------------------------------------
	// --- EXECUTING Framework/Engine application with EXPORTED arguments. ---
	// -----------------------------------------------------------------------

	// If START button pushed.
	if(G_start_selection)
	{
		// Lets first check if the framework/engine/game file exist.
		char framework_filename[] = "data/frm060_v036.exe";

		FILE* file_test = fopen(framework_filename, "r");

		if (file_test == NULL)
		{
			char tmp_info[64];
			memset(tmp_info, 0, sizeof(tmp_info));	
			sprintf(tmp_info, "Can't find framework/engine file: %s", framework_filename);

			STARTER_Message_Box(STARTER_WINDOW_NAME, tmp_info, "OK");
			return 0;
		}
		else fclose(file_test);

		// Lets prepare string variable that will contain selected framework exe filename and selected parameters:
		// display mode, mode_id, screen width, screen height and buffers number.
		char filename_with_arguments[64];
		memset(filename_with_arguments, 0, sizeof(filename_with_arguments));	

		sprintf(filename_with_arguments, "%s %u %u %u %u %u", framework_filename, EXPORT_display_mode, EXPORT_mode_id, EXPORT_width, EXPORT_height, EXPORT_buffers);
		
		// After everything is closed and cleanup-ed - start Framework/Engine - according to selected SCREEN MODE:
		Execute(filename_with_arguments, 0, 0);
	}

	return 0;
}

// -----------------------------
// --- Functions DEFINITIONS ---
// -----------------------------
UBYTE 	STARTER_Init()
{
	// ------------------------------
	// --- GRAPHICS INIT AND TEST ---
	// ------------------------------

	// Test if CGX or RTG library is in the system.
 	is_RTG_lib = Test_RTG();

	if (is_RTG_lib != 0) 
	{
		// Test are any 32bit modes definied.
		is_RTG_32bit = Test_32bit_Modes();
	}

	// Now lets test if there is a native Amiga resolution suitable for HAM mode.
	// For now lets consider only 1280x256 for 320x256 game screen.
	is_HAM = Test_HAM_Modes();

	// Lets summarize the informations about CGX/RTG and HAM and prompt message if neede.
	
	if	(!is_RTG_lib && !is_HAM) 	
	{
		// If both CGX/RTG and HAM was not found, prompt message and exit application.
		STARTER_Message_Box(STARTER_WINDOW_NAME, "I couldn't open cybergraphics.library\nor rtg.library or their equivalent on this System.\nTherefore, the recommended 32-bit modes\nare not available.\n\nI was also unable to find modes\nthat allow the display in HAM mode.", "OK");
		return 0;
	}
	else if	(!is_RTG_32bit && !is_HAM) 	
	{
		// If 32 bit modes and HAM was not found, prompt message and exit application.
		STARTER_Message_Box(STARTER_WINDOW_NAME, "I haven't found any defined modes for 32-bit graphics on this System.\nTherefore, the recommended 32-bit modes\nare not available.\n\nI was also unable to find modes\nthat allow the display in HAM mode.", "OK");
		return 0;
	}
	else if	(!is_RTG_lib && is_HAM) 	
	{
		// If CGX/RTG was not found but HAM is available, prompt message.
		STARTER_Message_Box(STARTER_WINDOW_NAME, "I couldn't open cybergraphics.library\nor rtg.library or their equivalent on this System.\nTherefore, the recommended 32-bit modes\nare not available.\n\nOnly HAM mode will be available.", "OK");
		G_mode_selection = G_MODE_HAM;
	}
	else if	(!is_RTG_32bit && !is_HAM) 	
	{
		// If 32 bit was not found but HAM is available, prompt message.
		STARTER_Message_Box(STARTER_WINDOW_NAME, "I haven't found any defined modes for 32-bit graphics on this System.\nTherefore, the recommended 32-bit modes\nare not available.\n\nOnly HAM mode will be available.", "OK");
		G_mode_selection = G_MODE_HAM;
	}

	// ----------------
	// --- GUI INIT ---
	// ----------------

	// Get pointer to Workbench public screen.
	if (!(STARTER_pubscreen = LockPubScreen(NULL))) 
	{
		STARTER_Message_Box(STARTER_WINDOW_NAME, "I couldn't lock Workbench public screen.", "OK");
		return 0;
	}

	// Get visual info handler.
	STARTER_visual_info = GetVisualInfo(STARTER_pubscreen, TAG_END);

	// Init list.
	Clear_Screen_List();

	// Now lets test if ReAction GUI library is installed on this System.
	// If not we will use standard Intuition GUI.
	is_Reaction = GUI_REACTION_Init();

	// If ReAction is not available lets try use Intuition instead.
	if(is_Reaction == 0)
	{
		if (!GUI_INTUITION_Create())
        {
			// If we couldn't also create Intuition GUI, just exit the app.
            STARTER_Message_Box(STARTER_WINDOW_NAME, "I couldn't create standard Intuition window and gadgets.\nSomething might be wrong with this System.", "OK");
            return 0;
        }
	}
	else
    {
        // Lets try init the ReAction GUI.
        if (!GUI_REACTION_Create())
        {
            // If creating ReAction GUI is impossible, try create standard Intuition window.
			is_Reaction = 0;

            if (!GUI_INTUITION_Create())
            {
				// If we couldn't also create Intuition GUI, just exit the app.
                STARTER_Message_Box(STARTER_WINDOW_NAME, "I couldn't create standard Intuition window and gadgets.\nSomething might be wrong with this System.", "OK");
                return 0;
            }
        }
    }

    return 1;
}

void 	STARTER_Cleanup()
{	
	Clear_Screen_List();

	// Clean up ReAction things.
	if (is_Reaction)
	{
		DisposeObject(REACTION_window_object);

		GUI_REACTION_Clear_Chooser_Labels(&REACTION_mode_labels);
		GUI_REACTION_Clear_Chooser_Labels(&REACTION_aspect_fullscreen_labels);
		GUI_REACTION_Clear_Chooser_Labels(&REACTION_aspect_window_labels);
		GUI_REACTION_Clear_Chooser_Labels(&REACTION_pixel_labels);
		GUI_REACTION_Clear_Chooser_Labels(&REACTION_buffering_labels_3);
		GUI_REACTION_Clear_Chooser_Labels(&REACTION_buffering_labels_2);
	}
	else
	{
		CloseWindow(STARTER_window);		
		FreeGadgets(G_gadgets[GID_GADGETS_LIST]);
	}

	// Close ReAction libraries.
	CloseLibrary(ChooserBase);
	CloseLibrary(ListBrowserBase);
	CloseLibrary(ListBrowserBase);
	CloseLibrary(LabelBase);
	CloseLibrary(LayoutBase);
	CloseLibrary(WindowBase);

  	// Free visual info.
    FreeVisualInfo(STARTER_visual_info);

	// Unlock Worckbench public screen.
	UnlockPubScreen(NULL, STARTER_pubscreen);	

    // Close CGX/RTG library.
	CloseLibrary(CyberGfxBase);
}

void 	STARTER_Message_Box(const char* _title, const char* _info, const char* _options)
{
    struct EasyStruct tmp_ES =
    {
        sizeof(struct EasyStruct),
        0,
        (STRPTR)_title,
        (STRPTR)_info,
        (STRPTR)_options,
    };

    EasyRequest(NULL, &tmp_ES, NULL);
}


UBYTE 	Test_RTG(void)
{
    // Try open cybergraphics library.
    CyberGfxBase = (struct Library*)OpenLibrary("cybergraphics.library", 41L);

    // If no CGX/RTG system found, return 0.
	if (CyberGfxBase == NULL)
        return 0;

	return 1;
}

UBYTE	Test_32bit_Modes(void)
{
	// Lets count all 32bit modes.
	ULONG count = 0;

	// Lets go thru all CGX/RTG available graphic modes.
 	ULONG mode_id = INVALID_ID;

 	while ( (mode_id = NextDisplayInfo(mode_id)) != INVALID_ID)
 	{
		if (IsCyberModeID(mode_id))
		{
			// Let get cgx id attribute.
			ULONG cgx_pixel_format = GetCyberIDAttr(CYBRIDATTR_PIXFMT, mode_id);

			// For 32 bit modes we are only interested in cgx_id_attr = PIXFMT_ARGB32, PIXFMT_BGRA32 or PIXFMT_RGBA32.
			if (cgx_pixel_format != PIXFMT_ARGB32 && cgx_pixel_format != PIXFMT_BGRA32 && cgx_pixel_format != PIXFMT_RGBA32)
				continue;

			// Lets count valid modes.
			count++;

			// Lets count different modes.
			switch(cgx_pixel_format)
			{
				case PIXFMT_ARGB32:
					count_ARGB32++;
					break;

				case PIXFMT_BGRA32:
					count_BGRA32++;
					break;

				case PIXFMT_RGBA32:
					count_RGBA32++;
					break;
			}
		}
	}

	// We should avoid to show empty list at the begining. Lets test number of modes.
	// We prefer ARGB as default, if no just select next that has at least 1 mode.
	if 		(count_ARGB32 > 0)	G_pixel_selection = G_PIXEL_ARGB;
	else if	(count_BGRA32 > 0)	G_pixel_selection = G_PIXEL_BGRA;
	else						G_pixel_selection = G_PIXEL_ARGB;

	if (count > 0) 	return 1;
	else 			return 0;
}

UBYTE	Test_HAM_Modes(void)
{
	// Lets go thru all available modes.
	// But we are interested only in native Amiga chipset modes that suppports HAM mode.
 	ULONG mode_id = INVALID_ID;

 	while ( (mode_id = NextDisplayInfo(mode_id)) != INVALID_ID)
 	{
		struct NameInfo name_info;
		struct DisplayInfo display_info;

 		if (GetDisplayInfoData(NULL, (UBYTE*)&name_info, sizeof(struct NameInfo), DTAG_NAME, mode_id))
  		{
        	GetDisplayInfoData(NULL, (UBYTE*)&display_info, sizeof(struct DisplayInfo), DTAG_DISP, mode_id);

			if (display_info.PropertyFlags & DIPF_IS_PAL)
			{
  				struct DimensionInfo dn_Dimensioninfo;

 				if (!(display_info.NotAvailable)) 
 				{
	    			GetDisplayInfoData(NULL, (UBYTE*)&dn_Dimensioninfo, sizeof(struct DimensionInfo), DTAG_DIMS, mode_id);

					// Return TRUE if at least one of these modes found: 1280x256 or 640x256.

					if ( ((dn_Dimensioninfo.Nominal.MaxX + 1) == 1280 && (dn_Dimensioninfo.Nominal.MaxY + 1) == 256) || 
						 ((dn_Dimensioninfo.Nominal.MaxX + 1) == 640 && (dn_Dimensioninfo.Nominal.MaxY + 1) == 256)	)
						return 1;
				}
			}
		}
	}

	return 0;
}

ULONG 	Get_Fullscreen_32bit_Modes(void)
{
	if (!is_RTG_32bit) 
	{
		Add_To_Screen_List_Empty();
		return 0;
	}

	// Lets count the 32bit modes that we are interested in.
	ULONG count = 0;

	// What pixel format is currently selected (in cgx specification).
	ULONG selected_pixel_format = 0;

	switch(G_pixel_selection)
	{
		case G_PIXEL_ARGB:
			selected_pixel_format = PIXFMT_ARGB32;
			break;

		case G_PIXEL_BGRA:
			selected_pixel_format = PIXFMT_BGRA32;
			break;

		case G_PIXEL_RGBA:
			selected_pixel_format = PIXFMT_RGBA32;
			break;
	}	

	// What is the value of screen aspect.
	float selected_aspect_value = 0.0f;

	switch(G_aspect_selection)
	{
		case G_ASPECT_SYSTEM:
			{
				// Getting system/workbench screen parameters.
				struct Screen* pub_screen = LockPubScreen(NULL);

				selected_aspect_value = (float)pub_screen->Width / (float)pub_screen->Height;

				UnlockPubScreen(NULL, pub_screen);	
			}
			break;

		case G_ASPECT_16x9:
			selected_aspect_value = 16.0f / 9.0f;
			break;

		case G_ASPECT_16x10:
			selected_aspect_value = 16.0f / 10.0f;
			break;

		case G_ASPECT_4x3:
			selected_aspect_value = 4.0f / 3.0f;
			break;
			
		case G_ASPECT_5x4:
			selected_aspect_value = 5.0f / 4.0f;
			break;
	}	

	// Lets go thru all CGX/RTG available graphic modes.
 	ULONG mode_id = INVALID_ID;

 	while ( (mode_id = NextDisplayInfo(mode_id)) != INVALID_ID)
 	{
		if (IsCyberModeID(mode_id))
		{
			// Lets get it parameters.
			ULONG cgx_pixel_format = GetCyberIDAttr(CYBRIDATTR_PIXFMT, mode_id);
			ULONG cgx_width = GetCyberIDAttr(CYBRIDATTR_WIDTH, mode_id);
			ULONG cgx_height = GetCyberIDAttr(CYBRIDATTR_HEIGHT, mode_id);

			UBYTE result = 0;

			// Lest test if the parameters fits our selected specifications.
			if (cgx_pixel_format == selected_pixel_format)
			{
				if (G_aspect_selection == G_ASPECT_ALL)
					result = 1;
				else
				{
					float cgx_aspect = (float)cgx_width / (float)cgx_height;

					if ( cgx_aspect >= (selected_aspect_value - 0.0001f) && cgx_aspect <= (selected_aspect_value + 0.0001f) )
						result = 1;
				}

				if (result)
				{
						// If so add that mode to the list.
						count++;

						// Get full name - so we can extract Driver Name.
 						struct NameInfo name_info;

						DisplayInfoHandle display_info_handler = FindDisplayInfo(mode_id);
						GetDisplayInfoData(display_info_handler, (APTR)&name_info, sizeof (struct NameInfo), DTAG_NAME, INVALID_ID);

						Add_To_Screen_List(mode_id, cgx_width, cgx_height, name_info.Name);
				}
			}
		}
	}

	if (count == 0)
		Add_To_Screen_List_Empty();
	
	return count;
}

ULONG 	Get_HAM_Modes(void)
{
	// Lets count modes that we are interested in.
	ULONG count = 0;

	// Lets go thru all available modes.
	// But we are interested only in native Amiga chipset modes that suppports HAM mode.
 	ULONG mode_id = INVALID_ID;

	// 1280x256 and 640x256 repeats twice, so lets add them only once.
	UBYTE check_1280 = 0, check_640 = 0;

 	while ( (mode_id = NextDisplayInfo(mode_id)) != INVALID_ID)
 	{
		struct NameInfo name_info;
		struct DisplayInfo display_info;

 		if (GetDisplayInfoData(NULL, (UBYTE*)&name_info, sizeof(struct NameInfo), DTAG_NAME, mode_id))
  		{
        	GetDisplayInfoData(NULL, (UBYTE*)&display_info, sizeof(struct DisplayInfo), DTAG_DISP, mode_id);

			if (display_info.PropertyFlags & DIPF_IS_PAL)
			{
  				struct DimensionInfo dn_Dimensioninfo;

 				if (!(display_info.NotAvailable)) 
 				{
	    			GetDisplayInfoData(NULL, (UBYTE*)&dn_Dimensioninfo, sizeof(struct DimensionInfo), DTAG_DIMS, mode_id);

					// For PAL 1280x256 screen size, lets create 3 sizes: 320x256, 320x240 and 320x180.
					if ( ((dn_Dimensioninfo.Nominal.MaxX + 1) == 1280 && (dn_Dimensioninfo.Nominal.MaxY + 1) == 256) && check_1280 == 0)
					{
						Add_To_Screen_List(mode_id, 320, 180, name_info.Name);
						Add_To_Screen_List(mode_id, 320, 240, name_info.Name);
						Add_To_Screen_List(mode_id, 320, 256, name_info.Name);
						count += 3;
						check_1280 = 1;
					}

					// For PAL 640x256 screen size, lets create 1 size: 160x256.
					if ( ((dn_Dimensioninfo.Nominal.MaxX + 1) == 640 && (dn_Dimensioninfo.Nominal.MaxY + 1) == 256)	 && check_640 == 0 )
					{
						Add_To_Screen_List(mode_id, 160, 256, name_info.Name);
						count++;
						check_640 = 1;
					}					
				}
			}
		}
	}

	if (count == 0)
		Add_To_Screen_List_Empty();
	
	return count;
}


void	Add_To_Screen_List(ULONG _mode_id, ULONG _width, ULONG _height, const char* _driver)
{
	char buffer_driver[16];
	memset(buffer_driver, 0, sizeof(buffer_driver));

	// Extract first chars (the driver name is at the beginning and ends with ':' )
	for (UBYTE i = 0; i < 16; i++)
	{
		if ( _driver[i] == ':')
		break;

		buffer_driver[i] = _driver[i];
	}

	char buffer_resolution[16];
	memset(buffer_resolution, 0, sizeof(buffer_resolution));
	sprintf(buffer_resolution, "%d x %d", _width, _height);

	char buffer_full_info[32];
	memset(buffer_full_info, 0, 32);		
	sprintf(buffer_full_info, "%s:   %d x %d", buffer_driver, _width, _height);

 	sMY_Data_Node* my_data_node = (sMY_Data_Node*)malloc(sizeof(sMY_Data_Node));

	if (my_data_node)
	{
		my_data_node->node.ln_Type = 100L;
		my_data_node->node.ln_Pri = 0;

		my_data_node->mode_id = _mode_id;
		my_data_node->width = _width;
		my_data_node->height = _height;

		memcpy(my_data_node->driver_string, buffer_driver, 16);
		memcpy(my_data_node->resolution_string, buffer_resolution, 16);

		memcpy(my_data_node->full_info_string, buffer_full_info, 32);	
		my_data_node->node.ln_Name = my_data_node->full_info_string;

		AddTail(&G_screen_list, (struct Node*)my_data_node);																		
	}                  
}

void	Add_To_Screen_List_Empty(void)
{
	char buffer[32];
	memset(buffer, 0, 32);		
	sprintf(buffer, "-empty-");

 	sMY_Data_Node* my_data_node = (sMY_Data_Node*)malloc(sizeof(sMY_Data_Node));

	if (my_data_node)
	{
		memcpy(my_data_node->full_info_string, buffer, 32);	
		my_data_node->node.ln_Name = my_data_node->full_info_string;
		my_data_node->node.ln_Type = 100L;
		my_data_node->node.ln_Pri = 0;
		my_data_node->mode_id = INVALID_ID;

		memcpy(my_data_node->resolution_string, "-empty-", 16 );
		memcpy(my_data_node->driver_string, "", 10);

		AddTail(&G_screen_list, (struct Node*)my_data_node);																		
	}                  
}

void	Clear_Screen_List(void)
{
    // Free main list and nodes.
  	sMY_Data_Node *worknode;
  	sMY_Data_Node *nextnode;

    worknode = (sMY_Data_Node*)(G_screen_list.lh_Head);

    while ( (nextnode = (sMY_Data_Node*)(worknode->node.ln_Succ)) ) 
	{
        Remove((struct Node*)worknode);
        worknode = nextnode;
    }

	NewList(&G_screen_list);

	// For ReAction clear its list.
	if (is_Reaction)
	{
		FreeListBrowserList(&REACTION_list_browser_labels);		
		NewList(&REACTION_list_browser_labels);
	}
}


void	Get_Export_Values()
{
	// Lets save values to be exported as input argument for the Framework/Engine.

	// Selected index in list browser.
	ULONG selected = 0;

	if  (is_Reaction)
	{
		GetAttr(LISTBROWSER_Selected, G_gadgets[GID_SCREEN_LIST], (ULONG*)&selected);

		GetAttr(CHOOSER_Selected, G_gadgets[GID_MODE_CHOOSER], (ULONG*)&EXPORT_display_mode);
		GetAttr(CHOOSER_Selected, G_gadgets[GID_BUFFER_CHOOSER], (ULONG*)&EXPORT_buffers);
	}
	else
	{
		GT_GetGadgetAttrs(G_gadgets[GID_SCREEN_LIST], STARTER_window, NULL, GTLV_Selected, (ULONG)&selected, TAG_DONE);

		GT_GetGadgetAttrs(G_gadgets[GID_MODE_CHOOSER], STARTER_window, NULL, GTCY_Active, (ULONG)&EXPORT_display_mode, TAG_DONE);
		GT_GetGadgetAttrs(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL, GTCY_Active, (ULONG)&EXPORT_buffers, TAG_DONE);
	}
	
	// Lets add "1" to EXPORT_buffers, 
	// so the number will contain the real number of selected buffers not just the index number.
	EXPORT_buffers++;

	// Scan the screen list to find selected node.
	sMY_Data_Node* tmp_my_data_node;
    
	struct List tmp_list;
    tmp_list = G_screen_list;

	ULONG counter = 0;

	for (tmp_my_data_node = (APTR)tmp_list.lh_Head; tmp_my_data_node->node.ln_Succ != NULL; tmp_my_data_node = (APTR)tmp_my_data_node->node.ln_Succ)
	{
		if (counter == selected) 
        {
        	//get selected display ID and exit main loop
            EXPORT_mode_id = tmp_my_data_node->mode_id;	
			EXPORT_width = tmp_my_data_node->width;
			EXPORT_height = tmp_my_data_node->height;
            break;
        }
		counter++;		
	}				
}


void	GUI_Update(void)
{
	if  (is_Reaction)
	{
		// Disconnect the list from list browser.
 		SetAttrs(G_gadgets[GID_SCREEN_LIST], LISTBROWSER_Labels , (ULONG)-1, TAG_END);	

		// Get selected Display Mode and make changes accorgind to it.
		GetAttr(CHOOSER_Selected, G_gadgets[GID_MODE_CHOOSER], (ULONG*)&G_mode_selection);
		GetAttr(CHOOSER_Selected, G_gadgets[GID_ASPECT_CHOOSER], (ULONG*)&G_aspect_selection);
		GetAttr(CHOOSER_Selected, G_gadgets[GID_PIXEL_CHOOSER], (ULONG*)&G_pixel_selection);
	}
	else
	{
		// Disconnect the list from list browser.
		GT_SetGadgetAttrs(G_gadgets[GID_SCREEN_LIST], STARTER_window, NULL, GTLV_Labels, -1, TAG_END);	

		// Get selected Display Mode and make changes accorgind to it.
		GT_GetGadgetAttrs(G_gadgets[GID_MODE_CHOOSER], STARTER_window, NULL, GTCY_Active, (ULONG)&G_mode_selection, TAG_DONE);
		GT_GetGadgetAttrs(G_gadgets[GID_ASPECT_CHOOSER], STARTER_window, NULL, GTCY_Active, (ULONG)&G_aspect_selection, TAG_DONE);
		GT_GetGadgetAttrs(G_gadgets[GID_PIXEL_CHOOSER], STARTER_window, NULL, GTCY_Active, (ULONG)&G_pixel_selection, TAG_DONE);
	}
	
	// Clear old list.
	Clear_Screen_List();

	// Update DISPLAY MODES LIST and GUI according to selected parameters.
	// For ReAction and Intuition we can use the same OnGadget/OffGadget funtions to enable/disable gadgets.
	switch(G_mode_selection)
	{
		case G_MODE_FULLSCREEN_32BIT:
		{
			// What gadgets to show/hide.
			OnGadget(G_gadgets[GID_ASPECT_CHOOSER], STARTER_window, NULL);
			OnGadget(G_gadgets[GID_PIXEL_CHOOSER], STARTER_window, NULL);
			OnGadget(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL);	

			// Update CHOOSER/CYCLE strings
			if (is_Reaction) 	
			{
				SetAttrs(G_gadgets[GID_ASPECT_CHOOSER], CHOOSER_Labels, (ULONG)&REACTION_aspect_fullscreen_labels, TAG_END);
				SetAttrs(G_gadgets[GID_BUFFER_CHOOSER], CHOOSER_Labels, (ULONG)&REACTION_buffering_labels_3, TAG_END);
			}
			else
			{
				GT_SetGadgetAttrs(G_gadgets[GID_ASPECT_CHOOSER], STARTER_window, NULL, GTCY_Labels, (ULONG)G_aspect_fullscreen_strings, TAG_END);	
				GT_SetGadgetAttrs(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL, GTCY_Labels, (ULONG)G_buffering_strings_3, TAG_END);
			}

			// Generate new list.
			if (Get_Fullscreen_32bit_Modes() == 0) 	OffGadget(G_gadgets[GID_START_BUTTON], STARTER_window, NULL);
			else 									OnGadget(G_gadgets[GID_START_BUTTON], STARTER_window, NULL);
		}
		break;

		case G_MODE_HAM:
		{
			// What gadgets to show/hide.
			OffGadget(G_gadgets[GID_ASPECT_CHOOSER], STARTER_window, NULL);
			OffGadget(G_gadgets[GID_PIXEL_CHOOSER], STARTER_window, NULL);
			OnGadget(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL);	

			// Update CHOOSER/CYCLE strings
			if (is_Reaction) 	
			{
				SetAttrs(G_gadgets[GID_BUFFER_CHOOSER], CHOOSER_Labels, (ULONG)&REACTION_buffering_labels_2, TAG_END);
			}
			else
			{
				GT_SetGadgetAttrs(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL, GTCY_Labels, (ULONG)G_buffering_strings_2, TAG_END);
			}

			// Generate new list.
			if (Get_HAM_Modes() == 0)	OffGadget(G_gadgets[GID_START_BUTTON], STARTER_window, NULL);
			else						OnGadget(G_gadgets[GID_START_BUTTON], STARTER_window, NULL);
		}
		break;
	}

	if (is_Reaction)
	{
		// For ReAction we need to re-format data from main list into a new list.
		GUI_REACTION_Convert_List();

		// Re-connect the list to list browser.
		SetAttrs(G_gadgets[GID_SCREEN_LIST], LISTBROWSER_Labels , (ULONG)&REACTION_list_browser_labels, TAG_END);	
		SetAttrs(G_gadgets[GID_SCREEN_LIST], LISTBROWSER_Selected, (ULONG)0, TAG_END);

		// Refresh the list browser.
		RefreshGadgets(G_gadgets[GID_SCREEN_LIST], STARTER_window, NULL);		

		// Refresh aspect chooser.
		RefreshGadgets(G_gadgets[GID_ASPECT_CHOOSER], STARTER_window, NULL);	

		// Refresh bufffering chooser.
		RefreshGadgets(G_gadgets[GID_BUFFER_CHOOSER], STARTER_window, NULL);	
	}
	else
	{	
		// Re-connect the list to list browser.
		GT_SetGadgetAttrs(G_gadgets[GID_SCREEN_LIST], STARTER_window, NULL, GTLV_Labels, (ULONG)&G_screen_list, TAG_END);	
		GT_SetGadgetAttrs(G_gadgets[GID_SCREEN_LIST], STARTER_window, NULL, GTLV_Selected, 0, TAG_END);
	}
}

void	GUI_Show_Help(void)
{
	// Lets first check if the framework/engine/game file exist.
	char help_filename[] = "amiga_help.guide";
	char help_path_filename[] = "data/docs/amiga_help.guide";

	FILE* file_test = fopen(help_path_filename, "r");

	if (file_test == NULL)
	{
		char tmp_info[64];
		memset(tmp_info, 0, sizeof(tmp_info));	
		sprintf(tmp_info, "Can't find Help file: %s", help_path_filename);

		STARTER_Message_Box(STARTER_WINDOW_NAME, tmp_info, "OK");
		return;
	}
	else fclose(file_test);

	// The help file is in AmigaGuide format and should be opened from "sys:utilities/multiview".
	char command_to_execute[128];
	memset(command_to_execute, 0, sizeof(command_to_execute));	

	sprintf(command_to_execute, "cd data/docs\nsys:utilities/multiview %s", help_filename);

	// After everything is closed and cleanup-ed - start Framework/Engine - according to selected SCREEN MODE:
	Execute(command_to_execute, 0, 0);
}


UBYTE 	GUI_REACTION_Init(void)
{
    // Lets test if the system has ReAction Libraries installed so we can use modern GUI
    WindowBase  	= OpenLibrary("window.class", 0L);
    LayoutBase  	= OpenLibrary("gadgets/layout.gadget", 0L);
    LabelBase   	= OpenLibrary("images/label.image", 0L);
	ButtonBase		= OpenLibrary("gadgets/button.gadget", 0L);
	ListBrowserBase	= OpenLibrary("gadgets/listbrowser.gadget", 0L);
	ChooserBase		= OpenLibrary("gadgets/chooser.gadget", 0L);

    if (WindowBase == NULL || LayoutBase == NULL || LabelBase == NULL || ButtonBase	== NULL || ListBrowserBase == NULL || ChooserBase == NULL)
		return 0;

	return 1;
}

UBYTE 	GUI_REACTION_Create(void)
{
	GUI_REACTION_Strings_To_Chooser_Labels(G_mode_strings, &REACTION_mode_labels);
	GUI_REACTION_Strings_To_Chooser_Labels(G_aspect_fullscreen_strings, &REACTION_aspect_fullscreen_labels);
	GUI_REACTION_Strings_To_Chooser_Labels(G_aspect_window_strings, &REACTION_aspect_window_labels);
	GUI_REACTION_Strings_To_Chooser_Labels(G_pixel_strings, &REACTION_pixel_labels);
	GUI_REACTION_Strings_To_Chooser_Labels(G_buffering_strings_3, &REACTION_buffering_labels_3);
	GUI_REACTION_Strings_To_Chooser_Labels(G_buffering_strings_2, &REACTION_buffering_labels_2);

	ULONG font_width = STARTER_pubscreen->RastPort.TxWidth;

	// Create ReAction gadgets.
	REACTION_window_object = 							
							NewObject(WINDOW_GetClass(), NULL,
								WA_Width, font_width * 50,
								WA_ScreenTitle, (ULONG)STARTER_WINDOW_NAME,
								WA_Title, (ULONG)STARTER_WINDOW_NAME,
								WA_Activate, TRUE,
								WA_DragBar, TRUE,
								WA_DepthGadget, TRUE,
								WA_CloseGadget, TRUE,
								WA_SmartRefresh, TRUE,	

								WINDOW_IconTitle, (ULONG)STARTER_WINDOW_NAME,
								WINDOW_Position, WPOS_CENTERSCREEN,	

								WINDOW_ParentGroup, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
									LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
									LAYOUT_DeferLayout, TRUE,

									// SPLIT INTO TWO COLUMNS
									LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
										LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
										LAYOUT_SpaceOuter, TRUE,

										// LEFT SIDE
										LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
											LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

											// display mode
											LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
												LAYOUT_BevelStyle, BVS_SBAR_VERT,
												LAYOUT_Label, (ULONG)"Display mode:",

												LAYOUT_AddChild, (ULONG)(G_gadgets[GID_MODE_CHOOSER] = NewObject(CHOOSER_GetClass(), NULL, 
													GA_ID, GID_MODE_CHOOSER,
													GA_RelVerify, TRUE,
													CHOOSER_PopUp, TRUE,
													CHOOSER_Justification, CHJ_CENTER,
													CHOOSER_Labels, (ULONG)&REACTION_mode_labels,
													CHOOSER_Selected, (ULONG)G_mode_selection,
												TAG_END)),	
												CHILD_MinHeight, 23,	

												LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												TAG_END),									
	
											TAG_END),	
										
											// screen aspect
											LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
												LAYOUT_BevelStyle, BVS_SBAR_VERT,
												LAYOUT_Label, (ULONG)"Screen aspect:",

												LAYOUT_AddChild, (ULONG)(G_gadgets[GID_ASPECT_CHOOSER] = NewObject(CHOOSER_GetClass(), NULL, 
													GA_ID, GID_ASPECT_CHOOSER,
													GA_RelVerify, TRUE,
													CHOOSER_PopUp, TRUE,		
													CHOOSER_Justification, CHJ_CENTER,						
													CHOOSER_Labels, (ULONG)&REACTION_aspect_fullscreen_labels,
													CHOOSER_Selected, (ULONG)0,
												TAG_END)),		
												CHILD_MinHeight, 23,	

												LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												TAG_END),											
	
											TAG_END),

											// pixel format
											LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
												LAYOUT_BevelStyle, BVS_SBAR_VERT,
												LAYOUT_Label, (ULONG)"Pixel format:",

												LAYOUT_AddChild, (ULONG)(G_gadgets[GID_PIXEL_CHOOSER] = NewObject(CHOOSER_GetClass(), NULL, 
													GA_ID, GID_PIXEL_CHOOSER,
													GA_RelVerify, TRUE,
													CHOOSER_PopUp, TRUE,			
													CHOOSER_Justification, CHJ_CENTER,					
													CHOOSER_Labels, (ULONG)&REACTION_pixel_labels,
													CHOOSER_Selected, (ULONG)G_pixel_selection,
												TAG_END)),	
												CHILD_MinHeight, 23,	

												LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												TAG_END),										
	
											TAG_END),

											// buffering
											LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
												LAYOUT_BevelStyle, BVS_SBAR_VERT,
												LAYOUT_Label, (ULONG)"Screen buffering:",

												LAYOUT_AddChild, (ULONG)(G_gadgets[GID_BUFFER_CHOOSER] = NewObject(CHOOSER_GetClass(), NULL, 
													GA_ID, GID_BUFFER_CHOOSER,
													GA_RelVerify, TRUE,
													CHOOSER_PopUp, TRUE,			
													CHOOSER_Justification, CHJ_CENTER,					
													CHOOSER_Labels, (ULONG)&REACTION_buffering_labels_3,
													CHOOSER_Selected, (ULONG)2,
												TAG_END)),	
												CHILD_MinHeight, 23,	

											TAG_END),
										TAG_END),	
										CHILD_WeightedWidth, 45,

										// RIGHT SIDE
										LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
											LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

											// screen size - list view
											LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
												LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
												LAYOUT_BevelStyle, BVS_SBAR_VERT,
												LAYOUT_Label, (ULONG)"Screen size:",
												
												LAYOUT_AddChild, (ULONG)(G_gadgets[GID_SCREEN_LIST] = NewObject(LISTBROWSER_GetClass(), NULL,	
													GA_ID, GID_SCREEN_LIST,
													GA_RelVerify, TRUE,
													LISTBROWSER_Labels, (ULONG)&REACTION_list_browser_labels,
													LISTBROWSER_ColumnInfo, (ULONG)&REACTION_lb_column_info,
													LISTBROWSER_ColumnTitles, TRUE,
													LISTBROWSER_Separators, TRUE,
													LISTBROWSER_Hierarchical, FALSE,
													LISTBROWSER_Editable, FALSE,
													LISTBROWSER_MultiSelect, FALSE,
													LISTBROWSER_ShowSelected, TRUE,
												TAG_END)),	
											TAG_END),
										TAG_END),
										CHILD_WeightedWidth, 55,

									TAG_END),	

									// ADD ROW AT THE BOTTOM
									LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
										LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
										LAYOUT_SpaceOuter, TRUE,
										
										LAYOUT_AddChild, (ULONG)(G_gadgets[GID_HELP_BUTTON] = NewObject(BUTTON_GetClass(), NULL,
											GA_ID, GID_HELP_BUTTON,
											GA_RelVerify, TRUE,
											GA_Text, (ULONG)"HELP",
										TAG_END)),
										CHILD_WeightedWidth, 30,								
										CHILD_MinHeight, 28,												

										LAYOUT_AddChild, (ULONG)(G_gadgets[GID_START_BUTTON] = NewObject(BUTTON_GetClass(), NULL,
											GA_ID, GID_START_BUTTON,
											GA_RelVerify, TRUE,
											GA_Text, (ULONG)"START RAYCASTER",
										TAG_END)),
										CHILD_WeightedWidth, 70,
										CHILD_MinHeight, 28,
								
									TAG_END),
									CHILD_WeightedHeight, 0,

								TAG_END),
							TAG_END);

	if (!REACTION_window_object) 
		return 0;

	// Create Reaction window.
	STARTER_window = (struct Window *)RA_OpenWindow(REACTION_window_object);

	if(!STARTER_window)
		return 0;
	return 1;
}

void 	GUI_REACTION_Message_Loop(void)
{
	ULONG win_signals, wait_signals, result;
	UWORD code;

	GetAttr(WINDOW_SigMask, REACTION_window_object, &win_signals);

	// enter main loop
	UBYTE is_loop = TRUE;

	G_start_selection = FALSE;

	// Update before start.
	GUI_Update();

	while(is_loop)
	{
		wait_signals = Wait( win_signals | SIGBREAKF_CTRL_C);
		 
		if (wait_signals & SIGBREAKF_CTRL_C) is_loop = FALSE;
		else
		{
			while (( result = RA_HandleInput(REACTION_window_object, &code)) != WMHI_LASTMSG)
			{	
				switch (result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
						G_start_selection = FALSE;
             			is_loop = FALSE;
              			break;

			  		case WMHI_GADGETUP:
							
						switch (result & WMHI_GADGETMASK)
					  	{
							case GID_MODE_CHOOSER:
							case GID_ASPECT_CHOOSER:
							case GID_PIXEL_CHOOSER:
								GUI_Update();
								break;

							case GID_HELP_BUTTON:
								GUI_Show_Help();
								break;

							case GID_START_BUTTON:
								Get_Export_Values();
								G_start_selection = TRUE;
								is_loop = FALSE;
								break;
						}
						break;
				}
			}
		}
    }
}

void	GUI_REACTION_Convert_List(void)
{
	struct Node *tmp_node;
	sMY_Data_Node* tmp_my_data_node;    

	struct List tmp_list;
	tmp_list = G_screen_list;

	// We need to reformat data from main list to ReAction list browser.
	for (tmp_my_data_node = (APTR)tmp_list.lh_Head; tmp_my_data_node->node.ln_Succ != NULL; tmp_my_data_node = (APTR)tmp_my_data_node->node.ln_Succ)
	{
		tmp_node = AllocListBrowserNode( 	2,
											LBNA_Column, 0,
												LBNCA_CopyText, TRUE,
												LBNCA_Text, (ULONG)tmp_my_data_node->driver_string,
												LBNCA_MaxChars, 10,
												LBNCA_Justification, LCJ_LEFT,
											LBNA_Column, 1,
												LBNCA_CopyText, TRUE,
												LBNCA_Text, (ULONG)tmp_my_data_node->resolution_string,
												LBNCA_MaxChars, 16,
												LBNCA_Justification, LCJ_RIGHT,
											TAG_DONE);

		// Add node to list.
		AddTail(&REACTION_list_browser_labels, tmp_node);
	}		
}

void	GUI_REACTION_Strings_To_Chooser_Labels(const char* _strings[], struct List* _list)
{
	NewList(_list);

	struct Node *tmp_node;
	ULONG i = 0;

	do
	{
		tmp_node = AllocChooserNode(CNA_Text, (ULONG)_strings[i], TAG_DONE);
		AddTail(_list, tmp_node);

		i++;
	} while(_strings[i] != 0);
}

void	GUI_REACTION_Clear_Chooser_Labels(struct List* _list)
{
 	// Free main list and nodes.
  	struct Node *worknode;
  	struct Node *nextnode;

    worknode = _list->lh_Head;

    while ( (nextnode = worknode->ln_Succ) ) 
	{
		FreeChooserNode(worknode);
        worknode = nextnode;
    }

	NewList(_list);
}


UBYTE 	GUI_INTUITION_Create(void)
{
	// get some helpers to create more compact window
	ULONG	font_width = STARTER_pubscreen->RastPort.TxWidth;
	ULONG	font_height = STARTER_pubscreen->RastPort.TxHeight;

	// *****************************************************************
	// GADGETS setup
	// *****************************************************************

		struct Gadget*		g_context = CreateContext(&G_gadgets[GID_GADGETS_LIST]);
	 	struct NewGadget 	ng;

		// For all gadgets.
		ng.ng_TextAttr   = STARTER_pubscreen->Font;
		ng.ng_VisualInfo = STARTER_visual_info;

		// -- LEFT COLUMN --

			// create text
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8;
			ng.ng_Width      = font_width * 17;
			ng.ng_Height     = font_height;
			ng.ng_Flags      = 0;

			g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
										GTTX_Text, (ULONG)"Display mode:", 
										TAG_END);

			// Create cycle/chooser
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2;
			ng.ng_Width      = font_width * 22;
			ng.ng_Height     = font_height + 10;
			ng.ng_GadgetID   = GID_MODE_CHOOSER;
			ng.ng_Flags      = 0;

			G_gadgets[GID_MODE_CHOOSER] = g_context = CreateGadget(	CYCLE_KIND, g_context, &ng, 
																	GTCY_Labels, (ULONG)G_mode_strings, 
																	GTCY_Active, (ULONG)G_mode_selection,
																	TAG_END);


			// create text
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15;
			ng.ng_Width      = font_width * 17;
			ng.ng_Height     = font_height;
			ng.ng_Flags      = 0;

			g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
										GTTX_Text, (ULONG)"Screen aspect:", 
										TAG_END);

			// Create cycle/chooser
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2;
			ng.ng_Width      = font_width * 22;
			ng.ng_Height     = font_height + 10;
			ng.ng_GadgetID   = GID_ASPECT_CHOOSER;
			ng.ng_Flags      = 0;

			G_gadgets[GID_ASPECT_CHOOSER] = g_context = CreateGadget(	CYCLE_KIND, g_context, &ng, 
																		GTCY_Labels, (ULONG)G_aspect_fullscreen_strings, 
																		GTCY_Active, (ULONG)0,
																		TAG_END);


			// create text
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15;
			ng.ng_Width      = font_width * 17;
			ng.ng_Height     = font_height;
			ng.ng_Flags      = 0;

			g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
										GTTX_Text, (ULONG)"Pixel format:", 
										TAG_END);

			// Create cycle/chooser
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2;
			ng.ng_Width      = font_width * 22;
			ng.ng_Height     = font_height + 10;
			ng.ng_GadgetText = NULL;
			ng.ng_GadgetID   = GID_PIXEL_CHOOSER;
			ng.ng_Flags      = 0;

			G_gadgets[GID_PIXEL_CHOOSER] = g_context = CreateGadget(	CYCLE_KIND, g_context, &ng, 
																		GTCY_Labels, (ULONG)G_pixel_strings, 
																		GTCY_Active, (ULONG)G_pixel_selection,
																		TAG_END);


			// create text
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15;
			ng.ng_Width      = font_width * 17;
			ng.ng_Height     = font_height;
			ng.ng_GadgetText = NULL;
			ng.ng_Flags      = 0;

			g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
										GTTX_Text, (ULONG)"Screen buffering:", 
										TAG_END);

			// Create cycle/chooser
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2;
			ng.ng_Width      = font_width * 22;
			ng.ng_Height     = font_height + 10;
			ng.ng_GadgetText = NULL;
			ng.ng_GadgetID   = GID_BUFFER_CHOOSER;
			ng.ng_Flags      = 0;

			G_gadgets[GID_BUFFER_CHOOSER] = g_context = CreateGadget(	CYCLE_KIND, g_context, &ng, 
																		GTCY_Labels, (ULONG)G_buffering_strings_3, 
																		GTCY_Active, (ULONG)2,
																		TAG_END);

		// -- RIGHT COLUMN --

			// create text
			ng.ng_LeftEdge   = 10 + font_width * 22 + 10;
			ng.ng_TopEdge    = font_height + 8;
			ng.ng_Width      = font_width * 17;
			ng.ng_Height     = font_height;
			ng.ng_GadgetText = NULL;
			ng.ng_Flags      = 0;

			g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
										GTTX_Text, (ULONG)"Screen size:", 
										TAG_END);


			// Create list view for screen modes.
			ng.ng_LeftEdge   = 10 + font_width * 22 + 10;
			ng.ng_TopEdge    = font_height + 8 + font_height + 2;
			ng.ng_Width      = font_width * 24;
			ng.ng_Height     = font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + 8;
			ng.ng_GadgetText = NULL;
			ng.ng_GadgetID   = GID_SCREEN_LIST;
			ng.ng_Flags      = 0;

			G_gadgets[GID_SCREEN_LIST] = g_context = CreateGadget (	LISTVIEW_KIND, g_context, &ng, 
														GTLV_ShowSelected, 0,
														GTLV_Selected, 0,
														GTLV_Labels, (ULONG)&G_screen_list,
														TAG_END);

		// -- BOTTOM ROW --
	
			ULONG bottom_row_top_edge = font_height + 8 + font_height + 2 + font_height + 8 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + 2 + font_height + 15 + font_height + (font_height / 2);

			// Create help button.
			ng.ng_LeftEdge   = 10;
			ng.ng_TopEdge    = bottom_row_top_edge;
			ng.ng_Width      = font_width * 14;
			ng.ng_Height     = font_height + 12;
			ng.ng_GadgetText = (STRPTR)"HELP";
			ng.ng_GadgetID   = GID_HELP_BUTTON;
			ng.ng_Flags      = 0;

			G_gadgets[GID_HELP_BUTTON] = g_context = CreateGadget(BUTTON_KIND, g_context, &ng, TAG_END);

			// Create start button.
			ng.ng_LeftEdge   = 10 + font_width * 14 + 10;
			ng.ng_TopEdge    = bottom_row_top_edge;
			ng.ng_Width      = font_width * 32;
			ng.ng_Height     = font_height + 12;
			ng.ng_GadgetText = (STRPTR)"START RAYCASTER";
			ng.ng_GadgetID   = GID_START_BUTTON;
			ng.ng_Flags      = 0;

			G_gadgets[GID_START_BUTTON] = g_context = CreateGadget(BUTTON_KIND, g_context, &ng, TAG_END);


	// ***********************************************************************

	ULONG window_width	= 15 + font_width * 22 + font_width * 24 + 15;
	ULONG window_height = bottom_row_top_edge + font_height + 12 + 5;

	ULONG 	window_pos_x = (STARTER_pubscreen->Width / 2) - (window_width / 2);
	ULONG	window_pos_y = (STARTER_pubscreen->Height / 2) - (window_height / 2);

 	// --- Create window ---
    STARTER_window = OpenWindowTags(NULL, 
	 								WA_Left, window_pos_x, 
									WA_Top, window_pos_y, 
									WA_Width, window_width, 
									WA_Height, window_height, 
									WA_Title, (ULONG)STARTER_WINDOW_NAME, 
									WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE, 
									WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | BUTTONIDCMP | LISTVIEWIDCMP | IDCMP_GADGETDOWN | IDCMP_GADGETUP,
									WA_Gadgets, (ULONG)G_gadgets[GID_GADGETS_LIST],
									TAG_END);

	// must be refreshed after creation - GT - for gaddets
	GT_RefreshWindow(STARTER_window, NULL);

	if (!STARTER_window) 	return 0;
	else 					return 1;
}

void 	GUI_INTUITION_Message_Loop(void)
{
  	// Signals for event and messages.
 	ULONG win_signals = 1L << STARTER_window->UserPort->mp_SigBit;
	ULONG wait_signals;

	G_start_selection = FALSE;

	// Enter main loop.
	UBYTE is_loop = TRUE;

	GUI_Update();

	while(is_loop)
	{
		wait_signals = Wait(win_signals | SIGBREAKF_CTRL_C);
		if (win_signals & SIGBREAKF_CTRL_C) is_loop = FALSE;

		if (wait_signals & win_signals)
		{
			struct IntuiMessage* imsg;
			while (imsg = GT_GetIMsg (STARTER_window->UserPort))
			{
					switch (imsg->Class)
					{
						case IDCMP_REFRESHWINDOW:
							GT_BeginRefresh(STARTER_window);
							GT_EndRefresh(STARTER_window, TRUE);
							break;

						case IDCMP_CLOSEWINDOW:
							G_start_selection = FALSE;
							is_loop = FALSE;
							break;
							
						case IDCMP_GADGETUP:
						{
							struct Gadget *tmp_gadget = (struct Gadget *) imsg->IAddress;

							switch (tmp_gadget->GadgetID)
							{
								case GID_MODE_CHOOSER:
								case GID_ASPECT_CHOOSER:
								case GID_PIXEL_CHOOSER:
									GUI_Update();
									break;

								case GID_HELP_BUTTON:
									GUI_Show_Help();
									break;

								case GID_START_BUTTON:
									Get_Export_Values();
									G_start_selection = TRUE;
									is_loop = FALSE;
									break;
							}
						}
						break;
					}

					GT_ReplyIMsg (imsg);
			}
		}
	}
}

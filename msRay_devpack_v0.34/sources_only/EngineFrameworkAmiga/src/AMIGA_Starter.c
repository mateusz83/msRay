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

// Our helper common globals.
#include "AMIGA_Globals.h"

// ----------------------------------------
// --- AMIGA Starter - global variables ---
// ----------------------------------------
#define STA_WINDOW_WIDTH	320
#define STA_WINDOW_HEIGHT	280

#define CYBERGFXVERSION     41

struct Screen*      STA_pubscreen;     
struct Window*      STA_window;
struct Library*     CyberGfxBase 	= NULL;

// Only for ReAction GUI library.
struct Library*     WindowBase 		= NULL;
struct Library*     LayoutBase 		= NULL;
struct Library*     LabelBase 		= NULL;
struct Library*		ButtonBase 		= NULL;
struct Library*		ListBrowserBase = NULL;
struct Library*		ChooserBase 	= NULL;

// It will hold ReAction gadgets structure.
Object* STA_objects;

// ReAction ListBrowser gadget pointer.
struct Gadget *STA_lb_gad;

// ReAction textures cycle/chooser gadget pointer.
struct Gadget *STA_textures_gad;

// ReAction textures cycle/chooser list of elements.
struct List STA_textures_options_list;

// This List will hold screen modes for ReAction listbrowser.
struct List STA_lb_list;

// Additinal application message port for ReAction.
struct MsgPort *STA_app_port;

// Is Reaction gadgets library available? If not we will use standard Intuition Gadgets Library...
UBYTE STA_is_reaction = 0;

// Global values to EXPORT at the end.
ULONG STA_requested_display_id;
ULONG STA_requested_texture_size;

// Only for standard gadtools gadgets.
APTR 			STA_visual_info;			// pointer to visual informations for gadgets
struct Gadget*	STA_g_gadgets_list;			// list	of all gadgets
struct Gadget*	STA_g_listview;			    // pointer to listview gadget with screen modes list

// All gadgets numerations id-s. Both for standard gadtools and ReAaction gadgets.
enum { GID_MAIN = 0, GID_LABEL, GID_LIST, GID_LABEL_2, /*GID_TEXTURES,*/ GID_START, GID_EXIT, GID_LAST };

// A system list structure that will hold all screen modes.
struct List STA_screen_modes_list; 		

// A helper structure that will hold single screen mode in list.
typedef struct
{
    struct Node node; 

	BYTE driver[10];
	BYTE resolution[16];
	BYTE format[16];

	ULONG id;
    BYTE full_text_info[64];    

} sMY_Data_Node;

// ----------------------------------------------
// --- AMIGA Starter - functions DECLARATIONS ---
// ----------------------------------------------
UBYTE   STA_Init();
void    STA_Run();
void    STA_Cleanup();

void    STA_Message_Box(const char*, const char*, const char*);

UBYTE   STA_Open_Libraries();
UBYTE   STA_Get_Screen_Modes();

UBYTE   STA_GUI_Standard_Init();
void    STA_GUI_Standard_Message_Loop();

UBYTE   STA_GUI_ReAction_Init();
void    STA_GUI_ReAction_Message_Loop();

// --------------------------------------------
// --- AMIGA Starter - main() - entry point ---
// --------------------------------------------
int main(void)
{
	// Init AmigaOS Starter GUI, so we can select input parameters like resolution, screen mode etc.
	// and run the rest of framework and Raycaster Engine itself...
	if (!STA_Init())
	{
		// If initialisation went wrong, clean up the resources and end application.
		STA_Cleanup();
		return 0;
	}

	// Run Starter Message Loop, and when its done, get selected screen mode, so it can be passed to Framework.
	STA_Run();
	
	if (!STA_requested_display_id)
	{
		// If we just exited the Starter just clean up the resources and end application.
		STA_Cleanup();
		return 0;
	}

	// After ending Starter Message Loop (with selected screen mode), cleanup the resources.
	STA_Cleanup();

	// Lets first check if final file exist - because we have different builds, ex. for 128 and 256 texture sizes.
	char tmp_filename_to_check[64];
	memset(tmp_filename_to_check, 0, sizeof(tmp_filename_to_check));	

	// After everything is closed and cleanup-ed - start Framework/Engine - according to selected SCREEN MODE:
	sprintf(tmp_filename_to_check, "%s", GLOBAL_OUTPUT_FILENAME);
	
	FILE* file_to_check = fopen(tmp_filename_to_check, "r");
	if (file_to_check == NULL)
	{
		char tmp_info[64];
		memset(tmp_info, 0, sizeof(tmp_info));	
		sprintf(tmp_info, "Can't find file: %s", tmp_filename_to_check);

		STA_Message_Box(GLOBAL_STA_WIN_NAME, tmp_info, "OK");
		return 0;
	}

	// Lets prepare string variable that will contain selected framework .exe filename and display_id parameter.
	char tmp_filename_with_arguments[64];
	memset(tmp_filename_with_arguments, 0, sizeof(tmp_filename_with_arguments));	

	// After everything is closed and cleanup-ed - start Framework/Engine - according to selected SCREEN MODE:
	sprintf(tmp_filename_with_arguments, "%s %u", GLOBAL_OUTPUT_FILENAME, (unsigned int)STA_requested_display_id);
	Execute(tmp_filename_with_arguments, 0, 0);

	return 0;
}

// ---------------------------------------------
// --- AMIGA Starter - functions DEFINITIONS ---
// ---------------------------------------------

UBYTE STA_Init()
{
    // Open all needed licraries.
    if (!STA_Open_Libraries())
	{
        return 0;
	}

	// Get pointer to Workbench public screen.
	if (!(STA_pubscreen = LockPubScreen(NULL))) 
	{
		STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't get Workbench public screen.", "OK");
		return 0;
	}

    // Try get screen modes we need.
    if (!STA_Get_Screen_Modes())
    {
        STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't find any 32-bit colour screen mode.\nThe application needs at least one 32-bit screen mode.\nTry add new 32-bit mode in Picasso96Mode or CyberGraphX tools\nfrom Workbench/Prefs/ folder. ", "OK");
        return 0;
    }

    // Init selected interface.
    if (WindowBase == NULL || LayoutBase == NULL || LabelBase == NULL || ButtonBase	== NULL || ListBrowserBase == NULL || ChooserBase == NULL)
    {
        // no ReAction GUI libraries found on this system, so we are using standard gadgets
		STA_is_reaction = 0;

        if (!STA_GUI_Standard_Init())
        {
            STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't create standard window and gadgets.\nSomething might be wrong with the System.", "OK");
            return 0;
        }
    }
    else
    {
        // we will use ReAction GUI
        if (!STA_GUI_ReAction_Init())
        {
            // If creating ReACtion gui is impossible...
            // STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't create ReAction window and gadgets.\nSomething might be wrong with the ReAction Library.\nI will now try to create standard window and gadgets.", "OK");

			STA_is_reaction = 0;

            // ...try create standard window.
            if (!STA_GUI_Standard_Init())
            {
                STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't create standard window and gadgets.\nSomething might be wrong with the System.", "OK");
                return 0;
            }
        }
		else STA_is_reaction = 1;
    }

    return 1;
}

void STA_Run()
{
    if (STA_is_reaction)
    {
        STA_GUI_ReAction_Message_Loop();
    }
    else
    {
        STA_GUI_Standard_Message_Loop();
    }
}

void STA_Cleanup()
{	
    // Close standard Window.
    CloseWindow(STA_window);

    // free standard GADTOOLS
    FreeGadgets(STA_g_gadgets_list);

    // free visual info
    FreeVisualInfo(STA_visual_info);

    // Free list and nodes.
  	sMY_Data_Node *worknode;
  	sMY_Data_Node *nextnode;

    worknode = (sMY_Data_Node*)(STA_screen_modes_list.lh_Head);

    while ( (nextnode = (sMY_Data_Node*)(worknode->node.ln_Succ)) ) 
	{
        Remove((struct Node*)worknode);
        worknode = nextnode;
    }

    // Unlock Worckbench public screen.
	UnlockPubScreen(NULL, STA_pubscreen);	

	struct Node* n_worknode;
  	struct Node* n_nextnode;

 	n_worknode = (struct Node*)(STA_lb_list.lh_Head);

    while ( (n_nextnode = (struct Node*)(n_worknode->ln_Succ)) ) 
	{
        FreeListBrowserNode(n_worknode);
		Remove(n_worknode);
        n_worknode = n_nextnode;
    }

	DeleteMsgPort(STA_app_port);

    // Close ReAction libraries.
	CloseLibrary(ChooserBase);
	CloseLibrary(ListBrowserBase);
	CloseLibrary(ButtonBase);
	CloseLibrary(LabelBase);
	CloseLibrary(LayoutBase);
	CloseLibrary(WindowBase);
	
    // Close CGX library.
	CloseLibrary(CyberGfxBase);
}


void STA_Message_Box(const char* _title, const char* _info, const char* _options)
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

UBYTE STA_Open_Libraries()
{
    // try open cybergraphics library
    CyberGfxBase = (struct Library*)OpenLibrary("cybergraphics.library", CYBERGFXVERSION);

    // if no RTG system found end the application
	if (CyberGfxBase == NULL)
	{
		STA_Message_Box(GLOBAL_STA_WIN_NAME, "Couldn't find -cybergraphics.library- or its equivalent.\nThe application needs some kind of RTG system to run.", "OK");
        return 0;
	}

    // now lets test if the system has ReAction Libraries installed so we can use modern GUI
    WindowBase  	= OpenLibrary("window.class", 0L);
    LayoutBase  	= OpenLibrary("gadgets/layout.gadget", 0L);
    LabelBase   	= OpenLibrary("images/label.image", 0L);
	ButtonBase		= OpenLibrary("gadgets/button.gadget", 0L);
	ListBrowserBase	= OpenLibrary("gadgets/listbrowser.gadget", 0L);
	ChooserBase		= OpenLibrary("gadgets/chooser.gadget", 0L);

	return 1;
}

UBYTE STA_Get_Screen_Modes()
{
	// for naming screen formats in list
	char* formats[] = {  "LUT 8", "RGB 15", "BGR 15", "RGB 15 PC", "BGR 15 PC", "RGB 16", "BGR 16", "RGB 16 PC", "BGR 16 PC", "RGB 24", "BGR 24", "ARGB 32-bit *", "BGRA 32-bit", "RGBA 32-bit", "(unknown)" };	

	// prepare lists structure that will hold screen modes
	NewList(&STA_screen_modes_list);
	
	// helper Node
	sMY_Data_Node* my_data_node;

    // count all modes from the list
    int count = 0;

	// getting modes for fullscreen
 	ULONG modeID = INVALID_ID;

 	while ( (modeID = NextDisplayInfo(modeID)) != INVALID_ID)
 	{

		ULONG attr_format = GetCyberIDAttr(CYBRIDATTR_PIXFMT, modeID);
		if (attr_format < 0) attr_format = 0;
        if (attr_format > 14) attr_format = 14;

		if (	IsCyberModeID(modeID)  &&  (
				(attr_format == PIXFMT_ARGB32)	||
				(attr_format == PIXFMT_BGRA32)	||
				(attr_format == PIXFMT_RGBA32)
			))
		
			{
                    count++;

					// Get Width and Height of found screen mode.
					int attr_width = GetCyberIDAttr(CYBRIDATTR_WIDTH, modeID);
					int attr_height = GetCyberIDAttr(CYBRIDATTR_HEIGHT, modeID);

					// Get full name - so we can extract Driver Name.
 					struct NameInfo nameInfo;

					DisplayInfoHandle display_info_handler = FindDisplayInfo (modeID);
					GetDisplayInfoData (display_info_handler, (APTR)&nameInfo, sizeof (struct NameInfo), DTAG_NAME, INVALID_ID);

					char buffer_driver[10];
					memset(buffer_driver, 0, 10);

					// Extract first 8 chars from Full name (the driver is at the beginning and end with ':' )
					for (UBYTE i = 0; i < 10; i++)
					{
						if (  nameInfo.Name[i] == ':')
							break;

						buffer_driver[i] = nameInfo.Name[i];
					}

					char buffer_3[16];
					memset(buffer_3, 0, 16);
					sprintf(buffer_3, "%s", formats[attr_format] );	

					char buffer_2[16];
					memset(buffer_2, 0, 16);
					sprintf(buffer_2, "%d x %d", attr_width, attr_height);

					char buffer_1[64];
					memset(buffer_1, 0, 64);		
					sprintf(buffer_1, "%s:   %d x %d       %s", buffer_driver, attr_width, attr_height, formats[attr_format] );

                    my_data_node = (sMY_Data_Node*)malloc(sizeof(sMY_Data_Node));

					if (my_data_node)
					{
						memcpy(my_data_node->full_text_info, buffer_1, 64);	
						my_data_node->node.ln_Name = my_data_node->full_text_info;
						my_data_node->node.ln_Type = 100L;
						my_data_node->node.ln_Pri = 0;
						my_data_node->id = modeID;

						memcpy(my_data_node->resolution, buffer_2, 16 );
						memcpy(my_data_node->format, buffer_3, 16 );
						memcpy(my_data_node->driver, buffer_driver, 10);

						AddTail(&STA_screen_modes_list, (struct Node*)my_data_node);																		
					}
			}
	}

	return count;
}

// Only for standard intuition gadtools gadgets.

UBYTE STA_GUI_Standard_Init()
{
    // Get visual info for gadgets.
	STA_visual_info = GetVisualInfo(STA_pubscreen, TAG_END);

	// get some helpers to create more compact window
	ULONG	font_width = STA_pubscreen->RastPort.TxWidth;;
	ULONG	font_height = STA_pubscreen->RastPort.TxHeight;

	ULONG 	win_pos_x = (STA_pubscreen->Width / 2) - (STA_WINDOW_WIDTH / 2);
	ULONG	win_pos_y = (STA_pubscreen->Height / 2) - (STA_WINDOW_HEIGHT / 2);

	// ---------------------
	// --- GADGETS setup ---
	// ---------------------

		struct Gadget*		g_context = CreateContext(&STA_g_gadgets_list);
	 	struct NewGadget 	ng;

		// For all gadgets.
		ng.ng_TextAttr   = STA_pubscreen->Font;
		ng.ng_VisualInfo = STA_visual_info;

		// create text
		ng.ng_LeftEdge   = 15;
		ng.ng_TopEdge    = font_height + 14;
		ng.ng_Width      = font_width * 17;
		ng.ng_Height     = font_height;
		ng.ng_GadgetText = NULL;
		ng.ng_GadgetID   = GID_LABEL;
		ng.ng_Flags      = 0;

   		g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
		   							GTTX_Text, (ULONG)"Settings:", 
                                    TAG_END);

		// Create list view for screen modes.
		ng.ng_LeftEdge   = 15;
		ng.ng_TopEdge    = ng.ng_TopEdge + ng.ng_Height + 5;
		ng.ng_Width      = STA_WINDOW_WIDTH - 15 - 15;
		ng.ng_Height     = STA_WINDOW_HEIGHT - 120;
		ng.ng_GadgetText = NULL;
		ng.ng_GadgetID   = GID_LIST;
		ng.ng_Flags      = 0;

    	STA_g_listview = g_context = CreateGadget (	LISTVIEW_KIND, g_context, &ng, 
												    GTLV_ShowSelected, 0,
                                    			    GTLV_Selected, 0,
                                    			    GTLV_Labels, (ULONG)&STA_screen_modes_list,
                                    		    	TAG_END);

		// create text
		ng.ng_LeftEdge   = 15;
		ng.ng_TopEdge    = font_height + 203;
		ng.ng_Width      = font_width * 17;
		ng.ng_Height     = font_height;
		ng.ng_GadgetText = NULL;
		ng.ng_GadgetID   = GID_LABEL_2;
		ng.ng_Flags      = 0;

   		g_context = CreateGadget(	TEXT_KIND, g_context, &ng, 
		   							GTTX_Text, (ULONG)"* ARGB format is recommended for faster loading.", 
                                    TAG_END);


		// Create start button.
		ng.ng_TextAttr   = STA_pubscreen->Font;
		ng.ng_VisualInfo = STA_visual_info;
		ng.ng_LeftEdge   = 15;
		ng.ng_TopEdge    = STA_WINDOW_HEIGHT - 38;
		ng.ng_Width      = STA_WINDOW_WIDTH - 95;
		ng.ng_Height     = 25;
		ng.ng_GadgetText = (STRPTR)"START RAYCASTER";
		ng.ng_GadgetID   = GID_START;
		ng.ng_Flags      = 0;

   		g_context = CreateGadget(BUTTON_KIND, g_context, &ng, TAG_END);


		// Create exit button.
		ng.ng_TextAttr   = STA_pubscreen->Font;
		ng.ng_VisualInfo = STA_visual_info;
		ng.ng_LeftEdge   = STA_WINDOW_WIDTH - 70;
		ng.ng_TopEdge    = STA_WINDOW_HEIGHT - 38;
		ng.ng_Width      = 55;
		ng.ng_Height     = 25;
		ng.ng_GadgetText = (STRPTR)"EXIT";
		ng.ng_GadgetID   = GID_EXIT;
		ng.ng_Flags      = 0;

   		g_context = CreateGadget(BUTTON_KIND, g_context, &ng, TAG_END);

	// -------------------------
	// --- end GADGETS setup ---
	// -------------------------

    // --- create window ---
    STA_window = OpenWindowTags(	NULL, 
	 								WA_Left, win_pos_x, 
									WA_Top, win_pos_y, 
									WA_Width, STA_WINDOW_WIDTH, 
									WA_Height, STA_WINDOW_HEIGHT, 
									WA_Title, (ULONG)GLOBAL_STA_WIN_NAME, 
									WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE, 
									WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | BUTTONIDCMP | LISTVIEWIDCMP | IDCMP_GADGETDOWN | IDCMP_GADGETUP,
									WA_PubScreen, (ULONG)STA_pubscreen,
									WA_Gadgets, (ULONG)STA_g_gadgets_list,
									TAG_END);

    if (!STA_window)
        return 0;

	// must be refreshed after creation - GT - for gaddets
	GT_RefreshWindow(STA_window, NULL);

    return 1;
}

void STA_GUI_Standard_Message_Loop()
{
    // signals for event and messages
 	ULONG win_signals = 1L << STA_window->UserPort->mp_SigBit;
	ULONG wait_signals;

	// enter main loop
	UBYTE is_loop = TRUE;

    // When done return selected screen mode or 0 if Starter just exit.
    STA_requested_display_id = 0;

	while(is_loop)
	{
		wait_signals = Wait(win_signals | SIGBREAKF_CTRL_C);
		if (win_signals & SIGBREAKF_CTRL_C) is_loop = FALSE;

		if (wait_signals & win_signals)
		{
			struct IntuiMessage* imsg;
			while (imsg = GT_GetIMsg (STA_window->UserPort))
			{
					switch (imsg->Class)
					{
						case IDCMP_REFRESHWINDOW:
							GT_BeginRefresh(STA_window);
							GT_EndRefresh(STA_window, TRUE);
							break;

						case IDCMP_CLOSEWINDOW:
							is_loop = FALSE;
							break;
							
						case IDCMP_GADGETUP:
						{
							struct Gadget *tmp_gadget = (struct Gadget *) imsg->IAddress;

							switch (tmp_gadget->GadgetID)
							{
								case GID_START:
								{
									// get what is selected on list
									ULONG g_selected;
									GT_GetGadgetAttrs(STA_g_listview, STA_window, NULL, GTLV_Selected, (ULONG)&g_selected, TAG_DONE);

									// scan the selected list and find current entry
									sMY_Data_Node* tmp_my_data_node;
                                    struct List tmp_list;
                                    tmp_list = STA_screen_modes_list;

									ULONG counter = 0;
									for (tmp_my_data_node = (APTR)tmp_list.lh_Head; tmp_my_data_node->node.ln_Succ != NULL; tmp_my_data_node = (APTR)tmp_my_data_node->node.ln_Succ)
									{
										if (counter == g_selected) 
                                        {
                                            	//get selected display ID and exit main loop
                                                STA_requested_display_id = tmp_my_data_node->id;
                                                is_loop = FALSE;	
                                                break;
                                        }
										counter++;		
									}							
								}
								break;

                                case GID_EXIT:
                                    is_loop = FALSE;
                                    break;

								case GID_LIST:									
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

// Only for ReAction gadgets.

UBYTE STA_GUI_ReAction_Init()
{
	// Create message port.
	STA_app_port = CreateMsgPort();

	// Define columns structure.
	struct ColumnInfo lb_ci[] =
	{
		{  32, "Driver", 0  },
		{  34, "Resolution", 0 },
		{  34, "Pixel format", 0 },
		{ -1, (STRPTR)~0, -1 }
	};

	// Fillup the List structure with Screen Modes.
	NewList(&STA_lb_list);

	struct Node *tmp_node;
	sMY_Data_Node* tmp_my_data_node;    

	struct List tmp_list;
	tmp_list = STA_screen_modes_list;

	for (tmp_my_data_node = (APTR)tmp_list.lh_Head; tmp_my_data_node->node.ln_Succ != NULL; tmp_my_data_node = (APTR)tmp_my_data_node->node.ln_Succ)
	{
		// Create node with informations.
		tmp_node = AllocListBrowserNode( 	3,
											LBNA_Column, 0,
												LBNCA_CopyText, TRUE,
												LBNCA_Text, (ULONG)tmp_my_data_node->driver,
												LBNCA_MaxChars, 10,
												LBNCA_Justification, LCJ_LEFT,
											LBNA_Column, 1,
												LBNCA_CopyText, TRUE,
												LBNCA_Text, (ULONG)tmp_my_data_node->resolution,
												LBNCA_MaxChars, 16,
												LBNCA_Justification, LCJ_RIGHT,
											LBNA_Column, 2,
												LBNCA_Text, (ULONG)tmp_my_data_node->format,
												LBNCA_MaxChars, 16,
												LBNCA_Justification, LCJ_RIGHT,
											TAG_DONE);

		// Add node to list.
		AddTail(&STA_lb_list, tmp_node);
	}		

	// Create ReAction gadgets.
	STA_objects = NewObject( WINDOW_GetClass(), NULL,
								WA_ScreenTitle, (ULONG)GLOBAL_STA_WIN_NAME,
								WA_Title, (ULONG)GLOBAL_STA_WIN_NAME,
								WA_Activate, TRUE,
								WA_DepthGadget, TRUE,
								WA_DragBar, TRUE,
								WA_CloseGadget, TRUE,
								WA_Width, STA_WINDOW_WIDTH,
								WA_Height, STA_WINDOW_HEIGHT,
								WINDOW_IconifyGadget, TRUE,
								WINDOW_IconTitle, (ULONG)GLOBAL_STA_WIN_NAME,
								WINDOW_AppPort, (ULONG)STA_app_port,
								WINDOW_Position, WPOS_CENTERSCREEN,
								WINDOW_ParentGroup, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
									LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
									LAYOUT_SpaceOuter, TRUE,
									LAYOUT_DeferLayout, TRUE,

									LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
										LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
										LAYOUT_SpaceOuter, TRUE,
										LAYOUT_BevelStyle, BVS_GROUP,
										LAYOUT_Label, (ULONG)"Use *ARGB format for faster loading:",

										LAYOUT_AddChild, (ULONG)(STA_lb_gad = NewObject(LISTBROWSER_GetClass(), NULL,
											GA_ID, GID_LIST,
											GA_RelVerify, TRUE,
											LISTBROWSER_Labels, (ULONG)&STA_lb_list,
											LISTBROWSER_ColumnInfo, (ULONG)&lb_ci,
											LISTBROWSER_ColumnTitles, TRUE,
											LISTBROWSER_Separators, TRUE,
											LISTBROWSER_Hierarchical, FALSE,
											LISTBROWSER_Editable, FALSE,
											LISTBROWSER_MultiSelect, FALSE,
											LISTBROWSER_Selected, (ULONG)0,
											LISTBROWSER_ShowSelected, TRUE,
										TAG_END)),																

										LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(), NULL, 
											LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,

											LAYOUT_AddChild, (ULONG)NewObject(NULL, "button.gadget",
												GA_ID, GID_START,
												GA_RelVerify, TRUE,
												GA_Text, (ULONG)"START RAYCASTER",
											TAG_END),
											CHILD_WeightedWidth, 75,
										
											LAYOUT_AddChild, (ULONG)NewObject(NULL, "button.gadget",
												GA_ID, GID_EXIT,
												GA_RelVerify, TRUE,
												GA_Text, (ULONG)"EXIT",
											TAG_END),
											CHILD_WeightedWidth, 25,

										TAG_END),
										CHILD_WeightedHeight, 12,												
									TAG_END),																
								TAG_END),
							TAG_END);

	if (!STA_objects) 
		return 0;

	// Create Reaction window.
	STA_window = (struct Window *)RA_OpenWindow(STA_objects);

	if(!STA_window)
		return 0;

	return 1;
}

void STA_GUI_ReAction_Message_Loop()
{
	ULONG app_signals = (1L << STA_app_port->mp_SigBit);
	ULONG win_signals, wait_signals, result;
	UWORD code;

	GetAttr(WINDOW_SigMask, STA_objects, &win_signals);

	// enter main loop
	UBYTE is_loop = TRUE;

    // When done return selected screen mode or 0 if Starter just exit.
    STA_requested_display_id = 0;

	while(is_loop)
	{
		wait_signals = Wait( win_signals | SIGBREAKF_CTRL_C | app_signals);
		 
		if (wait_signals & SIGBREAKF_CTRL_C) is_loop = FALSE;
		else
		{
			while (( result = RA_HandleInput(STA_objects, &code)) != WMHI_LASTMSG)
			{
				switch (result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
             			is_loop = FALSE;
              			break;

			  		case WMHI_GADGETUP:
						switch (result & WMHI_GADGETMASK)
					  	{
							case GID_START:
						  		{
									// get what is selected on list
									ULONG g_selected;
									GetAttr(LISTBROWSER_Selected, STA_lb_gad, (ULONG*)&g_selected);

									// scan the selected list and find current entry
									sMY_Data_Node* tmp_my_data_node;
                                    struct List tmp_list;
                                    tmp_list = STA_screen_modes_list;

									ULONG counter = 0;
									for (tmp_my_data_node = (APTR)tmp_list.lh_Head; tmp_my_data_node->node.ln_Succ != NULL; tmp_my_data_node = (APTR)tmp_my_data_node->node.ln_Succ)
									{
										if (counter == g_selected) 
                                        {
                                            	//get selected display ID and exit main loop
                                                STA_requested_display_id = tmp_my_data_node->id;
                                                is_loop = FALSE;	
                                                break;
                                        }
										counter++;		
									}							
								}
						  		break;

							case GID_EXIT:
						  		is_loop = FALSE;
						  		break;
						}
						break;
				}
			}
		}
    }
}
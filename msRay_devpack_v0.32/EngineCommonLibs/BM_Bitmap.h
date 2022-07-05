#ifndef __BM_BITMAP_H__
#define __BM_BITMAP_H__

#include "IO_In_Out.h"

typedef struct
{
    u_int16     width;
    u_int16     height;
    u_int16     num_colors;
    u_int8*     image_data;     // keeps values <0..255> 
    u_int32*    color_data;     // keeps 4 byte color values like: (R,G,B,A), where each RGBA = <0..255>
} sBM_Bitmap_Indexed;

// ----------------------------------------
// --- BM_BITMAP functions declarations ---
// ----------------------------------------

int32 BM_Read_Bitmap_Indexed_TGA(char*, sBM_Bitmap_Indexed*, sIO_Prefs*);
void  BM_Free_Bitmap_Indexed(sBM_Bitmap_Indexed*);

int32 BM_Read_Texture_RAW(char*, u_int32*, u_int8*, sIO_Prefs*);
int32 BM_Read_Lightmaps_RAW_Separate(char*, u_int8*, u_int8*, u_int8*, int16, int16);
int32 BM_Read_Lightmaps_RAW_All(char*, u_int8*, int16);

// Used in Level Editor to convert bitmaps.
int32 BM_Convert_And_Save_Bitmap_Indexed_To_Texture_RAW(char*, sBM_Bitmap_Indexed*, u_int8, u_int8, u_int8);
int32 BM_Convert_And_Save_Bitmap_Indexed_To_Lightmap_RAW(char*, sBM_Bitmap_Indexed*, int32, int32);

#endif
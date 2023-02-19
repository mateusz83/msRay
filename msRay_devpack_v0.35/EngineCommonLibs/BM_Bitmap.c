#include "BM_Bitmap.h"
#include "BM_Bitmap_TGA.h"

#include "MA_Math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// ---------------------------------------
// --- BM_BITMAP functions definitions ---
// ---------------------------------------


//  ---------------------------------------------------------------------------------
// 
//  Loads fixed 256 color indexed TGA into sBM_Bitmap_Indexed structure
//  and returns:
// 
//       0  when file is NULL
//      -1  when TGA is not indexed
//      -2  when can't alloc mem for pixel indexes
//      -3  when can't alloc mem for color map
//      -4  TGA reading error
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Read_Bitmap_Indexed_TGA(char* _input_filename, sBM_Bitmap_Indexed* _output_bitmap, sIO_Prefs* _prefs)
{
    FILE* file_in;
    file_in = fopen(_input_filename, "rb");

    u_int32 mem_size_image_data = 0;
    u_int32 mem_size_color_data = 0;

    if (file_in == NULL) return 0;
    else
    {
        // TGA reading routine from external TGA library.
        tga_image img;

        // Exit if format problem.
        if (tga_read_from_FILE(&img, file_in) != 0)
        {
            fclose(file_in);
            return -4;
        }

        fclose(file_in);

        // If image is NOT indexed color map then exit.
        if (img.color_map_type == 0)
        {
            tga_free_buffers(&img);
            return -1;
        }
        else
        {
            // Get bitmap size.
            _output_bitmap->width = img.width;
            _output_bitmap->height = img.height;

            // Allocate space for bitmap data - in this case indexes to color map.
            mem_size_image_data = img.width * img.height * sizeof(u_int8);

            _output_bitmap->image_data = (u_int8*)malloc(mem_size_image_data);
            if (_output_bitmap->image_data == NULL) return -2;

            // We must find out, how many colors are exactly in the color map - because this TGA library always truncates to 256.
            // We must figure out it ourselves...
            u_int16 tmp_table[256];
            memset(tmp_table, 0, sizeof(tmp_table));

            u_int8  i_min = img.image_data[0],
                    i_max = i_min;

            for (u_int32 y = 0; y < img.height; y++)
            {
                u_int32 yy = y * img.width;

                for (u_int32 x = 0; x < img.width; x++)
                {
                    u_int32 index = img.image_data[x + yy];

                    if (index > i_max) i_max = index;
                    if (index < i_min) i_min = index;

                    tmp_table[(u_int8)index]++;
                }
            }

            // Real number of colors in the color table.
            u_int16 num_colors = 0;
            
            for (int i = 0; i < 256; i++)
                if (tmp_table[i] > 0) num_colors++;

            // Put that value in our structure.
            _output_bitmap->num_colors = num_colors;

            // Allocating mem for color table - 4 bytes per color.
            mem_size_color_data = num_colors * sizeof(u_int32);

            _output_bitmap->color_data = (u_int32*)malloc(mem_size_color_data);
            if (_output_bitmap->color_data == NULL) return -3;

            // Flip image and copy its data to our structure.
            tga_flip_vert(&img);
            memcpy(_output_bitmap->image_data, img.image_data, mem_size_image_data);

            // But another action is needed.
            // Depending on the software that saves the TGA file, the colors in the color table,
            // can start from the beginning or end etc.
            // So we must also take care of that - to be sure that all colors has been copied to our structure.
            
            if (i_min != 0)
            {
                for (int32 y = 0; y < img.height; y++)
                {
                    int32 yy = y * img.width;

                    for (int32 x = 0; x < img.width; x++)
                    {
                        _output_bitmap->image_data[x + yy] -= i_min;
                    }
                }
            }

            // Lets modify the color table according to our desired RGBA byte order - setup in prefs.
            int32 index = 0;

            // Byte order after load is BGR - 3 bytes
            for (int16 color = 0; color < num_colors * 3; color += 3)
            {
                // get the pixel RGB values and calculate the intensity as (channel value * intensity) / 255
                u_int32 B = img.color_map_data[3 * i_min + color];
                u_int32 G = img.color_map_data[3 * i_min + color + 1];
                u_int32 R = img.color_map_data[3 * i_min + color + 2];

                // Merge the color into 4 byte value and put into correct place in the intensity table.
                u_int32 output_color = (R << _prefs->ch1) | (G << _prefs->ch2) | (B << _prefs->ch3);
                _output_bitmap->color_data[index] = output_color;
                
                index++;
            }
        }

        // Now we can free the data read from the TGA file.
        tga_free_buffers(&img);
    }

    return mem_size_image_data + mem_size_color_data;
}


//  ---------------------------------------------------------------------------------
//  Free all the data get by previous function.
//  ---------------------------------------------------------------------------------
void BM_Free_Bitmap_Indexed(sBM_Bitmap_Indexed* _input_bitmap)
{
    free(_input_bitmap->color_data);
    free(_input_bitmap->image_data);

    _input_bitmap->color_data = NULL;
    _input_bitmap->image_data = NULL;
}


//  ---------------------------------------------------------------------------------
// 
//  BM_Read_Texture - will read in the already converted and precalculated texture.
//  This is our engine raw format so the loading will be much faster (especially when using RGBA mode).
//  As input parametes it takes the filename (*.tw, *.tf) and starting pointers for indexed and color table.
//  Memory for indexes and color table need to be allocated outside of this function.
//  and returns:
// 
//       0  when file is NULL
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Read_Texture_RAW(char* _input_filename, u_int32* _output_color_data_ptr, u_int8* _output_image_data_ptr, sIO_Prefs* _prefs)
{
    FILE* file_in;
    file_in = fopen(_input_filename, "rb");

    if (file_in == NULL) return 0;

    // Read in the intensity color tables - to where the '_output_color_data_ptr' points to.
    int32 read_color_data = (int32)fread(_output_color_data_ptr, IO_TEXTURE_COLOR_DATA_BYTE_SIZE, 1, file_in) * IO_TEXTURE_COLOR_DATA_BYTE_SIZE;

    // Read in all textures mip maps - to where the '_output_image_data_ptr' points to.
    int32 read_image_data = (int32)fread(_output_image_data_ptr, IO_TEXTURE_IMAGE_DATA_BYTE_SIZE, 1, file_in) * IO_TEXTURE_IMAGE_DATA_BYTE_SIZE;

    fclose(file_in);

    // By default the raw texture should be saved as ARGB, so if desired pixel format is ARGB
    // don't make any converion -->  if (_prefs->ch1 == 16 && _prefs->ch2 == 8 && _prefs->ch3 == 0)
      
    // From ARGBA to RGBA
    if (_prefs->ch1 == 24 && _prefs->ch2 == 16 && _prefs->ch3 == 8)
    {
        for (int32 intensity = 0; intensity < IO_TEXTURE_MAX_SHADES; intensity++)
        {
            int32 intensity__mul__my_num_colors = intensity * IO_TEXTURE_MAX_COLORS;

            for (int32 color = 0; color < IO_TEXTURE_MAX_COLORS; color++)
            {
                u_int32 source_color = _output_color_data_ptr[intensity__mul__my_num_colors + color];

                u_int8 s_aa = (source_color >> 24) & 0xFF;  // A
                u_int8 s_rr = (source_color >> 16) & 0xFF;  // R
                u_int8 s_gg = (source_color >> 8) & 0xFF;   // G
                u_int8 s_bb = source_color & 0xFF;          // B

                _output_color_data_ptr[intensity__mul__my_num_colors + color] = (s_rr << _prefs->ch1) | (s_gg << _prefs->ch2) | (s_bb << _prefs->ch3) | s_aa;
            }
        }
    }

    // From ARGB to BGRA
    if (_prefs->ch1 == 8 && _prefs->ch2 == 16 && _prefs->ch3 == 24)
    {
        for (int32 intensity = 0; intensity < IO_TEXTURE_MAX_SHADES; intensity++)
        {
            int32 intensity__mul__my_num_colors = intensity * IO_TEXTURE_MAX_COLORS;

            for (int32 color = 0; color < IO_TEXTURE_MAX_COLORS; color++)
            {
                u_int32 source_color = _output_color_data_ptr[intensity__mul__my_num_colors + color];

                u_int8 s_aa = (source_color >> 24) & 0xFF;  // A
                u_int8 s_rr = (source_color >> 16) & 0xFF;  // R
                u_int8 s_gg = (source_color >> 8) & 0xFF;   // G
                u_int8 s_bb = source_color & 0xFF;          // B

                _output_color_data_ptr[intensity__mul__my_num_colors + color] = (s_rr << _prefs->ch1) | (s_gg << _prefs->ch2) | (s_bb << _prefs->ch3) | s_aa;
            }
        }
    }

    return read_color_data + read_image_data;
}


//  ---------------------------------------------------------------------------------
// 
//  BM_Read_Lightmap_RAW_Separate - will read in the already converted and precalculated lightmaps bitmap,
//  it will split the data into separate buffers: wall, floor, ceil.
//  This is our engine raw format so the loading will be much faster.
//  As input parametes it takes the filename (*.lm), wall, floor and ceil lightmaps pointers and number of lightmaps.
//  Memory for lightmaps data need to be allocated outside of this function.
//
//  Used in Level Editor.
//  Returns:
// 
//       0  when file is NULL
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Read_Lightmaps_RAW_Separate(   char* _input_filename, u_int8* _output_wall_lm_ptr, u_int8* _output_floor_lm_ptr, u_int8* _output_ceil_lm_ptr, 
                                        int16 _wall_lightmaps, int16 _flat_lightmaps)
{
    FILE* file_in;
    file_in = fopen(_input_filename, "rb");

    if (file_in == NULL) return 0;

    // Read in wall lightmaps - they are saved first in file...
    int32 wall_lm_byte_size = _wall_lightmaps * IO_LIGHTMAP_BYTE_SIZE;
    int32 read_wall_lightmaps_data = (int32)fread(_output_wall_lm_ptr, wall_lm_byte_size, 1, file_in) * wall_lm_byte_size;

    // Read in flat maps, floor first then ceil...
    int32 floor_or_ceil_lm_byte_size = _flat_lightmaps * IO_LIGHTMAP_BYTE_SIZE / 2;

    int32 read_floor_lightmaps_data = (int32)fread(_output_floor_lm_ptr, floor_or_ceil_lm_byte_size, 1, file_in) * floor_or_ceil_lm_byte_size;
    int32 read_ceil_lightmaps_data = (int32)fread(_output_ceil_lm_ptr, floor_or_ceil_lm_byte_size, 1, file_in) * floor_or_ceil_lm_byte_size;

    fclose(file_in);

    return (read_wall_lightmaps_data + read_floor_lightmaps_data + read_ceil_lightmaps_data);
}


//  ---------------------------------------------------------------------------------
// 
//  BM_Read_Lightmap_RAW_All - will read in the already converted and precalculated
//  lightmaps bitmap AT ONCE into buffer.
//  This is our engine raw format so the loading will be much faster.
//  As input parametes it takes the filename (*.lm), lightmap buffer and number of lightmaps.
//  Memory for lightmaps data need to be allocated outside of this function.
//  and returns:
// 
//       0  when file is NULL
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Read_Lightmaps_RAW_All(char* _input_filename, u_int8* _output_lightmaps_ptr, int16 _lightmaps_num)
{
    FILE* file_in;
    file_in = fopen(_input_filename, "rb");

    if (file_in == NULL) return 0;

    // Read in all lightmaps at once - wall lightmaps are first, then floor lightmaps and ceil lightmaps as last.
    int32 lightmaps_byte_size = _lightmaps_num * IO_LIGHTMAP_BYTE_SIZE;
    int32 read_lightmaps_data = (int32)fread(_output_lightmaps_ptr, lightmaps_byte_size, 1, file_in) * lightmaps_byte_size;

    fclose(file_in);

    return read_lightmaps_data;
}


//  ---------------------------------------------------------------------------------
// 
//  Convert Bitmap Index to Wall Texture (will rotate 90* each mip map left and make it continuously and add intentisty color tables)
//  Convert Bitmap Index to Flat Texture (will make each mip map continuously and add intentisty color tables)
//  The colors will be saved as ARGB (swapped).
// 
//  Used in Level Editor.
//  Returns:
// 
//       0  when file is NULL
//      -2  when can't alloc mem for pixel indexes
//      -3  when can't alloc mem for color map
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Convert_And_Save_Bitmap_Indexed_To_Texture_RAW(char* _output_filename, sBM_Bitmap_Indexed* _input_bitmap, u_int8 _tex_type, u_int8 _light_mode, u_int8 _light_treshold)
{
    // Allocate space for 7 texture mipmaps - from 256x256 to 4x4 - only indexes so 1 byte per index.
    // This value is constant for all textures.

    u_int8* output_image_data = (u_int8*)malloc(IO_TEXTURE_IMAGE_DATA_BYTE_SIZE);
    if (output_image_data == NULL) return -2;

    // Lets allocate space not for one main color table - but for 256 color tables with decreasing intensity from original down to (0,0,0).
    // To get rid of problems with endiannes and byte order - lets put evetything byte per byte instead 4 byte at once.

    u_int32* output_intensity_color_data = (u_int32*)malloc(IO_TEXTURE_COLOR_DATA_BYTE_SIZE);
    if (output_intensity_color_data == NULL)
    {
        free(output_image_data);
        return -3;
    }

    int32 dst_index = 0;

    int32 mm_size[3] = { 128, 64, 32 };
    int32 mm_index = 0;

    int32 x0 = 0;
    int32 distance = 0;

    // For every mipmap...
    for (int32 mm = 0; mm < 3; mm++)
    {
        // For height of current mipmap...
        for (int32 y = 0; y < mm_size[mm_index]; y++)
        {
            if (_tex_type == 0)
            {
                // If we want to convert texture to WALL TEXTURE - it will be rotated 90* degree LEFT and saved continously line by line...

                // For x0 to x1 of current mipmap...
                for (int32 x = x0; x < (x0 + mm_size[mm_index]); x++)
                {
                    int xx = y;
                    int yy = mm_size[mm_index] - 1 - x  + x0;

                    output_image_data[distance + xx + yy * mm_size[mm_index]] = _input_bitmap->image_data[x + y * _input_bitmap->width];
                }
            }
            else
            {
                // If we want to convert texture to FLAT TEXTURE - it will be just saved continously line by line...

                // For x0 to x1 of current mipmap...
                for (int32 x = x0; x < (x0 + mm_size[mm_index]); x++)
                {
                    output_image_data[dst_index] = _input_bitmap->image_data[x + y * _input_bitmap->width];
                    dst_index++;
                }
            }
        }


        // Set starting x0 of next mipmap.
        x0 += mm_size[mm_index];

        distance = distance + mm_size[mm_index] * mm_size[mm_index];

        // Set size of next mipmap.
        mm_index++;
    }

    // Now, lets create out intensity LUT - by making 128 color tables for this texture.
    // We are using RGBA to ARGB order from input bitmap - not modify anything.
    for (int32 intensity = 0; intensity < IO_TEXTURE_MAX_SHADES; intensity++)
    {
        int32 index = 0;
        int32 intensity__mul__my_num_colors = intensity * IO_TEXTURE_MAX_COLORS;

        for (int32 color = 0; color < IO_TEXTURE_MAX_COLORS; color++)
        {
            // Get the pixel RGB values and calculate the intensity as (channel value * intensity) / 255.
            u_int32 source_color = _input_bitmap->color_data[color];

            u_int8 s_c1 = (source_color >> 24) & 0xFF;
            u_int8 s_c2 = (source_color >> 16) & 0xFF;
            u_int8 s_c3 = (source_color >> 8) & 0xFF;
            u_int8 s_c4 = source_color & 0xFF;

            u_int32 c1 = 0, c2 = 0, c3 = 0, c4 = 0;

            switch (_light_mode)
            {
            case 2:
                break;

            case 1:
            {
                u_int8 sum = 0;

                if (s_c1 >= _light_treshold) sum++;
                if (s_c2 >= _light_treshold) sum++;
                if (s_c3 >= _light_treshold) sum++;
                if (s_c4 >= _light_treshold) sum++;

                if (sum >= 1)
                {
                    c1 = s_c1;
                    c2 = s_c2;
                    c3 = s_c3;
                    c4 = s_c4;
                }
                else
                {
                    c1 = (s_c1 * intensity) / IO_TEXTURE_MAX_SHADES;
                    c2 = (s_c2 * intensity) / IO_TEXTURE_MAX_SHADES;
                    c3 = (s_c3 * intensity) / IO_TEXTURE_MAX_SHADES;
                    c4 = (s_c4 * intensity) / IO_TEXTURE_MAX_SHADES;
                }
            }
            break;

            default:
            {
                c1 = (s_c1 * intensity) / IO_TEXTURE_MAX_SHADES;
                c2 = (s_c2 * intensity) / IO_TEXTURE_MAX_SHADES;
                c3 = (s_c3 * intensity) / IO_TEXTURE_MAX_SHADES;
                c4 = (s_c4 * intensity) / IO_TEXTURE_MAX_SHADES;
            }
            break;
            }

            // ENDIANNES NOTE:
            // We want to save RAW data as Big-Endian for Amiga for faster loading.
            // Lets swap 32bit color values..

            // Save as ARGB order.
            output_intensity_color_data[intensity__mul__my_num_colors + index] = MA_swap_uint32( (c4 << 24) | (c1 << 16) | (c2 << 8) | c3 );

            index++;
        }
    }

    // Save all the data to file.
    FILE* output_file;

    output_file = fopen(_output_filename, "wb");

    if (output_file == NULL)
    {
        free(output_intensity_color_data);
        free(output_image_data);

        return 0;
    }

    // Then the color data and image data.
    fwrite(output_intensity_color_data, IO_TEXTURE_COLOR_DATA_BYTE_SIZE, 1, output_file);
    fwrite(output_image_data, IO_TEXTURE_IMAGE_DATA_BYTE_SIZE, 1, output_file);

    fclose(output_file);

    // Free memory resources.
    free(output_intensity_color_data);
    free(output_image_data);

    return IO_TEXTURE_IMAGE_DATA_BYTE_SIZE + IO_TEXTURE_COLOR_DATA_BYTE_SIZE;
}


//  ---------------------------------------------------------------------------------
// 
//  That functin will convert indexed TGA file that contains 32x32 px lightmaps tiles
//  into RAW lightmap file that 32x32 px lightmaps tiles are saved continuously.
//  The output RAW file is grayscaled, so it only contains 1 byte values per pixel. 
//  The first are wall lightmaps - rotated 90* degree right, then ceil lightmaps and then floor lightmaps.
// 
//  Used in Level Editor.
//  Returns:
// 
//       0  when file is NULL
//      -2  when can't alloc mem for pixel indexes
//     > 0  positive number is allocated memory size
// 
//  ------------------------------------------------------------------------
int32 BM_Convert_And_Save_Bitmap_Indexed_To_Lightmap_RAW(char* _output_filename, sBM_Bitmap_Indexed* _input_bitmap, int32 _wall_lm, int32 _flat_lm)
{
    // Allocate space for all lightmaps tiles - each tile is 32x32 px - multiply by sum of wall and flat lightmaps.
    // Values will be 1 byte per pixel.

    u_int32 output_size = IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE * (_wall_lm  + _flat_lm);

    u_int8* output_image_data = (u_int8*)malloc(output_size);
    if (output_image_data == NULL) return -2;

    int32 x = 0, y = 0;
    int32 wall_lm = 0;
    int32 dst_index = 0;

    // Lets convert, rotate and store wall lightmaps...
    for (y = 0; y < _input_bitmap->height / IO_LIGHTMAP_SIZE; y++)
    {
        for (x = 0; x < _input_bitmap->width / IO_LIGHTMAP_SIZE; x++)
        {
            if (dst_index < _wall_lm * IO_LIGHTMAP_SIZE * IO_LIGHTMAP_SIZE)
            {
                for (int32 xx = (IO_LIGHTMAP_SIZE - 1); xx >= 0; xx--)
                {
                    for (int32 yy = 0; yy < IO_LIGHTMAP_SIZE; yy++)
                    {
                        int32 src_index = IO_LIGHTMAP_SIZE * x + xx + (y * IO_LIGHTMAP_SIZE * _input_bitmap->width) + yy * _input_bitmap->width;

                        u_int8 input_index = _input_bitmap->image_data[src_index];
                        u_int32 input_color = _input_bitmap->color_data[input_index];

                        u_int8 input_shade = (input_color >> 16) & 0xFF;

                        output_image_data[dst_index] = input_shade;

                        dst_index++;
                    }
                }
            }
            else
            {
                for (int32 yy = 0; yy < IO_LIGHTMAP_SIZE; yy++)
                {
                    for (int32 xx = 0; xx < IO_LIGHTMAP_SIZE; xx++)
                    {
                        int32 src_index = IO_LIGHTMAP_SIZE * x + xx + (y * IO_LIGHTMAP_SIZE * _input_bitmap->width) + yy * _input_bitmap->width;

                        u_int8 input_index = _input_bitmap->image_data[src_index];
                        u_int32 input_color = _input_bitmap->color_data[input_index];

                        u_int8 input_shade = (input_color >> 16) & 0xFF;

                        output_image_data[dst_index] = input_shade;

                        dst_index++;
                    }
                }
            }

            wall_lm++;
            if (wall_lm >= _wall_lm + _flat_lm) break;
        }

        if (wall_lm >= _wall_lm +_flat_lm) break;
    }

    // Save all the data to file.
    FILE* output_file;
    output_file = fopen(_output_filename, "wb");

    if (output_file == NULL)
    {
        free(output_image_data);
        return 0;
    }

    // Then the color data and image data.
    int32 saved_size = (int32)fwrite(output_image_data, output_size, 1, output_file) * output_size;

    fclose(output_file);

    // Free memory resources.
    free(output_image_data);

    return saved_size;
}
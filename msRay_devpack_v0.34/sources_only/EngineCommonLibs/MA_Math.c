#include "MA_Math.h"

// ----------------------------------------------------------------------------------------------------
// --- FOR ENDIANNES SWAPPING - used for files created on different platforms like level maps, etc. ---
// ----------------------------------------------------------------------------------------------------
u_int16 MA_swap_uint16(u_int16 val)
{
    return (val << 8) | (val >> 8);
}
int16   MA_swap_int16(int16 val)
{
    return (val << 8) | ((val >> 8) & 0xFF);
}
u_int32 MA_swap_uint32(u_int32 val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}
int32   MA_swap_int32(int32 val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

void    MA_Add_Number_Spaces(int32 _number, char* _output_string)
{
    char tmp[16];
    memset(tmp, 0, sizeof(tmp));

    sprintf(tmp, "%d", (int)_number);

    int32 length = (int32)strlen(tmp);
    int32 spaces = length / 3;
    int32 index = length - 1 + spaces;
    int32 counter = 0;

    for (int32 i = length - 1; i >= 0; i--)
    {
        _output_string[index] = tmp[i];
        index--;

        counter++;
        if (counter == 3)
        {
            counter = 0;
            _output_string[index] = L' ';
            index--;
        }
    }
}
#include "GR_Visible_Cells_List.h"

#include "string.h"

void GR_VCL_Reset(void)
{
	// Cleanup the previous visible cell list.
	memset(GR_vcl_found, 0, LV_MAP_CELLS_COUNT);

	// Reset previous vc list size.
	GR_vcl_size = 0;
}
void GR_VCL_Add_Cell(int16 _cell_id)
{
	// Add cell id to the visible cell list.
	if (GR_vcl_found[_cell_id] == 0)
	{
		GR_vcl_list[GR_vcl_size] = _cell_id;
		GR_vcl_found[_cell_id] = 1;
		GR_vcl_size++;
	}
}

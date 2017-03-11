#pragma once

#include "Globals.h"

void swap(float *i1, float *i2)
{
	float *itemp = i1;
	i1 = i2;
	i2 = itemp;
}

void DrawLine(float sx,float sy, float ex, float ey, COLORREF color)
{
	float dx,dy;
	dx = abs(sx-ex);
	dy = abs(sy-ey);

	int lenght = (dx > dy) ? dx : dy;

	dx = dx/lenght;
	dy = dy/lenght;

	for(int i = 0; i < lenght; i++)
	{

		SetPixelV(hdc,sx,sy,color);
		sx+=dx;
		sy-=dy;
	}
}
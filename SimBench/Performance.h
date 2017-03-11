#pragma once

#include <windows.h>
#include <ctime>

// Performance
static LARGE_INTEGER lFreq, lStart;

static void PrecisionTimer()
{
	QueryPerformanceFrequency(&lFreq);
}

static void StartPerformance()
{
	PrecisionTimer();
	QueryPerformanceCounter(&lStart);
}

static void StopPerformance()
{
	LARGE_INTEGER lEnd;

	QueryPerformanceCounter(&lEnd);
	printf("\nPERFORMANCE: %f sek\n",(double(lEnd.QuadPart - lStart.QuadPart) / lFreq.QuadPart));
}
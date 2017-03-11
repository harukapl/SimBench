#pragma once

#include <conio.h>
#include <stdio.h>
#include <malloc.h>
#include <map>
#include <math.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <direct.h>// Katalog

// Moje
#include "Performance.h"

// LOG
#include <iostream>
#include <fstream>

#pragma warning(disable:4996 4018)

using namespace std;

#define INT_SIZE 4
#define BYTE_SIZE 1

#define BYTE unsigned char

#define SAFE_FREE(x) if (x != NULL) { free(x); x = NULL; }
#define SAFE_FILECLOSE(x) if (x != NULL) { fclose(x); x = NULL; }
//#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }

enum EGate {G_NONE = -1, G_OR, G_NOR, G_AND, G_NAND, G_XOR, G_XNOR, G_NOT, G_DFF, G_INPUT, G_OUTPUT } GType;

char *Skip = " (),=\r\n";	// Symbole do ignorowania
char *SkipError = " ->/";

HWND wnd;
HDC hdc;

int GRAFXSIZE = 400;
int GRAFYSIZE = 100;

void gotoxy(int x, int y) 
{ 
    COORD pos = {x, y};
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, pos);
}

int exit()
{
	getch();
	return 0;
}
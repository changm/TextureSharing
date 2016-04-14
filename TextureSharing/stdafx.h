// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

enum ConstantBuffers {
	WORLD,
	PROJECTION,
	VIEW,
	NUM_BUFFERS
};

static bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

// Since the incoming numbers are FLOATS, the values are actually 0-1, not 0-255
static void InitColor(FLOAT* aFloatOut, FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
	aFloatOut[0] = r;
	aFloatOut[1] = g;
	aFloatOut[2] = b;
	aFloatOut[3] = a;
}

// TODO: reference additional headers your program requires here
#include "DeviceManager.h"
#include "Drawing.h"
#include "Texture.h"
#include "pipe.h"


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
#include <stdio.h>

enum ConstantBuffers {
	WORLD,
	PROJECTION,
	VIEW,
	NUM_BUFFERS
};

static bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

static void PrintError(HRESULT aResult) {
	switch (aResult) {
	case D3D11_ERROR_FILE_NOT_FOUND:
		printf("File not found\n");
		break;
	case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
		printf("Too many unique state objects\n");
		break;
	case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
		printf("Too many unique view objects\n");
		break;
	case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
		printf("Error deferred context map\n");
		break;
	case DXGI_ERROR_INVALID_CALL:
		printf("d3d error invalid call\n");
		break;
	case DXGI_ERROR_WAS_STILL_DRAWING:
		printf("Still drawing\n");
		break;
	case E_FAIL:
		printf("E_FAIL debug fail\n");
		break;
	case E_INVALIDARG:
		printf("Invalid arg\n");
		break;
	case E_NOTIMPL:
		printf("error not implemented\n");
		break;
	case E_OUTOFMEMORY:
		printf("Error out of memory\n");
		break;
	case S_FALSE:
		printf("S_FALSE unknown error\n");
		break;
	default:
		printf("Unknown error\n");
		break;
	}
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


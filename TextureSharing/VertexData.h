#pragma once

#include <DirectXMath.h>
using namespace DirectX;

/**
 * Split the textures such that each texture gets 1/4 of the back buffer
 */
struct VertexData
{
	XMFLOAT3 Position; // Goes from -1..1
	XMFLOAT2 Tex;	// (u,v) goes from 0..1, with 0 being top left
};

static VertexData TopLeft[] =
{
	{ XMFLOAT3(-1, 0, 0), XMFLOAT2(0,1) }, // bottom left
	{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0,0) }, // top left
	{ XMFLOAT3(0, 1, 0), XMFLOAT2(1,0) },	// top right
	{ XMFLOAT3(0, 0, 0), XMFLOAT2(1,1) },  // bottom right
};

static VertexData TopRight[] =
{
	{ XMFLOAT3(0, 0, 0) , XMFLOAT2(0,1) }, // bottom left
	{ XMFLOAT3(0, 1, 0), XMFLOAT2(0,0) }, // top left
	{ XMFLOAT3(1, 1, 0), XMFLOAT2(1,0) },	// top right
	{ XMFLOAT3(1, 0, 0), XMFLOAT2(1,1) },  // bottom right
};

static VertexData BottomLeft[] =
{
	{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0,1) }, // bottom left
	{ XMFLOAT3(-1, 0, 0), XMFLOAT2(0,0) }, // top left
	{ XMFLOAT3(0, 0, 0), XMFLOAT2(1,0) },	// top right
	{ XMFLOAT3(0, -1, 0), XMFLOAT2(1,1) },  // bottom right
};

static VertexData BottomRight[] =
{
	{ XMFLOAT3(0, -1, 0), XMFLOAT2(0,1) }, // bottom left
	{ XMFLOAT3(0, 0, 0), XMFLOAT2(0,0) }, // top left
	{ XMFLOAT3(1, 0, 0), XMFLOAT2(1,0) },	// top right
	{ XMFLOAT3(1, -1, 0), XMFLOAT2(1,1) },  // bottom right
};
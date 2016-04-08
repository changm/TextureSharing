
#include "stdafx.h"
#include "Drawing.h"

#include "stdafx.h"
#include <stdio.h>
#include <dxgi.h>
#include <assert.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d3dcompiler.h>

#include "Vertex.h"
#include <DirectXMath.h>
#include "Texture.h"

using namespace DirectX;

// Since the incoming numbers are FLOATS, the values are actually 0-1, not 0-255
static void InitColor(FLOAT* aFloatOut, FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
	aFloatOut[0] = r;
	aFloatOut[1] = g;
	aFloatOut[2] = b;
	aFloatOut[3] = a;
}

Drawing::Drawing(HWND aOutputWindow,
								ID3D11Device* aDevice,
								ID3D11DeviceContext* aContext,
								LONG aWidth,
								LONG aHeight)
	: mOutputWindow(aOutputWindow)
	, mDevice(aDevice)
	, mContext(aContext)
	, mWidth(aWidth)
	, mHeight(aHeight)
{
	InitViewport();
	InitTexture();
	SetRenderTarget();
	
	InitMatrices();
	UpdateConstantBuffers();

	CompileShaders();
}

Drawing::~Drawing()
{
	mVertexShader->Release();
	mPixelShader->Release();
	mDevice->Release();
	mContext->Release();
	mVertexBuffer->Release();
	mIndexBuffer->Release();
	delete mTexture;
}

void Drawing::CompileShaders()
{
	HRESULT result;

	// Compile our shaders
	result = D3DCompileFromFile(L"VertexShader.hlsl", NULL, NULL,
								"main", "vs_5_0", 0, 0,
								&mVertexShaderBytecode, NULL);
	assert(SUCCESS(result));

	result = D3DCompileFromFile(L"PixelShader.hlsl", NULL, NULL,
								"main", "ps_5_0", 0, 0,
								&mPixelShaderBytecode, NULL);
	assert(SUCCESS(result));
	assert(mDevice);

	// Create the pixel shaders from the bytecode
	result = mDevice->CreatePixelShader(mPixelShaderBytecode->GetBufferPointer(), mPixelShaderBytecode->GetBufferSize(), NULL, &mPixelShader);
	assert(SUCCESS(result));

	mDevice->CreateVertexShader(mVertexShaderBytecode->GetBufferPointer(), mVertexShaderBytecode->GetBufferSize(), NULL, &mVertexShader);
	assert(SUCCESS(result));

	// Finally set them as our default pixel shaders
	mContext->VSSetShader(mVertexShader, 0, 0);
	mContext->PSSetShader(mPixelShader, 0, 0);
}

void Drawing::ClearRect(FLOAT* aRGBAColor)
{
	//mContext->ClearRenderTargetView(mBackBufferView, aRGBAColor);
}

void Drawing::InitViewport()
{
	// D3d goes from -1, 1 and that maps to device space via the viewport.
	D3D11_VIEWPORT viewport;
	memset(&viewport, 0, sizeof(D3D11_VIEWPORT));
	viewport.Width = mWidth;
	viewport.Height = mHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	mContext->RSSetViewports(1, &viewport);
}

void
Drawing::SetRenderTarget()
{
	ID3D11RenderTargetView* textureView = mTexture->GetRenderTargetView();
	mContext->OMSetRenderTargets(1, &textureView, NULL);
}

void Drawing::InitTexture()
{
	assert(mDevice);
	assert(mContext);
	mTexture = new Texture(mDevice, mContext);
	mTexture->AllocateTexture(mWidth, mHeight);
}

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

void Drawing::InitMatrices()
{
	mWorldMatrix = XMMatrixIdentity();

	// view matrix is basically telling the camera where to look
	XMVECTOR eyePosition = XMVectorSet(0, 0, -5, 1); // look -5 away for the camera
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);	// Look at the origin
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);	// Set up to the the Y axis, notice W is 0 here.
	mViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Projection matrix takes things in the -1..1 space and puts it onto the viewport
	// Compute the exact client dimensions. This will be used
	// to initialize the render targets for our swap chain.
	mProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), mWidth / mHeight, 0.1f, 100.0f);
}

void Drawing::UpdateConstantBuffers()
{
	D3D11_BUFFER_DESC constantBufferDesc;
	memset(&constantBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = mDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffers[ConstantBuffers::WORLD]);
	assert(SUCCESS(hr));

	hr = mDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffers[ConstantBuffers::PROJECTION]);
	assert(SUCCESS(hr));

	hr = mDevice->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffers[ConstantBuffers::VIEW]);
	assert(SUCCESS(hr));

	mContext->UpdateSubresource(mConstantBuffers[ConstantBuffers::PROJECTION], 0, nullptr, &mProjectionMatrix, 0, 0);
	mContext->UpdateSubresource(mConstantBuffers[ConstantBuffers::VIEW], 0, nullptr, &mViewMatrix, 0, 0);
	mContext->UpdateSubresource(mConstantBuffers[ConstantBuffers::WORLD], 0, nullptr, &mWorldMatrix, 0, 0);
	mContext->VSSetConstantBuffers(0, ConstantBuffers::NUM_BUFFERS, mConstantBuffers);
}

// let's try a square
VertexPosColor vertices[] =
{
	{ XMFLOAT3(-1, -1, 0), XMFLOAT3(1, 1, 1) }, // bottom left
	{ XMFLOAT3(-1, 1, 0), XMFLOAT3(1, 1, 1) },	// top left
	{ XMFLOAT3(1, 1, 0), XMFLOAT3(1, 1, 1) },		// top right
	{ XMFLOAT3(1, -1, 0), XMFLOAT3(1, 1, 1) },	// bottom right
};

int indices[] =
{
	0, 1, 2,	// bottom left, top left, top right
	3, 0, 2		// bottom right, bottom left, top right
};

void Drawing::SetIndexBuffers()
{
	D3D11_BUFFER_DESC indexBufferDesc;
	memset(&indexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	int indexCount = 6;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(int) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA indexResourceData;
	memset(&indexResourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	indexResourceData.pSysMem = indices;

	HRESULT result = mDevice->CreateBuffer(&indexBufferDesc, &indexResourceData, &mIndexBuffer);
	assert(SUCCESS(result));

	mContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Drawing::UploadVertices()
{
		// Now we create a buffer for our triangle
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPosColor) * _countof(vertices);	// Because we have 3 vertices
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA resourceData;
	memset(&resourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = vertices;

	// This uploads the data to the gpu
	HRESULT result = mDevice->CreateBuffer(&bufferDesc, &resourceData, &mVertexBuffer);
	assert(SUCCESS(result));

	// Finally draw the things, set the vertex buffers to the one that we uploaded to the gpu
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	mContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
}

void
Drawing::SetInputLayout()
{
	// Time to create the input buffer things
	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 12 since we have 3 floats bfeore the color
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ID3D11InputLayout* inputLayout;
	HRESULT result = mDevice->CreateInputLayout(inputDesc, 2,
														mVertexShaderBytecode->GetBufferPointer(),
														mVertexShaderBytecode->GetBufferSize(),
														&inputLayout);
	assert(SUCCESS(result));
	mContext->IASetInputLayout(inputLayout);
	// Tell the GPU we just have a list of triangles
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//mContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

ID3D11Texture2D* Drawing::Draw()
{
	UploadVertices();
	SetInputLayout();
	SetIndexBuffers();

	// Finally draw the 3 vertices we have
	int indexCount = 6;	// 2 triangles to make a square!
	mContext->DrawIndexed(indexCount, 0, 0);
	return mTexture->GetTexture();
}
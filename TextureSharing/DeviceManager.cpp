#include "stdafx.h"
#include "DeviceManager.h"
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

DeviceManager::DeviceManager(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	Init();
}

DeviceManager::~DeviceManager()
{
	mVertexShader->Release();
	mPixelShader->Release();
	mSwapChain->Release();
	mBackBufferView->Release();
	mDevice->Release();
	mContext->Release();
	mBackBuffer->Release();
	delete mTexture;
}

bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

void DeviceManager::Init()
{
	InitD3D();
	InitD2D();
	InitViewport();

	InitBackBuffer();
	InitTexture();
	SetRenderTarget();
	
	InitMatrices();
	UpdateConstantBuffers();

	CompileShaders();
}

void DeviceManager::CompileShaders()
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

// A color here should be a RGBA float
void DeviceManager::ClearRect(FLOAT* aRGBAColor)
{
	mContext->ClearRenderTargetView(mBackBufferView, aRGBAColor);
}

void DeviceManager::InitViewport()
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
DeviceManager::SetRenderTarget()
{
	ID3D11RenderTargetView* textureView = mTexture->GetRenderTargetView();
	mContext->OMSetRenderTargets(1, &textureView, NULL);
	//mContext->OMSetRenderTargets(1, &mBackBufferView, NULL);
}

void DeviceManager::InitTexture()
{
	assert(mDevice);
	assert(mContext);
	mTexture = new Texture(mDevice, mContext);
	mTexture->AllocateTexture(mWidth, mHeight);
}

void DeviceManager::InitBackBuffer()
{
	assert(mSwapChain);
	// Query information about the back buffer, don't actually use anything.
	HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&mBackBuffer);
	assert(SUCCESS(hr));

	hr = mDevice->CreateRenderTargetView(mBackBuffer, NULL, &mBackBufferView);
	assert(SUCCESS(hr));
}

void DeviceManager::InitD3D()
{
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&mFactory);
	assert(SUCCESS(hr));

	// Get the primary adapter, which is the GPU
	hr = mFactory->EnumAdapters1(0, &mAdapter);
	assert(SUCCESS(hr));

	// Get the device!
	hr = D3D11CreateDevice(mAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		NULL,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL, 0, // d3d feature levels
		D3D11_SDK_VERSION,
		&mDevice,
		NULL, &mContext);
	assert(SUCCESS(hr));


	RECT clientRect;
	GetClientRect(mOutputWindow, &clientRect);

	// Compute the exact client dimensions. This will be used
	// to initialize the render targets for our swap chain.
	mWidth = clientRect.right - clientRect.left;
	mHeight = clientRect.bottom - clientRect.top;

	// Create the swap chain
	DXGI_SWAP_CHAIN_DESC swapDesc;
	memset(&swapDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferDesc.Width = mWidth;
	swapDesc.BufferDesc.Height = mHeight;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Only 1 buffer since the desktop screen is our front buffer?
	swapDesc.BufferCount = 1;
	swapDesc.OutputWindow = mOutputWindow;
	swapDesc.Windowed = TRUE;
	swapDesc.Flags = 0;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

	hr = mFactory->CreateSwapChain(mDevice, &swapDesc, &mSwapChain);
	assert(SUCCESS(hr));
}

void DeviceManager::InitD2D()
{
	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_MULTI_THREADED,
		&mD2DFactory);
	assert(SUCCESS(hr));
}

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

void DeviceManager::DrawTriangle()
{
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1, -1, 0), XMFLOAT3(1, 1, 1) }, // origin
		{ XMFLOAT3(-1, 1, 0), XMFLOAT3(1, 1, 1) }, // y
		{ XMFLOAT3(1, 1, 0), XMFLOAT3(1, 1, 1) },
	};

	// Now we create a buffer for our triangle
	ID3D11Buffer* triangleBuffer;
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
	HRESULT result = mDevice->CreateBuffer(&bufferDesc, &resourceData, &triangleBuffer);
	assert(SUCCESS(result));

	// Time to create the input buffer things
	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// 12 since we have 3 floats bfeore the color
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ID3D11InputLayout* inputLayout;
	result = mDevice->CreateInputLayout(inputDesc, 2,
		mVertexShaderBytecode->GetBufferPointer(),
		mVertexShaderBytecode->GetBufferSize(),
		&inputLayout);
	assert(SUCCESS(result));
	mContext->IASetInputLayout(inputLayout);

	// Finally draw the things, set the vertex buffers to the one that we uploaded to the gpu
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	mContext->IASetVertexBuffers(0, 1, &triangleBuffer, &stride, &offset);

	// Tell the GPU we just have a list of triangles
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Finally draw the 3 vertices we have
	int vertexCount = 3;
	mContext->Draw(vertexCount, 0);
}

void DeviceManager::InitMatrices()
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

void DeviceManager::UpdateConstantBuffers()
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

void DeviceManager::CopyToBackBuffer()
{
	mContext->CopyResource(mBackBuffer, mTexture->GetTexture());
}

void DeviceManager::Draw()
{
	DrawTriangle();
	CopyToBackBuffer();
	mSwapChain->Present(0, 0);
}
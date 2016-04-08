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
#include "Drawing.h"

using namespace DirectX;

DeviceManager::DeviceManager(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	InitD3D();
	InitD2D();
	InitBackBuffer();
}

DeviceManager::~DeviceManager()
{
	mSwapChain->Release();
	mBackBufferView->Release();
	mDevice->Release();
	mContext->Release();
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

void
DeviceManager::CopyToBackBuffer(ID3D11Texture2D* aTexture)
{
	mContext->CopyResource(mBackBuffer, aTexture);
}

void
DeviceManager::CompileTextureShaders()
{
	HRESULT result;

	// Compile our shaders
	result = D3DCompileFromFile(L"TextureMappingVS.hlsl", NULL, NULL,
								"main", "vs_5_0", 0, 0,
								&mVertexShaderBytecode, NULL);
	assert(SUCCESS(result));

	result = D3DCompileFromFile(L"TextureMappingPS.hlsl", NULL, NULL,
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

struct VertexData
{
	XMFLOAT3 Position; // Goes from -1..1
	XMFLOAT2 Tex;	// (u,v) goes from 0..1, with 0 being top left
};

void
DeviceManager::InitVertexBuffers()
{
	VertexData vertices[] =
	{
		{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0,1) }, // bottom left
		{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0,0) },  // top left
		{ XMFLOAT3(1, 1, 0), XMFLOAT2(1,0) },	  // top right
		{ XMFLOAT3(1, -1, 0), XMFLOAT2(1,1) },  // bottom right
	};

	// Now we create a buffer for our triangle
	ID3D11Buffer* screenBuffer;
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexData) * _countof(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA resourceData;
	memset(&resourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	resourceData.pSysMem = vertices;

	// This uploads the data to the gpu
	HRESULT result = mDevice->CreateBuffer(&bufferDesc, &resourceData, &screenBuffer);
	assert(SUCCESS(result));

	// Time to create the input buffer things
	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexData, Tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ID3D11InputLayout* inputLayout;
	const int inputCount = sizeof(inputDesc) / sizeof(VertexData);
	//assert(inputCount == 2);

	result = mDevice->CreateInputLayout(inputDesc, 2,
		mVertexShaderBytecode->GetBufferPointer(),
		mVertexShaderBytecode->GetBufferSize(),
		&inputLayout);
	assert(SUCCESS(result));
	mContext->IASetInputLayout(inputLayout);

	// Finally draw the things, set the vertex buffers to the one that we uploaded to the gpu
	UINT stride = sizeof(VertexData);
	UINT offset = 0;
	mContext->IASetVertexBuffers(0, 1, &screenBuffer, &stride, &offset);

	// Tell the GPU we just have a list of triangles
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Finally draw the 3 vertices we have
	int vertexCount = 4;
	mContext->Draw(vertexCount, 0);
}

void
DeviceManager::DrawViaTextureShaders(ID3D11Texture2D* aTexture)
{
	mContext->OMSetRenderTargets(0, &mBackBufferView, NULL);
	CompileTextureShaders();
	InitVertexBuffers();
}

void DeviceManager::Draw()
{
	Drawing draw(mOutputWindow, mDevice, mContext, mWidth, mHeight);
	ID3D11Texture2D* result = draw.Draw();
	CopyToBackBuffer(result);
	//DrawViaTextureShaders(result);
	mSwapChain->Present(0, 0);
}
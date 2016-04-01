#include "stdafx.h"
#include "DeviceManager.h"
#include <stdio.h>
#include <dxgi.h>
#include <assert.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d3dcompiler.h>

DeviceManager::DeviceManager(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	Init();
}

DeviceManager::~DeviceManager()
{
	mSwapChain->Release();
	mBackBuffer->Release();
	mDevice->Release();
	mContext->Release();
}

bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

void DeviceManager::Init()
{
	InitD3D();
	InitD2D();
	InitBackBuffer();
	InitViewport();

	CompileShaders();
}

void DeviceManager::CompileShaders()
{
	HRESULT result;
	ID3D10Blob* vertexShader;
	ID3D10Blob* pixelShader;

	// Compile our shaders
	result = D3DCompileFromFile(L"VertexShader.hlsl", NULL, NULL,
								"VertexShaderMain", "vs_5_0", 0, 0,
								&vertexShader, NULL);
	assert(SUCCESS(result));

	result = D3DCompileFromFile(L"PixelShader.hlsl", NULL, NULL,
								"main", "ps_5_0", 0, 0,
								&pixelShader, NULL);
	assert(SUCCESS(result));
	assert(mDevice);

	// Create the pixel shaders from the bytecode
	result = mDevice->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), NULL, &mPixelShader);
	assert(SUCCESS(result));

	mDevice->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), NULL, &mVertexShader);
	assert(SUCCESS(result));

	// Finally set them as our default pixel shaders
	mContext->VSSetShader(mVertexShader, 0, 0);
	mContext->PSSetShader(mPixelShader, 0, 0);
}

// A color here should be a RGBA float
void DeviceManager::ClearRect(FLOAT* aRGBAColor)
{
	mContext->ClearRenderTargetView(mBackBuffer, aRGBAColor);
}

void DeviceManager::InitViewport()
{
	// D3d goes from -1, 1 and that maps to device space via the viewport.
	D3D11_VIEWPORT viewport;
	memset(&viewport, 0, sizeof(D3D11_VIEWPORT));
	viewport.Height = 1024;
	viewport.Width = 1024;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	mContext->RSSetViewports(1, &viewport);
}

void DeviceManager::InitBackBuffer()
{
	assert(mSwapChain);
	ID3D11Texture2D* backBufferInfo;
	// Query information about the back buffer, don't actually use anything.
	HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferInfo);
	assert(SUCCESS(hr));

	hr = mDevice->CreateRenderTargetView(backBufferInfo, NULL, &mBackBuffer);
	assert(SUCCESS(hr));
	// sadly we only need the information to create the render target view
	backBufferInfo->Release();

	mContext->OMSetRenderTargets(1, &mBackBuffer, NULL);
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

	// Create the swap chain
	DXGI_SWAP_CHAIN_DESC swapDesc;
	memset(&swapDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferDesc.Width = 0;
	swapDesc.BufferDesc.Height = 0;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
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

void DeviceManager::Draw()
{
	printf("DeviceManager::Draw");
	FLOAT red[4];
	red[0] = 255;
	red[1] = 0;
	red[2] = 0;
	red[3] = 0;
	ClearRect(red);

	mSwapChain->Present(0, 0);
}
#include "stdafx.h"
#include "DeviceManager.h"
#include <stdio.h>
#include <dxgi.h>
#include <assert.h>
#include <d3d11.h>
#include <d2d1.h>

DeviceManager::DeviceManager(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	Init();
}

bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

void DeviceManager::Init()
{
	InitD3D();
	InitD2D();
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
	mContext->
}
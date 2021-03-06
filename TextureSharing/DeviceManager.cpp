#include "stdafx.h"
#include "DeviceManager.h"
#include <stdio.h>
#include <dxgi.h>
#include <assert.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d3dcompiler.h>
#include <D3D11SDKLayers.h>
#include <DXGIDebug.h>

#include "Vertex.h"
#include <DirectXMath.h>
#include "Texture.h"
#include "Drawing.h"

using namespace DirectX;

DeviceManager::DeviceManager()
{
	InitD3D();
	InitD2D();
}

DeviceManager::~DeviceManager()
{
	mFactory->Release();
	mContext->Release();
	mAdapter->Release();
	mD2DFactory->Release();
	mDevice->Release();
}

void DeviceManager::ReportLiveObjects()
{
	ID3D11Debug* debug;
	mDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
	if (!debug) {
		return;
	}

	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	debug->Release();
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
		//D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL, 0, // d3d feature levels
		D3D11_SDK_VERSION,
		&mDevice,
		NULL, &mContext);
	assert(SUCCESS(hr));
}

void DeviceManager::CreateSwapChain(IDXGISwapChain** aOutChain, LONG aWidth, LONG aHeight, HWND aOutputWindow)
{
	assert(aWidth);
	assert(aHeight);
	// Create the swap chain
	DXGI_SWAP_CHAIN_DESC swapDesc;
	memset(&swapDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferDesc.Width = aWidth;
	swapDesc.BufferDesc.Height = aHeight;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Only 1 buffer since the desktop screen is our front buffer?
	swapDesc.BufferCount = 1;
	swapDesc.OutputWindow = aOutputWindow;
	swapDesc.Windowed = TRUE;
	swapDesc.Flags = 0;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

	HRESULT hr = mFactory->CreateSwapChain(mDevice, &swapDesc, aOutChain);
	assert(SUCCESS(hr));
}

void DeviceManager::InitD2D()
{
	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_MULTI_THREADED,
		&mD2DFactory);
	assert(SUCCESS(hr));
}
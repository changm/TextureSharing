#pragma once
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXMath.h>

using namespace DirectX;

class Texture;

// Manages our d3d devices for us
class DeviceManager {
public:
	DeviceManager();
	~DeviceManager();

	ID3D11Device* GetDevice() { return mDevice; }
	ID3D11DeviceContext* GetDeviceContext() { return mContext; }
	void CreateSwapChain(IDXGISwapChain** aOutSwapChain, LONG aWidth, LONG aHeight, HWND aOutputWindow);

private:
	// Init all the things
	void InitD3D();
	void InitD2D();

	// Setup D3D
	IDXGIFactory1* mFactory;
	ID3D11Device* mDevice;
	IDXGIAdapter1* mAdapter;
	ID3D11DeviceContext* mContext;

	// Now we can setup D2D
	ID2D1Factory* mD2DFactory;
};

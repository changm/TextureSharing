#pragma once
#include <d3d11.h>
#include <d2d1.h>

// Manages our d3d devices for us
class DeviceManager {
public:
	DeviceManager(HWND aHDC);
	void Draw();

private:
	void Init();
	void InitD3D();
	void InitD2D();

	// Setup D3D
	IDXGIFactory1* mFactory;
	ID3D11Device* mDevice;
	IDXGIAdapter1* mAdapter;
	IDXGISwapChain* mSwapChain;
	ID3D11DeviceContext* mContext;
	HWND mOutputWindow;

	// Now we can setup D2D
	ID2D1Factory* mD2DFactory;

};

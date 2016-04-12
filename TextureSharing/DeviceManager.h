#pragma once
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXMath.h>

using namespace DirectX;

class Texture;

// Manages our d3d devices for us
class DeviceManager {
public:
	DeviceManager(HWND aHDC);
	void Draw();
	~DeviceManager();

	ID3D11Device* GetDevice() { return mDevice; }
	ID3D11DeviceContext* GetDeviceContext() { return mContext; }
	IDXGISwapChain* GetSwapChain() { return mSwapChain; }
	HWND GetOutputWindow() { return mOutputWindow;  }
	LONG GetBackBufferWidth() { return mWidth;  }
	LONG GetBackBufferHeight() { return mHeight;  }

private:
	// Init all the things
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

	// Stuff we're actually rendering to
	// A render target is just a wrapper around the back buffer!
	ID3D11RenderTargetView* mBackBufferView;
	ID3D11Texture2D* mBackBuffer;

	LONG mWidth;
	LONG mHeight;
};

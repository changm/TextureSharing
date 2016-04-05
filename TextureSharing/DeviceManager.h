#pragma once
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXMath.h>

using namespace DirectX;

enum ConstantBuffers {
	WORLD,
	PROJECTION,
	VIEW,
	NUM_BUFFERS
};

// Manages our d3d devices for us
class DeviceManager {
public:
	DeviceManager(HWND aHDC);
	void Draw();
	~DeviceManager();

private:
	// Init all the things
	void Init();
	void InitD3D();
	void InitD2D();
	void InitBackBuffer();
	void InitViewport();
	void UpdateConstantBuffers();
	void InitMatrices();

	// draw a clear color.
	void ClearRect(FLOAT* aRGBAColor);
	void DrawTriangle();

	// Let's use some shaders now
	void CompileShaders();
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D10Blob* mVertexShaderBytecode;
	ID3D10Blob* mPixelShaderBytecode;

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
	ID3D11RenderTargetView* mBackBuffer;

	ID3D11Buffer* mConstantBuffers[ConstantBuffers::NUM_BUFFERS];

	XMMATRIX mProjectionMatrix;
	XMMATRIX mViewMatrix;
	XMMATRIX mWorldMatrix;
};

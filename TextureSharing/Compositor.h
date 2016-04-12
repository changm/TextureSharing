#pragma once
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXMath.h>

using namespace DirectX;

class DeviceManager;
class Texture;

class Compositor {
public:
	Compositor(HWND aOutputWindow);
	~Compositor();

	void Composite();
	static Compositor* GetCompositor(HWND aOutputWindow);

private:
	void ReadTextures();
	void CopyToBackBuffer(ID3D11Texture2D* aTexture);
	void InitBackBuffer();
	void InitSwapChain();

	HWND mOutputWindow;
	static Compositor* mCompositor;
	DeviceManager* mDeviceManager;

	ID3D11DeviceContext* mContext;
	ID3D11Device* mDevice;
	IDXGISwapChain* mSwapChain;
	ID3D11SamplerState* mSamplerState;

	// functions required to draw via shaders
	void DrawViaTextureShaders(ID3D11Texture2D* aTexture);
	void CompileTextureShaders();
	void InitVertexBuffers();
	void SetInputLayout();
	void SetIndexBuffers();
	void SetTextureSampling(ID3D11Texture2D* aTexture);

	// Things to draw the texture
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;
	ID3D11ShaderResourceView* mTextureView;

	ID3D10Blob* mVertexShaderBytecode;
	ID3D10Blob* mPixelShaderBytecode;
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	LONG mWidth;
	LONG mHeight;
	// Stuff we're actually rendering to
	// A render target is just a wrapper around the back buffer!
	ID3D11RenderTargetView* mBackBufferView;
	ID3D11Texture2D* mBackBuffer;
};
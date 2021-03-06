#pragma once
#include <d3d11.h>
#include <d2d1.h>
#include <DirectXMath.h>
#include <vector>
#include <map>

using namespace DirectX;

class DeviceManager;
class Texture;
struct VertexData;

class Compositor {
public:
	Compositor(HWND aOutputWindow);
	~Compositor();

	void Composite(std::vector<HANDLE>& aSharedHandles, HANDLE aSyncHandle);
	void CompositeWithSync(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle);
	static Compositor* GetCompositor(HWND aOutputWindow);
	LONG GetWidth() { return mWidth; }
	LONG GetHeight() { return mHeight; }
	void ResizeBuffers();
	void ReportLiveObjects();
	void Clean();
	void InitSyncTexture(HANDLE aSyncHandle);

private:
	void InitDrawQuery();
	void InitViewport();
	void CalculateDimensions();
	void CopyToBackBuffer(Texture* aTexture);
	void CopyToBackBuffer(ID3D11Texture2D* aTexture);
	void InitBackBuffer();
	void InitColors(FLOAT aColors[][4], int aCount);
	void LockSyncHandle(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle);
	void WaitForDrawExecution();
	ID3D11Texture2D* GetTexture(HANDLE aHandle);

	HWND mOutputWindow;
	static Compositor* mCompositor;
	DeviceManager* mDeviceManager;

	ID3D11DeviceContext* mContext;
	ID3D11Device* mDevice;
	IDXGISwapChain* mSwapChain;
	ID3D11SamplerState* mSamplerState;

	// functions required to draw via shaders
	void DrawViaTextureShaders(Texture* aTexture, VertexData* aLocation);
	void DrawViaTextureShaders(ID3D11Texture2D* aTexture, VertexData* aLocation);
	void PrepareDrawing();
	void CompileTextureShaders();
	void InitVertexBuffers();
	void SetVertexBuffers(VertexData* aData);
	void SetInputLayout();
	void SetIndexBuffers();
	void SetTextureSampling(ID3D11Texture2D* aTexture);

	// Things to draw the texture
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;
	ID3D11ShaderResourceView* mTextureView;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	LONG mWidth;
	LONG mHeight;
	// Stuff we're actually rendering to
	// A render target is just a wrapper around the back buffer!
	ID3D11RenderTargetView* mBackBufferView;
	ID3D11Texture2D* mBackBuffer;

	ID3D11Texture2D* mSyncTexture;
	std::map<HANDLE, ID3D11Texture2D*> mTextures;
	HANDLE mMutex;

	ID3D11Query* mDrawQuery;
};
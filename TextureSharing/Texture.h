#pragma once
#include <d3d11.h>

// Represents a texture. Allocates it on the GPU, locks / unlocks it all.
class Texture
{
public:
	Texture(LONG aWidth, LONG aHeight);
	~Texture();

	static Texture* AllocateTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, LONG aWidth, LONG aHeight, bool aUseMutex);
	ID3D11Texture2D* GetTexture() {
		return mTexture;
	}

	ID3D11RenderTargetView* GetRenderTargetView() {
		return mTextureRenderTarget;
	}

	HANDLE GetSharedHandle() {
		return mSharedHandle;
	}

	void Lock();
	void Unlock();
	LONG GetWidth() { return mWidth; }
	LONG GetHeight() { return mHeight; }

protected:
	void InitTextureRenderTarget(ID3D11Device* aDevice);
	void InitShaderResourceView(ID3D11Device* aDevice);

	HANDLE mSharedHandle;
	ID3D11Texture2D* mTexture;
	ID3D11RenderTargetView* mTextureRenderTarget;
	ID3D11ShaderResourceView* mShaderResourceView; // who knows yet
	IDXGIKeyedMutex* mMutex;

	LONG mWidth;
	LONG mHeight;
};
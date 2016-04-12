#pragma once
#include <d3d11.h>

// Represents a texture. Allocates it on the GPU, locks / unlocks it all.
class Texture
{
public:
	Texture(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext);
	~Texture();

	void AllocateTexture(int aWidth, int aHeight);
	void Deallocate();
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

private:
	void InitTextureRenderTarget();
	void InitShaderResourceView();

	HANDLE mSharedHandle;
	ID3D11DeviceContext* mContext;
	ID3D11Device* mDevice;
	ID3D11Texture2D* mTexture;
	ID3D11RenderTargetView* mTextureRenderTarget;
	ID3D11ShaderResourceView* mShaderResourceView; // who knows yet
	IDXGIKeyedMutex* mMutex;

	int mWidth;
	int mHeight;
};
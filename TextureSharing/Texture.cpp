#include "stdafx.h"
#include "Texture.h"
#include <stdio.h>

#include <d3d11.h>
#include <assert.h>

Texture::Texture(ID3D11Device* aDevice, ID3D11DeviceContext* aDeviceContext)
	: mDevice(aDevice)
	, mContext(aDeviceContext)
{
	assert(mDevice);
	mDevice->AddRef();
	mContext->AddRef();
}

Texture::~Texture()
{
	Deallocate();
}

void
Texture::Lock()
{
	mTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mMutex);
	if (mMutex) {
		HRESULT hr = mMutex->AcquireSync(0, 10000);
		assert(SUCCESS(hr));
	}
}

void
Texture::Unlock()
{
	if (mMutex) {
		HRESULT hr = mMutex->ReleaseSync(0);
		assert(SUCCESS(hr));
		mMutex->Release();
		mMutex = nullptr;
	}
}

void
Texture::AllocateTexture(int aWidth, int aHeight)
{
	assert(aWidth);
	assert(aHeight);
	mWidth = aWidth;
	mHeight = aHeight;

	// This is only because our d2d backend does this see:
	// https://dxr.mozilla.org/mozilla-central/source/gfx/layers/d3d11/TextureD3D11.cpp#357
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	CD3D11_TEXTURE2D_DESC bufferDesc(DXGI_FORMAT_B8G8R8A8_UNORM, mWidth, mHeight,
																		1, 1, bindFlags);

	// Shared w/o a mutex
	//bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	HRESULT hr = mDevice->CreateTexture2D(&bufferDesc, nullptr, &mTexture);
	assert(hr == S_OK);

	// Get a shared handle
	IDXGIResource* sharedResource;
	hr = mTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&sharedResource);
	assert(hr == S_OK);
	sharedResource->GetSharedHandle(&mSharedHandle);
	sharedResource->Release();

	InitTextureRenderTarget();	// the RT wraps around the texture so that we draw to it.
	InitShaderResourceView();
}

void
Texture::Deallocate()
{
	if (mTexture) {
		printf("Deallocating texture\n");
		mTexture->Release();
		mTextureRenderTarget->Release();
		mShaderResourceView->Release();
		mDevice->Release();

		if (mMutex) {
			mMutex->Release();
		}
	}
}

void
Texture::InitShaderResourceView()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
	memset(&shaderViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderViewDesc.Texture2D.MipLevels = 1;
	shaderViewDesc.Texture2D.MostDetailedMip = 0;
	HRESULT result = mDevice->CreateShaderResourceView(mTexture, &shaderViewDesc, &mShaderResourceView);
	assert(result == S_OK);
}

void
Texture::InitTextureRenderTarget()
{
	printf("Init texture render target\n");
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
	memset(&renderTargetDesc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderTargetDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetDesc.Texture2D.MipSlice = 0;

	HRESULT hr = mDevice->CreateRenderTargetView(mTexture, &renderTargetDesc, &mTextureRenderTarget);
	assert(hr == S_OK);
}

#include "stdafx.h"
#include "Texture.h"
#include <stdio.h>

#include <d3d11.h>
#include <assert.h>

Texture::Texture(LONG aWidth, LONG aHeight, bool aUseMutex)
	: mWidth(aWidth)
	, mHeight(aHeight)
	, mUseMutex(aUseMutex)
{
}

Texture::~Texture()
{
	mTexture->Release();
	mTextureRenderTarget->Release();
	mShaderResourceView->Release();
	CloseHandle(mSharedHandle);
	mMutex = nullptr;
}

void
Texture::Lock()
{
	if (mUseMutex) {
		mTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mMutex);
		HRESULT hr = mMutex->AcquireSync(0, INFINITE);
		assert(SUCCESS(hr));
	}
}

void
Texture::Unlock()
{
	if (mUseMutex) {
		HRESULT hr = mMutex->ReleaseSync(0);
		assert(SUCCESS(hr));
		mMutex->Release();
		mMutex = nullptr;
	}
}

/* static */ Texture*
Texture::AllocateTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, LONG aWidth, LONG aHeight, bool aUseMutex)
{
	assert(aWidth);
	assert(aHeight);
	Texture* texture = new Texture(aWidth, aHeight, aUseMutex);

	// This is only because our d2d backend does this see:
	// https://dxr.mozilla.org/mozilla-central/source/gfx/layers/d3d11/TextureD3D11.cpp#357
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	CD3D11_TEXTURE2D_DESC bufferDesc(DXGI_FORMAT_B8G8R8A8_UNORM, aWidth, aHeight,
																		1, 1, bindFlags);

	UINT miscFlags = aUseMutex ? D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX : D3D11_RESOURCE_MISC_SHARED;
	bufferDesc.MiscFlags = miscFlags;

	HRESULT hr = aDevice->CreateTexture2D(&bufferDesc, nullptr, &texture->mTexture);
	assert(hr == S_OK);

	// Get a shared handle
	IDXGIResource* sharedResource;
	hr = texture->mTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&sharedResource);
	assert(hr == S_OK);
	sharedResource->GetSharedHandle(&texture->mSharedHandle);
	sharedResource->Release();

	texture->InitShaderResourceView(aDevice);
	texture->InitTextureRenderTarget(aDevice);
	return texture;
}

void
Texture::InitShaderResourceView(ID3D11Device* aDevice)
{
	Lock();
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDesc;
	memset(&shaderViewDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	shaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderViewDesc.Texture2D.MipLevels = 1;
	shaderViewDesc.Texture2D.MostDetailedMip = 0;
	HRESULT result = aDevice->CreateShaderResourceView(mTexture, &shaderViewDesc, &mShaderResourceView);
	assert(result == S_OK);
	Unlock();
}

void
Texture::InitTextureRenderTarget(ID3D11Device* aDevice)
{
	Lock();
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
	memset(&renderTargetDesc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderTargetDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetDesc.Texture2D.MipSlice = 0;

	HRESULT hr = aDevice->CreateRenderTargetView(mTexture, &renderTargetDesc, &mTextureRenderTarget);
	assert(hr == S_OK);
	Unlock();
}

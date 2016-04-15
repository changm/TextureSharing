#include "stdafx.h"

#include "stdafx.h"
#include "SyncTexture.h"
#include <stdio.h>

#include <d3d11.h>
#include <assert.h>

SyncTexture::SyncTexture(LONG aWidth, LONG aHeight)
	: Texture(aWidth, aHeight)
{

}

/* static */
SyncTexture* SyncTexture::AllocateSyncTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, LONG aWidth, LONG aHeight)
{
	assert(aWidth);
	assert(aHeight);
	SyncTexture* texture = new SyncTexture(aWidth, aHeight);

	// This is only because our d2d backend does this see:
	// https://dxr.mozilla.org/mozilla-central/source/gfx/layers/d3d11/TextureD3D11.cpp#357
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	CD3D11_TEXTURE2D_DESC bufferDesc(DXGI_FORMAT_B8G8R8A8_UNORM, aWidth, aHeight,
																		1, 1, bindFlags);

	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

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
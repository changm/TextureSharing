#include "stdafx.h"
#include "Texture.h"
#include <stdio.h>

#include <d3d11.h>
#include <assert.h>

Texture::Texture(ID3D11Device* aDevice)
	: mDevice(aDevice)
{
	assert(mDevice);
}

Texture::~Texture()
{
	Deallocate();
}

void
Texture::Allocate()
{
	printf("Allocating texture\n");
	assert(mDevice);
	// This is only because our d2d backend does this see:
	// https://dxr.mozilla.org/mozilla-central/source/gfx/layers/d3d11/TextureD3D11.cpp#357
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	CD3D11_TEXTURE2D_DESC bufferDesc(DXGI_FORMAT_B8G8R8A8_UNORM, mWidth, mHeight,
																		1, 1, bindFlags);

	// Shared w/o a mutex
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	HRESULT hr = mDevice->CreateTexture2D(&bufferDesc, nullptr, &mTexture);
	assert(hr == S_OK);
}

void
Texture::Deallocate()
{
	if (mTexture) {
		printf("Deallocating texture\n");
		mTexture->Release();
	}
}


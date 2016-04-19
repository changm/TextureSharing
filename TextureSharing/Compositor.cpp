#include "stdafx.h"

#include "Compositor.h"
#include "DeviceManager.h"
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Drawing.h"
#include <stdio.h>
#include "VertexData.h"
#include <vector>
#include "TextureMappingPS.h"
#include "TextureMappingVS.h"

Compositor* Compositor::mCompositor = nullptr;
Compositor::Compositor(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	CalculateDimensions();

	mDeviceManager = new DeviceManager();
	mContext = mDeviceManager->GetDeviceContext();
	mDevice = mDeviceManager->GetDevice();

	mContext->AddRef();
	mDevice->AddRef();

	mDeviceManager->CreateSwapChain(&mSwapChain, mWidth, mHeight, mOutputWindow);
	PrepareDrawing();
}

Compositor::~Compositor()
{
	delete mDeviceManager;
}

void
Compositor::Clean()
{
	for (auto it = mTextures.begin(); it != mTextures.end(); it++) {
		ID3D11Texture2D* sharedTexture = it->second;
		sharedTexture->Release();
	}
	mTextures.clear();

	mDrawQuery->Release();
	mSyncTexture->Release();
	mDevice->Release();
	mContext->Release();
	mVertexBuffer->Release();
	mVertexShader->Release();
	mIndexBuffer->Release();
	mPixelShader->Release();
	mSwapChain->Release();
	mBackBuffer->Release();
	mBackBufferView->Release();
}

void
Compositor::ResizeBuffers()
{
	mBackBuffer->Release();
	mBackBufferView->Release();
	mBackBuffer = nullptr;
	mBackBufferView = nullptr;

	mSwapChain->ResizeBuffers(0, mWidth, mHeight, DXGI_FORMAT_UNKNOWN, 0);
	InitBackBuffer();
	mContext->OMSetRenderTargets(1, &mBackBufferView, NULL);
}

void
Compositor::CalculateDimensions()
{
	RECT clientRect;
	GetClientRect(mOutputWindow, &clientRect);

	// Compute the exact client dimensions. This will be used
	// to initialize the render targets for our swap chain.
	mWidth = clientRect.right - clientRect.left;
	mHeight = clientRect.bottom - clientRect.top;
}

void
Compositor::CompileTextureShaders()
{
	HRESULT result;
	result = mDevice->CreatePixelShader(g_TexturePixelShader, sizeof(g_TexturePixelShader), NULL, &mPixelShader);
	assert(SUCCESS(result));

	mDevice->CreateVertexShader(g_TextureVertexShader, sizeof(g_TextureVertexShader), NULL, &mVertexShader);
	assert(SUCCESS(result));

	// Finally set them as our default pixel shaders
	mContext->VSSetShader(mVertexShader, 0, 0);
	mContext->PSSetShader(mPixelShader, 0, 0);
}

void
Compositor::SetInputLayout()
{
	// Time to create the input buffer things
	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexData, Tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ID3D11InputLayout* inputLayout;
	const int inputCount = sizeof(inputDesc) / sizeof(VertexData);
	assert(inputCount == 2);

	HRESULT result = mDevice->CreateInputLayout(inputDesc, inputCount,
							g_TextureVertexShader, sizeof(g_TextureVertexShader),
							&inputLayout);
	assert(SUCCESS(result));
	mContext->IASetInputLayout(inputLayout);
	inputLayout->Release();
}

void
Compositor::SetTextureSampling(ID3D11Texture2D* aTexture)
{
	CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
	HRESULT result = mDevice->CreateSamplerState(&samplerDesc, &mSamplerState);
	assert(SUCCESS(result));

	result = mDevice->CreateShaderResourceView(aTexture, NULL, &mTextureView);
	assert(SUCCESS(result));

	mContext->PSSetShaderResources(0, 1, &mTextureView);
	mContext->PSSetSamplers(0, 1, &mSamplerState);

	mSamplerState->Release();
	mTextureView->Release();
}

void
Compositor::SetVertexBuffers(VertexData* aData)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	HRESULT hr = mContext->Map(mVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &resource);
	assert(hr == S_OK);
	memcpy(resource.pData, aData, sizeof(VertexData) * 4);	// 4 vertices
	mContext->Unmap(mVertexBuffer, NULL);
}

void
Compositor::InitVertexBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;	// Can be accessed by the CPU
	bufferDesc.ByteWidth = sizeof(VertexData) * 4; // 4 because we always draw a square
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// This uploads the data to the gpu
	HRESULT result = mDevice->CreateBuffer(&bufferDesc, NULL, &mVertexBuffer);
	assert(SUCCESS(result));

	UINT stride = sizeof(VertexData);
	UINT offset = 0;
	mContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
}

static int indices[] =
{
	0, 1, 2,	// bottom left, top left, top right
	3, 0, 2		// bottom right, bottom left, top right
};

void 
Compositor::SetIndexBuffers()
{
	D3D11_BUFFER_DESC indexBufferDesc;
	memset(&indexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	int indexCount = 6;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(int) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA indexResourceData;
	memset(&indexResourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
	indexResourceData.pSysMem = indices;

	HRESULT result = mDevice->CreateBuffer(&indexBufferDesc, &indexResourceData, &mIndexBuffer);
	assert(SUCCESS(result));

	mContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void
Compositor::InitViewport()
{
	assert(mWidth);
	assert(mHeight);

	// D3d goes from -1, 1 and that maps to device space via the viewport.
	D3D11_VIEWPORT viewport;
	memset(&viewport, 0, sizeof(D3D11_VIEWPORT));
	viewport.Width = mWidth;
	viewport.Height = mHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	mContext->RSSetViewports(1, &viewport);
}

void
Compositor::PrepareDrawing()
{
	InitBackBuffer();
	mContext->OMSetRenderTargets(1, &mBackBufferView, NULL);

	InitViewport();
	CompileTextureShaders();
	SetInputLayout();
	SetIndexBuffers();
	InitVertexBuffers();
	InitDrawQuery();
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

/// WARNING: ASSUMES YOU ALREADY LOCKED THE TEXTURE
void
Compositor::DrawViaTextureShaders(ID3D11Texture2D* aTexture, VertexData* aLocation)
{
	SetVertexBuffers(aLocation);
	SetTextureSampling(aTexture);

	// Finally draw the 6 vertices we have
	int indexCount = 6;
	mContext->DrawIndexed(indexCount, 0, 0);
}

void
Compositor::DrawViaTextureShaders(Texture* aTexture, VertexData* aLocation)
{
	aTexture->Lock();
	DrawViaTextureShaders(aTexture->GetTexture(), aLocation);
	aTexture->Unlock();
}

/* static */ Compositor*
Compositor::GetCompositor(HWND aOutputWindow) {
	if (!Compositor::mCompositor) {
		mCompositor = new Compositor(aOutputWindow);
	}
	return Compositor::mCompositor;
}

void
Compositor::InitBackBuffer()
{
	assert(mSwapChain);
	// Query information about the back buffer, don't actually use anything.
	HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&mBackBuffer);
	assert(SUCCESS(hr));

	hr = mDevice->CreateRenderTargetView(mBackBuffer, NULL, &mBackBufferView);
	assert(SUCCESS(hr));
}

void
Compositor::CopyToBackBuffer(Texture* aTexture)
{
	aTexture->Lock();
	mContext->CopyResource(mBackBuffer, aTexture->GetTexture());
	aTexture->Unlock();
}

void
Compositor::CopyToBackBuffer(ID3D11Texture2D* aTexture)
{
	mContext->CopyResource(mBackBuffer, aTexture);
}

void
Compositor::InitColors(FLOAT aColors[][4], int aCount)
{
	assert(aCount == 6);
	InitColor(aColors[0], 1, 0, 0, 1);
	InitColor(aColors[1], 0, 1, 0, 1);
	InitColor(aColors[2], 0, 0, 1, 1);
	InitColor(aColors[3], 1, 1, 0, 1);
	InitColor(aColors[4], 1, 0, 1, 1);
	InitColor(aColors[5], 0.5, 0.5, 0.5, 1);
}

void
Compositor::ReportLiveObjects()
{
	mDeviceManager->ReportLiveObjects();
}

void
Compositor::InitSyncTexture(HANDLE aSyncHandle)
{
	assert(aSyncHandle);
	HRESULT hr = mDevice->OpenSharedResource(aSyncHandle, __uuidof(ID3D11Texture2D), (void**)&mSyncTexture);
	assert(SUCCESS(hr));

	IDXGIKeyedMutex* mutex;
	mSyncTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mutex);
	hr = mutex->AcquireSync(0, 10000);
	assert(SUCCESS(hr));

	mutex->ReleaseSync(0);
	mutex->Release();
}

void
Compositor::LockSyncHandle(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle)
{
	assert(aSyncHandle);
	assert(mSyncTexture);
	IDXGIKeyedMutex* mutex;
	// Only lock our sync texture
	mSyncTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mutex);
	HRESULT hr = mutex->AcquireSync(0, INFINITE);
	assert(SUCCESS(hr));

	// Copy 1 pixel from every shared handle onto our sync texture
	/*
	for (std::vector<HANDLE>::iterator it = aHandles.begin(); it != aHandles.end(); it++) {
		HANDLE handle = *it;
		ID3D11Texture2D* sharedTexture = GetTexture(handle);

		D3D11_BOX box;
		box.front = box.top = box.left = 0;
		box.back = box.right = box.bottom = 1;
		mContext->CopySubresourceRegion(mSyncTexture, 0, 0, 0, 0, sharedTexture, 0, &box);
	}
	*/

	mutex->ReleaseSync(0);
	mutex->Release();
}

ID3D11Texture2D*
Compositor::GetTexture(HANDLE aHandle)
{
	auto texture = mTextures.find(aHandle);
	if (texture == mTextures.end()) {
		ID3D11Texture2D* sharedTexture;
		HRESULT hr = mDevice->OpenSharedResource(aHandle, __uuidof(ID3D11Texture2D), (void**)&sharedTexture);
		assert(SUCCESS(hr));

		std::pair<HANDLE, ID3D11Texture2D*> pair(aHandle, sharedTexture);
		mTextures.insert(pair);
		return sharedTexture;
	}

	return texture->second;
}

void
Compositor::InitDrawQuery()
{
	D3D11_QUERY_DESC queryDesc;
	memset(&queryDesc, 0, sizeof(D3D11_QUERY_DESC));
	queryDesc.Query = D3D11_QUERY_EVENT;

	HRESULT hr = mDevice->CreateQuery(&queryDesc, &mDrawQuery);
	assert(SUCCESS(hr));
}

void
Compositor::WaitForDrawExecution()
{
	for (;;) {
		HRESULT hr = mContext->GetData(mDrawQuery, NULL, 0, 0);
		if (SUCCESS(hr)) {
			return;
		}
	}
}

void
Compositor::CompositeWithSync(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle)
{
	mContext->Begin(mDrawQuery);
	LockSyncHandle(aHandles, aSyncHandle);

	int handleCount = aHandles.size();
	int position = 0;

	for (std::vector<HANDLE>::iterator it = aHandles.begin(); it != aHandles.end(); it++) {
		HANDLE handle = *it;
		ID3D11Texture2D* sharedTexture = GetTexture(handle);
		DrawViaTextureShaders(sharedTexture, POSITIONS[position++]);
	}

	mContext->Flush();
	mSwapChain->Present(0, 0);
	mContext->End(mDrawQuery);

	// The draw calls above might not finish by the time we call present, so if we tell
	// the child to draw now, the child could still draw to the texture before
	// the compositor executes it's draw calls. This could happen since they are on 
	// differnet ID3D11Devices and both are async, so we can't guarantee draw order either.
	// I think?
	WaitForDrawExecution();
}

void
Compositor::Composite(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle)
{
	int handleCount = aHandles.size();
	int position = 0;

	for (std::vector<HANDLE>::iterator it = aHandles.begin(); it != aHandles.end(); it++) {
		HANDLE handle = *it;
		ID3D11Texture2D* sharedTexture = GetTexture(handle);

		IDXGIKeyedMutex* mutex;
		sharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mutex);
		HRESULT hr = mutex->AcquireSync(0, 10000);
		assert(SUCCESS(hr));

		DrawViaTextureShaders(sharedTexture, POSITIONS[position++]);

		mutex->ReleaseSync(0);
		mutex->Release();
	}

	mContext->Flush();
	mSwapChain->Present(0, 0);
}
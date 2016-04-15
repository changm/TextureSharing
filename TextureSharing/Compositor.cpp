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

	mMutex = CreateMutex(NULL, false, L"CompositorMutex");
	PrepareDrawing();
}

Compositor::~Compositor()
{
	delete mDeviceManager;
}

void
Compositor::Clean()
{
	mDevice->Release();
	mContext->Release();
	mVertexBuffer->Release();
	mVertexShader->Release();
	mIndexBuffer->Release();
	mVertexShaderBytecode->Release();
	mPixelShaderBytecode->Release();
	mPixelShader->Release();
	mSwapChain->Release();
	mBackBuffer->Release();
	mBackBufferView->Release();
}

void
Compositor::ResizeBuffers()
{
	WaitForSingleObject(mMutex, INFINITE);
	mBackBuffer->Release();
	mBackBufferView->Release();
	mBackBuffer = nullptr;
	mBackBufferView = nullptr;

	mSwapChain->ResizeBuffers(0, mWidth, mHeight, DXGI_FORMAT_UNKNOWN, 0);
	InitBackBuffer();
	mContext->OMSetRenderTargets(1, &mBackBufferView, NULL);
	ReleaseMutex(mMutex);
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

	// Compile our shaders
	result = D3DCompileFromFile(L"TextureMappingVS.hlsl", NULL, NULL,
								"main", "vs_5_0", 0, 0,
								&mVertexShaderBytecode, NULL);
	assert(SUCCESS(result));

	result = D3DCompileFromFile(L"TextureMappingPS.hlsl", NULL, NULL,
								"main", "ps_5_0", 0, 0,
								&mPixelShaderBytecode, NULL);
	assert(SUCCESS(result));
	assert(mDevice);

	// Create the pixel shaders from the bytecode
	result = mDevice->CreatePixelShader(mPixelShaderBytecode->GetBufferPointer(), mPixelShaderBytecode->GetBufferSize(), NULL, &mPixelShader);
	assert(SUCCESS(result));

	mDevice->CreateVertexShader(mVertexShaderBytecode->GetBufferPointer(), mVertexShaderBytecode->GetBufferSize(), NULL, &mVertexShader);
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
							mVertexShaderBytecode->GetBufferPointer(),
							mVertexShaderBytecode->GetBufferSize(),
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
	WaitForSingleObject(mMutex, INFINITE);
	InitBackBuffer();
	mContext->OMSetRenderTargets(1, &mBackBufferView, NULL);

	InitViewport();
	CompileTextureShaders();
	SetInputLayout();
	SetIndexBuffers();
	InitVertexBuffers();
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ReleaseMutex(mMutex);
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

/*
 * Don't need to actually do anything with the texture, we hope this forces a flush.
 */
void
Compositor::LockSyncHandle(HANDLE aSyncHandle)
{
	assert(aSyncHandle);
	ID3D11Texture2D* sharedTexture;
	HRESULT hr = mDevice->OpenSharedResource(aSyncHandle, __uuidof(ID3D11Texture2D), (void**)&sharedTexture);
	assert(SUCCESS(hr));

	IDXGIKeyedMutex* mutex;
	sharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mutex);
	hr = mutex->AcquireSync(0, 10000);
	assert(SUCCESS(hr));

	sharedTexture->Release();
	mutex->ReleaseSync(0);
	mutex->Release();
}

void
Compositor::ReportLiveObjects()
{
	mDeviceManager->ReportLiveObjects();
}

void
Compositor::CompositeWithSync(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle)
{
	WaitForSingleObject(mMutex, INFINITE);
	LockSyncHandle(aSyncHandle);

	int handleCount = aHandles.size();
	int position = 0;

	for (std::vector<HANDLE>::iterator it = aHandles.begin(); it != aHandles.end(); it++) {
		HANDLE handle = *it;
		ID3D11Texture2D* sharedTexture;
		HRESULT hr = mDevice->OpenSharedResource(handle, __uuidof(ID3D11Texture2D), (void**)&sharedTexture);
		assert(SUCCESS(hr));

		DrawViaTextureShaders(sharedTexture, POSITIONS[position++]);
		sharedTexture->Release();
	}

	mContext->Flush();

	mSwapChain->Present(0, 0);
	ReleaseMutex(mMutex);
}

void
Compositor::Composite(std::vector<HANDLE>& aHandles, HANDLE aSyncHandle)
{
	WaitForSingleObject(mMutex, INFINITE);
	int handleCount = aHandles.size();
	int position = 0;

	for (std::vector<HANDLE>::iterator it = aHandles.begin(); it != aHandles.end(); it++) {
		HANDLE handle = *it;
		ID3D11Texture2D* sharedTexture;
		HRESULT hr = mDevice->OpenSharedResource(handle, __uuidof(ID3D11Texture2D), (void**)&sharedTexture);
		assert(SUCCESS(hr));

		IDXGIKeyedMutex* mutex;
		sharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&mutex);
		hr = mutex->AcquireSync(0, 10000);
		assert(SUCCESS(hr));

		DrawViaTextureShaders(sharedTexture, POSITIONS[position++]);

		sharedTexture->Release();
		mutex->ReleaseSync(0);
		mutex->Release();
	}

	mContext->Flush();

	mSwapChain->Present(0, 0);
	ReleaseMutex(mMutex);
}
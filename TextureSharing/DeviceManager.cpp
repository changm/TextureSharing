#include "stdafx.h"
#include "DeviceManager.h"
#include <stdio.h>
#include <dxgi.h>
#include <assert.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d3dcompiler.h>

#include "Vertex.h"

// Since the incoming numbers are FLOATS, the values are actually 0-1, not 0-255
static void InitColor(FLOAT* aFloatOut, FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
	aFloatOut[0] = r;
	aFloatOut[1] = g;
	aFloatOut[2] = b;
	aFloatOut[3] = a;
}

DeviceManager::DeviceManager(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	Init();
}

DeviceManager::~DeviceManager()
{
	mVertexShader->Release();
	mPixelShader->Release();
	mSwapChain->Release();
	mBackBuffer->Release();
	mDevice->Release();
	mContext->Release();
}

bool SUCCESS(HRESULT aResult) {
	return aResult == S_OK;
}

void DeviceManager::Init()
{
	InitD3D();
	InitD2D();
	InitBackBuffer();
	InitViewport();

	CompileShaders();
}

void DeviceManager::CompileShaders()
{
	HRESULT result;

	// Compile our shaders
	result = D3DCompileFromFile(L"VertexShader.hlsl", NULL, NULL,
								"VertexShaderMain", "vs_5_0", 0, 0,
								&mVertexShaderBytecode, NULL);
	assert(SUCCESS(result));

	result = D3DCompileFromFile(L"PixelShader.hlsl", NULL, NULL,
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

// A color here should be a RGBA float
void DeviceManager::ClearRect(FLOAT* aRGBAColor)
{
	mContext->ClearRenderTargetView(mBackBuffer, aRGBAColor);
}

void DeviceManager::InitViewport()
{
	// D3d goes from -1, 1 and that maps to device space via the viewport.
	D3D11_VIEWPORT viewport;
	memset(&viewport, 0, sizeof(D3D11_VIEWPORT));
	viewport.Height = 1024;
	viewport.Width = 1024;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	mContext->RSSetViewports(1, &viewport);
}

void DeviceManager::InitBackBuffer()
{
	assert(mSwapChain);
	ID3D11Texture2D* backBufferInfo;
	// Query information about the back buffer, don't actually use anything.
	HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferInfo);
	assert(SUCCESS(hr));

	hr = mDevice->CreateRenderTargetView(backBufferInfo, NULL, &mBackBuffer);
	assert(SUCCESS(hr));
	// sadly we only need the information to create the render target view
	backBufferInfo->Release();

	mContext->OMSetRenderTargets(1, &mBackBuffer, NULL);
}

void DeviceManager::InitD3D()
{
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&mFactory);
	assert(SUCCESS(hr));

	// Get the primary adapter, which is the GPU
	hr = mFactory->EnumAdapters1(0, &mAdapter);
	assert(SUCCESS(hr));

	// Get the device!
	hr = D3D11CreateDevice(mAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		NULL,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		NULL, 0, // d3d feature levels
		D3D11_SDK_VERSION,
		&mDevice,
		NULL, &mContext);
	assert(SUCCESS(hr));

	// Create the swap chain
	DXGI_SWAP_CHAIN_DESC swapDesc;
	memset(&swapDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferDesc.Width = 0;
	swapDesc.BufferDesc.Height = 0;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Only 1 buffer since the desktop screen is our front buffer?
	swapDesc.BufferCount = 1;
	swapDesc.OutputWindow = mOutputWindow;
	swapDesc.Windowed = TRUE;
	swapDesc.Flags = 0;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

	hr = mFactory->CreateSwapChain(mDevice, &swapDesc, &mSwapChain);
	assert(SUCCESS(hr));
}

void DeviceManager::InitD2D()
{
	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_MULTI_THREADED,
		&mD2DFactory);
	assert(SUCCESS(hr));
}

void DeviceManager::DrawTriangle()
{
	float white[4] = { 1, 1, 1, 1 };
	Vertex vertices[] =
	{
		{0, 0, 0, white},	// Origin
		{0, 1, 0, white},	// top
		{1, 0, 0, white},	// right
	};

	// Now we create a buffer for our triangle
	ID3D11Buffer* triangleBuffer;
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(Vertex) * 3;	// Because we have 3 vertices
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = mDevice->CreateBuffer(&bufferDesc, NULL, &triangleBuffer);
	assert(SUCCESS(result));

	// We now have a GPU buffer, and our vertices are on the CPU
	// Mapping allows us to communicate with the GPU
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	mContext->Map(triangleBuffer, NULL, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, vertices, sizeof(vertices));
	mContext->Unmap(triangleBuffer, NULL);

	// Time to create the input buffer things
	D3D11_INPUT_ELEMENT_DESC inputDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		// 12 since we have 3 floats bfeore the color
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	ID3D11InputLayout* inputLayout;
	result = mDevice->CreateInputLayout(inputDesc, 2,
		mVertexShaderBytecode->GetBufferPointer(),
		mVertexShaderBytecode->GetBufferSize(),
		&inputLayout);
	assert(SUCCESS(result));
	mContext->IASetInputLayout(inputLayout);

	// Finally draw the things, set the vertex buffers to the one that we uploaded to the gpu
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	mContext->IAGetVertexBuffers(0, 1, &triangleBuffer, &stride, &offset);
	
	// Tell the GPU we just have a list of triangles
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Finally draw the 3 vertices we have
	int vertexCount = 3;
	mContext->Draw(vertexCount, 0);

}

void DeviceManager::Draw()
{
	printf("DeviceManager::Draw");
	DrawTriangle();
	//FLOAT red[4];
	//InitColor(red, 0, 1, 0, 0);
	//ClearRect(red);

	mSwapChain->Present(0, 0);
}
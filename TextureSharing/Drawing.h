#pragma once
#include <direct.h>
#include <d3d11.h>
#include <DirectXMath.h>

// Basically draw target, but couldn't come up with a better name
class Texture;
using namespace DirectX;

// Manages our d3d devices for us
class Drawing {
public:
	Drawing(ID3D11Device* aDevice,
					ID3D11DeviceContext* aContext,
					LONG aWidth,
					LONG aHeight);

	// Draws to this texture
	void Draw(Texture* aTexture);

	// Returns the finished drawing! Only alive as long as this object is alive
	ID3D11Texture2D* Draw();
	HANDLE GetSharedTextureHandle();
	~Drawing();
	void Lock();
	void Unlock();

private:
	// Init all the things
	void SetRenderTarget(Texture* aTexture);
	void InitTexture();
	void InitViewport();
	void UpdateConstantBuffers();
	void InitMatrices();
	void UploadVertices();
	void SetInputLayout();
	int SetIndexBuffers();

	// draw a clear color.
	void ClearRect(FLOAT* aRGBAColor);

	// Let's use some shaders now
	void CompileShaders();
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D10Blob* mVertexShaderBytecode;
	ID3D10Blob* mPixelShaderBytecode;

	// Setup D3D
	ID3D11Device* mDevice;
	ID3D11DeviceContext* mContext;

	ID3D11RenderTargetView* mRenderTarget;
	Texture* mTexture;

	ID3D11Buffer* mConstantBuffers[ConstantBuffers::NUM_BUFFERS];

	XMMATRIX mProjectionMatrix;
	XMMATRIX mViewMatrix;
	XMMATRIX mWorldMatrix;

	LONG mWidth;
	LONG mHeight;

	// Our buffers
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;
};

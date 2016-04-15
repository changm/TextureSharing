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
					ID3D11DeviceContext* aContext);

	// Draws to this texture
	void Draw(Texture* aTexture, FLOAT* aColor);
	~Drawing();

private:
	void SetRenderTarget(Texture* aTexture);
	void SetViewport(Texture* aTexture);
	void UpdateConstantBuffers();
	void InitMatrices(Texture* aTexture);
	void UploadVertices(FLOAT* aColor);
	void SetInputLayout();
	int SetIndexBuffers();

	// Let's use some shaders now
	void CompileShaders();
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D10Blob* mVertexShaderBytecode;
	ID3D10Blob* mPixelShaderBytecode;

	// Setup D3D
	ID3D11Device* mDevice;
	ID3D11DeviceContext* mContext;

	ID3D11Buffer* mConstantBuffers[ConstantBuffers::NUM_BUFFERS];

	XMMATRIX mProjectionMatrix;
	XMMATRIX mViewMatrix;
	XMMATRIX mWorldMatrix;

	// Our buffers
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;
};

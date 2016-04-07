#pragma once
#include <d3d11.h>

// Represents a texture. Allocates it on the GPU, locks / unlocks it all.
class Texture
{
public:
	Texture(ID3D11Device* aDevice);
	~Texture();

	void Allocate();
	void Deallocate();
	ID3D11Texture2D* GetTexture() {
		return mTexture;
	}

private:
	ID3D11Device* mDevice;
	ID3D11Texture2D* mTexture;
	const int mWidth = 1024;
	const int mHeight = 1024;
};
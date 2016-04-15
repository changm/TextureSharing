#pragma once

#include "Texture.h"

/***
 * This is used to test our theory of a sync texture that is locked last on content and
 * locked first on the parent side.
 */
class SyncTexture : public Texture
{
public:
	static SyncTexture* AllocateSyncTexture(ID3D11Device* aDevice, ID3D11DeviceContext* aContext, LONG aWidth, LONG aHeight);

private:
	SyncTexture(LONG aWidth, LONG aHeight);
};

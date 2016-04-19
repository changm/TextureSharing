#pragma once

class DeviceManager;
class ChildPipe;
class SyncTexture;

class Child
{
public:
	Child();
	~Child();

	void Draw();
	void DrawWithSyncHandle();
	void MessageLoop();
	
	void InitSyncTexture();
	void SendMsg(MESSAGES aMessage, DWORD aData = 0);

private:
	void InitColors(FLOAT aColors[][4]);
	void InitTextures(bool aUseMutex);
	void Clean();
	void Swap();

	DeviceManager* mDeviceManager;
	ChildPipe* mPipe;
	Drawing* mDraw;

	LONG mWidth;
	LONG mHeight;
	
	static const int mTextureCount= 4;
	FLOAT white[4];
	FLOAT mColors[6][4];
	// draw 4 textures
	Texture* mFrontBuffers[mTextureCount];
	Texture* mBackBuffers[mTextureCount];

	Texture** mCurrentTextures;


	SyncTexture* mSyncTexture;
};

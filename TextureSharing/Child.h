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
	
	void SendSharedHandle(Texture* aTexture);
	void SendDrawFinished();
	void SendSyncTexture();

private:
	void InitColors(FLOAT aColors[][4]);
	void InitTextures(bool aUseMutex);
	void Clean();
	void SendCloseFinish();

	DeviceManager* mDeviceManager;
	ChildPipe* mPipe;
	Drawing* mDraw;

	LONG mWidth;
	LONG mHeight;
	
	static const int mTextureCount= 4;
	FLOAT white[4];
	FLOAT mColors[6][4];
	// draw 4 textures
	Texture* mTextures[mTextureCount];
	SyncTexture* mSyncTexture;
};

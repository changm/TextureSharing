#pragma once

class DeviceManager;
class ChildPipe;

class Child
{
public:
	Child();
	~Child();

	void Draw();
	void MessageLoop();
	
	void SendSharedHandle(Texture* aTexture);
	void SendDrawFinished();

private:
	void InitColors(FLOAT aColors[][4]);
	void InitTextures();

	DeviceManager* mDeviceManager;
	ChildPipe* mPipe;
	Drawing* mDraw;

	LONG mWidth;
	LONG mHeight;
	
	static const int mTextureCount= 4;
	FLOAT mColors[6][4];
	// draw 4 textures
	Texture* mTextures[mTextureCount];
};

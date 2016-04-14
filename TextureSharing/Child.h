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
	void InitColors(FLOAT aColors[][4], int aCount);
	void SendSharedHandle(Texture* aTexture);
	void SendDrawFinished();

private:
	DeviceManager* mDeviceManager;
	ChildPipe* mPipe;
	Drawing* mDraw;

	LONG mWidth;
	LONG mHeight;
};

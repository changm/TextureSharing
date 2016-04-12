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

private:
	DeviceManager* mDeviceManager;
	ChildPipe* mPipe;
	Drawing* mDraw;

	LONG mWidth;
	LONG mHeight;
};

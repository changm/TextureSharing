#pragma once

class DeviceManager;

class Compositor {
public:
	Compositor(HWND aOutputWindow);
	~Compositor();

	void Composite();
	static Compositor* GetCompositor(HWND aOutputWindow);

private:
	void ReadTextures();

	DeviceManager* mDeviceManager;
	HWND mOutputWindow;
	static Compositor* mCompositor;
};
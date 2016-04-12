#pragma once

class Compositor;
class DeviceManager;

class Parent
{
public:
	Parent(HINSTANCE aInstance, int nCmdShow);
	~Parent();

	void GenerateWindow();

private:
	void CreateContentProcess();

	// Windows desktop code
	HACCEL							LoadNativeWindow();
	ATOM                RegisterWindow(HINSTANCE hInstance);
	BOOL                InitInstance(HINSTANCE, int);

	int mCmdShow;
	HWND mOutputWindow;

	PROCESS_INFORMATION mChildProcess;
	Compositor* mCompositor;
};
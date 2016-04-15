#pragma once

class Compositor;
class DeviceManager;
class ServerPipe;
#include <vector>

class Parent
{
public:
	Parent(HINSTANCE aInstance, int nCmdShow);
	~Parent();

	void GenerateWindow();
	void ParentMessageLoop();
	void SendDraw();
	void InitChildDraw();
	static BOOL IsCompositorThread();
	static DWORD sCompositorThread;

	bool mInitChild;

private:
	void CreateContentProcess();
	void CreateMessageLoopThread();

	// Windows desktop code
	HACCEL LoadNativeWindow();
	ATOM RegisterWindow(HINSTANCE hInstance);
	BOOL InitInstance(HINSTANCE, int);
	HWND mOutputWindow;
	int mCmdShow;

	PROCESS_INFORMATION mChildProcess;
	Compositor* mCompositor;

	ServerPipe* mPipe;
	MessageData mChildMessages;
	HANDLE mMessageLoop;

	std::vector<HANDLE> mSharedHandles;
	HANDLE mCompositorHandle;
};
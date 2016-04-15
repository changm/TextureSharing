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
	void SendDrawOnly();
	static BOOL IsCompositorThread();
	static DWORD sCompositorThread;
	void CloseChild();

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
	HANDLE mSyncHandle;	// Our sync handle that is locked last on content, first on parent
};
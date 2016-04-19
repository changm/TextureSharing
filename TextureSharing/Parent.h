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

	void SendMsg(MESSAGES aMessage, DWORD aData = 0);

	void GenerateWindow();
	void ParentMessageLoop();
	void SendDraw();
	void InitChildDraw();
	static BOOL IsCompositorThread();
	static DWORD sCompositorThread;
	void CloseChild();

	bool mInitChild;
	HWND mOutputWindow;

private:
	void CreateContentProcess();
	void CreateMessageLoopThread();
	void Swap();

	// Windows desktop code
	HACCEL LoadNativeWindow();
	ATOM RegisterWindow(HINSTANCE hInstance);
	BOOL InitInstance(HINSTANCE, int);
	int mCmdShow;

	PROCESS_INFORMATION mChildProcess;
	Compositor* mCompositor;

	ServerPipe* mPipe;
	MessageData mChildMessages;
	HANDLE mMessageLoop;

	std::vector<HANDLE> mSharedFrontHandles;
	std::vector<HANDLE> mSharedBackHandles;
	std::vector<HANDLE>* mCurrentBuffer;

	HANDLE mSyncHandle;	// Our sync handle that is locked last on content, first on parent
};
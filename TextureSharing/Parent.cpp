#include "stdafx.h"
#include "Parent.h"
#include "Compositor.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <dxgi.h>
#include <assert.h>
#include "windows.h"
#include <string>
#include <shellapi.h>
#include <atlbase.h>
#include <atlconv.h>
#include "Resource.h"

#include "DeviceManager.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Listen for the about
INT_PTR CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		Compositor::GetCompositor(hWnd)->CompositeSolo();
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	} // end switch
	return 0;
}

HACCEL
Parent::LoadNativeWindow() {
	// Initialize global strings
	LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInst, IDC_TEXTURESHARING, szWindowClass, MAX_LOADSTRING);
	RegisterWindow(hInst);

	// Perform application initialization:
	if (!InitInstance(hInst, mCmdShow)) {
		printf("Could not create window, exiting");
		exit(1);
	}

	return LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_TEXTURESHARING));
}

void
Parent::CreateContentProcess()
{
	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	memset(&mChildProcess, 0, sizeof(mChildProcess));

	std::string commandLine = GetCommandLineA();
	commandLine += "child";
	BOOL success = CreateProcess(NULL, CA2W(commandLine.c_str()),
																NULL, NULL, FALSE,
																NULL, NULL, NULL, &startupInfo, &mChildProcess);

	if (success) {
		WaitForInputIdle(mChildProcess.hProcess, 100);
	} else {
		DWORD error = GetLastError();
		printf("Could not craete content process, %u\n", error);
	}
}

ATOM
Parent::RegisterWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEXTURESHARING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TEXTURESHARING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL
Parent::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   mOutputWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!mOutputWindow) {
      return FALSE;
   }

   ShowWindow(mOutputWindow, nCmdShow);
   return TRUE;
}

Parent::Parent(HINSTANCE aInstance, int aCmdShow)
	: mCmdShow(aCmdShow)
{
	mPipe = new ServerPipe();
	mPipe->CreateServerPipe();
	CreateContentProcess();
	mPipe->WaitForChild();
}

Parent::~Parent()
{
		// Message loop is closed after generate window
	//assert(CloseHandle(mMessageLoop));
	mPipe->ClosePipe();
	CloseHandle(mChildProcess.hProcess);
	CloseHandle(mChildProcess.hThread);
	delete mPipe;
}

void Parent::StartChildDrawing()
{
	LONG width = Compositor::GetCompositor(mOutputWindow)->GetWidth();
	LONG height = Compositor::GetCompositor(mOutputWindow)->GetHeight();

	MessageData draw = { MESSAGES::CHILD_DRAW, 0 };
	MessageData msgWidth = { MESSAGES::WIDTH, (int) width };
	MessageData msgHeight = { MESSAGES::HEIGHT, (int) height };
	MessageData initDraw = { MESSAGES::INIT_DRAW, 0 };

	mPipe->SendMsg(&msgWidth);
	mPipe->SendMsg(&msgHeight);
	mPipe->SendMsg(&initDraw);
	mPipe->SendMsg(&draw);
}

void Parent::ParentMessageLoop()
{
	// Happens on another thread
	while (mPipe->ReadMsg(&mChildMessages)) {
		switch (mChildMessages.type) {
		case MESSAGES::HANDLE_MESSAGE:
		{
			HANDLE sharedTextureHandle = (HANDLE) mChildMessages.data;
			Compositor::GetCompositor(mOutputWindow)->Composite(sharedTextureHandle);
			break;
		}
		default:
			break;
		} // end switch
	}
}

static DWORD PaintLoop(void* aParentInstance)
{
	Parent* parent = (Parent*)aParentInstance;
	parent->StartChildDrawing();
	parent->ParentMessageLoop();
	return 0;
}

void Parent::CreateMessageLoopThread()
{
	mMessageLoop = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&PaintLoop, this, 0, NULL);
}

void Parent::GenerateWindow()
{
	// These are for like "File" menu and such i guess / windows keyboard shortcuts
	HACCEL keyBindings = LoadNativeWindow();
	assert(mOutputWindow);
	UpdateWindow(mOutputWindow);

	// Don't create he message loop thread until we create the window
	CreateMessageLoopThread();

	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, keyBindings, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
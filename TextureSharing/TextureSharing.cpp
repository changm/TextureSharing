// TextureSharing.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TextureSharing.h"
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

#include "DeviceManager.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HWND globalOutputWindow;
DeviceManager* globalDeviceManager;

// So we can have printf
static void InitConsole()
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE* pFile;
	freopen_s(&pFile, "CON", "w", stdout);
}

HACCEL LoadNativeWindow(HINSTANCE hInstance, int nCmdShow) {
	// Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TEXTURESHARING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
			printf("Could not create window, exiting");
			exit(1);
    }

    return LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEXTURESHARING));
}

void CreateContentProcess()
{
	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION newProcInfo;
	memset(&newProcInfo, 0, sizeof(newProcInfo));

	std::string commandLine = GetCommandLineA();
	commandLine += "child";
	BOOL success = CreateProcess(NULL, CA2W(commandLine.c_str()),
																NULL, NULL, FALSE,
																NULL, NULL, NULL, &startupInfo, &newProcInfo);

	if (success) {
		WaitForInputIdle(newProcInfo.hProcess, 100);
		CloseHandle(newProcInfo.hProcess);
		CloseHandle(newProcInfo.hThread);
	}
	else {
		DWORD error = GetLastError();
		printf("Could not craete content process, %u\n", error);
	}
}

static int ParentMain(HINSTANCE hInstance, int nCmdShow) {
	CreateContentProcess();

	// These are for like "File" menu and such i guess / windows keyboard shortcuts
	HACCEL keyBindings = LoadNativeWindow(hInstance, nCmdShow);

	assert(globalOutputWindow);
	DeviceManager deviceManager(globalOutputWindow);
	globalDeviceManager = &deviceManager;

	UpdateWindow(globalOutputWindow);

	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
	if (!TranslateAccelerator(msg.hwnd, keyBindings, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

static void ChildMain()
{
	exit(1);
}

static bool IsParent(LPWSTR aCommandLine) {
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(aCommandLine, &argc);
	return wcscmp(L"child", argv[0]);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	InitConsole();
	if (IsParent(lpCmdLine)) {
		ParentMain(hInstance, nCmdShow);
	} else {
		ChildMain();
	}
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
ATOM MyRegisterClass(HINSTANCE hInstance) {
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

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   globalOutputWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!globalOutputWindow) {
      return FALSE;
   }

   ShowWindow(globalOutputWindow, nCmdShow);
   return TRUE;
}

void PaintOurContent(HDC aHDC, PAINTSTRUCT& aPS) {
	globalDeviceManager->Draw();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
						PaintOurContent(hdc, ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

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
#include "Parent.h"
#include "Child.h"

#include "DeviceManager.h"

// So we can have printf
static void InitConsole()
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE* pFile;
	freopen_s(&pFile, "CON", "w", stdout);
}

static void AttachToParentConsole()
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	FILE* pFile;
	freopen_s(&pFile, "CON", "w", stdout);
}

static void ChildMain()
{
	printf("Initializing child\n");
	Child child;
	child.MessageLoop();
	//child.Draw();

	/*
	MSG msg;
	while (::GetMessageA(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	*/
	printf("Finished child\n");
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


	if (IsParent(lpCmdLine)) {
		InitConsole();
		Parent parent(hInstance, nCmdShow);
		parent.GenerateWindow();
	} else {
		AttachToParentConsole();
		ChildMain();
	}
}


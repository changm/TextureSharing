#include "stdafx.h"

#include "Compositor.h"
#include "DeviceManager.h"

Compositor* Compositor::mCompositor = nullptr;
Compositor::Compositor(HWND aOutputWindow)
	: mOutputWindow(aOutputWindow)
{
	mDeviceManager = new DeviceManager(aOutputWindow);
}

Compositor::~Compositor()
{
	delete mDeviceManager;
}

void
Compositor::ReadTextures()
{

}

/* static */ Compositor*
Compositor::GetCompositor(HWND aOutputWindow) {
	if (!Compositor::mCompositor) {
		mCompositor = new Compositor(aOutputWindow);
	}
	return Compositor::mCompositor;
}

void
Compositor::Composite()
{
	mDeviceManager->Draw();
}
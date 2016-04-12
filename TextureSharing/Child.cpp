#include "stdafx.h"
#include "Child.h"
#include "DeviceManager.h"
#include "Drawing.h"

Child::Child()
{
	mPipe = new ChildPipe();
	mPipe->ConnectToServerPipe();
}

Child::~Child()
{
	delete mPipe;
}

void Child::MessageLoop()
{
	MessageData msg;
	while (mPipe->ReadMsg(&msg))
	{
		switch(msg.type)
		{
		case MESSAGES::CLOSE:
			return;
		default:
			break;
		}
	}
}

void Child::Draw()
{
	DeviceManager deviceManager;
	LONG width = 500;
	LONG height = 500;
	Drawing testDraw(deviceManager.GetDevice(), deviceManager.GetDeviceContext(), width,  height);
	ID3D11Texture2D* drawnTexture = testDraw.Draw();
}
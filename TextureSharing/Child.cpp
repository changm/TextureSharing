#include "stdafx.h"
#include "Child.h"
#include "DeviceManager.h"
#include "Drawing.h"
#include <stdio.h>

Child::Child()
{
	mPipe = new ChildPipe();
	mPipe->ConnectToServerPipe();

	mDeviceManager = new DeviceManager();;

}

Child::~Child()
{
	delete mDraw;
	delete mDeviceManager;
	delete mPipe;
}

void Child::MessageLoop()
{
	MessageData msg;
	while (mPipe->ReadMsg(&msg))
	{
		switch(msg.type)
		{
		case MESSAGES::WIDTH:
		{
			mWidth = (LONG) msg.data;
			break;
		}
		case MESSAGES::HEIGHT:
		{
			mHeight = (LONG) msg.data;
			break;
		}
		case MESSAGES::INIT_DRAW:
		{
			mDraw = new Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);
			break;
		}
		case MESSAGES::CLOSE:
			return;
		case MESSAGES::CHILD_DRAW:
			Draw();
			break;
		default:
			break;
		}
	}
}

void Child::Draw()
{
	ID3D11Texture2D* drawnTexture = mDraw->Draw();
	HANDLE sharedTexture = mDraw->GetSharedTextureHandle();

	MessageData sharedHandle = {
		MESSAGES::HANDLE_MESSAGE,
		(DWORD)sharedTexture,
	};

	printf("Child sending shared handle\n");
	mPipe->SendMsg(&sharedHandle);
}
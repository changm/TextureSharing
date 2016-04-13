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
			mDraw = new Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext());
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
	Texture* targetTexture = Texture::AllocateTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);

	FLOAT red[4];
	red[0] = 1;
	red[1] = 0;
	red[2] = 0;
	red[3] = 0;

	mDraw->Draw(targetTexture, red);
	HANDLE sharedTexture = targetTexture->GetSharedHandle();

	MessageData sharedHandle = {
		MESSAGES::HANDLE_MESSAGE,
		(DWORD)sharedTexture,
	};

	printf("Child sending shared handle\n");
	mPipe->SendMsg(&sharedHandle);
}
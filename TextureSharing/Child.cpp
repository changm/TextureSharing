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

void
Child::InitColors(FLOAT aColors[][4], int aCount)
{
	assert(aCount == 6);
	InitColor(aColors[0], 1, 0, 0, 1);
	InitColor(aColors[1], 0, 1, 0, 1);
	InitColor(aColors[2], 0, 0, 1, 1);
	InitColor(aColors[3], 1, 1, 0, 1);
	InitColor(aColors[4], 1, 0, 1, 1);
	InitColor(aColors[5], 0.5, 0.5, 0.5, 1);
}

void
Child::SendSharedHandle(Texture* aTexture)
{
	HANDLE sharedTexture = aTexture->GetSharedHandle();

	MessageData sharedHandle = {
		MESSAGES::HANDLE_MESSAGE,
		(DWORD)sharedTexture,
	};

	mPipe->SendMsg(&sharedHandle);
}

void
Child::SendDrawFinished()
{
	MessageData finishMessage = {
		MESSAGES::CHILD_FINISHED,
		0,
	};

	mPipe->SendMsg(&finishMessage);
}

void
Child::Draw()
{
	const int size = 6;
	FLOAT colors[size][4];
	Texture* textures[size];
	InitColors(colors, size);

	for (int i = 0; i < size; i++) {
		textures[i] = Texture::AllocateTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);
		SendSharedHandle(textures[i]);
	}

	for (int i = 0; i < size; i++) {
		mDraw->Draw(textures[i], colors[i]);
	}

	for (int i = 0; i < size; i++) {
		delete textures[i];
	}

	SendDrawFinished();
}
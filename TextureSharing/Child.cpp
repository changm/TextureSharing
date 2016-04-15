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
	for (int i = 0; i < mTextureCount; i++) {
		delete mTextures[i];
	}

	delete mDraw;
	delete mDeviceManager;
	delete mPipe;
}

void Child::MessageLoop()
{
	MessageData msg;
	printf("Child message loop start\n");
	while (mPipe->ReadMsg(&msg))
	{
		switch(msg.type)
		{
		case MESSAGES::WIDTH:
		{
			printf("[Child] width\n");
			mWidth = (LONG) msg.data;
			break;
		}
		case MESSAGES::HEIGHT:
		{
			printf("[Child] height\n");
			mHeight = (LONG) msg.data;
			break;
		}
		case MESSAGES::INIT_DRAW:
		{
			printf("[Child] init draw\n");
			assert(mWidth);
			assert(mHeight);
			mDraw = new Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext());
			InitColors(mColors);
			InitTextures();
			break;
		}
		case MESSAGES::CLOSE:
		{
			printf("[Child] Child closing channel\n");
			return;
		}
		case MESSAGES::CHILD_DRAW:
		{
			assert(mDraw, "asked to draw before we initialized drawing");
			printf("[Child] DRAW\n");
			Draw();
			break;
		}
		default:
			break;
		}

		printf("[Child] waiting on messages\n");
		DWORD waitReturn = WaitForSingleObjectEx(mPipe->GetPipe(), INFINITE, TRUE);
		printf("[Child] finished waiting %d\n", waitReturn);
	}
}

void
Child::InitColors(FLOAT aColors[][4])
{
	InitColor(aColors[0], 1, 0, 0, 1);
	InitColor(aColors[1], 0, 1, 0, 1);
	InitColor(aColors[2], 0, 0, 1, 1);
	InitColor(aColors[3], 1, 1, 0, 1);
	InitColor(aColors[4], 1, 0, 1, 1);
	InitColor(aColors[5], 0.5, 0.5, 0.5, 1);
}

void
Child::InitTextures()
{
	// Only draw 4 textures for now
	for (int i = 0; i < mTextureCount; i++) {
		mTextures[i] = Texture::AllocateTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);
		SendSharedHandle(mTextures[i]);
	}

	for (int i = 0; i < mTextureCount; i++) {
		mDraw->Draw(mTextures[i], mColors[i]);
	}


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
	printf("[Child] Child Drawing\n");
	FLOAT white[4];
	InitColor(white, 1, 1, 1, 1);

	for (int i = 0; i < mTextureCount; i++) {
		mDraw->Draw(mTextures[i], white);
		mDraw->Draw(mTextures[i], mColors[i]);
	}

	SendDrawFinished();
}
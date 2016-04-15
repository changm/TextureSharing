#include "stdafx.h"
#include "Child.h"
#include "DeviceManager.h"
#include "Drawing.h"
#include <stdio.h>
#include "SyncTexture.h"

Child::Child()
{
	mPipe = new ChildPipe();
	mPipe->ConnectToServerPipe();

	mDeviceManager = new DeviceManager();;
}

Child::~Child()
{
	delete mPipe;
}

void Child::Clean()
{
	for (int i = 0; i < mTextureCount; i++) {
		delete mTextures[i];
	}

	delete mSyncTexture;
	delete mDraw;
	delete mDeviceManager;
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
			//printf("[Child] width\n");
			mWidth = (LONG) msg.data;
			break;
		}
		case MESSAGES::HEIGHT:
		{
			//printf("[Child] height\n");
			mHeight = (LONG) msg.data;
			break;
		}
		case MESSAGES::INIT_CHILD_DRAW:
		{
			//printf("[Child] init draw\n");
			assert(mWidth);
			assert(mHeight);
			mDraw = new Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext());
			InitColors(mColors);
			bool useMutex = true;
			InitTextures(useMutex);
			InitColor(white, 1, 1, 1, 1);
			break;
		}
		case MESSAGES::INIT_CHILD_DRAW_SYNC_HANDLE:
		{
			printf("[Child] init sync draw\n");
			assert(mWidth);
			assert(mHeight);
			mDraw = new Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext());
			InitColors(mColors);
			bool useMutex = false;
			InitTextures(useMutex);
			InitColor(white, 1, 1, 1, 1);
			break;
		}
		case MESSAGES::CHILD_CLOSE_START:
		{
			printf("[Child] Child closing channel %d\n", GetCurrentProcessId());
			Clean();
			SendCloseFinish();
			return;
		}
		case MESSAGES::CHILD_DRAW:
		{
			assert(mDraw);
			//printf("[Child] DRAW %d\n", GetCurrentProcessId());
			Draw();
			break;
		}
		case MESSAGES::CHILD_DRAW_WITH_SYNC_HANDLE:
		{
			assert(mDraw);
			printf("[Child] DRAW with sync %d\n", GetCurrentProcessId());
			DrawWithSyncHandle();
			break;
		}
		default:
			break;
		}
	}
}

void
Child::SendCloseFinish()
{
	MessageData finishMessage = {
		MESSAGES::CHILD_CLOSE_FINISH,
		0,
	};

	mPipe->SendMsg(&finishMessage);
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
Child::InitTextures(bool aUseMutex)
{
	// Only draw 4 textures for now
	for (int i = 0; i < mTextureCount; i++) {
		mTextures[i] = Texture::AllocateTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight, aUseMutex);
		SendSharedHandle(mTextures[i]);
	}

	SendSyncTexture();

	for (int i = 0; i < mTextureCount; i++) {
		mDraw->Draw(mTextures[i], mColors[i]);
	}
}

void
Child::SendSyncTexture()
{
	assert(mWidth);
	assert(mHeight);

	mSyncTexture = SyncTexture::AllocateSyncTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);
	HANDLE syncTexture = mSyncTexture->GetSharedHandle();

	MessageData sharedHandle = {
		MESSAGES::SYNC_TEXTURE_HANDLE,
		(DWORD)syncTexture,
	};

	printf("Child sending sync handle\n");
	mPipe->SendMsg(&sharedHandle);
}

void
Child::SendSharedHandle(Texture* aTexture)
{
	HANDLE sharedTexture = aTexture->GetSharedHandle();

	MessageData sharedHandle = {
		MESSAGES::SHARED_HANDLE,
		(DWORD)sharedTexture,
	};

	mPipe->SendMsg(&sharedHandle);
}

void
Child::SendMsg(MESSAGES aMessage)
{
	MessageData msg = { aMessage, 0 };
	mPipe->SendMsg(&msg);
}

void
Child::SendDrawFinished()
{
	MessageData finishMessage = {
		MESSAGES::CHILD_FINISH_DRAW,
		0,
	};

	mPipe->SendMsg(&finishMessage);
}

void
Child::DrawWithSyncHandle()
{
	for (int i = 0; i < mTextureCount; i++) {
		mDraw->DrawNoLock(mTextures[i], white);
		mDraw->DrawNoLock(mTextures[i], mColors[i]);
	}

	mSyncTexture->Lock();
	mSyncTexture->Unlock();

	SendMsg(MESSAGES::CHILD_FINISH_DRAW_SYNC_HANDLE);
}

void
Child::Draw()
{
	/*
	for (int i = 0; i < mTextureCount; i++) {
		mTextures[i]->Lock();
	}
	*/

	for (int i = 0; i < mTextureCount; i++) {
		mDraw->Draw(mTextures[i], white);
		mDraw->Draw(mTextures[i], mColors[i]);
	}

	/*
	for (int i = 0; i < mTextureCount; i++) {
		mTextures[i]->Unlock();
	}
	*/

	SendDrawFinished();
}
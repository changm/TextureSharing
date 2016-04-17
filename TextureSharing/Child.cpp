#include "stdafx.h"
#include "Child.h"
#include "DeviceManager.h"
#include "Drawing.h"
#include <stdio.h>
#include "SyncTexture.h"
#include <new.h>

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
	delete mDeviceManager;

	mDraw->~Drawing();
	_aligned_free(mDraw);
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
		case MESSAGES::INIT_CHILD_DRAW_SYNC_HANDLE:
		case MESSAGES::INIT_CHILD_DRAW:
		{
			assert(mWidth);
			assert(mHeight);
			// Have to use aligned malloc since XMMATRIX uses SSE and needs to be 16 byte aligned
			void* aligned = _aligned_malloc(sizeof(Drawing), 16);
			mDraw = new (aligned) Drawing(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext());

			InitColors(mColors);
			bool useMutex = msg.type == MESSAGES::INIT_CHILD_DRAW;
			InitTextures(useMutex);
			InitColor(white, 1, 1, 1, 1);
			break;
		}
		case MESSAGES::CHILD_CLOSE_START:
		{
			Clean();
			SendMsg(MESSAGES::CHILD_CLOSE_FINISH);
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
			DrawWithSyncHandle();
			break;
		}
		default:
			break;
		}
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
Child::InitTextures(bool aUseMutex)
{
	// Only draw 4 textures for now
	for (int i = 0; i < mTextureCount; i++) {
		mTextures[i] = Texture::AllocateTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight, aUseMutex);
		SendMsg(MESSAGES::SHARED_HANDLE, (DWORD) mTextures[i]->GetSharedHandle());
	}

	InitSyncTexture();

	for (int i = 0; i < mTextureCount; i++) {
		mDraw->Draw(mTextures[i], mColors[i]);
	}
}

void
Child::InitSyncTexture()
{
	assert(mWidth);
	assert(mHeight);

	mSyncTexture = SyncTexture::AllocateSyncTexture(mDeviceManager->GetDevice(), mDeviceManager->GetDeviceContext(), mWidth, mHeight);
	HANDLE syncTexture = mSyncTexture->GetSharedHandle();

	SendMsg(MESSAGES::SYNC_TEXTURE_HANDLE, (DWORD)mSyncTexture->GetSharedHandle());
}

void
Child::SendMsg(MESSAGES aMessage, DWORD aData)
{
	MessageData msg = { aMessage, aData };
	mPipe->SendMsg(&msg);
}

void
Child::DrawWithSyncHandle()
{
	for (int i = 0; i < mTextureCount; i++) {
		mDraw->DrawNoLock(mTextures[i], white);
		mDraw->DrawNoLock(mTextures[i], mColors[i]);
	}

	// Draw into our sync texture although it's not really visible
	mSyncTexture->Lock();
	mDraw->DrawNoLock(mSyncTexture, white);
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

	SendMsg(MESSAGES::CHILD_FINISH_DRAW);
}
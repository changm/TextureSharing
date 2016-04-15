#include "stdafx.h"
#include <stdio.h>

// pipe names have to be in this format
static const char* PIPE_NAME =  "\\\\.\\pipe\\texturepipe";

Pipe::Pipe()
{
	memset(&mOverlap, 0, sizeof(OVERLAPPED));
}

Pipe::~Pipe()
{
	BOOL closed = CloseHandle(mPipe);
	assert(closed);
}

/***
 * We need an overlap object to simultaneously read/write
 * to the same pipe. e.g. if the child loops in read()
 * and the parent writes(), both deadlock.
 */
BOOL Pipe::ReadMsg(MessageData* aOutMessage)
{
	const int bufferSize = sizeof(MessageData);
	BOOL ok = ReadFile(mPipe, aOutMessage, bufferSize, NULL, &mOverlap);
	if (ok) {
		return TRUE;
	}

	DWORD transferResults;
	BOOL overlappedResults = GetOverlappedResult(mPipe, &mOverlap, &transferResults, TRUE);
	if (!overlappedResults) {
		printf("Did not get overlap results: %d\n", GetLastError());
	}

	return TRUE;
}

BOOL Pipe::SendMsg(MessageData* aSendMessage)
{
	const int bufferSize = sizeof(MessageData);
	BOOL ok = WriteFile(mPipe, aSendMessage, bufferSize, NULL, &mOverlap);
	if (ok) {
		return TRUE;
	}

	DWORD transferResults;
	BOOL overlappedResults = GetOverlappedResult(mPipe, &mOverlap, &transferResults, TRUE);
	if (!overlappedResults) {
		printf("Did not get overlap results: %d\n", GetLastError());
	}

	return TRUE;
}

BOOL
Pipe::ReadMsgSync(MessageData* aOutData)
{
	const int bufferSize = sizeof(MessageData);
	BOOL ok = ReadFile(mPipe, aOutData, bufferSize, NULL, NULL);
	assert(ok || (GetLastError() == ERROR_BROKEN_PIPE));	// Means was already closed
	return TRUE;
}

BOOL
Pipe::SendMsgSync(MessageData* aSendData)
{
	const int bufferSize = sizeof(MessageData);
	BOOL ok = WriteFile(mPipe, aSendData, bufferSize, NULL, NULL);
	assert(ok);
	return TRUE;
}

void ServerPipe::CreateServerPipe()
{
	DWORD openMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED;	// Can read/write in both directions
	DWORD pipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE;	// Use messages
	int numInstances = 1;
	int bufferSize = 512;	// For no reason
	int timeoutTimeMS = 0;	// Use default

	mPipe = CreateNamedPipeA(PIPE_NAME, openMode, pipeMode, numInstances,
													 bufferSize, bufferSize, timeoutTimeMS, NULL);
	if (mPipe == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to create pipe: %d\n", GetLastError());
	}

	assert(mPipe != INVALID_HANDLE_VALUE);
}

void ServerPipe::WaitForChild()
{
	MessageData childData;
	ReadMsg(&childData);
}

void ServerPipe::ClosePipe()
{
	CancelIoEx(mPipe, NULL);
	printf("CLOSING PIPE %d\n", GetCurrentProcessId());
}

void ChildPipe::ConnectToServerPipe()
{
	DWORD access = GENERIC_READ | GENERIC_WRITE;
	DWORD shareMode = 0;	// Only we can connect to this pipe
	DWORD creationFlags = OPEN_EXISTING;
	DWORD flags = FILE_ATTRIBUTE_NORMAL;

	mPipe = CreateFileA(PIPE_NAME, access, shareMode, NULL, creationFlags, flags, NULL);
	if (mPipe == INVALID_HANDLE_VALUE) {
		wprintf(L"Failed to create child pipe: %d\n", GetLastError());
	}

	assert(mPipe != INVALID_HANDLE_VALUE);

	DWORD mode = PIPE_READMODE_MESSAGE;
	BOOL ok = SetNamedPipeHandleState(mPipe, &mode, NULL, NULL);
	assert(ok);

	MessageData sendAck{
		MESSAGES::CHILD_OPEN,
		0
	};
	SendMsg(&sendAck);
}
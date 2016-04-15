#pragma once

enum MESSAGES
{
	CHILD_OPEN,
	SHARED_HANDLE,
	CHILD_DRAW,
	CHILD_DRAW_WITH_SYNC_HANDLE,
	WIDTH,
	HEIGHT,
	INIT_CHILD_DRAW,
	INIT_CHILD_DRAW_SYNC_HANDLE,
	CHILD_FINISH_DRAW,
	CHILD_FINISH_DRAW_SYNC_HANDLE,
	CHILD_CLOSE_START,
	CHILD_CLOSE_FINISH,
	SYNC_TEXTURE_HANDLE,
};

struct MessageData
{
	MESSAGES type;
	DWORD data;
};

// Our connection to the child process
class Pipe
{
public:
	Pipe();
	~Pipe();

	BOOL ReadMsg(MessageData* aOutData);
	BOOL SendMsg(MessageData* aSendData);
	HANDLE GetPipe() { return mPipe; }

	BOOL ReadMsgSync(MessageData* aOutData);
	BOOL SendMsgSync(MessageData* aSendData);

protected:
	HANDLE mPipe;
	HANDLE mWaitEvent;
	OVERLAPPED mOverlap;
};

class ServerPipe : public Pipe
{
public:
	void CreateServerPipe();
	void WaitForChild();
	void ClosePipe();
};

class ChildPipe : public Pipe
{
public:
	void ConnectToServerPipe();
};

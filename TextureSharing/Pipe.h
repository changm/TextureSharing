#pragma once

enum MESSAGES
{
	CHILD_OPEN,
	HANDLE_MESSAGE,
	CLOSE,
	CHILD_DRAW,
	WIDTH,
	HEIGHT,
	INIT_DRAW,
	CHILD_FINISHED,
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

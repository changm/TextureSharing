#pragma once

class DeviceManager;

class Child
{
public:
	Child();
	~Child();

	void Draw();

private:
	DeviceManager* mDeviceManager;
};

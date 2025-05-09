#ifndef DEVICE_H
#define DEVICE_H

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class Device {
protected:
	bool isConnected;
	bool isDeviceChanged;
	std::vector<bool> deviceStatus;

public:
	Device();
	virtual ~Device();

	virtual void initialize() = 0;
	void sendDeviceStatus();

	bool getConnectionStatus() const;
	void setConnectionStatus(bool status);
	std::vector<bool> getDeviceStatus() const;
	void updateDeviceStatus(int deviceIndex, bool status);
};

#endif	// DEVICE_H
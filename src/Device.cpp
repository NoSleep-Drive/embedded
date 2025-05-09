#include "../include/Device.h"

Device::Device() : isConnected(false), isDeviceChanged(false) {
	// Initialize device status for all three devices (camera, accelerometer, speaker)
	deviceStatus = {false, false, false};
}

Device::~Device() {}

bool Device::getConnectionStatus() const {
	return isConnected;
}

void Device::setConnectionStatus(bool status) {
	isConnected = status;
	isDeviceChanged = true;
}

std::vector<bool> Device::getDeviceStatus() const {
	return deviceStatus;
}

void Device::updateDeviceStatus(int deviceIndex, bool status) {
	if (deviceIndex >= 0 && deviceIndex < deviceStatus.size()) {
		deviceStatus[deviceIndex] = status;
		isDeviceChanged = true;
	}
}

void Device::sendDeviceStatus() {
	// TODO: 추후 개발
}
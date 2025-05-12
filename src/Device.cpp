#include "../include/Device.h"

Device::Device() : isConnected(false), isDeviceChanged(false) {
	// 장치 상태 벡터: [0] = 카메라, [1] = 가속도 센서, [2] = 스피커
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
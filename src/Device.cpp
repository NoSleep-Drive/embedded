#include "../include/Device.h"
#include <cpr/cpr.h>
#include "../include/Utils.h"

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
    std::string hash = getenv("EMBEDDED_HASH");
    std::string deviceUid = getenv("DEVICE_UID");
    std::string serverIP = getenv("SERVER_IP");

    cpr::Header headers = {
        {"Content-Type", "application/json; charset=utf-8"},
        {"Authorization", "Bearer "+ hash}
    };
    std::string status_1 = deviceStatus[0] ? "true" : "false";
    std::string status_2 = deviceStatus[1] ? "true" : "false";
    std::string status_3 = deviceStatus[2] ? "true" : "false";


    std::string jsonBody = "{"
        "\"deviceUid\": \"" + deviceUid + "\","
        "\"cameraState\": " + status_1 + ","
        "\"accelerationSensorState\": " + status_2 + ","
        "\"speakerState\": " + status_3+
        "}";

    cpr::Response r = cpr::Patch(
        cpr::Url{ serverIP + "/vehicles/status" },
        headers,
        cpr::Body{ jsonBody }
    );
}
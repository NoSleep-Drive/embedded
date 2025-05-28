#include "../include/Device.h"

#include <cpr/cpr.h>

#include <iostream>

#include "../include/Utils.h"

// DeviceStatusManager 구현
DeviceStatusManager& DeviceStatusManager::getInstance() {
	static DeviceStatusManager instance;
	return instance;
}

DeviceStatusManager::DeviceStatusManager() {
	// 장치 상태 벡터: [0] = 카메라, [1] = 가속도 센서, [2] = 스피커
	deviceStatus = {false, false, false};
}

void DeviceStatusManager::updateDeviceStatus(int deviceIndex, bool status) {
	if (deviceIndex >= 0 && deviceIndex < deviceStatus.size()) {
		deviceStatus[deviceIndex] = status;
		std::cout << "Device " << deviceIndex
							<< " status updated to: " << (status ? "connected" : "disconnected") << std::endl;
	}
}

std::vector<bool> DeviceStatusManager::getAllDeviceStatus() const {
	return deviceStatus;
}

bool DeviceStatusManager::getDeviceStatus(int deviceIndex) const {
	if (deviceIndex >= 0 && deviceIndex < deviceStatus.size()) {
		return deviceStatus[deviceIndex];
	}
	return false;
}

void DeviceStatusManager::sendDeviceStatusToBackend() {
	const char* hashC = std::getenv("EMBEDDED_HASH");
	const char* uidC = std::getenv("DEVICE_UID");
	const char* ipC = std::getenv("SERVER_IP");

	if (!hashC || !uidC || !ipC) {
		std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
		return;
	}

	std::string hash(hashC);
	std::string deviceUid(uidC);
	std::string serverIP(ipC);

	cpr::Header headers = {{"Content-Type", "application/json; charset=utf-8"},
												 {"Authorization", "Bearer " + hash}};

	std::string status_1 = deviceStatus[0] ? "true" : "false";
	std::string status_2 = deviceStatus[1] ? "true" : "false";
	std::string status_3 = deviceStatus[2] ? "true" : "false";

	nlohmann::json jsonData = {{"deviceUid", deviceUid},
														 {"cameraState", deviceStatus[0]},
														 {"accelerationSensorState", deviceStatus[1]},
														 {"speakerState", deviceStatus[2]}};
	std::string jsonBody = jsonData.dump();

	std::cout << "백엔드로 장치 상태 전송 중..." << std::endl;
	std::cout << "Camera: " << status_1 << ", AccelSensor: " << status_2 << ", Speaker: " << status_3
						<< std::endl;

	try {
		cpr::Response r = cpr::Patch(cpr::Url{serverIP + "/vehicles/status"}, headers,
																 cpr::Body{jsonBody}, cpr::Timeout{5000}	// 5초 타임아웃
		);

		if (r.error) {
			std::cerr << "장치 상태 전송 오류: " << r.error.message << std::endl;
		} else if (r.status_code == 200) {
			std::cout << "장치 상태 전송 성공" << std::endl;
		} else {
			std::cerr << "장치 상태 전송 실패 - 상태 코드: " << r.status_code << ", 응답: " << r.text
								<< std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "장치 상태 전송 중 예외 발생: " << e.what() << std::endl;
	}
}

// Device 클래스 구현
Device::Device() : isConnected(false), isDeviceChanged(false) {}

Device::~Device() {}

bool Device::getConnectionStatus() const {
	return isConnected;
}

void Device::setConnectionStatus(bool status) {
	isConnected = status;
	isDeviceChanged = true;
}

void Device::updateDeviceStatus(int deviceIndex, bool status) {
	DeviceStatusManager::getInstance().updateDeviceStatus(deviceIndex, status);
}
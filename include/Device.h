#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// 전역 장치 상태 관리를 위한 싱글톤 클래스
class DeviceStatusManager {
public:
	static DeviceStatusManager& getInstance();

	// 장치 상태 업데이트
	void updateDeviceStatus(int deviceIndex, bool status);

	// 전체 장치 상태 조회
	std::vector<bool> getAllDeviceStatus() const;

	// 특정 장치 상태 조회
	bool getDeviceStatus(int deviceIndex) const;

	// 백엔드로 장치 상태 전송
	void sendDeviceStatusToBackend();

private:
	DeviceStatusManager();
	std::vector<bool> deviceStatus;	 // [0] = 카메라, [1] = 가속도 센서, [2] = 스피커
};

class Device {
protected:
	bool isConnected;
	bool isDeviceChanged;

public:
	Device();
	virtual ~Device();

	virtual void initialize() = 0;

	bool getConnectionStatus() const;
	void setConnectionStatus(bool status);

	// 장치 상태 업데이트 (전역 상태 매니저 사용)
	void updateDeviceStatus(int deviceIndex, bool status);
};

#endif	// DEVICE_H
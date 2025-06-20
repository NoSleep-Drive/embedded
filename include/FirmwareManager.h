#ifndef FIRMWARE_MANAGER_H
#define FIRMWARE_MANAGER_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Python.h 및 NumPy 헤더 포함
#include <Python.h>
#define PY_ARRAY_UNIQUE_SYMBOL NOSLEEP_ARRAY_API
#define NPY_NO_DEPRECATED_API	 NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include "AccelerationSensor.h"
#include "Camera.h"
#include "DBThreadMonitoring.h"
#include "EyeClosureQueueManagement.h"
#include "SleepinessDetector.h"
#include "Speaker.h"
#include "Utils.h"

class FirmwareManager {
private:
	// 장치 객체들
	std::unique_ptr<Camera> camera;
	std::unique_ptr<AccelerationSensor> accelerationSensor;
	std::unique_ptr<Speaker> speaker;
	std::unique_ptr<SleepinessDetector> sleepinessDetector;
	std::unique_ptr<EyeClosureQueueManagement> eyeClosureQueue;
	std::unique_ptr<Utils> utils;
	std::unique_ptr<DBThreadMonitoring> threadMonitor;

	// UUID 및 기타 필드
	std::string deviceUID;

	// 시스템 상태
	std::atomic<bool> isRunning;
	std::atomic<bool> isPaused;

	// 처리 주기 관련 변수
	int frameCycle;
	int diagnosticCycle;

	// 스레드
	std::thread mainThread;
	std::mutex detectionMutex;

	// 이전 졸음 상태
	bool previousSleepy = false;

	// 졸음 진단 폴더 경로 저장 스택 (DB 스레드 모니터링용)
	std::stack<std::string> sleepImgPathStack;

	// 내부 메서드
	void mainLoop();
	bool processSingleFrame();
	void requestDiagnosis();
	void initializeDevices();
	void handleVehicleStopped();
	void handleSleepinessDetected(const std::string& timestamp);

	// 장치 상태 백엔드 전송
	void sendDeviceStatusToBackend();

	// NumPy 초기화 메서드
	bool initializePythonAndNumpy();

public:
	FirmwareManager(const std::string& uid = "rasp-0001");
	~FirmwareManager();

	void start();
	void stop();
	void pause();
	void resume();

	std::string getDeviceUID() const;

	// 상태 확인 메서드
	bool isDeviceRunning() const;
};

#endif	// FIRMWARE_MANAGER_H
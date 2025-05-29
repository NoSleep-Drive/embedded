#ifndef FIRMWARE_MANAGER_H
#define FIRMWARE_MANAGER_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <future>

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

	// 내부 메서드
	void mainLoop();
	bool processSingleFrame();
	bool requestDiagnosis();
	void initializeDevices();
	void handleVehicleStopped();
	void handleSleepinessDetected(const std::string& timestamp);

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
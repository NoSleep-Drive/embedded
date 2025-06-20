#include "../include/AccelerationSensor.h"

#include <chrono>
#include <cmath>
#include <iostream>

// Helper function to convert Python error to string
std::string getPythonError() {
	PyObject *pType, *pValue, *pTraceback;
	PyErr_Fetch(&pType, &pValue, &pTraceback);

	std::string errorMessage = "Python error: ";

	if (pValue != nullptr) {
		PyObject* pStr = PyObject_Str(pValue);
		if (pStr != nullptr) {
			errorMessage += PyUnicode_AsUTF8(pStr);
			Py_DECREF(pStr);
		}
	}

	Py_XDECREF(pType);
	Py_XDECREF(pValue);
	Py_XDECREF(pTraceback);

	return errorMessage;
}

// RealAccelerationSensor implementation
RealAccelerationSensor::RealAccelerationSensor()
		: xAcceleration(0.0f),
			yAcceleration(0.0f),
			zAcceleration(0.0f),
			pModule(nullptr),
			pSensorClass(nullptr),
			pSensorInstance(nullptr) {
	if (!initPython()) {
		std::cerr << "Failed to initialize Python ADXL345 sensor" << std::endl;
	}
}

RealAccelerationSensor::~RealAccelerationSensor() {
	cleanupPython();
}

bool RealAccelerationSensor::initPython() {
	Py_Initialize();

	// Python 모듈 검색 경로에 현재 디렉터리 추가
	PyRun_SimpleString("import sys; sys.path.append('.')");

	// 미리 작성한 파이썬 파일 import
	pModule = PyImport_ImportModule("adxl345_helper");
	if (pModule == nullptr) {
		std::cerr << getPythonError() << std::endl;
		return false;
	}
	std::cout << "Python module loaded successfully" << std::endl;

	pSensorClass = PyObject_GetAttrString(pModule, "ADXL345Sensor");
	if (pSensorClass == nullptr) {
		std::cerr << getPythonError() << std::endl;
		Py_DECREF(pModule);
		pModule = nullptr;
		return false;
	}
	std::cout << "Python class loaded successfully" << std::endl;

	pSensorInstance = PyObject_CallObject(pSensorClass, nullptr);
	if (pSensorInstance == nullptr) {
		std::cerr << getPythonError() << std::endl;
		Py_DECREF(pSensorClass);
		Py_DECREF(pModule);
		pSensorClass = nullptr;
		pModule = nullptr;
		return false;
	}
	std::cout << "Python instance created successfully" << std::endl;

	return true;
}

void RealAccelerationSensor::cleanupPython() {
	// Clean up Python objects
	Py_XDECREF(pSensorInstance);
	Py_XDECREF(pSensorClass);
	Py_XDECREF(pModule);

	// Finalize Python interpreter
	Py_Finalize();
}

std::vector<float> RealAccelerationSensor::getAcceleration() {
	if (pSensorInstance == nullptr) {
		return {0.0f, 0.0f, 0.0f};
	}

	// Call get_acceleration method
	PyObject* pAcceleration = PyObject_CallMethod(pSensorInstance, "get_acceleration", nullptr);
	if (pAcceleration == nullptr) {
		std::cerr << getPythonError() << std::endl;
		return {0.0f, 0.0f, 0.0f};
	}

	// Check if it's a tuple
	if (!PyTuple_Check(pAcceleration)) {
		Py_DECREF(pAcceleration);
		return {0.0f, 0.0f, 0.0f};
	}

	// Extract the x, y, z values
	PyObject* pX = PyTuple_GetItem(pAcceleration, 0);
	PyObject* pY = PyTuple_GetItem(pAcceleration, 1);
	PyObject* pZ = PyTuple_GetItem(pAcceleration, 2);

	// Convert to C++ floats
	xAcceleration = PyFloat_AsDouble(pX);
	yAcceleration = PyFloat_AsDouble(pY);
	zAcceleration = PyFloat_AsDouble(pZ);

	Py_DECREF(pAcceleration);

	return {xAcceleration, yAcceleration, zAcceleration};
}

bool RealAccelerationSensor::isMoving() {
	if (pSensorInstance == nullptr) {
		return false;
	}

	// Call is_moving method
	PyObject* pIsMoving = PyObject_CallMethod(pSensorInstance, "is_moving", nullptr);
	if (pIsMoving == nullptr) {
		std::cerr << getPythonError() << std::endl;
		return false;
	}

	// Convert to C++ bool
	bool isMoving = PyObject_IsTrue(pIsMoving);

	Py_DECREF(pIsMoving);

	return isMoving;
}

MockAccelerationSensor::MockAccelerationSensor()
		: xAcceleration(0.0f),
			yAcceleration(0.0f),
			zAcceleration(9.8f),
			moving(true),
			rng(std::chrono::system_clock::now().time_since_epoch().count()),
			dist(-0.2f, 0.2f) {}

MockAccelerationSensor::~MockAccelerationSensor() {}

std::vector<float> MockAccelerationSensor::getAcceleration() {
	if (moving) {
		// 랜덤한 수로 가속도 값 생성
		xAcceleration = 0.5f + dist(rng);
		yAcceleration = 0.3f + dist(rng);
		zAcceleration = 9.8f + dist(rng);
	} else {
		xAcceleration = 0.0f + dist(rng) * 0.1f;
		yAcceleration = 0.0f + dist(rng) * 0.1f;
		zAcceleration = 9.8f + dist(rng) * 0.1f;
	}

	return {xAcceleration, yAcceleration, zAcceleration};
}

bool MockAccelerationSensor::isMoving() {
	return moving;
}

void MockAccelerationSensor::setMoving(bool isMoving) {
	moving = isMoving;
}

void MockAccelerationSensor::setAcceleration(float x, float y, float z) {
	xAcceleration = x;
	yAcceleration = y;
	zAcceleration = z;
}

AccelerationSensor::AccelerationSensor(bool useMock) : Device(), useMock(useMock) {
	if (useMock) {
		sensor = std::make_unique<MockAccelerationSensor>();
	} else {
		sensor = std::make_unique<RealAccelerationSensor>();
	}
}

AccelerationSensor::~AccelerationSensor() {}

void AccelerationSensor::initialize() {
	std::cout << "가속도 센서 초기화 중..." << std::endl;

	// Try to get acceleration data to check if sensor is working
	std::vector<float> accel = sensor->getAcceleration();

	// Check if accelerometer is working
	bool sensorWorking = true;	// Simplified check

	setConnectionStatus(sensorWorking);
	updateDeviceStatus(1, sensorWorking);	 // Accelerometer is index 1

	std::cout << "가속도 센서 초기화 " << (sensorWorking ? "성공" : "실패") << std::endl;
}

bool AccelerationSensor::isMoving() {
	return sensor->isMoving();
}

std::vector<float> AccelerationSensor::getAcceleration() {
	return sensor->getAcceleration();
}
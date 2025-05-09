#ifndef ACCELERATION_SENSOR_H
#define ACCELERATION_SENSOR_H

#include <Python.h>

#include <random>
#include <vector>

#include "Device.h"

// Interface for acceleration sensor to allow mocking
class IAccelerationSensor {
public:
	virtual ~IAccelerationSensor() = default;
	virtual std::vector<float> getAcceleration() = 0;
	virtual bool isMoving() = 0;
};

// Real implementation of acceleration sensor using Python adafruit_adxl34x
class RealAccelerationSensor : public IAccelerationSensor {
private:
	float xAcceleration;
	float yAcceleration;
	float zAcceleration;

	PyObject* pModule;
	PyObject* pSensorClass;
	PyObject* pSensorInstance;

	bool initPython();
	void cleanupPython();

public:
	RealAccelerationSensor();
	~RealAccelerationSensor() override;

	std::vector<float> getAcceleration() override;
	bool isMoving() override;
};

// Mock implementation remains the same
class MockAccelerationSensor : public IAccelerationSensor {
private:
	float xAcceleration;
	float yAcceleration;
	float zAcceleration;
	bool moving;
	std::mt19937 rng;
	std::uniform_real_distribution<float> dist;

public:
	MockAccelerationSensor();
	~MockAccelerationSensor() override;

	std::vector<float> getAcceleration() override;
	bool isMoving() override;

	// Additional methods for testing
	void setMoving(bool isMoving);
	void setAcceleration(float x, float y, float z);
};

// AccelerationSensor class remains the same
class AccelerationSensor : public Device {
private:
	std::unique_ptr<IAccelerationSensor> sensor;
	bool useMock;

public:
	AccelerationSensor(bool useMock = false);
	~AccelerationSensor() override;

	void initialize() override;

	bool isMoving();
	std::vector<float> getAcceleration();
};

#endif	// ACCELERATION_SENSOR_H
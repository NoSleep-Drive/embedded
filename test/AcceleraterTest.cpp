#include <chrono>
#include <iostream>
#include <thread>

#include "../include/AccelerationSensor.h"

void testRealAccelerationSensor() {
	std::cout << "========== Testing RealAccelerationSensor ==========" << std::endl;

	// Create a real acceleration sensor
	RealAccelerationSensor sensor;

	// Test isMoving and getAcceleration every second for 10 seconds
	for (int i = 0; i < 10; i++) {
		// Get acceleration data
		std::vector<float> accel = sensor.getAcceleration();

		// Check if moving
		bool moving = sensor.isMoving();

		// Print results
		std::cout << "Test #" << (i + 1) << ":" << std::endl;
		std::cout << "  Acceleration: X=" << accel[0] << ", Y=" << accel[1] << ", Z=" << accel[2]
							<< std::endl;
		std::cout << "  Moving: " << (moving ? "YES" : "NO") << std::endl;

		// Wait for a second
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "Test completed." << std::endl;
}

void testMockAccelerationSensor() {
	std::cout << "========== Testing MockAccelerationSensor ==========" << std::endl;

	// Create a mock acceleration sensor
	MockAccelerationSensor sensor;

	// Test both stationary and moving states
	std::cout << "Testing stationary state:" << std::endl;
	sensor.setMoving(false);

	std::vector<float> accel = sensor.getAcceleration();
	bool moving = sensor.isMoving();

	std::cout << "  Acceleration: X=" << accel[0] << ", Y=" << accel[1] << ", Z=" << accel[2]
						<< std::endl;
	std::cout << "  Moving: " << (moving ? "YES" : "NO") << std::endl;

	std::cout << "Testing moving state:" << std::endl;
	sensor.setMoving(true);

	accel = sensor.getAcceleration();
	moving = sensor.isMoving();

	std::cout << "  Acceleration: X=" << accel[0] << ", Y=" << accel[1] << ", Z=" << accel[2]
						<< std::endl;
	std::cout << "  Moving: " << (moving ? "YES" : "NO") << std::endl;

	std::cout << "Test completed." << std::endl;
}

void testAccelerationSensor() {
	std::cout << "========== Testing AccelerationSensor ==========" << std::endl;

	// Test with real sensor
	std::cout << "Using real sensor:" << std::endl;
	AccelerationSensor realSensor(false);
	realSensor.initialize();

	std::vector<float> accel = realSensor.getAcceleration();
	bool moving = realSensor.isMoving();

	std::cout << "  Acceleration: X=" << accel[0] << ", Y=" << accel[1] << ", Z=" << accel[2]
						<< std::endl;
	std::cout << "  Moving: " << (moving ? "YES" : "NO") << std::endl;

	// Test with mock sensor
	std::cout << "Using mock sensor:" << std::endl;
	AccelerationSensor mockSensor(true);
	mockSensor.initialize();

	accel = mockSensor.getAcceleration();
	moving = mockSensor.isMoving();

	std::cout << "  Acceleration: X=" << accel[0] << ", Y=" << accel[1] << ", Z=" << accel[2]
						<< std::endl;
	std::cout << "  Moving: " << (moving ? "YES" : "NO") << std::endl;

	std::cout << "Test completed." << std::endl;
}

// Entry point for test file
int runTests() {
	std::cout << "Starting acceleration sensor tests..." << std::endl;

	try {
		// Test RealAccelerationSensor directly
		testRealAccelerationSensor();

		// Test MockAccelerationSensor
		testMockAccelerationSensor();

		// Test AccelerationSensor class with both real and mock implementations
		testAccelerationSensor();

		std::cout << "All tests completed successfully!" << std::endl;
	} catch (const std::exception& ex) {
		std::cerr << "Test failed with exception: " << ex.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Test failed with unknown exception!" << std::endl;
		return 1;
	}

	return 0;
}
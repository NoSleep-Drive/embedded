#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "../include/AccelerationSensor.h"
#include "../include/Camera.h"

// Forward declaration of test function
int runTests();

void displayHelp() {
	std::cout << "NoSleep Drive - Acceleration Sensor Test" << std::endl;
	std::cout << "Commands:" << std::endl;
	std::cout << "  test       - Run all acceleration sensor tests" << std::endl;
	std::cout << "  real       - Run continuous test with real sensor" << std::endl;
	std::cout << "  mock       - Run continuous test with mock sensor" << std::endl;
	std::cout << "  integrated - Run continuous test with AccelerationSensor class" << std::endl;
	std::cout << "  exit       - Exit the program" << std::endl;
	std::cout << "  help       - Display this help message" << std::endl;
}

void runContinuousRealSensorTest() {
	std::cout << "Starting continuous test with real sensor. Press Ctrl+C to stop." << std::endl;

	RealAccelerationSensor sensor;

	int count = 0;
	while (true) {
		std::vector<float> accel = sensor.getAcceleration();
		bool moving = sensor.isMoving();

		std::cout << "\r                                                                    ";
		std::cout << "\rTest #" << ++count << ": X=" << accel[0] << ", Y=" << accel[1]
							<< ", Z=" << accel[2] << " | Moving: " << (moving ? "YES" : "NO");
		std::cout.flush();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void runContinuousMockSensorTest() {
	std::cout << "Starting continuous test with mock sensor. Press Ctrl+C to stop." << std::endl;

	MockAccelerationSensor sensor;

	// Alternate between moving and stationary every 5 seconds
	bool shouldBeMobving = false;
	int secondsInState = 0;
	int count = 0;

	while (true) {
		if (secondsInState >= 5) {
			shouldBeMobving = !shouldBeMobving;
			secondsInState = 0;
			sensor.setMoving(shouldBeMobving);
			std::cout << std::endl
								<< "Changing state to: " << (shouldBeMobving ? "MOVING" : "STATIONARY")
								<< std::endl;
		}

		std::vector<float> accel = sensor.getAcceleration();
		bool moving = sensor.isMoving();

		std::cout << "\r                                                                    ";
		std::cout << "\rTest #" << ++count << ": X=" << accel[0] << ", Y=" << accel[1]
							<< ", Z=" << accel[2] << " | Moving: " << (moving ? "YES" : "NO");
		std::cout.flush();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		secondsInState += 0.5;
	}
}

void runContinuousIntegratedTest() {
	std::cout << "Do you want to use mock sensor? (y/n): ";
	char choice;
	std::cin >> choice;

	bool useMock = (choice == 'y' || choice == 'Y');

	std::cout << "Starting continuous test with " << (useMock ? "mock" : "real")
						<< " sensor. Press Ctrl+C to stop." << std::endl;

	AccelerationSensor sensor(useMock);
	sensor.initialize();

	int count = 0;
	while (true) {
		std::vector<float> accel = sensor.getAcceleration();
		bool moving = sensor.isMoving();

		std::cout << "\r                                                                    ";
		std::cout << "\rTest #" << ++count << ": X=" << accel[0] << ", Y=" << accel[1]
							<< ", Z=" << accel[2] << " | Moving: " << (moving ? "YES" : "NO");
		std::cout.flush();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

int main(int argc, char* argv[]) {
	std::cout << "NoSleep Drive - Acceleration Sensor Test Program" << std::endl;
	std::cout << "==================================================" << std::endl;

	// If command line argument is provided, use it as the command
	std::string command;
	if (argc > 1) {
		command = argv[1];
	} else {
		displayHelp();
		std::cout << std::endl << "Enter command: ";
		std::cin >> command;
	}

	try {
		while (command != "exit") {
			if (command == "test") {
				runTests();
			} else if (command == "real") {
				runContinuousRealSensorTest();
			} else if (command == "mock") {
				runContinuousMockSensorTest();
			} else if (command == "integrated") {
				runContinuousIntegratedTest();
			} else if (command == "help") {
				displayHelp();
			} else {
				std::cout << "Unknown command: " << command << std::endl;
				displayHelp();
			}

			std::cout << std::endl << "Enter command: ";
			std::cin >> command;
		}
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unknown error occurred!" << std::endl;
		return 1;
	}

	std::cout << "Exiting program. Goodbye!" << std::endl;
	return 0;
}
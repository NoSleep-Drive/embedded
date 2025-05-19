#include <iostream>

#include "../include/Camera.h"

int runCameraTest() {
	std::cout << "NoSleep Drive - Camera Class Test" << std::endl;
	std::cout << "======================================" << std::endl;

	Camera camera;
	camera.initialize();

	if (!camera.getCameraStatus()) {
		std::cerr << "Camera initialization failed. Exiting..." << std::endl;
		return -1;
	}

	std::cout << "Camera initialized successfully. Capturing frames..." << std::endl;

	while (true) {
		cv::Mat frame = camera.captureFrame();

		if (frame.empty()) {
			std::cerr << "Empty frame received. Exiting..." << std::endl;
			break;
		}

		cv::imshow("Camera Feed", frame);

		// 30ms 대기, 'q' 키로 종료
		if (cv::waitKey(30) == 'q') {
			break;
		}
	}

	cv::destroyAllWindows();
	std::cout << "Camera test completed." << std::endl;

	return 0;
}
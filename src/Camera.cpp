#include "../include/Camera.h"

Camera::Camera() : Device() {
	resolution = {1920, 1080};
}

Camera::~Camera() {}

void Camera::initialize() {
	cap.open(0);

	if (!cap.isOpened()) {
		std::cerr << "Error: Could not open camera." << std::endl;
		setConnectionStatus(false);
		updateDeviceStatus(0, false);	 // Camera is index 0
		return;
	}

	// Set resolution
	cap.set(cv::CAP_PROP_FRAME_WIDTH, resolution[0]);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, resolution[1]);

	// Capture a test frame to ensure camera is working
	cv::Mat testFrame;
	cap >> testFrame;

	if (testFrame.empty()) {
		std::cerr << "Error: Could not capture frame." << std::endl;
		setConnectionStatus(false);
		updateDeviceStatus(0, false);	 // Camera is index 0
		return;
	}

	// Camera is working
	setConnectionStatus(true);
	updateDeviceStatus(0, true);	// Camera is index 0
}

cv::Mat Camera::captureFrame() {
	if (!cap.isOpened() && !cap.open(0)) {
		std::cerr << "Error: Could not open camera." << std::endl;
		setCameraStatus(false);
		return cv::Mat();
	}

	cv::Mat frame;
	cap >> frame;

	if (frame.empty()) {
		std::cerr << "Error: Could not capture frame." << std::endl;
		setCameraStatus(false);
		return cv::Mat();
	}

	return frame;
}

void Camera::setCameraStatus(bool status) {
	updateDeviceStatus(0, status);	// Camera is index 0
}

bool Camera::getCameraStatus() const {
	return deviceStatus[0];
}
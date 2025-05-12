#include "../include/Camera.h"

#include <iostream>

Camera::Camera() : Device() {
	resolution = {1920, 1080};
	cameraName =
			"/base/soc/i2c0mux/i2c@1/ov5647@36";	// 기본 카메라 경로 (실제 환경에 맞게 수정 필요)
}

Camera::~Camera() {
	if (cap.isOpened()) {
		cap.release();
	}
}

void Camera::initialize() {
	// GStreamer 파이프라인 구성
	gstreamerPipeline = "libcamerasrc camera-name=" + cameraName +
											" ! video/x-raw,width=" + std::to_string(resolution[0]) +
											",height=" + std::to_string(resolution[1]) + ",framerate=10/1,format=RGBx" +
											" ! videoconvert ! videoscale" + " ! video/x-raw,format=BGR" + " ! appsink";

	// GStreamer 파이프라인으로 카메라 열기
	cap.open(gstreamerPipeline, cv::CAP_GSTREAMER);

	if (!cap.isOpened()) {
		std::cerr << "Error: Could not open camera with GStreamer pipeline." << std::endl;
		std::cerr << "Pipeline: " << gstreamerPipeline << std::endl;
		setConnectionStatus(false);
		updateDeviceStatus(0, false);	 // Camera is index 0
		return;
	}

	// 테스트 프레임 캡처
	cv::Mat testFrame;
	bool success = cap.read(testFrame);

	if (!success || testFrame.empty()) {
		std::cerr << "Error: Could not capture test frame." << std::endl;
		setConnectionStatus(false);
		updateDeviceStatus(0, false);	 // Camera is index 0
		return;
	}

	// 카메라 작동 중
	std::cout << "Camera initialized successfully!" << std::endl;
	setConnectionStatus(true);
	updateDeviceStatus(0, true);	// Camera is index 0
}

cv::Mat Camera::captureFrame() {
	if (!cap.isOpened()) {
		std::cerr << "Error: Camera is not open. Attempting to initialize..." << std::endl;
		initialize();

		if (!cap.isOpened()) {
			setCameraStatus(false);
			return cv::Mat();
		}
	}

	cv::Mat frame;
	bool success = cap.read(frame);

	if (!success || frame.empty()) {
		std::cerr << "Error: Failed to capture frame." << std::endl;
		setCameraStatus(false);
		return cv::Mat();
	}

	setCameraStatus(true);
	return frame;
}

void Camera::setCameraStatus(bool status) {
	updateDeviceStatus(0, status);	// Camera is index 0
}

bool Camera::getCameraStatus() const {
	return deviceStatus[0];
}

void Camera::setResolution(int width, int height) {
	// 이미 열려있는 경우 닫기
	if (cap.isOpened()) {
		cap.release();
	}

	resolution[0] = width;
	resolution[1] = height;

	// 새 해상도로 다시 초기화
	initialize();
}

std::vector<int> Camera::getResolution() const {
	return resolution;
}
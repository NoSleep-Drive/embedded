#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>

#include "Device.h"

class Camera : public Device {
private:
	cv::Mat frameBuffer;
	std::vector<int> resolution;
	cv::VideoCapture cap;

public:
	Camera();
	~Camera() override;

	void initialize() override;

	cv::Mat captureFrame();
	void setCameraStatus(bool status);
	bool getCameraStatus() const;
};

#endif	// CAMERA_H
#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include <string>

#include "Device.h"

class Camera : public Device {
private:
	cv::VideoCapture cap;
	std::vector<int> resolution;
	std::string cameraName;
	std::string gstreamerPipeline;

public:
	Camera();
	~Camera();

	void initialize() override;
	cv::Mat captureFrame();
	void setCameraStatus(bool status);
	bool getCameraStatus() const;
	void setResolution(int width, int height);
	std::vector<int> getResolution() const;
};

#endif	// CAMERA_H
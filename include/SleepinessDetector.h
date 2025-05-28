#ifndef SLEEPINESS_DETECTOR_H
#define SLEEPINESS_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <queue>
#include <stack>
#include <string>

#include "EyeClosureQueueManagement.h"

class SleepinessDetector {
private:
	static const int closureCountForSleepiness = 48;
	std::string sleepImgPath;
	std::stack<std::string> sleepImgPathStack;

public:
	SleepinessDetector();

	void sendDriverFrame(const cv::Mat& frame);
	void requestAIDetection(
			const std::string& uid, const std::string& requestTime,
			std::function<void(bool success, bool isDrowsy, const std::string& message)> callback);
	bool getLocalDetection(EyeClosureQueueManagement& eyeManager);
	void updateBaseSleepImgPath(const std::string& path);
};

#endif

#ifndef SLEEPINESS_DETECTOR_H
#define SLEEPINESS_DETECTOR_H

#include <string>
#include <stack>
#include <queue>
#include <opencv2/opencv.hpp>
#include "EyeClosureQueueManagement.h"

class SleepinessDetector {
private:
    static const int closureCountForSleepiness = 48;
    std::string sleepImgPath;
    std::stack<std::string> sleepImgPathStack;

public:
    SleepinessDetector();

    void sendDriverFrame(const cv::Mat& frame);
    bool requestAIDetection(const std::string& uid, const std::string& requestTime);
    bool getLocalDetection(EyeClosureQueueManagement& eyeManager);
    void updateBaseSleepImgPath(const std::string& path);
};

#endif

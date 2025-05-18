#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <opencv2/opencv.hpp>

class Utils {
public:
    Utils(const std::string& saveDirectory = "./frames");

    bool saveFrame(const cv::Mat& frame, const std::string& path, const std::string& name);

    bool saveFrameToCurrentFrameFolder(const cv::Mat& frame, const std::string& name);

    bool saveFrameToSleepinessFolder(const cv::Mat& frame, const std::string& name);

    bool removeFolder(const std::string& path);

    std::vector<cv::Mat> loadFramesFromRecentFolder();

    std::vector<cv::Mat> loadFramesFromFolder(const std::string& folderPath);

    std::string createSleepinessDir(const std::string& timeStamp);

private:
    std::string saveDirectory;
    std::string recentFolder;
    std::string sleepFolder;
};

#endif
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <opencv2/opencv.hpp>

#include <cstdlib>
#include <string>

void setEnvVar(const std::string& key, const std::string& value);

class Utils {
public:
    Utils(const std::string& saveDirectory = "./frames");

    bool saveFrame(const cv::Mat& frame, const std::string& path, const std::string& name);

    bool saveFrameToCurrentFrameFolder(const cv::Mat& frame, const std::string& name);

    bool saveFrameToSleepinessFolder(const cv::Mat& frame, const std::string& name);

    bool removeSleepinessEvidenceFolder() { return removeFolder(sleepFolder); }

    bool removeFolder(const std::string& path);

    std::vector<cv::Mat> loadFramesFromRecentFolder(const std::string& timeStamp);

    std::vector<std::pair<std::string, std::string>> getRecentFramePathsAndNames(
        const std::string& timeStamp);

    std::string createSleepinessDir(const std::string& timeStamp);

    void loadEnvFile(const std::string& filename);

    long long parseFileTimeMillis(const std::string& filename);

    int sleepinessEvidenceCount = 0;
    const int MAX_SLEEPINESS_EVIDENCE_COUNT = 120;
    bool IsSavingSleepinessEvidence = false;
    std::string saveDirectory;

private:
    std::string recentFolder;
    std::string sleepFolder;
};

#endif
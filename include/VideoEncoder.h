#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

class VideoEncoder {
private:
    const int frameRate = 24;
    const cv::Size resolution = cv::Size(1280, 720);

public:
    std::vector<uchar> convertFramesToMP4(const std::string& path);
};

#endif

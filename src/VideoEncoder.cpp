#include "../include/VideoEncoder.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>

std::vector<uchar> VideoEncoder::convertFramesToMP4(const std::string& path) {
    std::cout << "파일 경로 " << path << " 생성 시간 기준 전후 각각 2.5초 이미지 프레임들을 영상으로 변환" << std::endl;
    // TODO: 파일 경로 기준 이미지 프레임들 -> 영상으로 변환
    std::vector<cv::String> framePaths;
    std::vector<uchar> videoBuffer;

    return videoBuffer;
}

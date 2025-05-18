#include "../include/VideoEncoder.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <fstream>

std::vector<uchar> VideoEncoder::convertFramesToMP4(const std::string& path) {
    // example path(folderPath): ./frames/20230518_120000
    std::cout << "파일 경로 " << path << " 생성 시간 기준 전후 각각 2.5초 이미지 프레임들을 영상으로 변환" << std::endl;
    std::vector<cv::String> framePaths;
    std::vector<uchar> videoBuffer;

    std::string folderName = std::filesystem::path(path).filename().string();
    std::tm folderTime = {};
    std::istringstream ss(folderName);
    ss >> std::get_time(&folderTime, "%Y%m%d_%H%M%S");

    if (ss.fail()) {
        std::cerr << "폴더 이름에서 타임스탬프를 파싱할 수 없음: " << folderName << std::endl;
        return videoBuffer;
    }

    auto baseTime = std::chrono::system_clock::from_time_t(std::mktime(&folderTime));

    auto startTime = baseTime - std::chrono::milliseconds(2500);
    auto endTime = baseTime + std::chrono::milliseconds(2500);

    std::string baseDir = std::filesystem::path(path).parent_path().string();
    for (const auto& entry : std::filesystem::directory_iterator(baseDir)) {
        if (!entry.is_directory()) continue;

        std::string dirName = entry.path().filename().string();
        std::tm dirTime = {};
        std::istringstream dirSS(dirName);
        dirSS >> std::get_time(&dirTime, "%Y%m%d_%H%M%S");

        if (dirSS.fail()) continue;

        auto dirTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&dirTime));

        if (dirTimePoint >= startTime && dirTimePoint <= endTime) {
            for (const auto& imgEntry : std::filesystem::directory_iterator(entry)) {
                if (imgEntry.path().extension() == ".jpg" || imgEntry.path().extension() == ".png") {
                    framePaths.push_back(imgEntry.path().string());
                }
            }
        }
    }

    if (framePaths.empty()) {
        std::cerr << "선택된 시간 범위에 이미지가 없음" << std::endl;
        return videoBuffer;
    }

    cv::Size frameSize(resolution[1], resolution[0]);
    std::string tempVideoPath = "temp.mp4";
    cv::VideoWriter writer(tempVideoPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), frameRate, frameSize);

    for (const auto& frame : framePaths) {
        cv::Mat img = cv::imread(frame);
        if (img.empty()) continue;
        cv::resize(img, img, frameSize);
        writer.write(img);
    }
    writer.release();

    std::ifstream tempVideoFile(tempVideoPath, std::ios::binary | std::ios::ate);
    if (!tempVideoFile) {
        std::cerr << "임시 비디오 파일을 읽을 수 없음" << std::endl;
        return videoBuffer;
    }

    std::streamsize size = tempVideoFile.tellg();
    tempVideoFile.seekg(0, std::ios::beg);

    videoBuffer.resize(size);
    if (!tempVideoFile.read(reinterpret_cast<char*>(videoBuffer.data()), size)) {
        std::cerr << "임시 비디오 파일을 메모리에 로드하는 데 실패" << std::endl;
        return videoBuffer;
    }

    tempVideoFile.close();
    std::filesystem::remove(tempVideoPath);

    return videoBuffer;
}

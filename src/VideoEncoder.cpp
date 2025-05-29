#include "../include/VideoEncoder.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

std::vector<uchar> VideoEncoder::convertFramesToMP4(const std::string& path) {
    std::vector<cv::String> framePaths;
    std::vector<uchar> videoBuffer;

    std::string folderName = std::filesystem::path(path).filename().string();
    std::tm folderTime = {};
    std::istringstream ss(folderName);
    ss >> std::get_time(&folderTime, "%Y%m%d_%H%M%S");

    if (ss.fail()) return videoBuffer;

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

    if (framePaths.empty()) return videoBuffer;

    const cv::Size frameSize(1280, 720);
    auto tmp = std::filesystem::temp_directory_path() / ("video_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".mp4");
    std::string tempVideoPath = tmp.string();
    std::string fixedVideoPath = tempVideoPath + "_fixed.mp4";

    // OpenCV로 MP4 생성
    cv::VideoWriter writer(tempVideoPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), frameRate, frameSize);
    for (const auto& frame : framePaths) {
        cv::Mat img = cv::imread(frame);
        if (img.empty()) continue;
        cv::resize(img, img, frameSize);
        writer.write(img);
    }
    writer.release();

    // ffmpeg 후처리: moov atom 앞으로 이동
    std::string command = "ffmpeg -y -i \"" + tempVideoPath + "\" -movflags faststart \"" + fixedVideoPath + "\"";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "ffmpeg 실행 실패" << std::endl;
        std::filesystem::remove(tempVideoPath);
        return videoBuffer;
    }

    // fixed mp4 파일을 메모리로 읽기
    std::ifstream fixedVideoFile(fixedVideoPath, std::ios::binary | std::ios::ate);
    if (!fixedVideoFile) {
        std::cerr << "fixed 비디오 파일을 읽을 수 없음" << std::endl;
        std::filesystem::remove(tempVideoPath);
        std::filesystem::remove(fixedVideoPath);
        return videoBuffer;
    }

    std::streamsize size = fixedVideoFile.tellg();
    fixedVideoFile.seekg(0, std::ios::beg);

    videoBuffer.resize(size);
    if (!fixedVideoFile.read(reinterpret_cast<char*>(videoBuffer.data()), size)) {
        std::cerr << "fixed 비디오 파일을 메모리에 로드하는 데 실패" << std::endl;
        fixedVideoFile.close();
        std::filesystem::remove(tempVideoPath);
        std::filesystem::remove(fixedVideoPath);
        return videoBuffer;
    }

    fixedVideoFile.close();
    std::filesystem::remove(tempVideoPath);
    std::filesystem::remove(fixedVideoPath);

    return videoBuffer;
}

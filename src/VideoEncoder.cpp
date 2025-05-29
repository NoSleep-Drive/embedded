#include "../include/VideoEncoder.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

std::vector<uchar> VideoEncoder::convertFramesToMP4(const std::string& path) {
	std::cout << "파일 경로 " << path
						<< " 생성 시간 기준 전후 각각 2.5초 이미지 프레임들을 영상으로 변환" << std::endl;
	std::vector<cv::String> framePaths;
	std::vector<uchar> videoBuffer;

	// 현재 폴더 안의 이미지 파일만 정렬해서 수집
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
			framePaths.push_back(entry.path().string());
		}
	}

	if (framePaths.empty()) {
		std::cerr << "선택된 폴더에 이미지가 없음" << std::endl;
		return videoBuffer;
	}

	// 파일 이름을 기준으로 정렬
	std::sort(framePaths.begin(), framePaths.end());

	const cv::Size frameSize(1280, 720);
	auto tmp = std::filesystem::temp_directory_path() /
						 ("video_" +
							std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".mp4");
	std::string tempVideoPath = tmp.string();
	cv::VideoWriter writer(tempVideoPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), frameRate,
												 frameSize);

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
	return videoBuffer;
}

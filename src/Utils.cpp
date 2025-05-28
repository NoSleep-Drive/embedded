#include "../include/Utils.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace fs = std::filesystem;

void setEnvVar(const std::string& key, const std::string& value) {
#ifdef _WIN32
	_putenv_s(key.c_str(), value.c_str());
#else
	setenv(key.c_str(), value.c_str(), 1);
#endif
}

Utils::Utils(const std::string& saveDirectory)
		: saveDirectory(saveDirectory), recentFolder("/recent"), sleepFolder("") {
	if (!std::filesystem::exists(saveDirectory)) {
		std::filesystem::create_directories(saveDirectory);
	}

	if (!std::filesystem::exists(saveDirectory + recentFolder)) {
		std::filesystem::create_directories(saveDirectory + recentFolder);
	}
}

bool Utils::saveFrame(const cv::Mat& frame, const std::string& path, const std::string& name) {
	if (frame.empty()) {
		std::cerr << "Error: Empty frame, cannot save." << std::endl;
		return false;
	}

	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path);
	}
	return cv::imwrite(path + "/" + name, frame);
}

bool Utils::saveFrameToCurrentFrameFolder(const cv::Mat& frame, const std::string& name) {
	return saveFrame(frame, saveDirectory + recentFolder, name);
}

bool Utils::saveFrameToSleepinessFolder(const cv::Mat& frame, const std::string& name) {
	if (sleepFolder.size() == 0) return false;
	return saveFrame(frame, saveDirectory + sleepFolder, name);
}

bool Utils::removeFolder(const std::string& folderName) {
	std::string folderPath = saveDirectory + "/" + folderName;
	try {
		if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
			return fs::remove_all(folderPath) > 0;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error removing folder: " << e.what() << std::endl;
	}
	return false;
}

std::vector<cv::Mat> Utils::loadFramesFromFolder(const std::string& folderName) {
	std::vector<cv::Mat> frames;
	std::vector<std::filesystem::directory_entry> entries;
	std::string fullPath = saveDirectory + "/" + folderName;
	if (!std::filesystem::exists(fullPath)) {
		std::cerr << "Directory does not exist: " << fullPath << std::endl;
		return frames;	// Return empty vector if directory does not exist
	}
	for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
		if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
			entries.push_back(entry);
		}
	}
	std::sort(entries.begin(), entries.end(),
						[](const auto& a, const auto& b) { return a.path().string() < b.path().string(); });
	for (const auto& entry : entries) {
		cv::Mat img = cv::imread(entry.path().string());
		if (!img.empty()) {
			frames.push_back(img);
		}
	}

	return frames;
}

std::vector<cv::Mat> Utils::loadFramesFromRecentFolder(const std::string& timeStamp) {
	if (timeStamp.empty()) {
		std::cerr << "Error: Time stamp is empty, cannot load frames." << std::endl;
		return {};
	}

	std::string folderPath = saveDirectory + recentFolder;
	long long timeStampNum = std::stoll(timeStamp);
	long long lowerBound = timeStampNum - 2500;

	using FileEntry = std::pair<long long, std::string>;	// (timestamp, path)
	auto cmp = [](const FileEntry& a, const FileEntry& b) {
		return a.first > b.first;
	};	// Min-heap by timestamp
	std::priority_queue<FileEntry, std::vector<FileEntry>, decltype(cmp)> minHeap(cmp);

	for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
		const std::string fileName = entry.path().stem().string();	// 확장자 제외

		// 빠른 유효성 검사: 파일명이 17자리 이상 숫자인지
		if (fileName.size() < 17 || !std::all_of(fileName.begin(), fileName.end(), ::isdigit)) continue;

		long long fileTime = std::stoll(fileName);
		if (fileTime <= timeStampNum && fileTime >= lowerBound) {
			minHeap.emplace(fileTime, entry.path().string());
			if (minHeap.size() > MAX_SLEEPINESS_EVIDENCE_COUNT) {
				minHeap.pop();	// 가장 오래된 파일 제거
			}
		}
	}

	// 힙에서 최신순으로 추출
	std::vector<FileEntry> selectedFiles;
	while (!minHeap.empty()) {
		selectedFiles.push_back(minHeap.top());
		minHeap.pop();
	}
	std::sort(selectedFiles.rbegin(), selectedFiles.rend());	// 최신순으로 정렬

	// 이미지 로드
	std::vector<cv::Mat> frames;
	for (const auto& [fileTime, path] : selectedFiles) {
		cv::Mat img = cv::imread(path);
		if (!img.empty()) {
			frames.push_back(img);
		}
	}

	return frames;
}

std::string Utils::createSleepinessDir(const std::string& timeStamp) {
	std::string path = "/" + timeStamp;
	if (!std::filesystem::exists(saveDirectory + path)) {
		std::filesystem::create_directories(saveDirectory + path);
	}

	this->sleepFolder = path;
	this->IsSavingSleepinessEvidence = true;
	return path;
}

void Utils::loadEnvFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Could not open .env file\n";
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		size_t delimiterPos = line.find('=');
		if (delimiterPos == std::string::npos) continue;

		std::string key = line.substr(0, delimiterPos);
		std::string value = line.substr(delimiterPos + 1);

		setEnvVar(key, value);
	}
}
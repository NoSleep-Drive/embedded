#include "../include/Utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

void setEnvVar(const std::string& key, const std::string& value) {
#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 1);
#endif
}

Utils::Utils(const std::string& saveDirectory) 
    : saveDirectory(saveDirectory), recentFolder("/recent"), sleepFolder("")
{
    if (!std::filesystem::exists(saveDirectory)) {
        std::filesystem::create_directories(saveDirectory);
    }

    if (!std::filesystem::exists(saveDirectory+recentFolder)) {
        std::filesystem::create_directories(saveDirectory+recentFolder);
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
    return cv::imwrite(path+"/"+name, frame);
}

bool Utils::saveFrameToCurrentFrameFolder(const cv::Mat& frame, const std::string& name) {
    return saveFrame(frame, saveDirectory + recentFolder, name);
}

bool Utils::saveFrameToSleepinessFolder(const cv::Mat& frame, const std::string& name) {
    if (sleepFolder.size() == 0) return false;
    return saveFrame(frame, saveDirectory + sleepFolder, name);
}

bool Utils::removeFolder(const std::string& folderName){
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

std::vector<cv::Mat> Utils::loadFramesFromRecentFolder(const std::string& timeStamp) {
    if (timeStamp.empty()) {
        std::cerr << "Error: Time stamp is empty, cannot load frames." << std::endl;
        return {};
    }

    std::string folderPath = saveDirectory + recentFolder;

    // timeStamp를 long long으로 변환 (년월일_시간분초_밀리초 -> 숫자 비교)
    long long timeStampNum = std::stoll(timeStamp);

    // 선택된 파일 리스트 (파일명, 경로)
    std::vector<std::pair<long long, std::string>> selectedFiles;

    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
            std::string fileName = entry.path().stem().string();	// 확장자 제외
            try {
                long long fileTime = std::stoll(fileName);
                if (fileTime <= timeStampNum+2500 && fileTime >= timeStampNum - 2500) {
                    selectedFiles.emplace_back(fileTime, entry.path().string());
                }
            }
            catch (const std::invalid_argument& e) {
                std::cerr << "Warning: Invalid file name encountered: " << fileName << std::endl;
                continue;
            }
        }
    }

    // 최신순으로 정렬 (파일명 숫자가 클수록 최신)
    std::sort(selectedFiles.begin(), selectedFiles.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    // 최대 MAX_SLEEPINESS_EVIDENCE_COUNT개의 프레임만 읽기
    std::vector<cv::Mat> frames;
    for (size_t i = 0;
        i < std::min(selectedFiles.size(), static_cast<size_t>(MAX_SLEEPINESS_EVIDENCE_COUNT));
        ++i) {
        cv::Mat img = cv::imread(selectedFiles[i].second);
        if (!img.empty()) {
            frames.push_back(img);
        }
    }

    return frames;
}

std::vector<std::pair<std::string, std::string>> Utils::getRecentFramePathsAndNames(
    const std::string& timeStamp) {
    std::vector<std::pair<std::string, std::string>> files;

    if (timeStamp.empty()) {
        std::cerr << "Error: Time stamp is empty, cannot load frames." << std::endl;
        return files;
    }

    std::string folderPath = saveDirectory + recentFolder;
    long long timeStampNum = Utils::parseFileTimeMillis(timeStamp);

    std::vector<std::pair<long long, std::filesystem::directory_entry>> selectedFiles;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
            std::string fileName = entry.path().stem().string();
            try {
                std::string fileName = entry.path().stem().string(); // "20250608_001047_555"
                long long fileTime = parseFileTimeMillis(fileName);

                if (fileTime == -1) continue;

                if (fileTime <= timeStampNum + 2500 && fileTime >= timeStampNum - 2500) {
                    selectedFiles.emplace_back(fileTime, entry);
                }
            }
            catch (const std::invalid_argument&) {
                continue;
            }
        }
    }

    std::sort(selectedFiles.begin(), selectedFiles.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    for (size_t i = 0;
        i < std::min(selectedFiles.size(), static_cast<size_t>(MAX_SLEEPINESS_EVIDENCE_COUNT));
        ++i) {
        files.emplace_back(selectedFiles[i].second.path().string(),
            selectedFiles[i].second.path().filename().string());
    }

    return files;
}

long long Utils::parseFileTimeMillis(const std::string& filename) {
    if (filename.size() < 18) return -1;  

    int year = std::stoi(filename.substr(0, 4));
    int month = std::stoi(filename.substr(4, 2));
    int day = std::stoi(filename.substr(6, 2));
    int hour = std::stoi(filename.substr(9, 2));
    int min = std::stoi(filename.substr(11, 2));
    int sec = std::stoi(filename.substr(13, 2));
    int millis = std::stoi(filename.substr(16, 3)); // ← 여기도 수정

    std::tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&t));
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count() + millis;
}

std::string Utils::createSleepinessDir(const std::string& timeStamp){
    std::string path = "/"+timeStamp;
    if (!std::filesystem::exists(saveDirectory+path)) {
        std::filesystem::create_directories(saveDirectory+path);
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
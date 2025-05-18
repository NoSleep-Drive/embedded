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

bool Utils::saveFrameToCurrentFrameFolder(const cv::Mat& frame, const std::string& name){
    if(sleepFolder.size() == 0) return false;
    return saveFrame(frame, saveDirectory+sleepFolder, name);
}

bool Utils::saveFrameToSleepinessFolder(const cv::Mat& frame, const std::string& name){
    return saveFrame(frame, saveDirectory+recentFolder, name);
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

std::vector<cv::Mat> Utils::loadFramesFromFolder(const std::string& folderName){
    std::vector<cv::Mat> frames;
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(saveDirectory+"/"+folderName)) {
        if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
            entries.push_back(entry);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.path().string() < b.path().string();
    });
    for (const auto& entry : entries) {
        cv::Mat img = cv::imread(entry.path().string());
        if (!img.empty()) {
            frames.push_back(img);
        }
    }

    return frames;
}

std::vector<cv::Mat> Utils::loadFramesFromRecentFolder() {
    return loadFramesFromFolder(recentFolder);
}

std::string Utils::createSleepinessDir(const std::string& timeStamp){
    std::string path = "/"+timeStamp;
    if (!std::filesystem::exists(saveDirectory+path)) {
        std::filesystem::create_directories(saveDirectory+path);
    }

    this->sleepFolder = path;
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
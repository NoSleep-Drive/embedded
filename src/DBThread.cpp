#include "../include/DBThread.h"
#include "../include/DBThreadMonitoring.h"
#include <vector>
#include <iostream>
#include <filesystem>
#include <thread>

DBThread::DBThread(const std::string& uid, const std::string& folder, DBThreadMonitoring* monitor)
    : deviceUid(uid), folderPath(folder), monitoring(monitor) {
    time = std::filesystem::last_write_time(folderPath);
}

void DBThread::deleteFolderSafe(const std::string& path) {
    try {
        std::filesystem::remove_all(path);
        std::cout << "[DBThread] 폴더 삭제 성공: " << path << std::endl;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[DBThread] 폴더 삭제 실패: " << e.what() << std::endl;
    }
}

bool DBThread::sendDataToDB() {
    VideoEncoder encoder;
    std::cout << "백서버 통신 전 이미지 데이터들을 영상 데이터로 변환" << std::endl;

    std::vector<uchar> videoData = encoder.convertFramesToMP4(folderPath);
    if (videoData.empty()) {
        std::cerr << "영상 생성 실패로 영상 전송 통신 취소." << std::endl;
        setIsDBThreadRunningFalse();
        return false;
    }

    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 1000;
    bool backendResponse = false;
    int attempt = 0;

    while (!backendResponse && attempt < MAX_RETRIES) {
        std::cout << "백엔드 서버 통신 " << (attempt + 1) << " 번째 시도" << std::endl;
        // TODO: 실제 백엔드 전송 로직 필요
        backendResponse = true; // 임시로 성공으로 가정

        if (backendResponse) {
            std::cout << "백엔드 영상 저장 성공, 로컬 폴더 삭제 : " << folderPath << std::endl;
            deleteFolderSafe(folderPath);
            setIsDBThreadRunningFalse();
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        attempt++;
    }

    std::cerr << "백엔드 전송 실패, 로컬 폴더 삭제 : " << folderPath << std::endl;
    deleteFolderSafe(folderPath);

    setIsDBThreadRunningFalse();
    return false;
}

void DBThread::setIsDBThreadRunningFalse() {
    if (monitoring) {
        monitoring->setIsDBThreadRunning(false);
    }
}

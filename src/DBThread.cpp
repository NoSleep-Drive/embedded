#include "../include/DBThread.h"
#include "../include/DBThreadMonitoring.h"
#include <iostream>
#include <filesystem>
#include <thread>

DBThread::DBThread(const std::string& uid, const std::string& folder, DBThreadMonitoring* monitor) 
    : deviceUid(uid), folderPath(folder), monitoring(monitor) {
    time = std::filesystem::last_write_time(folderPath);
}

bool DBThread::sendDataToDB() {
    VideoEncoder encoder;
    std::cout << "백서버 통신 전 이미지 데이터들을 영상 데이터로 변환" << std::endl;
    // TODO: 파일 경로 기준으로 5초간의 이미지를 추적 -> 임베디드 파일 시스템 경로에 따라 로직 수정 예정
    encoder.convertFramesToMP4(folderPath);

    std::cout << "DB 저장 요청을 보낼 라즈베리 파이 UID: " << deviceUid << std::endl;

    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 1000; // 1초

    bool backendResponse = false;
    int attempt = 0;

    while (!backendResponse && attempt < MAX_RETRIES) {
        std::cout << "백엔드 서버 통신 " << (attempt + 1) << " 번째 시도" << std::endl;
        // TODO: 백엔드 서버와 통신 로직 필요
        // 영상 저장이 잘 되었다는 응답을 받으면 backendResponse true 할당
        backendResponse = true;

        if (backendResponse) {
            std::cout << "백엔드 영상 저장 성공, 로컬 폴더 삭제 : " << folderPath << std::endl;
            std::filesystem::remove_all(folderPath);
            return true;
        }
        else {
            std::cout << "백엔드 영상 저장 실패, 통신 재시도 " << RETRY_DELAY_MS << " ms..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
            attempt++;
        }
    }

    setIsDBThreadRunningFalse();
    return false;
}

void DBThread::setIsDBThreadRunningFalse() {
    if (monitoring) {
        monitoring->setIsDBThreadRunning(false);
    }
}

#include "../include/DBThread.h"
#include "../include/DBThreadMonitoring.h"
#include <opencv2/opencv.hpp>
#include <vector>
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

    std::vector<uchar> videoData = encoder.convertFramesToMP4(folderPath);
    if (videoData.empty()) {
        std::cerr << "영상 생성 실패로 영상 전송 통신 취소." << std::endl;
        setIsDBThreadRunningFalse();
        return false;
    }

    std::cout << "DB 저장 요청을 보낼 라즈베리 파이 UID: " << deviceUid << std::endl;

    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 1000;

    bool backendResponse = false;
    int attempt = 0;

    struct ScopedMonitor {
        DBThread* thread;
        ~ScopedMonitor() {
            if (thread) {
                thread->setIsDBThreadRunningFalse();
                std::cout << "DBThread 종료: 모니터링 상태 false로 설정됨" << std::endl;
            }
        }
    } monitorGuard{ this };

    try {
        while (!backendResponse && attempt < MAX_RETRIES) {
            std::cout << "백엔드 서버 통신 " << (attempt + 1) << " 번째 시도" << std::endl;
            // TODO: 실제 백엔드 전송 로직 필요 (예: HTTP 전송)
            backendResponse = true; // 임시로 성공으로 가정

            if (backendResponse) {
                std::cout << "백엔드 영상 저장 성공, 로컬 폴더 삭제 : " << folderPath << std::endl;
                // TODO: 파일 삭제 시 경로 수정 필요
                // std::filesystem::remove_all(folderPath);
                return true;
            }
            else {
                std::cout << "백엔드 영상 저장 실패, 통신 재시도 " << RETRY_DELAY_MS << " ms..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                attempt++;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "DBThread 예외 발생: " << e.what() << std::endl;
    }

    return false;
}


void DBThread::setIsDBThreadRunningFalse() {
    if (monitoring) {
        monitoring->setIsDBThreadRunning(false);
    }
}

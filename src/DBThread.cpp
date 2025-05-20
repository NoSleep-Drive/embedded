#include "../include/DBThread.h"
#include "../include/DBThreadMonitoring.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <cpr/cpr.h>
#include <ctime>

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

    bool success = sendVideoToBackend(videoData);

    if (success) {
        std::cout << "백엔드 영상 저장 성공, 로컬 폴더 삭제 : " << folderPath << std::endl;
        deleteFolderSafe(folderPath);
        setIsDBThreadRunningFalse();
        return true;
    }
    else {
        std::cerr << "백엔드 전송 실패, 로컬 폴더 삭제 : " << folderPath << std::endl;
        deleteFolderSafe(folderPath);
        setIsDBThreadRunningFalse();
        return false;
    }
}

void DBThread::setIsDBThreadRunningFalse() {
    if (monitoring) {
        monitoring->setIsDBThreadRunning(false);
    }
}

std::string DBThread::getDetectedAtFromFolder() const {
    std::string folderName = std::filesystem::path(folderPath).filename().string();
    std::tm folderTime = {};
    std::istringstream ss(folderName);
    ss >> std::get_time(&folderTime, "%Y%m%d_%H%M%S");

    if (ss.fail()) {
        std::cerr << "DBThread 폴더 이름에서 타임스탬프를 파싱할 수 없음: " << folderName << std::endl;
        return "";
    }

    char timeStr[30];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S", &folderTime);
    return std::string(timeStr);
}

bool DBThread::sendVideoToBackend(const std::vector<uchar>& videoData) {
    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 1000;
    bool backendResponse = false;
    int attempt = 0;

    while (!backendResponse && attempt < MAX_RETRIES) {
        std::cout << "백엔드 서버 통신 " << (attempt + 1) << " 번째 시도" << std::endl;

        std::string hash = getenv("EMBEDDED_HASH");
        std::string deviceUidEnv = getenv("DEVICE_UID");
        std::string serverIP = getenv("SERVER_IP");

        if (hash.empty() || deviceUidEnv.empty() || serverIP.empty()) {
            std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
            return false;
        }

        std::string detectedAt = getDetectedAtFromFolder();
        if (detectedAt.empty()) {
            std::cerr << "detectedAt 추출 실패" << std::endl;
            return false;
        }

        std::string tempVideoPath = (std::filesystem::temp_directory_path() / "temp_video.mp4").string();
        std::ofstream outFile(tempVideoPath, std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(videoData.data()), videoData.size());
        outFile.close();

        cpr::Header headers = {
            {"Authorization", "Bearer " + hash}
        };

        cpr::Multipart multipart{
            {"deviceUid", deviceUidEnv},
            {"detectedAt", detectedAt},
            {"videoFile", cpr::File{tempVideoPath, "video/mp4"}}
        };

        std::string url = serverIP + "/sleep";
        cpr::Response r = cpr::Post(cpr::Url{ url }, headers, multipart);

        if (r.status_code == 200 && r.text.find("졸음 감지 데이터가 저장되었습니다.") != std::string::npos) {
            backendResponse = true;
        }
        else {
            std::cerr << "백엔드 응답 실패 (" << r.status_code << "): " << r.error.message << "\n응답 본문: " << r.text << std::endl;
        }

        try {
            std::filesystem::remove(tempVideoPath);
        }
        catch (const std::exception& e) {
            std::cerr << "임시 영상 파일 삭제 실패: " << e.what() << std::endl;
        }

        if (backendResponse) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        attempt++;
    }

    return false;
}

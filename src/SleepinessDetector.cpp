#include "../include/SleepinessDetector.h"
#include "../include/EyeClosureQueueManagement.h"
#include <iostream>
#include <ctime>
#include <string>
#include <cstdlib> 
#include <opencv2/opencv.hpp>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>

SleepinessDetector::SleepinessDetector() {
    sleepImgPath = "./frames";
}

void SleepinessDetector::sendDriverFrame(const cv::Mat& frame) {
    // TODO: issue #22 AI Server 큐 저장 요청 로직과 merge 필요
}

bool SleepinessDetector::requestAIDetection(const std::string& uid, const std::string& requestTime) {
    std::cout << "AI Server 진단 요청하는 파이 UID : " << uid << " 및 요청 시각 : " << requestTime << std::endl;

    const char* uidC = std::getenv("DEVICE_UID");
    const char* ipC = std::getenv("AI_SERVER_IP");

    if (!uidC || !ipC) {
        std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
        return false;
    }

    std::string deviceUidEnv(uidC);
    std::string serverIP(ipC);


    std::string url = serverIP + "/diagnosis/drowiness?deviceUid=" + cpr::util::urlEncode(deviceUidEnv);

    cpr::Response r = cpr::Get(cpr::Url{ url });

    if (r.status_code != 200) {
        std::cerr << "서버 응답 실패 - 상태 코드: " << r.status_code << "\n본문: " << r.text << std::endl;
        return false;
    }

    try {
        nlohmann::json jsonResp = nlohmann::json::parse(r.text);

        if (jsonResp.contains("success") && jsonResp["success"] == true) {
            bool isDrowsy = jsonResp["isDrowsinessDrive"];
            std::string detectionTime = jsonResp["detectionTime"];

            std::cout << "AI 진단 성공 - 졸음 운전 여부: " << (isDrowsy ? "예" : "아니오")
                << ", 감지 시각: " << detectionTime << std::endl;

            return isDrowsy;
        }
        else {
            const auto& err = jsonResp["error"];
            std::cerr << "AI 서버 오류 - 코드: " << err["code"]
                << ", 메시지: " << err["message"]
                << ", 메서드: " << err["method"]
                << ", 상세: " << err["detail_message"] << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
        return false;
    }

    // TODO: 타임아웃 시 로컬 진단 getLocalDetection 필요
}

bool SleepinessDetector::getLocalDetection(EyeClosureQueueManagement& eyeManager) {
    return eyeManager.detectSleepiness();
}

void SleepinessDetector::updateBaseSleepImgPath(const std::string& path) {
    sleepImgPath = path;
    // TODO
}

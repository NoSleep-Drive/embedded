#include "../include/SleepinessDetector.h"

#include <cpr/cpr.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "../include/EyeClosureQueueManagement.h"

namespace {
std::string base64_encode(const std::string& input) {
	const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string encoded;
	int val = 0;
	int valb = -6;

	for (unsigned char c : input) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			encoded.push_back(chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}

	if (valb > -6) {
		encoded.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
	}

	while (encoded.size() % 4) {
		encoded.push_back('=');
	}

	return encoded;
}
}	 // namespace

SleepinessDetector::SleepinessDetector() {
	sleepImgPath = "./frames";
}

void SleepinessDetector::sendDriverFrame(const cv::Mat& frame) {
	static int frameIndex = 0;

	// 이미지 데이터 인코딩
	std::vector<uchar> buffer;
	cv::imencode(".jpg", frame, buffer);

	std::string base64Image = base64_encode(std::string(buffer.begin(), buffer.end()));

	const char* uidC = std::getenv("DEVICE_UID");
	const char* ipC = std::getenv("AI_SERVER_IP");

	if (!uidC || !ipC) {
		std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
		return;
	}

	std::string deviceUidEnv(uidC);
	std::string serverIP(ipC);

	// 요청 데이터 생성
	nlohmann::json jsonData = {
			{"deviceUid", deviceUidEnv}, {"frameIdx", frameIndex++}, {"driverFrame", base64Image}};

	// 요청 URL 생성
	std::string url = serverIP + "/api/save/frame";

	// API 요청
	std::thread([](std::string url, std::string data) {
		cpr::Post(cpr::Url{ url },
		cpr::Header{ {"Content-Type", "application/json"} },
		cpr::Body{ data });
		}, url, jsonData.dump()).detach();

}

bool SleepinessDetector::requestAIDetection(const std::string& uid, const std::string& requestTime) {
	std::cout << "AI Server 진단 요청하는 파이 UID : " << uid << " 및 요청 시각 : " << requestTime
						<< std::endl;

	const char* uidC = std::getenv("DEVICE_UID");
	const char* ipC = std::getenv("AI_SERVER_IP");

	if (!uidC || !ipC) {
		std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
		return false;
	}

	std::string deviceUidEnv(uidC);
	std::string serverIP(ipC);

	auto encodedSecure = cpr::util::urlEncode(deviceUidEnv);
	std::string encodedUid(encodedSecure.begin(), encodedSecure.end());

	std::string url = serverIP + "/diagnosis/drowiness?deviceUid=" + encodedUid;

	cpr::Response r = cpr::Get(
		cpr::Url{url},
		cpr::Timeout{ 3000 }
	);

	if (r.status_code != 200) {
		std::cerr << "서버 응답 실패 - 상태 코드: " << r.status_code << "\n본문: " << r.text
							<< std::endl;
		return false;
	}

	try {
		nlohmann::json jsonResp = nlohmann::json::parse(r.text);

		if (jsonResp.contains("success") && jsonResp["success"] == true) {
			bool isDrowsy = jsonResp["isDrowsinessDrive"];
			std::string detectionTime = jsonResp["detectionTime"];

			//std::cout << "AI 진단 성공 - 졸음 운전 여부: " << (isDrowsy ? "예" : "아니오")
			//					<< ", 감지 시각: " << detectionTime << std::endl;

			return isDrowsy;
		} else {
			const auto& err = jsonResp["error"];
			//std::cerr << "AI 서버 오류 - 코드: " << err["code"] << ", 메시지: " << err["message"]
			//					<< ", 메서드: " << err["method"] << ", 상세: " << err["detail_message"]
			//					<< std::endl;
			return false;
		}
	} catch (const std::exception& e) {
		//std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
		return false;
	}
}

bool SleepinessDetector::getLocalDetection(EyeClosureQueueManagement& eyeManager) {
	return eyeManager.detectSleepiness();
}

void SleepinessDetector::updateBaseSleepImgPath(const std::string& path) {
	// 기본 저장 경로 업데이트
	sleepImgPath = path;
	std::cout << "졸음 이미지 기본 경로 업데이트됨: " << path << std::endl;

	// 경로가 존재하지 않으면 생성
	if (!std::filesystem::exists(sleepImgPath)) {
		std::filesystem::create_directories(sleepImgPath);
		std::cout << "졸음 이미지 기본 경로 생성됨: " << path << std::endl;
	}
}
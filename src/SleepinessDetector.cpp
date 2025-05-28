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

	// 콜백 기반 비동기 요청
	cpr::PostCallback(
			[](cpr::Response r) {
				if (r.error) {
					// std::cerr << "통신 오류: " << r.error.message << std::endl;
				}
			},
			cpr::Url{url}, cpr::Header{{"Content-Type", "application/json"}}, cpr::Body{jsonData.dump()},
			cpr::Timeout{5000}	// 5초 타임아웃
	);
}

void SleepinessDetector::requestAIDetection(
		const std::string& uid, const std::string& requestTime,
		std::function<void(bool success, bool isDrowsy, const std::string& message)> callback) {
	std::cout << "AI Server 진단 요청하는 파이 UID : " << uid << " 및 요청 시각 : " << requestTime
						<< std::endl;

	const char* uidC = std::getenv("DEVICE_UID");
	const char* ipC = std::getenv("AI_SERVER_IP");

	if (!uidC || !ipC) {
		std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
		callback(false, false, "환경 변수 설정 오류");
		return;
	}

	std::string deviceUidEnv(uidC);
	std::string serverIP(ipC);

	auto encodedSecure = cpr::util::urlEncode(deviceUidEnv);
	std::string encodedUid(encodedSecure.begin(), encodedSecure.end());

	std::string url = serverIP + "/diagnosis/drowiness?deviceUid=" + encodedUid;

	// 동기 HTTP 요청 (2초 타임아웃)
	cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{2000});

	if (r.error) {
		std::cerr << "통신 오류: " << r.error.message << std::endl;
		callback(false, false, "통신 오류: " + r.error.message);
		return;
	}

	if (r.status_code != 200) {
		std::cerr << "서버 응답 실패 - 상태 코드: " << r.status_code << "\n본문: " << r.text
							<< std::endl;
		callback(false, false, "HTTP 오류: " + std::to_string(r.status_code));
		return;
	}

	try {
		nlohmann::json jsonResp = nlohmann::json::parse(r.text);

		if (jsonResp.contains("success") && jsonResp["success"] == true) {
			bool isDrowsy = jsonResp["isDrowsinessDrive"];
			std::string detectionTime = jsonResp["detectionTime"];
			std::cout << "AI 진단 성공 - 졸음 여부: " << (isDrowsy ? "예" : "아니오") << std::endl;
			callback(true, isDrowsy, "AI 진단 성공, 감지 시각: " + detectionTime);
		} else {
			const auto& err = jsonResp["error"];
			std::cerr << "AI 서버 오류 - 메시지: " << err["message"] << std::endl;
			callback(false, false, "AI 서버 오류: " + std::string(err["message"]));
		}
	} catch (const std::exception& e) {
		std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
		callback(false, false, "JSON 파싱 오류: " + std::string(e.what()));
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
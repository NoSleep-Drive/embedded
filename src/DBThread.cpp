#include "../include/DBThread.h"

#include <cpr/cpr.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include "../include/DBThreadMonitoring.h"

namespace {
std::string generateTempFilePath() {
	std::stringstream ss;
	ss << std::filesystem::temp_directory_path().string();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 15);
	ss << "temp_video_";
	for (int i = 0; i < 8; ++i) {
		ss << std::hex << dis(gen);
	}
	ss << ".mp4";

	return ss.str();
}
}	 // namespace

DBThread::DBThread(const std::string& uid, const std::string& folder, DBThreadMonitoring* monitor)
		: deviceUid(uid), folderPath(folder), monitoring(monitor) {
	time = std::filesystem::last_write_time(folderPath);
}

DBThread::~DBThread() {}

void DBThread::deleteFolderSafe(const std::string& path) {
	try {
		std::filesystem::remove_all(path);
		std::cout << "[DBThread] 폴더 삭제 성공: " << path << std::endl;
	} catch (const std::filesystem::filesystem_error& e) {
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
	} else {
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

		const char* hashC = std::getenv("EMBEDDED_HASH");
		const char* uidC = std::getenv("DEVICE_UID");
		const char* ipC = std::getenv("SERVER_IP");

		if (!hashC || !uidC || !ipC) {
			std::cerr << "환경 변수 설정 오류: 통신에 필요한 정보 누락" << std::endl;
			return false;
		}

		std::string hash(hashC);
		std::string deviceUidEnv(uidC);
		std::string serverIP(ipC);

		std::string detectedAt = getDetectedAtFromFolder();
		if (detectedAt.empty()) {
			std::cerr << "detectedAt 추출 실패" << std::endl;
			return false;
		}

		std::string tempVideoPath = generateTempFilePath();
		std::ofstream outFile(tempVideoPath, std::ios::binary);
		outFile.write(reinterpret_cast<const char*>(videoData.data()), videoData.size());
		outFile.close();

		cpr::Header headers = {{"Authorization", "Bearer " + hash}};

		cpr::Multipart multipart{{"deviceUid", deviceUidEnv},
														 {"detectedAt", detectedAt},
														 {"videoFile", cpr::File{tempVideoPath, "video/mp4"}}};

		std::string url = serverIP + "/sleep";
		cpr::Response r = cpr::Post(cpr::Url{url}, headers, multipart);

		if (r.status_code == 200 &&
				r.text.find("졸음 감지 데이터가 저장되었습니다.") != std::string::npos) {
			backendResponse = true;
		} else {
			std::cerr << "백엔드 응답 실패 (" << r.status_code << "): " << r.error.message
								<< "\n응답 본문: " << r.text << std::endl;
		}

		try {
			std::filesystem::remove(tempVideoPath);
		} catch (const std::exception& e) {
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
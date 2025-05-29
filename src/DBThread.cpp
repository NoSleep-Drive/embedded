#include "../include/DBThread.h"

#include <cpr/cpr.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

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
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();
	std::stringstream ss;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 15);

	ss << "temp_video_";
	for (int i = 0; i < 8; ++i) {
		ss << std::hex << dis(gen);
	}
	ss << ".mp4";

	// tempDir / filename
	std::filesystem::path tempFilePath = tempDir / ss.str();
	return tempFilePath.string();
}

// SHA256 해시 계산 함수
std::string calculateSHA256(const std::vector<uchar>& data) {
	EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
	if (mdctx == nullptr) {
		std::cerr << "EVP_MD_CTX_new 실패" << std::endl;
		return "";
	}

	if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
		std::cerr << "EVP_DigestInit_ex 실패" << std::endl;
		EVP_MD_CTX_free(mdctx);
		return "";
	}

	if (EVP_DigestUpdate(mdctx, data.data(), data.size()) != 1) {
		std::cerr << "EVP_DigestUpdate 실패" << std::endl;
		EVP_MD_CTX_free(mdctx);
		return "";
	}

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
		std::cerr << "EVP_DigestFinal_ex 실패" << std::endl;
		EVP_MD_CTX_free(mdctx);
		return "";
	}

	EVP_MD_CTX_free(mdctx);

	// 바이트를 16진수 문자열로 변환
	std::stringstream ss;
	for (unsigned int i = 0; i < hash_len; i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
	}

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

	// 폴더에 이미지 프레임 충분히 수집될 때까지 sleep
	std::this_thread::sleep_for(std::chrono::seconds(3));

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

	// 폴더 이름 형식: "20250529_124345_300" (년월일_시분초_밀리초)
	std::cout << "폴더 이름: " << folderName << std::endl;

	// 언더스코어로 분리하여 파싱
	std::vector<std::string> parts;
	std::stringstream ss(folderName);
	std::string part;

	while (std::getline(ss, part, '_')) {
		parts.push_back(part);
	}

	if (parts.size() != 3) {
		std::cerr << "DBThread 폴더 이름 형식이 올바르지 않음: " << folderName << std::endl;
		std::cerr << "예상 형식: 년월일_시분초_밀리초 (예: 20250529_124345_300)" << std::endl;
		return "";
	}

	std::string dateStr = parts[0];		 // 20250529
	std::string timeStr = parts[1];		 // 124345
	std::string millisStr = parts[2];	 // 300

	// 날짜와 시간 파싱
	std::tm folderTime = {};
	std::istringstream dateStream(dateStr + timeStr);
	dateStream >> std::get_time(&folderTime, "%Y%m%d%H%M%S");

	if (dateStream.fail()) {
		std::cerr << "DBThread 폴더 이름에서 타임스탬프를 파싱할 수 없음: " << folderName << std::endl;
		return "";
	}

	// 밀리초를 마이크로초로 변환 (3자리 -> 6자리)
	std::string microseconds = millisStr + "000";	 // 300 -> 300000

	// 날짜 형식 변환: "2025-05-12 12:06:13.000000"
	char timeBuffer[30];
	std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &folderTime);

	std::string result = std::string(timeBuffer) + "." + microseconds;
	std::cout << "변환된 날짜 형식: " << result << std::endl;

	return result;
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

		std::string checksum = calculateSHA256(videoData);
		if (checksum.empty()) {
			std::cerr << "체크섬 계산 실패" << std::endl;
			return false;
		}

		std::cout << "비디오 데이터 크기: " << videoData.size() << " bytes" << std::endl;
		std::cout << "체크섬: " << checksum << std::endl;
		std::cout << "감지 시각: " << detectedAt << std::endl;

		std::string tempVideoPath = generateTempFilePath();
		std::ofstream outFile(tempVideoPath, std::ios::binary);
		if (!outFile) {
			std::cerr << "임시 파일 열기 실패: " << tempVideoPath << std::endl;
			return false;
		}
		outFile.write(reinterpret_cast<const char*>(videoData.data()), videoData.size());
		outFile.close();

		std::ifstream checkFile(tempVideoPath, std::ios::binary | std::ios::ate);
		if (!checkFile) {
			std::cerr << "임시 파일 열기 실패 (검증용): " << tempVideoPath << std::endl;
			return false;
		}
		std::streamsize fileSize = checkFile.tellg();
		checkFile.close();
		if (fileSize <= 0) {
			std::cerr << "임시 파일 사이즈 0, 파일 손상 의심: " << tempVideoPath << std::endl;
			return false;
		}
		std::cout << "임시 파일 정상 생성, 크기: " << fileSize << " bytes" << std::endl;

		cpr::Header headers = {{"Authorization", "Bearer " + hash}};
		cpr::Multipart multipart{{"deviceUid", deviceUidEnv},
														 {"detectedAt", detectedAt},
														 {"videoFile", cpr::File{tempVideoPath, "video.mp4"}},
														 {"checksum", checksum}};

		cpr::Response r =
				cpr::Post(cpr::Url{serverIP + "/sleep"}, headers, multipart, cpr::Timeout{10000});

		std::cout << "응답 코드: " << r.status_code << std::endl;
		std::cout << "응답 메시지: " << r.text << std::endl;
		std::cout << "에러 메시지: " << r.error.message << std::endl;
		std::cout << "에러 코드: " << static_cast<int>(r.error.code) << std::endl;

		if (r.status_code == 201 &&
				r.text.find("졸음 감지 데이터가 저장되었습니다.") != std::string::npos) {
			backendResponse = true;
			std::cout << "백엔드 전송 성공!" << std::endl;
		} else if (r.status_code == 404) {
			std::cerr << "장치를 찾을 수 없음 (404): " << r.text << std::endl;
			break;
		} else if (r.status_code == 401) {
			std::cerr << "인증 실패 (401): " << r.text << std::endl;
			break;
		} else if (r.status_code == 400) {
			std::cerr << "잘못된 요청 데이터 (400): " << r.text << std::endl;
			break;
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

		if (r.status_code >= 400 && r.status_code < 500) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
		attempt++;
	}

	return false;
}

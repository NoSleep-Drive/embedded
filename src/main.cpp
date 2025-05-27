#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

// NumPy 중복 정의 방지
#define NO_IMPORT_ARRAY

#include "../include/FirmwareManager.h"
#include "../include/Utils.h"

std::atomic<bool> running(true);

// SIGINT(Ctrl+C) 및 SIGTERM 핸들러
void signalHandler(int signum) {
	std::cout << "interrupt signal (" << signum << ") received. terminating..." << std::endl;
	running = false;
}

int main(int argc, char* argv[]) {
	// 시그널 핸들러 설정
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	std::cout << "NoSleep Drive 서비스 시작 중..." << std::endl;

	// 환경 변수 설정 로드
	Utils envLoader;
	envLoader.loadEnvFile(".env");

	// 장치 ID 설정 (환경 변수 또는 기본값)
	std::string deviceUID = "rasp-0001";
	const char* envUID = std::getenv("DEVICE_UID");
	if (envUID) {
		deviceUID = envUID;
	}

	// FirmwareManager 초기화
	std::unique_ptr<FirmwareManager> manager = std::make_unique<FirmwareManager>(deviceUID);

	try {
		// 시스템 시작
		manager->start();
		std::cout << "NoSleep Drive 시스템이 성공적으로 시작되었습니다." << std::endl;
		std::cout << "장치 ID: " << deviceUID << std::endl;

		// 메인 루프
		while (running) {
			// 메인 스레드에서는 아무 작업도 하지 않고 대기
			// 실제 작업은 FirmwareManager 내부 스레드에서 수행
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		// 정상 종료 처리
		std::cout << "시스템 종료 중..." << std::endl;
		manager->stop();
	} catch (const std::exception& e) {
		std::cerr << "오류 발생: " << e.what() << std::endl;
		return 1;
	}

	std::cout << "NoSleep Drive 서비스가 정상적으로 종료되었습니다." << std::endl;
	return 0;
}
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "../include/Speaker.h"

// 현재 디렉토리 경로 출력 함수
void printCurrentPath() {
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::cout << "Current working directory: " << currentPath.string() << std::endl;
}

// 특정 경로의 파일 존재 여부 확인 함수
void checkFileExists(const std::string& filePath) {
	if (std::filesystem::exists(filePath)) {
		std::cout << "✅ File exists: " << filePath << std::endl;
	} else {
		std::cout << "❌ File NOT found: " << filePath << std::endl;

		// 경로의 부모 디렉토리 확인
		std::filesystem::path path(filePath);
		std::filesystem::path parentPath = path.parent_path();

		if (!std::filesystem::exists(parentPath)) {
			std::cout << "   Parent directory does not exist: " << parentPath.string() << std::endl;
		} else {
			std::cout << "   Parent directory exists. Contents:" << std::endl;
			for (const auto& entry : std::filesystem::directory_iterator(parentPath)) {
				std::cout << "   - " << entry.path().filename().string() << std::endl;
			}
		}
	}
}

int runSpeakerTest(int argc, char* argv[]) {
	std::cout << "=== Speaker Class Test ===\n" << std::endl;

	// 현재 디렉토리 확인
	printCurrentPath();

	// 기본 알람 파일 경로
	std::string defaultSoundPath = "./sounds/alert.mp3";

	// 명령행 인수로 다른 파일 경로를 받을 수 있음
	if (argc > 1) {
		defaultSoundPath = argv[1];
	}

	std::cout << "Using sound file: " << defaultSoundPath << std::endl;

	// 파일 존재 여부 확인
	checkFileExists(defaultSoundPath);

	// VLC 설치 여부 확인
	std::cout << "\nChecking VLC installation..." << std::endl;
	int vlcResult = system("which cvlc");
	if (vlcResult != 0) {
		std::cout << "❌ VLC not found. Please install it using: sudo apt-get install vlc" << std::endl;
		return 1;
	} else {
		std::cout << "✅ VLC is installed" << std::endl;
	}

	// Speaker 객체 생성 및 초기화
	std::cout << "\nInitializing Speaker..." << std::endl;
	Speaker speaker(defaultSoundPath, 70);	// 기본 볼륨 70%
	speaker.initialize();

	if (!speaker.getConnectionStatus()) {
		std::cout << "❌ Speaker initialization failed" << std::endl;
		return 1;
	}

	std::cout << "✅ Speaker initialized successfully" << std::endl;

	// 현재 볼륨 표시
	std::cout << "\nCurrent volume: " << speaker.getVolume() << "%" << std::endl;

	// 테스트 메뉴
	while (true) {
		std::cout << "\n=== Speaker Test Menu ===" << std::endl;
		std::cout << "1. Play alert sound" << std::endl;
		std::cout << "2. Change volume" << std::endl;
		std::cout << "3. Change sound file" << std::endl;
		std::cout << "4. Exit" << std::endl;
		std::cout << "Enter your choice (1-4): ";

		int choice;
		std::cin >> choice;

		switch (choice) {
			case 1:
				std::cout << "\nPlaying alert sound..." << std::endl;
				speaker.triggerAlert();
				// 잠시 대기하여 소리 재생을 기다림
				std::this_thread::sleep_for(std::chrono::seconds(3));
				break;

			case 2: {
				int newVolume;
				std::cout << "\nEnter new volume (0-100): ";
				std::cin >> newVolume;

				if (newVolume < 0 || newVolume > 100) {
					std::cout << "Invalid volume. Please enter a value between 0 and 100." << std::endl;
				} else {
					speaker.setVolume(newVolume);
					std::cout << "Volume set to " << speaker.getVolume() << "%" << std::endl;
				}
				break;
			}

			case 3: {
				std::string newSoundFile;
				std::cout << "\nEnter new sound file path: ";
				std::cin.ignore();	// 버퍼 비우기
				std::getline(std::cin, newSoundFile);

				// 파일 존재 여부 확인
				checkFileExists(newSoundFile);

				// 새 사운드 파일 설정
				speaker.setAlert(newSoundFile, speaker.getVolume());
				std::cout << "Sound file changed to: " << newSoundFile << std::endl;
				break;
			}

			case 4:
				std::cout << "\nExiting Speaker test program." << std::endl;
				return 0;

			default:
				std::cout << "Invalid choice. Please enter a number between 1 and 4." << std::endl;
		}
	}

	return 0;
}
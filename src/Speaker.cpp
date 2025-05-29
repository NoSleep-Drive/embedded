#include "../include/Speaker.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

Speaker::Speaker(const std::string& soundFilePath, int vol)
		: Device(), alertSoundFilePath(soundFilePath), volume(vol) {}

Speaker::~Speaker() {}

void Speaker::initialize() {
	std::cout << "스피커 초기화 중..." << std::endl;

	// VLC 플레이어 설치 확인
	int result = system("which cvlc > /dev/null 2>&1");

	if (result != 0) {
		std::cerr << "Error: VLC player not found. Please install it using: sudo apt-get install vlc"
							<< std::endl;
		setConnectionStatus(false);
		updateDeviceStatus(2, false);	 // 스피커는 인덱스 2
	} else {
		// 사운드 파일이 존재하는지 확인
		if (!std::filesystem::exists(alertSoundFilePath)) {
			std::cerr << "Warning: Alert sound file not found at: " << alertSoundFilePath << std::endl;
			std::cerr << "Speaker will be marked as connected, but alert may not work" << std::endl;
		}

		setConnectionStatus(true);
		updateDeviceStatus(2, true);	// 스피커는 인덱스 2
		std::cout << "Speaker initialized successfully with VLC player" << std::endl;
	}
}

void Speaker::setAlert(const std::string& soundFile, int vol) {
	alertSoundFilePath = soundFile;
	volume = vol;
}

void Speaker::triggerAlert() {
	playSound(alertSoundFilePath);
}

void Speaker::triggerStart() {
	playSound(startSountFilePath);
}

void Speaker::playSound(const std::string& soundFile) const {
	if (!getConnectionStatus()) {
		std::cerr << "Cannot trigger sound: Speaker not connected" << std::endl;
		return;
	}

	// 파일이 존재하는지 확인
	if (!std::filesystem::exists(soundFile)) {
		std::cerr << "Error: Sound sound file not found at: " << soundFile << std::endl;
		return;
	}

	std::cout << "Playing sound sound: " << soundFile << " at volume " << volume << "%" << std::endl;

	// VLC 명령어 구성
	// cvlc: 콘솔 모드 VLC (GUI 없음)
	// --play-and-exit: 재생 후 자동 종료
	// --no-loop: 반복 재생 없음
	// --volume=NNN: 볼륨 설정 (0-512, 100%는 256)
	// --no-video: 비디오 출력 없음
	// >/dev/null 2>&1 &: 출력 무시하고 백그라운드로 실행

	int vlcVolume = volume * 256 / 100;	 // VLC 볼륨은 0-512 범위 (100%는 256)
	std::string command =
			"cvlc --play-and-exit --no-loop --gain=" + std::to_string(vlcVolume / 256.0) +
			" --no-video \"" + soundFile + "\" >/dev/null 2>&1 &";

	int result = system(command.c_str());

	if (result != 0) {
		std::cerr << "Failed to play sound. Error code: " << result << std::endl;
	} else {
		std::cout << "Sound triggered successfully" << std::endl;
	}
}

void Speaker::setVolume(int vol) {
	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;

	volume = vol;

	// 라즈베리파이 시스템 볼륨 설정
	std::string command = "amixer set Master " + std::to_string(volume) + "% -q";
	system(command.c_str());

	std::cout << "System volume set to " << volume << "%" << std::endl;
}

int Speaker::getVolume() const {
	return volume;
}
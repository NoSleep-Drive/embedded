#ifndef SPEAKER_H
#define SPEAKER_H

#include <string>

#include "Device.h"

class Speaker : public Device {
private:
	std::string alertSoundFilePath = "../sounds/alert.mp3";
	std::string startSountFilePath = "../sounds/start.mp3";
	int volume;

	void playSound(const std::string& soundFile) const;

public:
	Speaker(const std::string& soundFilePath = "../sounds/alert.mp3", int vol = 70);
	~Speaker();

	void initialize() override;

	// 경고음 설정 및 출력 메서드
	void setAlert(const std::string& soundFile, int vol);
	void triggerAlert();
	void triggerStart();

	// 볼륨 관련 메서드
	void setVolume(int vol);
	int getVolume() const;
};

#endif	// SPEAKER_H
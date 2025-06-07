#ifndef SPEAKER_H
#define SPEAKER_H

#include <string>

#include "Device.h"

class Speaker : public Device {
private:
	std::string alertSoundFilePath;
	int volume;

public:
	Speaker(const std::string& soundFilePath = "../../../sounds/alert.mp3", int vol = 70);
	~Speaker();

	void initialize() override;

	// 경고음 설정 및 출력 메서드
	void setAlert(const std::string& soundFile, int vol);
	void triggerAlert();

	// 볼륨 관련 메서드
	void setVolume(int vol);
	int getVolume() const;
};

#endif	// SPEAKER_H
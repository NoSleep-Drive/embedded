#include "../include/Speaker.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "Winmm.lib")
#endif

Speaker::Speaker(const std::string& soundFilePath, int vol)
    : Device(), alertSoundFilePath(soundFilePath), volume(vol) {}

Speaker::~Speaker() {}

void Speaker::initialize() {
#ifdef _WIN32
    if (!std::filesystem::exists(alertSoundFilePath)) {
        std::cerr << "Warning: Sound file not found: " << alertSoundFilePath << std::endl;
        setConnectionStatus(false);
        updateDeviceStatus(2, false);
    }
    else {
        setConnectionStatus(true);
        updateDeviceStatus(2, true);
        std::cout << "Speaker initialized successfully (Windows)" << std::endl;
    }
#else
    int result = system("which cvlc > /dev/null 2>&1");

    if (result != 0) {
        std::cerr << "Error: VLC player not found. Install with: sudo apt install vlc" << std::endl;
        setConnectionStatus(false);
        updateDeviceStatus(2, false);
    }
    else {
        if (!std::filesystem::exists(alertSoundFilePath)) {
            std::cerr << "Warning: Sound file not found: " << alertSoundFilePath << std::endl;
        }

        setConnectionStatus(true);
        updateDeviceStatus(2, true);
        std::cout << "Speaker initialized successfully with VLC (Linux)" << std::endl;
    }
#endif

    sendDeviceStatus();
}

void Speaker::setAlert(const std::string& soundFile, int vol) {
    alertSoundFilePath = soundFile;
    volume = vol;
}

void Speaker::triggerAlert() {
    if (!getConnectionStatus()) {
        std::cerr << "Speaker not connected" << std::endl;
        return;
    }

    if (!std::filesystem::exists(alertSoundFilePath)) {
        std::cerr << "Sound file not found: " << alertSoundFilePath << std::endl;
        return;
    }

#ifdef _WIN32
    std::cout << "Playing alert (Windows): " << alertSoundFilePath << std::endl;
    PlaySound(alertSoundFilePath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
#else
    int vlcVolume = volume * 256 / 100;
    std::string command =
        "cvlc --play-and-exit --no-loop --gain=" + std::to_string(vlcVolume / 256.0) +
        " --no-video \"" + alertSoundFilePath + "\" >/dev/null 2>&1 &";
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Failed to play alert. Error code: " << result << std::endl;
    }
    else {
        std::cout << "Alert triggered (Linux)" << std::endl;
    }
#endif
}

void Speaker::setVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;

    volume = vol;

#ifdef _WIN32
    std::cout << "Volume updated (Windows): " << volume << "%" << std::endl;
#else
    std::string command = "amixer set Master " + std::to_string(volume) + "% -q";
    system(command.c_str());
    std::cout << "System volume set to " << volume << "%" << std::endl;
#endif
}

int Speaker::getVolume() const {
    return volume;
}

#include "../include/Utils.h"
#include "../include/Camera.h"
#include <cpr/cpr.h>

int main() {
	Utils test;
	test.loadEnvFile("../../../.env");
	const char* saveDir = std::getenv("SERVER_IP");
	std::cout << "SERVER_IP: " << saveDir << std::endl;

	Camera one;
	one.setCameraStatus(false);
	one.sendDeviceStatus();
}
#include "../include/Utils.h"

int main() {
	Utils test;
	test.loadEnvFile("../../../.env");
	const char* saveDir = std::getenv("SERVER_IP");
	std::cout << "SERVER_IP: " << saveDir << std::endl;
}
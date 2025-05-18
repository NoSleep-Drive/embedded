#include "../include/Utils.h"

int main() {
	Utils test;
	std::vector<cv::Mat> data = test.loadFramesFromRecentFolder();

	std::string path = test.createSleepinessDir("test1234");
	int count = 0;

	for (const cv::Mat& var : data) {
		std::string name = std::to_string(count++) + ".jpg";
		test.saveFrameToSleepinessFolder(var, name);
		test.saveFrameToCurrentFrameFolder(var, name);
	}

	// test.removeFolder(path);

	return 0;
}
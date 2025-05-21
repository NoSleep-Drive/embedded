#include <iostream>

#include "../include/Utils.h"

int runUtilsTest() {
	std::cout << "프로그램 시작..." << std::endl;

	Utils test;
	std::cout << "Utils 객체 생성 완료" << std::endl;

	std::cout << "recent 폴더에서 프레임 로딩 시도 중..." << std::endl;
	std::vector<cv::Mat> data = test.loadFramesFromRecentFolder();
	std::cout << "로딩된 프레임 수: " << data.size() << std::endl;

	std::cout << "새 디렉토리 생성 중..." << std::endl;
	std::string path = test.createSleepinessDir("test1234");
	std::cout << "생성된 디렉토리 경로: " << path << std::endl;

	int count = 0;
	std::cout << "프레임 저장 시작..." << std::endl;
	for (const cv::Mat& var : data) {
		std::string name = std::to_string(count) + ".jpg";
		std::cout << "프레임 " << count << " 처리 중..." << std::endl;

		bool success1 = test.saveFrameToSleepinessFolder(var, name);
		std::cout << "sleepiness 폴더에 저장: " << (success1 ? "성공" : "실패") << std::endl;

		bool success2 = test.saveFrameToCurrentFrameFolder(var, name);
		std::cout << "current 폴더에 저장: " << (success2 ? "성공" : "실패") << std::endl;

		count++;
	}
	std::cout << "총 " << count << "개의 프레임 처리 완료" << std::endl;

	bool removed = test.removeFolder(path);
	std::cout << "폴더 삭제: " << (removed ? "성공" : "실패") << std::endl;

	std::cout << "프로그램 종료" << std::endl;
	return 0;
}
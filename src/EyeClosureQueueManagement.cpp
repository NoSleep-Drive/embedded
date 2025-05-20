#include "../include/EyeClosureQueueManagement.h"

#include <chrono>
#include <iostream>

EyeClosureQueueManagement::EyeClosureQueueManagement() {
	// 덱 초기화
	eyeClosureDeque.clear();
}

EyeClosureQueueManagement::~EyeClosureQueueManagement() {
	// 덱 비우기
	eyeClosureDeque.clear();
}

std::deque<bool> EyeClosureQueueManagement::getEyeClosureHistory() {
	return eyeClosureDeque;
}

bool EyeClosureQueueManagement::saveEyeClosureStatus(bool eyeClosed) {
	// 덱이 최대 크기에 도달했으면 가장 오래된 데이터 제거
	if (eyeClosureDeque.size() >= MAX_DEQUE_SIZE) {
		eyeClosureDeque.pop_front();
	}

	// 새 데이터 추가
	eyeClosureDeque.push_back(eyeClosed);

	return eyeClosed;
}

bool EyeClosureQueueManagement::detectSleepiness() {
	// 덱의 데이터가 CONSECUTIVE_FRAMES_FOR_SLEEPINESS보다 적으면 졸음이 아님
	if (eyeClosureDeque.size() < CONSECUTIVE_FRAMES_FOR_SLEEPINESS) {
		std::cout << "Deque size (" << eyeClosureDeque.size()
							<< ") is less than required consecutive frames (" << CONSECUTIVE_FRAMES_FOR_SLEEPINESS
							<< ") for sleepiness detection." << std::endl;
		return false;
	}

	// 가장 최근 프레임부터 역순으로 연속된 눈 감음 검사
	int consecutiveClosedCount = 0;

	// 덱을 반복하면서 연속된 눈 감음 상태 검사
	for (int i = eyeClosureDeque.size() - 1; i >= 0; i--) {
		if (eyeClosureDeque[i]) {
			consecutiveClosedCount++;

			// 연속된 눈 감음 프레임이 기준치에 도달하면 졸음으로 판단
			if (consecutiveClosedCount >= CONSECUTIVE_FRAMES_FOR_SLEEPINESS) {
				std::cout << "Detected sleepiness: " << consecutiveClosedCount
									<< " consecutive closed-eye frames." << std::endl;
				return true;
			}
		} else {
			// 눈 감음이 연속되지 않으면 카운트 리셋
			consecutiveClosedCount = 0;
		}
	}

	std::cout << "No sleepiness detected. Maximum consecutive closed-eye frames: "
						<< consecutiveClosedCount << std::endl;
	return false;
}
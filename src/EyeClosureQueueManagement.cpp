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

void EyeClosureQueueManagement::saveEyeClosureStatus(bool eyeClosed) {
	// 덱이 최대 크기에 도달했으면 가장 오래된 데이터 제거
	if (eyeClosureDeque.size() >= MAX_DEQUE_SIZE) {
		eyeClosureDeque.pop_front();
	}

	// 새 데이터 추가
	eyeClosureDeque.push_back(eyeClosed);
}

bool EyeClosureQueueManagement::detectSleepiness() {
	if (eyeClosureDeque.size() < CONSECUTIVE_FRAMES_FOR_SLEEPINESS) {
		return false;
	}

	int consecutiveClosedFrames = 0;
	int maxConsecutiveClosedFrames = 0;

	// deque의 뒤에서부터 앞으로 순회하면서 현재 연속된 감김 프레임 수를 계산
	for (auto it = eyeClosureDeque.rbegin(); it != eyeClosureDeque.rend(); ++it) {
		if (*it) {	// 눈이 감긴 상태
			consecutiveClosedFrames++;
			maxConsecutiveClosedFrames = std::max(maxConsecutiveClosedFrames, consecutiveClosedFrames);
		} else {	// 눈이 열린 상태
			break;	// 연속성이 깨졌으므로 중단
		}
	}

	std::cout << "Current consecutive closed frames from end: " << consecutiveClosedFrames
						<< std::endl;

	// 현재 시점에서 연속으로 감긴 프레임이 임계값 이상인지 확인
	return consecutiveClosedFrames >= CONSECUTIVE_FRAMES_FOR_SLEEPINESS;
}
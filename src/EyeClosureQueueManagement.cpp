#include "../include/EyeClosureQueueManagement.h"

#include <chrono>
#include <iostream>

EyeClosureQueueManagement::EyeClosureQueueManagement() {
	// 큐 초기화
	while (!eyeClosureQueue.empty()) {
		eyeClosureQueue.pop();
	}
}

EyeClosureQueueManagement::~EyeClosureQueueManagement() {
	// 큐 비우기
	while (!eyeClosureQueue.empty()) {
		eyeClosureQueue.pop();
	}
}

std::queue<bool> EyeClosureQueueManagement::getEyeClosureHistory() {
	return eyeClosureQueue;
}

bool EyeClosureQueueManagement::saveEyeClosureStatus(bool eyeClosed) {
	// 큐가 최대 크기에 도달했으면 가장 오래된 데이터 제거
	if (eyeClosureQueue.size() >= MAX_QUEUE_SIZE) {
		eyeClosureQueue.pop();
	}

	// 새 데이터 추가
	eyeClosureQueue.push(eyeClosed);

	return eyeClosed;
}

bool EyeClosureQueueManagement::detectSleepiness() {
	// 큐의 데이터가 CONSECUTIVE_FRAMES_FOR_SLEEPINESS보다 적으면 졸음이 아님
	if (eyeClosureQueue.size() < CONSECUTIVE_FRAMES_FOR_SLEEPINESS) {
		return false;
	}

	// 큐의 데이터를 벡터로 복사 (큐는 앞쪽의 요소만 접근 가능하기 때문)
	std::vector<bool> eyeClosureHistory;
	std::queue<bool> tempQueue = eyeClosureQueue;	 // 원본 큐 보존을 위한 복사

	while (!tempQueue.empty()) {
		eyeClosureHistory.push_back(tempQueue.front());
		tempQueue.pop();
	}

	// 가장 최근 프레임부터 역순으로 연속된 눈 감음 검사
	int consecutiveClosedCount = 0;
	for (int i = eyeClosureHistory.size() - 1; i >= 0; i--) {
		if (eyeClosureHistory[i]) {
			consecutiveClosedCount++;

			// 연속된 눈 감음 프레임이 기준치에 도달하면 졸음으로 판단
			if (consecutiveClosedCount >= CONSECUTIVE_FRAMES_FOR_SLEEPINESS) {
				return true;
			}
		} else {
			// 눈 감음이 연속되지 않으면 카운트 리셋
			consecutiveClosedCount = 0;
		}
	}

	return false;
}
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

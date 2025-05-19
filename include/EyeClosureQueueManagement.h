#ifndef EYE_CLOSURE_QUEUE_MANAGEMENT_H
#define EYE_CLOSURE_QUEUE_MANAGEMENT_H

#include <opencv2/opencv.hpp>
#include <queue>

class EyeClosureQueueManagement {
private:
	std::queue<bool> eyeClosureQueue;
	static const int MAX_QUEUE_SIZE = 60;	 // 최대 큐 사이즈 (약 2.5초)

public:
	EyeClosureQueueManagement();
	~EyeClosureQueueManagement();

	// 큐 관리 메소드
	std::queue<bool> getEyeClosureHistory();

	// 눈 감음 상태 저장
	bool saveEyeClosureStatus(bool eyeClosed);
};

#endif	// EYE_CLOSURE_QUEUE_MANAGEMENT_H
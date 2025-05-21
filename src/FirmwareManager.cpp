#include "../include/FirmwareManager.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <thread>

#include "../include/DBThread.h"

// NumPy 배열 초기화를 위한 헬퍼 함수
bool initializePythonAndNumpy() {
	Py_Initialize();

	// NumPy 배열 초기화
	import_array1(false);	 // 실패 시 false 반환

	PyRun_SimpleString(
			"import sys; sys.path.append('.'); sys.path.append('..'); sys.path.append('../python')");

	return true;
}

FirmwareManager::FirmwareManager(const std::string& uid)
		: deviceUID(uid), isRunning(false), isPaused(false), frameCycle(0), diagnosticCycle(0) {
	// 객체들 초기화
	camera = std::make_unique<Camera>();
	accelerationSensor = std::make_unique<AccelerationSensor>(true);	// 목업 센서 사용
	speaker = std::make_unique<Speaker>();														// Speaker 클래스 필요
	// sleepinessDetector = std::make_unique<SleepinessDetector>();	// SleepinessDetector 클래스 필요
	eyeClosureQueue = std::make_unique<EyeClosureQueueManagement>();
	utils = std::make_unique<Utils>("./frames");
	threadMonitor = std::make_unique<DBThreadMonitoring>();

	// Python 및 NumPy 초기화
	if (!initializePythonAndNumpy()) {
		std::cerr << "Failed to initialize Python/NumPy" << std::endl;
	}

	// Python 모듈 로드
	PyObject* pModule = PyImport_ImportModule("eye_detection_lib");
	if (pModule == nullptr) {
		PyErr_Print();
		std::cerr << "Failed to import eye_detection_lib module" << std::endl;
	} else {
		// 초기화 함수 호출
		PyObject* pFunc = PyObject_GetAttrString(pModule, "initialize");
		if (pFunc != nullptr && PyCallable_Check(pFunc)) {
			PyObject* pValue = PyObject_CallObject(pFunc, nullptr);
			if (pValue != nullptr) {
				bool result = PyObject_IsTrue(pValue);
				if (result) {
					std::cout << "Python eye detection initialized successfully" << std::endl;
				} else {
					std::cerr << "Python eye detection initialization failed" << std::endl;
				}
				Py_DECREF(pValue);
			}
			Py_DECREF(pFunc);
		}
		Py_DECREF(pModule);
	}

	std::cout << "FirmwareManager initialized with UID: " << deviceUID << std::endl;
}

FirmwareManager::~FirmwareManager() {
	stop();

	// Python 종료
	Py_Finalize();

	std::cout << "FirmwareManager destroyed" << std::endl;
}

void FirmwareManager::initializeDevices() {
	std::cout << "Initializing devices..." << std::endl;

	// 장치 초기화
	camera->initialize();
	accelerationSensor->initialize();
	// speaker->initialize();

	std::cout << "All devices initialized" << std::endl;
}

void FirmwareManager::start() {
	if (isRunning.load()) {
		std::cout << "FirmwareManager is already running" << std::endl;
		return;
	}

	// 장치 초기화
	initializeDevices();

	isRunning.store(true);
	isPaused.store(false);

	// 메인 루프 스레드 시작
	mainThread = std::thread(&FirmwareManager::mainLoop, this);

	std::cout << "FirmwareManager started" << std::endl;
}

void FirmwareManager::stop() {
	if (!isRunning.load()) {
		return;
	}

	isRunning.store(false);

	if (mainThread.joinable()) {
		mainThread.join();
	}

	std::cout << "FirmwareManager stopped" << std::endl;
}

void FirmwareManager::pause() {
	isPaused.store(true);
	std::cout << "FirmwareManager paused" << std::endl;
}

void FirmwareManager::resume() {
	isPaused.store(false);
	std::cout << "FirmwareManager resumed" << std::endl;
}

std::string FirmwareManager::getDeviceUID() const {
	return deviceUID;
}

bool FirmwareManager::isDeviceRunning() const {
	return isRunning.load();
}

void FirmwareManager::mainLoop() {
	std::cout << "Main loop started" << std::endl;

	const auto frameInterval = std::chrono::microseconds(42000);	// 0.042초 (42ms)
	auto lastFrameTime = std::chrono::high_resolution_clock::now();

	while (isRunning.load()) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto elapsedTime = currentTime - lastFrameTime;

		// 0.042초마다 프레임 처리
		if (elapsedTime >= frameInterval) {
			lastFrameTime = currentTime;

			// 차량이 움직이고 있지 않으면 처리하지 않음
			if (!accelerationSensor->isMoving()) {
				// 차량이 정차 중일 때 처리 로직
				// 남은 영상 데이터를 서버에 전송하는 코드 등
				std::cout << "Vehicle is not moving, skipping processing" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			// 일시 중지 상태면 처리하지 않음
			if (isPaused.load()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// 프레임 처리
			processSingleFrame();

			// 24 주기마다 진단 요청
			frameCycle++;
			if (frameCycle >= 24) {
				frameCycle = 0;
				requestDiagnosis();
				diagnosticCycle++;
			}
		} else {
			// CPU 점유율 감소를 위해 짧은 시간 대기
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::cout << "Main loop ended" << std::endl;
}

bool FirmwareManager::processSingleFrame() {
	// 1. 카메라에서 프레임 가져오기
	cv::Mat frame = camera->captureFrame();
	if (frame.empty()) {
		std::cerr << "Error: Empty frame captured" << std::endl;
		return false;
	}

	// 2. 이미지 전처리
	cv::Mat lChannel, processed;
	try {
		// 2.1 LAB 변환 및 L 채널 추출
		cv::Mat lab;
		cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);
		std::vector<cv::Mat> labChannels(3);
		cv::split(lab, labChannels);
		lChannel = labChannels[0].clone();

		// 2.2 미디안 필터 적용
		cv::Mat medianL;
		cv::medianBlur(lChannel, medianL, 99);

		// 2.3 L 채널 반전
		cv::Mat invertedL;
		cv::bitwise_not(medianL, invertedL);

		// 2.4 그레이스케일 변환
		cv::Mat gray;
		cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

		// 2.5 그레이스케일과 반전된 L 채널 합성
		cv::addWeighted(gray, 0.75, invertedL, 0.25, 0, processed);
	} catch (const cv::Exception& e) {
		std::cerr << "OpenCV error during preprocessing: " << e.what() << std::endl;
		return false;
	}

	// 3. 눈 감음 판단 (Python 함수 호출)
	bool eyesClosed = false;
	PyGILState_STATE gstate = PyGILState_Ensure();
	try {
		PyObject* pModule = PyImport_ImportModule("eye_detection_lib");
		if (pModule != nullptr) {
			PyObject* pFunc = PyObject_GetAttrString(pModule, "is_eye_closed");
			if (pFunc != nullptr && PyCallable_Check(pFunc)) {
				// cv::Mat을 NumPy 배열로 변환
				npy_intp dims[3] = {processed.rows, processed.cols, processed.channels()};
				PyObject* pArray = PyArray_SimpleNewFromData(
						processed.channels() == 1 ? 2 : 3, dims,
						processed.depth() == CV_8U ? NPY_UINT8 : NPY_FLOAT32, processed.data);

				// 함수 인자 설정
				float threshold = 0.25f;
				PyObject* pThreshold = PyFloat_FromDouble(threshold);
				PyObject* pArgs = PyTuple_New(2);
				PyTuple_SetItem(pArgs, 0, pArray);
				PyTuple_SetItem(pArgs, 1, pThreshold);

				// 함수 호출
				PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
				Py_DECREF(pArgs);

				if (pValue != nullptr) {
					eyesClosed = PyObject_IsTrue(pValue);
					Py_DECREF(pValue);
				}

				Py_DECREF(pFunc);
			}
			Py_DECREF(pModule);
		}
	} catch (const std::exception& e) {
		std::cerr << "Error during eye detection: " << e.what() << std::endl;
	}
	PyGILState_Release(gstate);

	// 4. 눈 감음 상태 저장
	eyeClosureQueue->saveEyeClosureStatus(eyesClosed);

	// 5. 프레임 저장 (720p로 변환)
	cv::Mat resizedFrame;
	cv::resize(frame, resizedFrame, cv::Size(1280, 720));

	// 현재 시간을 파일명으로 사용하여 최근 프레임 폴더에 저장
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S_%03d");
	std::string timestamp = ss.str();

	utils->saveFrameToSleepinessFolder(resizedFrame, timestamp + ".jpg");

	// TODO: AI 서버로 진단용 이미지 전송 (실제 구현 필요)
	// sleepinessDetector->sendDriverFrame(processed);

	return true;
}

bool FirmwareManager::requestDiagnosis() {
	std::cout << "Requesting sleepiness diagnosis (cycle " << diagnosticCycle << ")" << std::endl;

	bool isSleepy = false;

	// 1. 타임스탬프
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
	std::string timestamp = ss.str();

	// AI 서버 요청 (타임아웃 2.5초)
	bool aiResponse = false;
	// TODO: 추후 sleepinessDetector->requestAIDetection(deviceUID, timestamp) 호출하도록 수정

	if (!aiResponse) {
		// AI 서버 응답이 없을 경우 로컬 알고리즘으로 판단
		isSleepy = eyeClosureQueue->detectSleepiness();
	} else {
		// AI 서버 응답이 있을 경우 해당 결과 사용
	}

	// 졸음으로 판단되면 처리
	if (isSleepy) {
		std::cout << "SLEEPINESS DETECTED! Triggering alert..." << std::endl;

		// 1. 경고음 출력
		speaker->triggerAlert();

		// 2. 졸음 근거 영상 저장 폴더 생성
		std::string sleepDir = utils->createSleepinessDir(timestamp);

		// 3. 최근 프레임을 졸음 폴더로 복사 (실제 구현 필요)
		// utils->saveFrames(sleepDir, startIdx, endIdx);

		// 4. DB 전송 스레드 생성
		auto dbThread = std::make_shared<DBThread>(deviceUID, sleepDir, threadMonitor.get());
		threadMonitor->addDBThread(dbThread);
	}

	return true;
}
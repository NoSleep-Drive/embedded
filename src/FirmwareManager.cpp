#include "../include/FirmwareManager.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <thread>

#include "../include/DBThread.h"
#include "../include/SleepinessDetector.h"

// NumPy 배열 초기화를 위한 헬퍼 함수
bool FirmwareManager::initializePythonAndNumpy() {
	Py_Initialize();
	if (!Py_IsInitialized()) {
		std::cerr << "Failed to initialize Python interpreter" << std::endl;
		return false;
	}

	// NumPy 배열 초기화
	import_array1(false);

	PyRun_SimpleString(
			"import sys; sys.path.append('.'); sys.path.append('..'); sys.path.append('../python')");

	return true;
}

FirmwareManager::FirmwareManager(const std::string& uid)
		: deviceUID(uid), isRunning(false), isPaused(false), frameCycle(0), diagnosticCycle(0) {
	std::cout << "NoSleep Drive 펌웨어 매니저 초기화 중 (ID: " << uid << ")..." << std::endl;

	try {
		// 환경 변수에 장치 UID 설정
		setEnvVar("DEVICE_UID", deviceUID);

		// Python 및 NumPy 초기화
		std::cout << "Python 및 NumPy 초기화 중..." << std::endl;
		if (!initializePythonAndNumpy()) {
			std::cerr << "Python/NumPy 초기화 실패" << std::endl;
			throw std::runtime_error("Python/NumPy 초기화 실패");
		}

		// Python 모듈 로드 (눈 감음 감지 라이브러리)
		std::cout << "Python 모듈 로드 중..." << std::endl;
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

		// 객체들 초기화
		std::cout << "컴포넌트 객체들 초기화 중..." << std::endl;
		camera = std::make_unique<Camera>();
		accelerationSensor = std::make_unique<AccelerationSensor>(true);	// 목업 센서 사용
		speaker = std::make_unique<Speaker>();
		sleepinessDetector = std::make_unique<SleepinessDetector>();
		eyeClosureQueue = std::make_unique<EyeClosureQueueManagement>();
		utils = std::make_unique<Utils>("./frames");
		threadMonitor = std::make_unique<DBThreadMonitoring>();

		std::cout << "FirmwareManager initialized with device UID: " << deviceUID << std::endl;
		std::cout << "NoSleep Drive 펌웨어 매니저 초기화 완료" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "FirmwareManager 초기화 중 예외 발생: " << e.what() << std::endl;
		throw;
	}
}

FirmwareManager::~FirmwareManager() {
	std::cout << "FirmwareManager 소멸자 시작" << std::endl;
	stop();

	// Python 종료
	if (Py_IsInitialized()) {
		Py_Finalize();
	}

	std::cout << "FirmwareManager destroyed" << std::endl;
}

void FirmwareManager::initializeDevices() {
	std::cout << "Initializing devices..." << std::endl;

	try {
		// 장치 초기화
		camera->initialize();
		accelerationSensor->initialize();
		speaker->initialize();

		// 장치 상태 확인
		if (!camera->getConnectionStatus()) {
			std::cerr << "경고: 카메라가 제대로 초기화되지 않았습니다." << std::endl;
		}

		if (!accelerationSensor->getConnectionStatus()) {
			std::cerr << "경고: 가속도 센서가 제대로 초기화되지 않았습니다." << std::endl;
		}

		if (!speaker->getConnectionStatus()) {
			std::cerr << "경고: 스피커가 제대로 초기화되지 않았습니다." << std::endl;
		}

	} catch (const std::exception& e) {
		std::cerr << "장치 초기화 중 오류 발생: " << e.what() << std::endl;
		throw;
	}

	std::cout << "모든 장치 초기화 완료" << std::endl;
}

void FirmwareManager::start() {
	if (isRunning.load()) {
		std::cout << "FirmwareManager is already running" << std::endl;
		return;
	}

	try {
		// 장치 초기화
		initializeDevices();

		isRunning.store(true);
		isPaused.store(false);

		// 메인 루프 스레드 시작
		mainThread = std::thread(&FirmwareManager::mainLoop, this);

		std::cout << "FirmwareManager started" << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "FirmwareManager 시작 중 오류: " << e.what() << std::endl;
		throw;
	}
}

void FirmwareManager::stop() {
	if (!isRunning.load()) {
		return;
	}

	std::cout << "FirmwareManager 정지 중..." << std::endl;
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

	// 실시간 프레임 저장 폴더 생성
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
	std::string recentFolder = utils->createSleepinessDir(ss.str() + "_recent");

	while (isRunning.load()) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto elapsedTime = currentTime - lastFrameTime;

		// 0.042초마다 프레임 처리 (1주기)
		if (elapsedTime >= frameInterval) {
			lastFrameTime = currentTime;

			// 차량이 움직이고 있지 않으면 처리하지 않음
			if (!accelerationSensor->isMoving()) {
				// 차량이 정차 중일 때 처리 로직
				handleVehicleStopped();

				// 짧은 대기 후 다음 반복으로
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

			// 24 주기(1초)마다 진단 요청
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

void FirmwareManager::handleVehicleStopped() {
	// 차량이 정차 중일 때 처리 로직

	// 백엔드에 전송해야하는 졸음 근거 영상이 남아 있는 경우
	if (threadMonitor->getIsDBThreadRunning()) {
		std::cout << "차량 정차 감지: 졸음 근거 영상 전송 중..." << std::endl;
		// 스레드가 이미 실행 중이므로 추가 작업 없음
	} else {
		std::cout << "차량 정차 감지: 실시간 영상 데이터 삭제" << std::endl;
		// 실시간 영상을 저장하는 폴더 내의 모든 이미지 데이터 삭제
		utils->removeFolder("recent");
	}
}

bool FirmwareManager::processSingleFrame() {
	// 1. 카메라에서 프레임 가져오기
	cv::Mat frame = camera->captureFrame();
	if (frame.empty()) {
		std::cerr << "Error: Empty frame captured" << std::endl;
		return false;
	}

	// 2. 이미지 전처리
	cv::Mat preprocessedFrame;
	try {
		// 2.1 LAB 변환 및 L 채널 추출
		cv::Mat lab;
		cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);
		std::vector<cv::Mat> labChannels(3);
		cv::split(lab, labChannels);
		cv::Mat lChannel = labChannels[0].clone();

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
		cv::addWeighted(gray, 0.75, invertedL, 0.25, 0, preprocessedFrame);
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
				npy_intp dims[3];
				int nd;
				int typenum;

				if (preprocessedFrame.channels() == 1) {
					// 그레이스케일 이미지
					nd = 2;
					dims[0] = preprocessedFrame.rows;
					dims[1] = preprocessedFrame.cols;
					typenum = NPY_UINT8;
				} else {
					// 컬러 이미지
					nd = 3;
					dims[0] = preprocessedFrame.rows;
					dims[1] = preprocessedFrame.cols;
					dims[2] = preprocessedFrame.channels();
					typenum = NPY_UINT8;
				}

				PyObject* pArray = PyArray_SimpleNewFromData(nd, dims, typenum, preprocessedFrame.data);
				if (pArray == nullptr) {
					std::cerr << "Failed to create NumPy array" << std::endl;
					Py_DECREF(pModule);
					PyGILState_Release(gstate);
					return false;
				}

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
	ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	ss << '_' << std::setfill('0') << std::setw(3) << ms.count();
	std::string timestamp = ss.str();

	// 프레임 저장
	utils->saveFrameToCurrentFrameFolder(resizedFrame, timestamp + ".jpg");

	// 6. AI 서버로 이미지 전송
	sleepinessDetector->sendDriverFrame(preprocessedFrame);

	return true;
}

bool FirmwareManager::requestDiagnosis() {
	std::cout << "Requesting sleepiness diagnosis (cycle " << diagnosticCycle << ")" << std::endl;

	bool isSleepy = false;

	// 1. 타임스탬프 생성
	auto now = std::chrono::system_clock::now();
	auto now_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
	std::string timestamp = ss.str();

	// 2. AI 서버에 진단 요청
	bool aiResponse = false;
	try {
		aiResponse = sleepinessDetector->requestAIDetection(deviceUID, timestamp);

		// AI 서버 응답에 따라 졸음 판단
		if (aiResponse) {
			isSleepy = true;	// AI가 졸음으로 판단한 경우
			std::cout << "AI 서버가 졸음으로 판단했습니다." << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "AI 서버 진단 요청 중 오류: " << e.what() << std::endl;
		// 예외 발생 시 aiResponse = false 유지
	}

	// 3. AI 서버 응답이 없거나 통신 실패 시 로컬 알고리즘 사용
	if (!aiResponse) {
		std::cout << "AI 서버 응답 없음, 로컬 알고리즘으로 진단합니다." << std::endl;
		// 로컬 눈 감음 데이터 기반 졸음 여부 판단
		isSleepy = sleepinessDetector->getLocalDetection(*eyeClosureQueue);
	}

	// 4. 졸음으로 판단된 경우 처리
	if (isSleepy) {
		handleSleepinessDetected(timestamp);
	}

	return true;
}

void FirmwareManager::handleSleepinessDetected(const std::string& timestamp) {
	std::cout << "***** 졸음 감지! 알람 작동 *****" << std::endl;

	// 1. 경고음 출력
	speaker->triggerAlert();

	// 2. 졸음 근거 영상 저장 폴더 생성
	std::string sleepDir = utils->createSleepinessDir(timestamp);
	std::cout << "졸음 영상 저장 경로: " << sleepDir << std::endl;

	// 3. 현재까지의 프레임 저장
	std::vector<cv::Mat> recentFrames = utils->loadFramesFromRecentFolder();

	// 4. DB 전송 스레드 생성 및 큐에 추가
	auto dbThread = std::make_shared<DBThread>(deviceUID, sleepDir, threadMonitor.get());
	threadMonitor->addDBThread(dbThread);

	// 5. 스레드 실행 상태 설정
	threadMonitor->setIsDBThreadRunning(true);
}
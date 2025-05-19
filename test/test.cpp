#include <Python.h>
#include <numpy/arrayobject.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "../include/Camera.h"

bool removeLight(const cv::Mat& input, cv::Mat& lChannel, cv::Mat& composed) {
	std::cout << "\n----- Image Preprocessing Started -----" << std::endl;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto totalStartTime = startTime;

	try {
		// Print input image information
		std::cout << "Input image size: " << input.cols << "x" << input.rows << std::endl;
		std::cout << "Input image channels: " << input.channels() << std::endl;

		// 1. Convert to grayscale
		std::cout << "1. Starting grayscale conversion..." << std::endl;
		cv::Mat gray;
		cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

		auto stepTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;
		startTime = stepTime;

		// 2. Convert BGR to LAB color space
		std::cout << "2. Starting LAB color space conversion..." << std::endl;
		cv::Mat lab;
		cv::cvtColor(input, lab, cv::COLOR_BGR2Lab);

		stepTime = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;
		startTime = stepTime;

		// 3. Extract L channel (brightness channel)
		std::cout << "3. Starting L channel extraction..." << std::endl;
		std::vector<cv::Mat> labChannels(3);
		cv::split(lab, labChannels);
		lChannel = labChannels[0].clone();	// L channel

		stepTime = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;
		startTime = stepTime;

		// 4. Apply median filter (noise reduction) - Python code uses 99
		std::cout << "4. Starting median filter application (kernel size: 99)..." << std::endl;
		cv::Mat medianL;
		cv::medianBlur(lChannel, medianL, 99);

		stepTime = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;
		startTime = stepTime;

		// 5. Invert L channel
		std::cout << "5. Starting L channel inversion..." << std::endl;
		cv::Mat invertedL;
		cv::bitwise_not(medianL, invertedL);

		stepTime = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;
		startTime = stepTime;

		// 6. Composite grayscale with inverted L channel (weights 0.75:0.25)
		std::cout << "6. Starting image composition (gray:0.75, inverted:0.25)..." << std::endl;
		cv::addWeighted(gray, 0.75, invertedL, 0.25, 0, composed);

		stepTime = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - startTime);
		std::cout << "   Completed! (Time taken: " << duration.count() << "ms)" << std::endl;

		// Output image information and total processing time
		auto totalDuration =
				std::chrono::duration_cast<std::chrono::milliseconds>(stepTime - totalStartTime);
		std::cout << "Output image size: " << composed.cols << "x" << composed.rows << std::endl;
		std::cout << "Output image channels: " << composed.channels() << std::endl;
		std::cout << "Complete preprocessing completed! (Total time: " << totalDuration.count() << "ms)"
							<< std::endl;
		std::cout << "----- Image Preprocessing Completed -----\n" << std::endl;

		return true;
	} catch (const cv::Exception& e) {
		std::cerr << "OpenCV Error during preprocessing: " << e.what() << std::endl;
		return false;
	} catch (const std::exception& e) {
		std::cerr << "Standard Error during preprocessing: " << e.what() << std::endl;
		return false;
	}

	return false;
}

/**
 * Python initialization for eye detection functions
 * @return Success status of Python initialization
 */
bool initializePython() {
	std::cout << "Initializing Python..." << std::endl;

	// Initialize Python
	Py_Initialize();
	if (!Py_IsInitialized()) {
		std::cerr << "Failed to initialize Python interpreter" << std::endl;
		return false;
	}

	// Import NumPy
	import_array();

	// Add current directory to Python path
	PyRun_SimpleString(
			"import sys; sys.path.append('.'); sys.path.append('..'); sys.path.append('../python')");

	// Import our eye_detection_lib module and initialize it
	PyObject* pModule = PyImport_ImportModule("eye_detection_lib");
	if (pModule == nullptr) {
		PyErr_Print();
		std::cerr << "Failed to import eye_detection_lib module" << std::endl;
		return false;
	}

	// Call initialize function
	PyObject* pFunc = PyObject_GetAttrString(pModule, "initialize");
	if (pFunc == nullptr || !PyCallable_Check(pFunc)) {
		if (PyErr_Occurred()) PyErr_Print();
		std::cerr << "Cannot find function 'initialize'" << std::endl;
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
		return false;
	}

	// Call the initialize function
	PyObject* pValue = PyObject_CallObject(pFunc, nullptr);
	bool result = false;

	if (pValue != nullptr) {
		result = PyObject_IsTrue(pValue);
		Py_DECREF(pValue);
	} else {
		PyErr_Print();
	}

	Py_XDECREF(pFunc);
	Py_DECREF(pModule);

	if (!result) {
		std::cerr << "Failed to initialize eye detection module" << std::endl;
		return false;
	}

	std::cout << "Python initialization completed!" << std::endl;
	return true;
}

/**
 * Call Python is_eye_closed function to detect eye closure
 * @param frame The preprocessed frame to analyze
 * @return True if eyes are closed, false otherwise
 */
bool detectEyeClosure(const cv::Mat& frame) {
	std::cout << "\n----- Eye Closure Detection Started -----" << std::endl;
	auto startTime = std::chrono::high_resolution_clock::now();

	// Create Python GIL state
	PyGILState_STATE gstate = PyGILState_Ensure();

	bool result = false;

	try {
		// Import eye_detection_lib module
		PyObject* pModule = PyImport_ImportModule("eye_detection_lib");

		if (pModule == nullptr) {
			PyErr_Print();
			std::cerr << "Failed to import eye_detection_lib module" << std::endl;
			PyGILState_Release(gstate);
			return false;
		}

		// Get is_eye_closed function
		PyObject* pFunc = PyObject_GetAttrString(pModule, "is_eye_closed");
		if (pFunc == nullptr || !PyCallable_Check(pFunc)) {
			if (PyErr_Occurred()) {
				PyErr_Print();
			}
			std::cerr << "Cannot find function 'is_eye_closed'" << std::endl;
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			PyGILState_Release(gstate);
			return false;
		}

		// Create NumPy array from cv::Mat
		npy_intp dims[3] = {frame.rows, frame.cols, frame.channels()};
		PyObject* pArray =
				PyArray_SimpleNewFromData(frame.channels() == 1 ? 2 : 3, dims,
																	frame.depth() == CV_8U ? NPY_UINT8 : NPY_FLOAT32, frame.data);

		if (pArray == nullptr) {
			PyErr_Print();
			std::cerr << "Failed to create NumPy array from cv::Mat" << std::endl;
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			PyGILState_Release(gstate);
			return false;
		}

		// Hard-coded threshold for testing purposes
		float threshold = 0.25f;
		PyObject* pThreshold = PyFloat_FromDouble(threshold);

		// Call the function with frame and threshold
		PyObject* pArgs = PyTuple_New(2);
		PyTuple_SetItem(pArgs, 0, pArray);			// PyTuple_SetItem steals the reference to pArray
		PyTuple_SetItem(pArgs, 1, pThreshold);	// PyTuple_SetItem steals the reference to pThreshold

		PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
		Py_DECREF(pArgs);

		if (pValue == nullptr) {
			PyErr_Print();
			std::cerr << "Call to is_eye_closed failed" << std::endl;
			Py_XDECREF(pValue);
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			PyGILState_Release(gstate);
			return false;
		}

		// Get the boolean result
		result = PyObject_IsTrue(pValue);

		// Cleanup
		Py_XDECREF(pValue);
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
	} catch (const std::exception& e) {
		std::cerr << "C++ exception during eye detection: " << e.what() << std::endl;
		PyGILState_Release(gstate);
		return false;
	}

	// Release the GIL
	PyGILState_Release(gstate);

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::cout << "Eye detection result: " << (result ? "EYES CLOSED" : "EYES OPEN") << std::endl;
	std::cout << "Eye detection completed! (Time taken: " << duration.count() << "ms)" << std::endl;
	std::cout << "----- Eye Closure Detection Completed -----\n" << std::endl;

	return result;
}

int runEyeDetectionTest(int argc, char** argv) {
	std::cout << "===== Camera Class, removeLight Function, and Eye Detection Test Started ====="
						<< std::endl;

	// Set test iteration count
	int iterations = 10;	// Default 10 times
	if (argc > 1) {
		iterations = std::stoi(argv[1]);
	}

	std::cout << "Test iteration count: " << iterations << std::endl;

	// Initialize Python interpreter
	if (!initializePython()) {
		std::cerr << "Error: Failed to initialize Python!" << std::endl;
		return -1;
	}

	// Create and initialize Camera object
	Camera camera;
	camera.initialize();

	if (!camera.getCameraStatus()) {
		std::cerr << "Error: Cannot initialize camera!" << std::endl;
		return -1;
	}

	std::cout << "Camera initialization successful!" << std::endl;
	std::vector<int> resolution = camera.getResolution();
	std::cout << "Camera resolution: " << resolution[0] << "x" << resolution[1] << std::endl;

	// Start testing
	for (int i = 0; i < iterations; ++i) {
		std::cout << "\n===== Test #" << (i + 1) << " =====" << std::endl;

		// Capture frame
		cv::Mat frame = camera.captureFrame();

		// Check if frame is empty
		if (frame.empty()) {
			std::cerr << "Error: Empty frame!" << std::endl;
			continue;
		}

		// Call removeLight function
		cv::Mat lChannel, processed;
		bool preprocessSuccess = removeLight(frame, lChannel, processed);

		if (!preprocessSuccess) {
			std::cerr << "Error: Image preprocessing failed!" << std::endl;
			continue;
		}

		// Detect eye closure using Python function
		bool eyesClosed = detectEyeClosure(processed);

		// Display results with eye status
		cv::Mat displayFrame = frame.clone();
		std::string eyeStatus = eyesClosed ? "EYES CLOSED" : "EYES OPEN";
		cv::putText(displayFrame, eyeStatus, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
								eyesClosed ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0), 2);

		cv::imshow("Original Image with Eye Status", displayFrame);
		cv::imshow("L Channel", lChannel);
		cv::imshow("Processed Image", processed);

		// Wait for 1 second (terminate if ESC key is pressed)
		int key = cv::waitKey(1000);
		if (key == 27) {	// ESC key
			std::cout << "Test interrupted by user" << std::endl;
			break;
		}
	}

	// Cleanup
	cv::destroyAllWindows();
	Py_Finalize();

	std::cout << "\n===== Camera Class, removeLight Function, and Eye Detection Test Completed ====="
						<< std::endl;
	return 0;
}

int main(int argc, char** argv) {
	runEyeDetectionTest(argc, argv);
}
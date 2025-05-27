#include "../include/EyeDetectionLib.h"

#include <algorithm>
#include <cmath>
#include <iostream>

EyeDetectionLib::EyeDetectionLib() : isInitialized(false) {
	// Initialize the face detector (HOG-based)
	detector = dlib::get_frontal_face_detector();
}

EyeDetectionLib::~EyeDetectionLib() {
	// Destructor - dlib objects will be automatically cleaned up
}

bool EyeDetectionLib::initialize(const std::string& modelPath) {
	try {
		// Load the shape predictor model
		dlib::deserialize(modelPath) >> predictor;
		isInitialized = true;
		std::cout << "[EyeDetectionLib] Face landmark model loaded successfully from: " << modelPath
							<< std::endl;
		return true;
	} catch (const std::exception& e) {
		std::cerr << "[EyeDetectionLib] Error loading face landmark model: " << e.what() << std::endl;
		isInitialized = false;
		return false;
	}
}

bool EyeDetectionLib::getInitializationStatus() const {
	return isInitialized;
}

cv::Mat EyeDetectionLib::preprocessImage(const cv::Mat& frame) {
	cv::Mat processedFrame;

	try {
		// Resize image to 400px width (similar to Python implementation)
		int newWidth = 400;
		double aspectRatio = static_cast<double>(frame.cols) / frame.rows;
		int newHeight = static_cast<int>(newWidth / aspectRatio);
		cv::resize(frame, processedFrame, cv::Size(newWidth, newHeight));

		// Convert to LAB color space for better illumination handling
		cv::Mat lab;
		cv::cvtColor(processedFrame, lab, cv::COLOR_BGR2Lab);

		// Split LAB channels
		std::vector<cv::Mat> labChannels(3);
		cv::split(lab, labChannels);
		cv::Mat lChannel = labChannels[0].clone();

		// Apply median filter to L channel
		cv::Mat medianL;
		cv::medianBlur(lChannel, medianL, 99);

		// Invert the median filtered L channel
		cv::Mat invertedL;
		cv::bitwise_not(medianL, invertedL);

		// Convert original to grayscale
		cv::Mat gray;
		cv::cvtColor(processedFrame, gray, cv::COLOR_BGR2GRAY);

		// Combine grayscale with inverted L channel
		cv::Mat result;
		cv::addWeighted(gray, 0.75, invertedL, 0.25, 0, result);

		return result;

	} catch (const cv::Exception& e) {
		std::cerr << "[EyeDetectionLib] Error in preprocessing: " << e.what() << std::endl;
		// Return grayscale as fallback
		cv::Mat gray;
		cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
		return gray;
	}
}

std::vector<cv::Point2f> EyeDetectionLib::extractEyePoints(const dlib::full_object_detection& shape,
																													 int startIdx, int endIdx) {
	std::vector<cv::Point2f> points;

	for (int i = startIdx; i <= endIdx; ++i) {
		dlib::point p = shape.part(i);
		points.emplace_back(static_cast<float>(p.x()), static_cast<float>(p.y()));
	}

	return points;
}

double EyeDetectionLib::calculateEAR(const std::vector<cv::Point2f>& eyePoints) {
	if (eyePoints.size() != 6) {
		std::cerr << "[EyeDetectionLib] Invalid number of eye points: " << eyePoints.size()
							<< std::endl;
		return 0.0;
	}

	// Calculate euclidean distances
	// Vertical distances
	double A = cv::norm(eyePoints[1] - eyePoints[5]);	 // |P2 - P6|
	double B = cv::norm(eyePoints[2] - eyePoints[4]);	 // |P3 - P5|

	// Horizontal distance
	double C = cv::norm(eyePoints[0] - eyePoints[3]);	 // |P1 - P4|

	// Eye Aspect Ratio
	double ear = (A + B) / (2.0 * C);

	return ear;
}

bool EyeDetectionLib::isEyeClosed(const cv::Mat& frame, double threshold) {
	if (!isInitialized) {
		std::cerr << "[EyeDetectionLib] Library not initialized. Call initialize() first." << std::endl;
		return false;
	}

	if (frame.empty()) {
		std::cerr << "[EyeDetectionLib] Empty frame provided." << std::endl;
		return false;
	}

	try {
		// Preprocess the image
		cv::Mat processedFrame = preprocessImage(frame);

		// Convert OpenCV Mat to dlib image
		dlib::cv_image<unsigned char> dlibImage(processedFrame);

		// Detect faces
		std::vector<dlib::rectangle> faces = detector(dlibImage);

		if (faces.empty()) {
			std::cout << "[EyeDetectionLib] No face detected in frame." << std::endl;
			return false;
		}

		// Use the first detected face
		dlib::full_object_detection shape = predictor(dlibImage, faces[0]);

		// Extract eye points
		std::vector<cv::Point2f> leftEyePoints = extractEyePoints(shape, LEFT_EYE_START, LEFT_EYE_END);
		std::vector<cv::Point2f> rightEyePoints =
				extractEyePoints(shape, RIGHT_EYE_START, RIGHT_EYE_END);

		// Calculate EAR for both eyes
		double leftEAR = calculateEAR(leftEyePoints);
		double rightEAR = calculateEAR(rightEyePoints);

		// Average EAR
		double bothEAR = (leftEAR + rightEAR) / 2.0;

		std::cout << "[EyeDetectionLib] EAR: " << std::fixed << std::setprecision(3) << bothEAR
							<< ", Threshold: " << threshold << std::endl;

		// Determine if eyes are closed
		return bothEAR < threshold;

	} catch (const std::exception& e) {
		std::cerr << "[EyeDetectionLib] Error during eye closure detection: " << e.what() << std::endl;
		return false;
	}
}
#ifndef EYE_DETECTION_LIB_H
#define EYE_DETECTION_LIB_H

#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv.h>

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class EyeDetectionLib {
private:
	// dlib face detector and shape predictor
	dlib::frontal_face_detector detector;
	dlib::shape_predictor predictor;
	bool isInitialized;

	// Eye landmark indices (based on dlib's 68-point model)
	static const int LEFT_EYE_START = 36;
	static const int LEFT_EYE_END = 41;
	static const int RIGHT_EYE_START = 42;
	static const int RIGHT_EYE_END = 47;

	// Helper functions
	double calculateEAR(const std::vector<cv::Point2f>& eyePoints);
	std::vector<cv::Point2f> extractEyePoints(const dlib::full_object_detection& shape, int startIdx,
																						int endIdx);
	cv::Mat preprocessImage(const cv::Mat& frame);

public:
	EyeDetectionLib();
	~EyeDetectionLib();

	// Initialize the face predictor model
	bool initialize(const std::string& modelPath = "shape_predictor_68_face_landmarks.dat");

	// Main function to determine if eyes are closed
	bool isEyeClosed(const cv::Mat& frame, double threshold = 0.25);

	// Check if library is properly initialized
	bool getInitializationStatus() const;
};

#endif	// EYE_DETECTION_LIB_H
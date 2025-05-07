#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * @brief Entry point for the test program demonstrating OpenCV integration.
 *
 * Prints the OpenCV version, creates a 100x100 pixel 3-channel image matrix initialized to zeros, and confirms successful matrix creation.
 *
 * @return int Returns 0 upon successful execution.
 */
int main() {
    std::cout << "OpenCV 버전: " << CV_VERSION << std::endl;
    
    cv::Mat testImage = cv::Mat::zeros(100, 100, CV_8UC3);
    
    if (!testImage.empty()) {
        std::cout << "OpenCV Mat 생성 성공!" << std::endl;
    }
    
    return 0;
}
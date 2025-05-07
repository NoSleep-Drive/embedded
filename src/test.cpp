#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "OpenCV 버전: " << CV_VERSION << std::endl;
    
    cv::Mat testImage = cv::Mat::zeros(100, 100, CV_8UC3);
    
    if (!testImage.empty()) {
        std::cout << "OpenCV Mat 생성 성공!" << std::endl;
    }
    
    return 0;
}
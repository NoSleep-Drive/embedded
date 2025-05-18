#include "../include/VideoEncoder.h"
#include <iostream>

bool VideoEncoder::convertFramesToMP4(const std::string& path) {
    std::cout << "Converting frames at " << path << " to MP4 format for 5 seconds (2.5s before and after)." << std::endl;
    // Simulate conversion
    return true;
}

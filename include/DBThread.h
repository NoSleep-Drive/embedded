#ifndef DBTHREAD_H
#define DBTHREAD_H

#include <string>
#include <filesystem>
#include "VideoEncoder.h"

class DBThreadMonitoring;

class DBThread {
private:
    std::filesystem::file_time_type time;
    std::string deviceUid;
    std::string folderPath;
    DBThreadMonitoring* monitoring;

public:
    DBThread(const std::string& uid, const std::string& folder, DBThreadMonitoring* monitor);
    bool sendDataToDB();
    void setIsDBThreadRunningFalse();
};

#endif

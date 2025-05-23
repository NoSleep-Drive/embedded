#ifndef DBTHREAD_H
#define DBTHREAD_H

#include <string>
#include <filesystem>
#include <vector>
#include <opencv2/core.hpp> 
#include "VideoEncoder.h"

class DBThreadMonitoring;

class DBThread {
private:
    std::filesystem::file_time_type time;
    std::string deviceUid;
    std::string folderPath;
    DBThreadMonitoring* monitoring;

    void deleteFolderSafe(const std::string& path);

public:
    DBThread(const std::string& uid, const std::string& folder, DBThreadMonitoring* monitor);
    ~DBThread();
    bool sendDataToDB();
    bool sendVideoToBackend(const std::vector<uchar>& videoData);
    void setIsDBThreadRunningFalse();
    std::string getDetectedAtFromFolder() const;

    const std::string& getDeviceUid() const { return deviceUid; }
    const std::string& getFolderPath() const { return folderPath; }
};

#endif

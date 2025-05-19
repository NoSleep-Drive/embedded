#ifndef DBTHREADMONITORING_H
#define DBTHREADMONITORING_H

#include <queue>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <condition_variable>
#include <string>
#include <atomic>
#include <memory>

class DBThread;

class DBThreadMonitoring {
private:
    std::atomic<bool> isDBThreadRunning = false;
    std::atomic<bool> terminate = false;

    std::queue<std::shared_ptr<DBThread>> threadQueue;
    std::unordered_set<std::string> activeThreadKeys;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::thread monitoringThread;

    void processThreadQueue();
    std::string generateThreadKey(const std::string& deviceUid, const std::string& folderPath);

public:
    DBThreadMonitoring();
    ~DBThreadMonitoring();
    void startDBMonitoring();
    void addDBThread(std::shared_ptr<DBThread> thread);
    void removeActiveThread(const std::string& key);

    bool getIsDBThreadRunning() const;
    void setIsDBThreadRunning(bool value);
    
};

#endif

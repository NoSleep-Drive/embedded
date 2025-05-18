#ifndef DBTHREADMONITORING_H
#define DBTHREADMONITORING_H

#include <queue>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <condition_variable>
#include <string>

class DBThread;

class DBThreadMonitoring {
private:
    std::atomic<bool> isDBThreadRunning = false;
    std::queue<DBThread*> threadQueue;
    std::unordered_set<std::string> activeThreadKeys;
    std::mutex queueMutex;
    std::condition_variable condition;

    void processThreadQueue();
    std::string generateThreadKey(const std::string& deviceUid, const std::string& folderPath);

public:
    DBThreadMonitoring();
    void startDBMonitoring();
    void addDBThread(DBThread* thread);

    bool getIsDBThreadRunning() const;
    void setIsDBThreadRunning(bool value);
    void removeActiveThread(const std::string& key);
};

#endif

#ifndef DBTHREADMONITORING_H
#define DBTHREADMONITORING_H

#include <queue>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <condition_variable>

class DBThread;

class DBThreadMonitoring {
private:
    std::atomic<bool> isDBThreadRunning = false;
    std::queue<DBThread*> threadQueue;
    std::unordered_set<DBThread*> activeThreads;
    std::mutex queueMutex;
    std::condition_variable condition;

    void processThreadQueue();

public:
    DBThreadMonitoring();
    void startDBMonitoring();
    void addDBThread(DBThread* thread);

    bool getIsDBThreadRunning() const;
    void setIsDBThreadRunning(bool value);
};

#endif

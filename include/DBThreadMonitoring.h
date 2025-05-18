#ifndef DBTHREADMONITORING_H
#define DBTHREADMONITORING_H

#include <queue>
#include <thread>
#include "DBThread.h"

class DBThreadMonitoring {
private:
    bool isDBThreadRunning = false;
    std::queue<std::thread> threadQueue;

public:
    DBThreadMonitoring() : isDBThreadRunning(false) {}

    void startDBMonitoring();
    void startDBThread(DBThread* thread);

    bool getIsDBThreadRunning() const;
    void setIsDBThreadRunning(bool value);

    friend class DBThread;
};

#endif

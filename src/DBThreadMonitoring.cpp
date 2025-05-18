#include "../include/DBThreadMonitoring.h"

void DBThreadMonitoring::startDBMonitoring() {
    isDBThreadRunning = false;

    while (true) {
        if (!isDBThreadRunning && !threadQueue.empty()) {
            isDBThreadRunning = true;
            std::thread& t = threadQueue.front();
            t.join();
            threadQueue.pop();
        }
    }
}

void DBThreadMonitoring::startDBThread(DBThread* thread) {
    threadQueue.push(std::thread(&DBThread::sendDataToDB, thread));
}

bool DBThreadMonitoring::getIsDBThreadRunning() const {
    return isDBThreadRunning;
}

void DBThreadMonitoring::setIsDBThreadRunning(bool value) {
    isDBThreadRunning = value;
}

#include "../include/DBThreadMonitoring.h"
#include "../include/DBThread.h"
#include <iostream>

DBThreadMonitoring::DBThreadMonitoring() {
    startDBMonitoring();
}

void DBThreadMonitoring::startDBMonitoring() {
    std::thread([this] {
        processThreadQueue();
        }).detach();
}

void DBThreadMonitoring::processThreadQueue() {
    std::unique_lock<std::mutex> lock(queueMutex);

    while (true) {
        condition.wait(lock, [this] {
            return !threadQueue.empty();
            });

        while (!threadQueue.empty()) {
            DBThread* thread = threadQueue.front();
            threadQueue.pop();

            activeThreads.insert(thread);
            lock.unlock();

            std::thread([thread, this] {
                thread->sendDataToDB();
                std::unique_lock<std::mutex> lock(queueMutex);
                activeThreads.erase(thread);
                condition.notify_all();
                }).detach();

                lock.lock();
        }
    }
}

void DBThreadMonitoring::addDBThread(DBThread* thread) {
    std::unique_lock<std::mutex> lock(queueMutex);
    if (activeThreads.find(thread) == activeThreads.end()) {
        threadQueue.push(thread);
        condition.notify_one();
        std::cout << "DBThread 추가됨, 큐 크기: " << threadQueue.size() << std::endl;
    }
    else {
        std::cout << "이미 실행 중인 스레드입니다." << std::endl;
    }
}

bool DBThreadMonitoring::getIsDBThreadRunning() const {
    return isDBThreadRunning.load();
}

void DBThreadMonitoring::setIsDBThreadRunning(bool value) {
    std::unique_lock<std::mutex> lock(queueMutex);
    isDBThreadRunning = value;
    condition.notify_all();
}

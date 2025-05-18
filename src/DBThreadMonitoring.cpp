#include "../include/DBThreadMonitoring.h"
#include "../include/DBThread.h"
#include <iostream>

std::string DBThreadMonitoring::generateThreadKey(const std::string& deviceUid, const std::string& folderPath) {
    return deviceUid + "::" + folderPath;
}

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

            lock.unlock();
            std::thread([thread, this] {
                thread->sendDataToDB();
                removeActiveThread(generateThreadKey(thread->getDeviceUid(), thread->getFolderPath())); // 🔧 중복 제거
                }).detach();
                lock.lock();
        }
    }
}

void DBThreadMonitoring::addDBThread(DBThread* thread) {
    std::unique_lock<std::mutex> lock(queueMutex);
    std::string threadKey = generateThreadKey(thread->getDeviceUid(), thread->getFolderPath());

    if (activeThreadKeys.find(threadKey) == activeThreadKeys.end()) {
        activeThreadKeys.insert(threadKey);
        threadQueue.push(thread);
        condition.notify_one();
        std::cout << "DBThread 추가됨, 큐 크기: " << threadQueue.size() << std::endl;
    }
    else {
        std::cout << "이미 실행 중인 스레드입니다 : " << threadKey << std::endl;
    }
}

void DBThreadMonitoring::removeActiveThread(const std::string& key) {
    std::unique_lock<std::mutex> lock(queueMutex);
    activeThreadKeys.erase(key);
    condition.notify_all();
}

bool DBThreadMonitoring::getIsDBThreadRunning() const {
    return isDBThreadRunning.load();
}

void DBThreadMonitoring::setIsDBThreadRunning(bool value) {
    std::unique_lock<std::mutex> lock(queueMutex);
    isDBThreadRunning = value;
    condition.notify_all();
}

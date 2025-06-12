#include "../include/DBThreadMonitoring.h"
#include "../include/DBThread.h"
#include <iostream>

std::string DBThreadMonitoring::generateThreadKey(const std::string& deviceUid, const std::string& folderPath) {
    return deviceUid + "::" + folderPath;
}

DBThreadMonitoring::DBThreadMonitoring() {
    //startDBMonitoring();
}

DBThreadMonitoring::~DBThreadMonitoring() {
    terminate = true;
    condition.notify_all();
    if (monitoringThread.joinable()) {
        monitoringThread.join();
    }
}

void DBThreadMonitoring::startDBMonitoring() {
    try {
        monitoringThread = std::thread([this] {
            processThreadQueue();
            });
    }
    catch (const std::exception& e) {
        std::cout << "Error during starting thread: " << e.what() << std::endl;
    }
}

void DBThreadMonitoring::processThreadQueue() {
    std::unique_lock<std::mutex> lock(queueMutex);

    while (!terminate) {
        condition.wait(lock, [this] {
            return terminate || !threadQueue.empty();
         });

        while (!threadQueue.empty()) {
            auto thread = threadQueue.front();
            threadQueue.pop();
            lock.unlock();

            std::thread([ptr = thread, this] {
                try {
                    ptr->sendDataToDB();
                }
                catch (const std::exception& e) {
                    std::cerr << "DBThread exception: " << e.what() << std::endl;
                }
                removeActiveThread(generateThreadKey(ptr->getDeviceUid(), ptr->getFolderPath()));
                }).detach();
                lock.lock();
        }
    }
}

void DBThreadMonitoring::addDBThread(std::shared_ptr<DBThread> thread) {
    std::unique_lock<std::mutex> lock(queueMutex);
    std::string threadKey = generateThreadKey(thread->getDeviceUid(), thread->getFolderPath());

    if (activeThreadKeys.find(threadKey) == activeThreadKeys.end()) {
        activeThreadKeys.insert(threadKey);
        threadQueue.push(thread);
        condition.notify_one();
        std::cout << "DBThread added, queue size: " << threadQueue.size() << std::endl;
    }
    else {
        std::cout << "thread is already started : " << threadKey << std::endl;
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
    isDBThreadRunning.store(value, std::memory_order_release);
    condition.notify_all();
}

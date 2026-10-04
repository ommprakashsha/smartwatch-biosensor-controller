#ifndef BIOWATCH_CONTROLLER_HPP
#define BIOWATCH_CONTROLLER_HPP

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "DeviceHandle.hpp"

class BioWatchController {
public:
    explicit BioWatchController(const std::string& devicePath = "/dev/smart_watch_bio");
    ~BioWatchController();

    void start();
    void stop();
    void setHeartRateThreshold(int32_t maxBpm);
    void simulateManualSpike();

private:
    void vitalsReaderWorker();
    void hapticSafetyWorker();

    std::unique_ptr<DeviceHandle> device_;
    std::atomic<bool> isRunning_;
    int32_t hrLimitBpm_;

    std::mutex vitalsMutex_;
    std::condition_variable cvVitalsReady_;
    struct biowatch_vitals_t latestVitals_;
    bool hasNewVitals_;

    std::thread readerThread_;
    std::thread hapticThread_;
};

#endif // BIOWATCH_CONTROLLER_HPP

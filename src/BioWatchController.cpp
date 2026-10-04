#include "BioWatchController.hpp"
#include "Logger.hpp"
#include <chrono>
#include <sstream>

BioWatchController::BioWatchController(const std::string& devicePath)
    : isRunning_(false), hrLimitBpm_(130), hasNewVitals_(false) {
    device_ = std::make_unique<DeviceHandle>(devicePath);

    if (device_->isSimulatedMode()) {
        Logger::getInstance().info("BioWatchController: Running in [Standalone Simulation Mode] (Virtual Register Space active).");
    } else {
        Logger::getInstance().info("BioWatchController: Connected to Linux Kernel Driver at " + devicePath);
    }
}

BioWatchController::~BioWatchController() {
    stop();
}

void BioWatchController::setHeartRateThreshold(int32_t maxBpm) {
    hrLimitBpm_ = maxBpm;
    device_->setHeartRateLimit(maxBpm);
    std::ostringstream oss;
    oss << "BioWatch: Configured Maximum Safe Heart Rate Alert Limit to " << maxBpm << " BPM.";
    Logger::getInstance().info(oss.str());
}

void BioWatchController::start() {
    if (isRunning_) return;

    isRunning_ = true;
    setHeartRateThreshold(hrLimitBpm_);

    Logger::getInstance().info("BioWatchController: Starting Vitals Reader & Haptic Guardian Threads...");
    readerThread_ = std::thread(&BioWatchController::vitalsReaderWorker, this);
    hapticThread_ = std::thread(&BioWatchController::hapticSafetyWorker, this);
}

void BioWatchController::stop() {
    if (!isRunning_) return;

    Logger::getInstance().info("BioWatchController: Gracefully stopping guardian threads...");
    isRunning_ = false;
    cvVitalsReady_.notify_all();

    if (readerThread_.joinable()) readerThread_.join();
    if (hapticThread_.joinable()) hapticThread_.join();

    try {
        device_->triggerHapticMotor(0);
        device_->clearAlerts();
    } catch (...) {}

    Logger::getInstance().info("BioWatchController: All guardian threads safely terminated.");
}

void BioWatchController::vitalsReaderWorker() {
    Logger::getInstance().info("Worker-1 (PPG Vitals Reader) active.");

    while (isRunning_) {
        try {
            struct biowatch_vitals_t vitals = device_->getVitals();

            {
                std::lock_guard<std::mutex> lock(vitalsMutex_);
                latestVitals_ = vitals;
                hasNewVitals_ = true;
            }
            cvVitalsReady_.notify_one();

            std::ostringstream oss;
            oss << "Vitals: HeartRate=" << vitals.heart_rate_bpm << " BPM"
                << " | Steps=" << vitals.step_count
                << " | SpO2=" << vitals.spo2_percent << "%"
                << " | Haptic=" << vitals.haptic_intensity << "%"
                << " | Flags=0x" << std::hex << vitals.status_flags;

            if (vitals.status_flags & BIO_STATUS_HR_ALERT) {
                Logger::getInstance().alert(oss.str());
            } else {
                Logger::getInstance().info(oss.str());
            }

        } catch (const std::exception& ex) {
            Logger::getInstance().error(std::string("Reader Thread Error: ") + ex.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void BioWatchController::hapticSafetyWorker() {
    Logger::getInstance().info("Worker-2 (Haptic Guardian Actuator) active.");

    while (isRunning_) {
        std::unique_lock<std::mutex> lock(vitalsMutex_);
        cvVitalsReady_.wait(lock, [this] { return hasNewVitals_ || !isRunning_; });

        if (!isRunning_) break;

        struct biowatch_vitals_t current = latestVitals_;
        hasNewVitals_ = false;
        lock.unlock();

        try {
            // Safety Evaluation: High Heart Rate (Tachycardia Alert)
            if (current.heart_rate_bpm >= current.hr_threshold_limit) {
                if (current.haptic_intensity == 0) {
                    uint32_t buzzPwm = 85; // 85% haptic vibration
                    device_->triggerHapticMotor(buzzPwm);
                    std::ostringstream oss;
                    oss << "CRITICAL ALERT: Tachycardia detected (" << current.heart_rate_bpm 
                        << " BPM >= " << current.hr_threshold_limit 
                        << " BPM limit)! Firing Haptic Vibration Buzz (" << buzzPwm << "% PWM).";
                    Logger::getInstance().haptic(oss.str());
                }
            } else if (current.heart_rate_bpm <= (current.hr_threshold_limit - 10)) {
                // Heart rate has recovered back to safe zone
                if (current.haptic_intensity > 0) {
                    device_->triggerHapticMotor(0);
                    std::ostringstream oss;
                    oss << "NORMALIZED: Heart rate stabilized at " << current.heart_rate_bpm 
                        << " BPM. Haptic motor disengaged.";
                    Logger::getInstance().info(oss.str());
                }
            }
        } catch (const std::exception& ex) {
            Logger::getInstance().error(std::string("Haptic Guardian Error: ") + ex.what());
        }
    }
}

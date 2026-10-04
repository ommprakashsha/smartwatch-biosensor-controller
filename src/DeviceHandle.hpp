#ifndef DEVICE_HANDLE_HPP
#define DEVICE_HANDLE_HPP

#include <string>
#include <stdexcept>
#include <mutex>
#include "../driver/biowatch_ioctl.h"

class DeviceException : public std::runtime_error {
public:
    explicit DeviceException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief RAII Device Handle supporting Dual-Mode Execution:
 *        1. Real Linux Kernel Mode: Interfaces with /dev/smart_watch_bio via POSIX syscalls & IOCTL.
 *        2. Standalone Simulation Mode: Seamlessly emulates hardware registers in memory if
 *           the kernel driver is not loaded or running on Windows/host systems.
 */
class DeviceHandle {
public:
    explicit DeviceHandle(const std::string& devicePath = "/dev/smart_watch_bio");
    ~DeviceHandle();

    DeviceHandle(const DeviceHandle&) = delete;
    DeviceHandle& operator=(const DeviceHandle&) = delete;

    DeviceHandle(DeviceHandle&& other) noexcept;
    DeviceHandle& operator=(DeviceHandle&& other) noexcept;

    // Core Hardware Interfaces
    std::string readRawStream();
    struct biowatch_vitals_t getVitals();
    void setHeartRateLimit(int32_t limitBpm);
    void triggerHapticMotor(uint32_t intensityPercent);
    void resetPedometer();
    void clearAlerts();

    bool isSimulatedMode() const noexcept { return isSimulated_; }
    bool isOpen() const noexcept { return fd_ >= 0 || isSimulated_; }

private:
    int fd_;
    std::string path_;
    bool isSimulated_;

    // In-memory hardware register emulation for standalone testing
    struct biowatch_vitals_t simState_;
    std::mutex simMutex_;
    void updateSimulatedState();
};

#endif // DEVICE_HANDLE_HPP

#include "DeviceHandle.hpp"
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#endif

DeviceHandle::DeviceHandle(const std::string& devicePath)
    : fd_(-1), path_(devicePath), isSimulated_(false) {

    // Initialize simulated register state
    simState_.heart_rate_bpm = 74;
    simState_.step_count = 1200;
    simState_.spo2_percent = 98;
    simState_.hr_threshold_limit = 135;
    simState_.haptic_intensity = 0;
    simState_.status_flags = BIO_STATUS_READY;
    simState_.timestamp_ms = 0;

#ifndef _WIN32
    fd_ = ::open(devicePath.c_str(), O_RDWR);
    if (fd_ < 0) {
        // Fall back to in-memory hardware simulation mode
        isSimulated_ = true;
    }
#else
    isSimulated_ = true;
#endif
}

DeviceHandle::~DeviceHandle() {
#ifndef _WIN32
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

DeviceHandle::DeviceHandle(DeviceHandle&& other) noexcept
    : fd_(other.fd_), path_(std::move(other.path_)), isSimulated_(other.isSimulated_), simState_(other.simState_) {
    other.fd_ = -1;
}

DeviceHandle& DeviceHandle::operator=(DeviceHandle&& other) noexcept {
    if (this != &other) {
#ifndef _WIN32
        if (fd_ >= 0) ::close(fd_);
#endif
        fd_ = other.fd_;
        path_ = std::move(other.path_);
        isSimulated_ = other.isSimulated_;
        simState_ = other.simState_;
        other.fd_ = -1;
    }
    return *this;
}

void DeviceHandle::updateSimulatedState() {
    std::lock_guard<std::mutex> lock(simMutex_);
    simState_.step_count += (1 + (std::rand() % 3));

    if (simState_.haptic_intensity > 0) {
        simState_.heart_rate_bpm -= (2 + (std::rand() % 3));
        if (simState_.heart_rate_bpm < 70) simState_.heart_rate_bpm = 70;
    } else {
        simState_.heart_rate_bpm += ((std::rand() % 5) - 1);
        if (simState_.heart_rate_bpm > 160) simState_.heart_rate_bpm = 160;
    }

    simState_.status_flags = BIO_STATUS_READY;
    if (simState_.heart_rate_bpm >= simState_.hr_threshold_limit) {
        simState_.status_flags |= BIO_STATUS_HR_ALERT;
    }
    if (simState_.haptic_intensity > 0) {
        simState_.status_flags |= BIO_STATUS_HAPTIC_ACTIVE;
    }
}

std::string DeviceHandle::readRawStream() {
    if (isSimulated_) {
        updateSimulatedState();
        std::ostringstream oss;
        oss << "HR:" << simState_.heart_rate_bpm << "_BPM;STEPS:" << simState_.step_count 
            << ";SPO2:" << simState_.spo2_percent << "%;HAPTIC:" << simState_.haptic_intensity 
            << "%;FLAGS:0x" << std::hex << simState_.status_flags << "\n";
        return oss.str();
    }

#ifndef _WIN32
    char buffer[128];
    ssize_t bytesRead = ::read(fd_, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) throw DeviceException(std::strerror(errno));
    buffer[bytesRead] = '\0';
    return std::string(buffer);
#else
    return "";
#endif
}

struct biowatch_vitals_t DeviceHandle::getVitals() {
    if (isSimulated_) {
        updateSimulatedState();
        std::lock_guard<std::mutex> lock(simMutex_);
        return simState_;
    }

#ifndef _WIN32
    struct biowatch_vitals_t vitals;
    if (::ioctl(fd_, BIO_IOCTL_GET_VITALS, &vitals) < 0) {
        throw DeviceException("IOCTL_GET_VITALS failed");
    }
    return vitals;
#else
    return simState_;
#endif
}

void DeviceHandle::setHeartRateLimit(int32_t limitBpm) {
    if (isSimulated_) {
        std::lock_guard<std::mutex> lock(simMutex_);
        simState_.hr_threshold_limit = limitBpm;
        return;
    }

#ifndef _WIN32
    if (::ioctl(fd_, BIO_IOCTL_SET_HR_LIMIT, &limitBpm) < 0) {
        throw DeviceException("IOCTL_SET_HR_LIMIT failed");
    }
#endif
}

void DeviceHandle::triggerHapticMotor(uint32_t intensityPercent) {
    if (isSimulated_) {
        std::lock_guard<std::mutex> lock(simMutex_);
        simState_.haptic_intensity = intensityPercent;
        if (intensityPercent > 0) simState_.status_flags |= BIO_STATUS_HAPTIC_ACTIVE;
        else simState_.status_flags &= ~BIO_STATUS_HAPTIC_ACTIVE;
        return;
    }

#ifndef _WIN32
    if (::ioctl(fd_, BIO_IOCTL_TRIGGER_HAPTIC, &intensityPercent) < 0) {
        throw DeviceException("IOCTL_TRIGGER_HAPTIC failed");
    }
#endif
}

void DeviceHandle::resetPedometer() {
    if (isSimulated_) {
        std::lock_guard<std::mutex> lock(simMutex_);
        simState_.step_count = 0;
        return;
    }

#ifndef _WIN32
    ::ioctl(fd_, BIO_IOCTL_RESET_STEPS);
#endif
}

void DeviceHandle::clearAlerts() {
    if (isSimulated_) {
        std::lock_guard<std::mutex> lock(simMutex_);
        simState_.heart_rate_bpm = 74;
        simState_.haptic_intensity = 0;
        simState_.status_flags = BIO_STATUS_READY;
        return;
    }

#ifndef _WIN32
    ::ioctl(fd_, BIO_IOCTL_CLEAR_ALERTS);
#endif
}

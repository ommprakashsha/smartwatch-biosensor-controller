#ifndef BIOWATCH_IOCTL_H
#define BIOWATCH_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif

/*
 * Hardware Register Map (Simulating Memory-Mapped I/O for Smartwatch Biosensor)
 */
#define REG_STATUS_OFFSET        0x00  /* Status Register (Read-Only) */
#define REG_HEART_RATE_OFFSET    0x04  /* Optical PPG Heart Rate (BPM) (Read-Only) */
#define REG_STEP_COUNT_OFFSET    0x08  /* Pedometer 3-Axis Steps (Read-Only) */
#define REG_SPO2_OFFSET          0x0C  /* Blood Oxygen Saturation % (Read-Only) */
#define REG_HR_LIMIT_OFFSET      0x10  /* Tachycardia Alert Limit (Read/Write) */
#define REG_HAPTIC_MOTOR_OFFSET  0x14  /* Haptic Vibration PWM % (Read/Write) */

/* Bitwise status flags for REG_STATUS */
#define BIO_STATUS_READY             (1 << 0) /* Bit 0: Sensor active and calibrated */
#define BIO_STATUS_HR_ALERT          (1 << 1) /* Bit 1: Heart rate exceeded threshold */
#define BIO_STATUS_HAPTIC_ACTIVE     (1 << 2) /* Bit 2: Vibration motor spinning */
#define BIO_STATUS_FALL_DETECTED     (1 << 3) /* Bit 3: Sudden high-G shock detected */

/*
 * Telemetry structure transferred across Kernel <-> User Space Boundary
 */
struct biowatch_vitals_t {
    int32_t heart_rate_bpm;      /* Current Heart Rate (BPM) */
    uint32_t step_count;         /* Total steps counted */
    int32_t spo2_percent;        /* Blood Oxygen (e.g. 98%) */
    int32_t hr_threshold_limit;  /* Max safe heart rate (e.g. 140 BPM) */
    uint32_t haptic_intensity;   /* Vibration motor intensity (0 - 100%) */
    uint32_t status_flags;       /* Bitwise flags (BIO_STATUS_*) */
    uint64_t timestamp_ms;       /* Kernel jiffies or epoch timestamp */
};

/*
 * IOCTL Definitions
 * Magic number chosen uniquely: 'w' (0x77) for SmartWatch
 */
#define BIOWATCH_IOCTL_MAGIC 'w'

/* Command 1: Configure Heart Rate Alert Threshold (Write) */
#define BIO_IOCTL_SET_HR_LIMIT      _IOW(BIOWATCH_IOCTL_MAGIC, 1, int32_t)

/* Command 2: Read complete vitals packet (Read) */
#define BIO_IOCTL_GET_VITALS        _IOR(BIOWATCH_IOCTL_MAGIC, 2, struct biowatch_vitals_t)

/* Command 3: Trigger Haptic Vibration Motor (Write, 0-100%) */
#define BIO_IOCTL_TRIGGER_HAPTIC    _IOW(BIOWATCH_IOCTL_MAGIC, 3, uint32_t)

/* Command 4: Reset pedometer step count to zero (No arg) */
#define BIO_IOCTL_RESET_STEPS       _IO(BIOWATCH_IOCTL_MAGIC, 4)

/* Command 5: Clear emergency alert flags (No arg) */
#define BIO_IOCTL_CLEAR_ALERTS      _IO(BIOWATCH_IOCTL_MAGIC, 5)

#endif /* BIOWATCH_IOCTL_H */

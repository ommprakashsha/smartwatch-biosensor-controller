/**
 * @file biowatch_driver.c
 * @brief Linux Character Device Driver for Smartwatch Biosensor & Haptic Actuator.
 * @details Emulates optical PPG heart rate sensor, pedometer step counter, and haptic
 *          vibration motor registers with IOCTL command handlers and kernel mutex synchronization.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/random.h>
#include "biowatch_ioctl.h"

#define DRIVER_NAME "smart_watch_bio"
#define CLASS_NAME  "biowatch_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smartwatch Wearable Team");
MODULE_DESCRIPTION("Smartwatch Biosensor & Haptic Alert Character Device Driver");
MODULE_VERSION("1.0");

static dev_t dev_number;
static struct cdev biowatch_cdev;
static struct class *biowatch_class = NULL;
static struct device *biowatch_device = NULL;

/* Emulated Hardware State */
struct biowatch_hardware_state {
    int32_t heart_rate_bpm;
    uint32_t step_count;
    int32_t spo2_percent;
    int32_t hr_threshold_limit;
    uint32_t haptic_intensity;
    uint32_t status_flags;
    struct mutex dev_mutex;
};

static struct biowatch_hardware_state hw_state;

/**
 * @brief Helper: Simulates dynamic biosensor physiological changes
 */
static void update_simulated_vitals(void)
{
    unsigned int rand_val;
    get_random_bytes(&rand_val, sizeof(rand_val));

    /* Steps increment as user is walking */
    hw_state.step_count += (1 + (rand_val % 4));

    /*
     * If haptic motor is buzzing, user relaxes / slows down -> HR drops.
     * Otherwise, HR fluctuates naturally with exercise spikes.
     */
    if (hw_state.haptic_intensity > 0) {
        hw_state.heart_rate_bpm -= (2 + (rand_val % 3));
        if (hw_state.heart_rate_bpm < 68) {
            hw_state.heart_rate_bpm = 68;
        }
    } else {
        hw_state.heart_rate_bpm += ((int)(rand_val % 5) - 1);
        if (hw_state.heart_rate_bpm > 165) {
            hw_state.heart_rate_bpm = 165;
        }
    }

    /* Update Status Register flags */
    hw_state.status_flags = BIO_STATUS_READY;
    if (hw_state.heart_rate_bpm >= hw_state.hr_threshold_limit) {
        hw_state.status_flags |= BIO_STATUS_HR_ALERT;
    }
    if (hw_state.haptic_intensity > 0) {
        hw_state.status_flags |= BIO_STATUS_HAPTIC_ACTIVE;
    }
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    pr_info("%s: Smartwatch biosensor session opened\n", DRIVER_NAME);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    pr_info("%s: Smartwatch biosensor session closed\n", DRIVER_NAME);
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    char msg[96];
    int msg_len;
    int ret;

    if (*offset > 0) return 0; // EOF

    mutex_lock(&hw_state.dev_mutex);
    update_simulated_vitals();
    msg_len = snprintf(msg, sizeof(msg),
                       "HR:%d_BPM;STEPS:%u;SPO2:%d%%;HAPTIC:%u%%;FLAGS:0x%X\n",
                       hw_state.heart_rate_bpm,
                       hw_state.step_count,
                       hw_state.spo2_percent,
                       hw_state.haptic_intensity,
                       hw_state.status_flags);
    mutex_unlock(&hw_state.dev_mutex);

    if (len < msg_len) return -EINVAL;

    ret = copy_to_user(buffer, msg, msg_len);
    if (ret != 0) {
        pr_err("%s: Failed to copy %d bytes to user space\n", DRIVER_NAME, ret);
        return -EFAULT;
    }

    *offset = msg_len;
    return msg_len;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    char kbuf[32];
    size_t copy_size = min(len, sizeof(kbuf) - 1);

    if (copy_from_user(kbuf, buffer, copy_size)) {
        return -EFAULT;
    }
    kbuf[copy_size] = '\0';

    mutex_lock(&hw_state.dev_mutex);
    if (strstr(kbuf, "SPIKE")) {
        hw_state.heart_rate_bpm = 152; // Inject tachycardia alert
        pr_info("%s: Injected manual Heart Rate spike (152 BPM)\n", DRIVER_NAME);
    }
    mutex_unlock(&hw_state.dev_mutex);

    return len;
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int32_t new_limit;
    uint32_t new_haptic;
    struct biowatch_vitals_t vitals;

    if (_IOC_TYPE(cmd) != BIOWATCH_IOCTL_MAGIC) {
        return -ENOTTY;
    }

    mutex_lock(&hw_state.dev_mutex);

    switch (cmd) {
    case BIO_IOCTL_SET_HR_LIMIT:
        if (copy_from_user(&new_limit, (int32_t __user *)arg, sizeof(int32_t))) {
            ret = -EFAULT;
            break;
        }
        if (new_limit < 80 || new_limit > 200) {
            ret = -EINVAL;
            break;
        }
        hw_state.hr_threshold_limit = new_limit;
        pr_info("%s: Configured HR alert limit to %d BPM\n", DRIVER_NAME, new_limit);
        break;

    case BIO_IOCTL_GET_VITALS:
        update_simulated_vitals();
        vitals.heart_rate_bpm = hw_state.heart_rate_bpm;
        vitals.step_count = hw_state.step_count;
        vitals.spo2_percent = hw_state.spo2_percent;
        vitals.hr_threshold_limit = hw_state.hr_threshold_limit;
        vitals.haptic_intensity = hw_state.haptic_intensity;
        vitals.status_flags = hw_state.status_flags;
        vitals.timestamp_ms = get_jiffies_64();

        if (copy_to_user((struct biowatch_vitals_t __user *)arg, &vitals, sizeof(vitals))) {
            ret = -EFAULT;
            break;
        }
        break;

    case BIO_IOCTL_TRIGGER_HAPTIC:
        if (copy_from_user(&new_haptic, (uint32_t __user *)arg, sizeof(uint32_t))) {
            ret = -EFAULT;
            break;
        }
        if (new_haptic > 100) new_haptic = 100;
        hw_state.haptic_intensity = new_haptic;
        pr_info("%s: Haptic motor PWM set to %u%%\n", DRIVER_NAME, new_haptic);
        break;

    case BIO_IOCTL_RESET_STEPS:
        hw_state.step_count = 0;
        pr_info("%s: Pedometer step count reset to 0\n", DRIVER_NAME);
        break;

    case BIO_IOCTL_CLEAR_ALERTS:
        hw_state.heart_rate_bpm = 75;
        hw_state.haptic_intensity = 0;
        hw_state.status_flags = BIO_STATUS_READY;
        pr_info("%s: Cleared emergency alerts, restored normal baseline\n", DRIVER_NAME);
        break;

    default:
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&hw_state.dev_mutex);
    return ret;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int __init biowatch_init(void)
{
    int ret;
    pr_info("%s: Loading Smartwatch Biosensor Driver...\n", DRIVER_NAME);

    ret = alloc_chrdev_region(&dev_number, 0, 1, DRIVER_NAME);
    if (ret < 0) return ret;

    cdev_init(&biowatch_cdev, &fops);
    biowatch_cdev.owner = THIS_MODULE;
    ret = cdev_add(&biowatch_cdev, dev_number, 1);
    if (ret < 0) goto unregister_chrdev;

    biowatch_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(biowatch_class)) {
        ret = PTR_ERR(biowatch_class);
        goto del_cdev;
    }

    biowatch_device = device_create(biowatch_class, NULL, dev_number, NULL, DRIVER_NAME);
    if (IS_ERR(biowatch_device)) {
        ret = PTR_ERR(biowatch_device);
        goto destroy_class;
    }

    mutex_init(&hw_state.dev_mutex);
    hw_state.heart_rate_bpm = 74;
    hw_state.step_count = 1450;
    hw_state.spo2_percent = 98;
    hw_state.hr_threshold_limit = 135;
    hw_state.haptic_intensity = 0;
    hw_state.status_flags = BIO_STATUS_READY;

    pr_info("%s: Device initialized at /dev/%s\n", DRIVER_NAME, DRIVER_NAME);
    return 0;

destroy_class:
    class_destroy(biowatch_class);
del_cdev:
    cdev_del(&biowatch_cdev);
unregister_chrdev:
    unregister_chrdev_region(dev_number, 1);
    return ret;
}

static void __exit biowatch_exit(void)
{
    pr_info("%s: Unloading driver...\n", DRIVER_NAME);
    mutex_destroy(&hw_state.dev_mutex);
    device_destroy(biowatch_class, dev_number);
    class_destroy(biowatch_class);
    cdev_del(&biowatch_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("%s: Driver unloaded successfully\n", DRIVER_NAME);
}

module_init(biowatch_init);
module_exit(biowatch_exit);

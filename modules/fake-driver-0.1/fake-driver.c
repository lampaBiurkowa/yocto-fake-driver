#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>

#define BUFFER_SIZE (32)
static char messageBuffer[BUFFER_SIZE];

static ssize_t read(struct file *filePtr, char __user *userBuffer, size_t count, loff_t *offset);
static int majorNumber = 0;
static const char name[] = "fake driver";
static struct file_operations operations = 
{
    .owner = THIS_MODULE,
    .read = read
};

static int initDriver(void)
{
    printk(KERN_NOTICE "attempt to load fake driver\n" );

    int result = register_chrdev(0, name, &operations);
    if (result < 0)
    {
        printk(KERN_WARNING "error registering fake driver: %i\n", result);
        return result;
    }

    majorNumber = result;
    printk(KERN_INFO "fake driver loaded (%i)\n", majorNumber);

    return 0;
}

static void exitDriver(void)
{
    printk(KERN_NOTICE "fake driver exit\n");
    
    if (majorNumber != 0)
        unregister_chrdev(majorNumber, name);
}


static ssize_t read(struct file *filePtr, char __user *userBuffer, size_t count, loff_t *offset)
{
    int bytesRead = 0;
    struct timespec64 now;
    struct tm tmNow;
    time64_t seconds;
    
    ktime_get_real_ts64(&now);
    seconds = now.tv_sec;
    time64_to_tm(seconds, 0, &tmNow);

    snprintf(messageBuffer, BUFFER_SIZE, "%04ld-%02d-%02d %02d:%02d:%02d\n",
           tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
           tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);

    size_t messageLength = strlen(messageBuffer);
    if (*offset >= messageLength)
        return 0;

    size_t to_copy = umin(count, messageLength - *offset);
    if (copy_to_user(userBuffer, messageBuffer + *offset, to_copy) != 0)
    {
        printk(KERN_ALERT "Failed to copy time string to user space\n");
        return -EFAULT;
    }

    bytesRead = to_copy;
    *offset += bytesRead;
    return bytesRead;
}

module_init(initDriver);
module_exit(exitDriver);

MODULE_LICENSE("GPL");
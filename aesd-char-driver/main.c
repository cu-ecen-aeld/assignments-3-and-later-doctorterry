/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/types.h>
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("doctorterry"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Piistachyoo");
MODULE_LICENSE("Dual BSD/GPL");

loff_t offset_backup = 0;
uint8_t ioctl_called = 0;
struct aesd_dev aesd_device;
struct class *aesd_class;
struct device *aesd_device_node;
int aesd_open(struct inode *inode, struct file *filp) {
    struct aesd_dev *ptr_aesd_dev;
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    ptr_aesd_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = ptr_aesd_dev;
    if (ioctl_called) {
        ioctl_called = 0;
        filp->f_pos = offset_backup;
    }

    PDEBUG("filp->f_offs = %lld\n", filp->f_pos);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp) {
    PDEBUG("release");
    PDEBUG("filp->f_offs = %lld\n", filp->f_pos);
    // offset_backup = filp->f_pos;
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos) {
    ssize_t retval = 0;
    ssize_t offset;
    struct aesd_buffer_entry *ret_entry = 0;
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    PDEBUG("read %zu bytes with offset %lld\n", count, *f_pos);
    PDEBUG("filp->f_offs = %lld\n", filp->f_pos);
    /**
     * TODO: handle read
     */
    if (mutex_lock_interruptible(&(dev->lock))) {
        PDEBUG("ERROR: Couldn't acquire lock\n");
        retval = -ERESTARTSYS;
        goto inCaseOfFailure;
    }

    ret_entry = aesd_circular_buffer_find_entry_offset_for_fpos(
        &(dev->buffer), *f_pos, &offset);
    PDEBUG("offset return = %ld\n", offset);
    if (ret_entry == NULL) {
        PDEBUG("Not enough data written!\n");
        retval = 0;
        goto inCaseOfFailure;
    }
    retval = ret_entry->size - offset;
    if (copy_to_user(buf, (void *)(ret_entry->buffptr + offset), retval)) {
        PDEBUG("Error copying data to user buffer\n");
        retval = -EFAULT;
        goto inCaseOfFailure;
    } else {
        *f_pos += retval;
    }

    PDEBUG("Data copied: %s", ret_entry->buffptr + offset);
    PDEBUG("new offset: %lld\n", *f_pos);
    PDEBUG("retval: %ld\n", retval);
    PDEBUG("-------------------------------\n");

inCaseOfFailure:
    mutex_unlock(&(dev->lock));
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos) {
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle write
     */
    if (mutex_lock_interruptible(&(dev->lock))) {
        PDEBUG("ERROR: Couldn't acquire lock\n");
        retval = -ERESTARTSYS;
        goto inCaseOfFailure;
    }

    dev->data_buffer.buffptr = krealloc(
        dev->data_buffer.buffptr, dev->data_buffer.size + count, GFP_KERNEL);
    if (dev->data_buffer.buffptr == NULL) {
        PDEBUG("Error allocating memory!\n");
        retval = -ENOMEM;
        goto inCaseOfFailure;
    }

    /* Copy data from user space to kernel space */
    if (copy_from_user(
            (char *)(dev->data_buffer.buffptr + dev->data_buffer.size), buf,
            count)) {
        PDEBUG("Error copying data from user buffer\n");
        retval = -EFAULT;
        goto inCaseOfFailure;
    }
    retval = count;
    dev->data_buffer.size += count;
    *f_pos = *f_pos + retval;

    /* Check if it was ended with endline or not */
    if (dev->data_buffer.buffptr[dev->data_buffer.size - 1] != '\n') {
        PDEBUG("WARNING: Buffer has no end line\n");
        PDEBUG("Skipping data entry\n");
    } else {
        PDEBUG("Data in buffer: %s", dev->data_buffer.buffptr);
        PDEBUG("Size of buffer: %zu\n", dev->data_buffer.size);
        aesd_circular_buffer_add_entry(&(dev->buffer), &(dev->data_buffer));
        PDEBUG("Entry added\n");
        dev->data_buffer.size = 0;
        dev->data_buffer.buffptr = NULL;
    }

    PDEBUG("-------------------------------\n");

inCaseOfFailure:
    mutex_unlock(&(dev->lock));
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t f_offs, int whence) {
    uint8_t index;
    struct aesd_dev *dev = filp->private_data;
    loff_t retval;
    loff_t buffer_size = 0;

    if (mutex_lock_interruptible(&(dev->lock)))
        return -ERESTARTSYS;

    switch (whence) {
    case SEEK_SET: // Use specified offset as file position
        retval = f_offs;
        PDEBUG("Used SEEK_SET to set the offset to %lld\n", retval);
        break;

    case SEEK_CUR: // Increment or decrement file position
        retval = filp->f_pos + f_offs;
        PDEBUG("Used SEEK_CUR to set the offset to %lld\n", retval);
        break;

    case SEEK_END: // Use EOF as file position
        for (index = 0; index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
             index++) {
            if (dev->buffer.entry[index].buffptr) {
                buffer_size += dev->buffer.entry[index].size;
            }
        }
        retval = buffer_size + f_offs;
        PDEBUG("Used SEEK_END to set the offset to %lld\n", retval);
        break;

    default:
        retval = -EINVAL;
        goto inCaseOfFailure;
    }

    if (retval < 0) {
        PDEBUG("Invalid arguments, offset can't be set to %lld\n", retval);
        retval = -EINVAL;
        goto inCaseOfFailure;
    }

    filp->f_pos = retval;

inCaseOfFailure:
    mutex_unlock(&(dev->lock));
    return retval;
}

/**
 * Adjust the file offset (f_pos) parameter of @param filp based on the location
 * specified by
 * @param write_cmd (the zero referenced command to locate)
 * and @param write_cmd_offset (the zero referenced offset into the command)
 * @return 0 if successful, negative if error occured:
 *      -ERESTARTSYS if mutex could not be obtained
 *      -EINVAL if @param write_cmd or @param write_cmd_offset is out of range
 */
static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd,
                                    unsigned int write_cmd_offset) {
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &(dev->buffer);
    uint8_t index;
    long retval;
    long new_offset = 0;

    if (mutex_lock_interruptible(&(dev->lock))) {
        return -ERESTARTSYS;
    }

    /* Validate arguments */
    if ((write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) ||
        (buffer->entry[write_cmd].buffptr == NULL) ||
        (buffer->entry[write_cmd].size <= write_cmd_offset)) {
        retval = -EINVAL;
        PDEBUG("Error: invalid argument\n");
        goto inCaseOfFailure;
    }

    /* Set the offset to the demanded offset */

    /* Seek to the start of write_cmd entry */
    for (index = 0; index < write_cmd; index++) {
        new_offset += buffer->entry[index].size;
    }

    /* Add offset */
    new_offset += write_cmd_offset;
    filp->f_pos = new_offset;
    offset_backup = filp->f_pos;
    ioctl_called = 1;
    retval = 0;
    mutex_unlock(&(dev->lock));

    PDEBUG("Your new f_pos is: %lld", filp->f_pos);
inCaseOfFailure:
    return retval;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    long retval;
    PDEBUG("********************************************\n");
    PDEBUG("ioctl is called with command %u\n", cmd);
    switch (cmd) {
    case AESDCHAR_IOCSEEKTO:
        struct aesd_seekto seekto;
        if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) {
            PDEBUG("ioctl: Error copying data from user\n");
            retval = -EFAULT;
        } else {
            PDEBUG("ioctl: Adjusting file offset to %u, %u\n", seekto.write_cmd,
                   seekto.write_cmd_offset);
            retval = aesd_adjust_file_offset(filp, seekto.write_cmd,
                                             seekto.write_cmd_offset);
        }
        break;
    default:
        PDEBUG("ioctl: Wrong command\n");
        retval = -EINVAL;
    }
    PDEBUG("********************************************\n");
    return retval;
}

struct file_operations aesd_fops = {.owner = THIS_MODULE,
                                    .read = aesd_read,
                                    .write = aesd_write,
                                    .open = aesd_open,
                                    .release = aesd_release,
                                    .llseek = aesd_llseek,
                                    .unlocked_ioctl = aesd_ioctl};

static int aesd_setup_cdev(struct aesd_dev *dev) {
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err) {
        PDEBUG("Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void) {
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        // printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        PDEBUG("Can't get major %d\n", aesd_major);
        goto deviceNumberFail;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    result = aesd_setup_cdev(&aesd_device);
    if (result) {
        goto cdevRegisterFail;
    }

    aesd_circular_buffer_init(&aesd_device.buffer);

    aesd_device.data_buffer.buffptr = kmalloc(0, GFP_KERNEL);
    aesd_device.data_buffer.size = 0;

    return 0;

cdevRegisterFail:
    unregister_chrdev_region(dev, 1);
deviceNumberFail:
    return result;
}

void aesd_cleanup_module(void) {
    uint8_t index;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    for (index = 0; index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; index++) {
        if (aesd_device.buffer.entry[index].buffptr != NULL) {
            kfree(aesd_device.buffer.entry[index].buffptr);
        }
    }

    if (aesd_device.data_buffer.buffptr != NULL) {
        kfree(aesd_device.data_buffer.buffptr);
    }

    cdev_del(&(aesd_device.cdev));
    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
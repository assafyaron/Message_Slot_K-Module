// Include the necessary libraries
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "message_slot.h"

// Define the module information
MODULE_LICENSE("GPL");

/*
=========== This is a kernel module implementing the message slot IPC mechanism ===========
*/

/* =========== global variables =========== */

static struct message_slot* all_slots[MAX_MINOR_NUM];

/* =========== Helper Functions =========== */

int legal_minor(int minor) {
    /*
    Check if the minor number is legal (between 0 and MAX_MINOR_NUM)

    Args:
        minor: minor number

    Returns:
        1 if legal, 0 if not legal
    */
    printk(KERN_INFO "Checking legal_minor helper function\n");

    return ((minor >= 0) && (minor < MAX_MINOR_NUM));
}

struct channel* find_channel(struct message_slot* slot, unsigned long channel_id) {
    /*
    Find a channel in the message slot channels list

    Args:
        slot: message slot struct
        channel_id: channel id

    Returns:
        channel struct if found, NULL if not found 
    */

    struct channel* temp;
    temp = slot->next;

    printk(KERN_INFO "Entering find_channel helper function\n");

    while (temp != NULL) {
        if (temp->channel_id == channel_id) {
            printk(KERN_INFO "Find_channel exits success, channel found\n");
            return temp;
        }
        temp = temp->next;
    }
    printk(KERN_INFO "Find_channel exits success, channel not found\n");
    return NULL;
}

int insert_newchannel(struct message_slot* slot, unsigned long channel_id) {
    /*
    Insert a new channel to the message slot channels list

    Args:
        slot: message slot struct
        channel_id: channel id

    Returns:
        0 on success, -ENOMEM on failure to allocate memory for new channel
    */

    struct channel* new_channel;
    struct channel* temp;

    printk(KERN_INFO "Entering insert_channel helper function\n");

    // Allocate memory for new channel
    new_channel = kmalloc(sizeof(struct channel), GFP_KERNEL); // Flag: GFP_KERNEL = memory allocation in kernel space
    if (new_channel == NULL) {
        printk(KERN_ERR "Failed to allocate memory for new_channel\n");
        return -ENOMEM;
    }

    new_channel->channel_id = channel_id;
    new_channel->next = NULL;
    
    // Allocate memory for new channel message
    new_channel->message = kmalloc((1+MAX_MESSAGE_SIZE)*sizeof(char), GFP_KERNEL); // Flag: GFP_KERNEL = memory allocation in kernel space
    if (new_channel->message == NULL) {
        kfree(new_channel);
        printk(KERN_ERR "Failed to allocate memory for new_channel->message\n");
        return -ENOMEM;
    }

    // Save the tail of the channels list
    if (slot->tail == NULL) {
        slot->tail = new_channel;
        slot->next = new_channel;
    }
    else {
        temp = slot -> tail;
        // Insert channel to the message_slot channels list and update the tail
        temp->next = new_channel;
        slot->tail = new_channel;
    }

    printk(KERN_INFO "Insert_channel exits success\n");

    return 0;
}

void freechannels(struct message_slot* slot) {
    /*
    Free all channels in the message slot and the message in each channel
    Used in the cleanup function

    Args:
        slot: message slot struct

    Returns:
        None 
    */

    struct channel* temp = slot->next;
    struct channel* next;

    printk(KERN_INFO "Entering free_channels helper function\n");

    while (temp != NULL) {
        next = temp->next;
        kfree(temp->message);
        kfree(temp);
        temp = next;
    }
    slot->next = NULL;

    printk(KERN_INFO "Free_channels exits success\n");
}

/* =========== File operations functions =========== */

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset) {
    /*
    Read a message from the channel of the message slot

    Args:
        file: file struct
        buffer: user buffer
        length: length of the message
        offset: offset of the message

    Returns:
        length on success, 
        -EINVAL on illegal variables,
        -EWOULDBLOCK on empty message,
        -ENOSPC on buffer too small,
        -EFAULT on failure to copy message to user buffer
    */

    struct message_slot* slot;
    struct channel* channel;
    unsigned long channel_id;
    int minor;
    ssize_t message_length;
    
    printk(KERN_INFO "Entering device_read\n");

    // Verify variables given by the user
    if (length == 0) {
        printk(KERN_ERR "Illegal length: %ld\n", length);
        return -EINVAL;}
    if (buffer == NULL) {
        printk(KERN_ERR "Illegal buffer: NULL\n");
        return -EINVAL;}
    if (file == NULL) {
        printk(KERN_ERR "Illegal file: NULL\n");
        return -EINVAL;}

    minor = iminor(file->f_inode);
    if (!legal_minor(minor)) {
        printk(KERN_ERR "Illegal minor number: %d\n", minor);
        return -EINVAL;}

    channel_id = (unsigned long)(uintptr_t)file->private_data;
    slot = all_slots[minor];
    channel = find_channel(slot,channel_id);

    if (channel == NULL) {
        printk(KERN_ERR "No channel was set for this file\n");
        return -EINVAL;}

    // Make sure message was initialized
    if (channel->message == NULL) {
        printk(KERN_ERR "Message is empty\n");
        return -EWOULDBLOCK;}

    // Get the length of the message
    message_length = strlen(channel->message);

    if (message_length == 0) {
        printk(KERN_ERR "No message exists\n");
        return -EWOULDBLOCK;}

    if (length < message_length) {
        printk(KERN_ERR "Buffer is too small\n");
        return -ENOSPC;}

    // Copy the message to the user buffer
    if (copy_to_user(buffer, channel->message, message_length) != 0) {
        printk(KERN_ERR "Failed to copy message to user buffer\n");
        return -EIO;
    }

    // Null-terminate in user buffer
    if (copy_to_user(buffer + message_length, "\0", 1) != 0) {
        printk(KERN_ERR "Failed to copy null terminator\n");
        return -EIO;
    }
    
    printk(KERN_INFO "Device_read exits success\n");

    // Return the number of bytes read
    return message_length;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    /*
    Write a message to the channel of the message slot

    Args:
        file: file struct
        buffer: user buffer
        length: length of the message
        offset: offset of the message

    Returns:
        length on success, 
        -EINVAL on illegal variables,
        -ENOMEM on failure to allocate memory for new channel message,
        -EMSGSIZE on illegal message size
        -EFAULT on failure to copy message from user buffer 
    */

    struct message_slot* slot;
    struct channel* channel;
    unsigned long channel_id;
    int minor;
    char* kernel_buffer;
    
    printk(KERN_INFO "Entering device_write\n");

    // Verify variables given by the user
    if (buffer == NULL) {
        printk(KERN_ERR "Illegal buffer: NULL\n");
        return -EINVAL;}
    if (file == NULL) {
        printk(KERN_ERR "Illegal file: NULL\n");
        return -EINVAL;}

    minor = iminor(file->f_inode);
    if (!legal_minor(minor)) {
        printk(KERN_ERR "Illegal minor number: %d\n", minor);
        return -EINVAL;}

    channel_id = (unsigned long)(uintptr_t)file->private_data;
    slot = all_slots[minor];
    channel = find_channel(slot,channel_id);

    if (channel == NULL) {
        printk(KERN_ERR "No channel was set for this file\n");
        return -EINVAL;}

    if ((length > MAX_MESSAGE_SIZE) || (length == 0)) {
        printk(KERN_ERR "Illegaly sized message: %ld\n",length);
        return -EMSGSIZE;}

    // Allocate memory for midway buffer
    kernel_buffer = kmalloc(length + 1, GFP_KERNEL);
    if (kernel_buffer == NULL) {
        printk(KERN_ERR "Failed to allocate kernel buffer\n");
        kfree(channel->message);
        channel->message = NULL;
        return -ENOMEM;
    }

    // Copy the message from the user buffer to the kernel buffer
    if (copy_from_user(kernel_buffer, buffer, length) != 0) {
        printk(KERN_ERR "Failed to copy message from user buffer\n");
        kfree(kernel_buffer);
        return -EIO;
    }

    // Copy message on success and free kernel buffer
    strncpy(channel->message, kernel_buffer, MAX_MESSAGE_SIZE);
    kfree(kernel_buffer);

    printk(KERN_ERR "Device_write exits success\n");

    // Return the number of bytes written
    return length;
}

static int device_open(struct inode* inode, struct file* file) {
    /*
    Init a new message slot if it doesn't exist, or reuse the existing message slot

    Args:
        inode: inode struct
        file: file struct

    Returns:
        0 on success, -EINVAL on illegal variables, -ENOMEM on failure to allocate memory for new message slot
    */

    int minor;
    struct message_slot* slot;

    printk(KERN_INFO "Entering device_open\n");

    // Verify variables given by the user
    if (inode == NULL) {
        printk(KERN_ERR "Illegal inode: NULL\n");
        return -EINVAL;}
    if (file == NULL) {
        printk(KERN_ERR "Illegal file: NULL\n");
        return -EINVAL;}

    minor = iminor(inode);
    if (!legal_minor(minor)) {
        printk(KERN_ERR "Illegal minor number: %d\n", minor);
        return -EINVAL;}

    // Check if a message slot with the same minor number already exists
    if (all_slots[minor] != NULL) {
        printk(KERN_ERR "Reusing existing message_slot for minor: %d\n",minor);
        return 0; // Success
    }

    // Else, create a new message slot
    slot = kmalloc(sizeof(struct message_slot), GFP_KERNEL); // Flag: GFP_KERNEL = memory allocation in kernel space
    if (slot == NULL) {
        printk(KERN_ERR "Failed to allocate memory for message_slot\n");
        return -ENOMEM;
    }

    // update all_slots array
    all_slots[minor] = slot;
    // Init message_slot fields
    slot->channel_id = 0;
    slot->next = NULL;
    slot->tail = NULL;
    
    printk(KERN_INFO "Device_open exits success\n");

    return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param) {
    /*
    Init a new channel in the message slot if it doesn't exist, or set the current channel to the given channel_id
    The channel_id will be saved in the file->private_data field and will be used in the read and write functions

    Args:
        file: file struct
        ioctl_command_id: ioctl command id
        ioctl_param: channel_id

    Returns:
        0 on success, -EINVAL on illegal variables, -ENOMEM on failure to allocate memory for new channel
    */
    struct message_slot* slot;
    struct channel* channel;
    int minor;
    unsigned long channel_id;
    int flag;

    printk(KERN_INFO "Entering device_ioctl\n");

    // Verify variables given by the user
    if (file == NULL) {
        printk(KERN_ERR "Illegal file: NULL\n");
        return -EINVAL;}
    // Make sure the ioctl command is for the message slot channel
    if (ioctl_command_id != MSG_SLOT_CHANNEL) {
        printk(KERN_ERR "Illegal ioctl command id: %d\n", ioctl_command_id);
        return -EINVAL;}
    // Make sure the ioctl_param is not 0
    if (ioctl_param == 0) {
        printk(KERN_ERR "Illegal ioctl_param (channel): %ld\n", ioctl_param);
        return -EINVAL;}

    channel_id = ioctl_param;
    minor = iminor(file->f_inode);
    if (!legal_minor(minor)) {
        printk(KERN_ERR "Illegal minor number: %d\n", minor);
        return -EINVAL;}

    slot = all_slots[minor];
    channel = find_channel(slot, channel_id);

    if (channel != NULL) {
        file->private_data = (void *)(uintptr_t)channel_id; 
    }
    else { // Add the new channel to the message slot "channels list"
        flag = insert_newchannel(slot, channel_id);
        if (flag < 0) {
            return flag;
        }
        file->private_data = (void *)(uintptr_t)channel_id;
    }

    printk(KERN_INFO "Device_ioctl exits success\n");    

    return 0;
}

// File operations struct
static struct file_operations message_slot_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
};

/* =========== Module loader and unloader functions =========== */

// loader
static int __init message_slot_init(void) {
    /*
    Loader function for the message slot module, registers the device and initializes all_slots array

    Args:
        None

    Returns:
        0 on success, -EFAULT on failure to register the device
    */

    int i;
    int register_val;

    printk(KERN_INFO "Entering message_slot module init\n");

    // Init all_slots array entries to NULL
    for (i = 0; i < MAX_MINOR_NUM; i++) {
        all_slots[i] = NULL;
    }

    register_val = register_chrdev(MAJOR_NUM, "message_slot", &message_slot_fops);
    if (register_val < 0) {
        printk(KERN_ERR "Failed to register the device\n");
        return -EFAULT;
    }
    
    printk(KERN_INFO "Message_slot module init exits success\n");

    return 0;
}

// unloader
static void __exit message_slot_cleanup(void) {
    /*
    Cleanup function for the message slot module, frees all allocated memory and unregisters the device

    Args:
        None

    Returns:
        None
    */

    int i;

    printk(KERN_INFO "Entering message_slot module cleanup\n");

    /* 
    =========== Free Memory =========== 
    Free all_slots entries:
        0. message_slots:
            1. free channels:
                2.1 free message
                2.2 free channel->next
                2.3 free channel
        1. free message_slot
    */
    for (i = 0; i < MAX_MINOR_NUM; i++) {
        if (all_slots[i] != NULL) {
            freechannels(all_slots[i]);
            kfree(all_slots[i]);
        }
    }

    unregister_chrdev(MAJOR_NUM, "message_slot");

    printk(KERN_INFO "Message_slot module cleanup exits success\n");
}

// Register the loader and unloader functions
module_init(message_slot_init);
module_exit(message_slot_cleanup);


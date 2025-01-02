#ifndef _MESSAGE_SLOT_H_
#define _MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MAX_MESSAGE_SIZE 128
#define MAX_MINOR_NUM 256

// The ioctl command to set the channel id
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#endif // _MESSAGE_SLOT_H_
#ifndef _MESSAGE_SLOT_H_
#define _MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MAX_MESSAGE_SIZE 128
#define MAX_MINOR_NUM 256

// The ioctl command to set the channel id
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

/* =========== Structs =========== */

struct message_slot {
    /*
    Struct to describe individual message slots (device files with different minor numbers)
    Contains a list of channels in the message slot
    */
    unsigned long channel_id;
    struct channel* next;
    struct channel* tail;
}; 

struct channel {
    /*
    Struct to describe individual channels in a message slot
    */
    char* message;
    unsigned long channel_id;
    struct channel* next;
};

#endif // _MESSAGE_SLOT_H_

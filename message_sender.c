#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]) {
    /*
    Message sender program to write a message to a channel in a message slot device file

    Args:
        argv[1]: device file
        argv[2]: channel id
        argv[3]: message

    Returns:
        0 on success, 1 on failure
    */

    int fd;
    unsigned int channel_id;
    char* buffer;
    ssize_t bytes_written;

    // Check for correct number of arguments
    if (argc != 4) {
        perror("Insufficient arguments in: message_sender <device_file> <channel_id> <message>\n");
        exit(1);}

    // Parse values from arguments
    channel_id = atoi(argv[2]);
    fd = open(argv[1], O_RDWR);
    buffer = argv[3];

    if (fd < 0) {
        perror("Failed to open device file\n");
        exit(1);}

    // Open the channel using icotl function
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("Failed to open channel\n");
        close(fd);
        exit(1);
    }

    // Write the message to the channel
    bytes_written = write(fd, buffer, strlen(argv[3]));
    if (bytes_written < 0) {
        perror("Failed to write message to channel\n");
        close(fd);
        exit(1);
    }

    // Close device file
    close(fd);

    exit(0); // Success
}

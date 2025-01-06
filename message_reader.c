#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "message_slot.h"

int main(int argc, char *argv[]) {
    /*
    Message reader program to read a message from a channel in a message slot device file

    Args:
        argv[1]: device file
        argv[2]: channel id

    Returns:
        0 on success, 1 on failure
    */

    int fd;
    unsigned long channel_id;
    char buffer[MAX_MESSAGE_SIZE];
    ssize_t bytes_read;

    if (argc != 3) {
        perror("Insufficient arguments in: message_reader <device_file> <channel_id>\n");
        exit(1);}

    // Parse values from arguments
    channel_id = atoi(argv[2]);
    fd = open(argv[1], O_RDWR);

    if (fd < 0) {
        perror("Failed to open device file\n");
        exit(1);}

    // Open the channel using icotl function
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("Failed to open channel\n");
        close(fd);
        exit(1);
    }

    // Read the message from the channel
    bytes_read = read(fd, buffer, MAX_MESSAGE_SIZE);
    if (bytes_read < 0) {
        perror("Failed to read message from channel\n");
        close(fd);
        exit(1);
    }

    // Close device file
    close(fd);

    // Print the message to standard output
    if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
        perror("Failed to write message to standard output\n");
        exit(1);
    }

    exit(0);
}

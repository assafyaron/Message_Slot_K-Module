# Message Slot Kernel Module

## From A Bird's View

This project implements a message slot kernel module that facilitates inter-process communication (IPC) via message slots. The module allows user-space programs to send and receive messages through device files representing message slots with different minor numbers.

## File Structure

- `Makefile`: Makefile to bundle and compile
- `message_slot.c`: Kernel module
- `message_reader.c`: User program reading messages and printing to STDOUT
- `message_sender.c`: User program sending messages
- `message_slot.h`: Header file
- `tester1.c`: Verifies edge cases and interfunction logic
- `tester2.c`: Verfies writes and reads over large random data
- `tester3.c`: Verifies kernel module

## Setup, Saftey and Workflow

Make sure to run this code on VM, beacuse it can casue serious harm to your private computer.

1. Compile the module and user programs:
```
make
//this runs gcc -O3 -Wall -std=c11 <user_programs>
```
2. Load kernel module:
```
sudo insmod message_slot.ko
```
3. Create device file:
```
sudo mknod <device_file> c <MAJOR#> <SLOT#>
<MAJOR#> := 235, <device_file> := /dev/your_slot_name, 0 <= SLOT# <= 256
```
4. Change permissions:
```
sudo chmod o+rw <device_file>
```
5. Call message_sender:
```
./message_sender <device_file> <channel_id> <message>
```
6. Call message_reader:
```
./message_reader <device_file> <channel_id>
```
7. Call tester:
```
gcc -O3 -Wall -std=c11 tester#.c -o tester#
./tester <device_file>
```

Shortcut to running tests (loads->tests->unloads):
```
make test1
make test2
make test3
```

## Kernel Module: `message_slot.c`

The kernel module implements the following key operations:
- **open**: Opens a message slot device, initializing a new slot if it does not exist.
- **write**:Writes a message to a specified channel.
- **read**: Reads a message from a specified channel.
- **ioctl**: Sets or retrieves the channel ID for the current file.

### Important Features
- The kernel module supports multiple (256) message slots.
- Each message slot can support un-bounded number of channels

## Error Handling
return -errno, and system will derive this as return value of -1:

1. **EINVAL**: Invalid arguments, such as illegal file descriptors, invalid minor numbers, or invalid channel IDs.
2. **ENOMEM**: Failed to allocate memory for new channels or messages.
3. **EWOULDBLOCK**: No message available to read from the channel.
4. **EMSGSIZE**: Message size is either too large or too small for the allocated buffer.
5. **EFAULT**: Failure to copy data between user space and kernel space.

## Cleanup

To remove the kernel module and clean up the compiled files, run:
```
make clean
sudo rmmod message_slot
```

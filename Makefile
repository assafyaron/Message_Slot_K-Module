obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -O3 -Wall -std=c11 message_sender.c -o message_sender
	gcc -O3 -Wall -std=c11 message_reader.c -o message_reader

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f message_sender message_reader tester1 tester2 tester3

load:
	make
	sudo insmod message_slot.ko
	lsmod | grep message_slot

test1:
	make load
	sudo mknod /dev/msgslot1 c 235 1
	sudo chmod o+rw /dev/msgslot1
	gcc -O3 -Wall -std=c11 tester1.c -o tester1
	./tester1 /dev/msgslot1
	make unload12

test2:
	make load
	sudo mknod /dev/msgslot1 c 235 1
	sudo chmod o+rw /dev/msgslot1
	gcc -O3 -Wall -std=c11 tester2.c -o tester2
	./tester2 /dev/msgslot1
	make unload12

test3:
	make load
	sudo mknod /dev/test0 c 235 0
	sudo mknod /dev/test1 c 235 1
	sudo chmod 0777 /dev/test0
	sudo chmod 0777 /dev/test1
	gcc -O3 -Wall -std=c11 tester3.c -o tester3
	./tester3
	make unload3

unload12:
	sudo rm /dev/msgslot1
	sudo rmmod message_slot
	make clean

unload3:
	sudo rm /dev/test0
	sudo rm /dev/test1
	sudo rmmod message_slot
	make clean

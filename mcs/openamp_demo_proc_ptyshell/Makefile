rpmsg_main: rpmsg_main.o rpmsg_linux_endpoint.o rpmsg_rtos_endpoint.o
	$(CC) rpmsg_main.o rpmsg_linux_endpoint.o rpmsg_rtos_endpoint.o -g $(SDKTARGETSYSROOT)/usr/lib64/libmetal.so $(SDKTARGETSYSROOT)/usr/lib64/libopen_amp.so  $(SDKTARGETSYSROOT)/lib64/libsysfs.so -o rpmsg_main

rpmsg_main.o: rpmsg_main.c rpmsg-internal.h
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_main.c -o rpmsg_main.o

rpmsg_linux_endpoint.o: rpmsg_linux_endpoint.c rpmsg-internal.h
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_linux_endpoint.c -o rpmsg_linux_endpoint.o

rpmsg_rtos_endpoint.o: rpmsg_rtos_endpoint.c
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_rtos_endpoint.c -o rpmsg_rtos_endpoint.o

clean:
	rm -rf *.o rpmsg_main

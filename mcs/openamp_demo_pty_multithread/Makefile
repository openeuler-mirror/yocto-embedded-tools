rpmsg_main: rpmsg_main.o rpmsg_ping.o rpmsg_pty.o
	$(CC) -g rpmsg_main.o rpmsg_ping.o rpmsg_pty.o $(SDKTARGETSYSROOT)/usr/lib64/libmetal.so $(SDKTARGETSYSROOT)/usr/lib64/libopen_amp.so  $(SDKTARGETSYSROOT)/lib64/libsysfs.so $(SDKTARGETSYSROOT)/lib64/libpthread.so.0 -o rpmsg_main

rpmsg_main.o: rpmsg_main.c rpmsg-internal.h
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_main.c -o rpmsg_main.o

rpmsg_ping.o: rpmsg_ping.c rpmsg-internal.h
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_ping.c -o rpmsg_ping.o

rpmsg_pty.o: rpmsg_pty.c
	$(CC) -g -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_pty.c -o rpmsg_pty.o

clean:
	rm -rf *.o rpmsg_main

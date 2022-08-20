rpmsg_main: clean rpmsg_main.o rpmsg_ping.o rpmsg_app.o
	$(CC) rpmsg_main.o rpmsg_ping.o rpmsg_app.o $(SDKTARGETSYSROOT)/usr/lib64/libmetal.so $(SDKTARGETSYSROOT)/usr/lib64/libopen_amp.so  $(SDKTARGETSYSROOT)/lib64/libsysfs.so -o rpmsg_main

rpmsg_main.o: rpmsg_main.c rpmsg-internal.h
	$(CC) -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_main.c -o rpmsg_main.o

rpmsg_ping.o: rpmsg_ping.c rpmsg-internal.h
	$(CC) -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_ping.c -o rpmsg_ping.o

rpmsg_app.o: rpmsg_ping.c rpmsg-internal.h
	$(CC) -I$(SDKTARGETSYSROOT)/usr/include -c rpmsg_app.c -o rpmsg_app.o

clean:
	rm -rf *.o rpmsg_main

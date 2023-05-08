CFLAGS = -I. -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
CFLAGS += -g -Werror=return-type

LDLIBS = -lglib-2.0 -lpthread

HEADERS = misc/*.h proto/*.h
TARGETS = vclient
VCLIENT_OBJS = vclient.o proto/tuntap.o platform/portable_network.o

all: ${TARGETS}

vclient: ${VCLIENT_OBJS} $(HEADERS)

*.o: $(HEADERS)

clean:
	rm -f ${VCLIENT_OBJS} ${TARGETS}
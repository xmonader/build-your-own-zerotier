CFLAGS = -I.
CFLAGS += -Werror=return-type

LDLIBS = -lpthread

HEADERS = sys_utils.h tap_utils.h
TARGETS = vclient
VCLIENT_OBJS = vclient.o tap_utils.o

all: ${TARGETS}

vclient: ${VCLIENT_OBJS} $(HEADERS)

*.o: $(HEADERS)

clean:
	rm -f ${VCLIENT_OBJS} ${TARGETS}
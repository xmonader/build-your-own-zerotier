CFLAGS = -I.
CFLAGS += -g -Werror=return-type

HEADERS = *.h misc/*.h proto/*.h
TARGETS = vclient

all: ${TARGETS}

vclient: tuntap.o vclient.o $(HEADERS)

*.o: $(HEADERS)

clean:
	rm -f *.o ${TARGETS}
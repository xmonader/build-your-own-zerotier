CFLAGS = -I.
CFLAGS += -g -Werror=return-type

HEADERS = misc/*.h *.h
TARGETS = main

all: ${TARGETS}

main: tuntap.o main.o $(HEADERS)

*.o: $(HEADERS)

clean:
	rm -f *.o ${TARGETS}
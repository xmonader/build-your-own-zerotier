CFLAGS = -I.
CFLAGS += -g -Werror=return-type

HEADERS = *.h misc/*.h proto/*.h
TARGETS = main

all: ${TARGETS}

main: tuntap.o main.o $(HEADERS)

*.o: $(HEADERS)

clean:
	rm -f *.o ${TARGETS}
CC = $(CROSS_COMPILE)gcc

CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)

PRODUCT= aesdsocket

CFILES= aesdsocket.c

SRCS= ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

writer:	aesdsocket.o
	$(CC) $(CFLAGS) -o $@ aesdsocket.o $(LIBS)

clean:
	rm -f *.o 
	rm -f ${PRODUCT}

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
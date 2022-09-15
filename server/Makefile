INCLUDE_DIRS = 
LIB_DIRS = 

CC = $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT= aesdsocket

HFILES=
CFILES= aesdsocket.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

writer:	aesdsocket.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ aesdsocket.o $(LIBS)

clean:
	rm -f *.o 
	rm -f ${PRODUCT}

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

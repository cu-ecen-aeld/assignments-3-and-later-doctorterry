INCLUDE_DIRS = 
LIB_DIRS = 

CC = $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT= writer

HFILES=
CFILES= writer.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

writer:	writer.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ writer.o $(LIBS)

clean:
	rm -f *.o 
	rm -f ${PRODUCT}

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
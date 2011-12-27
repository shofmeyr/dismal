#CC=upcc
#CFLAGS=-pthreads=1 -DEMPLOY_VERSION=0.1 -network=smp 
CC=gcc
CFLAGS=-DEMPLOY_VERSION=0.1 -std=gnu99
#LDFLAGS=-network=smp -pthreads=4 -nolink-cache
DFLAGS=
SOURCES=dismal.c cfg.c utils.c
HEADERS=cfg.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dismal

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	etags *.c *.h

$(OBJECTS): $(HEADERS)

.c.o:
	$(CC) $(CFLAGS) -c $< 

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE) TAGS *pthread-link


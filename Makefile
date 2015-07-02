CC=gcc
REMOVE=rm
CFLAGS=-c -O2 -ansi -Wall -pedantic -Wextra -Wall
LDFLAGS=-s
SOURCES=main.c pmdx2obj.c dxFileIO.c bmpconvert.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dxPmdxConverter

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	$(REMOVE) $(OBJECTS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(REMOVE) $(OBJECTS)
	$(REMOVE) $(EXECUTABLE)

upx:
	upx --ultra-brute $(EXECUTABLE) && mv $(EXECUTABLE) bin/$(EXECUTABLE).linux64.bin


macos:
	make -f Make.macos
	make -f Make.macos upx

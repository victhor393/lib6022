CC = gcc
CFLAGS = -lusb-1.0 -O2 -Wall

LIBS = ../lib
INCL = ../include
LIB_NAME = lib6022.so
TARGET = test_lib6022

all: $(TARGET)

$(LIBS)/$(LIB_NAME): lib6022.c $(INCL)/lib6022.h
	$(CC) -shared -fPIC $(CFLAGS) -o $@ $<

$(TARGET): test_lib6022.c $(LIBS)/$(LIB_NAME)
	$(CC) $(CFLAGS) -l6022 -I$(INCL) -L$(LIBS) -o $@ $<

.PHONY: clean

clean:
	rm -f obj/*.o ../lib/*.so $(TARGET)

#CC = arm-linux-gnueabihf-gcc
CFLAGS = -lusb-1.0 -O3 -g -Wall -mavx -mtune=native -fopt-info-vec-optimized -ftree-vectorize -ffast-math
#CFLAGS = -lusb-1.0 -O0 -g -Wall
#CFLAGS= -lusb-1.0 -O3 -Wall -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a15 -funsafe-math-optimizations -mvectorize-with-neon-double -ftree-vectorize -ffast-math -funroll-loops -fopt-info-vec-optimized
LIBS = ../lib
INCL = ../include
LIB_NAME = lib6022.so
#TARGET = test_lib6022

all: $(TARGET) test

$(LIBS)/$(LIB_NAME): lib6022.c $(INCL)/lib6022.h
	$(CC) -shared -fPIC $(CFLAGS) -o $@ $<

#$(TARGET): test_lib6022.c $(LIBS)/$(LIB_NAME)
#	$(CC) $(CFLAGS) -l6022 -I$(INCL) -L$(LIBS) -o $@ $<

#test: test.c $(LIBS)/$(LIB_NAME)
#	$(CC) $(CFLAGS) -pthread -l6022 -I$(INCL) -L$(LIBS) -o $@ $<
#pthreads_lib6022: pthreads_lib6022.c $(LIBS)/$(LIB_NAME)
#	$(CC) $(CFLAGS) -pthread -l6022 -I$(INCL) -L$(LIBS) -o $@ $<
#pthreads_lib6022_ORG: pthreads_lib6022_ORG.c $(LIBS)/$(LIB_NAME)
#	$(CC) $(CFLAGS) -pthread -l6022 -I$(INCL) -L$(LIBS) -o $@ $<

.PHONY: clean

clean:
	rm -f obj/*.o ../lib/*.so $(TARGET)

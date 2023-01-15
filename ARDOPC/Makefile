#	ARDOPC Makefile

OBJS = LinSerial.o KISSModule.o pktARDOP.o pktSession.o BusyDetect.o i2cDisplay.o ALSASound.o ARDOPC.o ardopSampleArrays.o ARQ.o FFT.o FEC.o HostInterface.o Modulate.o rs.o berlekamp.o galois.o SoundInput.o TCPHostInterface.o SCSHostInterface.o

# Configuration:
CFLAGS = -DLINBPQ -MMD -g 
LDFLAGS := -Xlinker -Map=output.map -lrt -lm -lpthread -lasound 
LDFLAGS += -lhamlib
CC = gcc

vpath %.c /mnt/ardopc
vpath %.h /mnt/ardopc
vpath %.o ./
			                       
all: ardopc
			
ardopc: $(OBJS)
	gcc $(OBJS) ${LDFLAGS} -o ardopc

-include *.d

clean :
	rm -f ardopc $(OBJS) output.map
	rm -f *.d

WARNING_FLAGS=-Wall -Wextra -Wno-sign-compare 
OPT_FLAGS=-O3 -DNDEBUG
CXXFLAGS=-msse4.2 $(WARNING_FLAGS) $(OPT_FLAGS)

# Makes the linker g++
CC=g++

BINARIES=crc32c_test crc32cbench
all: $(BINARIES)

crc32c_test: crc32c_test.o crc32ctables.o crc32c.o stupidunit.o
crc32cbench: crc32cbench.o crc32ctables.o crc32c.o

clean:
	$(RM) $(BINARIES) *.o

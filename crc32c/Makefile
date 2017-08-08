ifdef CPU_TYPE
ifeq ($(CPU_TYPE),x32)
  LBITS := 32
  BITS=-m32
endif
ifeq ($(CPU_TYPE),amd64)
  LBITS := 64
  BITS=-m64
endif
else
  LBITS := $(shell getconf LONG_BIT)
endif

OBJECTS = crc32ctables.o crc32c.o stupidunit.o crc32intelc.o crc32inteltable.o crc32adler.o

ifeq ($(LBITS),64)
   OBJECTS += crc32intelasm.o crc_iscsi_v_pcl.o
else
   # do 32 bit stuff here
endif

ifdef DEBUG
  OPT_FLAGS= -g -O0
else
  OPT_FLAGS=-O3 -DNDEBUG -flto
endif

WARNING_FLAGS=-Wall -Wextra -Wno-sign-compare 
CXXFLAGS+=-msse4.2 -mpclmul $(BITS) $(WARNING_FLAGS) $(OPT_FLAGS)
CFLAGS+=-msse4.2 -mpclmul $(BITS) $(WARNING_FLAGS) $(OPT_FLAGS)

BINARIES=crc32c_test crc32cbench
all: $(BINARIES)

crc32c_test: crc32c_test.o $(OBJECTS)
	$(CXX) -o $@ $^ $(CXXFLAGS)
	
crc32cbench: crc32cbench.o $(OBJECTS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	$(RM) $(BINARIES) *.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

crc_iscsi_v_pcl.o: crc_iscsi_v_pcl.asm
	yasm -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX -o crc_iscsi_v_pcl.o crc_iscsi_v_pcl.asm

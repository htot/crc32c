// Copyright 2008,2009,2010 Massachusetts Institute of Technology.
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <cassert>
#include <cstdio>

#include "logging/crc32c.h"
#include "stupidunit/stupidunit.h"

using namespace logging;

TEST(CRC32C, CPUDetection) {
    CRC32CFunctionPtr initial = crc32c;
    /* uint32_t crc = */ crc32c(crc32cInit(), NULL, 0);
    // These should not be equal!
    CRC32CFunctionPtr final = crc32c;
    EXPECT_NE(initial, final);

    // Calling the function again does not change it
    /* crc = */ crc32c(crc32cInit(), NULL, 0);
    EXPECT_EQ(final, crc32c);

    EXPECT_EQ(final, detectBestCRC32C());
}

struct CRC32CFunctionInfo {
    CRC32CFunctionPtr crcfn;
    const char* name;
};

#define MAKE_FN_STRUCT(x) { x, # x }
static const CRC32CFunctionInfo FNINFO[] = {
    MAKE_FN_STRUCT(crc32cSarwate),
    MAKE_FN_STRUCT(crc32cSlicingBy4),
    MAKE_FN_STRUCT(crc32cSlicingBy8),
    MAKE_FN_STRUCT(crc32cHardware32),
#ifdef __LP64__
    MAKE_FN_STRUCT(crc32cHardware64),
    MAKE_FN_STRUCT(crc32cIntelAsm),
#endif
    MAKE_FN_STRUCT(crc32cAdler),
    MAKE_FN_STRUCT(crc32cIntelC),
};
#undef MAKE_FN_STRUCT

static size_t numValidFunctions() {
    size_t numFunctions = sizeof(FNINFO)/sizeof(*FNINFO);
    bool hasHardware = (detectBestCRC32C() != crc32cSlicingBy8);
    if (!hasHardware) {
        while (FNINFO[numFunctions-1].crcfn == crc32cHardware32 ||
                FNINFO[numFunctions-1].crcfn == crc32cHardware64 ||
                FNINFO[numFunctions-1].crcfn == crc32cIntelC) {
            numFunctions -= 1;
        }
    }
    return numFunctions;
}
static const size_t NUM_VALID_FUNCTIONS = numValidFunctions();

static bool check(const CRC32CFunctionInfo& fninfo, const void* data, size_t length, uint32_t value) {
    uint32_t crc = fninfo.crcfn(crc32cInit(), data, length);
    crc = crc32cFinish(crc);
    if (crc != value) {
        printf("Function %s failed; expected: 0x%08x actual: 0x%08x. ", fninfo.name, value, crc);
	switch(*(char *)data) {
	  case '1':
	    printf("Test: NUMBERS len=%d\n", (int)length);
	    break;
	  case '2':
	    printf("Test: NUMBERS not aligned len=%d\n", (int)length);
	    break;
	  case 'T':
	    printf("Test: PHRASE len=%d\n", (int)length);
	    break;
	  case 'L':
	    printf("Test: LONGPHRASE len=%d\n", (int)length);
	    break;
	  default:
	    printf("Test: VARSIZE len=%d\n", (int)length);
	};
        return false;
    }
    return true;
}

#define CHECK_SIZE 10000

TEST(CRC32C, KnownValues) {
    static const char NUMBERS[] = "1234567890";
    static const char PHRASE[] = "The quick brown fox jumps over the lazy dog";
    static const char LONGPHRASE[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc omni virtuti vitium contrario nomine opponitur. "
    "Conferam tecum, quam cuique verso rem subicias; Te ipsum, dignissimum maioribus tuis, voluptasne induxit, ut adolescentulus eriperes "
    "P. Conclusum est enim contra Cyrenaicos satis acute, nihil ad Epicurum. Duo Reges: constructio interrete. Tum Torquatus: Prorsus, inquit, assentior;\n"
    "Quando enim Socrates, qui parens philosophiae iure dici potest, quicquam tale fecit? Sed quid sentiat, non videtis. Haec quo modo conveniant, non "
    "sane intellego. Sed ille, ut dixi, vitiose. Dic in quovis conventu te omnia facere, ne doleas. Quod si ita se habeat, non possit beatam praestare "
    "vitam sapientia. Quis suae urbis conservatorem Codrum, quis Erechthei filias non maxime laudat? Primum divisit ineleganter; Huic mori optimum esse "
    "propter desperationem sapientiae, illi propter spem vivere.";
    char VARSIZE[CHECK_SIZE];
    uint32_t VAR_CRC[CHECK_SIZE];
    
    for (int i = 0; i < CHECK_SIZE; i++) {
      VARSIZE[i] = i;
      VAR_CRC[i] = crc32cFinish(crc32cSlicingBy8(crc32cInit(), VARSIZE, size_t(i)));
    };


    for (int i = 0; i < NUM_VALID_FUNCTIONS; ++i) {
        EXPECT_TRUE(check(FNINFO[i], NUMBERS, 9, 0xE3069283));
        EXPECT_TRUE(check(FNINFO[i], NUMBERS+1, 8, 0xBFE92A83));
        EXPECT_TRUE(check(FNINFO[i], NUMBERS, 10, 0xf3dbd4fe));
        EXPECT_TRUE(check(FNINFO[i], PHRASE, sizeof(PHRASE)-1, 0x22620404));
	EXPECT_TRUE(check(FNINFO[i], LONGPHRASE, sizeof(LONGPHRASE)-1, 0xfcb7575a));
	for(int j = 0; j < CHECK_SIZE; j++) {
	  EXPECT_TRUE(check(FNINFO[i], VARSIZE, j, VAR_CRC[j]));
	};
    }
}

/*
static size_t misalignedLeadingBytes(const void* pointer, int alignment) {
    size_t misalignedBytes = (alignment - (intptr_t)pointer) & (alignment - 1);
    return misalignedBytes;
}
*/

template <typename T>
static T* alignBlock(T* pointer, int alignment) {
    return (T*) (((intptr_t) pointer + (alignment - 1)) & ~(alignment - 1));
}

TEST(CRC32C, Alignment) {
    // The 64-bit algorithms use 8 byte alignment
    static const int ALIGNMENT_BITS = 3;
    static const int ALIGNMENT_SIZE = 1 << ALIGNMENT_BITS;
    // Allocate 4 aligned blocks. We will drop the first if the allocation is not aligned
    static char BUFFER[ALIGNMENT_SIZE * 4];
    // Get the first aligned block in BUFFER
    static char* const ALIGNED_BUFFER = alignBlock(BUFFER, ALIGNMENT_SIZE);

    // Fill the buffer with non-zero data
    for (char* p = ALIGNED_BUFFER; p < ALIGNED_BUFFER + ALIGNMENT_SIZE * 3; ++p) {
        *p = (char)(p - ALIGNED_BUFFER);
    }

    /* This test uses 3 blocks of variable sizes:
    (leading misaligned bytes 0-7)(aligned block 0 or 8)(trailing misaligned bytes 0-7)
    
    We increment an integer to iterate through all posibilities and cross check all CRC
    implementations. */
    static const int MAX_ITERATIONS = 1 << (ALIGNMENT_BITS * 2 + 1);
    static const int LEADING_MASK = (ALIGNMENT_SIZE - 1) << (ALIGNMENT_BITS + 1);
    static const int ALIGN_MASK = 1 << ALIGNMENT_BITS;
    static const int TRAILING_MASK = (ALIGNMENT_SIZE - 1);
    static char* const ALIGNED_BLOCK = ALIGNED_BUFFER + ALIGNMENT_SIZE;
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        int leading = (i & LEADING_MASK) >> (ALIGNMENT_BITS + 1);
        int aligned = i & ALIGN_MASK;
        int trailing = i & TRAILING_MASK;
        //~ printf("%d = %d %d %d\n", i, leading, aligned, trailing);

        char* start = ALIGNED_BLOCK - leading;
        char* end = ALIGNED_BLOCK;
        if (aligned) end += ALIGNMENT_SIZE;
        end += trailing;
        //~ printf("start: %p end: %p; %d bytes\n", start, end, end - start);

        uint32_t crc = 0;
        for (int j = 0; j < NUM_VALID_FUNCTIONS; ++j) {
            uint32_t crcTemp = FNINFO[j].crcfn(crc32cInit(), start, end - start);
            crcTemp = crc32cFinish(crcTemp);
            if (j == 0) {
                crc = crcTemp;
            } else {
                if (crc != crcTemp) {
                    printf("Failed %s i = 0x%08x expected 0x%08x actual 0x%08x\n", FNINFO[j].name, i, crc, crcTemp);
                }
                EXPECT_EQ(crc, crcTemp);
            }
        }
    }
}

int main() {
    printf("Testing CRC32C functions\n");
    return TestSuite::globalInstance()->runAll();
}
